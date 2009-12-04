/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        sqliteapi.c
 *
 * Abstract:
 *
 *        Registry
 *
 *        Inter-process communication (Server) API for Users
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
 */
#include "includes.h"

static
PCWSTR
RegStrrchr(
    PCWSTR pwszStr,
    wchar16_t wch
    )
{
	int iIndex = RtlWC16StringNumChars(pwszStr);

	for (; iIndex >= 0 ; iIndex--)
	{
		if (pwszStr[iIndex] == wch)
		{
			return &pwszStr[iIndex];
		}
	}

	return NULL;
}

NTSTATUS
SqliteGetParentKeyName(
    PCWSTR pwszInputString,
    wchar16_t c,
    PWSTR *ppwszOutputString
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    //Do not free
    PCWSTR pwszFound = NULL;
    PWSTR pwszOutputString = NULL;

    BAIL_ON_NT_INVALID_STRING(pwszInputString);

    pwszFound = RegStrrchr(pwszInputString, c);
    if (pwszFound)
    {
        status = LW_RTL_ALLOCATE((PVOID*)&pwszOutputString, wchar16_t,
			                  sizeof(*pwszOutputString)* (pwszFound - pwszInputString +1));
        BAIL_ON_NT_STATUS(status);

        memcpy(pwszOutputString, pwszInputString,(pwszFound - pwszInputString) * sizeof(*pwszOutputString));
    }

    *ppwszOutputString = pwszOutputString;

cleanup:

    return status;

error:
    LWREG_SAFE_FREE_MEMORY(pwszOutputString);

    goto cleanup;
}

NTSTATUS
SqliteCreateKeyHandle(
    IN PREG_ENTRY pRegEntry,
    OUT PHKEY phkResult
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PREG_KEY_CONTEXT pKeyResult = NULL;

    BAIL_ON_INVALID_REG_ENTRY(pRegEntry);

    status = LW_RTL_ALLOCATE((PVOID*)&pKeyResult, REG_KEY_CONTEXT, sizeof(*pKeyResult));
    BAIL_ON_NT_STATUS(status);

    pKeyResult->refCount = 1;

    pthread_rwlock_init(&pKeyResult->mutex, NULL);
    pKeyResult->pMutex = &pKeyResult->mutex;

    status = LwRtlWC16StringDuplicate(&pKeyResult->pwszKeyName, pRegEntry->pwszKeyName);
    BAIL_ON_NT_STATUS(status);

    status = SqliteGetParentKeyName(pRegEntry->pwszKeyName, (wchar16_t)'\\',&pKeyResult->pwszParentKeyName);
    BAIL_ON_NT_STATUS(status);

    *phkResult = (HKEY)pKeyResult;

cleanup:
    return status;

error:
    RegSrvSafeFreeKeyContext(pKeyResult);
    *phkResult = (HKEY)NULL;

    goto cleanup;
}

/* Create a new key, if the key exists already,
 * open the existing key
 */
NTSTATUS
SqliteCreateKeyInternal(
    IN PWSTR pwszKeyName,
    IN OPTIONAL PCWSTR pSubKey, //pSubKey is null only when creating HKEY_LIKEWISE
    OUT OPTIONAL PHKEY ppKeyResult
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    PREG_ENTRY pRegEntry = NULL;
    PREG_KEY_CONTEXT pKeyResult = NULL;
    PWSTR pwszKeyNameWithSubKey = NULL;
    // Do not free
    PWSTR pwszFullKeyName = NULL;
    BOOLEAN bInLock = FALSE;
    PWSTR pwszParentKeyName = NULL;


    BAIL_ON_NT_INVALID_STRING(pwszKeyName); //parent key name

    if (pSubKey)
    {
        if (!RegSrvIsValidKeyName(pSubKey))
        {
		//invalid keyName passed in
		status = STATUS_OBJECT_NAME_INVALID;
            BAIL_ON_NT_STATUS(status);
        }

        status = LwRtlWC16StringAllocatePrintfW(
                        &pwszKeyNameWithSubKey,
                        L"%ws\\%ws",
                        pwszKeyName,
                        pSubKey);
        BAIL_ON_NT_STATUS(status);
    }

    pwszFullKeyName = pSubKey ? pwszKeyNameWithSubKey : pwszKeyName;

    LWREG_LOCK_MUTEX(bInLock, &gActiveKeyList.mutex);

    pKeyResult = SqliteCacheLocateActiveKey_inlock(pwszFullKeyName);
    if (pKeyResult)
    {
	status = STATUS_OBJECTID_EXISTS;
	BAIL_ON_NT_STATUS(status);
    }

	status = RegDbOpenKey(ghCacheConnection,
			              pwszFullKeyName,
						  &pRegEntry);
	if (!status)
	{
	status = STATUS_OBJECTID_EXISTS;
	BAIL_ON_NT_STATUS(status);
	}
	else if (STATUS_OBJECT_NAME_NOT_FOUND == status)
	{
		status = RegDbCreateKey(ghCacheConnection,
				                pwszFullKeyName,
								&pRegEntry);
		BAIL_ON_NT_STATUS(status);

		status = SqliteGetParentKeyName(pwszFullKeyName, (wchar16_t)'\\',&pwszParentKeyName);
		BAIL_ON_NT_STATUS(status);

		if (pwszParentKeyName)
		{
			SqliteCacheResetParentKeySubKeyInfo_inlock(pwszParentKeyName);
		}
	}
	BAIL_ON_NT_STATUS(status);

	status = SqliteCreateKeyHandle(pRegEntry, (PHKEY)&pKeyResult);
	BAIL_ON_NT_STATUS(status);

	// Cache this new key in gActiveKeyList
	status = SqliteCacheInsertActiveKey_inlock(pKeyResult);
	BAIL_ON_NT_STATUS(status);


    if (ppKeyResult)
    {
        *ppKeyResult = pKeyResult;
        pKeyResult = NULL;
    }
    else
    {
        SqliteCacheReleaseKey_inlock(pKeyResult);
    }

cleanup:
    LWREG_UNLOCK_MUTEX(bInLock, &gActiveKeyList.mutex);

    LWREG_SAFE_FREE_MEMORY(pwszParentKeyName);
    LWREG_SAFE_FREE_MEMORY(pwszKeyNameWithSubKey);
    RegDbSafeFreeEntry(&pRegEntry);

    return status;

error:
    SqliteCacheReleaseKey_inlock(pKeyResult);

    if (ppKeyResult)
    {
        *ppKeyResult = NULL;
    }

    goto cleanup;
}

