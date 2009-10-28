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
        dwError = LwAllocateString(pszInKeyName, &pszNewPath);
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
        dwError = LwAllocateMemory(dwFullPathLen, (LW_PVOID) &pszNewPath);
        BAIL_ON_REG_ERROR(dwError);

        if (pszInDefaultKey)
        {
            strcat(pszNewPath, "\\");
            strcat(pszNewPath, pszInDefaultKey);
        }
        if (pszInKeyName)
        {
            /* Copy of pszInKeyName because strtok_r modifies this value */
            dwError = LwAllocateString(pszInKeyName, &pszStrtokValue);
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
                    dwError = LW_ERROR_INVALID_CONTEXT;
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
        dwError = LwAllocateString("\\", &pszNewPath);
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
        dwError = LwAllocateString(pszNewPath, ppszParentPath);
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
            dwError = LwAllocateString(pszTmp+1, ppszSubKey);
            BAIL_ON_REG_ERROR(dwError);
        }
        else
        {
            dwError = LwAllocateString("", ppszSubKey);
            BAIL_ON_REG_ERROR(dwError);
        }

    }
    if (ppszFullPath)
    {
        *ppszFullPath = pszNewPath;
    }

cleanup:
    LW_SAFE_FREE_MEMORY(pszStrtokValue);
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
        pszRootKeyName = LIKEWISE_ROOT_KEY;
    }
    dwError = RegOpenKeyExA(hReg, NULL, pszRootKeyName, 0, 0, &pRootKey);
    BAIL_ON_REG_ERROR(dwError);

    dwError = LwMbsToWc16s(pszKey, &pSubKey);
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

    LW_SAFE_FREE_MEMORY(pSubKey);
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

    BAIL_ON_INVALID_HANDLE(hReg);

    if (!pszRootKeyName)
    {
        pszRootKeyName = LIKEWISE_ROOT_KEY;
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
        dwError = LwMbsToWc16s(pszToken, &pwszSubKey);
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

        LW_SAFE_FREE_MEMORY(pwszSubKey);

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
    if (pCurrentKey)
    {
        RegCloseKey(hReg, pCurrentKey);
    }
    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pwszSubKey);
    goto cleanup;
}


