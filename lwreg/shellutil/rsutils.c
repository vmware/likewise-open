/*
 * Copyright Likewise Software    2004-2009
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        export.c
 *
 * Abstract:
 *
 *        Registry
 *
 *        Registry utility functions for regshell
 *
 * Authors: Wei Fu (wfu@likewise.com)
 *          Adam Bernstein (abernstein@likewise.com)
 *
 */
#include "rsutils.h"

DWORD
RegShellCanonicalizePath(
    PSTR pszInDefaultKey,
    PSTR pszInKeyName,
    PSTR *ppszFullPath,
    PSTR *ppszParentPath,
    PSTR *ppszSubKey)
{
    DWORD dwError = 0;
    DWORD dwFullPathLen = 0;
    PSTR pszNewPath = NULL;
    PSTR pszToken = NULL;
    PSTR pszStrtokState = NULL;
    PSTR pszTmp = NULL;
    PSTR pszStrtokValue = NULL;

    /*
     * Path is already fully qualified, so just return KeyName as the full path
     */
    if (pszInKeyName && (pszInKeyName[0] == '\\'))
    {
        dwError = LwRtlCStringDuplicate(&pszNewPath, pszInKeyName);
        BAIL_ON_REG_ERROR(dwError);
    }
    else if (pszInKeyName || pszInDefaultKey)
    {
        if (pszInDefaultKey)
        {
            dwFullPathLen += strlen(pszInDefaultKey);
        }
        if (pszInKeyName)
        {
            dwFullPathLen += strlen(pszInKeyName);
        }

        /* Leading and separating \ and \0 characters */
        dwFullPathLen += 3;

        dwError = LW_RTL_ALLOCATE((PVOID*)&pszNewPath, CHAR, sizeof(*pszNewPath) * dwFullPathLen);
        BAIL_ON_REG_ERROR(dwError);

        if (pszInDefaultKey)
        {
            strcat(pszNewPath, "\\");
            strcat(pszNewPath, pszInDefaultKey);
        }
        if (pszInKeyName)
        {
            /* Copy of pszInKeyName because strtok_r modifies this value */
            dwError = LwRtlCStringDuplicate(&pszStrtokValue, pszInKeyName);
            BAIL_ON_REG_ERROR(dwError);

            pszToken = strtok_r(pszStrtokValue, "\\", &pszStrtokState);
            while (pszToken)
            {
                if (strcmp(pszToken, ".") == 0)
                {
                    /* Do nothing */
                }
                else if (strcmp(pszToken, "..") == 0)
                {
                    /*
                     * Subtract one directory off the NewPath being
                     * constructed
                     */
                    pszTmp = strrchr(pszNewPath, '\\');
                    if (pszTmp)
                    {
                        *pszTmp = '\0';
                    }
                    else if (strlen(pszNewPath) > 0)
                    {
                        *pszNewPath = '\0';
                    }
                }
                else if (strncmp(pszToken, "...", 3) == 0)
                {
                    dwError = LWREG_ERROR_INVALID_CONTEXT;
                    BAIL_ON_REG_ERROR(dwError);
                }
                else
                {
                    /*
                     * Append token to end of newPath
                     */
                    strcat(pszNewPath, "\\");
                    strcat(pszNewPath, pszToken);
                }
                pszToken = strtok_r(NULL, "\\/", &pszStrtokState);
            }
        }
    }
    else if (!pszInDefaultKey)
    {
        dwError = LwRtlCStringDuplicate(&pszNewPath, "\\");
        BAIL_ON_REG_ERROR(dwError);
    }

    if (strlen(pszNewPath) == 0)
    {
        strcpy(pszNewPath, "\\");
    }
#ifdef _LW_DEBUG
    printf("CanonicalizePath: pszNewPath='%s'\n", pszNewPath);
#endif
    if (ppszParentPath)
    {
        dwError = LwRtlCStringDuplicate(ppszParentPath, pszNewPath);
        BAIL_ON_REG_ERROR(dwError);

        pszTmp = strrchr(*ppszParentPath, '\\');
        if (pszTmp)
        {
            if (pszTmp == *ppszParentPath)
            {
                pszTmp[1] = '\0';
            }
            else
            {
                pszTmp[0] = '\0';
            }
        }
    }
    if (ppszSubKey)
    {
        pszTmp = strrchr(pszNewPath, '\\');
        if (pszTmp)
        {
            dwError = LwRtlCStringDuplicate(ppszSubKey, pszTmp+1);
            BAIL_ON_REG_ERROR(dwError);
        }
        else
        {
            dwError = LwRtlCStringDuplicate(ppszSubKey, "");
            BAIL_ON_REG_ERROR(dwError);
        }

    }
    if (ppszFullPath)
    {
        *ppszFullPath = pszNewPath;
    }

cleanup:
    LWREG_SAFE_FREE_MEMORY(pszStrtokValue);
    return dwError;

error:
    goto cleanup;
}



