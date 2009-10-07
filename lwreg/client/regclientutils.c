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
 *        testutils.c
 *
 * Abstract:
 *
 *        Registry
 *
 * Authors: Wei Fu (wfu@likewise.com)
 *
 */

#include "client.h"

DWORD
GetValueAsStr(
    IN REG_DATA_TYPE dataType,
    IN PBYTE value,
    IN BOOLEAN bDoAnsi,
    IN DWORD dwValueLen,
    OUT PSTR* ppszValue
    )
{
    DWORD dwError = 0;
    PSTR pszValue = NULL;
    DWORD dwValue = 0;

    switch (dataType)
     {
     case REG_BINARY:
         dwError = LwByteArrayToHexStr(
                          (UCHAR*)value,
                          dwValueLen,
                          &pszValue);
         BAIL_ON_REG_ERROR(dwError);

         break;

     case REG_SZ:
         dwError = LwAllocateMemory(sizeof(*pszValue)*(dwValueLen+1), (PVOID)&pszValue);
         BAIL_ON_REG_ERROR(dwError);

         memcpy(pszValue, value, dwValueLen*sizeof(*value));

         break;

     case REG_DWORD:
         dwValue = *(DWORD*)&value[0];

         dwError = LwAllocateMemory(sizeof(*pszValue)*(sizeof(dwValue)), (PVOID)&pszValue);
         BAIL_ON_REG_ERROR(dwError);

         sprintf(pszValue, "%d", dwValue);

         break;

     default:
         dwError = LW_ERROR_UNKNOWN_DATA_TYPE;
         BAIL_ON_REG_ERROR(dwError);
     }

     *ppszValue = pszValue;

cleanup:
    return dwError;

error:
    *ppszValue = NULL;
    LW_SAFE_FREE_STRING(pszValue);

    goto cleanup;
}

DWORD
CreateKeys(
    IN HANDLE hReg,
    IN HKEY pRootKey,
    IN PSTR pszParentKey,
    IN PSTR* ppszSubKeys,
    IN DWORD dwSubKeyNum,
    OUT PHKEY phkParentKey,
    OUT OPTIONAL PHKEY* pphkSubKeys
    )
{
    DWORD dwError = 0;
    PWSTR pKey = NULL;
    HKEY pNewKey = NULL; //this will be the key that to be performed more operations on
    PHKEY phkSubKeys = NULL;
    int iCount = 0;

    BAIL_ON_INVALID_POINTER(ppszSubKeys);

    if (LW_IS_NULL_OR_EMPTY_STR(pszParentKey))
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_REG_ERROR(dwError);
    }

    printf("Create key - %s\n", pszParentKey);
    dwError = LwMbsToWc16s(pszParentKey, &pKey);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegCreateKeyEx(
        (HANDLE) hReg,
        pRootKey,
        pKey,
        (DWORD) 0,
        (PWSTR)NULL,
        (DWORD) 0,
        (REGSAM) 0,
        (PSECURITY_ATTRIBUTES) NULL,
        (PHKEY)&pNewKey,
        (PDWORD) NULL);
    BAIL_ON_REG_ERROR(dwError);
    LW_SAFE_FREE_MEMORY(pKey);

    dwError =  LwAllocateMemory(sizeof(*phkSubKeys)*dwSubKeyNum, (PVOID)&phkSubKeys);
    BAIL_ON_REG_ERROR(dwError);

    for (iCount = 0; iCount < dwSubKeyNum; iCount++)
    {
        if (LW_IS_NULL_OR_EMPTY_STR(ppszSubKeys[iCount]))
        {
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_REG_ERROR(dwError);
        }

        printf("Create sub key %d - %s\n", iCount, ppszSubKeys[iCount]);
        dwError = LwMbsToWc16s(ppszSubKeys[iCount], &pKey);
        BAIL_ON_REG_ERROR(dwError);

        dwError = RegCreateKeyEx(
            (HANDLE) hReg,
            pNewKey,
            pKey,
            (DWORD) 0,
            (PWSTR)NULL,
            (DWORD) 0,
            (REGSAM) 0,
            (PSECURITY_ATTRIBUTES) NULL,
            &phkSubKeys[iCount],
            (PDWORD) NULL);
        BAIL_ON_REG_ERROR(dwError);
        LW_SAFE_FREE_MEMORY(pKey);
    }

    *phkParentKey = pNewKey;

    if (pphkSubKeys)
    {
        *pphkSubKeys = phkSubKeys;
    }

