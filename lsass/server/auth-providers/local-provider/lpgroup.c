/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 * Authors: Krishna Ganugapati (krishnag@likewise.com)
 *          Sriram Nambakam (snambakam@likewise.com)
 *          Rafal Szczesniak (rafal@likewise.com)
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

static
DWORD
LocalDirValidateGID(
    gid_t gid
    );

static
DWORD
LocalAddMembersToGroup(
    PLOCAL_PROVIDER_CONTEXT pContext,
    PWSTR                   pwszGroupDN,
    PSTR*                   ppszMembers
    );

static
DWORD
LocalDirCreateForeignPrincipalDN(
    HANDLE     hProvider,
    PWSTR      pwszSID,
    PWSTR     *ppwszDN
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

            dwError = LW_ERROR_UNSUPPORTED_GROUP_LEVEL;

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
    wchar16_t wszAttrNameNetBIOSDomain[]  = LOCAL_DIR_ATTR_NETBIOS_NAME;
    PWSTR wszAttrs[] =
    {
        &wszAttrNameGID[0],
        &wszAttrNameSamAccountName[0],
        &wszAttrNameDN[0],
        &wszAttrNameObjectSID[0],
        &wszAttrNameNetBIOSDomain[0],
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

    dwError = LwAllocateStringPrintf(
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
        dwError = LW_ERROR_NO_SUCH_GROUP;
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

    LW_SAFE_FREE_STRING(pszFilter);
    LW_SAFE_FREE_MEMORY(pwszFilter);

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

    LW_SAFE_FREE_MEMORY(pwszDN);

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
    wchar16_t wszAttrNameNetBIOSDomain[]      = LOCAL_DIR_ATTR_NETBIOS_NAME;
    PWSTR wszAttrs[] =
    {
        &wszAttrNameGID[0],
        &wszAttrNameSamAccountName[0],
        &wszAttrNameDN[0],
        &wszAttrNameObjectSID[0],
        &wszAttrNameNetBIOSDomain[0],
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

    dwError = LwAllocateStringPrintf(
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
        dwError = LW_ERROR_NO_SUCH_GROUP;
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

    LW_SAFE_FREE_STRING(pszFilter);
    LW_SAFE_FREE_MEMORY(pwszFilter);

    LW_SAFE_FREE_MEMORY(pwszDN);

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
    *ppGroupInfo = NULL;

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

            dwError = LW_ERROR_UNSUPPORTED_GROUP_LEVEL;

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
    wchar16_t wszAttrNameNetBIOSDomain[]      = LOCAL_DIR_ATTR_NETBIOS_NAME;
    PWSTR wszAttrs[] =
    {
        &wszAttrNameGID[0],
        &wszAttrNameSamAccountName[0],
        &wszAttrNameDN[0],
        &wszAttrNameObjectSID[0],
        &wszAttrNameNetBIOSDomain[0],
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

    dwError = LwAllocateStringPrintf(
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
        dwError = LW_ERROR_NO_SUCH_GROUP;
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

    LW_SAFE_FREE_STRING(pszFilter);
    LW_SAFE_FREE_MEMORY(pwszFilter);

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
    *ppGroupInfo = NULL;

    LW_SAFE_FREE_MEMORY(pwszGroupDN);

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
    wchar16_t wszAttrNameNetBIOSDomain[]      = LOCAL_DIR_ATTR_NETBIOS_NAME;
    PWSTR wszAttrs[] =
    {
        &wszAttrNameGID[0],
        &wszAttrNameSamAccountName[0],
        &wszAttrNameDN[0],
        &wszAttrNameObjectSID[0],
        &wszAttrNameNetBIOSDomain[0],
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

    dwError = LwAllocateStringPrintf(
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
        dwError = LW_ERROR_NO_SUCH_GROUP;
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

    LW_SAFE_FREE_STRING(pszFilter);
    LW_SAFE_FREE_MEMORY(pwszFilter);

    LW_SAFE_FREE_MEMORY(pwszGroupDN);

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
    *ppGroupInfo = NULL;

    if (pGroupInfo)
    {
        LsaFreeGroupInfo(dwInfoLevel, pGroupInfo);
    }

    goto cleanup;
}

DWORD
LocalDirGetGroupsForUser(
    HANDLE  hProvider,
    PWSTR   pwszUserDN,
    DWORD   dwInfoLevel,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
    )
{
    DWORD dwError = 0;

    switch(dwInfoLevel)
    {
        case 0:

            dwError = LocalDirGetGroupsForUser_0(
                                hProvider,
                                pwszUserDN,
                                pdwGroupsFound,
                                pppGroupInfoList
                                );
            break;

        case 1:

            dwError = LocalDirGetGroupsForUser_1(
                                hProvider,
                                pwszUserDN,
                                pdwGroupsFound,
                                pppGroupInfoList
                                );
            break;

        default:

            dwError = LW_ERROR_UNSUPPORTED_GROUP_LEVEL;

            break;
    }

    return dwError;
}

DWORD
LocalDirGetGroupsForUser_0(
    HANDLE  hProvider,
    PWSTR   pwszUserDN,
    PDWORD  pdwNumGroupsFound,
    PVOID** pppGroupInfoList
    )
{
    DWORD dwError = 0;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;
    wchar16_t wszAttrNameGID[]            = LOCAL_DIR_ATTR_GID;
    wchar16_t wszAttrNameSamAccountName[] = LOCAL_DIR_ATTR_SAM_ACCOUNT_NAME;
    wchar16_t wszAttrNameDN[]             = LOCAL_DIR_ATTR_DISTINGUISHED_NAME;
    wchar16_t wszAttrNameObjectSID[]      = LOCAL_DIR_ATTR_OBJECT_SID;
    wchar16_t wszAttrNameNetBIOSDomain[]      = LOCAL_DIR_ATTR_NETBIOS_NAME;
    PWSTR     pwszAttrs[] =
    {
        &wszAttrNameGID[0],
        &wszAttrNameSamAccountName[0],
        &wszAttrNameDN[0],
        &wszAttrNameObjectSID[0],
        &wszAttrNameNetBIOSDomain[0],
        NULL
    };
    PDIRECTORY_ENTRY   pDirectoryEntries = NULL;
    DWORD              iEntry = 0;
    DWORD              dwGroupInfoLevel = 0;
    PLSA_GROUP_INFO_0* ppGroupInfoList = NULL;
    DWORD              dwNumGroupsFound = 0;

    dwError = DirectoryGetMemberships(
                    pContext->hDirectory,
                    pwszUserDN,
                    pwszAttrs,
                    &pDirectoryEntries,
                    &dwNumGroupsFound);
    BAIL_ON_LSA_ERROR(dwError);

    if (dwNumGroupsFound)
    {
        dwError = LwAllocateMemory(
                        sizeof(PLSA_GROUP_INFO_0) * dwNumGroupsFound,
                        (PVOID*)&ppGroupInfoList);
        BAIL_ON_LSA_ERROR(dwError);
    }

    for (; iEntry < dwNumGroupsFound; iEntry++)
    {
        PDIRECTORY_ENTRY pEntry = &pDirectoryEntries[iEntry];

        dwError = LocalMarshalEntryToGroupInfo_0(
                        pEntry,
                        NULL,
                        &ppGroupInfoList[iEntry]);
        BAIL_ON_LSA_ERROR(dwError);
    }

    *pppGroupInfoList = (PVOID*)ppGroupInfoList;
    *pdwNumGroupsFound = dwNumGroupsFound;

cleanup:

    if (pDirectoryEntries)
    {
        DirectoryFreeEntries(pDirectoryEntries, dwNumGroupsFound);
    }

    return dwError;

error:

    *pppGroupInfoList = NULL;
    *pdwNumGroupsFound = 0;

    if (ppGroupInfoList)
    {
        LsaFreeGroupInfoList(
                dwGroupInfoLevel,
                (PVOID*)ppGroupInfoList,
                dwNumGroupsFound);
    }

    goto cleanup;
}

DWORD
LocalDirGetGroupsForUser_1(
    HANDLE  hProvider,
    PWSTR   pwszUserDN,
    PDWORD  pdwNumGroupsFound,
    PVOID** pppGroupInfoList
    )
{
    DWORD dwError = 0;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;
    wchar16_t wszAttrNameGID[]            = LOCAL_DIR_ATTR_GID;
    wchar16_t wszAttrNameSamAccountName[] = LOCAL_DIR_ATTR_SAM_ACCOUNT_NAME;
    wchar16_t wszAttrNameDN[]             = LOCAL_DIR_ATTR_DISTINGUISHED_NAME;
    wchar16_t wszAttrNameObjectSID[]      = LOCAL_DIR_ATTR_OBJECT_SID;
    wchar16_t wszAttrNameNetBIOSDomain[]      = LOCAL_DIR_ATTR_NETBIOS_NAME;
    PWSTR     pwszAttrs[] =
    {
        &wszAttrNameGID[0],
        &wszAttrNameSamAccountName[0],
        &wszAttrNameDN[0],
        &wszAttrNameObjectSID[0],
        &wszAttrNameNetBIOSDomain[0],
        NULL
    };
    PDIRECTORY_ENTRY   pDirectoryEntries = NULL;
    DWORD              iEntry = 0;
    DWORD              dwGroupInfoLevel = 1;
    PWSTR              pwszGroupDN = NULL;
    PLSA_GROUP_INFO_1* ppGroupInfoList = NULL;
    DWORD              dwNumGroupsFound = 0;

    dwError = DirectoryGetMemberships(
                    pContext->hDirectory,
                    pwszUserDN,
                    pwszAttrs,
                    &pDirectoryEntries,
                    &dwNumGroupsFound);
    BAIL_ON_LSA_ERROR(dwError);

    if (dwNumGroupsFound)
    {
        dwError = LwAllocateMemory(
                        sizeof(PLSA_GROUP_INFO_1) * dwNumGroupsFound,
                        (PVOID*)&ppGroupInfoList);
        BAIL_ON_LSA_ERROR(dwError);
    }

    for (; iEntry < dwNumGroupsFound; iEntry++)
    {
        PDIRECTORY_ENTRY pEntry = &pDirectoryEntries[iEntry];

        LW_SAFE_FREE_MEMORY(pwszGroupDN);

        dwError = LocalMarshalEntryToGroupInfo_1(
                        pEntry,
                        &pwszGroupDN,
                        &ppGroupInfoList[iEntry]);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LocalDirGetGroupMemberNames(
                        pContext,
                        pwszGroupDN,
                        &ppGroupInfoList[iEntry]->ppszMembers);
        BAIL_ON_LSA_ERROR(dwError);
    }

    *pppGroupInfoList = (PVOID*)ppGroupInfoList;
    *pdwNumGroupsFound = dwNumGroupsFound;

cleanup:

    if (pDirectoryEntries)
    {
        DirectoryFreeEntries(pDirectoryEntries, dwNumGroupsFound);
    }

    LW_SAFE_FREE_MEMORY(pwszGroupDN);

    return dwError;

error:

    *pppGroupInfoList = NULL;
    *pdwNumGroupsFound = 0;

    if (ppGroupInfoList)
    {
        LsaFreeGroupInfoList(
                dwGroupInfoLevel,
                (PVOID*)ppGroupInfoList,
                dwNumGroupsFound);
    }

    goto cleanup;
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

            dwError = LW_ERROR_UNSUPPORTED_USER_LEVEL;

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
    wchar16_t wszAttrNameNetBIOSDomain[]      = LOCAL_DIR_ATTR_NETBIOS_NAME;
    PWSTR wszAttrs[] =
    {
        &wszAttrNameGID[0],
        &wszAttrNameSamAccountName[0],
        &wszAttrNameObjectSID[0],
        &wszAttrNameNetBIOSDomain[0],
        NULL
    };
    PCSTR pszFilterTemplate =
                    LOCAL_DB_DIR_ATTR_DOMAIN   " IN (\"%s\", \"%s\")" \
                    " AND " LOCAL_DB_DIR_ATTR_OBJECT_CLASS " = %d";
    PSTR pszFilter = NULL;
    PWSTR pwszFilter = NULL;
    PLOCAL_PROVIDER_ENUM_STATE pEnumState = NULL;

    dwError = LocalCreateGroupState(
                        hProvider,
                        dwInfoLevel,
                        &pEnumState);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateStringPrintf(
                    &pszFilter,
                    pszFilterTemplate,
                    gLPGlobals.pszLocalDomain,
                    gLPGlobals.pszBuiltinDomain,
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

    LW_SAFE_FREE_STRING(pszFilter);
    LW_SAFE_FREE_MEMORY(pwszFilter);

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
    DWORD dwInfoLevel = 1;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;
    wchar16_t wszAttrNameGID[]            = LOCAL_DIR_ATTR_GID;
    wchar16_t wszAttrNameSamAccountName[] = LOCAL_DIR_ATTR_SAM_ACCOUNT_NAME;
    wchar16_t wszAttrNameDN[]             = LOCAL_DIR_ATTR_DISTINGUISHED_NAME;
    wchar16_t wszAttrNameObjectSID[]      = LOCAL_DIR_ATTR_OBJECT_SID;
    wchar16_t wszAttrNameNetBIOSDomain[]      = LOCAL_DIR_ATTR_NETBIOS_NAME;
    PWSTR wszAttrs[] =
    {
        &wszAttrNameGID[0],
        &wszAttrNameSamAccountName[0],
        &wszAttrNameDN[0],
        &wszAttrNameObjectSID[0],
        &wszAttrNameNetBIOSDomain[0],
        NULL
    };
    PCSTR pszFilterTemplate =
                    LOCAL_DB_DIR_ATTR_DOMAIN   " IN (\"%s\", \"%s\")" \
                    " AND " LOCAL_DB_DIR_ATTR_OBJECT_CLASS " = %d";
    PSTR pszFilter = NULL;
    PWSTR pwszFilter = NULL;
    PLOCAL_PROVIDER_ENUM_STATE pEnumState = NULL;

    dwError = LocalCreateGroupState(
                        hProvider,
                        dwInfoLevel,
                        &pEnumState);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateStringPrintf(
                    &pszFilter,
                    pszFilterTemplate,
                    gLPGlobals.pszLocalDomain,
                    gLPGlobals.pszBuiltinDomain,
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

    LW_SAFE_FREE_STRING(pszFilter);
    LW_SAFE_FREE_MEMORY(pwszFilter);

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

            dwError = LW_ERROR_UNSUPPORTED_GROUP_LEVEL;

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
        dwError = LW_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwNumGroupsAvailable = pEnumState->dwNumEntries - pEnumState->dwNextStartingId;

    dwNumGroupsFound = LSA_MIN(dwNumMaxGroups, dwNumGroupsAvailable);

    if (!dwNumGroupsFound)
    {
        dwError = LW_ERROR_NO_MORE_GROUPS;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwAllocateMemory(
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

    if (pEnumState->dwInfoLevel != 1)
    {
        dwError = LW_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwNumGroupsAvailable = pEnumState->dwNumEntries - pEnumState->dwNextStartingId;

    dwNumGroupsFound = LSA_MIN(dwNumMaxGroups, dwNumGroupsAvailable);

    if (!dwNumGroupsFound)
    {
        dwError = LW_ERROR_NO_MORE_GROUPS;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwAllocateMemory(
                    dwNumGroupsFound * sizeof(PLSA_GROUP_INFO_1),
                    (PVOID*)&ppGroupInfoList);
    BAIL_ON_LSA_ERROR(dwError);

    for (; iGroup < dwNumGroupsFound; iGroup++)
    {
        PDIRECTORY_ENTRY pEntry = NULL;

        pEntry = &pEnumState->pEntries[pEnumState->dwNextStartingId + iGroup];

        LW_SAFE_FREE_MEMORY(pwszGroupDN);

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

    LW_SAFE_FREE_MEMORY(pwszGroupDN);

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
            LwFreeString(ppszMembers[iMember++]);
        }

        LwFreeMemory(ppszMembers);
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
                    LsaHashCaselessStringHash,
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

    dwError = LwAllocateMemory(
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

    LW_SAFE_FREE_MEMORY(ppGroupMembers);

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
    wchar16_t wszAttrNameNetbiosName[]    = LOCAL_DIR_ATTR_NETBIOS_NAME;
    PWSTR     pwszAttrs[] =
    {
            &wszAttrNameObjectClass[0],
            &wszAttrNameObjectSID[0],
            &wszAttrNameDN[0],
            &wszAttrNameSamAccountName[0],
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
                    LW_SAFE_FREE_MEMORY(pwszChildGroupDN);

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
            case LOCAL_OBJECT_CLASS_GROUP_MEMBER:

                LW_SAFE_FREE_STRING(pszSID);

                dwError = LocalMarshalAttrToANSIFromUnicodeString(
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
                    dwError = LwAllocateMemory(
                                    sizeof(LOCAL_PROVIDER_GROUP_MEMBER),
                                    (PVOID*)&pGroupMember);
                    BAIL_ON_LSA_ERROR(dwError);

                    pGroupMember->pszSID = pszSID;
                    pszSID = NULL;

                    /* netbios and sam account names may not be set if
                       member is a domain group or user */

                    dwError = LocalMarshalAttrToANSIFromUnicodeString(
                                    pEntry,
                                    &wszAttrNameNetbiosName[0],
                                    &pGroupMember->pszNetbiosDomain);
                    if (dwError != 0&&
                        dwError != LW_ERROR_NO_ATTRIBUTE_VALUE) {
                        BAIL_ON_LSA_ERROR(dwError);
                    }

                    dwError = LocalMarshalAttrToANSIFromUnicodeString(
                                    pEntry,
                                    &wszAttrNameSamAccountName[0],
                                    &pGroupMember->pszSamAccountName);
                    if (dwError != 0&&
                        dwError != LW_ERROR_NO_ATTRIBUTE_VALUE) {
                        BAIL_ON_LSA_ERROR(dwError);
                    }

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

                dwError = LW_ERROR_INTERNAL;
                BAIL_ON_LSA_ERROR(dwError);

                break;
        }
    }

cleanup:

    LW_SAFE_FREE_MEMORY(pwszChildGroupDN);
    LW_SAFE_FREE_MEMORY(pszSID);

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

            dwError = LW_ERROR_UNSUPPORTED_GROUP_LEVEL;

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
        LOCAL_DAG0_IDX_COMMON_NAME,
        LOCAL_DAG0_IDX_OBJECTCLASS,
        LOCAL_DAG0_IDX_DOMAIN,
        LOCAL_DAG0_IDX_NETBIOS_DOMAIN,
        LOCAL_DAG0_IDX_GID,
        LOCAL_DAG0_IDX_SENTINEL
    };
    ATTRIBUTE_VALUE attrValues[] =
    {
        {       /* LOCAL_DIR_ADD_USER_0_IDX_SAM_ACCOUNT_NAME */
                .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                .data.pwszStringValue = NULL
        },
        {       /* LOCAL_DIR_ADD_USER_0_IDX_COMMON_NAME */
                .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                .data.pwszStringValue = NULL
        },
        {       /* LOCAL_DIR_ADD_USER_0_IDX_OBJECTCLASS */
                .Type = DIRECTORY_ATTR_TYPE_INTEGER,
                .data.ulValue = LOCAL_OBJECT_CLASS_GROUP
        },
        {       /* LOCAL_DIR_ADD_USER_0_IDX_DOMAIN */
                .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                .data.pwszStringValue = NULL
        },
        {       /* LOCAL_DIR_ADD_USER_0_IDX_NETBIOS_DOMAIN */
                .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                .data.pwszStringValue = NULL
        },
        {       /* LOCAL_DIR_ADD_USER_0_IDX_GID */
                .Type = DIRECTORY_ATTR_TYPE_INTEGER,
                .data.ulValue = pGroupInfo->gid
        }
    };
    WCHAR wszAttrObjectClass[]        = LOCAL_DIR_ATTR_OBJECT_CLASS;
    WCHAR wszAttrSamAccountName[]     = LOCAL_DIR_ATTR_SAM_ACCOUNT_NAME;
    WCHAR wszAttrCommonName[]         = LOCAL_DIR_ATTR_COMMON_NAME;
    WCHAR wszAttrDomain[]             = LOCAL_DIR_ATTR_DOMAIN;
    WCHAR wszAttrNameNetBIOSDomain[]  = LOCAL_DIR_ATTR_NETBIOS_NAME;
    WCHAR wszAttrNameGID[]            = LOCAL_DIR_ATTR_GID;
    DIRECTORY_MOD modObjectClass =
    {
        DIR_MOD_FLAGS_ADD,
        &wszAttrObjectClass[0],
        1,
        &attrValues[LOCAL_DAG0_IDX_OBJECTCLASS]
    };
    DIRECTORY_MOD modSamAccountName =
    {
        DIR_MOD_FLAGS_ADD,
        &wszAttrSamAccountName[0],
        1,
        &attrValues[LOCAL_DAG0_IDX_SAM_ACCOUNT_NAME]
    };
    DIRECTORY_MOD modCommonName =
    {
        DIR_MOD_FLAGS_ADD,
        &wszAttrCommonName[0],
        1,
        &attrValues[LOCAL_DAG0_IDX_COMMON_NAME]
    };
    DIRECTORY_MOD modDomain =
    {
        DIR_MOD_FLAGS_ADD,
        &wszAttrDomain[0],
        1,
        &attrValues[LOCAL_DAG0_IDX_DOMAIN]
    };
    DIRECTORY_MOD modNetBIOSDomain =
    {
        DIR_MOD_FLAGS_ADD,
        &wszAttrNameNetBIOSDomain[0],
        1,
        &attrValues[LOCAL_DAG0_IDX_NETBIOS_DOMAIN]
    };
    DIRECTORY_MOD modGID =
    {
        DIR_MOD_FLAGS_ADD,
        &wszAttrNameGID[0],
        1,
        &attrValues[LOCAL_DAG0_IDX_GID]
    };
    DIRECTORY_MOD mods[LOCAL_DAG0_IDX_SENTINEL + 1];
    DWORD iMod = 0;
    PWSTR pwszSamAccountName = NULL;
    PWSTR pwszDomain = NULL;
    PWSTR pwszNetBIOSDomain = NULL;

    memset(&mods[0], 0, sizeof(mods));

    BAIL_ON_INVALID_STRING(pGroupInfo->pszName);

    if (pGroupInfo->gid) {
        dwError = LocalDirValidateGID(pGroupInfo->gid);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LocalCrackDomainQualifiedName(
                    pGroupInfo->pszName,
                    &pLoginInfo);
    BAIL_ON_LSA_ERROR(dwError);

    if (!LocalServicesDomain(pLoginInfo->pszDomainNetBiosName))
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaMbsToWc16s(
                    pLoginInfo->pszFullDomainName,
                    &pwszDomain);
    BAIL_ON_LSA_ERROR(dwError);

    attrValues[LOCAL_DAG0_IDX_DOMAIN].data.pwszStringValue = pwszDomain;

    dwError = LsaMbsToWc16s(
                    pLoginInfo->pszDomainNetBiosName,
                    &pwszNetBIOSDomain);
    BAIL_ON_LSA_ERROR(dwError);

    attrValues[LOCAL_DAG0_IDX_NETBIOS_DOMAIN].data.pwszStringValue = pwszNetBIOSDomain;

    dwError = LsaMbsToWc16s(
                    pGroupInfo->pszName,
                    &pwszSamAccountName);
    BAIL_ON_LSA_ERROR(dwError);

    attrValues[LOCAL_DAG0_IDX_SAM_ACCOUNT_NAME].data.pwszStringValue = pwszSamAccountName;
    attrValues[LOCAL_DAG0_IDX_COMMON_NAME].data.pwszStringValue = pwszSamAccountName;

    dwError = LocalBuildDN(
                    pLoginInfo,
                    &pwszGroupDN);
    BAIL_ON_LSA_ERROR(dwError);

    mods[iMod++] = modObjectClass;
    if (pGroupInfo->gid)
    {
        mods[iMod++] = modGID;
    }
    mods[iMod++] = modSamAccountName;
    mods[iMod++] = modCommonName;
    mods[iMod++] = modDomain;
    mods[iMod++] = modNetBIOSDomain;

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

    LW_SAFE_FREE_MEMORY(pwszGroupDN);
    LW_SAFE_FREE_MEMORY(pwszSamAccountName);
    LW_SAFE_FREE_MEMORY(pwszDomain);
    LW_SAFE_FREE_MEMORY(pwszNetBIOSDomain);

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
        LOCAL_DAG1_IDX_SAM_ACCOUNT_NAME = 0,
        LOCAL_DAG1_IDX_COMMON_NAME,
        LOCAL_DAG1_IDX_OBJECTCLASS,
        LOCAL_DAG1_IDX_DOMAIN,
        LOCAL_DAG1_IDX_NETBIOS_DOMAIN,
        LOCAL_DAG1_IDX_GID,
        LOCAL_DAG1_IDX_SENTINEL
    };
    ATTRIBUTE_VALUE attrValues[] =
    {
        {       /* LOCAL_DIR_ADD_USER_0_IDX_SAM_ACCOUNT_NAME */
                .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                .data.pwszStringValue = NULL
        },
        {       /* LOCAL_DIR_ADD_USER_0_IDX_COMMON_NAME */
                .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                .data.pwszStringValue = NULL
        },
        {       /* LOCAL_DIR_ADD_USER_0_IDX_OBJECTCLASS */
                .Type = DIRECTORY_ATTR_TYPE_INTEGER,
                .data.ulValue = LOCAL_OBJECT_CLASS_GROUP
        },
        {       /* LOCAL_DIR_ADD_USER_0_IDX_DOMAIN */
                .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                .data.pwszStringValue = NULL
        },
        {       /* LOCAL_DIR_ADD_USER_0_IDX_NETBIOS_DOMAIN */
                .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                .data.pwszStringValue = NULL
        },
        {       /* LOCAL_DIR_ADD_USER_0_IDX_GID */
                .Type = DIRECTORY_ATTR_TYPE_INTEGER,
                .data.ulValue = pGroupInfo->gid
        }
    };
    WCHAR wszAttrObjectClass[]    = LOCAL_DIR_ATTR_OBJECT_CLASS;
    WCHAR wszAttrSamAccountName[] = LOCAL_DIR_ATTR_SAM_ACCOUNT_NAME;
    WCHAR wszAttrCommonName[]     = LOCAL_DIR_ATTR_COMMON_NAME;
    WCHAR wszAttrDomain[]         = LOCAL_DIR_ATTR_DOMAIN;
    WCHAR wszAttrNameNetBIOSDomain[]  = LOCAL_DIR_ATTR_NETBIOS_NAME;
    WCHAR wszAttrNameGID[] = LOCAL_DIR_ATTR_GID;
    DIRECTORY_MOD modObjectClass =
    {
        DIR_MOD_FLAGS_ADD,
        &wszAttrObjectClass[0],
        1,
        &attrValues[LOCAL_DAG1_IDX_OBJECTCLASS]
    };
    DIRECTORY_MOD modSamAccountName =
    {
        DIR_MOD_FLAGS_ADD,
        &wszAttrSamAccountName[0],
        1,
        &attrValues[LOCAL_DAG1_IDX_SAM_ACCOUNT_NAME]
    };
    DIRECTORY_MOD modCommonName =
    {
        DIR_MOD_FLAGS_ADD,
        &wszAttrCommonName[0],
        1,
        &attrValues[LOCAL_DAG1_IDX_COMMON_NAME]
    };
    DIRECTORY_MOD modDomain =
    {
        DIR_MOD_FLAGS_ADD,
        &wszAttrDomain[0],
        1,
        &attrValues[LOCAL_DAG1_IDX_DOMAIN]
    };
    DIRECTORY_MOD modNetBIOSDomain =
    {
        DIR_MOD_FLAGS_ADD,
        &wszAttrNameNetBIOSDomain[0],
        1,
        &attrValues[LOCAL_DAG1_IDX_NETBIOS_DOMAIN]
    };
    DIRECTORY_MOD modGID =
    {
        DIR_MOD_FLAGS_ADD,
        &wszAttrNameGID[0],
        1,
        &attrValues[LOCAL_DAG1_IDX_GID]
    };
    DIRECTORY_MOD mods[LOCAL_DAG1_IDX_SENTINEL + 1];
    DWORD iMod = 0;
    PWSTR pwszSamAccountName = NULL;
    PWSTR pwszDomain = NULL;
    PWSTR pwszNetBIOSDomain = NULL;

    memset(&mods[0], 0, sizeof(mods));

    BAIL_ON_INVALID_STRING(pGroupInfo->pszName);

    if (pGroupInfo->gid) {
        dwError = LocalDirValidateGID(pGroupInfo->gid);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LocalCrackDomainQualifiedName(
                    pGroupInfo->pszName,
                    &pLoginInfo);
    BAIL_ON_LSA_ERROR(dwError);

    if (!LocalServicesDomain(pLoginInfo->pszDomainNetBiosName))
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaMbsToWc16s(
                    pLoginInfo->pszFullDomainName,
                    &pwszDomain);
    BAIL_ON_LSA_ERROR(dwError);

    attrValues[LOCAL_DAG1_IDX_DOMAIN].data.pwszStringValue = pwszDomain;

    dwError = LsaMbsToWc16s(
                    pLoginInfo->pszDomainNetBiosName,
                    &pwszNetBIOSDomain);
    BAIL_ON_LSA_ERROR(dwError);

    attrValues[LOCAL_DAG1_IDX_NETBIOS_DOMAIN].data.pwszStringValue = pwszNetBIOSDomain;

    dwError = LsaMbsToWc16s(
                    pGroupInfo->pszName,
                    &pwszSamAccountName);
    BAIL_ON_LSA_ERROR(dwError);

    attrValues[LOCAL_DAG1_IDX_SAM_ACCOUNT_NAME].data.pwszStringValue = pwszSamAccountName;
    attrValues[LOCAL_DAG1_IDX_COMMON_NAME].data.pwszStringValue = pwszSamAccountName;

    dwError = LocalBuildDN(
                    pLoginInfo,
                    &pwszGroupDN);
    BAIL_ON_LSA_ERROR(dwError);

    mods[iMod++] = modObjectClass;
    if (pGroupInfo->gid)
    {
        mods[iMod++] = modGID;
    }
    mods[iMod++] = modSamAccountName;
    mods[iMod++] = modCommonName;
    mods[iMod++] = modDomain;
    mods[iMod++] = modNetBIOSDomain;

    dwError = DirectoryAddObject(
                    pContext->hDirectory,
                    pwszGroupDN,
                    mods);
    BAIL_ON_LSA_ERROR(dwError);

    if (!LW_IS_NULL_OR_EMPTY_STR(pGroupInfo->ppszMembers))
    {
        dwError = LocalAddMembersToGroup(
                        pContext,
                        pwszGroupDN,
                        pGroupInfo->ppszMembers);
        BAIL_ON_LSA_ERROR(dwError);
    }

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

    LW_SAFE_FREE_MEMORY(pwszGroupDN);
    LW_SAFE_FREE_MEMORY(pwszSamAccountName);
    LW_SAFE_FREE_MEMORY(pwszDomain);
    LW_SAFE_FREE_MEMORY(pwszNetBIOSDomain);

    return dwError;

error:

    goto cleanup;
}

static
DWORD
LocalDirValidateGID(
    gid_t gid
    )
{
    DWORD dwError = 0;

    /* Check whether group gid is within permitted range */
    if (gid < LOWEST_GID) {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

DWORD
LocalDirModifyGroup(
    HANDLE hProvider,
    PLSA_GROUP_MOD_INFO pGroupModInfo
    )
{
    DWORD dwError = 0;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;
    DWORD dwGroupInfoLevel = 0;
    PWSTR pwszGroupDN = NULL;
    DWORD i = 0;
    PSTR pszSID = NULL;
    PSTR pszDN = NULL;
    PWSTR pwszSID = NULL;
    PWSTR pwszDN = NULL;
    DWORD dwObjectClassGroupMember = LOCAL_OBJECT_CLASS_GROUP_MEMBER;
    DWORD dwObjectClassLocalUser = LOCAL_OBJECT_CLASS_USER;
    PLSA_GROUP_INFO_0 pGroupInfo = NULL;
    PWSTR pwszBase = NULL;
    ULONG ulScope = 0;
    wchar_t wszFilterFmt[] = L"(%ws=%d OR %ws=%d) AND %ws=\'%ws\' AND %ws=\'%ws\'";
    wchar_t wszFilterFmtSidOnly[] = L"(%ws=%d OR %ws=%d) AND %ws=\'%ws\'";
    PWSTR pwszFilter = NULL;
    DWORD dwFilterLen = 0;
    WCHAR wszAttrObjectClass[] = LOCAL_DIR_ATTR_OBJECT_CLASS;
    WCHAR wszAttrDistinguishedName[] = LOCAL_DIR_ATTR_DISTINGUISHED_NAME;
    WCHAR wszAttrObjectSid[] = LOCAL_DIR_ATTR_OBJECT_SID;
    WCHAR wszAttrSamAccountName[] = LOCAL_DIR_ATTR_SAM_ACCOUNT_NAME;
    PWSTR wszAttributes[] = {
        wszAttrObjectClass,
        wszAttrObjectSid,
        wszAttrDistinguishedName,
        wszAttrSamAccountName,
        NULL
    };

    PDIRECTORY_ENTRY pMember = NULL;
    DWORD dwNumEntries = 0;

    enum AttrValueIndex
    {
        GRP_MEMBER_IDX_OBJECTCLASS,
        GRP_MEMBER_IDX_DN,
        GRP_MEMBER_IDX_SID
    };

    ATTRIBUTE_VALUE attrValues[] = {
        {
            .Type = DIRECTORY_ATTR_TYPE_INTEGER,
            .data.ulValue = LOCAL_OBJECT_CLASS_GROUP_MEMBER
        },
        {
            .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            .data.pwszStringValue = NULL
        },
        {
            .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            .data.pwszStringValue = NULL
        }
    };

    DIRECTORY_MOD modObjectClass = {
        DIR_MOD_FLAGS_ADD,
        wszAttrObjectClass,
        1,
        &attrValues[GRP_MEMBER_IDX_OBJECTCLASS]
    };

    DIRECTORY_MOD modDistinguishedName = {
        DIR_MOD_FLAGS_ADD,
        wszAttrDistinguishedName,
        1,
        &attrValues[GRP_MEMBER_IDX_DN]
    };

    DIRECTORY_MOD modObjectSID = {
        DIR_MOD_FLAGS_ADD,
        wszAttrObjectSid,
        1,
        &attrValues[GRP_MEMBER_IDX_SID]
    };

    DIRECTORY_MOD MemberMods[] = {
        modObjectClass,
        modDistinguishedName,
        modObjectSID,
        { 0, NULL, 0, NULL }
    };

    dwError = LocalDirFindGroupById(
                    hProvider,
                    pGroupModInfo->gid,
                    dwGroupInfoLevel,
                    &pwszGroupDN,
                    (PVOID*)&pGroupInfo);
    BAIL_ON_LSA_ERROR(dwError);

    if (pGroupModInfo->actions.bAddMembers)
    {
        for (i = 0; i < pGroupModInfo->dwAddMembersNum; i++)
        {
            pszSID = pGroupModInfo->pAddMembers[i].pszSid;
            pszDN  = pGroupModInfo->pAddMembers[i].pszDN;

            dwError = LsaMbsToWc16s(pszSID, &pwszSID);
            BAIL_ON_LSA_ERROR(dwError);

            if (pszDN)
            {
                dwError = LsaMbsToWc16s(pszDN, &pwszDN);
                BAIL_ON_LSA_ERROR(dwError);
            }

            dwFilterLen = (sizeof(wszAttrObjectClass) - 2) +
                           10 +
                          (sizeof(wszAttrObjectClass) - 2) +
                           10 +
                          (sizeof(wszAttrObjectSid) - 2) +
                          (wc16slen(pwszSID) * sizeof(WCHAR));
            if (pwszDN)
            {
                dwFilterLen += (sizeof(wszAttrDistinguishedName) - 2) +
                               (wc16slen(pwszDN) * sizeof(WCHAR)) +
                               sizeof(wszFilterFmt);
            }
            else
            {
                dwFilterLen += sizeof(wszFilterFmtSidOnly);
            }


            dwError = LwAllocateMemory(
                        dwFilterLen,
                        (PVOID*)&pwszFilter);
            BAIL_ON_LSA_ERROR(dwError);

            if (pwszDN)
            {
                sw16printfw(pwszFilter, dwFilterLen/sizeof(WCHAR), wszFilterFmt,
                            wszAttrObjectClass, dwObjectClassGroupMember,
                            wszAttrObjectClass, dwObjectClassLocalUser,
                            wszAttrObjectSid, pwszSID,
                            wszAttrDistinguishedName, pwszDN);
            }
            else
            {
                sw16printfw(pwszFilter, dwFilterLen/sizeof(WCHAR),
                            wszFilterFmtSidOnly,
                            wszAttrObjectClass, dwObjectClassGroupMember,
                            wszAttrObjectClass, dwObjectClassLocalUser,
                            wszAttrObjectSid, pwszSID);
            }

            dwError = DirectorySearch(
                        pContext->hDirectory,
                        pwszBase,
                        ulScope,
                        pwszFilter,
                        wszAttributes,
                        0,
                        &pMember,
                        &dwNumEntries);
            BAIL_ON_LSA_ERROR(dwError);

            if (dwNumEntries == 0) {
                dwError = LocalDirCreateForeignPrincipalDN(hProvider,
                                                           pwszSID,
                                                           &pwszDN);
                BAIL_ON_LSA_ERROR(dwError);

                MemberMods[GRP_MEMBER_IDX_DN].pAttrValues[0].data.pwszStringValue = pwszDN;
                MemberMods[GRP_MEMBER_IDX_SID].pAttrValues[0].data.pwszStringValue = pwszSID;

                dwError = DirectoryAddObject(
                            pContext->hDirectory,
                            pwszDN,
                            MemberMods);
                BAIL_ON_LSA_ERROR(dwError);

                dwError = DirectorySearch(
                            pContext->hDirectory,
                            pwszBase,
                            ulScope,
                            pwszFilter,
                            wszAttributes,
                            0,
                            &pMember,
                            &dwNumEntries);
                BAIL_ON_LSA_ERROR(dwError);
            }

            dwError = DirectoryAddToGroup(
                        pContext->hDirectory,
                        pwszGroupDN,
                        pMember);
            BAIL_ON_LSA_ERROR(dwError);

            if (pwszDN)
            {
                LW_SAFE_FREE_MEMORY(pwszDN);
                pwszDN = NULL;
            }

            if (pwszSID)
            {
                LW_SAFE_FREE_MEMORY(pwszSID);
                pwszDN = NULL;
            }

            if (pwszFilter)
            {
                LW_SAFE_FREE_MEMORY(pwszFilter);
                pwszFilter = NULL;
            }

            if (pMember)
            {
                DirectoryFreeEntries(pMember, dwNumEntries);
                pMember = NULL;
            }
        }

    } else if (pGroupModInfo->actions.bRemoveMembers)
    {
        for (i = 0; i < pGroupModInfo->dwRemoveMembersNum; i++)
        {
            pszSID = pGroupModInfo->pRemoveMembers[i].pszSid;
            pszDN  = pGroupModInfo->pRemoveMembers[i].pszDN;

            dwError = LsaMbsToWc16s(pszSID, &pwszSID);
            BAIL_ON_LSA_ERROR(dwError);

            if (pszDN) {
                dwError = LsaMbsToWc16s(pszDN, &pwszDN);
                BAIL_ON_LSA_ERROR(dwError);
            }

            dwFilterLen = (sizeof(wszAttrObjectClass) - 1) +
                           10 +
                          (sizeof(wszAttrObjectClass) - 2) +
                           10 +
                          (sizeof(wszAttrObjectSid) - 1) +
                          (strlen(pszSID) * sizeof(WCHAR));
            if (pwszDN) {
                dwFilterLen += (sizeof(wszAttrDistinguishedName) - 2) +
                               (strlen(pszDN) * sizeof(WCHAR)) +
                               sizeof(wszFilterFmt);
            } else {
                dwFilterLen += sizeof(wszFilterFmtSidOnly);
            }

            dwError = LwAllocateMemory(
                        dwFilterLen,
                        (PVOID*)&pwszFilter);
            BAIL_ON_LSA_ERROR(dwError);

            if (pwszDN) {
                sw16printfw(pwszFilter, dwFilterLen/sizeof(WCHAR), wszFilterFmt,
                            wszAttrObjectClass, dwObjectClassGroupMember,
                            wszAttrObjectClass, dwObjectClassLocalUser,
                            wszAttrObjectSid, pwszSID,
                            wszAttrDistinguishedName, pwszDN);

            } else {
                sw16printfw(pwszFilter, dwFilterLen/sizeof(WCHAR),
                            wszFilterFmtSidOnly,
                            wszAttrObjectClass, dwObjectClassGroupMember,
                            wszAttrObjectClass, dwObjectClassLocalUser,
                            wszAttrObjectSid, pwszSID);
            }

            dwError = DirectorySearch(
                        pContext->hDirectory,
                        pwszBase,
                        ulScope,
                        pwszFilter,
                        wszAttributes,
                        0,
                        &pMember,
                        &dwNumEntries);
            BAIL_ON_LSA_ERROR(dwError);

            if (dwNumEntries == 0) {
                dwError = ERROR_MEMBER_NOT_IN_GROUP;
                BAIL_ON_LSA_ERROR(dwError);

            } else if (dwNumEntries > 1) {
                dwError = LW_ERROR_SAM_DATABASE_ERROR;
                BAIL_ON_LSA_ERROR(dwError);
            }

            dwError = DirectoryRemoveFromGroup(
                        pContext->hDirectory,
                        pwszGroupDN,
                        pMember);
            BAIL_ON_LSA_ERROR(dwError);

            if (pwszDN)
            {
                LW_SAFE_FREE_MEMORY(pwszDN);
                pwszDN = NULL;
            }

            if (pwszSID)
            {
                LW_SAFE_FREE_MEMORY(pwszSID);
                pwszDN = NULL;
            }

            if (pwszFilter)
            {
                LW_SAFE_FREE_MEMORY(pwszFilter);
                pwszFilter = NULL;
            }

            if (pMember)
            {
                DirectoryFreeEntries(pMember, dwNumEntries);
                pMember = NULL;
            }
        }
    }

cleanup:
    if (pGroupInfo)
    {
        LsaFreeGroupInfo(dwGroupInfoLevel, pGroupInfo);
    }

    if (pwszFilter)
    {
        LW_SAFE_FREE_MEMORY(pwszFilter);
    }

    if (pwszDN)
    {
        LW_SAFE_FREE_MEMORY(pwszDN);
    }

    if (pwszSID)
    {
        LW_SAFE_FREE_MEMORY(pwszSID);
    }

    LW_SAFE_FREE_MEMORY(pwszGroupDN);

    if (pMember)
    {
        DirectoryFreeEntries(pMember, dwNumEntries);
    }

    return dwError;

error:
    goto cleanup;
}

static
DWORD
LocalAddMembersToGroup(
    PLOCAL_PROVIDER_CONTEXT pContext,
    PWSTR                   pwszGroupDN,
    PSTR*                   ppszMembers
    )
{
    DWORD dwError = 0;
    DWORD dwUserInfoLevel  = 0;
    DWORD dwGroupInfoLevel = 0;
    PLSA_LOGIN_NAME_INFO pLoginInfo = NULL;
    PLSA_USER_INFO_0  pUserInfo = NULL;
    PLSA_GROUP_INFO_0 pGroupInfo = NULL;
    PWSTR pwszObjectDN = NULL;
    DWORD dwObjectClass = LOCAL_OBJECT_CLASS_UNKNOWN;
    DWORD dwMember = 0;

    while (!LW_IS_NULL_OR_EMPTY_STR(ppszMembers[dwMember]))
    {
        PCSTR pszMemberId = ppszMembers[dwMember];

        if (pUserInfo)
        {
            LsaFreeUserInfo(dwUserInfoLevel, pUserInfo);
            pUserInfo = NULL;
        }

        if (pGroupInfo)
        {
            LsaFreeGroupInfo(dwGroupInfoLevel, pGroupInfo);
            pGroupInfo = NULL;
        }

        if (pLoginInfo)
        {
            LsaFreeNameInfo(pLoginInfo);
            pLoginInfo = NULL;
        }

        dwError = LocalCrackDomainQualifiedName(
                      pszMemberId,
                      &pLoginInfo);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LocalFindObjectByName(
                      pContext,
                      pLoginInfo->pszName,
                      pLoginInfo->pszFullDomainName,
                      &dwObjectClass,
                      &pwszObjectDN);
        BAIL_ON_LSA_ERROR(dwError);

        if ((dwObjectClass != LOCAL_OBJECT_CLASS_USER) &&
            (dwObjectClass != LOCAL_OBJECT_CLASS_GROUP))
        {
            LSA_LOG_ERROR("Skip adding group member [%s] "
                          "with object class [%d]",
                          LSA_SAFE_LOG_STRING(pszMemberId),
                          dwObjectClass);
        }
        else
        {
#if 0
            dwError = DirectoryAddToGroup(
                            pContext->hDirectory,
                            pwszGroupDN,
                            pwszObjectDN);
            BAIL_ON_LSA_ERROR(dwError);
#endif
        }

        dwMember++;
    }

cleanup:

    if (pUserInfo)
    {
        LsaFreeUserInfo(dwUserInfoLevel, pUserInfo);
    }

    if (pGroupInfo)
    {
        LsaFreeGroupInfo(dwGroupInfoLevel, pGroupInfo);
    }

    if (pLoginInfo)
    {
        LsaFreeNameInfo(pLoginInfo);
    }

    return dwError;

error:

    goto cleanup;
}


static
DWORD
LocalDirCreateForeignPrincipalDN(
    HANDLE     hProvider,
    PWSTR      pwszSID,
    PWSTR     *ppwszDN
    )
{
    DWORD dwError = 0;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;
    PWSTR pwszBase = NULL;
    ULONG ulScope = 0;
    wchar_t wszFilterFmt[] = L"%ws=%d";
    DWORD dwFilterLen = 0;
    PWSTR pwszFilter = NULL;
    WCHAR wszAttrObjectClass[] = LOCAL_DIR_ATTR_OBJECT_CLASS;
    WCHAR wszAttrObjectDomain[] = LOCAL_DIR_ATTR_DOMAIN;
    DWORD dwDomainObjectClass = LOCAL_OBJECT_CLASS_DOMAIN;
    PDIRECTORY_ENTRY pEntries = NULL;
    PDIRECTORY_ENTRY pEntry = NULL;
    DWORD dwNumEntries = 0;
    PWSTR pwszDomainName = NULL;
    wchar_t wszForeignDnFmt[] = L"CN=%ws,"
                                L"CN=ForeignSecurityPrincipals,"
                                L"DC=%ws";
    DWORD dwSidStrLen = 0;
    DWORD dwDomainNameLen = 0;
    DWORD dwForeignDnLen = 0;
    PWSTR pwszDn = NULL;

    PWSTR wszAttributes[] = {
        wszAttrObjectClass,
        wszAttrObjectDomain,
        NULL
    };

    dwFilterLen = ((sizeof(wszAttrObjectClass)/sizeof(WCHAR)) - 1) +
                  10 +
                  (sizeof(wszFilterFmt)/sizeof(wszFilterFmt[0]));

    dwError = LwAllocateMemory(sizeof(WCHAR) * dwFilterLen,
                               OUT_PPVOID(&pwszFilter));
    BAIL_ON_LSA_ERROR(dwError);

    sw16printfw(pwszFilter, dwFilterLen, wszFilterFmt,
                wszAttrObjectClass,
                dwDomainObjectClass);

    dwError = DirectorySearch(pContext->hDirectory,
                              pwszBase,
                              ulScope,
                              pwszFilter,
                              wszAttributes,
                              FALSE,
                              &pEntries,
                              &dwNumEntries);
    BAIL_ON_LSA_ERROR(dwError);

    if (dwNumEntries == 0 ||
        dwNumEntries > 1)
    {
        dwError = LW_ERROR_SAM_DATABASE_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pEntry = &(pEntries[0]);

    dwError = DirectoryGetEntryAttrValueByName(
                              pEntry,
                              wszAttrObjectDomain,
                              DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                              (PVOID)&pwszDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwWc16sLen(pwszSID, &dwSidStrLen);
    BAIL_ON_LSA_ERROR(dwError);

    if (dwSidStrLen == 0)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwWc16sLen(pwszDomainName, &dwDomainNameLen);
    BAIL_ON_LSA_ERROR(dwError);

    dwForeignDnLen = dwSidStrLen +
                     dwDomainNameLen +
                     (sizeof(wszForeignDnFmt)/sizeof(wszForeignDnFmt[0]));

    dwError = LwAllocateMemory(sizeof(WCHAR) * dwForeignDnLen,
                               OUT_PPVOID(&pwszDn));
    BAIL_ON_LSA_ERROR(dwError);

    sw16printfw(pwszDn, dwForeignDnLen, wszForeignDnFmt,
                pwszSID,
                pwszDomainName);

    *ppwszDN = pwszDn;

cleanup:
    if (pEntries)
    {
        DirectoryFreeEntries(pEntries, dwNumEntries);
    }

    LW_SAFE_FREE_MEMORY(pwszFilter);

    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pwszDn);

    *ppwszDN = NULL;

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

    LwFreeMemory(ppMemberList);
}

VOID
LocalDirFreeGroupMember(
    PLOCAL_PROVIDER_GROUP_MEMBER pMember
    )
{
    LW_SAFE_FREE_STRING(pMember->pszNetbiosDomain);
    LW_SAFE_FREE_STRING(pMember->pszSamAccountName);
    LW_SAFE_FREE_STRING(pMember->pszSID);

    LwFreeMemory(pMember);
}


DWORD
LocalDirGetGroupMembershipByProvider(
    IN HANDLE    hProvider,
    IN PCSTR     pszSid,
    IN DWORD     dwGroupInfoLevel,
    OUT PDWORD   pdwGroupsCount,
    OUT PVOID  **pppMembershipInfo
    )
{
    DWORD dwError = 0;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;
    WCHAR wszAttrNameDN[] = LOCAL_DIR_ATTR_DISTINGUISHED_NAME;
    PWSTR wszAttrs[] = {
        &wszAttrNameDN[0],
        NULL
    };
    PDIRECTORY_ENTRY pEntries = NULL;
    PDIRECTORY_ENTRY pEntry = NULL;
    DWORD dwNumEntries = 0;
    PCSTR pszFilterFmt = LOCAL_DB_DIR_ATTR_OBJECT_SID " = \"%s\"" \
                         " AND (" LOCAL_DB_DIR_ATTR_OBJECT_CLASS " = %d " \
                         " OR " LOCAL_DB_DIR_ATTR_OBJECT_CLASS " = %d)";
    PSTR pszFilter = NULL;
    PWSTR pwszFilter = NULL;
    PWSTR pwszDN = NULL;
    DWORD dwGroupsCount = 0;
    PVOID *ppMembershipInfo = NULL;

    dwError = LwAllocateStringPrintf(
                    &pszFilter,
                    pszFilterFmt,
                    pszSid,
                    LOCAL_OBJECT_CLASS_USER,
                    LOCAL_OBJECT_CLASS_GROUP_MEMBER);
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
        dwError = LW_ERROR_NO_SUCH_USER;
    }
    else if (dwNumEntries == 0)
    {
        dwError = LW_ERROR_DATA_ERROR;
    }
    BAIL_ON_LSA_ERROR(dwError);

    pEntry = &(pEntries[0]);

    dwError = DirectoryGetEntryAttrValueByName(
                    pEntry,
                    wszAttrNameDN,
                    DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                    (PVOID)&pwszDN);
    BAIL_ON_LSA_ERROR(dwError);

    if (!pwszDN)
    {
        dwError = LW_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LocalDirGetGroupsForUser(
                    hProvider,
                    pwszDN,
                    dwGroupInfoLevel,
                    &dwGroupsCount,
                    &ppMembershipInfo);
    BAIL_ON_LSA_ERROR(dwError);

    *pdwGroupsCount    = dwGroupsCount;
    *pppMembershipInfo = ppMembershipInfo;

cleanup:
    LW_SAFE_FREE_STRING(pszFilter);
    LW_SAFE_FREE_MEMORY(pwszFilter);

    if (pEntries)
    {
        DirectoryFreeEntries(pEntries, dwNumEntries);
    }

    return dwError;

error:
    *pdwGroupsCount    = 0;
    *pppMembershipInfo = NULL;

    goto cleanup;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
