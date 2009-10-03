/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
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
 *        test_regkeys.c
 *
 * Abstract:
 *
 *        Registry
 *
 *        Basic Key operation test
 *
 * Authors: Wei Fu (wfu@likewise.com)
 *
 */

#include "includes.h"

int main(int argc, char **argv)
{
    DWORD dwError = LW_ERROR_SUCCESS;
    HANDLE hReg = (HANDLE)NULL;
    HKEY pRootKey = NULL;
    HKEY pNewKey = NULL; //this will be the key that to be performed more operations on
    PSTR pszParentKey = "RegKeysTest";
    PSTR ppszSubBadKeys[] = {"Subkey1With\\"};
    DWORD dwNumSubkeys = 3;
    PSTR ppszSubKeys[3] = {"Subkey1", "Subkey2", "Subkey3"};
    PHKEY pSubKeys = NULL;
    int iCount = 0;
    PWSTR pSubKeyName = NULL;
    PSTR* ppszRootKeyNames = NULL;
    DWORD dwNumRootKeys = 0;

    printf("Calling: RegOpenServer\n");
    dwError = RegOpenServer(&hReg);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegEnumRootKeys((HANDLE) hReg,
                              &ppszRootKeyNames,
                              &dwNumRootKeys);
    BAIL_ON_REG_ERROR(dwError);

    for (iCount = 0; iCount < dwNumRootKeys; iCount++)
    {
        printf("Hive [%d] name is %s\n", iCount, ppszRootKeyNames[iCount]);
    }

    dwError = RegOpenRootKey(
               (HANDLE) hReg,
               LIKEWISE_ROOT_KEY,
               &pRootKey);
    BAIL_ON_REG_ERROR(dwError);

    dwError = CreateKeys(
                 hReg,
                 pRootKey,
                 pszParentKey,
                 ppszSubBadKeys,
                 1,
                 &pNewKey,
                 NULL);
    if (LW_ERROR_INVALID_KEYNAME == dwError)
    {
        dwError = CreateKeys(
                     hReg,
                     pRootKey,
                     pszParentKey,
                     ppszSubKeys,
                     dwNumSubkeys,
                     &pNewKey,
                     &pSubKeys);
        BAIL_ON_REG_ERROR(dwError);
    }
    BAIL_ON_REG_ERROR(dwError);

    printf("Enumerate keys under %s\n", pszParentKey);
    dwError = EnumKeys(hReg, pNewKey);
    BAIL_ON_REG_ERROR(dwError);

    for (iCount = 0; iCount < dwNumSubkeys; iCount++)
    {
        LW_SAFE_FREE_MEMORY(pSubKeyName);

        dwError = LwMbsToWc16s(ppszSubKeys[iCount], &pSubKeyName);
        BAIL_ON_REG_ERROR(dwError);

        printf("Delete subkey %s...\n", ppszSubKeys[iCount]);

        dwError = RegDeleteKey(
                        hReg,
                        pNewKey,
                        pSubKeyName);
        if (LW_ERROR_KEY_IS_ACTIVE == dwError)
        {
            printf("Failed delete key - Please close the key '%s' before delete it\n", ppszSubKeys[iCount]);
            dwError = 0;
        }
        BAIL_ON_REG_ERROR(dwError);

        dwError = RegCloseKey(hReg,
                              pSubKeys[iCount]);
        BAIL_ON_REG_ERROR(dwError);

        dwError = RegDeleteKey(
                        hReg,
                        pNewKey,
                        pSubKeyName);
        BAIL_ON_REG_ERROR(dwError);

        printf("The key '%s' is deleted. Enumerate keys under %s\n",  ppszSubKeys[iCount], pszParentKey);
        dwError = EnumKeys(hReg, pNewKey);
        BAIL_ON_REG_ERROR(dwError);
    }

    dwError = RegCloseKey(hReg,
                          pNewKey);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegCloseKey(hReg,
                          pRootKey);
    BAIL_ON_REG_ERROR(dwError);

finish:
    if (ppszRootKeyNames)
    {
        LwFreeStringArray(ppszRootKeyNames, dwNumRootKeys);
    }
    LW_SAFE_FREE_MEMORY(pSubKeyName);
    RegCloseServer(hReg);
    fflush(stdout);
    return dwError;
error:
    if (dwError)
    {
        RegPrintError(NULL, dwError);
    }

    dwError = dwError ? 1 : 0;

    goto finish;
}
