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
 *        test_client.c
 *
 * Abstract:
 *
 *        Registry
 *
 *        Simple Test Client
 *
 * Authors: Marc Guy (mguy@likewisesoftware.com)
 *
 */

#include "includes.h"

#define TEST_CREATE_KEY_EX          0x0001
#define TEST_CLOSE_KEY              0x0002
#define TEST_DELETE_KEY             0x0004
#define TEST_DELETE_KEY_VALUE       0x0008
#define TEST_DELETE_TREE            0x0010
#define TEST_DELETE_VALUE           0x0020
#define TEST_ENUM_KEY_EX            0x0040
#define TEST_ENUM_VALUE             0x0080
#define TEST_GET_VALUE              0x0100
#define TEST_OPEN_KEY_EX            0x0200
#define TEST_QUERY_INFO_KEY         0x0400
#define TEST_QUERY_MULTIPLE_VALUES  0x0800
#define TEST_QUERY_VALUE_EX         0x1000
#define TEST_SET_VALUE_EX           0x2000

void
Usage(
    void
    )
{
    printf("test_client -cke -ck -dk -dkv -dt -dv -eke -ev -gv -oke -qik -qmv -qve -sve\n");
    printf("  Adding an option removes it from testing. By default, all option ARE tested\n");
    printf("    cke - create key ex\n");
    printf("    ck  - close key\n");
    printf("    dk  - delete key\n");
    printf("    dkv - delete key value\n");
    printf("    dt  - delete tree\n");
    printf("    dv  - delete value\n");
    printf("    eke - enum key ex\n");
    printf("    ev  - enum value\n");
    printf("    gv  - get value\n");
    printf("    oke - open key ex\n");
    printf("    qik - query info key\n");
    printf("    qmv - query multiple values\n");
    printf("    qve - query value ex\n");
    printf("    sve - set value ex\n");
}

