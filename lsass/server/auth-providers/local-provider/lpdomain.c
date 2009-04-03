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
 *        lpdomain.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Local Authentication Provider
 *
 *        Domain Management
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "includes.h"

static
DWORD
LocalGetSingleAttrValue(
    PATTRIBUTE_VALUE pAttrs,
    DWORD            dwNumAttrs,
    PSTR*            ppszValue
    );

DWORD
LocalGetDomainInfo(
    PWSTR pwszUserDN,
    PWSTR pwszCredentials,
    ULONG ulMethod,
    PSTR* ppszNetBIOSName,
    PSTR* ppszDomainName
    )
{
    DWORD  dwError = 0;
    PSTR   pszHostname = NULL;
    HANDLE hDirectory  = NULL;
    DWORD  objectClass = LOCAL_OBJECT_CLASS_DOMAIN;
    PCSTR  pszFilterTemplate = " WHERE " LOCAL_DB_DIR_ATTR_OBJECT_CLASS " = %d"\
                               "   AND " LOCAL_DB_DIR_ATTR_COMMON_NAME  " = %s";
    PSTR   pszFilter = NULL;
    PWSTR  pwszFilter = NULL;
    wchar16_t wszAttrNameDomain[]      = LOCAL_DIR_ATTR_DOMAIN;
    wchar16_t wszAttrNameNetBIOSName[] = LOCAL_DIR_ATTR_NETBIOS_NAME;
    PWSTR  wszAttrs[] =
    {
            &wszAttrNameDomain[0],
            &wszAttrNameNetBIOSName[0],
            NULL
    };
    DWORD dwNumAttrs = (sizeof(wszAttrs)/sizeof(wszAttrs[0])) - 1;
    PDIRECTORY_ENTRY pEntries = NULL;
    PDIRECTORY_ENTRY pEntry   = NULL;
    DWORD            dwNumEntries = 0;
    DWORD            iAttr = 0;
    ULONG            ulScope = 0;
    PSTR  pszDomainName  = NULL;
    PSTR  pszNetBIOSName = NULL;

    dwError = LsaDnsGetHostInfo(&pszHostname);
    BAIL_ON_LSA_ERROR(dwError);

    LsaStrToUpper(pszHostname);

    dwError = LsaAllocateStringPrintf(
                    &pszFilter,
                    pszFilterTemplate,
                    objectClass,
                    pszHostname);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaMbsToWc16s(
                    pszFilter,
                    &pwszFilter);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectoryOpen(&hDirectory);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectoryBind(
                    hDirectory,
                    pwszUserDN,
                    pwszCredentials,
                    ulMethod);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectorySearch(
                    hDirectory,
                    NULL,
                    ulScope,
                    pwszFilter,
                    wszAttrs,
                    FALSE,
                    &pEntries,
                    &dwNumEntries);
    BAIL_ON_LSA_ERROR(dwError);

    if (!dwNumEntries)
    {
        dwError = LSA_ERROR_NO_SUCH_DOMAIN;
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    if (dwNumEntries > 1)
    {
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pEntry = &pEntries[0];
    if (pEntry->ulNumAttributes != dwNumAttrs)
    {
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    for (; iAttr < pEntry->ulNumAttributes; iAttr++)
    {
        PDIRECTORY_ATTRIBUTE pAttr = &pEntry->pAttributes[iAttr];

        if (!wc16scasecmp(pAttr->pwszName, &wszAttrNameDomain[0]))
        {
            dwError = LocalGetSingleAttrValue(
                            pAttr->pValues,
                            pAttr->ulNumValues,
                            &pszDomainName);
        }
        else
        if (!wc16scasecmp(pAttr->pwszName, &wszAttrNameNetBIOSName[0]))
        {
            dwError = LocalGetSingleAttrValue(
                            pAttr->pValues,
                            pAttr->ulNumValues,
                            &pszNetBIOSName);
        }
        else
        {
            dwError = LSA_ERROR_DATA_ERROR;
        }
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppszDomainName = pszDomainName;
    *ppszNetBIOSName = pszNetBIOSName;

cleanup:

    if (pEntries)
    {
        DirectoryFreeEntries(pEntries, dwNumEntries);
    }

    if (hDirectory)
    {
        DirectoryClose(hDirectory);
    }

    LSA_SAFE_FREE_STRING(pszFilter);
    LSA_SAFE_FREE_STRING(pszHostname);
    LSA_SAFE_FREE_STRING(pszFilter);
    LSA_SAFE_FREE_MEMORY(pwszFilter);

    return dwError;

error:

    *ppszDomainName = NULL;
    *ppszNetBIOSName = NULL;

    LSA_SAFE_FREE_STRING(pszDomainName);
    LSA_SAFE_FREE_STRING(pszNetBIOSName);

    goto cleanup;
}

static
DWORD
LocalGetSingleAttrValue(
    PATTRIBUTE_VALUE pAttrs,
    DWORD            dwNumAttrs,
    PSTR*            ppszValue
    )
{
    DWORD dwError = 0;
    PSTR  pszValue = NULL;

    if ((dwNumAttrs != 1) ||
        (pAttrs[0].Type != DIRECTORY_ATTR_TYPE_UNICODE_STRING))
    {
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pAttrs[0].pwszStringValue)
    {
        dwError = LsaWc16sToMbs(
                        pAttrs[0].pwszStringValue,
                        &pszValue);
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppszValue = pszValue;

cleanup:

    return dwError;

error:

    LSA_SAFE_FREE_STRING(pszValue);

    *ppszValue = NULL;

    goto cleanup;
}
