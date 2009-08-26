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
 *        lpobject.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Local Authentication Provider
 *
 *        Object Management Routines
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "includes.h"

static
DWORD
LocalDirFindObjectBySID(
    HANDLE hProvider,
    PCSTR  pszSID,
    PDWORD pdwObjectClass,
    PSTR*  ppszNetBIOSDomain,
    PSTR*  ppszName
    );

DWORD
LocalFindObjectByName(
    HANDLE hProvider,
    PCSTR  pszName,
    PCSTR  pszDomainName,
    PDWORD pdwObjectClass,
    PWSTR* ppwszObjectDN
    )
{
    DWORD dwError = 0;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;
    wchar16_t  wszAttrNameObjectClass[] = LOCAL_DIR_ATTR_OBJECT_CLASS;
    wchar16_t wszAttrNameDN[]           = LOCAL_DIR_ATTR_DISTINGUISHED_NAME;
    PWSTR wszAttrs[] =
    {
        &wszAttrNameObjectClass[0],
        &wszAttrNameDN[0],
        NULL
    };
    DWORD dwNumAttrs = (sizeof(wszAttrs)/sizeof(wszAttrs[0])) - 1;
    PDIRECTORY_ENTRY pEntries = NULL;
    PDIRECTORY_ENTRY pEntry = NULL;
    DWORD dwNumEntries = 0;
    PCSTR pszFilterTemplate =
                    LOCAL_DB_DIR_ATTR_SAM_ACCOUNT_NAME " = \"%s\"" \
                    " AND " LOCAL_DB_DIR_ATTR_DOMAIN   " = \"%s\"";
    PSTR pszFilter = NULL;
    PWSTR pwszFilter = NULL;
    PWSTR pwszObjectDN = NULL;
    DWORD dwObjectClass = LOCAL_OBJECT_CLASS_UNKNOWN;

    dwError = LwAllocateStringPrintf(
                    &pszFilter,
                    pszFilterTemplate,
                    pszName,
                    pszDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaMbsToWc16s(
                    pszFilter,
                    &pwszFilter);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectorySearch(
                    pContext->hDirectory,
                    NULL,
                    0,
                    pwszFilter,
                    wszAttrs,
                    FALSE,
                    &pEntries,
                    &dwNumEntries);
    BAIL_ON_LSA_ERROR(dwError);

    if (dwNumEntries == 0)
    {
        dwError = LW_ERROR_NO_SUCH_OBJECT;
    }
    else if (dwNumEntries != 1)
    {
        dwError = LW_ERROR_DATA_ERROR;
    }
    BAIL_ON_LSA_ERROR(dwError);

    pEntry = &pEntries[0];
    if (pEntry->ulNumAttributes != dwNumAttrs)
    {
        dwError = LW_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LocalMarshalAttrToInteger(
                    pEntry,
                    &wszAttrNameObjectClass[0],
                    &dwObjectClass);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalMarshalAttrToUnicodeString(
                    pEntry,
                    &wszAttrNameDN[0],
                    &pwszObjectDN);
    BAIL_ON_LSA_ERROR(dwError);

    *pdwObjectClass = dwObjectClass;
    *ppwszObjectDN = pwszObjectDN;

cleanup:

    LW_SAFE_FREE_STRING(pszFilter);
    LW_SAFE_FREE_MEMORY(pwszFilter);

    if (pEntries)
    {
        DirectoryFreeEntries(pEntries, dwNumEntries);
    }

    return dwError;

error:

    *pdwObjectClass = LOCAL_OBJECT_CLASS_UNKNOWN;
    *ppwszObjectDN = NULL;

    LW_SAFE_FREE_MEMORY(pwszObjectDN);

    goto cleanup;
}

DWORD
LocalDirGetNamesBySidList(
    HANDLE          hProvider,
    size_t          sCount,
    PSTR*           ppszSidList,
    PSTR**          pppszDomainNames,
    PSTR**          pppszSamAccounts,
    ADAccountType** ppTypes
    )
{
    DWORD dwError = 0;
    PSTR* ppszDomainNames = NULL;
    PSTR* ppszSamAccounts = NULL;
    ADAccountType* pTypes = NULL;
    size_t iSid = 0;

    dwError = LwAllocateMemory(
                        sizeof(PSTR) * sCount,
                        (PVOID*)&ppszDomainNames);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateMemory(
                    sizeof(PSTR) * sCount,
                    (PVOID*)&ppszSamAccounts);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateMemory(
                    sizeof(ADAccountType*) * sCount,
                    (PVOID*)&pTypes);
    BAIL_ON_LSA_ERROR(dwError);

    for (; iSid < sCount; iSid++)
    {
        DWORD dwObjectClass = LOCAL_OBJECT_CLASS_UNKNOWN;
        PCSTR pszSID = ppszSidList[iSid];

        dwError = LocalDirFindObjectBySID(
                        hProvider,
                        pszSID,
                        &dwObjectClass,
                        &ppszDomainNames[iSid],
                        &ppszSamAccounts[iSid]);
        if (dwError == LW_ERROR_NO_SUCH_OBJECT)
        {
            dwError = LW_ERROR_SUCCESS;

            pTypes[iSid] = AccountType_NotFound;
            continue;
        }
        BAIL_ON_LSA_ERROR(dwError);

        switch (dwObjectClass)
        {
            case LOCAL_OBJECT_CLASS_DOMAIN:

                pTypes[iSid] = AccountType_Domain;

                break;

            case LOCAL_OBJECT_CLASS_GROUP:

                pTypes[iSid] = AccountType_Group;

                break;

            case LOCAL_OBJECT_CLASS_USER:

                pTypes[iSid] = AccountType_User;

                break;

            default:

                dwError = LW_ERROR_DATA_ERROR;
                BAIL_ON_LSA_ERROR(dwError);
        }
    }

    *pppszDomainNames = ppszDomainNames;
    *pppszSamAccounts = ppszSamAccounts;
    *ppTypes = pTypes;

cleanup:

    return dwError;

error:

    *pppszDomainNames = NULL;
    *pppszSamAccounts = NULL;
    *ppTypes = NULL;

    LwFreeStringArray(ppszDomainNames, sCount);
    LwFreeStringArray(ppszSamAccounts, sCount);
    LW_SAFE_FREE_MEMORY(pTypes);

    goto cleanup;
}

static
DWORD
LocalDirFindObjectBySID(
    HANDLE hProvider,
    PCSTR  pszSID,
    PDWORD pdwObjectClass,
    PSTR*  ppszNetBIOSDomain,
    PSTR*  ppszName
    )
{
    DWORD dwError = 0;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;
    wchar16_t wszAttrNameObjectClass[] = LOCAL_DIR_ATTR_OBJECT_CLASS;
    wchar16_t wszAttrSamAccountName[]  = LOCAL_DIR_ATTR_SAM_ACCOUNT_NAME;
    wchar16_t wszAttrNetBIOSDomain[]   = LOCAL_DIR_ATTR_NETBIOS_NAME;
    PWSTR wszAttrs[] =
    {
        &wszAttrNameObjectClass[0],
        &wszAttrSamAccountName[0],
        &wszAttrNetBIOSDomain[0],
        NULL
    };
    DWORD dwNumAttrs = (sizeof(wszAttrs)/sizeof(wszAttrs[0])) - 1;
    PDIRECTORY_ENTRY pEntries = NULL;
    PDIRECTORY_ENTRY pEntry = NULL;
    DWORD dwNumEntries = 0;
    PCSTR pszFilterTemplate = LOCAL_DB_DIR_ATTR_OBJECT_SID " = \"%s\"";
    PSTR  pszFilter = NULL;
    PWSTR pwszFilter = NULL;
    DWORD dwObjectClass = LOCAL_OBJECT_CLASS_UNKNOWN;
    PSTR  pszSamAccountName = NULL;
    PSTR  pszNetBIOSDomain = NULL;

    dwError = LwAllocateStringPrintf(
                    &pszFilter,
                    pszFilterTemplate,
                    pszSID);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaMbsToWc16s(
                    pszFilter,
                    &pwszFilter);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectorySearch(
                    pContext->hDirectory,
                    NULL,
                    0,
                    pwszFilter,
                    wszAttrs,
                    FALSE,
                    &pEntries,
                    &dwNumEntries);
    BAIL_ON_LSA_ERROR(dwError);

    if (dwNumEntries == 0)
    {
        dwError = LW_ERROR_NO_SUCH_OBJECT;
    }
    else if (dwNumEntries != 1)
    {
        dwError = LW_ERROR_DATA_ERROR;
    }
    BAIL_ON_LSA_ERROR(dwError);

    pEntry = &pEntries[0];
    if (pEntry->ulNumAttributes != dwNumAttrs)
    {
        dwError = LW_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LocalMarshalAttrToInteger(
                    pEntry,
                    &wszAttrNameObjectClass[0],
                    &dwObjectClass);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalMarshalAttrToANSIFromUnicodeString(
                    pEntry,
                    &wszAttrSamAccountName[0],
                    &pszSamAccountName);
    if (dwError == LSA_ERROR_NO_ATTRIBUTE_VALUE)
    {
        // This is just a stub object used for group membership. A complete
        // record of the object does not exist in this database.
        dwError = LSA_ERROR_NO_SUCH_OBJECT;
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalMarshalAttrToANSIFromUnicodeString(
                    pEntry,
                    &wszAttrNetBIOSDomain[0],
                    &pszNetBIOSDomain);
    BAIL_ON_LSA_ERROR(dwError);

    *pdwObjectClass = dwObjectClass;
    *ppszNetBIOSDomain = pszNetBIOSDomain;
    *ppszName = pszSamAccountName;

cleanup:

    LW_SAFE_FREE_STRING(pszFilter);
    LW_SAFE_FREE_MEMORY(pwszFilter);

    if (pEntries)
    {
        DirectoryFreeEntries(pEntries, dwNumEntries);
    }

    return dwError;

error:

    *pdwObjectClass = LOCAL_OBJECT_CLASS_UNKNOWN;
    *ppszName = NULL;
    *ppszNetBIOSDomain = NULL;

    LW_SAFE_FREE_MEMORY(pszNetBIOSDomain);
    LW_SAFE_FREE_MEMORY(pszSamAccountName);

    goto cleanup;
}