DWORD
RegShellUtilDeleteKey(
    HANDLE hReg,
    PSTR pszRootKeyName,
    PSTR pszDefaultKey,
    PSTR keyName)
{
    PWSTR pwszSubKey = NULL;
    HKEY pCurrentKey = NULL;
    DWORD dwError = 0;
    PSTR pszFullPath = NULL;
    PSTR pszParentPath = NULL;
    PSTR pszSubKey = NULL;
    HKEY pRootKey = NULL;

    BAIL_ON_INVALID_HANDLE(hReg);

    if (!pszRootKeyName)
    {
        pszRootKeyName = LIKEWISE_ROOT_KEY;
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
        dwError = LwMbsToWc16s(pszParentPath+1, &pwszSubKey);
        BAIL_ON_REG_ERROR(dwError);
        dwError = RegOpenKeyExW(
                      hReg,
                      pRootKey,
                      pwszSubKey,
                      0,
                      0,
                      &pCurrentKey);
        BAIL_ON_REG_ERROR(dwError);
        LW_SAFE_FREE_MEMORY(pwszSubKey);
    }

    dwError = LwMbsToWc16s(pszSubKey, &pwszSubKey);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegDeleteKey(hReg, pCurrentKey, pwszSubKey);
    BAIL_ON_REG_ERROR(dwError);

cleanup:
    if (pCurrentKey)
    {
        RegCloseKey(hReg, pCurrentKey);
    }
    LW_SAFE_FREE_MEMORY(pwszSubKey);
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
    PWSTR pwszSubKey = NULL;
    HKEY pCurrentKey = NULL;
    DWORD dwError = 0;
    PSTR pszFullPath = NULL;
    PSTR pszParentPath = NULL;
    PSTR pszSubKey = NULL;
    HKEY pRootKey = NULL;

    BAIL_ON_INVALID_HANDLE(hReg);

    if (!pszRootKeyName)
    {
        pszRootKeyName = LIKEWISE_ROOT_KEY;
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
        dwError = LwMbsToWc16s(pszParentPath+1, &pwszSubKey);
        BAIL_ON_REG_ERROR(dwError);
        dwError = RegOpenKeyExW(
                      hReg,
                      pRootKey,
                      pwszSubKey,
                      0,
                      0,
                      &pCurrentKey);
        BAIL_ON_REG_ERROR(dwError);
        LW_SAFE_FREE_MEMORY(pwszSubKey);
    }

    dwError = LwMbsToWc16s(pszSubKey, &pwszSubKey);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegDeleteTree(hReg, pCurrentKey, pwszSubKey);
    BAIL_ON_REG_ERROR(dwError);

cleanup:
    if (pCurrentKey)
    {
        RegCloseKey(hReg, pCurrentKey);
    }
    LW_SAFE_FREE_MEMORY(pwszSubKey);
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
    PSTR *ppszRootKeyNames = NULL;
    DWORD dwNumRootKeys = 0;

    BAIL_ON_INVALID_HANDLE(hReg);

    if (!pszRootKeyName)
    {
        dwError = RegEnumRootKeysA(
                      hReg,
                      &ppszRootKeyNames,
                      &dwNumRootKeys);
        BAIL_ON_REG_ERROR(dwError);

#if 1
        /*
         * Allocation of this array and conversion to unicode should not
         * be needed. This should be the type of data returned by
         * RegEnumRootKeysA(), and that array can be the return value of
         * this function.
         */
        dwError = LwAllocateMemory(
                      sizeof(LW_WCHAR *)*dwNumRootKeys,
                      (LW_PVOID) &subKeys);
        BAIL_ON_REG_ERROR(dwError);
        for (i=0; i<dwNumRootKeys; i++)
        {
            dwError = LwMbsToWc16s(
                          ppszRootKeyNames[i],
                          &subKeys[i]);
            LW_SAFE_FREE_STRING(ppszRootKeyNames[i]);
        }
        LW_SAFE_FREE_MEMORY(ppszRootKeyNames);
#endif

        *pppRetSubKeys = subKeys;
        *pdwRetSubKeyCount = dwNumRootKeys;
        return dwError;
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
        dwError = LwMbsToWc16s(pszParentPath+1, &pwszSubKey);
        BAIL_ON_REG_ERROR(dwError);
        dwError = RegOpenKeyExW(
                      hReg,
                      pRootKey,
                      pwszSubKey,
                      0,
                      0,
                      &pFullKey);
        BAIL_ON_REG_ERROR(dwError);
        LW_SAFE_FREE_MEMORY(pwszSubKey);
    }
    else
    {
        pFullKey = pRootKey;
    }

    dwError = RegQueryInfoKeyW(
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

    dwError = LwAllocateMemory(
                  sizeof(LW_WCHAR *)*dwSubKeyCount,
                  (LW_PVOID) &subKeys);
    BAIL_ON_REG_ERROR(dwError);

#ifdef _LW_DEBUG
    printf( "\nNumber of subkeys: %d\n", dwSubKeyCount);
#endif

    for (i = 0; i < dwSubKeyCount; i++)
    {
        dwSubKeyLen = dwMaxSubKeyLen+1;
        dwError = LwAllocateMemory(
                      dwSubKeyLen * sizeof(LW_WCHAR),
                      (LW_PVOID) &subKeys[i]);
        BAIL_ON_REG_ERROR(dwError);

        dwError = RegEnumKeyEx((HANDLE)hReg,
                                pFullKey,
                                i,
                                subKeys[i],
                                &dwSubKeyLen,
                                NULL,
                                NULL,
                                NULL,
                                NULL);
        BAIL_ON_REG_ERROR(dwError);
    }

    *pppRetSubKeys = subKeys;
    *pdwRetSubKeyCount = dwSubKeyCount;
cleanup:
    if (pFullKey && pFullKey != pRootKey)
    {
        RegCloseKey(hReg, pFullKey);
    }
    return dwError;

error:
    for (i = 0; i < dwSubKeyCount; i++)
    {
        LW_SAFE_FREE_MEMORY(subKeys[i]);
    }
    LW_SAFE_FREE_MEMORY(subKeys);
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
    DWORD dataLen)
{
    DWORD dwError = 0;
    PBYTE pData = NULL;
    SSIZE_T dwDataLen = 0;
    PWSTR pwszParentPath = NULL;
    HKEY pFullKey = NULL;
    HKEY pRootKey = NULL;
    PSTR pszParentPath = NULL;

    BAIL_ON_INVALID_HANDLE(hReg);

    if (!pszRootKeyName)
    {
        pszRootKeyName = LIKEWISE_ROOT_KEY;
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
        dwError = LwMbsToWc16s(pszParentPath+1, &pwszParentPath);
        BAIL_ON_REG_ERROR(dwError);
        dwError = RegOpenKeyExW(
                      hReg,
                      pRootKey,
                      pwszParentPath,
                      0,
                      0,
                      &pFullKey);
        BAIL_ON_REG_ERROR(dwError);
        LW_SAFE_FREE_MEMORY(pwszParentPath);
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
            dwError = LwAllocateString(data ? data : "", (LW_PVOID) &pData);
            BAIL_ON_REG_ERROR(dwError);

            dwDataLen = strlen((PSTR) pData);
            break;

        case REG_DWORD:
            dwError = LwAllocateMemory(sizeof(DWORD), (LW_PVOID) &pData);
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
        LW_SAFE_FREE_MEMORY(pData);
    }

cleanup:
    LW_SAFE_FREE_MEMORY(pwszParentPath);
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

    BAIL_ON_INVALID_HANDLE(hReg);

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
        dwError = LwMbsToWc16s(pszParentPath+1, &pwszParentPath);
        BAIL_ON_REG_ERROR(dwError);
        dwError = RegOpenKeyExW(
                      hReg,
                      pRootKey,
                      pwszParentPath,
                      0,
                      0,
                      &pFullKey);
        BAIL_ON_REG_ERROR(dwError);
        LW_SAFE_FREE_MEMORY(pwszParentPath);
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
    dwError = LwAllocateMemory(
                  dwValuesCount * sizeof(REGSHELL_UTIL_VALUE),
                  (LW_PVOID) &pValueArray);
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

        dwError = LwAllocateMemory(
                      dwValueNameLen,
                      (LW_PVOID) &pszValueName);
        BAIL_ON_REG_ERROR(dwError);

        dwDataLen = dwMaxValueLen;
        dwError = LwAllocateMemory(dwDataLen, (LW_PVOID) &pData);
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

        dwError = LwMbsToWc16s(pszValueName, &pValueArray[indx].pValueName);
        BAIL_ON_REG_ERROR(dwError);

        pValueArray[indx].type = regType;
        pValueArray[indx].pData = pData;
        pData = NULL;
        pValueArray[indx].dwDataLen = dwDataLen;
    }

    *valueArray = pValueArray;
    *pdwValueArrayLen = indx;

cleanup:
    if (pFullKey && pFullKey != pRootKey)
    {
        RegCloseKey(hReg, pFullKey);
    }
    LW_SAFE_FREE_MEMORY(pSubKey);
    LW_SAFE_FREE_MEMORY(pData);
    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pwszParentPath);
    for (indx=0; indx<dwValuesCount; indx++)
    {
        LW_SAFE_FREE_MEMORY(pValueArray[indx].pValueName);
        LW_SAFE_FREE_MEMORY(pValueArray[indx].pData);
    }
    LW_SAFE_FREE_MEMORY(pValueArray);
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
    HKEY pRootKey = NULL;
    PWSTR pSubKey = NULL;
    PWSTR pValueName = NULL;
    DWORD dwError = 0;

    BAIL_ON_INVALID_HANDLE(hReg);

    if (!pszRootKeyName)
    {
        pszRootKeyName = LIKEWISE_ROOT_KEY;
    }
    if (keyName)
    {
        dwError = LwMbsToWc16s(keyName, &pSubKey);
        BAIL_ON_REG_ERROR(dwError);
    }
    else if (pszDefaultKey)
    {
        dwError = LwMbsToWc16s(pszDefaultKey, &pSubKey);
        BAIL_ON_REG_ERROR(dwError);
    }

    dwError = LwMbsToWc16s(valueName, &pValueName);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegOpenKeyExA(hReg, NULL, pszRootKeyName, 0, 0, &pRootKey);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegDeleteKeyValue(hReg, pRootKey, pSubKey, pValueName);
    BAIL_ON_REG_ERROR(dwError);

