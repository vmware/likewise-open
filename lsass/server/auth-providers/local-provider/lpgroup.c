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

DWORD
LocalDirFindGroupByName(
    HANDLE  hProvider,
    PCSTR   pszDomainName,
    PCSTR   pszGroupName,
    DWORD   dwGroupInfoLevel,
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
                                ppGroupInfo
                                );
            break;

        case 1:

            dwError = LocalDirFindGroupByName_1(
                                hProvider,
                                pszDomainName,
                                pszGroupName,
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
    PVOID*  ppGroupInfo
    )
{
    DWORD dwError = 0;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;
    wchar16_t wszAttrNameGID[] = LOCAL_DIR_ATTR_GID;
    wchar16_t wszAttrNameSamAccountName[] = LOCAL_DIR_ATTR_SAM_ACCOUNT_NAME;
    wchar16_t wszAttrNameObjectSID[] = LOCAL_DIR_ATTR_OBJECT_SID;
    PWSTR wszAttrs[] =
    {
        &wszAttrNameGID[0],
        &wszAttrNameSamAccountName[0],
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
                    &pGroupInfo);
    BAIL_ON_LSA_ERROR(dwError);

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

    *ppGroupInfo = pGroupInfo;

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
    PVOID*  ppGroupInfo
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
LocalDirGetGroupsForUser_0(
    HANDLE  hProvider,
    uid_t uid,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
    )
{
    DWORD dwError = 0;

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
    PLOCAL_PROVIDER_ENUM_STATE pEnumState = NULL;

    dwError = LocalCreateGroupState(
                        hProvider,
                        dwInfoLevel,
                        &pEnumState);
    BAIL_ON_LSA_ERROR(dwError);

    // TODO: Query all groups

    *phResume = (HANDLE)pEnumState;

cleanup:

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

    // TODO:

    return dwError;
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

    // TODO:

    return dwError;
}

DWORD
LocalDirFindGroupById(
    HANDLE  hProvider,
    gid_t   gid,
    DWORD   dwGroupInfoLevel,
    PVOID*  ppGroupInfo
    )
{
    DWORD dwError = 0;

    switch(dwGroupInfoLevel)
    {
        case 0:

            dwError = LocalDirFindGroupById_0(hProvider, gid, ppGroupInfo);
            break;

        case 1:

            dwError = LocalDirFindGroupById_1(hProvider, gid, ppGroupInfo);
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
    PVOID* ppGroupInfo
    )
{
    DWORD dwError = 0;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;
    wchar16_t wszAttrNameGID[] = LOCAL_DIR_ATTR_GID;
    wchar16_t wszAttrNameSamAccountName[] = LOCAL_DIR_ATTR_SAM_ACCOUNT_NAME;
    wchar16_t wszAttrNameObjectSID[] = LOCAL_DIR_ATTR_OBJECT_SID;
    PWSTR wszAttrs[] =
    {
        &wszAttrNameGID[0],
        &wszAttrNameSamAccountName[0],
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
                    &pGroupInfo);
    BAIL_ON_LSA_ERROR(dwError);

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

    *ppGroupInfo = pGroupInfo;

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
    PVOID* ppGroupInfo
    )
{
    DWORD dwError = 0;

    return dwError;
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


