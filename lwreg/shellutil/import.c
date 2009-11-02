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
 *        import.c
 *
 * Abstract:
 *
 *        Registry
 *
 *        Registry import utilities (import a win32 compatible registry file)
 *        Originally implemented for regimport (regimport.c)
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
ProcessImportedKeyName(
    HANDLE hReg,
    PHKEY phRootKey,
    PCSTR pszKeyName
    )
{
    DWORD dwError = 0;
    HKEY hRootKey = *phRootKey;

    PCSTR pszDelim = "\\";
    PWSTR pSubKey = NULL;
    HKEY pNewKey = NULL;
    PSTR pszCurrKeyName = NULL;
    PSTR pszPrevKeyName = NULL;
    PSTR pszKeyToken = NULL;
    PSTR pszStrTokSav = NULL;
    HKEY hCurrKey = NULL;
    PSTR pszKeyNameCopy = NULL;
    BOOLEAN bIsRootKey = TRUE;


    dwError = LwAllocateString(pszKeyName, &pszKeyNameCopy);
    BAIL_ON_REG_ERROR(dwError);

    pszKeyToken = strtok_r (pszKeyNameCopy, pszDelim, &pszStrTokSav);
    if (!LW_IS_NULL_OR_EMPTY_STR(pszKeyToken))
    {
        if (!strcmp(pszKeyToken, LIKEWISE_ROOT_KEY))
        {
            if (!hRootKey)
            {
                dwError = RegOpenKeyExA(
                           hReg,
                           NULL,
                           LIKEWISE_ROOT_KEY,
                           0,
                           0,
                           &hRootKey);
                BAIL_ON_REG_ERROR(dwError);
            }
            hCurrKey = hRootKey;
        }
        else
        {
            REG_LOG_DEBUG("Found invalid Key Path in REG file that is imported");
            goto cleanup;
        }
    }

    pszKeyToken = strtok_r (NULL, pszDelim, &pszStrTokSav);
    while (!LW_IS_NULL_OR_EMPTY_STR(pszKeyToken))
    {
        dwError = LwMbsToWc16s(pszKeyToken, &pSubKey);
        BAIL_ON_REG_ERROR(dwError);

        dwError = RegCreateKeyExW(
            (HANDLE) hReg,
            (HKEY) hCurrKey,
            pSubKey,
            (DWORD) 0,
            (PWSTR) "",
            (DWORD) 0,
            (REGSAM) 0,
            (PSECURITY_ATTRIBUTES) NULL,
            (PHKEY) &pNewKey,
            (PDWORD) NULL
            );
        BAIL_ON_REG_ERROR(dwError);

        LW_SAFE_FREE_MEMORY(pSubKey);

        if (hCurrKey && !bIsRootKey)
        {
          dwError = RegCloseKey(hReg,hCurrKey);
          BAIL_ON_REG_ERROR(dwError);
          hCurrKey = NULL;
        }

        hCurrKey = pNewKey;

        pszKeyToken = strtok_r (NULL, pszDelim, &pszStrTokSav);
    }

    *phRootKey = hRootKey;

cleanup:
    LW_SAFE_FREE_STRING(pszCurrKeyName);
    LW_SAFE_FREE_STRING(pszPrevKeyName);
    LW_SAFE_FREE_MEMORY(pSubKey);
    LW_SAFE_FREE_STRING(pszKeyNameCopy);

    if (pNewKey)
    {
      dwError = RegCloseKey(hReg,pNewKey);
    }

    return dwError;

error:
    if (hRootKey)
    {
        dwError = RegCloseKey(hReg,hRootKey);
    }

    *phRootKey = NULL;

    goto cleanup;
}

static
DWORD
ProcessImportedValue(
    HANDLE hReg,
    PHKEY phRootKey,
    PREG_PARSE_ITEM pItem
    )
{
    DWORD dwError = 0;
    CHAR c = '\\';
    PCSTR pszDelim = "\\";
    PSTR pszKeyToken = NULL;
    PSTR pszStrTokSav = NULL;
    //Do not free
    PSTR pszSubKeyName = NULL;
    PWSTR pSubKey = NULL;
    //Do not close
    HKEY hKey = NULL;
    //Do not close
    HKEY hCurrRootKey = NULL;
    HKEY hRootKey = NULL;
    HKEY hSubKey = NULL;
    PSTR pszKeyName = NULL;

    BAIL_ON_INVALID_HANDLE(hReg);

    dwError = LwAllocateString((PCSTR)pItem->keyName, &pszKeyName);
    BAIL_ON_REG_ERROR(dwError);

    pszKeyToken = strtok_r (pszKeyName, pszDelim, &pszStrTokSav);

    if (LW_IS_NULL_OR_EMPTY_STR(pszKeyToken))
    {
        pszKeyToken = pszKeyName;
    }

    if (!strcmp(pszKeyToken, LIKEWISE_ROOT_KEY))
    {
        if (!hRootKey)
        {
            dwError = RegOpenKeyExA(hReg, NULL, LIKEWISE_ROOT_KEY, 0, 0, &hRootKey);
            BAIL_ON_REG_ERROR(dwError);
        }

        hCurrRootKey = hRootKey;
    }
    else
    {
        REG_LOG_DEBUG("Found invalid Key Path in REG file that is imported");
        goto cleanup;
    }

    pszSubKeyName = strchr(pItem->keyName, c);

    if (pszSubKeyName && !LW_IS_NULL_OR_EMPTY_STR(pszSubKeyName+1))
    {
        //Open the subkey
        dwError = LwMbsToWc16s(pszSubKeyName+1, &pSubKey);
        BAIL_ON_REG_ERROR(dwError);

        dwError = RegOpenKeyExW(
            hReg,
            hCurrRootKey,
            pSubKey,
            0,
            0,
            &hSubKey);
        BAIL_ON_REG_ERROR(dwError);

        hKey = hSubKey;
    }
    else
    {
        hKey = hCurrRootKey;
    }

    dwError = RegSetValueExA(
        hReg,
        hKey,
        pItem->valueName,
        0,
        pItem->type,
        pItem->value,
        pItem->valueLen+1);
    BAIL_ON_REG_ERROR(dwError);

    *phRootKey = hRootKey;

cleanup:
    LW_SAFE_FREE_MEMORY(pSubKey);
    LW_SAFE_FREE_STRING(pszKeyName);

    if (hSubKey)
    {
        dwError = RegCloseKey(hReg, hSubKey);
    }

    return dwError;

error:
    if (hRootKey)
    {
        dwError = RegCloseKey(hReg,hRootKey);
    }

    *phRootKey = NULL;

    goto cleanup;
}

DWORD RegShellUtilImportCallback(PREG_PARSE_ITEM pItem, HANDLE hReg)
{
    DWORD dwError = 0;
    HKEY pRootKey = NULL;

    if (pItem->type == REG_KEY)
    {
        dwError = ProcessImportedKeyName(
                hReg,
                &pRootKey,
                (PCSTR)pItem->keyName);
        BAIL_ON_REG_ERROR(dwError);
    }
    else
    {
        dwError = ProcessImportedValue(
                 hReg,
                 &pRootKey,
                 pItem);
        BAIL_ON_REG_ERROR(dwError);
    }

    if (pRootKey)
    {
        dwError = RegCloseKey(hReg,pRootKey);
        BAIL_ON_REG_ERROR(dwError);
    }

cleanup:

    return dwError;

error:
    goto cleanup;
}