cleanup:
    LW_SAFE_FREE_MEMORY(pSubKey);
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
        dwError = LwAllocateMemory(
                      (dwValueLen+1) * sizeof(CHAR),
                      (PVOID) &pBuf);
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
    DWORD dwError = 0;
    DWORD dwValueLen = 0;
    PDWORD pdwValue = (PDWORD) ppValue;
    PVOID pRetData = NULL;
    PBYTE *ppData = (PBYTE *) ppValue;
    PSTR *ppszValue = (PSTR *) ppValue;

    HKEY hRootKey = NULL;
    HKEY hDefaultKey = NULL;
    HKEY hFullKeyName = NULL;

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

    /* Open the sub key */
    dwError = RegOpenKeyExA(
                      hReg,
                      hDefaultKey,
                      pszKeyName,
                      0,
                      0,
                      &hFullKeyName);
    BAIL_ON_REG_ERROR(dwError);

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
            *ppData = pRetData;
            *pdwValueLen = dwValueLen;
            BAIL_ON_REG_ERROR(dwError);
            break;
    }

cleanup:
    RegCloseKey(hReg, hFullKeyName);
    RegCloseKey(hReg, hDefaultKey);
    RegCloseKey(hReg, hRootKey);
    return dwError;
error:
    goto cleanup;
}