DWORD
RegShellIsValidKey(
    HANDLE hReg,
    PSTR pszRootKeyName,
    PSTR pszKey)
{
    DWORD dwError = 0;
    HKEY pFullKey = NULL;
    HKEY pRootKey = NULL;
    PWSTR pSubKey = NULL;

    BAIL_ON_INVALID_HANDLE(hReg);
    BAIL_ON_INVALID_POINTER(pszKey);

    if (!pszRootKeyName)
    {
        pszRootKeyName = HKEY_THIS_MACHINE;
    }
    dwError = RegOpenKeyExA(hReg, NULL, pszRootKeyName, 0, 0, &pRootKey);
    BAIL_ON_REG_ERROR(dwError);

	dwError = LwRtlWC16StringAllocateFromCString(&pSubKey, pszKey);
	BAIL_ON_REG_ERROR(dwError);

    dwError = RegOpenKeyExW(
        hReg,
        pRootKey,
        pSubKey,
        0,
        0,
        &pFullKey);
    BAIL_ON_REG_ERROR(dwError);

cleanup:

    LWREG_SAFE_FREE_MEMORY(pSubKey);
    if (pFullKey && pFullKey != pRootKey)
    {
        RegCloseKey(hReg, pFullKey);
    }
    return dwError;

error:
    goto cleanup;
}

DWORD
RegShellUtilAddKey(
    HANDLE hReg,
    PSTR pszRootKeyName,
    PSTR pszDefaultKey,
    PSTR pszKeyName)
{
    HANDLE hRegLocal = NULL;
    HKEY pNextKey = NULL;
    HKEY pRootKey = NULL;
    HKEY pCurrentKey = NULL; //key that to be performed more operations on
    PWSTR pwszSubKey = NULL;
    PSTR pszToken = NULL;
    PSTR pszStrtokState = NULL;
    PSTR pszDelim = "\\";
    PSTR pszFullPath = NULL;
    PSTR pszSubKey = NULL;

    DWORD dwError = 0;

    if (!hReg)
    {
        dwError = RegOpenServer(&hRegLocal);
        BAIL_ON_REG_ERROR(dwError);
        hReg = hRegLocal;
    }

    if (!pszRootKeyName)
    {
        pszRootKeyName = HKEY_THIS_MACHINE;
    }
    dwError = RegShellCanonicalizePath(pszDefaultKey,
                                       pszKeyName,
                                       &pszFullPath,
                                       NULL,
                                       &pszSubKey);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegOpenKeyExA(hReg, NULL, pszRootKeyName, 0, 0, &pRootKey);
    BAIL_ON_REG_ERROR(dwError);

    pCurrentKey = pRootKey;
    pszToken = strtok_r(pszFullPath, pszDelim, &pszStrtokState);
    while (!LW_IS_NULL_OR_EMPTY_STR(pszToken))
    {
	dwError = LwRtlWC16StringAllocateFromCString(&pwszSubKey, pszToken);
	BAIL_ON_REG_ERROR(dwError);

        dwError = RegCreateKeyExW(
            hReg,
            pCurrentKey,
            pwszSubKey,
            0,
            NULL,
            0,
            0,
            NULL,
            &pNextKey,
            NULL);
        BAIL_ON_REG_ERROR(dwError);

        LWREG_SAFE_FREE_MEMORY(pwszSubKey);

        if (pCurrentKey)
        {
          dwError = RegCloseKey(hReg, pCurrentKey);
          BAIL_ON_REG_ERROR(dwError);
          pCurrentKey = NULL;
        }

        pCurrentKey = pNextKey;

        pszToken = strtok_r (NULL, pszDelim, &pszStrtokState);
    }

cleanup:
    RegCloseServer(hRegLocal);
    if (pCurrentKey)
    {
        RegCloseKey(hReg, pCurrentKey);
    }
    return dwError;

error:
    LWREG_SAFE_FREE_MEMORY(pwszSubKey);
    goto cleanup;
}


