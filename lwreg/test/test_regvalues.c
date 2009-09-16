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
 *        test_regqueryvals.c
 *
 * Abstract:
 *
 *        Registry
 *
 *        Simple Test Client
 *
 * Authors: Wei Fu (wfu@likewise.com)
 *
 */

#include "includes.h"

static
PSTR
GetTypeStr(
    REG_DATA_TYPE type
    )
{
    PSTR pszType = NULL;

    switch (type)
    {
        case REG_BINARY:
            pszType = "REG_BINARY";

            break;

        case REG_SZ:
            pszType = "REG_SZ";

            break;

        case REG_DWORD:
            pszType = "REG_DWORD";

            break;

        default:
            pszType = "REG_UNKNOWN_TYPE";
    }

    return pszType;
}

static
DWORD GetTypeFlags(
    REG_DATA_TYPE type
    )
{
    DWORD dwFlags = RRF_RT_REG_NONE;

    switch(type)
    {
    case REG_BINARY:
        dwFlags = RRF_RT_REG_BINARY;
        break;

    case REG_SZ:
        dwFlags = RRF_RT_REG_SZ;
        break;

    case REG_MULTI_SZ:
        dwFlags = RRF_RT_REG_MULTI_SZ;
        break;
    case REG_DWORD:
        dwFlags = RRF_RT_REG_DWORD;
        break;

    default:
        dwFlags = RRF_RT_REG_NONE;
    }

    return dwFlags;
}

static
DWORD
TestQueryMultiVals(
    IN HANDLE hReg,
    IN HKEY hKey,
    IN PVALENT pVal_list,
    IN DWORD dwValueNum
    )
{
    DWORD dwError = 0;
    PWSTR pValue = NULL;
    DWORD dwTotalSize = MAX_VALUE_LENGTH*dwValueNum;
    int iCount = 0;
    PWSTR pSingleValue = NULL;
    PSTR pszValueName = NULL;
    PSTR pszValueRetrieved = NULL;

    /* get the pValue buffer ready */
    dwError = LwAllocateMemory(sizeof(*pValue)*MAX_VALUE_LENGTH*dwValueNum, (PVOID)&pValue);
    BAIL_ON_REG_ERROR(dwError);

    printf("Calling RegQueryMultipleValues\n");

    dwError = RegQueryMultipleValues(
            (HANDLE) hReg,
            (HKEY)hKey,
            pVal_list,
            dwValueNum,
            pValue,
            &dwTotalSize);
    BAIL_ON_REG_ERROR(dwError);

    printf("Total buffer size returned is %d\n", dwTotalSize);

    for (iCount = 0; iCount < dwValueNum; iCount++)
    {
        dwError = LwWc16sToMbs(pVal_list[iCount].ve_valuename, &pszValueName);
        BAIL_ON_REG_ERROR(dwError);

        dwError = LwAllocateMemory(sizeof(*pSingleValue)*(pVal_list[iCount].ve_valuelen+1), (PVOID)&pSingleValue);
        BAIL_ON_REG_ERROR(dwError);

        memcpy(pSingleValue, (PWSTR)pVal_list[iCount].ve_valueptr, pVal_list[iCount].ve_valuelen*sizeof(*pSingleValue));

        dwError = LwWc16sToMbs(pSingleValue, &pszValueRetrieved);
        BAIL_ON_REG_ERROR(dwError);

        printf("Value '%s' has value '%s' with type '%s'\n", pszValueName, pszValueRetrieved, GetTypeStr((REG_DATA_TYPE)pVal_list[iCount].ve_type));

        LW_SAFE_FREE_STRING(pszValueName);
        LW_SAFE_FREE_STRING(pszValueRetrieved);
        LW_SAFE_FREE_MEMORY(pSingleValue);
    }

cleanup:
    LW_SAFE_FREE_STRING(pszValueName);
    LW_SAFE_FREE_STRING(pszValueRetrieved);
    LW_SAFE_FREE_MEMORY(pSingleValue);
    LW_SAFE_FREE_MEMORY(pValue);

    return dwError;

error:
    goto cleanup;
}

typedef enum __REG_GET_VALUE_FUNC
{
    RegGetValueFunc = 0,
    RegQueryValueExFunc = 1,
} REG_GET_VALUE_FUNC;

