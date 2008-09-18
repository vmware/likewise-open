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
 *        groups.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 * 
 *        Group Lookup and Management (Server)
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "api.h"

DWORD
LsaSrvFindGroupByName(
    HANDLE hServer,
    PCSTR  pszGroup,
    DWORD  dwGroupInfoLevel,
    PVOID* ppGroupInfo
    )
{
    DWORD dwError = 0;
    PLSA_AUTH_PROVIDER pProvider = NULL;
    BOOLEAN bInLock = FALSE;
    HANDLE hProvider = (HANDLE)NULL;
    
    dwError = LsaValidateGroupName(pszGroup);
    BAIL_ON_LSA_ERROR(dwError);
    
    ENTER_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);
    
    dwError = LSA_ERROR_NOT_HANDLED;
    
    for (pProvider = gpAuthProviderList; pProvider; pProvider = pProvider->pNext)
    {
        dwError = LsaSrvOpenProvider(hServer, pProvider, &hProvider);
        BAIL_ON_LSA_ERROR(dwError);
        
        dwError = pProvider->pFnTable->pfnLookupGroupByName(
                                            hProvider,
                                            pszGroup,
                                            dwGroupInfoLevel,
                                            ppGroupInfo);
        if (!dwError) {
            break;
        } else if ((dwError == LSA_ERROR_NOT_HANDLED) ||
                   (dwError == LSA_ERROR_NO_SUCH_GROUP)) {

            LsaSrvCloseProvider(pProvider, hProvider);
            hProvider = (HANDLE)NULL;

            continue;
        } else {
            BAIL_ON_LSA_ERROR(dwError);
        }
    }
    
cleanup:

    if (hProvider != (HANDLE)NULL) {
        LsaSrvCloseProvider(pProvider, hProvider);
    }

    LEAVE_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);
    
    if (!dwError)
    {
        LsaSrvIncrementMetricValue(LsaMetricSuccessfulGroupLookupsByName);
    }
    else
    {
        LsaSrvIncrementMetricValue(LsaMetricFailedGroupLookupsByName);
    }
    
    return(dwError);

error:

    LSA_LOG_ERROR("Failed to find group by name [%s]", IsNullOrEmptyString(pszGroup) ? "" : pszGroup);

    *ppGroupInfo = NULL;
    
    goto cleanup;
}

DWORD
LsaSrvFindGroupById(
    HANDLE hServer,
    gid_t  gid,
    DWORD  dwGroupInfoLevel,
    PVOID* ppGroupInfo
    )
{
    DWORD dwError = 0;
    PLSA_AUTH_PROVIDER pProvider = NULL;
    HANDLE hProvider = (HANDLE)NULL;
    BOOLEAN bInLock = FALSE;
    
    ENTER_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);
    
    dwError = LSA_ERROR_NOT_HANDLED;
    
    for (pProvider = gpAuthProviderList; pProvider; pProvider = pProvider->pNext)
    {
        dwError = LsaSrvOpenProvider(hServer, pProvider, &hProvider);
        BAIL_ON_LSA_ERROR(dwError);
        
        dwError = pProvider->pFnTable->pfnLookupGroupById(
                                            hProvider,
                                            gid,
                                            dwGroupInfoLevel,
                                            ppGroupInfo);
        if (!dwError) {
            break;
        } else if ((dwError == LSA_ERROR_NOT_HANDLED) ||
                   (dwError == LSA_ERROR_NO_SUCH_GROUP)) {

            LsaSrvCloseProvider(pProvider, hProvider);
            hProvider = (HANDLE)NULL;

            continue;
        } else {
            BAIL_ON_LSA_ERROR(dwError);
        }
    }
    
cleanup:

    if (hProvider != (HANDLE)NULL) {
        LsaSrvCloseProvider(pProvider, hProvider);
    }

    LEAVE_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);
    
    if (!dwError)
    {
        LsaSrvIncrementMetricValue(LsaMetricSuccessfulGroupLookupsById);
    }
    else
    {
        LsaSrvIncrementMetricValue(LsaMetricFailedGroupLookupsById);
    }

    return(dwError);

error:

    LSA_LOG_ERROR("Failed to find group by id [%ld]", (long)gid);

    *ppGroupInfo = NULL;

    goto cleanup;
}

DWORD
LsaSrvGetGroupsForUser(
    HANDLE hServer,
    uid_t   uid,
    DWORD   dwGroupInfoLevel,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
    )
{
    DWORD dwError = 0;
    PLSA_AUTH_PROVIDER pProvider = NULL;
    HANDLE hProvider = (HANDLE)NULL;
    BOOLEAN bInLock = FALSE;

    *pdwGroupsFound = 0;
    *pppGroupInfoList = NULL;
    
    ENTER_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);
    
    dwError = LSA_ERROR_NOT_HANDLED;
    for (pProvider = gpAuthProviderList; pProvider; pProvider = pProvider->pNext)
    {
        dwError = LsaSrvOpenProvider(hServer, pProvider, &hProvider);
        BAIL_ON_LSA_ERROR(dwError);
        
        if (pProvider->pFnTable->pfnGetGroupsForUser == NULL)
            dwError = LSA_ERROR_NOT_HANDLED;
        else
        {
            dwError = pProvider->pFnTable->pfnGetGroupsForUser(
                                                hProvider,
                                                uid,
                                                dwGroupInfoLevel,
                                                pdwGroupsFound,
                                                pppGroupInfoList);
        }
        if (!dwError && *pppGroupInfoList != NULL) {
            break;
        } else if (dwError == LSA_ERROR_NOT_HANDLED) {

            dwError = 0;
        }

        BAIL_ON_LSA_ERROR(dwError);
        LsaSrvCloseProvider(pProvider, hProvider);
        hProvider = (HANDLE)NULL;
    }
    