int main(int argc, char **argv)
{
    // Turn all options on
    int nOptions = 0xFFFF;

    argc--;
    argv++;

    while(argc)
    {
        if(!strcmp("-cke", *argv))
        {
            nOptions = nOptions & ~TEST_CREATE_KEY_EX;
        }
        else if(!strcmp("-ck", *argv))
        {
            nOptions = nOptions & ~TEST_CLOSE_KEY;
        }
        else if(!strcmp("-dk", *argv))
        {
            nOptions = nOptions & ~TEST_DELETE_KEY;
        }
        else if(!strcmp("-dkv", *argv))
        {
            nOptions = nOptions & ~TEST_DELETE_KEY_VALUE;
        }
        else if(!strcmp("-dt", *argv))
        {
            nOptions = nOptions & ~TEST_DELETE_TREE;
        }
        else if(!strcmp("-dv", *argv))
        {
            nOptions = nOptions & ~TEST_DELETE_VALUE;
        }
        else if(!strcmp("-eke", *argv))
        {
            nOptions = nOptions & ~TEST_ENUM_KEY_EX;
        }
        else if(!strcmp("-ev", *argv))
        {
            nOptions = nOptions & ~TEST_ENUM_VALUE;
        }
        else if(!strcmp("-gv", *argv))
        {
            nOptions = nOptions & ~TEST_GET_VALUE;
        }
        else if(!strcmp("-oke", *argv))
        {
            nOptions = nOptions & ~TEST_OPEN_KEY_EX;
        }
        else if(!strcmp("-qik", *argv))
        {
            nOptions = nOptions & ~TEST_QUERY_INFO_KEY;
        }
        else if(!strcmp("-qmv", *argv))
        {
            nOptions = nOptions & ~TEST_QUERY_MULTIPLE_VALUES;
        }
        else if(!strcmp("-qve", *argv))
        {
            nOptions = nOptions & ~TEST_QUERY_VALUE_EX;
        }
        else if(!strcmp("-sve", *argv))
        {
            nOptions = nOptions & ~TEST_SET_VALUE_EX;
        }

        argc--;
        argv++;
    }

    DWORD dwError = LW_ERROR_SUCCESS;
    HANDLE hReg = (HANDLE)NULL;
    HKEY pRootKey = NULL;
    HKEY pNewKey1 = NULL;
    HKEY pNewKey2 = NULL;
    HKEY pOpenedNewKey1 = NULL;
    PWSTR pSubKey = NULL;

    printf("Calling: RegOpenServer\n");
    dwError = RegOpenServer(&hReg);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegOpenRootKey(
               (HANDLE) hReg,
               LIKEWISE_ROOT_KEY,
               &pRootKey);
    BAIL_ON_REG_ERROR(dwError);

    /* Key APIs*/
    if(nOptions & TEST_CREATE_KEY_EX)
    {
        printf("Calling: RegCreateKeyEx\n");

        printf("Create TestSubKey1\n");
        dwError = LwMbsToWc16s("TestSubKey1", &pSubKey);
        BAIL_ON_REG_ERROR(dwError);

        dwError = RegCreateKeyEx(
            (HANDLE) hReg,
            pRootKey,
            pSubKey,
            (DWORD) 0,
            (PWSTR)NULL,
            (DWORD) 0,
            (REGSAM) 0,
            (PSECURITY_ATTRIBUTES) NULL,
            (PHKEY)&pNewKey1,
            (PDWORD) NULL);
        BAIL_ON_REG_ERROR(dwError);
        LW_SAFE_FREE_MEMORY(pSubKey);

        printf("Create TestSubKey2\n");
        dwError = LwMbsToWc16s("TestSubKey2", &pSubKey);
        BAIL_ON_REG_ERROR(dwError);

        dwError = RegCreateKeyEx(
            (HANDLE) hReg,
            pRootKey,
            pSubKey,
            (DWORD) 0,
            (PWSTR)NULL,
            (DWORD) 0,
            (REGSAM) 0,
            (PSECURITY_ATTRIBUTES) NULL,
            (PHKEY)&pNewKey2,
            (PDWORD) NULL);
        BAIL_ON_REG_ERROR(dwError);
        LW_SAFE_FREE_MEMORY(pSubKey);
    }

    if(nOptions & TEST_QUERY_INFO_KEY)
    {
        printf("Calling: RegQueryInfoKey\n");

        DWORD dwSubKeyCount = 0;
        DWORD dwMaxSubKeyLen = 0;

        dwError = RegQueryInfoKey(
            (HANDLE) hReg,
            (HKEY) pRootKey,
            (PWSTR) "",
            (PDWORD) NULL,
            (PDWORD) NULL,
            (PDWORD) &dwSubKeyCount,//Number of subkey
            (PDWORD) &dwMaxSubKeyLen,//longest subkey length
            (PDWORD) NULL,
            (PDWORD) NULL,
            (PDWORD) NULL,
            (PDWORD) NULL,
            (PDWORD) NULL,
            (PFILETIME) NULL
            );
        BAIL_ON_REG_ERROR(dwError);

        printf("HKEY_LIKEWISE root key has %d subkeys, longest subkey is %d long\n",
                dwSubKeyCount, dwMaxSubKeyLen);
    }


    if(nOptions & TEST_OPEN_KEY_EX)
    {
        printf("Calling: RegOpenKeyEx\n");

        dwError = RegOpenKeyEx(
            (HANDLE) hReg,
            (HKEY) pNewKey1,
            NULL,
            (DWORD) 0,
            (REGSAM) 0,
            (PHKEY) &pOpenedNewKey1
            );
        BAIL_ON_REG_ERROR(dwError);
    }

    if(nOptions & TEST_CLOSE_KEY)
    {
        printf("Calling: RegCloseKey\n");
        dwError = RegCloseKey(
            (HANDLE) hReg,
            pOpenedNewKey1
            );
        BAIL_ON_REG_ERROR(dwError);
    }

    if(nOptions & TEST_DELETE_KEY)
    {
        printf("Calling: RegDeleteKey\n");

        dwError = LwMbsToWc16s("TestSubKey1", &pSubKey);
        BAIL_ON_REG_ERROR(dwError);

        dwError = RegDeleteKey(
            (HANDLE) hReg,
            (HKEY) pRootKey,
            (PCWSTR) pSubKey
            );
        BAIL_ON_REG_ERROR(dwError);
    }

    if(nOptions & TEST_ENUM_KEY_EX)
    {
        DWORD dwSubKeyLen = MAX_KEY_LENGTH;
        LW_WCHAR subKey[MAX_KEY_LENGTH];   // buffer for subkey name

        printf("Calling: RegEnumKeyEx\n");
        dwError = RegEnumKeyEx(
            (HANDLE) hReg,
            (HKEY) pRootKey,
            (DWORD) 0,
             subKey,
             &dwSubKeyLen,
            (PDWORD) NULL,
            (PWSTR) "",
            (PDWORD) NULL,
            (PFILETIME) NULL
            );
        BAIL_ON_REG_ERROR(dwError);
    }


    PWSTR pValueName = NULL;

    /* Key Value APIs*/
    if(nOptions & TEST_SET_VALUE_EX)
    {
        dwError = LwMbsToWc16s("TestSubkey2_Value1", &pValueName);
        BAIL_ON_REG_ERROR(dwError);

        PSTR pszStr = "AB";
        PBYTE pData = NULL;
        DWORD dwDataLen = 0;

        dwError = LwHexStrToByteArray(
                       pszStr,
                       NULL,
                       &pData,
                       &dwDataLen);
        BAIL_ON_LW_ERROR(dwError);

        //Set value for "TestSubKey2_Value1"
        printf("Calling: RegSetValueEx\n");
        dwError = RegSetValueEx(
            (HANDLE) hReg,
            (HKEY) pNewKey2,
            (PCWSTR) pValueName,
            (DWORD) 0,
            (DWORD) REG_BINARY,
            (const BYTE *) pData,
            (DWORD) dwDataLen
            );
        BAIL_ON_REG_ERROR(dwError);
        LW_SAFE_FREE_MEMORY(pData);
        LW_SAFE_FREE_MEMORY(pValueName);

        //Set default value
        pszStr = "Default value";
        dwDataLen = strlen(pszStr) + 1;

        dwError = LwAllocateMemory(sizeof(*pData)*dwDataLen, (PVOID)&pData);
        BAIL_ON_REG_ERROR(dwError);

        memcpy(pData, pszStr, dwDataLen);

        printf("Calling: RegSetValueEx (set default value)\n");
        dwError = RegSetValueEx(
            (HANDLE) hReg,
            (HKEY) pNewKey2,
            NULL,
            (DWORD) 0,
            (DWORD) REG_SZ,
            (const BYTE *) pData,
            (DWORD) dwDataLen
            );
        BAIL_ON_REG_ERROR(dwError);
    }

    if(nOptions & TEST_GET_VALUE)
    {
        REG_DATA_TYPE dataType = 0;
        PSTR pszValueStr = NULL;

        printf("Calling: RegGetValue\n");
        dwError = LwMbsToWc16s("TestSubkey2_Value1", &pValueName);
        BAIL_ON_REG_ERROR(dwError);

        DWORD dwValueLen = MAX_VALUE_LENGTH;
        LW_WCHAR value[MAX_VALUE_LENGTH];   // buffer for subkey name

        dwError = RegGetValue(
            (HANDLE) hReg,
            (HKEY) pNewKey2,
            NULL,
            pValueName,
            (DWORD)RRF_RT_REG_BINARY,
            (PDWORD) &dataType,
            (PVOID) value,
            (PDWORD) &dwValueLen
            );
        BAIL_ON_REG_ERROR(dwError);

        if (REG_BINARY == dataType)
        {
            dwError = LwByteArrayToHexStr(
                             (UCHAR*)value,
                             dwValueLen,
                             &pszValueStr);
            printf("TestSubKey2_Value1 has value %s\n", pszValueStr);
        }
        LW_SAFE_FREE_MEMORY(pValueName);
    }

    if(nOptions & TEST_DELETE_KEY_VALUE)
    {
        printf("Calling: RegDeleteKeyValue\n");

        dwError = LwMbsToWc16s("TestSubkey2_Value1_not", &pValueName);
        BAIL_ON_REG_ERROR(dwError);
        dwError = RegDeleteKeyValue(
            (HANDLE) hReg,
            (HKEY) pNewKey2,
            (PCWSTR) NULL,
            (PCWSTR) pValueName
            );
        BAIL_ON_REG_ERROR(dwError);
        LW_SAFE_FREE_MEMORY(pValueName);
    }

    if(nOptions & TEST_DELETE_VALUE)
    {
        printf("Calling: RegDeleteValue\n");
        dwError = LwMbsToWc16s("TestSubkey2_Value1", &pValueName);
        BAIL_ON_REG_ERROR(dwError);
        dwError = RegDeleteValue(
            (HANDLE) hReg,
            (HKEY) pNewKey2,
            (PCWSTR)pValueName
            );
        BAIL_ON_REG_ERROR(dwError);
        LW_SAFE_FREE_MEMORY(pValueName);
    }

    if(nOptions & TEST_DELETE_TREE)
    {
        printf("Calling: RegDeleteTree\n");
        dwError = RegDeleteTree(
            (HANDLE) hReg,
            (HKEY) NULL,
            (PCWSTR) ""
            );
        BAIL_ON_REG_ERROR(dwError);
    }

    if(nOptions & TEST_ENUM_VALUE)
    {
        printf("Calling: RegEnumValue\n");
        dwError = RegEnumValue(
            (HANDLE) hReg,
            (HKEY) NULL,
            (DWORD) 0,
            (PWSTR) "",
            (PDWORD) NULL,
            (PDWORD) NULL,
            (PDWORD) NULL,
            (PBYTE) NULL,
            (PDWORD) NULL
            );
        BAIL_ON_REG_ERROR(dwError);
    }

#if 0
    if(nOptions & TEST_QUERY_MULTIPLE_VALUES)
    {
        printf("Calling: RegQueryMultipleValues\n");
        dwError = RegQueryMultipleValues(
            (HANDLE) hReg,
            (HKEY) NULL,
            (PVALENT) NULL,
            (DWORD) 0,
            (PWSTR) "",
            (PDWORD) NULL
            );
        BAIL_ON_REG_ERROR(dwError);
    }
#endif

    if(nOptions & TEST_QUERY_VALUE_EX)
    {
        printf("Calling: RegQueryValueEx\n");
        dwError = RegQueryValueEx(
            (HANDLE) hReg,
            (HKEY) NULL,
            (PCWSTR) "",
            (PDWORD) NULL,
            (PDWORD) NULL,
            (PBYTE) NULL,
            (PDWORD) NULL
            );
        BAIL_ON_REG_ERROR(dwError);
    }

finish:
    RegCloseServer(hReg);
    fflush(stdout);
    return dwError;
error:
    if (dwError)
    {
        PrintError(NULL, dwError);
    }

    dwError = dwError ? 1 : 0;

    goto finish;
}