static
DWORD
FillValueData(
    IN REG_DATA_TYPE type,
    IN PSTR pszName,
    IN OUT PREG_VALUE pValue,
    IN OUT PVALENT pVal_list,
    IN DWORD dwValNum
    )
{
    DWORD dwError = 0;
    PSTR pszValue = NULL;
    PSTR pszMadeUpBinData = "ABC0";
    DWORD dwSampleDwordVal = 888888;

    BAIL_ON_INVALID_POINTER(pValue);
    BAIL_ON_INVALID_POINTER(pszName);

    if (strcmp(pszName, "@"))
    {
        dwError = LwMbsToWc16s(pszName, &pValue->pwszValueName);
        BAIL_ON_REG_ERROR(dwError);
    }

    dwError = LwMbsToWc16s(pszName, &pVal_list->ve_valuename);
    BAIL_ON_REG_ERROR(dwError);

    pValue->type = type;

    switch (type)
    {
    case REG_SZ:
        dwError = LwAllocateStringPrintf(
                     &pszValue,
                     "value of %s",
                     pszName);
        BAIL_ON_REG_ERROR(dwError);

        pValue->dwDataLen = strlen(pszValue);

        dwError = LwAllocateMemory(sizeof(*pValue->pData)*(pValue->dwDataLen), (PVOID)&pValue->pData);
        BAIL_ON_REG_ERROR(dwError);

        memcpy(pValue->pData, pszValue, pValue->dwDataLen*sizeof(*pszValue));

        break;

    case REG_BINARY:
        dwError = LwHexStrToByteArray(
                       pszMadeUpBinData,
                       NULL,
                       &pValue->pData,
                       &pValue->dwDataLen);
        BAIL_ON_REG_ERROR(dwError);

        break;

    case REG_DWORD:
        pValue->dwDataLen = sizeof(DWORD)/sizeof(BYTE);

        dwError = LwAllocateMemory(sizeof(*pValue->pData)*pValue->dwDataLen, (PVOID)&pValue->pData);
        BAIL_ON_REG_ERROR(dwError);

        ConvertDwordToByteArray(dwSampleDwordVal, pValue->pData, pValue->dwDataLen);

        break;

    default:
        dwError = LW_ERROR_UNKNOWN_DATA_TYPE;
        BAIL_ON_REG_ERROR(dwError);
    }

cleanup:
    LW_SAFE_FREE_STRING(pszValue);

    return dwError;
error:
    goto cleanup;
}

static
DWORD
TestRegGetValue(
   HANDLE hReg,
   HKEY hKey,
   PVALENT val_list,
   DWORD num_vals,
   REG_GET_VALUE_FUNC func
   )
{
    int iCount = 0;
    DWORD dwError = 0;
    PSTR pszValue = NULL;

    for (iCount = 0; iCount < num_vals; iCount++)
    {
        DWORD dwValueLen = MAX_VALUE_LENGTH;
        BYTE value[MAX_VALUE_LENGTH];   // buffer for subkey name
        REG_DATA_TYPE dataType;

        switch (func)
        {
            case RegGetValueFunc:

                printf("Calling: RegGetValue\n");

                dwError = RegGetValue(
                    (HANDLE) hReg,
                    (HKEY) hKey,
                    NULL,
                    val_list[iCount].ve_valuename,
                    GetTypeFlags((REG_DATA_TYPE)val_list[iCount].ve_type),
                    (PDWORD) &dataType,
                    (PVOID) value,
                    (PDWORD) &dwValueLen
                    );
                BAIL_ON_REG_ERROR(dwError);

                break;

            case RegQueryValueExFunc:

                printf("Calling: RegQueryValueEx\n");

                dwError = RegQueryValueEx(
                    (HANDLE) hReg,
                    (HKEY) hKey,
                    val_list[iCount].ve_valuename,
                    NULL,
                    (PDWORD) &dataType,
                    (PVOID) value,
                    (PDWORD) &dwValueLen
                    );
                BAIL_ON_REG_ERROR(dwError);

                break;

            default:
                dwError = LW_ERROR_INVALID_PARAMETER;
                BAIL_ON_REG_ERROR(dwError);
        }

        dwError = GetValueAsStr(dataType,
                                value,
                                dwValueLen,
                                &pszValue);
        BAIL_ON_REG_ERROR(dwError);

        printf("Value %d has value '%s' with type %s\n", iCount, pszValue, GetTypeStr((REG_DATA_TYPE)val_list[iCount].ve_type));

        memset(value, 0, MAX_VALUE_LENGTH);
        LW_SAFE_FREE_STRING(pszValue);
        dwValueLen = MAX_VALUE_LENGTH;
    }
cleanup:

    return dwError;

error:

    goto cleanup;
}