/* Open a key, if not found,
 * do not create a new key */
NTSTATUS
SqliteOpenKeyInternal(
    IN PCWSTR pwszKeyName,
    IN OPTIONAL PCWSTR pSubKey,
    OUT PHKEY phkResult
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    PREG_ENTRY pRegEntry = NULL;
    PREG_KEY_CONTEXT pKeyResult = NULL;
    PWSTR pwszKeyNameWithSubKey = NULL;
    // Do not free
    PCWSTR pwszFullKeyName = NULL;
    BOOLEAN bInLock = FALSE;

    BAIL_ON_NT_INVALID_STRING(pwszKeyName);

    if (pSubKey)
    {
        status = LwRtlWC16StringAllocatePrintfW(
                        &pwszKeyNameWithSubKey,
                        L"%ws\\%ws",
                        pwszKeyName,
                        pSubKey);
        BAIL_ON_NT_STATUS(status);
    }

    pwszFullKeyName = pSubKey ? pwszKeyNameWithSubKey : pwszKeyName;

    LWREG_LOCK_MUTEX(bInLock, &gActiveKeyList.mutex);

    pKeyResult = SqliteCacheLocateActiveKey_inlock(pwszFullKeyName);
    if (!pKeyResult)
    {
        status = RegDbOpenKey(ghCacheConnection, pwszFullKeyName,&pRegEntry);
        BAIL_ON_NT_STATUS(status);

        status = SqliteCreateKeyHandle(pRegEntry,(PHKEY)&pKeyResult);
        BAIL_ON_NT_STATUS(status);

        // Cache this new key in gActiveKeyList
        status = SqliteCacheInsertActiveKey_inlock(pKeyResult);
        BAIL_ON_NT_STATUS(status);
    }

    *phkResult = (HKEY)pKeyResult;

cleanup:
    LWREG_UNLOCK_MUTEX(bInLock, &gActiveKeyList.mutex);
    RegDbSafeFreeEntry(&pRegEntry);
    LWREG_SAFE_FREE_MEMORY(pwszKeyNameWithSubKey);

    return status;

error:
    SqliteCacheReleaseKey_inlock(pKeyResult);
    pKeyResult = (HKEY)NULL;

    goto cleanup;
}

