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
 *        lpgroup.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Local Authentication Provider
 *
 *        User/Group Database Interface
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "includes.h"

static
DWORD
LocalDirBeginEnumGroups_0(
    HANDLE  hProvider,
    PHANDLE phResume
    );

static
DWORD
LocalDirBeginEnumGroups_1(
    HANDLE  hProvider,
    PHANDLE phResume
    );

static
DWORD
LocalDirEnumGroups_0(
    HANDLE                     hProvider,
    PLOCAL_PROVIDER_ENUM_STATE pEnumState,
    DWORD                      dwNumMaxGroups,
    PDWORD                     pdwNumGroupsFound,
    PVOID**                    pppGroupInfoList
    );

static
DWORD
LocalDirEnumGroups_1(
    HANDLE                     hProvider,
    PLOCAL_PROVIDER_ENUM_STATE pEnumState,
    DWORD                      dwNumMaxGroups,
    PDWORD                     pdwNumGroupsFound,
    PVOID**                    pppGroupInfoList
    );

static
DWORD
LocalDirGetGroupMembersInternal(
    PLOCAL_PROVIDER_CONTEXT pContext,
    PWSTR                   pwszGroupDN,
    DWORD                   dwGroupNestingLevel,
    DWORD                   dwMaxGroupNestingLevel,
    PLSA_HASH_TABLE         pMemberships
    );

DWORD
LocalDirFindGroupByName(
    HANDLE  hProvider,
    PCSTR   pszDomainName,
    PCSTR   pszGroupName,
    DWORD   dwGroupInfoLevel,
    PWSTR*  ppwszGroupDN,
    PVOID*  ppGroupInfo
    )
{
    DWORD dwError = 0;

    switch(dwGroupInfoLevel)
    {
        case 0:

            dwError = LocalDirFindGroupByName_0(
                                hProvider,
                                pszDomainName,
                                pszGroupName,
                                ppwszGroupDN,
                                ppGroupInfo
                                );
            break;

        case 1:

            dwError = LocalDirFindGroupByName_1(
                                hProvider,
                                pszDomainName,
                                pszGroupName,
                                ppwszGroupDN,
                                ppGroupInfo
                                );
            break;

        default:

            dwError = LSA_ERROR_UNSUPPORTED_GROUP_LEVEL;

            break;
    }

    return dwError;
}

