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
 *        Registry export utilities (export to a win32 compatible registry file)
 *
 * Authors: Wei Fu (wfu@likewise.com)
 *          Adam Bernstein (abernstein@likewise.com)
 *
 */
#include "rsutils.h"
#include <../parse/includes.h>
#include <regclient.h>
#include <reg/reg.h>

static
DWORD
PrintToRegFile(
    IN PSTR pszKeyName,
    IN REG_DATA_TYPE dataType,
    IN PSTR pszValueName,
    IN REG_DATA_TYPE type,
    IN PVOID value,
    IN DWORD dwValueLen,
    OUT PREG_DATA_TYPE pPrevType
    )
{
    PSTR dumpString = NULL;
    DWORD dumpStringLen = 0;

    RegExportEntry(pszKeyName,
                   dataType,
                   pszValueName,
                   type,
                   value,
                   dwValueLen,
                   &dumpString,
                   &dumpStringLen);

   if (dumpStringLen > 0 && dumpString)
   {
       switch (type)
       {
           case REG_KEY:
               printf("\r\n%.*s\r\n", dumpStringLen, dumpString);
               break;

           case REG_PLAIN_TEXT:
               if (*pPrevType && *pPrevType != type)
               {
                   printf("\n");
               }
               printf("%*s ", dwValueLen, (PCHAR) value);
               break;

           default:
               printf("%.*s\r\n", dumpStringLen, dumpString);
               break;
       }
   }
   fflush(stdout);
   *pPrevType = type;

   if (dumpString)
   {
       LwFreeMemory(dumpString);
       dumpString = NULL;
   }

   return 0;
}


static
DWORD
ProcessExportedKeyInfo(
    IN HANDLE hReg,
    IN HKEY hKey,
    IN PSTR pszKeyName,
    IN OUT PREG_DATA_TYPE pPrevType
    )
{
    DWORD dwError = 0;
    DWORD dwValueNameLen = MAX_KEY_LENGTH;
    LW_WCHAR valueName[MAX_KEY_LENGTH];   // buffer for subkey name
    PSTR pszValueName = NULL;
    REG_DATA_TYPE dataType = REG_UNKNOWN;
    BYTE value[MAX_VALUE_LENGTH] = {0};
    DWORD dwValueLen = 0;
    int iCount = 0;
    DWORD dwValuesCount = 0;

    dwError = PrintToRegFile(
                          pszKeyName,
                          REG_KEY,
                          NULL,
                          REG_KEY,
                          NULL,
                          0,
                          pPrevType);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegQueryInfoKey(
        hReg,
        hKey,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        &dwValuesCount,
        NULL,
        NULL,
        NULL,
        NULL);
    BAIL_ON_REG_ERROR(dwError);

    if (!dwValuesCount)
    {
        goto cleanup;
    }

    for (iCount = 0; iCount < dwValuesCount; iCount++)
   {
       dwValueLen = MAX_VALUE_LENGTH;
       dwValueNameLen = MAX_KEY_LENGTH;

       dwError = RegEnumValue((HANDLE)hReg,
                               hKey,
                               iCount,
                               valueName,
                               &dwValueNameLen,
                               NULL,
                               &dataType,
                               value,
                               &dwValueLen);
       BAIL_ON_REG_ERROR(dwError);

       dwError = LwWc16sToMbs(valueName,
                              &pszValueName);
       BAIL_ON_REG_ERROR(dwError);

       dwError = PrintToRegFile(
                      pszKeyName,
                      dataType,
                      pszValueName,
                      dataType,
                      value,
                      dwValueLen,
                      pPrevType);
       BAIL_ON_REG_ERROR(dwError);

       LW_SAFE_FREE_STRING(pszValueName);
       memset(valueName, 0 , dwValueNameLen);
       dwValueNameLen = MAX_KEY_LENGTH;
       memset(value, 0 , dwValueLen);
       dwValueLen = MAX_VALUE_LENGTH;
   }

cleanup:
    LW_SAFE_FREE_STRING(pszValueName);
    memset(valueName, 0 , dwValueNameLen);
    memset(value, 0 , dwValueLen);

    return dwError;

error:

    goto cleanup;
}