cleanup:

    if (hProvider != (HANDLE)NULL) {
        LsaSrvCloseProvider(pProvider, hProvider);
    }

    LEAVE_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    return(dwError);

error:

    LSA_LOG_ERROR("Failed to get user groups for user [%ld]", (long)uid);

    *pdwGroupsFound = 0;
    *pppGroupInfoList = NULL;

    goto cleanup;
}

DWORD
LsaSrvAddGroup(
    HANDLE hServer,
    DWORD  dwGroupInfoLevel,
    PVOID  pGroupInfo
    )
{
    DWORD dwError = 0;
    PLSA_GROUP_INFO_1 pGroup = NULL;
    PLSA_AUTH_PROVIDER pProvider = NULL;
    BOOLEAN bInLock = FALSE;
    HANDLE hProvider = (HANDLE)NULL;
    PLSA_SRV_API_STATE pServerState = (PLSA_SRV_API_STATE)hServer;
    
    if (pServerState->peerUID)
    {
        dwError = EACCES;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    if (dwGroupInfoLevel != 1) {
        dwError = LSA_ERROR_UNSUPPORTED_GROUP_LEVEL;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    dwError = LsaValidateGroupInfo(
                    pGroupInfo,
                    dwGroupInfoLevel);
    BAIL_ON_LSA_ERROR(dwError);
    
    pGroup = (PLSA_GROUP_INFO_1)pGroupInfo;
    
    ENTER_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);
    
    dwError = LSA_ERROR_NOT_HANDLED;
    
    for (pProvider = gpAuthProviderList; pProvider; pProvider = pProvider->pNext)
    {
        dwError = LsaSrvOpenProvider(hServer, pProvider, &hProvider);
        BAIL_ON_LSA_ERROR(dwError);
        
        dwError = pProvider->pFnTable->pfnAddGroup(
                                        hProvider,
                                        dwGroupInfoLevel,
                                        pGroupInfo);
        if (!dwError) {
            break;
        } else if (dwError == LSA_ERROR_NOT_HANDLED) {

            LsaSrvCloseProvider(pProvider, hProvider);
            hProvider = (HANDLE)NULL;

            continue;
        } else {
            BAIL_ON_LSA_ERROR(dwError);
        }
    }
    
cleanup:

    if (hProvider != (HANDLE)NULL) {
        LsaSrvCloseProvider(pProvider, hProvider);
    }

    LEAVE_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);
    
    return(dwError);

error:
    
    goto cleanup;
}

DWORD
LsaSrvDeleteGroup(
    HANDLE hServer,
    gid_t  gid
    )
{
    DWORD dwError = 0;
    PLSA_AUTH_PROVIDER pProvider = NULL;
    HANDLE hProvider = (HANDLE)NULL;
    BOOLEAN bInLock = FALSE;
    PLSA_SRV_API_STATE pServerState = (PLSA_SRV_API_STATE)hServer;
    
    if (pServerState->peerUID)
    {
        dwError = EACCES;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    ENTER_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);
    
    dwError = LSA_ERROR_NOT_HANDLED;
    
    for (pProvider = gpAuthProviderList; pProvider; pProvider = pProvider->pNext)
    {
        dwError = LsaSrvOpenProvider(hServer, pProvider, &hProvider);
        BAIL_ON_LSA_ERROR(dwError);
        
        dwError = pProvider->pFnTable->pfnDeleteGroup(hProvider, gid);
        if (!dwError) {
            break;
        } else if ((dwError == LSA_ERROR_NOT_HANDLED) ||
                   (dwError == LSA_ERROR_NO_SUCH_GROUP)) {

            LsaSrvCloseProvider(pProvider, hProvider);
            hProvider = (HANDLE)NULL;

            continue;
        } else {
            BAIL_ON_LSA_ERROR(dwError);
        }
    }
    
cleanup:

    if (hProvider != (HANDLE)NULL) {
        LsaSrvCloseProvider(pProvider, hProvider);
    }

    LEAVE_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);
    
    return(dwError);

error:
    
    goto cleanup;
}