static
DWORD
TestRegDeleteValues(
   HANDLE hReg,
   HKEY hKey,
   PVALENT val_list,
   DWORD num_vals
   )
{
    int iCount = 0;
    DWORD dwError = 0;

    for (iCount = 0; iCount < num_vals; iCount++)
    {
        printf("Calling: RegDeleteValue\n");

        dwError = EnumValues(hReg,hKey);
        BAIL_ON_REG_ERROR(dwError);

        dwError = RegDeleteValue(
            (HANDLE) hReg,
            (HKEY) hKey,
            val_list[iCount].ve_valuename
            );
        BAIL_ON_REG_ERROR(dwError);

        printf("Value %d is deleted.\n", iCount);

        dwError = EnumValues(hReg,hKey);
        BAIL_ON_REG_ERROR(dwError);
    }
cleanup:

    return dwError;

error:

    goto cleanup;
}

/*
static
DWORD
TestRegDeleteKeyValue(
   HANDLE hReg,
   HKEY hKey,
   HKEY hSubKey,
   PWSTR pwcsubKeyName
   )
{
    DWORD dwError = 0;
    DWORD dwSubKeyValueNum = 2;
    PREG_VALUE pRegValue = NULL;
    PREG_VALUE* ppSubKeyValues = NULL;
    PSTR ppszSubKeyValueNames[] = {"subkey1_dword", "subkey1_str1"};
    REG_DATA_TYPE subkeytype[] = {REG_DWORD, REG_SZ};
    PVALENT pSubKeyVal_list = NULL;
    int iCount = 0;


    dwError = LwAllocateMemory(sizeof(*ppSubKeyValues)*dwSubKeyValueNum, (PVOID)&ppSubKeyValues);
    BAIL_ON_REG_ERROR(dwError);

    dwError = LwAllocateMemory(sizeof(*pSubKeyVal_list)*dwSubKeyValueNum, (PVOID)&pSubKeyVal_list);
    BAIL_ON_REG_ERROR(dwError);

    for (iCount = 0; iCount < dwSubKeyValueNum; iCount++)
    {
        dwError = LwAllocateMemory(sizeof(*pRegValue), (PVOID)&pRegValue);
        BAIL_ON_REG_ERROR(dwError);

        dwError = FillValueData(subkeytype[iCount],
                                ppszSubKeyValueNames[iCount],
                                pRegValue,
                                pSubKeyVal_list+iCount,
                                dwSubKeyValueNum);
        BAIL_ON_REG_ERROR(dwError);

        ppSubKeyValues[iCount] = pRegValue;
        pRegValue = NULL;
        FreeRegValue(&pRegValue);
    }

    dwError = SetValues(
                 hReg,
                 hSubKey,
                 ppSubKeyValues,
                 dwSubKeyValueNum);
    BAIL_ON_REG_ERROR(dwError);

    dwError = EnumValues(hReg,hSubKey);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegDeleteKeyValue(hReg,
                                hKey,
                                pwcsubKeyName,
                                pSubKeyVal_list[0].ve_valuename
                                );
    BAIL_ON_REG_ERROR(dwError);

    dwError = EnumValues(hReg,hSubKey);
    BAIL_ON_REG_ERROR(dwError);

cleanup:
    for (iCount = 0; iCount < dwSubKeyValueNum; iCount++)
        FreePVALENT(&(pSubKeyVal_list[iCount]));
    LW_SAFE_FREE_MEMORY(pSubKeyVal_list);

    FreeRegValueList(ppSubKeyValues, dwSubKeyValueNum);

    return dwError;

error:

    goto cleanup;
}
*/

