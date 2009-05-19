/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        groupinfo.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Group Info
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Wei Fu (wfu@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 */
#include "includes.h"

static
void
LsaFreeGroupInfo_0(
    PLSA_GROUP_INFO_0 pGroupInfo
    )
{
    LSA_SAFE_FREE_STRING(pGroupInfo->pszName);
    LSA_SAFE_FREE_STRING(pGroupInfo->pszSid);
    LsaFreeMemory(pGroupInfo);
}

static
void
LsaFreeGroupInfo_1(
    PLSA_GROUP_INFO_1 pGroupInfo
    )
{
    LSA_SAFE_FREE_STRING(pGroupInfo->pszName);
    LSA_SAFE_FREE_STRING(pGroupInfo->pszPasswd);
    LSA_SAFE_FREE_STRING(pGroupInfo->pszSid);
    LSA_SAFE_FREE_STRING_ARRAY(pGroupInfo->ppszMembers);
    LsaFreeMemory(pGroupInfo);
}

void
LsaFreeGroupInfo(
    DWORD  dwLevel,
    PVOID  pGroupInfo
    )
{
    switch(dwLevel)
    {
        case 0:
        {
            LsaFreeGroupInfo_0((PLSA_GROUP_INFO_0)pGroupInfo);
            break;
        }
        case 1:
        {
            LsaFreeGroupInfo_1((PLSA_GROUP_INFO_1)pGroupInfo);
            break;
        }
        default:
        {
            LSA_LOG_ERROR("Unsupported Group Info Level [%d]", dwLevel);
        }
    }
}

DWORD
LsaBuildGroupModInfo(
    gid_t gid,
    PLSA_GROUP_MOD_INFO* ppGroupModInfo
    )
{
    DWORD dwError = 0;
    PLSA_GROUP_MOD_INFO pGroupModInfo = NULL;

    dwError = LsaAllocateMemory(
                    sizeof(LSA_GROUP_MOD_INFO),
                    (PVOID*)&pGroupModInfo);
    BAIL_ON_LSA_ERROR(dwError);

    pGroupModInfo->gid = gid;

    *ppGroupModInfo = pGroupModInfo;

cleanup:
    return dwError;

error:
    if (pGroupModInfo) {
        LsaFreeGroupModInfo(pGroupModInfo);
    }

    *ppGroupModInfo = NULL;
    goto cleanup;
}

DWORD
LsaModifyGroup_AddMembers(
    PLSA_GROUP_MOD_INFO pGroupModInfo,
    PVOID pData
    )
{
    DWORD dwError = 0;
    PCSTR pszDN = NULL;
    PCSTR pszSID = NULL;
    PCSTR *ppszMember = NULL;
    DWORD iMember = 0;

    BAIL_ON_INVALID_POINTER(pGroupModInfo);

    ppszMember = (PCSTR*)pData;
    pGroupModInfo->dwAddMembersNum++;

    dwError = LsaReallocMemory(pGroupModInfo->pAddMembers,
                               (PVOID*)&pGroupModInfo->pAddMembers,
                               sizeof(pGroupModInfo->pAddMembers[0]) *
                               pGroupModInfo->dwAddMembersNum);
    BAIL_ON_LSA_ERROR(dwError);

    if (ppszMember)
    {
        pszDN   = ppszMember[0];
        pszSID  = ppszMember[1];
        iMember = pGroupModInfo->dwAddMembersNum - 1;

        dwError = LsaAllocateString(
                    pszDN,
                    &pGroupModInfo->pAddMembers[iMember].pszDN);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaAllocateString(
                    pszSID,
                    &pGroupModInfo->pAddMembers[iMember].pszSid);
        BAIL_ON_LSA_ERROR(dwError);

        pGroupModInfo->actions.bAddMembers = TRUE;
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

DWORD
LsaModifyGroup_RemoveMembers(
    PLSA_GROUP_MOD_INFO pGroupModInfo,
    PVOID pData
    )
{
    DWORD dwError = 0;
    PCSTR pszDN = NULL;
    PCSTR pszSID = NULL;
    PCSTR *ppszMember = NULL;
    DWORD iMember = 0;

    BAIL_ON_INVALID_POINTER(pGroupModInfo);

    ppszMember = (PCSTR*)pData;
    pGroupModInfo->dwRemoveMembersNum++;

    dwError = LsaReallocMemory(pGroupModInfo->pRemoveMembers,
                               (PVOID*)&pGroupModInfo->pRemoveMembers,
                               sizeof(pGroupModInfo->pRemoveMembers[0]) *
                               pGroupModInfo->dwRemoveMembersNum);
    BAIL_ON_LSA_ERROR(dwError);


    if (ppszMember)
    {
        pszDN   = ppszMember[0];
        pszSID  = ppszMember[1];
        iMember = pGroupModInfo->dwRemoveMembersNum - 1;

        dwError = LsaAllocateString(
                    pszDN,
                    &pGroupModInfo->pRemoveMembers[iMember].pszDN);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaAllocateString(
                    pszSID,
                    &pGroupModInfo->pRemoveMembers[iMember].pszSid);
        BAIL_ON_LSA_ERROR(dwError);

        pGroupModInfo->actions.bRemoveMembers = TRUE;
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

void
LsaFreeGroupModInfo(
    PLSA_GROUP_MOD_INFO pGroupModInfo
    )
{
    DWORD i = 0;

    for (i = 0; i < pGroupModInfo->dwAddMembersNum; i++) {
        LSA_SAFE_FREE_STRING(pGroupModInfo->pAddMembers[i].pszDN);
        LSA_SAFE_FREE_STRING(pGroupModInfo->pAddMembers[i].pszSid);
    }
    LSA_SAFE_FREE_MEMORY(pGroupModInfo->pAddMembers);

    for (i = 0; i < pGroupModInfo->dwRemoveMembersNum; i++) {
        LSA_SAFE_FREE_STRING(pGroupModInfo->pRemoveMembers[i].pszDN);
        LSA_SAFE_FREE_STRING(pGroupModInfo->pRemoveMembers[i].pszSid);
    }
    LSA_SAFE_FREE_MEMORY(pGroupModInfo->pRemoveMembers);

    LsaFreeMemory(pGroupModInfo);
}

void
LsaFreeIpcGroupInfoList(
    PLSA_GROUP_INFO_LIST pGroupIpcInfoList
    )
{
    if (pGroupIpcInfoList)
    {
        switch (pGroupIpcInfoList->dwGroupInfoLevel)
        {
            case 0:
                LsaFreeGroupInfoList(0, (PVOID*)pGroupIpcInfoList->ppGroupInfoList.ppInfoList0, pGroupIpcInfoList->dwNumGroups);
                break;
            case 1:
                LsaFreeGroupInfoList(1, (PVOID*)pGroupIpcInfoList->ppGroupInfoList.ppInfoList1, pGroupIpcInfoList->dwNumGroups);
                break;

            default:
            {
                LSA_LOG_ERROR("Unsupported Group Info Level [%d]", pGroupIpcInfoList->dwGroupInfoLevel);
            }
        }
        LsaFreeMemory(pGroupIpcInfoList);
    }
}

void
LsaFreeGroupInfoList(
    DWORD  dwLevel,
    PVOID* pGroupInfoList,
    DWORD  dwNumGroups
    )
{
    DWORD iGroup = 0;
    for (;iGroup < dwNumGroups; iGroup++) {
        PVOID pGroupInfo = *(pGroupInfoList+iGroup);
        if (pGroupInfo) {
           LsaFreeGroupInfo(dwLevel, pGroupInfo);
        }
    }
    LsaFreeMemory(pGroupInfoList);
}

DWORD
LsaValidateGroupInfoLevel(
    DWORD dwGroupInfoLevel
    )
{
    return (((dwGroupInfoLevel < 0) || (dwGroupInfoLevel > 1)) ? LSA_ERROR_INVALID_GROUP_INFO_LEVEL : 0);
}

DWORD
LsaValidateGroupName(
    PCSTR pszName
    )
{
    DWORD dwError = 0;
    PLSA_LOGIN_NAME_INFO pParsedName = NULL;
    size_t sNameLen = 0;

    dwError = LsaCrackDomainQualifiedName(
                pszName,
                "unset",
                &pParsedName);
    BAIL_ON_LSA_ERROR(dwError);

    if (pParsedName->pszName == NULL)
    {
        dwError = LSA_ERROR_INVALID_GROUP_NAME;
        BAIL_ON_LSA_ERROR(dwError);
    }

    sNameLen = strlen(pParsedName->pszName);
    if (sNameLen > LSA_MAX_GROUP_NAME_LENGTH || sNameLen == 0)
    {
        dwError = LSA_ERROR_INVALID_GROUP_NAME;
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    if (pParsedName != NULL)
    {
        LsaFreeNameInfo(pParsedName);
    }
    return dwError;

error:

    goto cleanup;
}

DWORD
LsaValidateGroupInfo(
    PVOID pGroupInfo,
    DWORD dwGroupInfoLevel
    )
{
    DWORD dwError = 0;

    BAIL_ON_INVALID_POINTER(pGroupInfo);

    dwError = LsaValidateGroupInfoLevel(dwGroupInfoLevel);
    BAIL_ON_LSA_ERROR(dwError);

    switch (dwGroupInfoLevel)
    {
        case 0:
        {
            PLSA_GROUP_INFO_0 pGroupInfo_0 =
                (PLSA_GROUP_INFO_0)pGroupInfo;

            dwError = LsaValidateGroupName(pGroupInfo_0->pszName);
            BAIL_ON_LSA_ERROR(dwError);

            break;
        }

        case 1:
        {
            PLSA_GROUP_INFO_1 pGroupInfo_1 =
                (PLSA_GROUP_INFO_1)pGroupInfo;

            dwError = LsaValidateGroupName(pGroupInfo_1->pszName);
            BAIL_ON_LSA_ERROR(dwError);

            break;
        }

        default:

            dwError = LSA_ERROR_UNSUPPORTED_GROUP_LEVEL;
            BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    return dwError;

error:

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
