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
 *        lsassdb.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Local Authentication Provider
 *
 *        Directory User Management API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "includes.h"

DWORD
LocalDirFindUserByName(
    HANDLE  hProvider,
    PCSTR   pszDomainName,
    PCSTR   pszUserName,
    DWORD   dwUserInfoLevel,
    PVOID*  ppUserInfo
    )
{
    DWORD dwError = 0;

    switch(dwUserInfoLevel)
    {
        case 0:

            dwError = LocalDirFindUserByName_0(
                            hProvider,
                            pszDomainName,
                            pszUserName,
                            ppUserInfo
                            );
            break;

        case 1:

            dwError = LocalDirFindUserByName_1(
                            hProvider,
                            pszDomainName,
                            pszUserName,
                            ppUserInfo
                            );
            break;

        case 2:

            dwError = LocalDirFindUserByName_2(
                            hProvider,
                            pszDomainName,
                            pszUserName,
                            ppUserInfo
                            );
            break;

        default:

            dwError = LSA_ERROR_UNSUPPORTED_USER_LEVEL;

            break;
    }

    return dwError;
}

DWORD
LocalDirFindUserByName_0(
    HANDLE hProvider,
    PCSTR  pszDomainName,
    PCSTR  pszUserName,
    PVOID* ppUserInfo
    )
{
    DWORD dwError = 0;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;
    wchar16_t wszAttrNameUID[] = LOCAL_DIR_ATTR_UID;
    wchar16_t wszAttrNameGID[] = LOCAL_DIR_ATTR_GID;
    wchar16_t wszAttrNameSamAccountName[] = LOCAL_DIR_ATTR_SAM_ACCOUNT_NAME;
    wchar16_t wszAttrNamePassword[] = LOCAL_DIR_ATTR_PASSWORD;
    wchar16_t wszAttrNameGecos[] = LOCAL_DIR_ATTR_GECOS;
    wchar16_t wszAttrNameShell[] = LOCAL_DIR_ATTR_SHELL;
    wchar16_t wszAttrNameHomedir[] = LOCAL_DIR_ATTR_HOME_DIR;
    wchar16_t wszAttrNameObjectSID[] = LOCAL_DIR_ATTR_OBJECT_SID;
    PWSTR wszAttrs[] =
    {
        &wszAttrNameUID[0],
        &wszAttrNameGID[0],
        &wszAttrNameSamAccountName[0],
        &wszAttrNamePassword[0],
        &wszAttrNameGecos[0],
        &wszAttrNameShell[0],
        &wszAttrNameHomedir[0],
        &wszAttrNameObjectSID[0],
        NULL
    };
    DWORD dwNumAttrs = (sizeof(wszAttrs)/sizeof(wszAttrs[0])) - 1;
    PDIRECTORY_ENTRY pEntries = NULL;
    PDIRECTORY_ENTRY pEntry = NULL;
    DWORD dwNumEntries = 0;
    PCSTR pszFilterTemplate =
                    LOCAL_DB_DIR_ATTR_SAM_ACCOUNT_NAME " = \"%s\"" \
                    " AND " LOCAL_DB_DIR_ATTR_DOMAIN   " = \"%s\"" \
                    " AND " LOCAL_DB_DIR_ATTR_OBJECT_CLASS " = %d";
    PSTR pszFilter = NULL;
    PWSTR pwszFilter = NULL;
    PLSA_USER_INFO_0 pUserInfo = NULL;
    DWORD iAttr = 0;

    dwError = LsaAllocateStringPrintf(
                    &pszFilter,
                    pszFilterTemplate,
                    pszUserName,
                    pszDomainName,
                    LOCAL_OBJECT_CLASS_USER);
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
        dwError = LSA_ERROR_NO_SUCH_USER;
    }
    else if (dwNumEntries != 1)
    {
        dwError = LSA_ERROR_DATA_ERROR;
    }
    BAIL_ON_LSA_ERROR(dwError);

    pEntry = &pEntries[0];
    if (pEntry->ulNumAttributes != dwNumAttrs)
    {
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaAllocateMemory(
                    sizeof(LSA_USER_INFO_0),
                    (PVOID*)&pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    for (; iAttr < pEntry->ulNumAttributes; iAttr++)
    {
        PDIRECTORY_ATTRIBUTE pAttr = &pEntry->pAttributes[iAttr];

        if (!wc16scasecmp(pAttr->pwszName, &wszAttrNameUID[0]))
        {
            if ((pAttr->ulNumValues != 1) ||
                (pAttr->pValues[0].Type != DIRECTORY_ATTR_TYPE_INTEGER))
            {
                dwError = LSA_ERROR_DATA_ERROR;
                BAIL_ON_LSA_ERROR(dwError);
            }

            pUserInfo->uid = pAttr->pValues[0].data.ulValue;
        }
        else if (!wc16scasecmp(pAttr->pwszName, &wszAttrNameGID[0]))
        {
            if ((pAttr->ulNumValues != 1) ||
                (pAttr->pValues[0].Type != DIRECTORY_ATTR_TYPE_INTEGER))
            {
                dwError = LSA_ERROR_DATA_ERROR;
                BAIL_ON_LSA_ERROR(dwError);
            }

            pUserInfo->gid = pAttr->pValues[0].data.ulValue;
        }
        else if (!wc16scasecmp(pAttr->pwszName, &wszAttrNameSamAccountName[0]))
        {
            if ((pAttr->ulNumValues != 1) ||
                (pAttr->pValues[0].Type != DIRECTORY_ATTR_TYPE_UNICODE_STRING))
            {
                dwError = LSA_ERROR_DATA_ERROR;
                BAIL_ON_LSA_ERROR(dwError);
            }

            dwError = LsaWc16sToMbs(
                            pAttr->pValues[0].data.pwszStringValue,
                            &pUserInfo->pszName);
            BAIL_ON_LSA_ERROR(dwError);
        }
        else if (!wc16scasecmp(pAttr->pwszName, &wszAttrNamePassword[0]))
        {
            if (pAttr->ulNumValues > 0)
            {
                if ((pAttr->ulNumValues != 1) ||
                    (pAttr->pValues[0].Type != DIRECTORY_ATTR_TYPE_UNICODE_STRING))
                {
                    dwError = LSA_ERROR_DATA_ERROR;
                    BAIL_ON_LSA_ERROR(dwError);
                }

                dwError = LsaWc16sToMbs(
                                pAttr->pValues[0].data.pwszStringValue,
                                &pUserInfo->pszPasswd);
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
        else if (!wc16scasecmp(pAttr->pwszName, &wszAttrNameGecos[0]))
        {
            if (pAttr->ulNumValues > 0)
            {
                if ((pAttr->ulNumValues != 1) ||
                    (pAttr->pValues[0].Type != DIRECTORY_ATTR_TYPE_UNICODE_STRING))
                {
                    dwError = LSA_ERROR_DATA_ERROR;
                    BAIL_ON_LSA_ERROR(dwError);
                }

                dwError = LsaWc16sToMbs(
                                pAttr->pValues[0].data.pwszStringValue,
                                &pUserInfo->pszGecos);
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
        else if (!wc16scasecmp(pAttr->pwszName, &wszAttrNameShell[0]))
        {
            if (pAttr->ulNumValues > 0)
            {
                if ((pAttr->ulNumValues != 1) ||
                    (pAttr->pValues[0].Type != DIRECTORY_ATTR_TYPE_UNICODE_STRING))
                {
                    dwError = LSA_ERROR_DATA_ERROR;
                    BAIL_ON_LSA_ERROR(dwError);
                }

                dwError = LsaWc16sToMbs(
                                pAttr->pValues[0].data.pwszStringValue,
                                &pUserInfo->pszShell);
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
        else if (!wc16scasecmp(pAttr->pwszName, &wszAttrNameHomedir[0]))
        {
            if (pAttr->ulNumValues > 0)
            {
                if ((pAttr->ulNumValues != 1) ||
                    (pAttr->pValues[0].Type != DIRECTORY_ATTR_TYPE_UNICODE_STRING))
                {
                    dwError = LSA_ERROR_DATA_ERROR;
                    BAIL_ON_LSA_ERROR(dwError);
                }

                dwError = LsaWc16sToMbs(
                                pAttr->pValues[0].data.pwszStringValue,
                                &pUserInfo->pszHomedir);
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
        else if (!wc16scasecmp(pAttr->pwszName, &wszAttrNameObjectSID[0]))
        {
            if ((pAttr->ulNumValues != 1) ||
                (pAttr->pValues[0].Type != DIRECTORY_ATTR_TYPE_UNICODE_STRING))
            {
                dwError = LSA_ERROR_DATA_ERROR;
                BAIL_ON_LSA_ERROR(dwError);
            }

            dwError = LsaWc16sToMbs(
                            pAttr->pValues[0].data.pwszStringValue,
                            &pUserInfo->pszSid);
            BAIL_ON_LSA_ERROR(dwError);
        }
        else
        {
            dwError = LSA_ERROR_DATA_ERROR;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    *ppUserInfo = pUserInfo;

cleanup:

    LSA_SAFE_FREE_STRING(pszFilter);
    LSA_SAFE_FREE_MEMORY(pwszFilter);

    if (pEntries)
    {
        DirectoryFreeEntries(pEntries, dwNumEntries);
    }

    return dwError;

error:

    *ppUserInfo = pUserInfo;

    if (pUserInfo)
    {
        LsaFreeUserInfo(0, pUserInfo);
    }

    goto cleanup;
}

DWORD
LocalDirFindUserByName_1(
    HANDLE hProvider,
    PCSTR  pszDomainName,
    PCSTR  pszUserName,
    PVOID* ppUserInfo
    )
{
    DWORD dwError = 0;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;
    wchar16_t wszAttrNameUID[] = LOCAL_DIR_ATTR_UID;
    wchar16_t wszAttrNameGID[] = LOCAL_DIR_ATTR_GID;
    wchar16_t wszAttrNameSamAccountName[] = LOCAL_DIR_ATTR_SAM_ACCOUNT_NAME;
    wchar16_t wszAttrNamePassword[]  = LOCAL_DIR_ATTR_PASSWORD;
    wchar16_t wszAttrNameGecos[]     = LOCAL_DIR_ATTR_GECOS;
    wchar16_t wszAttrNameShell[]     = LOCAL_DIR_ATTR_SHELL;
    wchar16_t wszAttrNameHomedir[]   = LOCAL_DIR_ATTR_HOME_DIR;
    wchar16_t wszAttrNameUPN[]       = LOCAL_DIR_ATTR_USER_PRINCIPAL_NAME;
    wchar16_t wszAttrNameObjectSID[] = LOCAL_DIR_ATTR_OBJECT_SID;
    PWSTR wszAttrs[] =
    {
        &wszAttrNameUID[0],
        &wszAttrNameGID[0],
        &wszAttrNameSamAccountName[0],
        &wszAttrNamePassword[0],
        &wszAttrNameGecos[0],
        &wszAttrNameShell[0],
        &wszAttrNameHomedir[0],
        &wszAttrNameUPN[0],
        &wszAttrNameObjectSID[0],
        NULL
    };
    DWORD dwNumAttrs = (sizeof(wszAttrs)/sizeof(wszAttrs[0])) - 1;
    PDIRECTORY_ENTRY pEntries = NULL;
    PDIRECTORY_ENTRY pEntry = NULL;
    DWORD dwNumEntries = 0;
    PCSTR pszFilterTemplate =
                    LOCAL_DB_DIR_ATTR_SAM_ACCOUNT_NAME " = \"%s\"" \
                    " AND " LOCAL_DB_DIR_ATTR_DOMAIN   " = \"%s\"" \
                    " AND " LOCAL_DB_DIR_ATTR_OBJECT_CLASS " = %d";
    PSTR  pszFilter = NULL;
    PWSTR pwszFilter = NULL;
    DWORD dwInfoLevel = 1;
    DWORD iAttr = 0;
    PLSA_USER_INFO_1 pUserInfo = NULL;

    dwError = LsaAllocateStringPrintf(
                    &pszFilter,
                    pszFilterTemplate,
                    pszUserName,
                    pszDomainName,
                    LOCAL_OBJECT_CLASS_USER);
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
        dwError = LSA_ERROR_NO_SUCH_USER;
    }
    else if (dwNumEntries != 1)
    {
        dwError = LSA_ERROR_DATA_ERROR;
    }
    BAIL_ON_LSA_ERROR(dwError);

    pEntry = &pEntries[0];
    if (pEntry->ulNumAttributes != dwNumAttrs)
    {
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaAllocateMemory(
                    sizeof(LSA_USER_INFO_1),
                    (PVOID*)&pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    for (; iAttr < pEntry->ulNumAttributes; iAttr++)
    {
        PDIRECTORY_ATTRIBUTE pAttr = &pEntry->pAttributes[iAttr];

        if (!wc16scasecmp(pAttr->pwszName, &wszAttrNameUID[0]))
        {
            if ((pAttr->ulNumValues != 1) ||
                (pAttr->pValues[0].Type != DIRECTORY_ATTR_TYPE_INTEGER))
            {
                dwError = LSA_ERROR_DATA_ERROR;
                BAIL_ON_LSA_ERROR(dwError);
            }

            pUserInfo->uid = pAttr->pValues[0].data.ulValue;
        }
        else if (!wc16scasecmp(pAttr->pwszName, &wszAttrNameGID[0]))
        {
            if ((pAttr->ulNumValues != 1) ||
                (pAttr->pValues[0].Type != DIRECTORY_ATTR_TYPE_INTEGER))
            {
                dwError = LSA_ERROR_DATA_ERROR;
                BAIL_ON_LSA_ERROR(dwError);
            }

            pUserInfo->gid = pAttr->pValues[0].data.ulValue;
        }
        else if (!wc16scasecmp(pAttr->pwszName, &wszAttrNameSamAccountName[0]))
        {
            if ((pAttr->ulNumValues != 1) ||
                (pAttr->pValues[0].Type != DIRECTORY_ATTR_TYPE_UNICODE_STRING))
            {
                dwError = LSA_ERROR_DATA_ERROR;
                BAIL_ON_LSA_ERROR(dwError);
            }

            dwError = LsaWc16sToMbs(
                            pAttr->pValues[0].data.pwszStringValue,
                            &pUserInfo->pszName);
            BAIL_ON_LSA_ERROR(dwError);
        }
        else if (!wc16scasecmp(pAttr->pwszName, &wszAttrNamePassword[0]))
        {
            if (pAttr->ulNumValues > 0)
            {
                if ((pAttr->ulNumValues != 1) ||
                    (pAttr->pValues[0].Type != DIRECTORY_ATTR_TYPE_UNICODE_STRING))
                {
                    dwError = LSA_ERROR_DATA_ERROR;
                    BAIL_ON_LSA_ERROR(dwError);
                }

                dwError = LsaWc16sToMbs(
                                pAttr->pValues[0].data.pwszStringValue,
                                &pUserInfo->pszPasswd);
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
        else if (!wc16scasecmp(pAttr->pwszName, &wszAttrNameGecos[0]))
        {
            if (pAttr->ulNumValues > 0)
            {
                if ((pAttr->ulNumValues != 1) ||
                    (pAttr->pValues[0].Type != DIRECTORY_ATTR_TYPE_UNICODE_STRING))
                {
                    dwError = LSA_ERROR_DATA_ERROR;
                    BAIL_ON_LSA_ERROR(dwError);
                }

                dwError = LsaWc16sToMbs(
                                pAttr->pValues[0].data.pwszStringValue,
                                &pUserInfo->pszGecos);
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
        else if (!wc16scasecmp(pAttr->pwszName, &wszAttrNameShell[0]))
        {
            if (pAttr->ulNumValues > 0)
            {
                if ((pAttr->ulNumValues != 1) ||
                    (pAttr->pValues[0].Type != DIRECTORY_ATTR_TYPE_UNICODE_STRING))
                {
                    dwError = LSA_ERROR_DATA_ERROR;
                    BAIL_ON_LSA_ERROR(dwError);
                }

                dwError = LsaWc16sToMbs(
                                pAttr->pValues[0].data.pwszStringValue,
                                &pUserInfo->pszShell);
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
        else if (!wc16scasecmp(pAttr->pwszName, &wszAttrNameHomedir[0]))
        {
            if (pAttr->ulNumValues > 0)
            {
                if ((pAttr->ulNumValues != 1) ||
                    (pAttr->pValues[0].Type != DIRECTORY_ATTR_TYPE_UNICODE_STRING))
                {
                    dwError = LSA_ERROR_DATA_ERROR;
                    BAIL_ON_LSA_ERROR(dwError);
                }

                dwError = LsaWc16sToMbs(
                                pAttr->pValues[0].data.pwszStringValue,
                                &pUserInfo->pszHomedir);
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
        else if (!wc16scasecmp(pAttr->pwszName, &wszAttrNameUPN[0]))
        {
            if (pAttr->ulNumValues > 0)
            {
                if ((pAttr->ulNumValues != 1) ||
                    (pAttr->pValues[0].Type != DIRECTORY_ATTR_TYPE_UNICODE_STRING))
                {
                    dwError = LSA_ERROR_DATA_ERROR;
                    BAIL_ON_LSA_ERROR(dwError);
                }

                dwError = LsaWc16sToMbs(
                                pAttr->pValues[0].data.pwszStringValue,
                                &pUserInfo->pszUPN);
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
        else if (!wc16scasecmp(pAttr->pwszName, &wszAttrNameObjectSID[0]))
        {
            if ((pAttr->ulNumValues != 1) ||
                (pAttr->pValues[0].Type != DIRECTORY_ATTR_TYPE_UNICODE_STRING))
            {
                dwError = LSA_ERROR_DATA_ERROR;
                BAIL_ON_LSA_ERROR(dwError);
            }

            dwError = LsaWc16sToMbs(
                            pAttr->pValues[0].data.pwszStringValue,
                            &pUserInfo->pszSid);
            BAIL_ON_LSA_ERROR(dwError);
        }
        else
        {
            dwError = LSA_ERROR_DATA_ERROR;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    if (IsNullOrEmptyString(pUserInfo->pszUPN))
    {
        dwError = LsaAllocateStringPrintf(
                        &pUserInfo->pszUPN,
                        "%s@%s",
                        pUserInfo->pszName,
                        pszDomainName);
        BAIL_ON_LSA_ERROR(dwError);

        LsaStrToUpper(pUserInfo->pszUPN + strlen(pUserInfo->pszName) + 1);

        pUserInfo->bIsGeneratedUPN = TRUE;
    }

    pUserInfo->bIsLocalUser = TRUE;

    *ppUserInfo = pUserInfo;

cleanup:

    LSA_SAFE_FREE_STRING(pszFilter);
    LSA_SAFE_FREE_MEMORY(pwszFilter);

    if (pEntries)
    {
        DirectoryFreeEntries(pEntries, dwNumEntries);
    }

    return dwError;

error:

    *ppUserInfo = pUserInfo;

    if (pUserInfo)
    {
        LsaFreeUserInfo(dwInfoLevel, pUserInfo);
    }

    goto cleanup;
}

DWORD
LocalDirFindUserByName_2(
    HANDLE hProvider,
    PCSTR  pszDomainName,
    PCSTR  pszUserName,
    PVOID* ppUserInfo
    )
{
    DWORD dwError = 0;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;
    wchar16_t wszAttrNameUID[]            = LOCAL_DIR_ATTR_UID;
    wchar16_t wszAttrNameGID[]            = LOCAL_DIR_ATTR_GID;
    wchar16_t wszAttrNameSamAccountName[] = LOCAL_DIR_ATTR_SAM_ACCOUNT_NAME;
    wchar16_t wszAttrNamePassword[]       = LOCAL_DIR_ATTR_PASSWORD;
    wchar16_t wszAttrNameGecos[]          = LOCAL_DIR_ATTR_GECOS;
    wchar16_t wszAttrNameShell[]          = LOCAL_DIR_ATTR_SHELL;
    wchar16_t wszAttrNameHomedir[]        = LOCAL_DIR_ATTR_HOME_DIR;
    wchar16_t wszAttrNameUPN[]            = LOCAL_DIR_ATTR_USER_PRINCIPAL_NAME;
    wchar16_t wszAttrNameObjectSID[]      = LOCAL_DIR_ATTR_OBJECT_SID;
    wchar16_t wszAttrNameUserInfoFlags[]  = LOCAL_DIR_ATTR_USER_INFO_FLAGS;
    wchar16_t wszAttrNameAccountExpiry[]  = LOCAL_DIR_ATTR_ACCOUNT_EXPIRY;
    wchar16_t wszAttrNamePasswdLastSet[]  = LOCAL_DIR_ATTR_PASSWORD_LAST_SET;
    PWSTR wszAttrs[] =
    {
        &wszAttrNameUID[0],
        &wszAttrNameGID[0],
        &wszAttrNameSamAccountName[0],
        &wszAttrNamePassword[0],
        &wszAttrNameGecos[0],
        &wszAttrNameShell[0],
        &wszAttrNameHomedir[0],
        &wszAttrNameUPN[0],
        &wszAttrNameObjectSID[0],
        &wszAttrNameUserInfoFlags[0],
        &wszAttrNameAccountExpiry[0],
        &wszAttrNamePasswdLastSet[0],
        NULL
    };
    DWORD dwNumAttrs = (sizeof(wszAttrs)/sizeof(wszAttrs[0])) - 1;
    PDIRECTORY_ENTRY pEntries = NULL;
    PDIRECTORY_ENTRY pEntry = NULL;
    DWORD dwNumEntries = 0;
    PCSTR pszFilterTemplate =
                    LOCAL_DB_DIR_ATTR_SAM_ACCOUNT_NAME " = \"%s\"" \
                    " AND " LOCAL_DB_DIR_ATTR_DOMAIN   " = \"%s\"" \
                    " AND " LOCAL_DB_DIR_ATTR_OBJECT_CLASS " = %d";
    PSTR   pszFilter = NULL;
    PWSTR  pwszFilter = NULL;
    DWORD  dwUserInfoFlags = 0;
    LONG64 llAccountExpiry = 0;
    LONG64 llPasswordLastSet = 0;
    DWORD  dwInfoLevel = 2;
    DWORD  iAttr = 0;
    PLSA_USER_INFO_2 pUserInfo = NULL;

    dwError = LsaAllocateStringPrintf(
                    &pszFilter,
                    pszFilterTemplate,
                    pszUserName,
                    pszDomainName,
                    LOCAL_OBJECT_CLASS_USER);
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
        dwError = LSA_ERROR_NO_SUCH_USER;
    }
    else if (dwNumEntries != 1)
    {
        dwError = LSA_ERROR_DATA_ERROR;
    }
    BAIL_ON_LSA_ERROR(dwError);

    pEntry = &pEntries[0];
    if (pEntry->ulNumAttributes != dwNumAttrs)
    {
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaAllocateMemory(
                    sizeof(LSA_USER_INFO_2),
                    (PVOID*)&pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    for (; iAttr < pEntry->ulNumAttributes; iAttr++)
    {
        PDIRECTORY_ATTRIBUTE pAttr = &pEntry->pAttributes[iAttr];

        if (!wc16scasecmp(pAttr->pwszName, &wszAttrNameUID[0]))
        {
            if ((pAttr->ulNumValues != 1) ||
                (pAttr->pValues[0].Type != DIRECTORY_ATTR_TYPE_INTEGER))
            {
                dwError = LSA_ERROR_DATA_ERROR;
                BAIL_ON_LSA_ERROR(dwError);
            }

            pUserInfo->uid = pAttr->pValues[0].data.ulValue;
        }
        else if (!wc16scasecmp(pAttr->pwszName, &wszAttrNameGID[0]))
        {
            if ((pAttr->ulNumValues != 1) ||
                (pAttr->pValues[0].Type != DIRECTORY_ATTR_TYPE_INTEGER))
            {
                dwError = LSA_ERROR_DATA_ERROR;
                BAIL_ON_LSA_ERROR(dwError);
            }

            pUserInfo->gid = pAttr->pValues[0].data.ulValue;
        }
        else if (!wc16scasecmp(pAttr->pwszName, &wszAttrNameSamAccountName[0]))
        {
            if ((pAttr->ulNumValues != 1) ||
                (pAttr->pValues[0].Type != DIRECTORY_ATTR_TYPE_UNICODE_STRING))
            {
                dwError = LSA_ERROR_DATA_ERROR;
                BAIL_ON_LSA_ERROR(dwError);
            }

            dwError = LsaWc16sToMbs(
                            pAttr->pValues[0].data.pwszStringValue,
                            &pUserInfo->pszName);
            BAIL_ON_LSA_ERROR(dwError);
        }
        else if (!wc16scasecmp(pAttr->pwszName, &wszAttrNamePassword[0]))
        {
            if (pAttr->ulNumValues > 0)
            {
                if ((pAttr->ulNumValues != 1) ||
                    (pAttr->pValues[0].Type != DIRECTORY_ATTR_TYPE_UNICODE_STRING))
                {
                    dwError = LSA_ERROR_DATA_ERROR;
                    BAIL_ON_LSA_ERROR(dwError);
                }

                dwError = LsaWc16sToMbs(
                                pAttr->pValues[0].data.pwszStringValue,
                                &pUserInfo->pszPasswd);
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
        else if (!wc16scasecmp(pAttr->pwszName, &wszAttrNameGecos[0]))
        {
            if (pAttr->ulNumValues > 0)
            {
                if ((pAttr->ulNumValues != 1) ||
                    (pAttr->pValues[0].Type != DIRECTORY_ATTR_TYPE_UNICODE_STRING))
                {
                    dwError = LSA_ERROR_DATA_ERROR;
                    BAIL_ON_LSA_ERROR(dwError);
                }

                dwError = LsaWc16sToMbs(
                                pAttr->pValues[0].data.pwszStringValue,
                                &pUserInfo->pszGecos);
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
        else if (!wc16scasecmp(pAttr->pwszName, &wszAttrNameShell[0]))
        {
            if (pAttr->ulNumValues > 0)
            {
                if ((pAttr->ulNumValues != 1) ||
                    (pAttr->pValues[0].Type != DIRECTORY_ATTR_TYPE_UNICODE_STRING))
                {
                    dwError = LSA_ERROR_DATA_ERROR;
                    BAIL_ON_LSA_ERROR(dwError);
                }

                dwError = LsaWc16sToMbs(
                                pAttr->pValues[0].data.pwszStringValue,
                                &pUserInfo->pszShell);
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
        else if (!wc16scasecmp(pAttr->pwszName, &wszAttrNameHomedir[0]))
        {
            if (pAttr->ulNumValues > 0)
            {
                if ((pAttr->ulNumValues != 1) ||
                    (pAttr->pValues[0].Type != DIRECTORY_ATTR_TYPE_UNICODE_STRING))
                {
                    dwError = LSA_ERROR_DATA_ERROR;
                    BAIL_ON_LSA_ERROR(dwError);
                }

                dwError = LsaWc16sToMbs(
                                pAttr->pValues[0].data.pwszStringValue,
                                &pUserInfo->pszHomedir);
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
        else if (!wc16scasecmp(pAttr->pwszName, &wszAttrNameUPN[0]))
        {
            if (pAttr->ulNumValues > 0)
            {
                if ((pAttr->ulNumValues != 1) ||
                    (pAttr->pValues[0].Type != DIRECTORY_ATTR_TYPE_UNICODE_STRING))
                {
                    dwError = LSA_ERROR_DATA_ERROR;
                    BAIL_ON_LSA_ERROR(dwError);
                }

                dwError = LsaWc16sToMbs(
                                pAttr->pValues[0].data.pwszStringValue,
                                &pUserInfo->pszUPN);
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
        else if (!wc16scasecmp(pAttr->pwszName, &wszAttrNameObjectSID[0]))
        {
            if ((pAttr->ulNumValues != 1) ||
                (pAttr->pValues[0].Type != DIRECTORY_ATTR_TYPE_UNICODE_STRING))
            {
                dwError = LSA_ERROR_DATA_ERROR;
                BAIL_ON_LSA_ERROR(dwError);
            }

            dwError = LsaWc16sToMbs(
                            pAttr->pValues[0].data.pwszStringValue,
                            &pUserInfo->pszSid);
            BAIL_ON_LSA_ERROR(dwError);
        }
        else if (!wc16scasecmp(pAttr->pwszName, &wszAttrNameUserInfoFlags[0]))
        {
            if ((pAttr->ulNumValues != 1) ||
                (pAttr->pValues[0].Type != DIRECTORY_ATTR_TYPE_INTEGER))
            {
                dwError = LSA_ERROR_DATA_ERROR;
                BAIL_ON_LSA_ERROR(dwError);
            }

            dwUserInfoFlags = pAttr->pValues[0].data.ulValue;
        }
        else if (!wc16scasecmp(pAttr->pwszName, &wszAttrNameAccountExpiry[0]))
        {
            if ((pAttr->ulNumValues != 1) ||
                (pAttr->pValues[0].Type != DIRECTORY_ATTR_TYPE_LARGE_INTEGER))
            {
                dwError = LSA_ERROR_DATA_ERROR;
                BAIL_ON_LSA_ERROR(dwError);
            }

            llAccountExpiry = pAttr->pValues[0].data.llValue;
        }
        else if (!wc16scasecmp(pAttr->pwszName, &wszAttrNamePasswdLastSet[0]))
        {
            if ((pAttr->ulNumValues != 1) ||
                (pAttr->pValues[0].Type != DIRECTORY_ATTR_TYPE_LARGE_INTEGER))
            {
                dwError = LSA_ERROR_DATA_ERROR;
                BAIL_ON_LSA_ERROR(dwError);
            }

            llPasswordLastSet = pAttr->pValues[0].data.llValue;
        }
        else
        {
            dwError = LSA_ERROR_DATA_ERROR;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    if (IsNullOrEmptyString(pUserInfo->pszUPN))
    {
        dwError = LsaAllocateStringPrintf(
                        &pUserInfo->pszUPN,
                        "%s@%s",
                        pUserInfo->pszName,
                        pszDomainName);
        BAIL_ON_LSA_ERROR(dwError);

        LsaStrToUpper(pUserInfo->pszUPN + strlen(pUserInfo->pszName) + 1);

        pUserInfo->bIsGeneratedUPN = TRUE;
    }

    pUserInfo->bIsLocalUser = TRUE;

    dwError = LocalMarshallAccountFlags(
                    pUserInfo,
                    dwUserInfoFlags,
                    llPasswordLastSet,
                    llAccountExpiry);
    BAIL_ON_LSA_ERROR(dwError);

    *ppUserInfo = pUserInfo;

cleanup:

    LSA_SAFE_FREE_STRING(pszFilter);
    LSA_SAFE_FREE_MEMORY(pwszFilter);

    if (pEntries)
    {
        DirectoryFreeEntries(pEntries, dwNumEntries);
    }

    return dwError;

error:

    *ppUserInfo = pUserInfo;

    if (pUserInfo)
    {
        LsaFreeUserInfo(dwInfoLevel, pUserInfo);
    }

    goto cleanup;
}

DWORD
LocalDirFindUserById(
    HANDLE hProvider,
    uid_t  uid,
    DWORD  dwInfoLevel,
    PVOID* ppUserInfo
    )
{
    DWORD dwError = 0;

    if (uid == 0)
    {
        dwError = LSA_ERROR_NO_SUCH_USER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    switch(dwInfoLevel)
    {
        case 0:

            dwError = LocalDirFindUserById_0(hProvider, uid, ppUserInfo);

            break;

        case 1:

            dwError = LocalDirFindUserById_1(hProvider, uid, ppUserInfo);

            break;

        case 2:

            dwError = LocalDirFindUserById_2(hProvider, uid, ppUserInfo);

            break;

        default:

            dwError = LSA_ERROR_UNSUPPORTED_USER_LEVEL;

            break;
    }

error:

    return dwError;
}

DWORD
LocalDirFindUserById_0(
    HANDLE hProvider,
    uid_t  uid,
    PVOID* ppUserInfo
    )
{
    DWORD dwError = 0;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;
    wchar16_t wszAttrNameUID[] = LOCAL_DIR_ATTR_UID;
    wchar16_t wszAttrNameGID[] = LOCAL_DIR_ATTR_GID;
    wchar16_t wszAttrNameSamAccountName[] = LOCAL_DIR_ATTR_SAM_ACCOUNT_NAME;
    wchar16_t wszAttrNamePassword[] = LOCAL_DIR_ATTR_PASSWORD;
    wchar16_t wszAttrNameGecos[] = LOCAL_DIR_ATTR_GECOS;
    wchar16_t wszAttrNameShell[] = LOCAL_DIR_ATTR_SHELL;
    wchar16_t wszAttrNameHomedir[] = LOCAL_DIR_ATTR_HOME_DIR;
    wchar16_t wszAttrNameObjectSID[] = LOCAL_DIR_ATTR_OBJECT_SID;
    PWSTR wszAttrs[] =
    {
        &wszAttrNameUID[0],
        &wszAttrNameGID[0],
        &wszAttrNameSamAccountName[0],
        &wszAttrNamePassword[0],
        &wszAttrNameGecos[0],
        &wszAttrNameShell[0],
        &wszAttrNameHomedir[0],
        &wszAttrNameObjectSID[0],
        NULL
    };
    DWORD dwNumAttrs = (sizeof(wszAttrs)/sizeof(wszAttrs[0])) - 1;
    PDIRECTORY_ENTRY pEntries = NULL;
    PDIRECTORY_ENTRY pEntry = NULL;
    DWORD dwNumEntries = 0;
    PCSTR pszFilterTemplate = LOCAL_DB_DIR_ATTR_UID " = %u" \
                              " AND " LOCAL_DB_DIR_ATTR_OBJECT_CLASS " = %d";
    PSTR pszFilter = NULL;
    PWSTR pwszFilter = NULL;
    PLSA_USER_INFO_0 pUserInfo = NULL;
    DWORD iAttr = 0;

    // Should we include the domain also?
    dwError = LsaAllocateStringPrintf(
                    &pszFilter,
                    pszFilterTemplate,
                    uid,
                    LOCAL_OBJECT_CLASS_USER);
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
        dwError = LSA_ERROR_NO_SUCH_USER;
    }
    else if (dwNumEntries != 1)
    {
        dwError = LSA_ERROR_DATA_ERROR;
    }
    BAIL_ON_LSA_ERROR(dwError);

    pEntry = &pEntries[0];
    if (pEntry->ulNumAttributes != dwNumAttrs)
    {
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaAllocateMemory(
                    sizeof(LSA_USER_INFO_0),
                    (PVOID*)&pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    for (; iAttr < pEntry->ulNumAttributes; iAttr++)
    {
        PDIRECTORY_ATTRIBUTE pAttr = &pEntry->pAttributes[iAttr];

        if (!wc16scasecmp(pAttr->pwszName, &wszAttrNameUID[0]))
        {
            if ((pAttr->ulNumValues != 1) ||
                (pAttr->pValues[0].Type != DIRECTORY_ATTR_TYPE_INTEGER))
            {
                dwError = LSA_ERROR_DATA_ERROR;
                BAIL_ON_LSA_ERROR(dwError);
            }

            pUserInfo->uid = pAttr->pValues[0].data.ulValue;
        }
        else if (!wc16scasecmp(pAttr->pwszName, &wszAttrNameGID[0]))
        {
            if ((pAttr->ulNumValues != 1) ||
                (pAttr->pValues[0].Type != DIRECTORY_ATTR_TYPE_INTEGER))
            {
                dwError = LSA_ERROR_DATA_ERROR;
                BAIL_ON_LSA_ERROR(dwError);
            }

            pUserInfo->gid = pAttr->pValues[0].data.ulValue;
        }
        else if (!wc16scasecmp(pAttr->pwszName, &wszAttrNameSamAccountName[0]))
        {
            if ((pAttr->ulNumValues != 1) ||
                (pAttr->pValues[0].Type != DIRECTORY_ATTR_TYPE_UNICODE_STRING))
            {
                dwError = LSA_ERROR_DATA_ERROR;
                BAIL_ON_LSA_ERROR(dwError);
            }

            dwError = LsaWc16sToMbs(
                            pAttr->pValues[0].data.pwszStringValue,
                            &pUserInfo->pszName);
            BAIL_ON_LSA_ERROR(dwError);
        }
        else if (!wc16scasecmp(pAttr->pwszName, &wszAttrNamePassword[0]))
        {
            if (pAttr->ulNumValues > 0)
            {
                if ((pAttr->ulNumValues != 1) ||
                    (pAttr->pValues[0].Type != DIRECTORY_ATTR_TYPE_UNICODE_STRING))
                {
                    dwError = LSA_ERROR_DATA_ERROR;
                    BAIL_ON_LSA_ERROR(dwError);
                }

                dwError = LsaWc16sToMbs(
                                pAttr->pValues[0].data.pwszStringValue,
                                &pUserInfo->pszPasswd);
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
        else if (!wc16scasecmp(pAttr->pwszName, &wszAttrNameGecos[0]))
        {
            if (pAttr->ulNumValues > 0)
            {
                if ((pAttr->ulNumValues != 1) ||
                    (pAttr->pValues[0].Type != DIRECTORY_ATTR_TYPE_UNICODE_STRING))
                {
                    dwError = LSA_ERROR_DATA_ERROR;
                    BAIL_ON_LSA_ERROR(dwError);
                }

                dwError = LsaWc16sToMbs(
                                pAttr->pValues[0].data.pwszStringValue,
                                &pUserInfo->pszGecos);
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
        else if (!wc16scasecmp(pAttr->pwszName, &wszAttrNameShell[0]))
        {
            if (pAttr->ulNumValues > 0)
            {
                if ((pAttr->ulNumValues != 1) ||
                    (pAttr->pValues[0].Type != DIRECTORY_ATTR_TYPE_UNICODE_STRING))
                {
                    dwError = LSA_ERROR_DATA_ERROR;
                    BAIL_ON_LSA_ERROR(dwError);
                }

                dwError = LsaWc16sToMbs(
                                pAttr->pValues[0].data.pwszStringValue,
                                &pUserInfo->pszShell);
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
        else if (!wc16scasecmp(pAttr->pwszName, &wszAttrNameHomedir[0]))
        {
            if (pAttr->ulNumValues > 0)
            {
                if ((pAttr->ulNumValues != 1) ||
                    (pAttr->pValues[0].Type != DIRECTORY_ATTR_TYPE_UNICODE_STRING))
                {
                    dwError = LSA_ERROR_DATA_ERROR;
                    BAIL_ON_LSA_ERROR(dwError);
                }

                dwError = LsaWc16sToMbs(
                                pAttr->pValues[0].data.pwszStringValue,
                                &pUserInfo->pszHomedir);
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
        else if (!wc16scasecmp(pAttr->pwszName, &wszAttrNameObjectSID[0]))
        {
            if ((pAttr->ulNumValues != 1) ||
                (pAttr->pValues[0].Type != DIRECTORY_ATTR_TYPE_UNICODE_STRING))
            {
                dwError = LSA_ERROR_DATA_ERROR;
                BAIL_ON_LSA_ERROR(dwError);
            }

            dwError = LsaWc16sToMbs(
                            pAttr->pValues[0].data.pwszStringValue,
                            &pUserInfo->pszSid);
            BAIL_ON_LSA_ERROR(dwError);
        }
        else
        {
            dwError = LSA_ERROR_DATA_ERROR;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    *ppUserInfo = pUserInfo;

cleanup:

    LSA_SAFE_FREE_STRING(pszFilter);
    LSA_SAFE_FREE_MEMORY(pwszFilter);

    if (pEntries)
    {
        DirectoryFreeEntries(pEntries, dwNumEntries);
    }

    return dwError;

error:

    *ppUserInfo = pUserInfo;

    if (pUserInfo)
    {
        LsaFreeUserInfo(0, pUserInfo);
    }

    goto cleanup;
}

DWORD
LocalDirFindUserById_1(
    HANDLE hProvider,
    uid_t  uid,
    PVOID* ppUserInfo
    )
{
    DWORD dwError = 0;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;
    wchar16_t wszAttrNameUID[] = LOCAL_DIR_ATTR_UID;
    wchar16_t wszAttrNameGID[] = LOCAL_DIR_ATTR_GID;
    wchar16_t wszAttrNameSamAccountName[] = LOCAL_DIR_ATTR_SAM_ACCOUNT_NAME;
    wchar16_t wszAttrNamePassword[]  = LOCAL_DIR_ATTR_PASSWORD;
    wchar16_t wszAttrNameGecos[]     = LOCAL_DIR_ATTR_GECOS;
    wchar16_t wszAttrNameShell[]     = LOCAL_DIR_ATTR_SHELL;
    wchar16_t wszAttrNameHomedir[]   = LOCAL_DIR_ATTR_HOME_DIR;
    wchar16_t wszAttrNameUPN[]       = LOCAL_DIR_ATTR_USER_PRINCIPAL_NAME;
    wchar16_t wszAttrNameDomain[]    = LOCAL_DIR_ATTR_DOMAIN;
    wchar16_t wszAttrNameObjectSID[] = LOCAL_DIR_ATTR_OBJECT_SID;
    PWSTR wszAttrs[] =
    {
        &wszAttrNameUID[0],
        &wszAttrNameGID[0],
        &wszAttrNameSamAccountName[0],
        &wszAttrNamePassword[0],
        &wszAttrNameGecos[0],
        &wszAttrNameShell[0],
        &wszAttrNameHomedir[0],
        &wszAttrNameUPN[0],
        &wszAttrNameDomain[0],
        &wszAttrNameObjectSID[0],
        NULL
    };
    DWORD dwNumAttrs = (sizeof(wszAttrs)/sizeof(wszAttrs[0])) - 1;
    PDIRECTORY_ENTRY pEntries = NULL;
    PDIRECTORY_ENTRY pEntry = NULL;
    DWORD dwNumEntries = 0;
    PCSTR pszFilterTemplate =
                    LOCAL_DB_DIR_ATTR_UID " = %u" \
                    " AND " LOCAL_DB_DIR_ATTR_OBJECT_CLASS " = %d";
    PSTR  pszFilter = NULL;
    PWSTR pwszFilter = NULL;
    PSTR  pszDomain = NULL;
    DWORD dwInfoLevel = 1;
    DWORD iAttr = 0;
    PLSA_USER_INFO_1 pUserInfo = NULL;

    dwError = LsaAllocateStringPrintf(
                    &pszFilter,
                    pszFilterTemplate,
                    uid,
                    LOCAL_OBJECT_CLASS_USER);
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
        dwError = LSA_ERROR_NO_SUCH_USER;
    }
    else if (dwNumEntries != 1)
    {
        dwError = LSA_ERROR_DATA_ERROR;
    }
    BAIL_ON_LSA_ERROR(dwError);

    pEntry = &pEntries[0];
    if (pEntry->ulNumAttributes != dwNumAttrs)
    {
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaAllocateMemory(
                    sizeof(LSA_USER_INFO_1),
                    (PVOID*)&pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    for (; iAttr < pEntry->ulNumAttributes; iAttr++)
    {
        PDIRECTORY_ATTRIBUTE pAttr = &pEntry->pAttributes[iAttr];

        if (!wc16scasecmp(pAttr->pwszName, &wszAttrNameUID[0]))
        {
            if ((pAttr->ulNumValues != 1) ||
                (pAttr->pValues[0].Type != DIRECTORY_ATTR_TYPE_INTEGER))
            {
                dwError = LSA_ERROR_DATA_ERROR;
                BAIL_ON_LSA_ERROR(dwError);
            }

            pUserInfo->uid = pAttr->pValues[0].data.ulValue;
        }
        else if (!wc16scasecmp(pAttr->pwszName, &wszAttrNameGID[0]))
        {
            if ((pAttr->ulNumValues != 1) ||
                (pAttr->pValues[0].Type != DIRECTORY_ATTR_TYPE_INTEGER))
            {
                dwError = LSA_ERROR_DATA_ERROR;
                BAIL_ON_LSA_ERROR(dwError);
            }

            pUserInfo->gid = pAttr->pValues[0].data.ulValue;
        }
        else if (!wc16scasecmp(pAttr->pwszName, &wszAttrNameSamAccountName[0]))
        {
            if ((pAttr->ulNumValues != 1) ||
                (pAttr->pValues[0].Type != DIRECTORY_ATTR_TYPE_UNICODE_STRING))
            {
                dwError = LSA_ERROR_DATA_ERROR;
                BAIL_ON_LSA_ERROR(dwError);
            }

            dwError = LsaWc16sToMbs(
                            pAttr->pValues[0].data.pwszStringValue,
                            &pUserInfo->pszName);
            BAIL_ON_LSA_ERROR(dwError);
        }
        else if (!wc16scasecmp(pAttr->pwszName, &wszAttrNamePassword[0]))
        {
            if (pAttr->ulNumValues > 0)
            {
                if ((pAttr->ulNumValues != 1) ||
                    (pAttr->pValues[0].Type != DIRECTORY_ATTR_TYPE_UNICODE_STRING))
                {
                    dwError = LSA_ERROR_DATA_ERROR;
                    BAIL_ON_LSA_ERROR(dwError);
                }

                dwError = LsaWc16sToMbs(
                                pAttr->pValues[0].data.pwszStringValue,
                                &pUserInfo->pszPasswd);
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
        else if (!wc16scasecmp(pAttr->pwszName, &wszAttrNameGecos[0]))
        {
            if (pAttr->ulNumValues > 0)
            {
                if ((pAttr->ulNumValues != 1) ||
                    (pAttr->pValues[0].Type != DIRECTORY_ATTR_TYPE_UNICODE_STRING))
                {
                    dwError = LSA_ERROR_DATA_ERROR;
                    BAIL_ON_LSA_ERROR(dwError);
                }

                dwError = LsaWc16sToMbs(
                                pAttr->pValues[0].data.pwszStringValue,
                                &pUserInfo->pszGecos);
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
        else if (!wc16scasecmp(pAttr->pwszName, &wszAttrNameShell[0]))
        {
            if (pAttr->ulNumValues > 0)
            {
                if ((pAttr->ulNumValues != 1) ||
                    (pAttr->pValues[0].Type != DIRECTORY_ATTR_TYPE_UNICODE_STRING))
                {
                    dwError = LSA_ERROR_DATA_ERROR;
                    BAIL_ON_LSA_ERROR(dwError);
                }

                dwError = LsaWc16sToMbs(
                                pAttr->pValues[0].data.pwszStringValue,
                                &pUserInfo->pszShell);
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
        else if (!wc16scasecmp(pAttr->pwszName, &wszAttrNameHomedir[0]))
        {
            if (pAttr->ulNumValues > 0)
            {
                if ((pAttr->ulNumValues != 1) ||
                    (pAttr->pValues[0].Type != DIRECTORY_ATTR_TYPE_UNICODE_STRING))
                {
                    dwError = LSA_ERROR_DATA_ERROR;
                    BAIL_ON_LSA_ERROR(dwError);
                }

                dwError = LsaWc16sToMbs(
                                pAttr->pValues[0].data.pwszStringValue,
                                &pUserInfo->pszHomedir);
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
        else if (!wc16scasecmp(pAttr->pwszName, &wszAttrNameUPN[0]))
        {
            if (pAttr->ulNumValues > 0)
            {
                if ((pAttr->ulNumValues != 1) ||
                    (pAttr->pValues[0].Type != DIRECTORY_ATTR_TYPE_UNICODE_STRING))
                {
                    dwError = LSA_ERROR_DATA_ERROR;
                    BAIL_ON_LSA_ERROR(dwError);
                }

                dwError = LsaWc16sToMbs(
                                pAttr->pValues[0].data.pwszStringValue,
                                &pUserInfo->pszUPN);
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
        else if (!wc16scasecmp(pAttr->pwszName, &wszAttrNameDomain[0]))
        {
            if ((pAttr->ulNumValues != 1) ||
                (pAttr->pValues[0].Type != DIRECTORY_ATTR_TYPE_UNICODE_STRING))
            {
                dwError = LSA_ERROR_DATA_ERROR;
                BAIL_ON_LSA_ERROR(dwError);
            }

            dwError = LsaWc16sToMbs(
                            pAttr->pValues[0].data.pwszStringValue,
                            &pszDomain);
            BAIL_ON_LSA_ERROR(dwError);
        }
        else if (!wc16scasecmp(pAttr->pwszName, &wszAttrNameObjectSID[0]))
        {
            if ((pAttr->ulNumValues != 1) ||
                (pAttr->pValues[0].Type != DIRECTORY_ATTR_TYPE_UNICODE_STRING))
            {
                dwError = LSA_ERROR_DATA_ERROR;
                BAIL_ON_LSA_ERROR(dwError);
            }

            dwError = LsaWc16sToMbs(
                            pAttr->pValues[0].data.pwszStringValue,
                            &pUserInfo->pszSid);
            BAIL_ON_LSA_ERROR(dwError);
        }
        else
        {
            dwError = LSA_ERROR_DATA_ERROR;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    if (IsNullOrEmptyString(pUserInfo->pszUPN))
    {
        if (IsNullOrEmptyString(pszDomain))
        {
            dwError = LSA_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
        }

        dwError = LsaAllocateStringPrintf(
                        &pUserInfo->pszUPN,
                        "%s@%s",
                        pUserInfo->pszName,
                        pszDomain);
        BAIL_ON_LSA_ERROR(dwError);

        LsaStrToUpper(pUserInfo->pszUPN + strlen(pUserInfo->pszName) + 1);

        pUserInfo->bIsGeneratedUPN = TRUE;
    }

    pUserInfo->bIsLocalUser = TRUE;

    *ppUserInfo = pUserInfo;

cleanup:

    LSA_SAFE_FREE_STRING(pszFilter);
    LSA_SAFE_FREE_MEMORY(pwszFilter);
    LSA_SAFE_FREE_STRING(pszDomain);

    if (pEntries)
    {
        DirectoryFreeEntries(pEntries, dwNumEntries);
    }

    return dwError;

error:

    *ppUserInfo = pUserInfo;

    if (pUserInfo)
    {
        LsaFreeUserInfo(dwInfoLevel, pUserInfo);
    }

    goto cleanup;
}

DWORD
LocalDirFindUserById_2(
    HANDLE hProvider,
    uid_t  uid,
    PVOID* ppUserInfo
    )
{
    DWORD dwError = 0;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;
    wchar16_t wszAttrNameUID[]            = LOCAL_DIR_ATTR_UID;
    wchar16_t wszAttrNameGID[]            = LOCAL_DIR_ATTR_GID;
    wchar16_t wszAttrNameSamAccountName[] = LOCAL_DIR_ATTR_SAM_ACCOUNT_NAME;
    wchar16_t wszAttrNamePassword[]       = LOCAL_DIR_ATTR_PASSWORD;
    wchar16_t wszAttrNameGecos[]          = LOCAL_DIR_ATTR_GECOS;
    wchar16_t wszAttrNameShell[]          = LOCAL_DIR_ATTR_SHELL;
    wchar16_t wszAttrNameHomedir[]        = LOCAL_DIR_ATTR_HOME_DIR;
    wchar16_t wszAttrNameUPN[]            = LOCAL_DIR_ATTR_USER_PRINCIPAL_NAME;
    wchar16_t wszAttrNameObjectSID[]      = LOCAL_DIR_ATTR_OBJECT_SID;
    wchar16_t wszAttrNameUserInfoFlags[]  = LOCAL_DIR_ATTR_USER_INFO_FLAGS;
    wchar16_t wszAttrNameAccountExpiry[]  = LOCAL_DIR_ATTR_ACCOUNT_EXPIRY;
    wchar16_t wszAttrNamePasswdLastSet[]  = LOCAL_DIR_ATTR_PASSWORD_LAST_SET;
    wchar16_t wszAttrNameDomain[]         = LOCAL_DIR_ATTR_DOMAIN;
    PWSTR wszAttrs[] =
    {
        &wszAttrNameUID[0],
        &wszAttrNameGID[0],
        &wszAttrNameSamAccountName[0],
        &wszAttrNamePassword[0],
        &wszAttrNameGecos[0],
        &wszAttrNameShell[0],
        &wszAttrNameHomedir[0],
        &wszAttrNameUPN[0],
        &wszAttrNameObjectSID[0],
        &wszAttrNameUserInfoFlags[0],
        &wszAttrNameAccountExpiry[0],
        &wszAttrNamePasswdLastSet[0],
        &wszAttrNameDomain[0],
        NULL
    };
    DWORD dwNumAttrs = (sizeof(wszAttrs)/sizeof(wszAttrs[0])) - 1;
    PDIRECTORY_ENTRY pEntries = NULL;
    PDIRECTORY_ENTRY pEntry = NULL;
    DWORD dwNumEntries = 0;
    PCSTR pszFilterTemplate =
                    LOCAL_DB_DIR_ATTR_UID " = %u" \
                    " AND " LOCAL_DB_DIR_ATTR_OBJECT_CLASS " = %d";
    PSTR  pszFilter = NULL;
    PWSTR pwszFilter = NULL;
    PSTR  pszDomain = NULL;
    DWORD dwUserInfoFlags = 0;
    LONG64 llAccountExpiry = 0;
    LONG64 llPasswordLastSet = 0;
    DWORD dwInfoLevel = 2;
    DWORD iAttr = 0;
    PLSA_USER_INFO_2 pUserInfo = NULL;

    dwError = LsaAllocateStringPrintf(
                    &pszFilter,
                    pszFilterTemplate,
                    uid,
                    LOCAL_OBJECT_CLASS_USER);
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
        dwError = LSA_ERROR_NO_SUCH_USER;
    }
    else if (dwNumEntries != 1)
    {
        dwError = LSA_ERROR_DATA_ERROR;
    }
    BAIL_ON_LSA_ERROR(dwError);

    pEntry = &pEntries[0];
    if (pEntry->ulNumAttributes != dwNumAttrs)
    {
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaAllocateMemory(
                    sizeof(LSA_USER_INFO_2),
                    (PVOID*)&pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    for (; iAttr < pEntry->ulNumAttributes; iAttr++)
    {
        PDIRECTORY_ATTRIBUTE pAttr = &pEntry->pAttributes[iAttr];

        if (!wc16scasecmp(pAttr->pwszName, &wszAttrNameUID[0]))
        {
            if ((pAttr->ulNumValues != 1) ||
                (pAttr->pValues[0].Type != DIRECTORY_ATTR_TYPE_INTEGER))
            {
                dwError = LSA_ERROR_DATA_ERROR;
                BAIL_ON_LSA_ERROR(dwError);
            }

            pUserInfo->uid = pAttr->pValues[0].data.ulValue;
        }
        else if (!wc16scasecmp(pAttr->pwszName, &wszAttrNameGID[0]))
        {
            if ((pAttr->ulNumValues != 1) ||
                (pAttr->pValues[0].Type != DIRECTORY_ATTR_TYPE_INTEGER))
            {
                dwError = LSA_ERROR_DATA_ERROR;
                BAIL_ON_LSA_ERROR(dwError);
            }

            pUserInfo->gid = pAttr->pValues[0].data.ulValue;
        }
        else if (!wc16scasecmp(pAttr->pwszName, &wszAttrNameSamAccountName[0]))
        {
            if ((pAttr->ulNumValues != 1) ||
                (pAttr->pValues[0].Type != DIRECTORY_ATTR_TYPE_UNICODE_STRING))
            {
                dwError = LSA_ERROR_DATA_ERROR;
                BAIL_ON_LSA_ERROR(dwError);
            }

            dwError = LsaWc16sToMbs(
                            pAttr->pValues[0].data.pwszStringValue,
                            &pUserInfo->pszName);
            BAIL_ON_LSA_ERROR(dwError);
        }
        else if (!wc16scasecmp(pAttr->pwszName, &wszAttrNamePassword[0]))
        {
            if (pAttr->ulNumValues > 0)
            {
                if ((pAttr->ulNumValues != 1) ||
                    (pAttr->pValues[0].Type != DIRECTORY_ATTR_TYPE_UNICODE_STRING))
                {
                    dwError = LSA_ERROR_DATA_ERROR;
                    BAIL_ON_LSA_ERROR(dwError);
                }

                dwError = LsaWc16sToMbs(
                                pAttr->pValues[0].data.pwszStringValue,
                                &pUserInfo->pszPasswd);
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
        else if (!wc16scasecmp(pAttr->pwszName, &wszAttrNameGecos[0]))
        {
            if (pAttr->ulNumValues > 0)
            {
                if ((pAttr->ulNumValues != 1) ||
                    (pAttr->pValues[0].Type != DIRECTORY_ATTR_TYPE_UNICODE_STRING))
                {
                    dwError = LSA_ERROR_DATA_ERROR;
                    BAIL_ON_LSA_ERROR(dwError);
                }

                dwError = LsaWc16sToMbs(
                                pAttr->pValues[0].data.pwszStringValue,
                                &pUserInfo->pszGecos);
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
        else if (!wc16scasecmp(pAttr->pwszName, &wszAttrNameShell[0]))
        {
            if (pAttr->ulNumValues > 0)
            {
                if ((pAttr->ulNumValues != 1) ||
                    (pAttr->pValues[0].Type != DIRECTORY_ATTR_TYPE_UNICODE_STRING))
                {
                    dwError = LSA_ERROR_DATA_ERROR;
                    BAIL_ON_LSA_ERROR(dwError);
                }

                dwError = LsaWc16sToMbs(
                                pAttr->pValues[0].data.pwszStringValue,
                                &pUserInfo->pszShell);
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
        else if (!wc16scasecmp(pAttr->pwszName, &wszAttrNameHomedir[0]))
        {
            if (pAttr->ulNumValues > 0)
            {
                if ((pAttr->ulNumValues != 1) ||
                    (pAttr->pValues[0].Type != DIRECTORY_ATTR_TYPE_UNICODE_STRING))
                {
                    dwError = LSA_ERROR_DATA_ERROR;
                    BAIL_ON_LSA_ERROR(dwError);
                }

                dwError = LsaWc16sToMbs(
                                pAttr->pValues[0].data.pwszStringValue,
                                &pUserInfo->pszHomedir);
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
        else if (!wc16scasecmp(pAttr->pwszName, &wszAttrNameUPN[0]))
        {
            if (pAttr->ulNumValues > 0)
            {
                if ((pAttr->ulNumValues != 1) ||
                    (pAttr->pValues[0].Type != DIRECTORY_ATTR_TYPE_UNICODE_STRING))
                {
                    dwError = LSA_ERROR_DATA_ERROR;
                    BAIL_ON_LSA_ERROR(dwError);
                }

                dwError = LsaWc16sToMbs(
                                pAttr->pValues[0].data.pwszStringValue,
                                &pUserInfo->pszUPN);
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
        else if (!wc16scasecmp(pAttr->pwszName, &wszAttrNameDomain[0]))
        {
            if ((pAttr->ulNumValues != 1) ||
                (pAttr->pValues[0].Type != DIRECTORY_ATTR_TYPE_UNICODE_STRING))
            {
                dwError = LSA_ERROR_DATA_ERROR;
                BAIL_ON_LSA_ERROR(dwError);
            }

            dwError = LsaWc16sToMbs(
                            pAttr->pValues[0].data.pwszStringValue,
                            &pszDomain);
            BAIL_ON_LSA_ERROR(dwError);
        }
        else if (!wc16scasecmp(pAttr->pwszName, &wszAttrNameObjectSID[0]))
        {
            if ((pAttr->ulNumValues != 1) ||
                (pAttr->pValues[0].Type != DIRECTORY_ATTR_TYPE_UNICODE_STRING))
            {
                dwError = LSA_ERROR_DATA_ERROR;
                BAIL_ON_LSA_ERROR(dwError);
            }

            dwError = LsaWc16sToMbs(
                            pAttr->pValues[0].data.pwszStringValue,
                            &pUserInfo->pszSid);
            BAIL_ON_LSA_ERROR(dwError);
        }
        else if (!wc16scasecmp(pAttr->pwszName, &wszAttrNameUserInfoFlags[0]))
        {
            if ((pAttr->ulNumValues != 1) ||
                (pAttr->pValues[0].Type != DIRECTORY_ATTR_TYPE_INTEGER))
            {
                dwError = LSA_ERROR_DATA_ERROR;
                BAIL_ON_LSA_ERROR(dwError);
            }

            dwUserInfoFlags = pAttr->pValues[0].data.ulValue;
        }
        else if (!wc16scasecmp(pAttr->pwszName, &wszAttrNameAccountExpiry[0]))
        {
            if ((pAttr->ulNumValues != 1) ||
                (pAttr->pValues[0].Type != DIRECTORY_ATTR_TYPE_LARGE_INTEGER))
            {
                dwError = LSA_ERROR_DATA_ERROR;
                BAIL_ON_LSA_ERROR(dwError);
            }

            llAccountExpiry = pAttr->pValues[0].data.llValue;
        }
        else if (!wc16scasecmp(pAttr->pwszName, &wszAttrNamePasswdLastSet[0]))
        {
            if ((pAttr->ulNumValues != 1) ||
                (pAttr->pValues[0].Type != DIRECTORY_ATTR_TYPE_LARGE_INTEGER))
            {
                dwError = LSA_ERROR_DATA_ERROR;
                BAIL_ON_LSA_ERROR(dwError);
            }

            llPasswordLastSet = pAttr->pValues[0].data.llValue;
        }
        else
        {
            dwError = LSA_ERROR_DATA_ERROR;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    if (IsNullOrEmptyString(pUserInfo->pszUPN))
    {
        if (IsNullOrEmptyString(pszDomain))
        {
            dwError = LSA_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
        }

        dwError = LsaAllocateStringPrintf(
                        &pUserInfo->pszUPN,
                        "%s@%s",
                        pUserInfo->pszName,
                        pszDomain);
        BAIL_ON_LSA_ERROR(dwError);

        LsaStrToUpper(pUserInfo->pszUPN + strlen(pUserInfo->pszName) + 1);

        pUserInfo->bIsGeneratedUPN = TRUE;
    }

    pUserInfo->bIsLocalUser = TRUE;

    dwError = LocalMarshallAccountFlags(
                    pUserInfo,
                    dwUserInfoFlags,
                    llPasswordLastSet,
                    llAccountExpiry);
    BAIL_ON_LSA_ERROR(dwError);

    *ppUserInfo = pUserInfo;

cleanup:

    LSA_SAFE_FREE_STRING(pszFilter);
    LSA_SAFE_FREE_MEMORY(pwszFilter);
    LSA_SAFE_FREE_STRING(pszDomain);

    if (pEntries)
    {
        DirectoryFreeEntries(pEntries, dwNumEntries);
    }

    return dwError;

error:

    *ppUserInfo = pUserInfo;

    if (pUserInfo)
    {
        LsaFreeUserInfo(dwInfoLevel, pUserInfo);
    }

    goto cleanup;
}

DWORD
LocalDirEnumUsers(
    HANDLE  hProvider,
    DWORD   dwInfoLevel,
    DWORD   dwStartingRecordId,
    DWORD   nMaxUsers,
    PDWORD  pdwNumUsersFound,
    PVOID** pppUserInfoList
    )
{
    DWORD dwError = LSA_ERROR_UNSUPPORTED_USER_LEVEL;

    switch (dwInfoLevel)
    {
        case 0:
        {
            dwError = LocalDirEnumUsers_0(
                                hProvider,
                                dwStartingRecordId,
                                nMaxUsers,
                                pdwNumUsersFound,
                                pppUserInfoList
                                );
            break;
        }
        case 1:
        {
            dwError = LocalDirEnumUsers_1(
                                hProvider,
                                dwStartingRecordId,
                                nMaxUsers,
                                pdwNumUsersFound,
                                pppUserInfoList
                                );
            break;
        }
        case 2:
        {
            dwError = LocalDirEnumUsers_2(
                                hProvider,
                                dwStartingRecordId,
                                nMaxUsers,
                                pdwNumUsersFound,
                                pppUserInfoList
                                );
            break;
        }
    }

    return dwError;
}

DWORD
LocalDirEnumUsers_0(
    HANDLE  hProvider,
    DWORD   dwOffset,
    DWORD   dwLimit,
    PDWORD  pdwNumUsersFound,
    PVOID** pppUserInfoList
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
LocalDirEnumUsers_1(
    HANDLE hProvider,
    DWORD  dwOffset,
    DWORD  dwLimit,
    PDWORD pdwNumUsersFound,
    PVOID** pppUserInfoList
    )
{
    DWORD dwError = 0;

    return dwError;
}


DWORD
LocalDirEnumUsers_2(
    HANDLE  hProvider,
    DWORD   dwOffset,
    DWORD   dwLimit,
    PDWORD  pdwNumUsersFound,
    PVOID** pppUserInfoList
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
LocalDirGetGroupsForUser(
    HANDLE  hProvider,
    uid_t   uid,
    DWORD   dwInfoLevel,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
    )
{
    DWORD dwError = LSA_ERROR_UNSUPPORTED_GROUP_LEVEL;

    switch(dwInfoLevel)
    {
        case 0:
        {
            dwError = LocalDirGetGroupsForUser_0(
                                hProvider,
                                uid,
                                pdwGroupsFound,
                                pppGroupInfoList
                                );
            break;
        }
        case 1:
        {
            dwError = LocalDirGetGroupsForUser_1(
                                hProvider,
                                uid,
                                pdwGroupsFound,
                                pppGroupInfoList
                                );
            break;
        }

    }

    return dwError;
}

DWORD
LocalDirGetGroupsForUser_0_Unsafe(
    HANDLE  hProvider,
    uid_t   uid,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
LocalDirGetGroupsForUser_1_Unsafe(
    HANDLE  hProvider,
    uid_t   uid,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
LocalDirAddUser(
    HANDLE hProvider,
    DWORD  dwInfoLevel,
    PVOID  pUserInfo
    )
{
    DWORD dwError = 0;

    switch(dwInfoLevel)
    {
        case 0:

            dwError = LocalDirAddUser_0(
                            hProvider,
                            (PLSA_USER_INFO_0)pUserInfo);
            break;

        case 1:

            dwError = LocalDirAddUser_1(
                            hProvider,
                            (PLSA_USER_INFO_1)pUserInfo);

            break;

        case 2:

            dwError = LocalDirAddUser_2(
                            hProvider,
                            (PLSA_USER_INFO_2)pUserInfo);

            break;

        default:

            dwError = LSA_ERROR_UNSUPPORTED_USER_LEVEL;

            break;
    }
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
LocalDirAddUser_0(
    HANDLE           hProvider,
    PLSA_USER_INFO_0 pUserInfo
    )
{
    DWORD dwError = 0;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;
    BOOLEAN bEventlogEnabled = FALSE;
    PLSA_LOGIN_NAME_INFO pLoginInfo = NULL;
    PWSTR pwszUserDN = NULL;
    enum AttrValueIndex {
        LOCAL_DAU0_IDX_UID = 0,
        LOCAL_DAU0_IDX_GID,
        LOCAL_DAU0_IDX_SAM_ACCOUNT_NAME,
        LOCAL_DAU0_IDX_PASSWORD,
        LOCAL_DAU0_IDX_GECOS,
        LOCAL_DAU0_IDX_SHELL,
        LOCAL_DAU0_IDX_HOMEDIR,
        LOCAL_DAU0_IDX_OBJECTSID,
        LOCAL_DAU0_IDX_OBJECTCLASS
    };
    ATTRIBUTE_VALUE attrValues[] =
    {
            {       /* LOCAL_DIR_ADD_USER_0_IDX_UID */
                    .Type = DIRECTORY_ATTR_TYPE_INTEGER,
                    .data.ulValue = pUserInfo->uid
            },
            {       /* LOCAL_DIR_ADD_USER_0_IDX_GID */
                    .Type = DIRECTORY_ATTR_TYPE_INTEGER,
                    .data.ulValue = pUserInfo->gid
            },
            {       /* LOCAL_DIR_ADD_USER_0_IDX_SAM_ACCOUNT_NAME */
                    .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                    .data.pwszStringValue = NULL
            },
            {       /* LOCAL_DIR_ADD_USER_0_IDX_PASSWORD */
                    .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                    .data.pwszStringValue = NULL
            },
            {       /* LOCAL_DIR_ADD_USER_0_IDX_GECOS */
                    .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                    .data.pwszStringValue = NULL
            },
            {       /* LOCAL_DIR_ADD_USER_0_IDX_SHELL */
                    .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                    .data.pwszStringValue = NULL
            },
            {        /* LOCAL_DIR_ADD_USER_0_IDX_HOMEDIR */
                    .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                    .data.pwszStringValue = NULL
            },
            {       /* LOCAL_DIR_ADD_USER_0_IDX_OBJECTSID */
                    .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                    .data.pwszStringValue = NULL
            },
            {       /* LOCAL_DIR_ADD_USER_0_IDX_OBJECTCLASS */
                    .Type = DIRECTORY_ATTR_TYPE_INTEGER,
                    .data.ulValue = LOCAL_OBJECT_CLASS_USER
            }
    };
    WCHAR wszAttrObjectClass[]    = LOCAL_DIR_ATTR_OBJECT_CLASS;
    WCHAR wszAttrGID[]            = LOCAL_DIR_ATTR_GID;
    WCHAR wszAttrSamAccountName[] = LOCAL_DIR_ATTR_SAM_ACCOUNT_NAME;
    WCHAR wszAttrGecos[]          = LOCAL_DIR_ATTR_GECOS;
    WCHAR wszAttrShell[]          = LOCAL_DIR_ATTR_SHELL;
    WCHAR wszAttrHomedir[]        = LOCAL_DIR_ATTR_HOME_DIR;
    DIRECTORY_MOD mods[] =
    {
            {
                    DIR_MOD_FLAGS_ADD,
                    &wszAttrObjectClass[0],
                    1,
                    &attrValues[LOCAL_DAU0_IDX_OBJECTCLASS]
            },
            {
                    DIR_MOD_FLAGS_ADD,
                    &wszAttrSamAccountName[0],
                    1,
                    &attrValues[LOCAL_DAU0_IDX_SAM_ACCOUNT_NAME]
            },
            {
                    DIR_MOD_FLAGS_ADD,
                    &wszAttrGID[0],
                    1,
                    &attrValues[LOCAL_DAU0_IDX_GID]
            },
            {
                    DIR_MOD_FLAGS_ADD,
                    &wszAttrGecos[0],
                    1,
                    &attrValues[LOCAL_DAU0_IDX_GECOS]
            },
            {
                    DIR_MOD_FLAGS_ADD,
                    &wszAttrShell[0],
                    1,
                    &attrValues[LOCAL_DAU0_IDX_SHELL]
            },
            {
                    DIR_MOD_FLAGS_ADD,
                    &wszAttrHomedir[0],
                    1,
                    &attrValues[LOCAL_DAU0_IDX_HOMEDIR]
            },
            {
                    DIR_MOD_FLAGS_ADD,
                    NULL,
                    1,
                    NULL
            }
    };
    PWSTR pwszSamAccountName = NULL;
    PWSTR pwszGecos = NULL;
    PWSTR pwszShell = NULL;
    PWSTR pwszHomedir = NULL;
    PWSTR pwszPassword = NULL;

    BAIL_ON_INVALID_STRING(pUserInfo->pszName);

    dwError = LsaCrackDomainQualifiedName(
                    pUserInfo->pszName,
                    NULL,
                    &pLoginInfo);
    BAIL_ON_LSA_ERROR(dwError);

    if (!LocalServicesDomain(pLoginInfo->pszDomainNetBiosName))
    {
        dwError = LSA_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaMbsToWc16s(
                    pUserInfo->pszName,
                    &pwszSamAccountName);
    BAIL_ON_LSA_ERROR(dwError);

    attrValues[LOCAL_DAU0_IDX_SAM_ACCOUNT_NAME].data.pwszStringValue = pwszSamAccountName;

    dwError = LocalBuildDN(
                    pLoginInfo,
                    &pwszUserDN);
    BAIL_ON_LSA_ERROR(dwError);

    if (!IsNullOrEmptyString(pUserInfo->pszGecos))
    {
        dwError = LsaMbsToWc16s(
                    pUserInfo->pszGecos,
                    &pwszGecos);
        BAIL_ON_LSA_ERROR(dwError);

        attrValues[LOCAL_DAU0_IDX_GECOS].data.pwszStringValue = pwszGecos;
    }

    if (!IsNullOrEmptyString(pUserInfo->pszHomedir))
    {
        dwError = LsaMbsToWc16s(
                    pUserInfo->pszHomedir,
                    &pwszHomedir);
        BAIL_ON_LSA_ERROR(dwError);

        attrValues[LOCAL_DAU0_IDX_HOMEDIR].data.pwszStringValue = pwszHomedir;
    }

    if (!IsNullOrEmptyString(pUserInfo->pszShell))
    {
        dwError = LsaMbsToWc16s(
                    pUserInfo->pszShell,
                    &pwszShell);
        BAIL_ON_LSA_ERROR(dwError);

        attrValues[LOCAL_DAU0_IDX_SHELL].data.pwszStringValue = pwszShell;
    }

    dwError = DirectoryAddObject(
                    pContext->hDirectory,
                    pwszUserDN,
                    mods);
    BAIL_ON_LSA_ERROR(dwError);

    if (!IsNullOrEmptyString(pUserInfo->pszPasswd))
    {
        dwError = LsaMbsToWc16s(
                        pUserInfo->pszPasswd,
                        &pwszPassword);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = DirectorySetPassword(
                        pContext->hDirectory,
                        pwszUserDN,
                        pwszPassword);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LocalCfgIsEventlogEnabled(&bEventlogEnabled);
    BAIL_ON_LSA_ERROR(dwError);

    if (bEventlogEnabled)
    {
        LocalEventLogUserAdd(pLoginInfo->pszName,
                             ((PLSA_USER_INFO_0)pUserInfo)->uid);
    }

cleanup:

    if (pLoginInfo)
    {
        LsaFreeNameInfo(pLoginInfo);
    }

    LSA_SAFE_FREE_MEMORY(pwszUserDN);
    LSA_SAFE_FREE_MEMORY(pwszSamAccountName);
    LSA_SAFE_FREE_MEMORY(pwszGecos);
    LSA_SAFE_FREE_MEMORY(pwszShell);
    LSA_SAFE_FREE_MEMORY(pwszHomedir);
    LSA_SAFE_FREE_MEMORY(pwszPassword);

    return dwError;

error:

    goto cleanup;
}

DWORD
LocalDirAddUser_1(
    HANDLE           hProvider,
    PLSA_USER_INFO_1 pUserInfo
    )
{
    return LSA_ERROR_NOT_SUPPORTED;
}

DWORD
LocalDirAddUser_2(
    HANDLE           hProvider,
    PLSA_USER_INFO_2 pUserInfo
    )
{
    return LSA_ERROR_NOT_SUPPORTED;
}

DWORD
LocalDirDeleteUser(
    HANDLE hProvider,
    PWSTR  pwszUserDN
    )
{
    DWORD dwError = 0;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;

    dwError = DirectoryDeleteObject(
                    pContext->hDirectory,
                    pwszUserDN);
    BAIL_ON_LSA_ERROR(dwError);

error:

    return dwError;
}

DWORD
LocalDirModifyUser(
    HANDLE             hProvider,
    PLSA_USER_MOD_INFO pUserModInfo
    )
{
    DWORD dwError = 0;
#if 0
    PVOID pUserInfo = NULL;
    DWORD dwUserInfoLevel = 0;

    // TODO: Implement this in a database transaction

    dwError = LocalFindUserById(
                        hProvider,
                        pUserModInfo->uid,
                        dwUserInfoLevel,
                        &pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    if (pUserModInfo->actions.bEnableUser) {
        dwError = LocalEnableUser(
                       hProvider,
                       pUserModInfo->uid);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserModInfo->actions.bDisableUser) {
        dwError = LocalDisableUser(
                        hProvider,
                        pUserModInfo->uid);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserModInfo->actions.bUnlockUser) {
        dwError = LocalUnlockUser(
                        hProvider,
                        pUserModInfo->uid);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserModInfo->actions.bSetChangePasswordOnNextLogon) {
        dwError = LocalSetChangePasswordOnNextLogon(
                        hProvider,
                        pUserModInfo->uid);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LocalSetPasswordExpires(
                        hProvider,
                        pUserModInfo->uid,
                        TRUE);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserModInfo->actions.bSetAccountExpiryDate) {
        dwError = LocalSetAccountExpiryDate(
                        hProvider,
                        pUserModInfo->uid,
                        pUserModInfo->pszExpiryDate);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserModInfo->actions.bRemoveFromGroups) {
        dwError = LocalRemoveFromGroups(
                        hProvider,
                        pUserModInfo->uid,
                        pUserModInfo->pszRemoveFromGroups);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserModInfo->actions.bAddToGroups) {
        dwError = LocalAddToGroups(
                        hProvider,
                        pUserModInfo->uid,
                        pUserModInfo->pszAddToGroups);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserModInfo->actions.bSetPasswordMustExpire) {
        dwError = LocalSetPasswordExpires(
                        hProvider,
                        pUserModInfo->uid,
                        TRUE);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserModInfo->actions.bSetPasswordNeverExpires) {
        dwError = LocalSetPasswordExpires(
                        hProvider,
                        pUserModInfo->uid,
                        FALSE);
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    if (pUserInfo) {
       LsaFreeUserInfo(dwUserInfoLevel, pUserInfo);
    }

    return dwError;

error:

    goto cleanup;
#else
    return dwError;
#endif
}

DWORD
LocalDirChangePassword(
    HANDLE hProvider,
    PWSTR  pwszUserDN,
    PWSTR  pwszOldPassword,
    PWSTR  pwszNewPassword
    )
{
    DWORD dwError = 0;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;

    dwError = DirectoryChangePassword(
                    pContext->hDirectory,
                    pwszUserDN,
                    pwszOldPassword,
                    pwszNewPassword);
    BAIL_ON_LSA_ERROR(dwError);

error:

    return dwError;
}

DWORD
LocalCreateHomeDirectory(
    PLSA_USER_INFO_0 pUserInfo
    )
{
    DWORD dwError = 0;
    BOOLEAN bExists = FALSE;
    mode_t  umask = 022;
    mode_t  perms = (S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);
    BOOLEAN bRemoveDir = FALSE;

    if (IsNullOrEmptyString(pUserInfo->pszHomedir))
    {
       LSA_LOG_ERROR("The user's [Uid:%ld] home directory is not defined",
                     (long)pUserInfo->uid);
       dwError = LSA_ERROR_FAILED_CREATE_HOMEDIR;
       BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaCheckDirectoryExists(
                    pUserInfo->pszHomedir,
                    &bExists);
    BAIL_ON_LSA_ERROR(dwError);

    if (!bExists)
    {
       dwError = LsaCreateDirectory(
                    pUserInfo->pszHomedir,
                    perms & (~umask));
       BAIL_ON_LSA_ERROR(dwError);

       bRemoveDir = TRUE;

       dwError = LsaChangeOwner(
                    pUserInfo->pszHomedir,
                    pUserInfo->uid,
                    pUserInfo->gid);
       BAIL_ON_LSA_ERROR(dwError);

       bRemoveDir = FALSE;

       dwError = LocalProvisionHomeDir(
                       pUserInfo->uid,
                       pUserInfo->gid,
                       pUserInfo->pszHomedir);
       BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    return dwError;

error:

    if (bRemoveDir) {
       LsaRemoveDirectory(pUserInfo->pszHomedir);
    }

    goto cleanup;
}

DWORD
LocalProvisionHomeDir(
    uid_t ownerUid,
    gid_t ownerGid,
    PCSTR pszHomedirPath
    )
{
    DWORD   dwError = 0;
    BOOLEAN bExists = FALSE;

    dwError = LsaCheckDirectoryExists(
                    "/etc/skel",
                    &bExists);
    BAIL_ON_LSA_ERROR(dwError);

    if (bExists)
    {
        dwError = LsaCopyDirectory(
                    "/etc/skel",
                    ownerUid,
                    ownerGid,
                    pszHomedirPath);
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
LocalCheckAccountFlags(
    PLSA_USER_INFO_2 pUserInfo2
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;

    BAIL_ON_INVALID_POINTER(pUserInfo2);

    if (pUserInfo2->bAccountDisabled)
    {
        dwError = LSA_ERROR_ACCOUNT_DISABLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserInfo2->bAccountLocked)
    {
        dwError = LSA_ERROR_ACCOUNT_LOCKED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserInfo2->bAccountExpired)
    {
        dwError = LSA_ERROR_ACCOUNT_EXPIRED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserInfo2->bPasswordExpired)
    {
        dwError = LSA_ERROR_PASSWORD_EXPIRED;
        BAIL_ON_LSA_ERROR(dwError);
    }

error:

    return dwError;
}

DWORD
LocalMarshallAccountFlags(
    PLSA_USER_INFO_2 pUserInfo,
    DWORD            dwUserInfoFlags,
    LONG64           llPwdLastSet,
    LONG64           llAcctExpiry
    )
{
    DWORD dwError = 0;
    BOOLEAN bPasswordNeverExpires = FALSE;
    BOOLEAN bAccountDisabled = FALSE;
    BOOLEAN bUserCanChangePassword = FALSE;
    BOOLEAN bAccountLocked = FALSE;
    BOOLEAN bAccountExpired = FALSE;
    BOOLEAN bPasswordExpired = FALSE;
    BOOLEAN bPromptPasswordChange = FALSE;
    DWORD   dwDaysToPasswordExpiry = 0;

    if (dwUserInfoFlags & LOCAL_ACB_PWNOEXP)
    {
        bPasswordNeverExpires = TRUE;
    }

    if (dwUserInfoFlags & LOCAL_ACB_DISABLED)
    {
        bAccountDisabled = TRUE;
    }

    if (dwUserInfoFlags & LOCAL_ACB_DOMTRUST)
    {
        bUserCanChangePassword = TRUE;
    }

    if (dwUserInfoFlags & LOCAL_ACB_NORMAL)
    {
        bAccountLocked = TRUE;
    }

    if (!bPasswordNeverExpires)
    {
        // Password expires

        if (dwUserInfoFlags & LOCAL_ACB_PW_EXPIRED)
        {
            bPasswordExpired = TRUE;
            dwDaysToPasswordExpiry = 0;
            bPromptPasswordChange = TRUE;
        }
        else
        {
            // Account has not expired yet

            LONG64 llMaxPwdAge = 0;
            LONG64 llPwdChangeTime = 0;
            LONG64 llCurTime = 0;
            LONG64 llTimeToExpiry = 0;

            dwError = LocalCfgGetMaxPasswordAge(&llMaxPwdAge);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LocalCfgGetPasswordChangeWarningTime(&llPwdChangeTime);
            BAIL_ON_LSA_ERROR(dwError);

            bPasswordExpired = FALSE;

            llCurTime = LocalGetNTTime(time(NULL));
            llTimeToExpiry = llMaxPwdAge - (llCurTime - llPwdLastSet);

            if (llTimeToExpiry <= llPwdChangeTime)
            {
                bPromptPasswordChange = TRUE;
            }
            else
            {
                bPromptPasswordChange = FALSE;
            }

            dwDaysToPasswordExpiry = llTimeToExpiry/(24 * 60 * 60 * 10000000LL);
        }
    }
    else
    {
        // Password never expires
        bPasswordExpired = FALSE;
        dwDaysToPasswordExpiry = 0;
        bPromptPasswordChange = FALSE;
    }

    if (llAcctExpiry)
    {
        LONG64 llCurTime = LocalGetNTTime(time(NULL));

        bAccountExpired = (llCurTime > llAcctExpiry) ? TRUE : FALSE;
    }

    pUserInfo->bAccountDisabled       = bAccountDisabled;
    pUserInfo->bAccountExpired        = bAccountExpired;
    pUserInfo->bAccountLocked         = bAccountLocked;
    pUserInfo->bPasswordExpired       = bPasswordExpired;
    pUserInfo->bPasswordNeverExpires  = bPasswordNeverExpires;
    pUserInfo->bPromptPasswordChange  = bPromptPasswordChange;
    pUserInfo->bUserCanChangePassword = bUserCanChangePassword;
    pUserInfo->dwDaysToPasswordExpiry = dwDaysToPasswordExpiry;

error:

    return dwError;
}

