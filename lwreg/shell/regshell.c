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
 *       regshell.c
 *
 * Abstract:
 *
 *        Registry
 *
 *        Registry Shell application
 *
 * Authors: Adam Bernstein (abernstein@likewise.com)
 */

#include "regshell.h"
#include <locale.h>
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
#ifdef _DEBUG
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
    PSTR pszKey)
{
    DWORD dwError = 0;
    HKEY pFullKey = NULL;
    HKEY pRootKey = NULL;
    PWSTR pSubKey = NULL;

    BAIL_ON_INVALID_HANDLE(hReg);
    BAIL_ON_INVALID_POINTER(pszKey);

    dwError = RegOpenRootKey(hReg, LIKEWISE_ROOT_KEY, &pRootKey);
    BAIL_ON_REG_ERROR(dwError);

    dwError = LwMbsToWc16s(pszKey, &pSubKey);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegOpenKeyEx(
        hReg,
        pRootKey,
        pSubKey,
        0,
        0,
        &pFullKey);
    BAIL_ON_REG_ERROR(dwError);

cleanup:
    if (pFullKey && pFullKey != pRootKey)
    {
        RegCloseKey(hReg, pFullKey);
    }
    return dwError;

error:
    goto cleanup;
}


DWORD
RegShellListKeys(
    PREGSHELL_PARSE_STATE pParseState,
    PREGSHELL_CMD_ITEM rsItem)
{
    HKEY pRootKey = NULL;

    DWORD dwError = 0;
    DWORD dwSubKeyCount = 0;
    DWORD dwMaxSubKeyLen = 0;
    DWORD i = 0;
    DWORD dwSubKeyLen = MAX_KEY_LENGTH;
    LW_WCHAR subKey[MAX_KEY_LENGTH];
    PSTR pszSubKey = NULL;
    HKEY pFullKey = NULL;
    HANDLE hReg = NULL;
    PSTR pszParentPath = NULL;
    PWSTR pwszSubKey = NULL;

    BAIL_ON_INVALID_HANDLE(pParseState);
    BAIL_ON_INVALID_HANDLE(pParseState->hReg);
    hReg = pParseState->hReg;

    dwError = RegShellCanonicalizePath(pParseState->pszDefaultKey,
                                       rsItem->keyName,
                                       &pszParentPath,
                                       NULL,
                                       NULL);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegOpenRootKey(hReg, LIKEWISE_ROOT_KEY, &pRootKey);
    BAIL_ON_REG_ERROR(dwError);

    if (pszParentPath && pszParentPath[1])
    {
        dwError = LwMbsToWc16s(pszParentPath+1, &pwszSubKey);
        BAIL_ON_REG_ERROR(dwError);
        dwError = RegOpenKeyEx(
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

    dwError = RegQueryInfoKey(
        hReg,
        pFullKey,
        NULL,
        NULL,
        NULL,
        &dwSubKeyCount,
        &dwMaxSubKeyLen,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL);
    BAIL_ON_REG_ERROR(dwError);

#ifdef _DEBUG
    printf( "\nNumber of subkeys: %d\n", dwSubKeyCount);
#endif

    for (i = 0; i < dwSubKeyCount; i++)
    {
        dwSubKeyLen = MAX_KEY_LENGTH;
        dwError = RegEnumKeyEx((HANDLE)hReg,
                                pFullKey,
                                i,
                                subKey,
                                &dwSubKeyLen,
                                NULL,
                                NULL,
                                NULL,
                                NULL);
        BAIL_ON_REG_ERROR(dwError);

        dwError = LwWc16sToMbs(subKey, &pszSubKey);
        BAIL_ON_REG_ERROR(dwError);

#ifndef _DEBUG
        printf("%s\n", pszSubKey);
#else
        printf("SubKey %d name is '%s'\n", i, pszSubKey);
#endif
        LW_SAFE_FREE_STRING(pszSubKey);
        pszSubKey = NULL;
    }

cleanup:
    LW_SAFE_FREE_STRING(pszSubKey);
    if (pFullKey && pFullKey != pRootKey)
    {
        RegCloseKey(hReg, pFullKey);
    }
    return dwError;

error:
    goto cleanup;
}


DWORD
RegShellAddKey(
    PREGSHELL_PARSE_STATE pParseState,
    PREGSHELL_CMD_ITEM rsItem)
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

    BAIL_ON_INVALID_HANDLE(pParseState);
    BAIL_ON_INVALID_HANDLE(pParseState->hReg);
    dwError = RegShellCanonicalizePath(pParseState->pszDefaultKey,
                                       rsItem->keyName,
                                       &pszFullPath,
                                       NULL,
                                       &pszSubKey);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegOpenRootKey(pParseState->hReg, LIKEWISE_ROOT_KEY, &pRootKey);
    BAIL_ON_REG_ERROR(dwError);

    pCurrentKey = pRootKey;
    pszToken = strtok_r(pszFullPath, pszDelim, &pszStrtokState);
    while (!LW_IS_NULL_OR_EMPTY_STR(pszToken))
    {
        dwError = LwMbsToWc16s(pszToken, &pwszSubKey);
        BAIL_ON_REG_ERROR(dwError);

        dwError = RegCreateKeyEx(
            pParseState->hReg,
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
          dwError = RegCloseKey(pParseState->hReg, pCurrentKey);
          BAIL_ON_REG_ERROR(dwError);
          pCurrentKey = NULL;
        }

        pCurrentKey = pNextKey;

        pszToken = strtok_r (NULL, pszDelim, &pszStrtokState);
    }

cleanup:
    if (pCurrentKey)
    {
        dwError = RegCloseKey(pParseState->hReg, pCurrentKey);
    }
    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pwszSubKey);
    if (dwError == LW_ERROR_ACCESS_DENIED)
    {

        PrintError("regshell", dwError);
        dwError = 0;
    }

    goto cleanup;
}