DWORD
RegShellUtilDeleteKey(
    HANDLE hReg,
    PSTR pszRootKeyName,
    PSTR pszDefaultKey,
    PSTR keyName)
{
    HANDLE hRegLocal = NULL;
    PWSTR pwszSubKey = NULL;
    HKEY pCurrentKey = NULL;
    DWORD dwError = 0;
    PSTR pszFullPath = NULL;
    PSTR pszParentPath = NULL;
    PSTR pszSubKey = NULL;
    HKEY pRootKey = NULL;

    if (!hReg)
    {
        dwError = RegOpenServer(&hRegLocal);
        BAIL_ON_REG_ERROR(dwError);
        hReg = hRegLocal;
    }


    if (!pszRootKeyName)
    {
        pszRootKeyName = HKEY_THIS_MACHINE;
    }
    dwError = RegShellCanonicalizePath(pszDefaultKey,
                                       keyName,
                                       &pszFullPath,
                                       &pszParentPath,
                                       &pszSubKey);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegShellIsValidKey(hReg, pszRootKeyName, pszFullPath+1);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegOpenKeyExA(hReg, NULL, pszRootKeyName, 0, 0, &pRootKey);
    BAIL_ON_REG_ERROR(dwError);

    pCurrentKey = pRootKey;
    if (pszParentPath && pszParentPath[1])
    {
	dwError = LwRtlWC16StringAllocateFromCString(&pwszSubKey, pszParentPath+1);
	BAIL_ON_REG_ERROR(dwError);

        dwError = RegOpenKeyExW(
                      hReg,
                      pRootKey,
                      pwszSubKey,
                      0,
                      0,
                      &pCurrentKey);
        BAIL_ON_REG_ERROR(dwError);
        LWREG_SAFE_FREE_MEMORY(pwszSubKey);
    }

	dwError = LwRtlWC16StringAllocateFromCString(&pwszSubKey, pszSubKey);
	BAIL_ON_REG_ERROR(dwError);

    dwError = RegDeleteKeyW(hReg, pCurrentKey, pwszSubKey);
    BAIL_ON_REG_ERROR(dwError);

cleanup:
    RegCloseServer(hRegLocal);
    if (pCurrentKey)
    {
        RegCloseKey(hReg, pCurrentKey);
    }
    LWREG_SAFE_FREE_MEMORY(pwszSubKey);
    return dwError;

error:
    goto cleanup;
}


DWORD
RegShellUtilDeleteTree(
    HANDLE hReg,
    PSTR pszRootKeyName,
    PSTR pszDefaultKey,
    PSTR keyName)
{
    HANDLE hRegLocal = NULL;
    PWSTR pwszSubKey = NULL;
    HKEY pCurrentKey = NULL;
    DWORD dwError = 0;
    PSTR pszFullPath = NULL;
    PSTR pszParentPath = NULL;
    PSTR pszSubKey = NULL;
    HKEY pRootKey = NULL;

    if (!hReg)
    {
        dwError = RegOpenServer(&hRegLocal);
        BAIL_ON_REG_ERROR(dwError);
        hReg = hRegLocal;
    }


    if (!pszRootKeyName)
    {
        pszRootKeyName = HKEY_THIS_MACHINE;
    }
    dwError = RegShellCanonicalizePath(pszDefaultKey,
                                       keyName,
                                       &pszFullPath,
                                       &pszParentPath,
                                       &pszSubKey);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegShellIsValidKey(hReg, pszRootKeyName, pszFullPath+1);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegOpenKeyExA(hReg, NULL, pszRootKeyName, 0, 0, &pRootKey);
    BAIL_ON_REG_ERROR(dwError);

    pCurrentKey = pRootKey;
    if (pszParentPath && pszParentPath[1])
    {
	dwError = LwRtlWC16StringAllocateFromCString(&pwszSubKey, pszParentPath+1);
	BAIL_ON_REG_ERROR(dwError);

        dwError = RegOpenKeyExW(
                      hReg,
                      pRootKey,
                      pwszSubKey,
                      0,
                      0,
                      &pCurrentKey);
        BAIL_ON_REG_ERROR(dwError);
        LWREG_SAFE_FREE_MEMORY(pwszSubKey);
    }

	dwError = LwRtlWC16StringAllocateFromCString(&pwszSubKey, pszSubKey);
	BAIL_ON_REG_ERROR(dwError);

    dwError = RegDeleteTreeW(hReg, pCurrentKey, pwszSubKey);
    BAIL_ON_REG_ERROR(dwError);

cleanup:
    RegCloseServer(hRegLocal);
    if (pCurrentKey)
    {
        RegCloseKey(hReg, pCurrentKey);
    }
    LWREG_SAFE_FREE_MEMORY(pwszSubKey);
    return dwError;

error:
    goto cleanup;
}

