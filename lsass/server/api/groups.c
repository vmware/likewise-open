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
    IN HANDLE hServer,
    IN PCSTR pszGroup,
    IN LSA_FIND_FLAGS FindFlags,
    IN DWORD dwGroupInfoLevel,
    OUT PVOID* ppGroupInfo
    )
{
    DWORD dwError = 0;
    DWORD dwTraceFlags[] = {LSA_TRACE_FLAG_USER_GROUP_QUERIES};
    PLSA_AUTH_PROVIDER pProvider = NULL;
    BOOLEAN bInLock = FALSE;
    HANDLE hProvider = (HANDLE)NULL;

    LSA_TRACE_BEGIN_FUNCTION(dwTraceFlags, sizeof(dwTraceFlags)/sizeof(dwTraceFlags[0]));

    dwError = LsaValidateGroupName(pszGroup);
    BAIL_ON_LSA_ERROR(dwError);

    ENTER_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    dwError = LW_ERROR_NOT_HANDLED;

    for (pProvider = gpAuthProviderList; pProvider; pProvider = pProvider->pNext)
    {
        dwError = LsaSrvOpenProvider(hServer, pProvider, &hProvider);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = pProvider->pFnTable->pfnLookupGroupByName(
                                            hProvider,
                                            pszGroup,
                                            FindFlags,
                                            dwGroupInfoLevel,
                                            ppGroupInfo);
        if (!dwError) {
            break;
        } else if ((dwError == LW_ERROR_NOT_HANDLED) ||
                   (dwError == LW_ERROR_NO_SUCH_GROUP)) {

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

    LSA_TRACE_END_FUNCTION(dwTraceFlags, sizeof(dwTraceFlags)/sizeof(dwTraceFlags[0]));

    return(dwError);

error:

    LSA_LOG_ERROR("Failed to find group by name [%s]", IsNullOrEmptyString(pszGroup) ? "" : pszGroup);

    *ppGroupInfo = NULL;

    goto cleanup;
}

DWORD
LsaSrvFindGroupById(
    IN HANDLE hServer,
    IN gid_t gid,
    IN LSA_FIND_FLAGS FindFlags,
    IN DWORD dwGroupInfoLevel,
    OUT PVOID* ppGroupInfo
    )
{
    DWORD dwError = 0;
    DWORD dwTraceFlags[] = {LSA_TRACE_FLAG_USER_GROUP_QUERIES};
    PLSA_AUTH_PROVIDER pProvider = NULL;
    HANDLE hProvider = (HANDLE)NULL;
    BOOLEAN bInLock = FALSE;

    LSA_TRACE_BEGIN_FUNCTION(dwTraceFlags, sizeof(dwTraceFlags)/sizeof(dwTraceFlags[0]));

    ENTER_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    dwError = LW_ERROR_NOT_HANDLED;

    for (pProvider = gpAuthProviderList; pProvider; pProvider = pProvider->pNext)
    {
        dwError = LsaSrvOpenProvider(hServer, pProvider, &hProvider);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = pProvider->pFnTable->pfnLookupGroupById(
                                            hProvider,
                                            gid,
                                            FindFlags,
                                            dwGroupInfoLevel,
                                            ppGroupInfo);
        if (!dwError) {
            break;
        } else if ((dwError == LW_ERROR_NOT_HANDLED) ||
                   (dwError == LW_ERROR_NO_SUCH_GROUP)) {

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

    LSA_TRACE_END_FUNCTION(dwTraceFlags, sizeof(dwTraceFlags)/sizeof(dwTraceFlags[0]));

    return(dwError);

error:

    LSA_LOG_ERROR("Failed to find group by id [%ld]", (long)gid);

    *ppGroupInfo = NULL;

    goto cleanup;
}

DWORD
LsaSrvGetGroupsForUser(
    IN HANDLE hServer,
    IN OPTIONAL PCSTR pszUserName,
    IN OPTIONAL uid_t uid,
    IN LSA_FIND_FLAGS FindFlags,
    IN DWORD dwGroupInfoLevel,
    OUT PDWORD pdwGroupsFound,
    OUT PVOID** pppGroupInfoList
    )
{
    DWORD dwError = 0;
    DWORD dwTraceFlags[] = {LSA_TRACE_FLAG_USER_GROUP_QUERIES};
    PLSA_AUTH_PROVIDER pProvider = NULL;
    HANDLE hProvider = (HANDLE)NULL;
    BOOLEAN bInLock = FALSE;

    *pdwGroupsFound = 0;
    *pppGroupInfoList = NULL;

    LSA_TRACE_BEGIN_FUNCTION(dwTraceFlags, sizeof(dwTraceFlags)/sizeof(dwTraceFlags[0]));

    ENTER_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    dwError = LW_ERROR_NOT_HANDLED;
    for (pProvider = gpAuthProviderList; pProvider; pProvider = pProvider->pNext)
    {
        dwError = LsaSrvOpenProvider(hServer, pProvider, &hProvider);
        BAIL_ON_LSA_ERROR(dwError);

        if (pProvider->pFnTable->pfnGetGroupsForUser == NULL)
        {
            dwError = LW_ERROR_NOT_HANDLED;
        }
        else
        {
            dwError = pProvider->pFnTable->pfnGetGroupsForUser(
                                                hProvider,
                                                pszUserName,
                                                uid,
                                                FindFlags,
                                                dwGroupInfoLevel,
                                                pdwGroupsFound,
                                                pppGroupInfoList);
        }

        if (!dwError && *pppGroupInfoList != NULL)
        {
            break;
        }
        else if ((dwError == LW_ERROR_NOT_HANDLED) ||
                 (dwError == LW_ERROR_NO_SUCH_USER))
        {

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

    LSA_TRACE_END_FUNCTION(dwTraceFlags, sizeof(dwTraceFlags)/sizeof(dwTraceFlags[0]));

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
    DWORD dwTraceFlags[] = {LSA_TRACE_FLAG_USER_GROUP_ADMINISTRATION};
    PLSA_GROUP_INFO_1 pGroup = NULL;
    PLSA_AUTH_PROVIDER pProvider = NULL;
    BOOLEAN bInLock = FALSE;
    HANDLE hProvider = (HANDLE)NULL;
    PLSA_SRV_API_STATE pServerState = (PLSA_SRV_API_STATE)hServer;

    LSA_TRACE_BEGIN_FUNCTION(dwTraceFlags, sizeof(dwTraceFlags)/sizeof(dwTraceFlags[0]));

    if (pServerState->peerUID)
    {
        dwError = EACCES;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (dwGroupInfoLevel != 1) {
        dwError = LW_ERROR_UNSUPPORTED_GROUP_LEVEL;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaValidateGroupInfo(
                    pGroupInfo,
                    dwGroupInfoLevel);
    BAIL_ON_LSA_ERROR(dwError);

    pGroup = (PLSA_GROUP_INFO_1)pGroupInfo;

    ENTER_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    dwError = LW_ERROR_NOT_HANDLED;

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
        } else if (dwError == LW_ERROR_NOT_HANDLED) {

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

    LSA_TRACE_END_FUNCTION(dwTraceFlags, sizeof(dwTraceFlags)/sizeof(dwTraceFlags[0]));

    return(dwError);

error:

    goto cleanup;
}

DWORD
LsaSrvModifyGroup(
    HANDLE hServer,
    PLSA_GROUP_MOD_INFO pGroupModInfo
    )
{
    DWORD dwError = 0;
    DWORD dwTraceFlags[] = {LSA_TRACE_FLAG_USER_GROUP_ADMINISTRATION};
    BOOLEAN bInLock = FALSE;
    PLSA_SRV_API_STATE pServerState = (PLSA_SRV_API_STATE)hServer;
    PLSA_AUTH_PROVIDER pProvider = NULL;
    HANDLE hProvider = (HANDLE)NULL;

    LSA_TRACE_BEGIN_FUNCTION(dwTraceFlags, sizeof(dwTraceFlags)/sizeof(dwTraceFlags[0]));

    if (pServerState->peerUID)
    {
        dwError = EACCES;
        BAIL_ON_LSA_ERROR(dwError);
    }

    ENTER_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    dwError = LW_ERROR_NOT_HANDLED;

    for (pProvider = gpAuthProviderList; pProvider; pProvider = pProvider->pNext)
    {
        dwError = LsaSrvOpenProvider(hServer, pProvider, &hProvider);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = pProvider->pFnTable->pfnModifyGroup(
                                        hProvider,
                                        pGroupModInfo);
        if (!dwError)
        {
            break;
        }
        else if ((dwError = LW_ERROR_NOT_HANDLED) ||
                 (dwError = LW_ERROR_NO_SUCH_GROUP))
        {
            LsaSrvCloseProvider(pProvider, hProvider);
            hProvider = (HANDLE)NULL;

            continue;
        }
        else
        {
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

cleanup:
    if (hProvider != (HANDLE)NULL)
    {
        LsaSrvCloseProvider(pProvider, hProvider);
    }

    LEAVE_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    LSA_TRACE_END_FUNCTION(dwTraceFlags, sizeof(dwTraceFlags)/sizeof(dwTraceFlags[0]));

    return dwError;

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
    DWORD dwTraceFlags[] = {LSA_TRACE_FLAG_USER_GROUP_ADMINISTRATION};
    PLSA_AUTH_PROVIDER pProvider = NULL;
    HANDLE hProvider = (HANDLE)NULL;
    BOOLEAN bInLock = FALSE;
    PLSA_SRV_API_STATE pServerState = (PLSA_SRV_API_STATE)hServer;

    LSA_TRACE_BEGIN_FUNCTION(dwTraceFlags, sizeof(dwTraceFlags)/sizeof(dwTraceFlags[0]));

    if (pServerState->peerUID)
    {
        dwError = EACCES;
        BAIL_ON_LSA_ERROR(dwError);
    }

    ENTER_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    dwError = LW_ERROR_NOT_HANDLED;

    for (pProvider = gpAuthProviderList; pProvider; pProvider = pProvider->pNext)
    {
        dwError = LsaSrvOpenProvider(hServer, pProvider, &hProvider);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = pProvider->pFnTable->pfnDeleteGroup(hProvider, gid);
        if (!dwError) {
            break;
        } else if ((dwError == LW_ERROR_NOT_HANDLED) ||
                   (dwError == LW_ERROR_NO_SUCH_GROUP)) {

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

    LSA_TRACE_END_FUNCTION(dwTraceFlags, sizeof(dwTraceFlags)/sizeof(dwTraceFlags[0]));

    return(dwError);

error:

    goto cleanup;
}

DWORD
LsaSrvBeginEnumGroups(
    HANDLE hServer,
    DWORD  dwGroupInfoLevel,
    DWORD  dwMaxNumGroups,
    BOOLEAN bCheckGroupMembersOnline,
    LSA_FIND_FLAGS FindFlags,
    PHANDLE phState
    )
{
    DWORD dwError = 0;
    DWORD dwTraceFlags[] = {LSA_TRACE_FLAG_USER_GROUP_QUERIES};
    PLSA_SRV_ENUM_STATE pEnumState = NULL;

    LSA_TRACE_BEGIN_FUNCTION(dwTraceFlags, sizeof(dwTraceFlags)/sizeof(dwTraceFlags[0]));

    dwError = LsaSrvCreateGroupEnumState(
                    hServer,
                    dwGroupInfoLevel,
                    dwMaxNumGroups,
                    bCheckGroupMembersOnline,
                    FindFlags,
                    &pEnumState);
    BAIL_ON_LSA_ERROR(dwError);

    *phState = (HANDLE) pEnumState;

cleanup:

    LSA_TRACE_END_FUNCTION(dwTraceFlags, sizeof(dwTraceFlags)/sizeof(dwTraceFlags[0]));

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaSrvEnumGroups(
    HANDLE  hServer,
    HANDLE  hState,
    PDWORD  pdwGroupInfoLevel,
    PVOID** pppGroupInfoList,
    PDWORD  pdwNumGroupsFound
    )
{
    DWORD dwError = 0;
    DWORD dwTraceFlags[] = {LSA_TRACE_FLAG_USER_GROUP_QUERIES};
    PLSA_SRV_ENUM_STATE pEnumState = (PLSA_SRV_ENUM_STATE) hState;
    PVOID* ppGroupInfoList_accumulate = NULL;
    DWORD  dwTotalNumGroupsFound = 0;
    PVOID* ppGroupInfoList = NULL;
    DWORD  dwNumGroupsFound = 0;
    DWORD  dwNumGroupsRemaining = 0;
    DWORD  dwGroupInfoLevel = 0;

    LSA_TRACE_BEGIN_FUNCTION(dwTraceFlags, sizeof(dwTraceFlags)/sizeof(dwTraceFlags[0]));

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
           if (dwError != LW_ERROR_NO_MORE_GROUPS) {
              BAIL_ON_LSA_ERROR(dwError);
           }
        }

        dwNumGroupsRemaining -= dwNumGroupsFound;

        if (dwNumGroupsRemaining) {
           pEnumState->pCurProviderState = pEnumState->pCurProviderState->pNext;
           if (dwError == LW_ERROR_NO_MORE_GROUPS){
             dwError = 0;
           }
        }

        dwError = LsaAppendAndFreePtrs(
                        &dwTotalNumGroupsFound,
                        &ppGroupInfoList_accumulate,
                        &dwNumGroupsFound,
                        &ppGroupInfoList);
        BAIL_ON_LSA_ERROR(dwError);
    }

    *pdwGroupInfoLevel = dwGroupInfoLevel;
    *pppGroupInfoList = ppGroupInfoList_accumulate;
    *pdwNumGroupsFound = dwTotalNumGroupsFound;

cleanup:

    LSA_TRACE_END_FUNCTION(dwTraceFlags, sizeof(dwTraceFlags)/sizeof(dwTraceFlags[0]));

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
    HANDLE  hServer,
    HANDLE  hState
    )
{
    DWORD dwError = 0;
    DWORD dwTraceFlags[] = {LSA_TRACE_FLAG_USER_GROUP_QUERIES};
    PLSA_SRV_ENUM_STATE pEnumState = (PLSA_SRV_ENUM_STATE) hState;
    PLSA_SRV_PROVIDER_STATE pProviderState = NULL;

    LSA_TRACE_BEGIN_FUNCTION(dwTraceFlags, sizeof(dwTraceFlags)/sizeof(dwTraceFlags[0]));

    for (pProviderState = pEnumState->pProviderStateList;
         pProviderState;
         pProviderState = pProviderState->pNext)
    {
        PLSA_AUTH_PROVIDER pProvider = pProviderState->pProvider;
        if (pProvider) {
           HANDLE hProvider = pProviderState->hProvider;
           pProvider->pFnTable->pfnEndEnumGroups(
                                       hProvider,
                                       pProviderState->hResume);
        }
    }

    LsaSrvFreeEnumState(pEnumState);

    LSA_TRACE_END_FUNCTION(dwTraceFlags, sizeof(dwTraceFlags)/sizeof(dwTraceFlags[0]));

    return dwError;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