DWORD
RegShellDeleteKey(
    PREGSHELL_PARSE_STATE pParseState,
    PREGSHELL_CMD_ITEM rsItem)
{
    PWSTR pwszSubKey = NULL;
    HKEY pCurrentKey = NULL;
    DWORD dwError = 0;
    HANDLE hReg = NULL;
    PSTR pszFullPath = NULL;
    PSTR pszParentPath = NULL;
    PSTR pszSubKey = NULL;
    HKEY pRootKey = NULL;


    BAIL_ON_INVALID_HANDLE(pParseState->hReg);
    hReg = pParseState->hReg;

    dwError = RegShellCanonicalizePath(pParseState->pszDefaultKey,
                                       rsItem->keyName,
                                       &pszFullPath,
                                       &pszParentPath,
                                       &pszSubKey);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegShellIsValidKey(pParseState->hReg, pszFullPath+1);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegOpenRootKey(hReg, LIKEWISE_ROOT_KEY, &pRootKey);
    BAIL_ON_REG_ERROR(dwError);

    pCurrentKey = pRootKey;
    if (pszParentPath && pszParentPath[1])
    {
        dwError = LwMbsToWc16s(pszParentPath+1, &pwszSubKey);
        BAIL_ON_REG_ERROR(dwError);
        dwError = RegOpenKeyEx(
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
    if (dwError == LW_ERROR_FAILED_DELETE_HAS_SUBKEY ||
        dwError == LW_ERROR_KEY_IS_ACTIVE ||
        dwError == LW_ERROR_NO_SUCH_KEY ||
        dwError == LW_ERROR_ACCESS_DENIED)
    {

        PrintError("regshell", dwError);
        dwError = 0;
    }
    goto cleanup;
}


/* registry server backend RegDeleteKeyValue() not working when value is
 * under the root of the hive.
 */
DWORD
RegShellDeleteValue(
    PREGSHELL_PARSE_STATE pParseState,
    PREGSHELL_CMD_ITEM rsItem)
{
    HKEY pRootKey = NULL;
    PWSTR pSubKey = NULL;
    PWSTR pValueName = NULL;
    DWORD dwError = 0;
    HANDLE hReg = NULL;

    BAIL_ON_INVALID_HANDLE(pParseState);
    BAIL_ON_INVALID_HANDLE(pParseState->hReg);
    hReg = pParseState->hReg;

    if (rsItem->keyName)
    {
        dwError = LwMbsToWc16s(rsItem->keyName, &pSubKey);
        BAIL_ON_REG_ERROR(dwError);
    }
    else if (pParseState->pszDefaultKey)
    {
        dwError = LwMbsToWc16s(pParseState->pszDefaultKey, &pSubKey);
        BAIL_ON_REG_ERROR(dwError);
    }

    dwError = LwMbsToWc16s(rsItem->valueName, &pValueName);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegOpenRootKey(hReg, LIKEWISE_ROOT_KEY, &pRootKey);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegDeleteKeyValue(hReg, pRootKey, pSubKey, pValueName);
    BAIL_ON_REG_ERROR(dwError);

cleanup:
    LW_SAFE_FREE_MEMORY(pSubKey);
    return dwError;

error:
    if (dwError == LW_ERROR_ACCESS_DENIED)
    {
        PrintError("regshell", dwError);
        dwError = 0;
    }
    goto cleanup;
}


DWORD
RegShellDeleteTree(
    PREGSHELL_PARSE_STATE pParseState,
    PREGSHELL_CMD_ITEM rsItem)
{
    HKEY pCurrentKey = NULL;
    DWORD dwError = 0;
    HANDLE hReg = NULL;
    PSTR pszFullPath = NULL;
    PSTR pszParentPath = NULL;
    PSTR pszSubKey = NULL;
    PWSTR pwszSubKey = NULL;
    HKEY pRootKey = NULL;

    BAIL_ON_INVALID_HANDLE(pParseState->hReg);
    hReg = pParseState->hReg;

    dwError = RegShellCanonicalizePath(pParseState->pszDefaultKey,
                                       rsItem->keyName,
                                       &pszFullPath,
                                       &pszParentPath,
                                       &pszSubKey);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegShellIsValidKey(pParseState->hReg, pszFullPath+1);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegOpenRootKey(hReg, LIKEWISE_ROOT_KEY, &pRootKey);
    BAIL_ON_REG_ERROR(dwError);

    pCurrentKey = pRootKey;
    if (pszParentPath && pszParentPath[1])
    {
        dwError = LwMbsToWc16s(pszParentPath+1, &pwszSubKey);
        BAIL_ON_REG_ERROR(dwError);
        dwError = RegOpenKeyEx(
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
    return dwError;

error:
    if (dwError == LW_ERROR_KEY_IS_ACTIVE ||
        dwError == LW_ERROR_NO_SUCH_KEY ||
        dwError == LW_ERROR_ACCESS_DENIED)
    {
        PrintError("regshell", dwError);
        dwError = 0;
    }
    goto cleanup;
}


DWORD
RegShellSetValue(
    PREGSHELL_PARSE_STATE pParseState,
    PREGSHELL_CMD_ITEM rsItem)
{
    DWORD dwError = 0;
    PBYTE pData = NULL;
    SSIZE_T dwDataLen = 0;
    PWSTR pValueName = NULL;
    PWSTR pwszParentPath = NULL;
    HKEY pFullKey = NULL;
    HKEY pRootKey = NULL;
    HANDLE hReg = NULL;
    PSTR pszParentPath = NULL;

    BAIL_ON_INVALID_HANDLE(pParseState->hReg);
    hReg = pParseState->hReg;

    dwError = RegShellCanonicalizePath(pParseState->pszDefaultKey,
                                       rsItem->keyName,
                                       &pszParentPath,
                                       NULL,
                                       NULL);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegOpenRootKey(hReg, LIKEWISE_ROOT_KEY, &pRootKey);
    BAIL_ON_REG_ERROR(dwError);

    pFullKey = pRootKey;
    if (pszParentPath && pszParentPath[1])
    {
        dwError = LwMbsToWc16s(pszParentPath+1, &pwszParentPath);
        BAIL_ON_REG_ERROR(dwError);
        dwError = RegOpenKeyEx(
                      hReg,
                      pRootKey,
                      pwszParentPath,
                      0,
                      0,
                      &pFullKey);
        BAIL_ON_REG_ERROR(dwError);
        LW_SAFE_FREE_MEMORY(pwszParentPath);
    }

    dwError = LwMbsToWc16s(rsItem->valueName, &pValueName);
    BAIL_ON_REG_ERROR(dwError);


    switch (rsItem->type)
    {
        case REG_MULTI_SZ:
            dwError = ConvertMultiStrsToByteArray(
                                        rsItem->args,
                                        &pData,
                                        &dwDataLen);
            BAIL_ON_REG_ERROR(dwError);
            break;

        case REG_SZ:
            dwError = LwAllocateString(rsItem->args[0], (LW_PVOID) &pData);
            BAIL_ON_REG_ERROR(dwError);

            /* Should not have to save NULL string terminator */
            dwDataLen = strlen((PSTR) pData) + 1;
            break;

        case REG_DWORD:
            dwError = LwAllocateMemory(sizeof(DWORD), (LW_PVOID) &pData);
            BAIL_ON_REG_ERROR(dwError);

            dwDataLen = rsItem->binaryValueLen;
            memcpy(pData, rsItem->binaryValue, dwDataLen);
            break;

        case REG_BINARY:
            pData = rsItem->binaryValue;
            dwDataLen = rsItem->binaryValueLen;
            break;

        default:
            printf("RegShellSetValue: unknown type %d\n", rsItem->type);
            break;
    }

    // Set specified value from command line
    dwError = RegSetValueEx(
        hReg,
        pFullKey,
        pValueName,
        0,
        rsItem->type,
        pData,
        dwDataLen);
    BAIL_ON_REG_ERROR(dwError);
    if (rsItem->type != REG_BINARY)
    {
        LW_SAFE_FREE_MEMORY(pData);
    }
    LW_SAFE_FREE_MEMORY(pValueName);

cleanup:
    LW_SAFE_FREE_MEMORY(pwszParentPath);
    if (pFullKey && pFullKey != pRootKey)
    {
        RegCloseKey(hReg, pFullKey);
    }
    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pValueName);
    if (dwError == LW_ERROR_DUPLICATE_KEYVALUENAME ||
        dwError == LW_ERROR_ACCESS_DENIED)
    {

        PrintError("regshell", dwError);
        dwError = 0;
    }

    goto cleanup;
}


DWORD
RegShellDumpByteArray(
    PBYTE pByteArray,
    DWORD dwByteArrayLen)
{
    DWORD i = 0;
    DWORD dwError = 0;

    BAIL_ON_INVALID_POINTER(pByteArray);

    for (i=0; i<dwByteArrayLen; i++)
    {
        printf("%02x", pByteArray[i]);
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
RegShellImportFile(
    HANDLE hReg,
    PREGSHELL_CMD_ITEM rsItem)
{
    IMPORT_CONTEXT ctx = {0};
    HANDLE parseH = NULL;
    DWORD dwError = 0;


    ctx.hReg = hReg;

    dwError = RegParseOpen(rsItem->args[0], NULL, NULL, &parseH);
    BAIL_ON_REG_ERROR(dwError);

    RegParseInstallCallback(parseH, parseDebugCallback, &ctx, NULL);
    RegParseInstallCallback(parseH, parseImportCallback, &ctx, NULL);

    dwError = RegParseRegistry(parseH);
    BAIL_ON_REG_ERROR(dwError);

    RegParseClose(parseH);

cleanup:
    return dwError;

error:
    goto cleanup;
}

DWORD
RegShellListValues(
    PREGSHELL_PARSE_STATE pParseState,
    PREGSHELL_CMD_ITEM rsItem)
{
    DWORD dwError = 0;
    DWORD indx = 0;
    wchar16_t pValueName[256] = {0};
    DWORD dwValueNameLen = sizeof(pValueName);
    DWORD regType = 0;
    PBYTE pData = NULL;
    DWORD dwDataLen = 0;
    PWSTR pSubKey = NULL;
    HKEY pRootKey = NULL;
    HKEY pFullKey = NULL;
    PSTR pszValueName = NULL;
    PSTR pszParentPath = NULL;
    PWSTR pwszParentPath = NULL;
    HANDLE hReg = NULL;

    BAIL_ON_INVALID_HANDLE(pParseState);
    BAIL_ON_INVALID_HANDLE(pParseState->hReg);
    hReg = pParseState->hReg;


    dwError = RegShellCanonicalizePath(pParseState->pszDefaultKey,
                                       rsItem->keyName,
                                       &pszParentPath,
                                       NULL,
                                       NULL);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegOpenRootKey(hReg, LIKEWISE_ROOT_KEY, &pRootKey);
    BAIL_ON_REG_ERROR(dwError);

    pFullKey = pRootKey;
    if (pszParentPath && strcmp(pszParentPath, "\\") != 0)
    {
        dwError = LwMbsToWc16s(pszParentPath+1, &pwszParentPath);
        BAIL_ON_REG_ERROR(dwError);
        dwError = RegOpenKeyEx(
                      hReg,
                      pRootKey,
                      pwszParentPath,
                      0,
                      0,
                      &pFullKey);
        BAIL_ON_REG_ERROR(dwError);
        LW_SAFE_FREE_MEMORY(pwszParentPath);
    }

    do {
#if 0
        /* test retrevial of data length */
        /* Does not work yet! */
        dwError = RegEnumValue(
                      hReg,
                      pFullKey,
                      indx,
                      pValueName,
                      &dwValueNameLen,
                      NULL,
                      &regType,
                      NULL,
                      &dwDataLen);
printf("RegShellListValues: reported length is <%d>\n", dwDataLen);
#else
        /* adam hack to make work for now! */
        dwDataLen = 8192;
#endif

        if (dwError == 0 && dwDataLen > 0)
        {
            dwError = LwAllocateMemory(dwDataLen, (LW_PVOID) &pData);
            BAIL_ON_REG_ERROR(dwError);
            dwError = RegEnumValue(
                          hReg,
                          pFullKey,
                          indx,
                          pValueName,
                          &dwValueNameLen,
                          NULL,
                          &regType,
                          pData,
                          &dwDataLen);
        }

        if (dwError == 0)
        {
            dwError = LwWc16sToMbs(pValueName, &pszValueName);
            BAIL_ON_REG_ERROR(dwError);

#ifndef _DEBUG
            printf("%s\n  ", pszValueName);
#else
            printf("ListValues: value='%s\n", pszValueName);
            printf("ListValues: dataLen='%d'\n", dwDataLen);
#endif
            switch (regType)
            {
                case REG_SZ:
                    printf("REG_SZ:     pData='%s'\n", pData);
                    break;

                case REG_DWORD:
                    printf("REG_DWORD:  pData=<%08x>\n", *((PDWORD) pData));
                    break;

                case REG_BINARY:
                    printf("REG_BINARY: ");
                    RegShellDumpByteArray(pData, dwDataLen);
                    printf("\n");
                    break;

                default:
                    printf("no handler for datatype %d\n", regType);
            }
            LW_SAFE_FREE_STRING(pszValueName);
            pszValueName = NULL;
            LW_SAFE_FREE_MEMORY(pData);
            pData = NULL;
            dwDataLen = 0;
            dwValueNameLen = sizeof(pValueName);
        }
        printf("\n");
        indx++;
    } while (dwError == 0);
    if (dwError == LW_ERROR_NO_MORE_ITEMS)
    {
        dwError = 0;
    }


cleanup:
    if (pFullKey && pFullKey != pRootKey)
    {
        RegCloseKey(hReg, pFullKey);
    }
    LW_SAFE_FREE_MEMORY(pSubKey);
    LW_SAFE_FREE_MEMORY(pData);
    return dwError;

error:
    goto cleanup;
}


DWORD
RegShellProcessCmd(
    PREGSHELL_PARSE_STATE pParseState,
    DWORD argc,
    PSTR *argv)
{

    DWORD dwError = 0;
    PREGSHELL_CMD_ITEM rsItem = NULL;
    PCSTR pszErrorPrefix = NULL;
    PSTR pszPwd = NULL;
    PSTR pszToken = NULL;
    PSTR pszKeyName = NULL;
    PSTR pszNewKeyName = NULL;
    PSTR pszNewDefaultKey = NULL;
    PSTR pszStrtokState = NULL;
    BOOLEAN bChdirOk = TRUE;

    dwError = RegShellCmdParse(argc, argv, &rsItem);
    if (dwError == 0)
    {
#ifdef _DEBUG
        RegShellDumpCmdItem(rsItem);
#endif
        switch (rsItem->command)
        {
            case REGSHELL_CMD_LIST_KEYS:
                pszErrorPrefix = "list_keys: failed ";
                dwError = RegShellListKeys(pParseState, rsItem);
                if (dwError)
                {
                    PrintError(pszErrorPrefix, dwError);
                    dwError = 0;
                }
                break;

            case REGSHELL_CMD_LIST:
            case REGSHELL_CMD_DIRECTORY:
                pszErrorPrefix = "list: failed ";
                dwError = RegShellListKeys(pParseState, rsItem);
                if (dwError)
                {
                    PrintError(pszErrorPrefix, dwError);
                    dwError = 0;
                }
                printf("\n");
                dwError = RegShellListValues(pParseState, rsItem);
                if (dwError)
                {
                    PrintError(pszErrorPrefix, dwError);
                    dwError = 0;
                }
                break;

            case REGSHELL_CMD_ADD_KEY:
                pszErrorPrefix = "add_key: failed";
                dwError = RegShellAddKey(pParseState, rsItem);
                BAIL_ON_REG_ERROR(dwError);
                break;

            case REGSHELL_CMD_DELETE_KEY:
                pszErrorPrefix = "delete_key: failed ";
                dwError = RegShellDeleteKey(pParseState, rsItem);
                BAIL_ON_REG_ERROR(dwError);
                break;

            case REGSHELL_CMD_DELETE_VALUE:
                pszErrorPrefix = "delete_value: failed ";
                dwError = RegShellDeleteValue(pParseState, rsItem);
                BAIL_ON_REG_ERROR(dwError);
                break;

            case REGSHELL_CMD_DELETE_TREE:
                pszErrorPrefix = "delete_tree: failed ";
                dwError = RegShellDeleteTree(pParseState, rsItem);
                BAIL_ON_REG_ERROR(dwError);
                break;

            case REGSHELL_CMD_SET_VALUE:
                pszErrorPrefix = "set_value: failed ";
                dwError = RegShellSetValue(pParseState, rsItem);
                BAIL_ON_REG_ERROR(dwError);
                break;

            case REGSHELL_CMD_ADD_VALUE:
                pszErrorPrefix = "add_value: failed ";
                dwError = RegShellSetValue(pParseState, rsItem);
                BAIL_ON_REG_ERROR(dwError);
                break;

            case REGSHELL_CMD_HELP:
                RegShellUsage(argv[0]);
                break;

            case REGSHELL_CMD_CHDIR:
                dwError = LwAllocateString(&argv[2][1], &pszNewKeyName);
                BAIL_ON_REG_ERROR(dwError);
                pszNewKeyName [strlen(pszNewKeyName) - 1] = '\0';
                pszKeyName = pszNewKeyName;
                pszToken = strtok_r(pszKeyName, "\\/", &pszStrtokState);
                if (!pszToken && strlen(pszKeyName) > 0)
                {
                    /*
                     * Handle special case where the only thing provided in
                     * the path is one or more \ characters (e.g [\\\]). In
                     * this case, strtok_r() return NULL when parsing
                     * a non-zero length pszKeyName string. This is essentially
                     * the cd \ case
                     */
                    LwFreeMemory(pParseState->pszDefaultKey);
                    pParseState->pszDefaultKey = NULL;
                }


                if (pParseState->pszDefaultKey)
                {
                    /*
                     * Another special case when the path begins with a / or \.
                     * Force the current working directory to root.
                     */
                    if (pszNewKeyName[0] != '/' && pszNewKeyName[0] != '\\')
                    {
                        dwError = LwAllocateString(pParseState->pszDefaultKey,
                                                   &pszNewDefaultKey);
                        BAIL_ON_REG_ERROR(dwError);
                    }
                }
                while (pszToken)
                {
                    pszKeyName = NULL;
                    if (strcmp(pszToken, "..") == 0)
                    {
                        if (pszNewDefaultKey)
                        {
                            pszPwd = strrchr(pszNewDefaultKey,
                                             '\\');
                            if (pszPwd)
                            {
                                    pszPwd[0] = '\0';
                            }
                            else
                            {
                                pszPwd = strrchr(pszNewDefaultKey,
                                                 '/');
                                if (pszPwd)
                                {
                                    pszPwd[0] = '\0';
                                }
                                else
                                {
                                    LwFreeMemory(pszNewDefaultKey);
                                    pszNewDefaultKey = NULL;
                                }
                            }
                        }
                    }
                    else if (strcmp(pszToken, ".") == 0)
                    {
                        /* This is a no-op */
                    }
                    else if (strcmp(pszToken, "...") == 0)
                    {
                        /* This is a broken path! */
                        dwError = LW_ERROR_INVALID_CONTEXT;
                        BAIL_ON_REG_ERROR(dwError);

                    }
                    else if (pszNewDefaultKey)
                    {
                        /* Append this token to current relative path */
                        dwError = LwAllocateMemory(
                                      strlen(pszToken) +
                                      strlen(pszNewDefaultKey)+3,
                                      (LW_PVOID) &pszPwd);
                        BAIL_ON_REG_ERROR(dwError);
                        strcpy(pszPwd, pszNewDefaultKey);
                        strcat(pszPwd, "\\");
                        strcat(pszPwd, pszToken);
                        dwError = RegShellIsValidKey(pParseState->hReg, pszPwd);
                        if (dwError)
                        {
                            dwError = 0;
                            printf("cd: key not valid '%s'\n", pszPwd);
                            LW_SAFE_FREE_MEMORY(pszPwd);
                            pszPwd = NULL;
                            bChdirOk = FALSE;
                            break;
                        }
                        else
                        {
                            LW_SAFE_FREE_MEMORY(pszNewDefaultKey);
                            pszNewDefaultKey = pszPwd;
                            pszPwd = NULL;
                        }
                    }
                    else
                    {
                        dwError = RegShellIsValidKey(pParseState->hReg,
                                                     pszToken);
                        if (dwError)
                        {
                            dwError = 0;
                            printf("cd: key not valid '%s'\n", pszToken);
                            bChdirOk = FALSE;
                            break;
                        }
                        else
                        {
                            dwError = LwAllocateString(
                                          pszToken,
                                          &pszNewDefaultKey);
                            BAIL_ON_REG_ERROR(dwError);
                        }
                    }
                    pszToken = strtok_r(pszKeyName, "\\/", &pszStrtokState);
                }
                if (bChdirOk)
                {
                    if (pParseState->pszDefaultKey)
                    {
                        LW_SAFE_FREE_MEMORY(pParseState->pszDefaultKey);
                    }
                    pParseState->pszDefaultKey = pszNewDefaultKey;
                }
                else
                {
                    LW_SAFE_FREE_MEMORY(pszNewDefaultKey);
                }
                break;

            case REGSHELL_CMD_PWD:
                if (pParseState->pszDefaultKey)
                {
                    printf("'%s'\n\n",
                            pParseState->pszDefaultKey);
                }
                else
                {
                    printf("'%s'\n\n", "\\");
                }
                break;

            case REGSHELL_CMD_QUIT:
                exit(0);
                break;

            case REGSHELL_CMD_LIST_VALUES:
                pszErrorPrefix = "list_values: failed ";
                dwError = RegShellListValues(pParseState, rsItem);
                if (dwError)
                {
                    PrintError(pszErrorPrefix, dwError);
                    dwError = 0;
                }
                break;

            case REGSHELL_CMD_IMPORT:
                dwError = RegShellImportFile(pParseState->hReg, rsItem);
                BAIL_ON_REG_ERROR(dwError);
                break;

            default:
                break;
        }
    }
    else
    {
        printf("%s: error parsing command, invalid syntax\n", argv[0]);
        RegShellUsage(argv[0]);
    }

cleanup:
    RegShellCmdParseFree(rsItem);

    LwFreeMemory(pszNewKeyName);
    return dwError;

error:
    PrintError("regshell", dwError);
    goto cleanup;
}


DWORD
RegShellInitParseState(
    PREGSHELL_PARSE_STATE *ppParseState)
{
    DWORD dwError = 0;
    PREGSHELL_PARSE_STATE pParseState = NULL;

    BAIL_ON_INVALID_POINTER(ppParseState);


    dwError = LwAllocateMemory(
                  sizeof(REGSHELL_PARSE_STATE),
                  (LW_PVOID) &pParseState);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegOpenServer(&pParseState->hReg);

    dwError = RegLexOpen(&pParseState->lexHandle);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegIOBufferOpen(&pParseState->ioHandle);
    BAIL_ON_REG_ERROR(dwError);

    *ppParseState = pParseState;

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
RegShellCloseParseState(
    PREGSHELL_PARSE_STATE pParseState)
{
    DWORD dwError = 0;
    BAIL_ON_INVALID_POINTER(pParseState);


    RegIOClose(pParseState->ioHandle);
    RegLexClose(pParseState->lexHandle);
    RegCloseServer(pParseState->hReg);
    LwFreeMemory(pParseState);

cleanup:
    return dwError;

error:
    goto cleanup;
}


int main(int argc, char *argv[])
{
    DWORD dwError = 0;
    PCSTR pszErrorPrefix = NULL;
    DWORD dwNewArgc = 0;
    PSTR *pszNewArgv = NULL;
    CHAR cmdLine[8192] = {0};
    PSTR pszCmdLine = NULL;
    PREGSHELL_PARSE_STATE parseState = NULL;

    setlocale(LC_ALL, "");
    dwError = RegShellInitParseState(&parseState);
    BAIL_ON_REG_ERROR(dwError);

    /* Interactive shell */
    if (argc == 1)
    {
        do
        {
            printf("%s> ",
                   parseState->pszDefaultKey ?
                   parseState->pszDefaultKey : "\\");
            fflush(stdout);
            pszCmdLine = fgets(cmdLine, sizeof(cmdLine)-1, stdin);
            if (cmdLine[strlen(cmdLine)-1] == '\n')
            {
               cmdLine[strlen(cmdLine)-1] = '\0';
            }
            if (pszCmdLine)
            {
                dwError = RegIOBufferSetData(
                              parseState->ioHandle,
                              cmdLine,
                              strlen(cmdLine));
                BAIL_ON_REG_ERROR(dwError);

                dwError = RegShellCmdlineParseToArgv(
                              parseState,
                              &dwNewArgc,
                              &pszNewArgv);

                if (dwError == 0 && dwNewArgc > 0 && pszNewArgv)
                {
                    dwError = RegShellProcessCmd(parseState,
                                                 dwNewArgc,
                                                 pszNewArgv);
                    if (dwError == LW_ERROR_INVALID_CONTEXT)
                    {
                        PrintError(pszErrorPrefix, dwError);
                        dwError = 0;
                    }
                    RegShellCmdlineParseFree(dwNewArgc, pszNewArgv);
                }
                else
                {
                    printf("regshell: unable to parse command '%s'\n\n",
                           cmdLine);
                }
            }
        } while (!feof(stdin));

    }
    else
    {
        dwError = RegShellProcessCmd(parseState, argc, argv);
        BAIL_ON_REG_ERROR(dwError);
    }

cleanup:
    RegShellCloseParseState(parseState);
    return dwError;

error:
    if (dwError)
    {
        PrintError(pszErrorPrefix, dwError);
    }
    goto cleanup;
}