static
DWORD
ProcessSubKeys(
    HANDLE hReg,
    HKEY hKey,
    PSTR pszKeyName,
    DWORD dwNumSubKeys,
    PREG_DATA_TYPE pPrevType
    )
{
    DWORD dwError = 0;
    int iCount = 0;
    DWORD dwSubKeyLen = MAX_KEY_LENGTH;
    LW_WCHAR subKey[MAX_KEY_LENGTH];   // buffer for subkey name
    PSTR pszSubKey = NULL;
    HKEY hSubKey = NULL;
    DWORD dwNumSubSubKeys = 0;
    CHAR c = '\\';
    //Do not free
    PSTR pszSubKeyName = NULL;
    PWSTR pSubKey = NULL;

    dwError = PrintToRegFile(
                          pszKeyName,
                          REG_KEY,
                          NULL,
                          REG_KEY,
                          NULL,
                          0,
                          pPrevType);
    BAIL_ON_REG_ERROR(dwError);

    // Get the subkeys and values under this key from registry
    for (iCount = 0; iCount < dwNumSubKeys; iCount++)
    {
        dwSubKeyLen = MAX_KEY_LENGTH;

        dwError = RegEnumKeyEx((HANDLE)hReg,
                                hKey,
                                iCount,
                                subKey,
                                &dwSubKeyLen,
                                NULL,
                                NULL,
                                NULL,
                                NULL);
        BAIL_ON_REG_ERROR(dwError);

        dwError = LwWc16sToMbs(subKey,
                               &pszSubKey);
        BAIL_ON_REG_ERROR(dwError);

        pszSubKeyName = strrchr(pszSubKey, c);

        if (LW_IS_NULL_OR_EMPTY_STR(pszSubKeyName+1))
        {
            continue;
        }

        //Open the subkey
        dwError = LwMbsToWc16s(pszSubKeyName+1, &pSubKey);
        BAIL_ON_REG_ERROR(dwError);

        dwError = RegOpenKeyEx(
            hReg,
            hKey,
            pSubKey,
            0,
            0,
            &hSubKey);
        BAIL_ON_REG_ERROR(dwError);

        dwError = RegQueryInfoKey(
            (HANDLE) hReg,
            hSubKey,
            NULL,
            NULL,
            NULL,
            &dwNumSubSubKeys,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL);
        BAIL_ON_REG_ERROR(dwError);

        dwError = RegShellUtilExport(
                        hReg,
                        hSubKey,
                        pszSubKey,
                        dwNumSubSubKeys);
        BAIL_ON_REG_ERROR(dwError);

        if (hSubKey)
        {
            dwError = RegCloseKey((HANDLE) hReg,
                                  hSubKey);
            BAIL_ON_REG_ERROR(dwError);
            hSubKey = NULL;
        }

        LW_SAFE_FREE_STRING(pszSubKey);
        memset(subKey, 0 , dwSubKeyLen);
        dwNumSubSubKeys = 0;
        LW_SAFE_FREE_MEMORY(pSubKey);
    }

cleanup:
    if (hSubKey)
    {
        RegCloseKey((HANDLE) hReg, hSubKey);
        hSubKey = NULL;
    }

    LW_SAFE_FREE_STRING(pszSubKey);
    memset(subKey, 0 , dwSubKeyLen);
    dwNumSubKeys = 0;
    LW_SAFE_FREE_MEMORY(pSubKey);

    return dwError;

error:

    goto cleanup;
}

static
DWORD
ProcessRootKeys(
    HANDLE hReg
    )
{
    DWORD dwError = 0;
    PSTR* ppszRootKeyNames = NULL;
    DWORD dwNumRootKeys = 0;
    DWORD iCount = 0;
    HKEY hRootKey = NULL;
    DWORD dwNumSubKeys = 0;


    dwError = RegEnumRootKeys(hReg,
                              &ppszRootKeyNames,
                              &dwNumRootKeys);
    BAIL_ON_REG_ERROR(dwError);

    if (!dwNumRootKeys)
    {
        goto cleanup;
    }

    for (iCount = 0; iCount < dwNumRootKeys; iCount++)
    {
        dwError = RegOpenRootKey((HANDLE) hReg,
                                 ppszRootKeyNames[iCount],
                                 &hRootKey);
        BAIL_ON_REG_ERROR(dwError);

        dwError = RegQueryInfoKey(
            (HANDLE) hReg,
            hRootKey,
            NULL,
            NULL,
            NULL,
            &dwNumSubKeys,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL);
        BAIL_ON_REG_ERROR(dwError);

        dwError = RegShellUtilExport(hReg,
                                     hRootKey,
                                     ppszRootKeyNames[iCount],
                                     dwNumSubKeys);
        BAIL_ON_REG_ERROR(dwError);

        if (hRootKey)
        {
            dwError = RegCloseKey((HANDLE) hReg,
                                          hRootKey);
            BAIL_ON_REG_ERROR(dwError);
            hRootKey = NULL;
        }
        dwNumSubKeys = 0;
    }

cleanup:
    LwFreeStringArray(ppszRootKeyNames, dwNumRootKeys);
    if (hRootKey)
    {
       RegCloseKey((HANDLE) hReg, hRootKey);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
RegShellUtilExport(
    HANDLE hReg,
    HKEY hKey,
    PSTR pszKeyName,
    DWORD dwNumSubKeys
    )
{
    DWORD dwError = 0;
    REG_DATA_TYPE prevType = REG_UNKNOWN;

    if (hKey && dwNumSubKeys == 0)
    {
        dwError = ProcessExportedKeyInfo(hReg,
                                         hKey,
                                         pszKeyName,
                                         &prevType);
        BAIL_ON_REG_ERROR(dwError);
    }
    else if (hKey && dwNumSubKeys != 0)
    {
        dwError = ProcessSubKeys(hReg,
                                 hKey,
                                 pszKeyName,
                                 dwNumSubKeys,
                                 &prevType);
        BAIL_ON_REG_ERROR(dwError);
    }
    else if (hKey == NULL && dwNumSubKeys == 0)
    {
        dwError = ProcessRootKeys(hReg);
        BAIL_ON_REG_ERROR(dwError);
    }
    else if (hKey == NULL && dwNumSubKeys != 0)
    {
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_REG_ERROR(dwError);
    }

cleanup:

    return dwError;

error:
    goto cleanup;
}