DWORD
LsaSrvBeginEnumGroups(
    HANDLE hServer,
    DWORD  dwGroupInfoLevel,
    DWORD  dwMaxNumGroups,
    PSTR*  ppszGUID
    )
{
    DWORD dwError = 0;
    PLSA_SRV_RECORD_ENUM_STATE pEnumState = NULL;
    PSTR pszGUID = NULL;
    
    dwError = LsaSrvAddGroupEnumState(
                    hServer,
                    dwGroupInfoLevel,
                    dwMaxNumGroups,
                    &pEnumState);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaAllocateString(pEnumState->pszGUID, &pszGUID);
    BAIL_ON_LSA_ERROR(dwError);
    
    *ppszGUID = pszGUID;
    
cleanup:

    return dwError;
    
error:

    *ppszGUID = NULL;

    goto cleanup;
}

DWORD
LsaSrvEnumGroups(
    HANDLE  hServer,
    PCSTR   pszGUID,
    PDWORD  pdwGroupInfoLevel,
    PVOID** pppGroupInfoList,
    PDWORD  pdwNumGroupsFound
    )
{
    DWORD dwError = 0;
    PLSA_SRV_RECORD_ENUM_STATE pEnumState = NULL;
    PVOID* ppGroupInfoList_accumulate = NULL;
    DWORD  dwTotalNumGroupsFound = 0;
    PVOID* ppGroupInfoList = NULL;
    DWORD  dwNumGroupsFound = 0;
    DWORD  dwNumGroupsRemaining = 0;
    DWORD  dwGroupInfoLevel = 0;
    
    pEnumState = LsaSrvFindGroupEnumState(hServer, pszGUID);
    if (!pEnumState) {
        dwError = LSA_ERROR_INTERNAL;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    dwGroupInfoLevel = pEnumState->dwInfoLevel;
    dwNumGroupsRemaining = pEnumState->dwNumMaxRecords;
    
    while (dwNumGroupsRemaining &&
           pEnumState->pCurProviderState)
    {
        PLSA_SRV_PROVIDER_STATE pProviderState = pEnumState->pCurProviderState;
        PLSA_AUTH_PROVIDER pProvider = pProviderState->pProvider;
        HANDLE hProvider = pProviderState->hProvider;
        HANDLE hResume = pProviderState->hResume;

        dwNumGroupsFound = 0;
        
        
        dwError = pProvider->pFnTable->pfnEnumGroups(
                        hProvider,
                        hResume,
                        dwNumGroupsRemaining,
                        &dwNumGroupsFound,
                        &ppGroupInfoList);
                       
        
        if (dwError) {
           if (dwError != LSA_ERROR_NO_MORE_GROUPS) {
              BAIL_ON_LSA_ERROR(dwError);
           }
        }
        
        dwNumGroupsRemaining -= dwNumGroupsFound;        
        
        if (dwNumGroupsRemaining) {
           pEnumState->pCurProviderState = pEnumState->pCurProviderState->pNext;
           if (dwError == LSA_ERROR_NO_MORE_GROUPS){ 
             dwError = 0;           
           }
        }
        
        dwError = LsaCoalesceGroupInfoList(
                        &ppGroupInfoList,
                        &dwNumGroupsFound,
                        &ppGroupInfoList_accumulate,
                        &dwTotalNumGroupsFound);
        BAIL_ON_LSA_ERROR(dwError);
    }
   
    *pdwGroupInfoLevel = dwGroupInfoLevel;
    *pppGroupInfoList = ppGroupInfoList_accumulate;
    *pdwNumGroupsFound = dwTotalNumGroupsFound;
    
cleanup:
    
    return(dwError);

error:
    
    *pdwGroupInfoLevel = 0;
    *pppGroupInfoList = NULL;
    *pdwNumGroupsFound = 0;
    
    
    if (ppGroupInfoList) {
        LsaFreeGroupInfoList(dwGroupInfoLevel, ppGroupInfoList, dwNumGroupsFound);
    }
    
    if (ppGroupInfoList_accumulate) {
        LsaFreeGroupInfoList(dwGroupInfoLevel, ppGroupInfoList_accumulate, dwTotalNumGroupsFound);
    }

    goto cleanup;
}

DWORD
LsaSrvEndEnumGroups(
    HANDLE hServer,
    PSTR   pszGUID
    )
{
    DWORD dwError = 0;
    PLSA_SRV_RECORD_ENUM_STATE pEnumState = NULL;
    PLSA_SRV_PROVIDER_STATE pProviderState = NULL;
    
    pEnumState = LsaSrvFindGroupEnumState(hServer, pszGUID);
    if (!pEnumState) {
        dwError = LSA_ERROR_INTERNAL;
        BAIL_ON_LSA_ERROR(dwError);
    }

    for (pProviderState = pEnumState->pProviderStateList;
         pProviderState;
         pProviderState = pProviderState->pNext)
    {
        PLSA_AUTH_PROVIDER pProvider = pProviderState->pProvider;
        if (pProvider) {
           HANDLE hProvider = pProviderState->hProvider;
           pProvider->pFnTable->pfnEndEnumGroups(
                                       hProvider,
                                       pszGUID);
        }
    }
        
    LsaSrvFreeGroupEnumState(
                        hServer,
                        pszGUID);

cleanup:

    return dwError;
    
error:

    goto cleanup;
}