cleanup:
    if (!pphkSubKeys && phkSubKeys)
    {
        for (iCount = 0; iCount < dwSubKeyNum; iCount++)
        {
            if (!phkSubKeys[iCount])
                continue;

            dwError = RegCloseKey(hReg,
                                  phkSubKeys[iCount]);
            BAIL_ON_REG_ERROR(dwError);
            phkSubKeys[iCount] = NULL;
        }
        LW_SAFE_FREE_MEMORY(phkSubKeys);
    }

    return dwError;

error:
    *phkParentKey = NULL;
    if (pphkSubKeys)
    {
        *pphkSubKeys = NULL;
    }

    goto cleanup;
}

DWORD
EnumKeys(
    HANDLE hReg,
    HKEY hKey
    )
{
    DWORD dwError = 0;
    DWORD dwSubKeyCount = 0;
    DWORD dwMaxSubKeyLen = 0;

    dwError = RegQueryInfoKey(
           (HANDLE) hReg,
           (HKEY) hKey,
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

   if (dwSubKeyCount)
   {
       int i = 0;
       DWORD dwSubKeyLen = MAX_KEY_LENGTH;
       LW_WCHAR subKey[MAX_KEY_LENGTH];   // buffer for subkey name
       PSTR pszSubKey = NULL;

       printf( "Number of subkeys: %d\n", dwSubKeyCount);

       for (i = 0; i < dwSubKeyCount; i++)
       {
           dwSubKeyLen = MAX_KEY_LENGTH;

           dwError = RegEnumKeyEx((HANDLE)hReg,
                                   hKey,
                                   i,
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

           printf("SubKey %d name is %s\n", i, pszSubKey);

           LW_SAFE_FREE_STRING(pszSubKey);
           memset(subKey, 0 , dwSubKeyLen);
       }
   }

error:
   return dwError;
}

DWORD
EnumValues(
    HANDLE hReg,
    HKEY hKey
    )
{
    DWORD dwError = 0;
    DWORD dwValueCount = 0;
    DWORD dwMaxValueNameLen = 0;
    DWORD dwMaxValueLen = 0;

    dwError = RegQueryInfoKey(
           (HANDLE) hReg,
           (HKEY) hKey,
           (PWSTR) "",
           (PDWORD) NULL,
           (PDWORD) NULL,
           (PDWORD) NULL,//Number of subkey
           (PDWORD) NULL,//longest subkey length
           (PDWORD) NULL,
           (PDWORD) &dwValueCount, //Number of value counts
           (PDWORD) &dwMaxValueNameLen, //longest value name length
           (PDWORD) &dwMaxValueLen, // longest value length
           (PDWORD) NULL,
           (PFILETIME) NULL
           );
   BAIL_ON_REG_ERROR(dwError);

   printf("Total number of values is %d  max value name length is %d and max value length is %d\n", dwValueCount, dwMaxValueNameLen, dwMaxValueLen);

   if (dwValueCount)
   {
       int i = 0;
       DWORD dwValueNameLen = MAX_KEY_LENGTH;
       LW_WCHAR valueName[MAX_KEY_LENGTH];   // buffer for subkey name
       PSTR pszValueName = NULL;
       REG_DATA_TYPE dataType = REG_UNKNOWN;
       BYTE value[MAX_VALUE_LENGTH] = {0};
       DWORD dwValueLen = 0;
       PSTR pszValue = NULL;


       printf("Calling RegEnumValue with NULL pData buffer\n");

       dwError = RegEnumValue((HANDLE)hReg,
                              hKey,
                              dwValueCount-1,
                              valueName,
                              &dwValueNameLen,
                              NULL,
                              &dataType,
                              NULL,
                              &dwValueLen);
       BAIL_ON_REG_ERROR(dwError);

       printf("The returned value length is %d\n", dwValueLen);

       printf("Number of values: %d\n", dwValueCount);

       for (i = 0; i < dwValueCount; i++)
       {
           dwValueLen = MAX_VALUE_LENGTH;
           dwValueNameLen = MAX_KEY_LENGTH;

           dwError = RegEnumValue((HANDLE)hReg,
                                   hKey,
                                   i,
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

           if (dwValueLen && dwValueLen <= MAX_VALUE_LENGTH)
           {

               dwError = GetValueAsStr(dataType,
                                       value,
                                       dwValueLen,
                                       &pszValue);
               BAIL_ON_REG_ERROR(dwError);

               printf("Value %d name is %s has value of %s\n", i, pszValueName, pszValue);
           }
           else
           {
               printf("Value %d name is %s has no value set\n", i, pszValueName);
           }

           LW_SAFE_FREE_STRING(pszValueName);
           memset(valueName, 0 , dwValueNameLen);
           dwValueNameLen = MAX_KEY_LENGTH;
           LW_SAFE_FREE_STRING(pszValue);
           memset(value, 0 , dwValueLen);
           dwValueLen = MAX_VALUE_LENGTH;
       }
   }

error:
   return dwError;
}

DWORD
SetValues(
    HANDLE hReg,
    HKEY hKey,
    PREG_VALUE* ppValues,
    DWORD dwValueNum
    )
{
    DWORD dwError = 0;
    int iCount = 0;
    PWSTR pValueName = NULL;

    BAIL_ON_INVALID_POINTER(ppValues);

    for (iCount = 0; iCount < dwValueNum; iCount++)
    {
        BAIL_ON_INVALID_POINTER(ppValues[iCount]);

        printf("Calling: RegSetValueEx\n");
        dwError = RegSetValueEx(
            (HANDLE) hReg,
            (HKEY) hKey,
            (PCWSTR) ppValues[iCount]->pwszValueName,
            (DWORD) 0,
            (DWORD) ppValues[iCount]->type,
            (const BYTE *) ppValues[iCount]->pData,
            (DWORD) ppValues[iCount]->dwDataLen
            );
        BAIL_ON_REG_ERROR(dwError);
        LW_SAFE_FREE_MEMORY(pValueName);
    }

cleanup:
    LW_SAFE_FREE_MEMORY(pValueName);

    return dwError;

error:
    goto cleanup;
}

void
FreeRegValue(
    PREG_VALUE* ppRegValue
    )
{
    PREG_VALUE pRegValue = NULL;

    if (ppRegValue != NULL && *ppRegValue != NULL)
    {
        pRegValue = *ppRegValue;

        LW_SAFE_FREE_MEMORY(pRegValue->pData);
        LW_SAFE_FREE_MEMORY(pRegValue->pwszValueName);
        pRegValue->type = 0;
        pRegValue->dwDataLen = 0;
        LW_SAFE_FREE_MEMORY(pRegValue);
        *ppRegValue = NULL;
    }
}

void
FreeRegValueList(
    PREG_VALUE* ppRegValue,
    DWORD  dwNumValues
    )
{
    int iValue = 0;
    for (; iValue < dwNumValues; iValue++) {
        PREG_VALUE pValue = *(ppRegValue+iValue);
        FreeRegValue(&pValue);
    }
    LwFreeMemory(ppRegValue);
}

void
FreePVALENT(
    PVALENT pVal
    )
{
    if (pVal != NULL)
    {
        LW_SAFE_FREE_MEMORY(pVal->ve_valuename);
        pVal->ve_valuelen = 0;
        pVal->ve_type = 0;
        pVal->ve_valueptr = (PDWORD)0;
    }
}