DWORD
RegShellUtilGetKeys(
    HANDLE hReg,
    PSTR pszRootKeyName,
    PSTR pszDefaultKey,
    PSTR keyName,
    LW_WCHAR ***pppRetSubKeys,
    PDWORD pdwRetSubKeyCount)
{
    HANDLE hRegLocal = NULL;
    HKEY pRootKey = NULL;
    DWORD dwError = 0;
    DWORD dwSubKeyCount = 0;
    DWORD dwValuesCount = 0;
    DWORD dwMaxSubKeyLen = 0;
    DWORD i = 0;
    DWORD dwSubKeyLen = MAX_KEY_LENGTH;
    LW_WCHAR **subKeys = NULL;
    HKEY pFullKey = NULL;
    PSTR pszParentPath = NULL;
    PWSTR pwszSubKey = NULL;
    PSTR pszKeyName = NULL;

    if (!hReg)
    {
        dwError = RegOpenServer(&hRegLocal);
        BAIL_ON_REG_ERROR(dwError);
        hReg = hRegLocal;
    }


    if (!pszRootKeyName)
    {
        return RegEnumRootKeysW(hReg,
                                pppRetSubKeys,
                                pdwRetSubKeyCount);
    }

    dwError = RegShellCanonicalizePath(pszDefaultKey,
                                       keyName,
                                       &pszParentPath,
                                       NULL,
                                       NULL);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegOpenKeyExA(hReg, NULL, pszRootKeyName, 0, 0, &pRootKey);
    BAIL_ON_REG_ERROR(dwError);

    if (pszParentPath && pszParentPath[1])
    {
	dwError = LwRtlWC16StringAllocateFromCString(&pwszSubKey, pszParentPath+1);
	BAIL_ON_REG_ERROR(dwError);

        dwError = RegOpenKeyExW(
                      hReg,
                      pRootKey,
                      pwszSubKey,
                      0,
                      0,
                      &pFullKey);
        BAIL_ON_REG_ERROR(dwError);
        LWREG_SAFE_FREE_MEMORY(pwszSubKey);
    }
    else
    {
        pFullKey = pRootKey;
    }

    dwError = RegQueryInfoKeyA(
        hReg,
        pFullKey,
        NULL,
        NULL,
        NULL,
        &dwSubKeyCount,
        &dwMaxSubKeyLen,
        NULL,
        &dwValuesCount,
        NULL,
        NULL,
        NULL,
        NULL);
    BAIL_ON_REG_ERROR(dwError);

    if (!dwSubKeyCount)
    {
	goto done;
    }

    dwError = LW_RTL_ALLOCATE((PVOID*)&subKeys, PWSTR, sizeof(*subKeys) * dwSubKeyCount);
    BAIL_ON_REG_ERROR(dwError);

#ifdef _LW_DEBUG
    printf( "\nNumber of subkeys: %d\n", dwSubKeyCount);
#endif

    for (i = 0; i < dwSubKeyCount; i++)
    {
        dwSubKeyLen = dwMaxSubKeyLen+1;

        dwError = LW_RTL_ALLOCATE((PVOID*)&pszKeyName, CHAR, sizeof(*pszKeyName) * dwSubKeyLen);
        BAIL_ON_REG_ERROR(dwError);

        dwError = RegEnumKeyExA((HANDLE)hReg,
                                pFullKey,
                                i,
                                pszKeyName,
                                &dwSubKeyLen,
                                NULL,
                                NULL,
                                NULL,
                                NULL);
        BAIL_ON_REG_ERROR(dwError);

	dwError = LwRtlWC16StringAllocateFromCString(&subKeys[i], pszKeyName);
	BAIL_ON_REG_ERROR(dwError);

        LWREG_SAFE_FREE_STRING(pszKeyName);
        pszKeyName = NULL;
    }

done:

    *pppRetSubKeys = subKeys;
    *pdwRetSubKeyCount = dwSubKeyCount;

cleanup:
    RegCloseServer(hRegLocal);
    if (pFullKey && pFullKey != pRootKey)
    {
        RegCloseKey(hReg, pFullKey);
    }
    LWREG_SAFE_FREE_STRING(pszKeyName);

    return dwError;

error:
    for (i = 0; i < dwSubKeyCount; i++)
    {
        LWREG_SAFE_FREE_MEMORY(subKeys[i]);
    }
    LWREG_SAFE_FREE_MEMORY(subKeys);
    goto cleanup;
}

