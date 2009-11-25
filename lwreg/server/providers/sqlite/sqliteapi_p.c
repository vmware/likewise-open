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

NTSTATUS
SqliteGetParentKeyName(
    PCSTR pszInputString,
    CHAR c,
    PSTR *ppszOutputString
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    //Do not free
    PSTR pszFound = NULL;
    PSTR pszOutputString = NULL;

    BAIL_ON_NT_INVALID_STRING(pszInputString);

    pszFound = strrchr(pszInputString, c);
    if (pszFound)
    {
        status = LW_RTL_ALLOCATE((PVOID*)&pszOutputString, CHAR,
			                  sizeof(*pszOutputString)* (pszFound - pszInputString +1));
        BAIL_ON_NT_STATUS(status);

        strncpy(pszOutputString,pszInputString,(pszFound - pszInputString));
    }

    *ppszOutputString = pszOutputString;

cleanup:

    return status;

error:
    LWREG_SAFE_FREE_STRING(pszOutputString);

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

    status = LwRtlCStringDuplicate(&pKeyResult->pszKeyName,
		                        pRegEntry->pszKeyName);
    BAIL_ON_NT_STATUS(status);

    status = SqliteGetParentKeyName(pKeyResult->pszKeyName, '\\',&pKeyResult->pszParentKeyName);
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
    IN PSTR pszKeyName,
    IN OPTIONAL PCWSTR pSubKey, //pSubKey is null only when creating HKEY_LIKEWISE
    OUT OPTIONAL PHKEY ppKeyResult
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    PREG_ENTRY pRegEntry = NULL;
    PREG_KEY_CONTEXT pKeyResult = NULL;
    PSTR pszSubKey = NULL;
    PSTR pszKeyNameWithSubKey = NULL;
    PSTR pszParentKeyName = NULL;
    // Do not free
    PSTR pszFullKeyName = NULL;
    BOOLEAN bInLock = FALSE;

    BAIL_ON_NT_INVALID_STRING(pszKeyName); //parent key name

    if (pSubKey)
    {
	status = LwRtlCStringAllocateFromWC16String(&pszSubKey, pSubKey);
        BAIL_ON_NT_STATUS(status);

        if (!RegSrvIsValidKeyName(pszSubKey))
        {
		//invalid keyName passed in
		status = STATUS_OBJECT_NAME_INVALID;
            BAIL_ON_NT_STATUS(status);
        }

        status = LwRtlCStringAllocatePrintf(
                    &pszKeyNameWithSubKey,
                    "%s\\%s",
                    pszKeyName,
                    pszSubKey);
        BAIL_ON_NT_STATUS(status);
    }

    pszFullKeyName = pSubKey ? pszKeyNameWithSubKey : pszKeyName;

    LWREG_LOCK_MUTEX(bInLock, &gActiveKeyList.mutex);

    pKeyResult = SqliteCacheLocateActiveKey_inlock(pszFullKeyName);
    if (!pKeyResult)
    {
	status = RegDbOpenKey(ghCacheConnection,
                               pszFullKeyName,
                               &pRegEntry);
        if (STATUS_OBJECT_NAME_NOT_FOUND == status)
        {
		status = RegDbCreateKey(ghCacheConnection,
                                     pszFullKeyName,
                                     &pRegEntry);
            BAIL_ON_NT_STATUS(status);

            status = SqliteGetParentKeyName(pszFullKeyName, '\\',&pszParentKeyName);
            BAIL_ON_NT_STATUS(status);

            if (pszParentKeyName)
            {
                SqliteCacheResetParentKeySubKeyInfo_inlock(pszParentKeyName);
            }
        }
        BAIL_ON_NT_STATUS(status);

        status = SqliteCreateKeyHandle(pRegEntry, (PHKEY)&pKeyResult);
        BAIL_ON_NT_STATUS(status);

        // Cache this new key in gActiveKeyList
        status = SqliteCacheInsertActiveKey_inlock(pKeyResult);
        BAIL_ON_NT_STATUS(status);
    }

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

    LWREG_SAFE_FREE_STRING(pszSubKey);
    LWREG_SAFE_FREE_STRING(pszKeyNameWithSubKey);
    LWREG_SAFE_FREE_STRING(pszParentKeyName);
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
    IN PSTR pszKeyName,
    IN OPTIONAL PCWSTR pSubKey,
    OUT PHKEY phkResult
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    PREG_ENTRY pRegEntry = NULL;
    PREG_KEY_CONTEXT pKeyResult = NULL;
    PSTR pszSubKey = NULL;
    PSTR pszKeyNameWithSubKey = NULL;
    // Do not free
    PSTR pszFullKeyName = NULL;
    BOOLEAN bInLock = FALSE;

    BAIL_ON_NT_INVALID_STRING(pszKeyName);

    if (pSubKey)
    {
	status = LwRtlCStringAllocateFromWC16String(&pszSubKey, pSubKey);
        BAIL_ON_NT_STATUS(status);

        status = LwRtlCStringAllocatePrintf(
                    &pszKeyNameWithSubKey,
                    "%s\\%s",
                    pszKeyName,
                    pszSubKey);
        BAIL_ON_NT_STATUS(status);
    }

    pszFullKeyName = pSubKey ? pszKeyNameWithSubKey : pszKeyName;

    LWREG_LOCK_MUTEX(bInLock, &gActiveKeyList.mutex);

    pKeyResult = SqliteCacheLocateActiveKey_inlock(pszFullKeyName);
    if (!pKeyResult)
    {
        status = RegDbOpenKey(ghCacheConnection, pszFullKeyName,&pRegEntry);
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
    LWREG_SAFE_FREE_STRING(pszSubKey);
    LWREG_SAFE_FREE_STRING(pszKeyNameWithSubKey);

    return status;

error:
    SqliteCacheReleaseKey_inlock(pKeyResult);
    pKeyResult = (HKEY)NULL;

    goto cleanup;
}

NTSTATUS
SqliteDeleteKeyInternal(
    IN PSTR pszKeyName
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    size_t sSubkeyCount = 0;
    PSTR pszParentKeyName = NULL;

    status = RegDbOpenKey(ghCacheConnection, pszKeyName, NULL);
    BAIL_ON_NT_STATUS(status);

    //Delete key from DB
    //Make sure this key does not have subkey before go ahead and delete it
    //Also need to delete the all of this subkey's values
    status = RegDbQueryInfoKeyCount(ghCacheConnection,
                                     pszKeyName,
                                     QuerySubKeys,
                                     &sSubkeyCount);
    BAIL_ON_NT_STATUS(status);

    if (sSubkeyCount == 0)
    {
        //Delete all the values of this key
        status = RegDbDeleteKey(ghCacheConnection, pszKeyName);
        BAIL_ON_NT_STATUS(status);

        status = SqliteGetParentKeyName(pszKeyName, '\\',&pszParentKeyName);
        BAIL_ON_NT_STATUS(status);

        if (!LW_IS_NULL_OR_EMPTY_STR(pszParentKeyName))
        {
            SqliteCacheResetParentKeySubKeyInfo(pszParentKeyName);
        }
    }
    else
    {
	status = STATUS_KEY_HAS_CHILDREN;
        BAIL_ON_NT_STATUS(status);
    }

cleanup:
    LWREG_SAFE_FREE_STRING(pszParentKeyName);

    return status;

error:
    goto cleanup;
}

NTSTATUS
SqliteDeleteActiveKey(
    IN PSTR pszKeyName
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PREG_KEY_CONTEXT pFoundKey = NULL;
    BOOLEAN bInLock = FALSE;

    LWREG_LOCK_MUTEX(bInLock, &gActiveKeyList.mutex);

    pFoundKey = SqliteCacheLocateActiveKey_inlock(pszKeyName);
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
