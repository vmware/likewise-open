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
LsaCoalesceGroupInfoList(
    PVOID** pppGroupInfoList,
    PDWORD  pdwNumGroupsFound,
    PVOID** pppGroupInfoList_accumulate,
    PDWORD  pdwTotalNumGroupsFound
    )
{
    DWORD dwError = 0;
    PVOID* ppGroupInfoList_current = *pppGroupInfoList_accumulate;
    PVOID* ppGroupInfoList_new = *pppGroupInfoList;
    DWORD dwNumCurGroupsFound = *pdwTotalNumGroupsFound;
    DWORD dwNumNewGroupsFound = *pdwNumGroupsFound;
    DWORD dwNumTotalGroupsFound = 0;
    PVOID* ppGroupInfoList_total = NULL;
    DWORD iGroup = 0;
    DWORD iNewGroup = 0;

    if (!dwNumCurGroupsFound) {

       *pppGroupInfoList_accumulate = ppGroupInfoList_new;
       *pdwTotalNumGroupsFound = dwNumNewGroupsFound;
       *pppGroupInfoList = NULL;
       *pdwNumGroupsFound = 0;
       
       goto cleanup;
    }
    
    dwNumTotalGroupsFound = dwNumCurGroupsFound;
    dwNumTotalGroupsFound += dwNumNewGroupsFound;
        
    dwError = LsaAllocateMemory(
                        sizeof(PVOID) * dwNumTotalGroupsFound,
                        (PVOID*)&ppGroupInfoList_total);
    BAIL_ON_LSA_ERROR(dwError);
        
    for (iGroup = 0; iGroup < dwNumCurGroupsFound; iGroup++) {
        *(ppGroupInfoList_total+iGroup) = *(ppGroupInfoList_current+iGroup);
        *(ppGroupInfoList_current+iGroup) = NULL;
    }
    
    for (iNewGroup = 0; iNewGroup < dwNumNewGroupsFound; iNewGroup++, iGroup++) {
        *(ppGroupInfoList_total+iGroup) = *(ppGroupInfoList_new+iNewGroup);
        *(ppGroupInfoList_new+iNewGroup) = NULL;
    }
    
    LsaFreeMemory(ppGroupInfoList_new);
    
    *pppGroupInfoList_accumulate = ppGroupInfoList_total;
    *pdwTotalNumGroupsFound = dwNumTotalGroupsFound;
    
    *pppGroupInfoList = NULL;
    *pdwNumGroupsFound = 0;
    
cleanup:

    return dwError;
    
error:

    if (ppGroupInfoList_total) {
       LsaFreeMemory(ppGroupInfoList_total);
    }

    goto cleanup;
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