DWORD
RegShellUtilSetValue(
    HANDLE hReg,
    PSTR pszRootKeyName,
    PSTR pszDefaultKey,
    PSTR keyName,
    PSTR valueName,
    REG_DATA_TYPE type,
    LW_PVOID data,
    DWORD dataLen
    )
{
    HANDLE hRegLocal = NULL;
    DWORD dwError = 0;
    PBYTE pData = NULL;
    SSIZE_T dwDataLen = 0;
    PWSTR pwszParentPath = NULL;
    HKEY pFullKey = NULL;
    HKEY pRootKey = NULL;
    PSTR pszParentPath = NULL;

    if (!hReg)
    {
        dwError = RegOpenServer(&hRegLocal);
        BAIL_ON_REG_ERROR(dwError);
        hReg = hRegLocal;
    }


    if (!pszRootKeyName)
    {
        pszRootKeyName = HKEY_THIS_MACHINE;
    }
    dwError = RegShellCanonicalizePath(pszDefaultKey,
                                       keyName,
                                       &pszParentPath,
                                       NULL,
                                       NULL);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegOpenKeyExA(hReg, NULL, pszRootKeyName, 0, 0, &pRootKey);
    BAIL_ON_REG_ERROR(dwError);

    pFullKey = pRootKey;
    if (pszParentPath && pszParentPath[1])
    {
	dwError = LwRtlWC16StringAllocateFromCString(&pwszParentPath, pszParentPath+1);
	BAIL_ON_REG_ERROR(dwError);

        dwError = RegOpenKeyExW(
                      hReg,
                      pRootKey,
                      pwszParentPath,
                      0,
                      0,
                      &pFullKey);
        BAIL_ON_REG_ERROR(dwError);
        LWREG_SAFE_FREE_MEMORY(pwszParentPath);
    }

    switch (type)
    {
        case REG_MULTI_SZ:
            dwError = RegMultiStrsToByteArrayA(
                                        data,
                                        &pData,
                                        &dwDataLen);
            BAIL_ON_REG_ERROR(dwError);
            break;

        case REG_SZ:
            dwError = LwRtlCStringDuplicate((LW_PVOID) &pData, data ? data : "");
            BAIL_ON_REG_ERROR(dwError);

            dwDataLen = strlen((PSTR) pData)+1;
            break;

        case REG_DWORD:
            dwError = LW_RTL_ALLOCATE((PVOID*)&pData, BYTE, sizeof(*pData) * sizeof(DWORD));
            BAIL_ON_REG_ERROR(dwError);

            dwDataLen = (SSIZE_T) dataLen;
            memcpy(pData, data, dwDataLen);
            break;

        case REG_BINARY:
            pData = data;
            dwDataLen = (SSIZE_T) dataLen;
            break;

        default:
            printf("RegShellSetValue: unknown type %d\n", type);
            break;
    }

    dwError = RegSetValueExA(
        hReg,
        pFullKey,
        valueName,
        0,
        type,
        pData,
        dwDataLen);
    BAIL_ON_REG_ERROR(dwError);
    if (type != REG_BINARY)
    {
        LWREG_SAFE_FREE_MEMORY(pData);
    }

cleanup:
    RegCloseServer(hRegLocal);
    LWREG_SAFE_FREE_MEMORY(pwszParentPath);
    if (pFullKey && pFullKey != pRootKey)
    {
        RegCloseKey(hReg, pFullKey);
    }
    return dwError;

error:
    goto cleanup;
}