NTSTATUS
SqliteDeleteKeyInternal(
    IN PCWSTR pwszKeyName
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    size_t sSubkeyCount = 0;
    PWSTR pwszParentKeyName = NULL;


    status = RegDbOpenKey(ghCacheConnection, pwszKeyName, NULL);
    BAIL_ON_NT_STATUS(status);

    //Delete key from DB
    //Make sure this key does not have subkey before go ahead and delete it
    //Also need to delete the all of this subkey's values
    status = RegDbQueryInfoKeyCount(ghCacheConnection,
		                        pwszKeyName,
                                    QuerySubKeys,
                                    &sSubkeyCount);
    BAIL_ON_NT_STATUS(status);

    if (sSubkeyCount == 0)
    {
        //Delete all the values of this key
        status = RegDbDeleteKey(ghCacheConnection, pwszKeyName);
        BAIL_ON_NT_STATUS(status);

        status = SqliteGetParentKeyName(pwszKeyName, '\\',&pwszParentKeyName);
        BAIL_ON_NT_STATUS(status);

        if (!LW_IS_NULL_OR_EMPTY_STR(pwszParentKeyName))
        {
		SqliteCacheResetParentKeySubKeyInfo(pwszParentKeyName);
        }
    }
    else
    {
	status = STATUS_KEY_HAS_CHILDREN;
        BAIL_ON_NT_STATUS(status);
    }

cleanup:
    LWREG_SAFE_FREE_MEMORY(pwszParentKeyName);

    return status;

error:
    goto cleanup;
}

NTSTATUS
SqliteDeleteActiveKey(
    IN PCWSTR pwszKeyName
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PREG_KEY_CONTEXT pFoundKey = NULL;
    BOOLEAN bInLock = FALSE;

    LWREG_LOCK_MUTEX(bInLock, &gActiveKeyList.mutex);

    pFoundKey = SqliteCacheLocateActiveKey_inlock(pwszKeyName);
    if (pFoundKey)
    {
	status = STATUS_RESOURCE_IN_USE;
        BAIL_ON_NT_STATUS(status);
    }

cleanup:
    SqliteCacheReleaseKey_inlock(pFoundKey);
    LWREG_UNLOCK_MUTEX(bInLock, &gActiveKeyList.mutex);

    return status;

error:
    goto cleanup;
}

/*delete all subkeys and values of hKey*/
NTSTATUS
SqliteDeleteTreeInternal(
    IN HANDLE Handle,
    IN HKEY hKey
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    HKEY hCurrKey = NULL;
    int iCount = 0;
    DWORD dwSubKeyCount = 0;
    LW_WCHAR psubKeyName[MAX_KEY_LENGTH];
    DWORD dwSubKeyLen = 0;
    PSTR* ppszSubKey = NULL;
    //Do not free
    PSTR pszSubKeyName = NULL;
    PWSTR pwszSubKey = NULL;

    status = SqliteQueryInfoKey(Handle,
                                 hKey,
                                 NULL,
                                 NULL,
                                 NULL,
                                 &dwSubKeyCount,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL);
    BAIL_ON_NT_STATUS(status);

    if (dwSubKeyCount)
    {
        status = LW_RTL_ALLOCATE((PVOID*)&ppszSubKey, PSTR, sizeof(*ppszSubKey) * dwSubKeyCount);
        BAIL_ON_NT_STATUS(status);
    }

    for (iCount = 0; iCount < dwSubKeyCount; iCount++)
    {
        dwSubKeyLen = MAX_KEY_LENGTH;
        memset(psubKeyName, 0, MAX_KEY_LENGTH);

        status = SqliteEnumKeyEx(Handle,
                                  hKey,
                                  iCount,
                                  psubKeyName,
                                  &dwSubKeyLen,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL);
        BAIL_ON_NT_STATUS(status);

	status = LwRtlCStringAllocateFromWC16String(&ppszSubKey[iCount], psubKeyName);
        BAIL_ON_NT_STATUS(status);
    }

    for (iCount = 0; iCount < dwSubKeyCount; iCount++)
    {
        pszSubKeyName = strrchr(ppszSubKey[iCount], '\\');

        if (LW_IS_NULL_OR_EMPTY_STR(pszSubKeyName))
        {
            status = STATUS_INTERNAL_ERROR;
            BAIL_ON_NT_STATUS(status);
        }

	status = LwRtlWC16StringAllocateFromCString(&pwszSubKey,
			                                     pszSubKeyName+1);
	BAIL_ON_NT_STATUS(status);

        status = SqliteOpenKeyEx(Handle,
                                  hKey,
                                  pwszSubKey,
                                  0,
                                  0,
                                  &hCurrKey);
        BAIL_ON_NT_STATUS(status);

        status = SqliteDeleteTreeInternal(Handle,
                                           hCurrKey);
        BAIL_ON_NT_STATUS(status);

        if (hCurrKey)
        {
            SqliteCloseKey(hCurrKey);
        }

        status = SqliteDeleteKey(Handle, hKey, pwszSubKey);
        BAIL_ON_NT_STATUS(status);

        LWREG_SAFE_FREE_MEMORY(pwszSubKey);
        hCurrKey = NULL;
    }

cleanup:
    if (hCurrKey)
    {
        SqliteCloseKey(hCurrKey);
    }
    RegFreeStringArray(ppszSubKey, dwSubKeyCount);
    LWREG_SAFE_FREE_MEMORY(pwszSubKey);

    return status;


error:
    goto cleanup;
}