DWORD
LocalDirFindGroupByName_0(
    HANDLE  hProvider,
    PCSTR   pszDomainName,
    PCSTR   pszGroupName,
    PWSTR*  ppwszGroupDN,
    PVOID*  ppGroupInfo
    )
{
    DWORD dwError = 0;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;
    wchar16_t wszAttrNameGID[]            = LOCAL_DIR_ATTR_GID;
    wchar16_t wszAttrNameSamAccountName[] = LOCAL_DIR_ATTR_SAM_ACCOUNT_NAME;
    wchar16_t wszAttrNameDN[]             = LOCAL_DIR_ATTR_DISTINGUISHED_NAME;
    wchar16_t wszAttrNameObjectSID[]      = LOCAL_DIR_ATTR_OBJECT_SID;
    PWSTR wszAttrs[] =
    {
        &wszAttrNameGID[0],
        &wszAttrNameSamAccountName[0],
        &wszAttrNameDN[0],
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
    PWSTR pwszDN = NULL;
    PLSA_GROUP_INFO_0 pGroupInfo = NULL;

    dwError = LsaAllocateStringPrintf(
                    &pszFilter,
                    pszFilterTemplate,
                    pszGroupName,
                    pszDomainName,
                    LOCAL_OBJECT_CLASS_GROUP);
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
        dwError = LSA_ERROR_NO_SUCH_GROUP;
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

    dwError = LocalMarshalEntryToGroupInfo_0(
                    pEntry,
                    (ppwszGroupDN ? &pwszDN : NULL),
                    &pGroupInfo);
    BAIL_ON_LSA_ERROR(dwError);

    if (ppwszGroupDN)
    {
        *ppwszGroupDN = pwszDN;
    }
    *ppGroupInfo = pGroupInfo;

cleanup:

    LSA_SAFE_FREE_STRING(pszFilter);
    LSA_SAFE_FREE_MEMORY(pwszFilter);

    if (pEntries)
    {
        DirectoryFreeEntries(pEntries, dwNumEntries);
    }

    return dwError;

error:

    if (ppwszGroupDN)
    {
        *ppwszGroupDN = NULL;
    }
    *ppGroupInfo = pGroupInfo;

    LSA_SAFE_FREE_MEMORY(pwszDN);

    if (pGroupInfo)
    {
        LsaFreeGroupInfo(0, pGroupInfo);
    }

    goto cleanup;
}


DWORD
LocalDirFindGroupByName_1(
    HANDLE  hProvider,
    PCSTR   pszDomainName,
    PCSTR   pszGroupName,
    PWSTR*  ppwszGroupDN,
    PVOID*  ppGroupInfo
    )
{
    DWORD dwError = 0;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;
    wchar16_t wszAttrNameGID[]            = LOCAL_DIR_ATTR_GID;
    wchar16_t wszAttrNameSamAccountName[] = LOCAL_DIR_ATTR_SAM_ACCOUNT_NAME;
    wchar16_t wszAttrNameDN[]             = LOCAL_DIR_ATTR_DISTINGUISHED_NAME;
    wchar16_t wszAttrNameObjectSID[]      = LOCAL_DIR_ATTR_OBJECT_SID;
    PWSTR wszAttrs[] =
    {
        &wszAttrNameGID[0],
        &wszAttrNameSamAccountName[0],
        &wszAttrNameDN[0],
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
    PWSTR pwszDN = NULL;
    DWORD dwInfoLevel = 1;
    PLSA_GROUP_INFO_1 pGroupInfo = NULL;

    dwError = LsaAllocateStringPrintf(
                    &pszFilter,
                    pszFilterTemplate,
                    pszGroupName,
                    pszDomainName,
                    LOCAL_OBJECT_CLASS_GROUP);
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
        dwError = LSA_ERROR_NO_SUCH_GROUP;
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

    dwError = LocalMarshalEntryToGroupInfo_1(
                    pEntry,
                    &pwszDN,
                    &pGroupInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalDirGetGroupMemberNames(
                    pContext,
                    pwszDN,
                    &pGroupInfo->ppszMembers);
    BAIL_ON_LSA_ERROR(dwError);

    if (ppwszGroupDN)
    {
        *ppwszGroupDN = pwszDN;
        pwszDN = NULL;
    }
    *ppGroupInfo = pGroupInfo;

cleanup:

    LSA_SAFE_FREE_STRING(pszFilter);
    LSA_SAFE_FREE_MEMORY(pwszFilter);

    LSA_SAFE_FREE_MEMORY(pwszDN);

    if (pEntries)
    {
        DirectoryFreeEntries(pEntries, dwNumEntries);
    }

    return dwError;

error:

    if (ppwszGroupDN)
    {
        *ppwszGroupDN = NULL;
    }
    *ppGroupInfo = pGroupInfo;

    if (pGroupInfo)
    {
        LsaFreeGroupInfo(dwInfoLevel, pGroupInfo);
    }

    goto cleanup;
}

DWORD
LocalDirFindGroupById(
    HANDLE  hProvider,
    gid_t   gid,
    DWORD   dwGroupInfoLevel,
    PWSTR*  ppwszGroupDN,
    PVOID*  ppGroupInfo
    )
{
    DWORD dwError = 0;

    switch(dwGroupInfoLevel)
    {
        case 0:

            dwError = LocalDirFindGroupById_0(
                            hProvider,
                            gid,
                            ppwszGroupDN,
                            ppGroupInfo);
            break;

        case 1:

            dwError = LocalDirFindGroupById_1(
                            hProvider,
                            gid,
                            ppwszGroupDN,
                            ppGroupInfo);
            break;

        default:

            dwError = LSA_ERROR_UNSUPPORTED_GROUP_LEVEL;

            break;
    }

    return dwError;
}

DWORD
LocalDirFindGroupById_0(
    HANDLE hProvider,
    gid_t  gid,
    PWSTR* ppwszGroupDN,
    PVOID* ppGroupInfo
    )
{
    DWORD dwError = 0;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;
    wchar16_t wszAttrNameGID[]            = LOCAL_DIR_ATTR_GID;
    wchar16_t wszAttrNameSamAccountName[] = LOCAL_DIR_ATTR_SAM_ACCOUNT_NAME;
    wchar16_t wszAttrNameDN[]             = LOCAL_DIR_ATTR_DISTINGUISHED_NAME;
    wchar16_t wszAttrNameObjectSID[]      = LOCAL_DIR_ATTR_OBJECT_SID;
    PWSTR wszAttrs[] =
    {
        &wszAttrNameGID[0],
        &wszAttrNameSamAccountName[0],
        &wszAttrNameDN[0],
        &wszAttrNameObjectSID[0],
        NULL
    };
    DWORD dwNumAttrs = (sizeof(wszAttrs)/sizeof(wszAttrs[0])) - 1;
    PDIRECTORY_ENTRY pEntries = NULL;
    PDIRECTORY_ENTRY pEntry = NULL;
    DWORD dwNumEntries = 0;
    PCSTR pszFilterTemplate =
                    LOCAL_DB_DIR_ATTR_GID " = %u" \
                    " AND " LOCAL_DB_DIR_ATTR_OBJECT_CLASS " = %d";
    PSTR pszFilter = NULL;
    PWSTR pwszFilter = NULL;
    PLSA_GROUP_INFO_0 pGroupInfo = NULL;
    PWSTR pwszGroupDN = NULL;

    dwError = LsaAllocateStringPrintf(
                    &pszFilter,
                    pszFilterTemplate,
                    gid,
                    LOCAL_OBJECT_CLASS_GROUP);
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
        dwError = LSA_ERROR_NO_SUCH_GROUP;
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

    dwError = LocalMarshalEntryToGroupInfo_0(
                    pEntry,
                    (ppwszGroupDN ? &pwszGroupDN : NULL),
                    &pGroupInfo);
    BAIL_ON_LSA_ERROR(dwError);

    if (ppwszGroupDN)
    {
        *ppwszGroupDN = pwszGroupDN;
    }
    *ppGroupInfo = pGroupInfo;

cleanup:

    LSA_SAFE_FREE_STRING(pszFilter);
    LSA_SAFE_FREE_MEMORY(pwszFilter);

    if (pEntries)
    {
        DirectoryFreeEntries(pEntries, dwNumEntries);
    }

    return dwError;

error:

    if (ppwszGroupDN)
    {
        *ppwszGroupDN = NULL;
    }
    *ppGroupInfo = pGroupInfo;

    LSA_SAFE_FREE_MEMORY(pwszGroupDN);

    if (pGroupInfo)
    {
        LsaFreeGroupInfo(0, pGroupInfo);
    }

    goto cleanup;
}

DWORD
LocalDirFindGroupById_1(
    HANDLE hProvider,
    gid_t  gid,
    PWSTR* ppwszGroupDN,
    PVOID* ppGroupInfo
    )
{
    DWORD dwError = 0;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;
    wchar16_t wszAttrNameGID[]            = LOCAL_DIR_ATTR_GID;
    wchar16_t wszAttrNameSamAccountName[] = LOCAL_DIR_ATTR_SAM_ACCOUNT_NAME;
    wchar16_t wszAttrNameDN[]             = LOCAL_DIR_ATTR_DISTINGUISHED_NAME;
    wchar16_t wszAttrNameObjectSID[]      = LOCAL_DIR_ATTR_OBJECT_SID;
    PWSTR wszAttrs[] =
    {
        &wszAttrNameGID[0],
        &wszAttrNameSamAccountName[0],
        &wszAttrNameDN[0],
        &wszAttrNameObjectSID[0],
        NULL
    };
    DWORD dwNumAttrs = (sizeof(wszAttrs)/sizeof(wszAttrs[0])) - 1;
    PDIRECTORY_ENTRY pEntries = NULL;
    PDIRECTORY_ENTRY pEntry = NULL;
    DWORD dwNumEntries = 0;
    PCSTR pszFilterTemplate =
                    LOCAL_DB_DIR_ATTR_GID " = %u" \
                    " AND " LOCAL_DB_DIR_ATTR_OBJECT_CLASS " = %d";
    PSTR pszFilter = NULL;
    PWSTR pwszFilter = NULL;
    DWORD dwInfoLevel = 1;
    PLSA_GROUP_INFO_1 pGroupInfo = NULL;
    PWSTR pwszGroupDN = NULL;

    dwError = LsaAllocateStringPrintf(
                    &pszFilter,
                    pszFilterTemplate,
                    gid,
                    LOCAL_OBJECT_CLASS_GROUP);
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
        dwError = LSA_ERROR_NO_SUCH_GROUP;
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

    dwError = LocalMarshalEntryToGroupInfo_1(
                    pEntry,
                    &pwszGroupDN,
                    &pGroupInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalDirGetGroupMemberNames(
                    pContext,
                    pwszGroupDN,
                    &pGroupInfo->ppszMembers);
    BAIL_ON_LSA_ERROR(dwError);

    if (ppwszGroupDN)
    {
        *ppwszGroupDN = pwszGroupDN;
        pwszGroupDN = NULL;
    }
    *ppGroupInfo = pGroupInfo;

cleanup:

    LSA_SAFE_FREE_STRING(pszFilter);
    LSA_SAFE_FREE_MEMORY(pwszFilter);

    LSA_SAFE_FREE_MEMORY(pwszGroupDN);

    if (pEntries)
    {
        DirectoryFreeEntries(pEntries, dwNumEntries);
    }

    return dwError;

error:

    if (ppwszGroupDN)
    {
        *ppwszGroupDN = NULL;
    }
    *ppGroupInfo = pGroupInfo;

    if (pGroupInfo)
    {
        LsaFreeGroupInfo(dwInfoLevel, pGroupInfo);
    }

    goto cleanup;
}

DWORD
LocalDirGetGroupsForUser_0(
    HANDLE  hProvider,
    uid_t   uid,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
    )
{
    DWORD dwError = 0;

    // TODO:

    return dwError;
}

DWORD
LocalDirGetGroupsForUser_1(
    HANDLE  hProvider,
    uid_t uid,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
    )
{
    DWORD dwError = 0;

    // TODO:

    return dwError;
}

DWORD
LocalDirBeginEnumGroups(
    HANDLE  hProvider,
    DWORD   dwInfoLevel,
    PHANDLE phResume
    )
{
    DWORD dwError = 0;

    switch (dwInfoLevel)
    {
        case 0:

            dwError = LocalDirBeginEnumGroups_0(
                            hProvider,
                            phResume);

            break;

        case 1:

            dwError = LocalDirBeginEnumGroups_1(
                            hProvider,
                            phResume);

            break;

        default:

            dwError = LSA_ERROR_UNSUPPORTED_USER_LEVEL;

            break;
    }

    return dwError;
}

static
DWORD
LocalDirBeginEnumGroups_0(
    HANDLE  hProvider,
    PHANDLE phResume
    )
{
    DWORD dwError = 0;
    DWORD dwInfoLevel = 0;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;
    wchar16_t wszAttrNameGID[]            = LOCAL_DIR_ATTR_GID;
    wchar16_t wszAttrNameSamAccountName[] = LOCAL_DIR_ATTR_SAM_ACCOUNT_NAME;
    wchar16_t wszAttrNameObjectSID[]      = LOCAL_DIR_ATTR_OBJECT_SID;
    PWSTR wszAttrs[] =
    {
        &wszAttrNameGID[0],
        &wszAttrNameSamAccountName[0],
        &wszAttrNameObjectSID[0],
        NULL
    };
    PCSTR pszFilterTemplate =
                    LOCAL_DB_DIR_ATTR_DOMAIN   " = \"%s\"" \
                    " AND " LOCAL_DB_DIR_ATTR_OBJECT_CLASS " = %d";
    PSTR pszFilter = NULL;
    PWSTR pwszFilter = NULL;
    PLOCAL_PROVIDER_ENUM_STATE pEnumState = NULL;

    dwError = LocalCreateGroupState(
                        hProvider,
                        dwInfoLevel,
                        &pEnumState);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAllocateStringPrintf(
                    &pszFilter,
                    pszFilterTemplate,
                    gLPGlobals.pszLocalDomain,
                    LOCAL_OBJECT_CLASS_GROUP);
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
                    &pEnumState->pEntries,
                    &pEnumState->dwNumEntries);
    BAIL_ON_LSA_ERROR(dwError);

    *phResume = (HANDLE)pEnumState;

cleanup:

    LSA_SAFE_FREE_STRING(pszFilter);
    LSA_SAFE_FREE_MEMORY(pwszFilter);

    return dwError;

error:

    *phResume = (HANDLE)NULL;

    if (pEnumState)
    {
        LocalFreeEnumState(pEnumState);
    }

    goto cleanup;
}

static
DWORD
LocalDirBeginEnumGroups_1(
    HANDLE  hProvider,
    PHANDLE phResume
    )
{
    DWORD dwError = 0;
    DWORD dwInfoLevel = 0;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;
    wchar16_t wszAttrNameGID[]            = LOCAL_DIR_ATTR_GID;
    wchar16_t wszAttrNameSamAccountName[] = LOCAL_DIR_ATTR_SAM_ACCOUNT_NAME;
    wchar16_t wszAttrNameDN[]             = LOCAL_DIR_ATTR_DISTINGUISHED_NAME;
    wchar16_t wszAttrNameObjectSID[]      = LOCAL_DIR_ATTR_OBJECT_SID;
    PWSTR wszAttrs[] =
    {
        &wszAttrNameGID[0],
        &wszAttrNameSamAccountName[0],
        &wszAttrNameDN[0],
        &wszAttrNameObjectSID[0],
        NULL
    };
    PCSTR pszFilterTemplate =
                    LOCAL_DB_DIR_ATTR_DOMAIN   " = \"%s\"" \
                    " AND " LOCAL_DB_DIR_ATTR_OBJECT_CLASS " = %d";
    PSTR pszFilter = NULL;
    PWSTR pwszFilter = NULL;
    PLOCAL_PROVIDER_ENUM_STATE pEnumState = NULL;

    dwError = LocalCreateGroupState(
                        hProvider,
                        dwInfoLevel,
                        &pEnumState);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAllocateStringPrintf(
                    &pszFilter,
                    pszFilterTemplate,
                    gLPGlobals.pszLocalDomain,
                    LOCAL_OBJECT_CLASS_GROUP);
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
                    &pEnumState->pEntries,
                    &pEnumState->dwNumEntries);
    BAIL_ON_LSA_ERROR(dwError);

    *phResume = (HANDLE)pEnumState;

cleanup:

    LSA_SAFE_FREE_STRING(pszFilter);
    LSA_SAFE_FREE_MEMORY(pwszFilter);

    return dwError;

error:

    *phResume = (HANDLE)NULL;

    if (pEnumState)
    {
        LocalFreeEnumState(pEnumState);
    }

    goto cleanup;
}

DWORD
LocalDirEnumGroups(
    HANDLE  hProvider,
    HANDLE  hResume,
    DWORD   nMaxGroups,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
    )
{
    DWORD dwError = 0;
    PLOCAL_PROVIDER_ENUM_STATE pEnumState = (PLOCAL_PROVIDER_ENUM_STATE)hResume;

    BAIL_ON_INVALID_POINTER(pEnumState);

    switch(pEnumState->dwInfoLevel)
    {
        case 0:

            dwError = LocalDirEnumGroups_0(
                            hProvider,
                            pEnumState,
                            nMaxGroups,
                            pdwGroupsFound,
                            pppGroupInfoList
                            );
            break;

        case 1:

            dwError = LocalDirEnumGroups_1(
                            hProvider,
                            pEnumState,
                            nMaxGroups,
                            pdwGroupsFound,
                            pppGroupInfoList
                            );
            break;

        default:

            dwError = LSA_ERROR_UNSUPPORTED_GROUP_LEVEL;

            break;
    }

error:

    return dwError;
}

static
DWORD
LocalDirEnumGroups_0(
    HANDLE                     hProvider,
    PLOCAL_PROVIDER_ENUM_STATE pEnumState,
    DWORD                      dwNumMaxGroups,
    PDWORD                     pdwNumGroupsFound,
    PVOID**                    pppGroupInfoList
    )
{
    DWORD dwError = 0;
    DWORD dwNumGroupsAvailable = 0;
    DWORD dwNumGroupsFound = 0;
    PLSA_GROUP_INFO_0* ppGroupInfoList = NULL;
    DWORD iGroup = 0;
    BOOLEAN bInLock = FALSE;

    LOCAL_LOCK_MUTEX(bInLock, &pEnumState->mutex);

    if (pEnumState->dwInfoLevel != 0)
    {
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwNumGroupsAvailable = pEnumState->dwNumEntries - pEnumState->dwNextStartingId;

    dwNumGroupsFound = LSA_MIN(dwNumMaxGroups, dwNumGroupsAvailable);

    if (!dwNumGroupsFound)
    {
        dwError = LSA_ERROR_NO_MORE_GROUPS;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaAllocateMemory(
                    dwNumGroupsFound * sizeof(PLSA_GROUP_INFO_0),
                    (PVOID*)&ppGroupInfoList);
    BAIL_ON_LSA_ERROR(dwError);

    for (; iGroup < dwNumGroupsFound; iGroup++)
    {
        PDIRECTORY_ENTRY pEntry = NULL;

        pEntry = &pEnumState->pEntries[pEnumState->dwNextStartingId + iGroup];

        dwError = LocalMarshalEntryToGroupInfo_0(
                        pEntry,
                        NULL,
                        &ppGroupInfoList[iGroup]);
        BAIL_ON_LSA_ERROR(dwError);
    }

    pEnumState->dwNextStartingId += dwNumGroupsFound;

    *pdwNumGroupsFound = dwNumGroupsFound;
    *pppGroupInfoList = (PVOID*)ppGroupInfoList;

cleanup:

    LOCAL_UNLOCK_MUTEX(bInLock, &pEnumState->mutex);

    return dwError;

error:

    *pdwNumGroupsFound = 0;
    *pppGroupInfoList = NULL;

    if (ppGroupInfoList)
    {
        LsaFreeGroupInfoList(
                pEnumState->dwInfoLevel,
                (PVOID*)ppGroupInfoList,
                dwNumGroupsFound);
    }

    goto cleanup;
}

static
DWORD
LocalDirEnumGroups_1(
    HANDLE                     hProvider,
    PLOCAL_PROVIDER_ENUM_STATE pEnumState,
    DWORD                      dwNumMaxGroups,
    PDWORD                     pdwNumGroupsFound,
    PVOID**                    pppGroupInfoList
    )
{
    DWORD dwError = 0;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;
    DWORD dwNumGroupsAvailable = 0;
    DWORD dwNumGroupsFound = 0;
    PLSA_GROUP_INFO_1* ppGroupInfoList = NULL;
    DWORD iGroup = 0;
    PWSTR pwszGroupDN = NULL;
    BOOLEAN bInLock = FALSE;

    LOCAL_LOCK_MUTEX(bInLock, &pEnumState->mutex);

    if (pEnumState->dwInfoLevel != 0)
    {
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwNumGroupsAvailable = pEnumState->dwNumEntries - pEnumState->dwNextStartingId;

    dwNumGroupsFound = LSA_MIN(dwNumMaxGroups, dwNumGroupsAvailable);

    if (!dwNumGroupsFound)
    {
        dwError = LSA_ERROR_NO_MORE_GROUPS;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaAllocateMemory(
                    dwNumGroupsFound * sizeof(PLSA_GROUP_INFO_1),
                    (PVOID*)&ppGroupInfoList);
    BAIL_ON_LSA_ERROR(dwError);

    for (; iGroup < dwNumGroupsFound; iGroup++)
    {
        PDIRECTORY_ENTRY pEntry = NULL;

        pEntry = &pEnumState->pEntries[pEnumState->dwNextStartingId + iGroup];

        LSA_SAFE_FREE_MEMORY(pwszGroupDN);

        dwError = LocalMarshalEntryToGroupInfo_1(
                        pEntry,
                        &pwszGroupDN,
                        &ppGroupInfoList[iGroup]);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LocalDirGetGroupMemberNames(
                        pContext,
                        pwszGroupDN,
                        &ppGroupInfoList[iGroup]->ppszMembers);
        BAIL_ON_LSA_ERROR(dwError);
    }

    pEnumState->dwNextStartingId += dwNumGroupsFound;

    *pdwNumGroupsFound = dwNumGroupsFound;
    *pppGroupInfoList = (PVOID*)ppGroupInfoList;

cleanup:

    LSA_SAFE_FREE_MEMORY(pwszGroupDN);

    LOCAL_UNLOCK_MUTEX(bInLock, &pEnumState->mutex);

    return dwError;

error:

    *pdwNumGroupsFound = 0;
    *pppGroupInfoList = NULL;

    if (ppGroupInfoList)
    {
        LsaFreeGroupInfoList(
                pEnumState->dwInfoLevel,
                (PVOID*)ppGroupInfoList,
                dwNumGroupsFound);
    }

    goto cleanup;
}

DWORD
LocalDirGetGroupMemberNames(
    PLOCAL_PROVIDER_CONTEXT pContext,
    PWSTR                   pwszGroupDN,
    PSTR**                  pppszMembers
    )
{
    DWORD dwError = 0;
    PLOCAL_PROVIDER_GROUP_MEMBER* ppMemberEntries = NULL;
    DWORD                         dwNumMemberEntries = 0;
    PSTR* ppszMembers = NULL;

    dwError = LocalDirGetGroupMembers(
                    pContext,
                    pwszGroupDN,
                    &ppMemberEntries,
                    &dwNumMemberEntries);
    BAIL_ON_LSA_ERROR(dwError);

    if (dwNumMemberEntries)
    {
        dwError = LocalMarshalEntryToGroupInfoMembers_1(
                        ppMemberEntries,
                        dwNumMemberEntries,
                        &ppszMembers);
        BAIL_ON_LSA_ERROR(dwError);
    }

    *pppszMembers = ppszMembers;

cleanup:

    if (ppMemberEntries)
    {
        LocalDirFreeGroupMemberList(ppMemberEntries, dwNumMemberEntries);
    }

    return dwError;

error:

    *pppszMembers = NULL;

    if (ppszMembers)
    {
        DWORD iMember = 0;
        while (ppszMembers[iMember])
        {
            LsaFreeString(ppszMembers[iMember++]);
        }

        LsaFreeMemory(ppszMembers);
    }

    goto cleanup;
}

DWORD
LocalDirGetGroupMembers(
    PLOCAL_PROVIDER_CONTEXT        pContext,
    PWSTR                          pwszGroupDN,
    PLOCAL_PROVIDER_GROUP_MEMBER** pppGroupMembers,
    PDWORD                         pdwNumMembers
    )
{
    DWORD dwError = 0;
    DWORD dwGroupNestingLevel = 0;
    DWORD dwMaxGroupNestingLevel = 0;
    PLSA_HASH_TABLE   pMemberships = NULL;
    LSA_HASH_ITERATOR hashIterator = {0};
    LSA_HASH_ENTRY*   pHashEntry = NULL;
    DWORD             dwNumMembers = 0;
    DWORD             iMember = 0;
    PLOCAL_PROVIDER_GROUP_MEMBER* ppGroupMembers = NULL;

    dwError = LocalCfgGetMaxGroupNestingLevel(&dwMaxGroupNestingLevel);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaHashCreate(
                    13,
                    LsaHashCaselessStringCompare,
                    LsaHashCaselessString,
                    NULL,
                    NULL,
                    &pMemberships);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalDirGetGroupMembersInternal(
                    pContext,
                    pwszGroupDN,
                    dwGroupNestingLevel,
                    dwMaxGroupNestingLevel,
                    pMemberships);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaHashGetIterator(pMemberships, &hashIterator);
    BAIL_ON_LSA_ERROR(dwError);

    while (LsaHashNext(&hashIterator))
    {
        dwNumMembers++;
    }

    dwError = LsaAllocateMemory(
                    sizeof(PLOCAL_PROVIDER_GROUP_MEMBER) * dwNumMembers,
                    (PVOID*)&ppGroupMembers);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaHashGetIterator(pMemberships, &hashIterator);
    BAIL_ON_LSA_ERROR(dwError);

    for (; (pHashEntry = LsaHashNext(&hashIterator)) != NULL; iMember++)
    {
        ppGroupMembers[iMember] = pHashEntry->pValue;
    }

    *pppGroupMembers = ppGroupMembers;
    *pdwNumMembers = dwNumMembers;

cleanup:

    if (pMemberships)
    {
        LsaHashSafeFree(&pMemberships);
    }

    return dwError;

error:

    *pppGroupMembers = NULL;
    *pdwNumMembers = 0;

    LSA_SAFE_FREE_MEMORY(ppGroupMembers);

    if (pMemberships)
    {
        DWORD dwError2 = 0;

        dwError2 = LsaHashGetIterator(pMemberships, &hashIterator);
        if (dwError2)
        {
            LSA_LOG_ERROR("Failed to iterate hash table [code: %d]", dwError2);
        }
        else
        {
            while ((pHashEntry = LsaHashNext(&hashIterator)) != NULL)
            {
                if (pHashEntry->pValue)
                {
                    LocalDirFreeGroupMember(
                            (PLOCAL_PROVIDER_GROUP_MEMBER)pHashEntry->pValue
                            );
                }
            }
        }
    }

    goto cleanup;
}

static
DWORD
LocalDirGetGroupMembersInternal(
    PLOCAL_PROVIDER_CONTEXT pContext,
    PWSTR                   pwszGroupDN,
    DWORD                   dwGroupNestingLevel,
    DWORD                   dwMaxGroupNestingLevel,
    PLSA_HASH_TABLE         pMemberships
    )
{
    DWORD dwError = 0;
    wchar16_t wszAttrNameObjectClass[]    = LOCAL_DIR_ATTR_OBJECT_CLASS;
    wchar16_t wszAttrNameObjectSID[]      = LOCAL_DIR_ATTR_OBJECT_SID;
    wchar16_t wszAttrNameDN[]             = LOCAL_DIR_ATTR_DISTINGUISHED_NAME;
    wchar16_t wszAttrNameSamAccountName[] = LOCAL_DIR_ATTR_SAM_ACCOUNT_NAME;
    wchar16_t wszAttrNameDomain[]         = LOCAL_DIR_ATTR_DOMAIN;
    wchar16_t wszAttrNameNetbiosName[]    = LOCAL_DIR_ATTR_NETBIOS_NAME;
    PWSTR     pwszAttrs[] =
    {
            &wszAttrNameObjectClass[0],
            &wszAttrNameObjectSID[0],
            &wszAttrNameDN[0],
            &wszAttrNameSamAccountName[0],
            &wszAttrNameDomain[0],
            &wszAttrNameNetbiosName[0],
            NULL
    };
    PLOCAL_PROVIDER_GROUP_MEMBER pGroupMember = NULL;
    PSTR             pszSID = NULL;
    PWSTR            pwszChildGroupDN = NULL;
    PDIRECTORY_ENTRY pMemberEntries = NULL;
    DWORD            dwNumEntries = 0;
    DWORD            iEntry = 0;

    dwError = DirectoryGetGroupMembers(
                    pContext->hDirectory,
                    pwszGroupDN,
                    pwszAttrs,
                    &pMemberEntries,
                    &dwNumEntries);
    BAIL_ON_LSA_ERROR(dwError);

    for (; iEntry < dwNumEntries; iEntry++)
    {
        DWORD dwObjectClass = 0;
        PDIRECTORY_ENTRY pEntry = &pMemberEntries[iEntry];

        dwError = LocalMarshalAttrToInteger(
                        pEntry,
                        &wszAttrNameObjectClass[0],
                        &dwObjectClass);
        BAIL_ON_LSA_ERROR(dwError);

        switch (dwObjectClass)
        {
            case LOCAL_OBJECT_CLASS_GROUP:

                if (dwGroupNestingLevel < dwMaxGroupNestingLevel)
                {
                    LSA_SAFE_FREE_MEMORY(pwszChildGroupDN);

                    dwError = LocalDirGetGroupMembersInternal(
                                    pContext,
                                    pwszChildGroupDN,
                                    dwGroupNestingLevel + 1,
                                    dwMaxGroupNestingLevel,
                                    pMemberships);
                    BAIL_ON_LSA_ERROR(dwError);
                }
                else
                {
                    LSA_LOG_VERBOSE("Maximum group nesting level [%d] reached. "
                                    "Skipping further resolution",
                                    dwMaxGroupNestingLevel);
                }

                break;

            case LOCAL_OBJECT_CLASS_USER:

                LSA_SAFE_FREE_STRING(pszSID);

                dwError = LocalMarshalAttrToANSIString(
                                pEntry,
                                &wszAttrNameObjectSID[0],
                                &pszSID);
                BAIL_ON_LSA_ERROR(dwError);

                dwError = LsaHashGetValue(
                            pMemberships,
                            (PCVOID)pszSID,
                            (PVOID*)&pGroupMember);
                if (dwError == ENOENT)
                {
                    dwError = LsaAllocateMemory(
                                    sizeof(LOCAL_PROVIDER_GROUP_MEMBER),
                                    (PVOID*)&pGroupMember);
                    BAIL_ON_LSA_ERROR(dwError);

                    pGroupMember->pszSID = pszSID;
                    pszSID = NULL;

                    dwError = LocalMarshalAttrToANSIString(
                                    pEntry,
                                    &wszAttrNameDomain[0],
                                    &pGroupMember->pszDomain);
                    BAIL_ON_LSA_ERROR(dwError);

                    dwError = LocalMarshalAttrToANSIString(
                                    pEntry,
                                    &wszAttrNameNetbiosName[0],
                                    &pGroupMember->pszNetbiosDomain);
                    BAIL_ON_LSA_ERROR(dwError);

                    dwError = LocalMarshalAttrToANSIString(
                                    pEntry,
                                    &wszAttrNameSamAccountName[0],
                                    &pGroupMember->pszSamAccountName);
                    BAIL_ON_LSA_ERROR(dwError);

                    dwError = LsaHashSetValue(
                                    pMemberships,
                                    (PVOID)pGroupMember->pszSID,
                                    (PVOID)pGroupMember);
                    BAIL_ON_LSA_ERROR(dwError);

                    pGroupMember = NULL;
                }
                BAIL_ON_LSA_ERROR(dwError);

                break;

            default:

                dwError = LSA_ERROR_INTERNAL;
                BAIL_ON_LSA_ERROR(dwError);

                break;
        }
    }

cleanup:

    LSA_SAFE_FREE_MEMORY(pwszChildGroupDN);
    LSA_SAFE_FREE_MEMORY(pszSID);

    if (pGroupMember)
    {
        LocalDirFreeGroupMember(pGroupMember);
    }

    if (pMemberEntries)
    {
        DirectoryFreeEntries(pMemberEntries, dwNumEntries);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
LocalDirAddGroup(
    HANDLE hProvider,
    DWORD  dwInfoLevel,
    PVOID  pGroupInfo
    )
{
    DWORD dwError = 0;

    switch(dwInfoLevel)
    {
        case 0:

            dwError = LocalDirAddGroup_0(
                            hProvider,
                            (PLSA_GROUP_INFO_0)pGroupInfo);
            break;

        case 1:

            dwError = LocalDirAddGroup_1(
                            hProvider,
                            (PLSA_GROUP_INFO_1)pGroupInfo);

            break;

        default:

            dwError = LSA_ERROR_UNSUPPORTED_GROUP_LEVEL;

            break;
    }
    BAIL_ON_LSA_ERROR(dwError);

error:

    return dwError;
}

DWORD
LocalDirAddGroup_0(
    HANDLE            hProvider,
    PLSA_GROUP_INFO_0 pGroupInfo
    )
{
    DWORD dwError = 0;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;
    BOOLEAN bEventlogEnabled = FALSE;
    PLSA_LOGIN_NAME_INFO pLoginInfo = NULL;
    PWSTR pwszGroupDN = NULL;
    enum AttrValueIndex
    {
        LOCAL_DAG0_IDX_SAM_ACCOUNT_NAME = 0,
        LOCAL_DAG0_IDX_OBJECTCLASS
    };
    ATTRIBUTE_VALUE attrValues[] =
    {
        {       /* LOCAL_DIR_ADD_USER_0_IDX_SAM_ACCOUNT_NAME */
                .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                .data.pwszStringValue = NULL
        },
        {       /* LOCAL_DIR_ADD_USER_0_IDX_OBJECTCLASS */
                .Type = DIRECTORY_ATTR_TYPE_INTEGER,
                .data.ulValue = LOCAL_OBJECT_CLASS_GROUP
        }
    };
    WCHAR wszAttrObjectClass[]    = LOCAL_DIR_ATTR_OBJECT_CLASS;
    WCHAR wszAttrSamAccountName[] = LOCAL_DIR_ATTR_SAM_ACCOUNT_NAME;
    WCHAR wszCNPrefix[]           = LOCAL_DIR_CN_PREFIX;
    DIRECTORY_MOD mods[] =
    {
            {
                    DIR_MOD_FLAGS_ADD,
                    &wszAttrObjectClass[0],
                    1,
                    &attrValues[LOCAL_DAG0_IDX_OBJECTCLASS]
            },
            {
                    DIR_MOD_FLAGS_ADD,
                    &wszAttrSamAccountName[0],
                    1,
                    &attrValues[LOCAL_DAG0_IDX_SAM_ACCOUNT_NAME]
            },
            {
                    DIR_MOD_FLAGS_ADD,
                    NULL,
                    1,
                    NULL
            }
    };
    PWSTR pwszSamAccountName = NULL;

    BAIL_ON_INVALID_STRING(pGroupInfo->pszName);

    dwError = LsaCrackDomainQualifiedName(
                    pGroupInfo->pszName,
                    NULL,
                    &pLoginInfo);
    BAIL_ON_LSA_ERROR(dwError);

    if (!LocalServicesDomain(pLoginInfo->pszDomainNetBiosName))
    {
        dwError = LSA_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaMbsToWc16s(
                    pGroupInfo->pszName,
                    &pwszSamAccountName);
    BAIL_ON_LSA_ERROR(dwError);

    attrValues[LOCAL_DAG0_IDX_SAM_ACCOUNT_NAME].data.pwszStringValue = pwszSamAccountName;

    dwError = LsaAllocateMemory(
                    sizeof(wszCNPrefix) + strlen(pGroupInfo->pszName) * sizeof(WCHAR),
                    (PVOID*)&pwszGroupDN);
    BAIL_ON_LSA_ERROR(dwError);

    // Build CN=<sam account name>
    memcpy((PBYTE)pwszGroupDN, (PBYTE)&wszCNPrefix[0], sizeof(wszCNPrefix) - sizeof(WCHAR));
    memcpy((PBYTE)(pwszGroupDN) + sizeof(wszCNPrefix) - sizeof(WCHAR),
           (PBYTE)pwszSamAccountName,
           wc16slen(pwszSamAccountName) * sizeof(WCHAR));

    dwError = DirectoryAddObject(
                    pContext->hDirectory,
                    pwszGroupDN,
                    mods);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalCfgIsEventlogEnabled(&bEventlogEnabled);
    BAIL_ON_LSA_ERROR(dwError);

    if (bEventlogEnabled)
    {
        LocalEventLogGroupAdd(pLoginInfo->pszName,
                             ((PLSA_GROUP_INFO_0)pGroupInfo)->gid);
    }

cleanup:

    if (pLoginInfo)
    {
        LsaFreeNameInfo(pLoginInfo);
    }

    LSA_SAFE_FREE_MEMORY(pwszGroupDN);
    LSA_SAFE_FREE_MEMORY(pwszSamAccountName);

    return dwError;

error:

    goto cleanup;
}

DWORD
LocalDirAddGroup_1(
    HANDLE            hProvider,
    PLSA_GROUP_INFO_1 pGroupInfo
    )
{
    DWORD dwError = 0;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;
    BOOLEAN bEventlogEnabled = FALSE;
    PLSA_LOGIN_NAME_INFO pLoginInfo = NULL;
    PWSTR pwszGroupDN = NULL;
    enum AttrValueIndex
    {
        LOCAL_DAG0_IDX_SAM_ACCOUNT_NAME = 0,
        LOCAL_DAG0_IDX_OBJECTCLASS
    };
    ATTRIBUTE_VALUE attrValues[] =
    {
        {       /* LOCAL_DIR_ADD_USER_0_IDX_SAM_ACCOUNT_NAME */
                .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                .data.pwszStringValue = NULL
        },
        {       /* LOCAL_DIR_ADD_USER_0_IDX_OBJECTCLASS */
                .Type = DIRECTORY_ATTR_TYPE_INTEGER,
                .data.ulValue = LOCAL_OBJECT_CLASS_GROUP
        }
    };
    WCHAR wszAttrObjectClass[]    = LOCAL_DIR_ATTR_OBJECT_CLASS;
    WCHAR wszAttrSamAccountName[] = LOCAL_DIR_ATTR_SAM_ACCOUNT_NAME;
    WCHAR wszCNPrefix[]           = LOCAL_DIR_CN_PREFIX;
    DIRECTORY_MOD mods[] =
    {
            {
                    DIR_MOD_FLAGS_ADD,
                    &wszAttrObjectClass[0],
                    1,
                    &attrValues[LOCAL_DAG0_IDX_OBJECTCLASS]
            },
            {
                    DIR_MOD_FLAGS_ADD,
                    &wszAttrSamAccountName[0],
                    1,
                    &attrValues[LOCAL_DAG0_IDX_SAM_ACCOUNT_NAME]
            },
            {
                    DIR_MOD_FLAGS_ADD,
                    NULL,
                    1,
                    NULL
            }
    };
    PWSTR pwszSamAccountName = NULL;

    BAIL_ON_INVALID_STRING(pGroupInfo->pszName);

    dwError = LsaCrackDomainQualifiedName(
                    pGroupInfo->pszName,
                    NULL,
                    &pLoginInfo);
    BAIL_ON_LSA_ERROR(dwError);

    if (!LocalServicesDomain(pLoginInfo->pszDomainNetBiosName))
    {
        dwError = LSA_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaMbsToWc16s(
                    pGroupInfo->pszName,
                    &pwszSamAccountName);
    BAIL_ON_LSA_ERROR(dwError);

    attrValues[LOCAL_DAG0_IDX_SAM_ACCOUNT_NAME].data.pwszStringValue = pwszSamAccountName;

    dwError = LsaAllocateMemory(
                    sizeof(wszCNPrefix) + strlen(pGroupInfo->pszName) * sizeof(WCHAR),
                    (PVOID*)&pwszGroupDN);
    BAIL_ON_LSA_ERROR(dwError);

    // Build CN=<sam account name>
    memcpy((PBYTE)pwszGroupDN, (PBYTE)&wszCNPrefix[0], sizeof(wszCNPrefix) - sizeof(WCHAR));
    memcpy((PBYTE)(pwszGroupDN) + sizeof(wszCNPrefix) - sizeof(WCHAR),
           (PBYTE)pwszSamAccountName,
           wc16slen(pwszSamAccountName) * sizeof(WCHAR));

    dwError = DirectoryAddObject(
                    pContext->hDirectory,
                    pwszGroupDN,
                    mods);
    BAIL_ON_LSA_ERROR(dwError);

    // TODO: Add group members
#if 0
    if (!IsNullOrEmptyString(pGroupInfo->ppszMembers))
    {
        dwError = DirectoryAddMembers(
                        pwszGroupDN,
                        pGroupInfo->ppszMembers);
        BAIL_ON_LSA_ERROR(dwError);
    }
#endif

    dwError = LocalCfgIsEventlogEnabled(&bEventlogEnabled);
    BAIL_ON_LSA_ERROR(dwError);

    if (bEventlogEnabled)
    {
        LocalEventLogGroupAdd(pLoginInfo->pszName,
                             ((PLSA_GROUP_INFO_0)pGroupInfo)->gid);
    }

cleanup:

    if (pLoginInfo)
    {
        LsaFreeNameInfo(pLoginInfo);
    }

    LSA_SAFE_FREE_MEMORY(pwszGroupDN);
    LSA_SAFE_FREE_MEMORY(pwszSamAccountName);

    return dwError;

error:

    goto cleanup;
}

DWORD
LocalDirDeleteGroup(
    HANDLE hProvider,
    PWSTR  pwszGroupDN
    )
{
    DWORD dwError = 0;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;

    dwError = DirectoryDeleteObject(
                    pContext->hDirectory,
                    pwszGroupDN);
    BAIL_ON_LSA_ERROR(dwError);

error:

    return dwError;
}

VOID
LocalDirFreeGroupMemberList(
    PLOCAL_PROVIDER_GROUP_MEMBER* ppMemberList,
    DWORD                         dwNumMembers
    )
{
    DWORD iMember = 0;

    for (; iMember < dwNumMembers; iMember++)
    {
        if (ppMemberList[iMember])
        {
            LocalDirFreeGroupMember(ppMemberList[iMember]);
        }
    }

    LsaFreeMemory(ppMemberList);
}

VOID
LocalDirFreeGroupMember(
    PLOCAL_PROVIDER_GROUP_MEMBER pMember
    )
{
    LSA_SAFE_FREE_STRING(pMember->pszDomain);
    LSA_SAFE_FREE_STRING(pMember->pszNetbiosDomain);
    LSA_SAFE_FREE_STRING(pMember->pszSamAccountName);
    LSA_SAFE_FREE_STRING(pMember->pszSID);

    LsaFreeMemory(pMember);
}