DWORD
RegShellUtilGetValues(
    HANDLE hReg,
    PSTR pszRootKeyName,
    PSTR pszDefaultKey,
    PSTR keyName,
    PREGSHELL_UTIL_VALUE *valueArray,
    PDWORD pdwValueArrayLen
    )
{
    HANDLE hRegLocal = NULL;
    DWORD dwError = 0;
    DWORD indx = 0;
    PSTR pszValueName = NULL;
    DWORD dwValueNameLen = 0;
    DWORD regType = 0;
    PBYTE pData = NULL;
    DWORD dwDataLen = 0;
    PWSTR pSubKey = NULL;
    HKEY pRootKey = NULL;
    HKEY pFullKey = NULL;
    PSTR pszParentPath = NULL;
    PWSTR pwszParentPath = NULL;
    DWORD dwValuesCount = 0;
    PREGSHELL_UTIL_VALUE pValueArray = NULL;
    DWORD dwMaxValueNameLen = 0;
    DWORD dwMaxValueLen = 0;

    if (!hReg)
    {
        dwError = RegOpenServer(&hRegLocal);
        BAIL_ON_REG_ERROR(dwError);
        hReg = hRegLocal;
    }


    if (!pszRootKeyName)
    {
        return 0;
    }
    dwError = RegShellCanonicalizePath(pszDefaultKey,
                                       keyName,
                                       &pszParentPath,
                                       NULL,
                                       NULL);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegOpenKeyExA(hReg, NULL, pszRootKeyName, 0, 0, &pRootKey);
    BAIL_ON_REG_ERROR(dwError);

    pFullKey = pRootKey;
    if (pszParentPath && strcmp(pszParentPath, "\\") != 0)
    {
	dwError = LwRtlWC16StringAllocateFromCString(&pwszParentPath, pszParentPath+1);
	BAIL_ON_REG_ERROR(dwError);

        dwError = RegOpenKeyExW(
                      hReg,
                      pRootKey,
                      pwszParentPath,
                      0,
                      0,
                      &pFullKey);
        BAIL_ON_REG_ERROR(dwError);
        LWREG_SAFE_FREE_MEMORY(pwszParentPath);
    }

    dwError = RegQueryInfoKeyA(
                  hReg,
                  pFullKey,
                  NULL,
                  NULL,
                  NULL,
                  NULL,
                  NULL,
                  NULL,
                  &dwValuesCount,
                  &dwMaxValueNameLen,
                  &dwMaxValueLen,
                  NULL,
                  NULL);
    BAIL_ON_REG_ERROR(dwError);

    if (!dwValuesCount)
    {
	goto done;
    }

    dwError = LW_RTL_ALLOCATE((PVOID*)&pValueArray, REGSHELL_UTIL_VALUE, sizeof(*pValueArray) * dwValuesCount);
    BAIL_ON_REG_ERROR(dwError);

    /*
     * Apparently the data size is not returned in bytes for REG_SZ
     * which is why this adjustment is needed.
     */
    dwMaxValueNameLen +=1;

    for (indx = 0; indx < dwValuesCount; indx++)
    {
        /*
         * TBD/adam
         * Add wide character NULL size here; bug in RegEnumValueA()?
         */
        dwValueNameLen = dwMaxValueNameLen;

        dwError = LW_RTL_ALLOCATE((PVOID*)&pszValueName, CHAR, sizeof(*pszValueName) * dwValueNameLen);
        BAIL_ON_REG_ERROR(dwError);

        dwDataLen = dwMaxValueLen;

        dwError = LW_RTL_ALLOCATE((PVOID*)&pData, BYTE, dwDataLen);
        BAIL_ON_REG_ERROR(dwError);

        dwError = RegEnumValueA(
                      hReg,
                      pFullKey,
                      indx,
                      pszValueName,
                      &dwValueNameLen,
                      NULL,
                      &regType,
                      pData,
                      &dwDataLen);
        BAIL_ON_REG_ERROR(dwError);

	dwError = LwRtlWC16StringAllocateFromCString(&pValueArray[indx].pValueName, pszValueName+1);
	BAIL_ON_REG_ERROR(dwError);

        pValueArray[indx].type = regType;
        pValueArray[indx].pData = pData;
        pData = NULL;
        pValueArray[indx].dwDataLen = dwDataLen;
    }

 done:

    *valueArray = pValueArray;
    *pdwValueArrayLen = indx;

cleanup:
    RegCloseServer(hRegLocal);
    if (pFullKey && pFullKey != pRootKey)
    {
        RegCloseKey(hReg, pFullKey);
    }
    LWREG_SAFE_FREE_MEMORY(pSubKey);
    LWREG_SAFE_FREE_MEMORY(pData);
    return dwError;

error:
    LWREG_SAFE_FREE_MEMORY(pwszParentPath);
    for (indx=0; indx<dwValuesCount; indx++)
    {
        LWREG_SAFE_FREE_MEMORY(pValueArray[indx].pValueName);
        LWREG_SAFE_FREE_MEMORY(pValueArray[indx].pData);
    }
    LWREG_SAFE_FREE_MEMORY(pValueArray);
    goto cleanup;
}


DWORD
RegShellUtilDeleteValue(
    HANDLE hReg,
    PSTR pszRootKeyName,
    PSTR pszDefaultKey,
    PSTR keyName,
    PSTR valueName)
{
    HANDLE hRegLocal = NULL;
    HKEY hRootKey = NULL;
    HKEY hDefaultKey = NULL;
    PWSTR pSubKey = NULL;
    PWSTR pDefaultKey = NULL;
    PWSTR pValueName = NULL;
    DWORD dwError = 0;

    if (!hReg)
    {
        dwError = RegOpenServer(&hRegLocal);
        BAIL_ON_REG_ERROR(dwError);
        hReg = hRegLocal;
    }

    if (!pszRootKeyName)
    {
        pszRootKeyName = HKEY_THIS_MACHINE;
    }
    if (keyName)
    {
	dwError = LwRtlWC16StringAllocateFromCString(&pSubKey, keyName);
	BAIL_ON_REG_ERROR(dwError);
    }
    if (pszDefaultKey)
    {
	dwError = LwRtlWC16StringAllocateFromCString(&pDefaultKey, pszDefaultKey);
	BAIL_ON_REG_ERROR(dwError);
    }

	dwError = LwRtlWC16StringAllocateFromCString(&pValueName, valueName);
	BAIL_ON_REG_ERROR(dwError);

    dwError = RegOpenKeyExA(hReg, NULL, pszRootKeyName, 0, 0, &hRootKey);
    BAIL_ON_REG_ERROR(dwError);
    if (pszDefaultKey)
    {
        dwError = RegOpenKeyExA(hReg, hRootKey, pszDefaultKey, 0, 0, &hDefaultKey);
    }
    else
    {
        hDefaultKey = hRootKey;
        hRootKey = NULL;
    }

    dwError = RegDeleteKeyValueW(hReg, hDefaultKey, pSubKey, pValueName);
    BAIL_ON_REG_ERROR(dwError);

cleanup:
    if (hDefaultKey)
    {
        RegCloseKey(hReg, hDefaultKey);
    }
    if (hRootKey)
    {
        RegCloseKey(hReg, hRootKey);
    }
    RegCloseServer(hRegLocal);
    LWREG_SAFE_FREE_MEMORY(pSubKey);
    return dwError;

error:
    goto cleanup;
}