int main(int argc, char **argv)
{
    DWORD dwError = LW_ERROR_SUCCESS;
    HANDLE hReg = (HANDLE)NULL;
    HKEY pRootKey = NULL;
    HKEY pNewKey = NULL;
    PHKEY pSubKeys = NULL;
    PSTR pszParentKey = "RegQueryMultiValuesTest";
    PSTR ppszSubKeys[] = {"Subkey1", "Subkey1"};

    DWORD dwValueNum = 6;
    PREG_VALUE pRegValue = NULL;
    PREG_VALUE* ppValues = NULL;
    PSTR ppszValueNames[] = {"@", "str1","dword1","bin1","dword2","str2"};
    REG_DATA_TYPE type[] = {REG_SZ, REG_SZ, REG_DWORD, REG_BINARY, REG_DWORD, REG_SZ};
    PVALENT pVal_list = NULL;

    int iCount = 0;
    DWORD dwIndex_subkey = 0;
    PWSTR pKey = NULL;


    printf("Calling: RegOpenServer\n");
    dwError = RegOpenServer(&hReg);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegOpenRootKey(
               (HANDLE) hReg,
               LIKEWISE_ROOT_KEY,
               &pRootKey);
    BAIL_ON_REG_ERROR(dwError);

    dwError = CreateKeys(
                 hReg,
                 pRootKey,
                 pszParentKey,
                 ppszSubKeys,
                 2,
                 &pNewKey,
                 &pSubKeys);
    BAIL_ON_REG_ERROR(dwError);

    dwError = LwMbsToWc16s(ppszSubKeys[dwIndex_subkey], &pKey);
    BAIL_ON_REG_ERROR(dwError);

/*    printf("----------Testing RegDeleteKeyValue API Begin---------------\n");

    dwError = TestRegDeleteKeyValue(hReg,
                                    pNewKey,
                                    pSubKeys[dwIndex_subkey],
                                    pKey);
    BAIL_ON_REG_ERROR(dwError);

    LW_SAFE_FREE_MEMORY(pKey);

    printf("----------Testing RegDeleteKeyValue API End------------------\n");

    */


    // Set up values for hNewKey
    dwError = LwAllocateMemory(sizeof(*ppValues)*dwValueNum, (PVOID)&ppValues);
    BAIL_ON_REG_ERROR(dwError);

    dwError = LwAllocateMemory(sizeof(*pVal_list)*dwValueNum, (PVOID)&pVal_list);
    BAIL_ON_REG_ERROR(dwError);

    for (iCount = 0; iCount < dwValueNum; iCount++)
    {
        dwError = LwAllocateMemory(sizeof(*pRegValue), (PVOID)&pRegValue);
        BAIL_ON_REG_ERROR(dwError);

        dwError = FillValueData(type[iCount],
                                ppszValueNames[iCount],
                                pRegValue,
                                pVal_list+iCount,
                                dwValueNum);
        BAIL_ON_REG_ERROR(dwError);

        ppValues[iCount] = pRegValue;
        pRegValue = NULL;
        FreeRegValue(&pRegValue);
    }

    dwError = SetValues(
                 hReg,
                 pNewKey,
                 ppValues,
                 dwValueNum);
    BAIL_ON_REG_ERROR(dwError);

    dwError = EnumValues(hReg,pNewKey);
    BAIL_ON_REG_ERROR(dwError);

    printf("----------Testing RegQueryMultiValuesEx API Begin-----------\n");

    dwError = TestQueryMultiVals(
                 hReg,
                 pNewKey,
                 pVal_list,
                 dwValueNum);
    BAIL_ON_REG_ERROR(dwError);

    printf("----------Testing RegQueryMultiValuesEx API End-----------\n");


    printf("----------Testing RegGetValue API Begin-------------------\n");

    dwError = TestRegGetValue(hReg,
                              pNewKey,
                              pVal_list,
                              dwValueNum,
                              RegGetValueFunc);
    BAIL_ON_REG_ERROR(dwError);

    printf("----------Testing RegGetValue API End---------------------\n");

    printf("----------Testing RegQueryValueEx API Begin---------------\n");

    dwError = TestRegGetValue(hReg,
                              pNewKey,
                              pVal_list,
                              dwValueNum,
                              RegQueryValueExFunc);
    BAIL_ON_REG_ERROR(dwError);

    printf("----------Testing RegQueryValueEx API End-----------------\n");

    printf("----------Testing RegDeleteValue API Begin----------------\n");

    dwError = TestRegDeleteValues(hReg,
                                 pNewKey,
                                 pVal_list,
                                 dwValueNum);
    BAIL_ON_REG_ERROR(dwError);

    printf("----------Testing RegDeleteValue API End------------------\n");

    dwError = RegCloseKey(hReg,
                          pNewKey);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegCloseKey(hReg,
                          pRootKey);
    BAIL_ON_REG_ERROR(dwError);

finish:
    if (pVal_list)
    {
        for (iCount = 0; iCount < dwValueNum; iCount++)
            FreePVALENT(&(pVal_list[iCount]));
    }
    LW_SAFE_FREE_MEMORY(pVal_list);

    FreeRegValueList(ppValues, dwValueNum);
    LW_SAFE_FREE_MEMORY(pKey);

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