DWORD
RegShellUtilAllocateMemory(
    HANDLE hReg,
    HKEY hKey,
    REG_DATA_TYPE regType,
    PSTR pszValueName,
    PVOID *ppRetBuf,
    PDWORD pdwRetBufLen)
{
    PBYTE pBuf = NULL;
    DWORD dwError = 0;
    DWORD dwValueLen = 0;

    switch (regType)
    {
        case REG_SZ:
        case REG_MULTI_SZ:
        case REG_BINARY:
            dwError = RegGetValueA(
                          hReg,
                          hKey,
                          NULL,
                          pszValueName,
                          regType,
                          NULL,
                          NULL,
                          &dwValueLen);
            BAIL_ON_REG_ERROR(dwError);
            break;

        default:
            break;
    }
    if (dwError == 0 && dwValueLen > 0)
    {
        dwError = LW_RTL_ALLOCATE((PVOID*)&pBuf, BYTE, (dwValueLen+1));
        BAIL_ON_REG_ERROR(dwError);
    }

    *ppRetBuf = (PVOID) pBuf;
    *pdwRetBufLen = dwValueLen;

cleanup:
    return dwError;
error:
    goto cleanup;
}


DWORD
RegShellUtilGetValue(
    HANDLE hReg,
    PSTR pszRootKeyName,
    PSTR pszDefaultKey,
    PSTR pszKeyName,
    PSTR pszValueName,
    REG_DATA_TYPE regType,
    PVOID *ppValue,
    PDWORD pdwValueLen)
{
    HANDLE hRegLocal = NULL;
    DWORD dwError = 0;
    DWORD dwValueLen = 0;
    PDWORD pdwValue = (PDWORD) ppValue;
    DWORD dwIndex = 0;
    PVOID pRetData = NULL;
    PBYTE *ppData = (PBYTE *) ppValue;
    PSTR *ppszValue = (PSTR *) ppValue;
    PSTR **pppszMultiValue = (PSTR **) ppValue;
    PSTR *ppszMultiStrArray = NULL;

    HKEY hRootKey = NULL;
    HKEY hDefaultKey = NULL;
    HKEY hFullKeyName = NULL;

    if (!hReg)
    {
        dwError = RegOpenServer(&hRegLocal);
        BAIL_ON_REG_ERROR(dwError);
        hReg = hRegLocal;
    }

    /* Open the root key */
    dwError = RegOpenKeyExA(
                      hReg,
                      NULL,
                      pszRootKeyName,
                      0,
                      0,
                      &hRootKey);
    BAIL_ON_REG_ERROR(dwError);

    /* Open the default key */
    dwError = RegOpenKeyExA(
                      hReg,
                      hRootKey,
                      pszDefaultKey,
                      0,
                      0,
                      &hDefaultKey);
    BAIL_ON_REG_ERROR(dwError);

    if (pszKeyName)
    {
        /* Open the sub key */
        dwError = RegOpenKeyExA(
                          hReg,
                          hDefaultKey,
                          pszKeyName,
                          0,
                          0,
                          &hFullKeyName);
        BAIL_ON_REG_ERROR(dwError);
    }
    else
    {
        hFullKeyName = hDefaultKey;
        hDefaultKey = NULL;
    }

    dwError = RegShellUtilAllocateMemory(
                  hReg,
                  hFullKeyName,
                  regType,
                  pszValueName,
                  &pRetData,
                  &dwValueLen);

    switch (regType)
    {
        case REG_SZ:
            dwError = RegGetValueA(
                          hReg,
                          hFullKeyName,
                          NULL,
                          pszValueName,
                          REG_SZ,
                          NULL,
                          pRetData,
                          &dwValueLen);
            BAIL_ON_REG_ERROR(dwError);
            *ppszValue = pRetData;
            *pdwValueLen = dwValueLen;
            break;

        case REG_DWORD:
            dwValueLen = sizeof(DWORD);
            dwError = RegGetValueA(
                          hReg,
                          hFullKeyName,
                          NULL,
                          pszValueName,
                          REG_DWORD,
                          NULL,
                          pdwValue,
                          &dwValueLen);
            BAIL_ON_REG_ERROR(dwError);
            *pdwValueLen = dwValueLen;
            break;

        case REG_BINARY:
            dwError = RegGetValueA(
                          hReg,
                          hFullKeyName,
                          NULL,
                          pszValueName,
                          REG_BINARY,
                          NULL,
                          pRetData,
                          &dwValueLen);
            BAIL_ON_REG_ERROR(dwError);
            *ppData = pRetData;
            *pdwValueLen = dwValueLen;
            break;
        case REG_MULTI_SZ:
            dwError = RegGetValueA(
                          hReg,
                          hFullKeyName,
                          NULL,
                          pszValueName,
                          REG_MULTI_SZ,
                          NULL,
                          pRetData,
                          &dwValueLen);
            BAIL_ON_REG_ERROR(dwError);

            dwError = RegByteArrayToMultiStrsA(
                          pRetData,
                          dwValueLen,
                          &ppszMultiStrArray);
            LWREG_SAFE_FREE_MEMORY(pRetData);
            BAIL_ON_REG_ERROR(dwError);

            /* Return number of entries in multi-string array, not byte count */
            for (dwIndex=0; ppszMultiStrArray[dwIndex]; dwIndex++)
                ;
            *pdwValueLen = dwIndex;
            *pppszMultiValue = ppszMultiStrArray;

            break;
    }

cleanup:
    RegCloseServer(hRegLocal);
    RegCloseKey(hReg, hFullKeyName);
    if (hDefaultKey)
    {
        RegCloseKey(hReg, hDefaultKey);
    }
    RegCloseKey(hReg, hRootKey);
    return dwError;
error:
    goto cleanup;
}


DWORD
RegShellUtilEscapeString(
    PSTR pszValue,
    PSTR *ppszRetValue,
    PDWORD pdwEscapeValueLen)
{
    DWORD i = 0;
    DWORD dwError = 0;
    DWORD dwLen = 0;
    DWORD dwEscapeValueLen = 0;
    PSTR pszRetValue = NULL;

    BAIL_ON_INVALID_POINTER(pszValue);
    BAIL_ON_INVALID_POINTER(ppszRetValue);
    BAIL_ON_INVALID_POINTER(pdwEscapeValueLen);

    /* Count number of \ found in string to escape */
    for (i=0; pszValue[i]; i++)
    {
        if (pszValue[i] == '\\' || pszValue[i] == '\n' ||
            pszValue[i] == '\r' || pszValue[i] == '"' ||
            pszValue[i] == '\t' || pszValue[i] == '\a' ||
            pszValue[i] == '\v' || pszValue[i] == '\f')
        {
            dwEscapeValueLen++;
        }
        dwEscapeValueLen++;
    }
    dwEscapeValueLen++;

    dwError = LW_RTL_ALLOCATE((PVOID*)&pszRetValue, CHAR,
		                  sizeof(*pszRetValue)* dwEscapeValueLen);
    BAIL_ON_REG_ERROR(dwError);

    for (i=0; pszValue[i]; i++)
    {
        if (pszValue[i] == '\n')
        {
            pszRetValue[dwLen++] = '\\';
            pszRetValue[dwLen++] = 'n';
        }
        if (pszValue[i] == '\r')
        {
            pszRetValue[dwLen++] = '\\';
            pszRetValue[dwLen++] = 'r';
        }
        else if (pszValue[i] == '"')
        {
            pszRetValue[dwLen++] = '\\';
            pszRetValue[dwLen++] = '"';
        }
        else if (pszValue[i] == '\t')
        {
            pszRetValue[dwLen++] = '\\';
            pszRetValue[dwLen++] = 't';
        }
        else if (pszValue[i] == '\a')
        {
            pszRetValue[dwLen++] = '\\';
            pszRetValue[dwLen++] = 'a';
        }
        else if (pszValue[i] == '\v')
        {
            pszRetValue[dwLen++] = '\\';
            pszRetValue[dwLen++] = 'v';
        }
        else if (pszValue[i] == '\f')
        {
            pszRetValue[dwLen++] = '\\';
            pszRetValue[dwLen++] = 'f';
        }
        else if (pszValue[i] == '\\')
        {
            pszRetValue[dwLen++] = '\\';
            pszRetValue[dwLen++] = '\\';
        }
        else
        {
            pszRetValue[dwLen++] = pszValue[i];
        }
    }
    pszRetValue[dwLen] = '\0';
    *ppszRetValue = pszRetValue;
    *pdwEscapeValueLen = dwLen;

cleanup:
    return dwError;

error:
    goto cleanup;
}
