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
 *        provider-main.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Active Directory Authentication Provider
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Wei Fu (wfu@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 */

#include "adprovider.h"



DWORD
LSA_INITIALIZE_PROVIDER(ad)(
    PSTR* ppszProviderName,
    PLSA_PROVIDER_FUNCTION_TABLE* ppFunctionTable
    )
{
    *ppFunctionTable = &gADProviderAPITable;
    *ppszProviderName = gpszADProviderName;

cleanup:


    return dwError;

error:

    *ppszProviderName = NULL;
    *ppFunctionTable = NULL;

    goto cleanup;
}


DWORD
LSA_SHUTDOWN_PROVIDER(ad)(
    PSTR pszProviderName,
    PLSA_PROVIDER_FUNCTION_TABLE pFnTable
    )
{
    DWORD dwError = 0;
    return dwError;
}


DWORD
ADOpenHandle(
    uid_t   uid,
    gid_t   gid,
    pid_t   pid,
    PHANDLE phProvider
    )
{
    DWORD dwError = 0;
    PADPROVIDER_CONTEXT pContext = NULL;

    dwError = LwAllocateMemory(
                    sizeof(ADPROVIDER_CONTEXT),
                    (PVOID*)&pContext);
    BAIL_ON_LSA_ERROR(dwError);

    pContext->uid = uid;
    pContext->gid = gid;
    pContext->pid = pid;

    *phProvider = (HANDLE)pContext;

cleanup:

    return dwError;

error:

    *phProvider = (HANDLE)NULL;

    if (pContext) {
        ADCloseHandle((HANDLE)pContext);
    }

    goto cleanup;
}

void
ADCloseHandle(
    HANDLE hProvider
    )
{
    PADPROVIDER_CONTEXT pContext = (PADPROVIDER_CONTEXT)hProvider;
    if (pContext)
    {
        LwFreeMemory(pContext);
    }
}

BOOLEAN
ADServicesDomain(
    PWSTR pszDomain
    )
{
    BOOLEAN bResult = FALSE;

    LsaAdProviderStateAcquireRead(gpLsaAdProviderState);

    if (gpLsaAdProviderState->joinState != LSA_ADJOINED)
    {
        goto cleanup;
    }

    //
    // Added Trusted domains support
    //
    if (LW_IS_NULL_OR_EMPTY_STR(pszDomain) ||
        LW_IS_NULL_OR_EMPTY_STR(gpADProviderData->szDomain) ||
        LW_IS_NULL_OR_EMPTY_STR(gpADProviderData->szShortDomain)) {
       goto cleanup;
    }

    bResult = LsaDmIsDomainPresent(pszDomain);
    if (!bResult)
    {
        LSA_LOG_INFO("ADServicesDomain was passed unknown domain '%s'", pszDomain);
    }

cleanup:

    LsaAdProviderStateRelease(gpLsaAdProviderState);

    return bResult;
}

DWORD
ADAuthenticateUser(
    HANDLE hProvider,
    PWSTR  pszLoginId,
    PWSTR  pszPassword
    )
{

    return dwError;
}

DWORD
ADAuthenticateUserEx(
    HANDLE hProvider,
    PLSA_AUTH_USER_PARAMS pUserParams,
    PLSA_AUTH_USER_INFO *ppUserInfo
    )
{
}

DWORD
ADValidateUser(
    HANDLE hProvider,
    PWSTR  pszLoginId,
    PWSTR  pszPassword
    )
{
    return dwError;

error:

    goto cleanup;
}

DWORD
ADCheckUserInList(
    HANDLE hProvider,
    PWSTR  pszUserName,
    PWSTR  pszListName
    )
{
    DWORD  dwError = 0;

    return dwError;

}

DWORD
ADFindUserByName(
    HANDLE  hProvider,
    PWSTR   pszLoginId,
    DWORD   dwUserInfoLevel,
    PVOID*  ppUserInfo
    )
{
    DWORD   dwError = 0;

	return dwError;
}

DWORD
ADFindUserById(
    HANDLE  hProvider,
    uid_t   uid,
    DWORD   dwUserInfoLevel,
    PVOID*  ppUserInfo
    )
{
    DWORD   dwError = 0;

}
DWORD
ADFindUserObjectById(
    IN HANDLE  hProvider,
    IN uid_t   uid,
    OUT PLSA_SECURITY_OBJECT* ppResult
    )
{
    return dwError;
}

DWORD
ADBeginEnumUsers(
    HANDLE  hProvider,
    DWORD   dwInfoLevel,
    LSA_FIND_FLAGS FindFlags,
    PHANDLE phResume
    )
{

}

DWORD
ADEnumUsers(
    HANDLE  hProvider,
    HANDLE  hResume,
    DWORD   dwMaxNumUsers,
    PDWORD  pdwUsersFound,
    PVOID** pppUserInfoList
    )
{
    DWORD dwError = 0;

	return dwError;
}

VOID
ADEndEnumUsers(
    HANDLE hProvider,
    HANDLE hResume
    )
{
    return;
}


static
DWORD
ADPreJoinDomain(
    HANDLE hProvider,
    PLSA_ADPROVIDER_STATE pState
    )
{
    DWORD dwError = 0;

    switch (pState->joinState)
    {
    case LSA_ADUNKNOWN:
    case LSA_ADNOT_JOINED:
        break;
    case LSA_ADJOINED:
        dwError = ADTransitionNotJoined(pState);
        BAIL_ON_LSA_ERROR(dwError);
        break;
    }

error:

    return dwError;
}

static
DWORD
ADPostJoinDomain(
    HANDLE hProvider,
    PLSA_ADPROVIDER_STATE pState
    )
{
    DWORD dwError = 0;

    dwError = ADTransitionJoined(pState);
    BAIL_ON_LSA_ERROR(dwError);

error:

    return dwError;
}


static
DWORD
ADJoinDomain(
    HANDLE  hProvider,
    uid_t   peerUID,
    gid_t   peerGID,
    DWORD   dwInputBufferSize,
    PVOID   pInputBuffer,
    DWORD* pdwOutputBufferSize,
    PVOID* ppOutputBuffer
    )
{
    DWORD dwError = 0;
    LWMsgDataContext* pDataContext = NULL;
    PLSA_ADIPC_JOIN_DOMAIN_REQ pRequest = NULL;
    PSTR pszMessage = NULL;
    BOOLEAN bLocked = FALSE;

    if (peerUID != 0)
    {
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = MAP_LWMSG_ERROR(lwmsg_data_context_new(NULL, &pDataContext));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_unmarshal_flat(
                                  pDataContext,
                                  LsaAdIPCGetJoinDomainReqSpec(),
                                  pInputBuffer,
                                  (size_t) dwInputBufferSize,
                                  OUT_PPVOID(&pRequest)));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_print_graph_alloc(
                                  pDataContext,
                                  LsaAdIPCGetJoinDomainReqSpec(),
                                  pRequest,
                                  &pszMessage));
    BAIL_ON_LSA_ERROR(dwError);

    LSA_LOG_TRACE("Domain join request: %s", pszMessage);

    LsaAdProviderStateAcquireWrite(gpLsaAdProviderState);
    bLocked = TRUE;

    dwError = ADPreJoinDomain(hProvider, gpLsaAdProviderState);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaNetJoinDomain(
        pRequest->pszHostname,
        pRequest->pszHostDnsDomain,
        pRequest->pszDomain,
        pRequest->pszOU,
        pRequest->pszUsername,
        pRequest->pszPassword,
        pRequest->pszOSName,
        pRequest->pszOSVersion,
        pRequest->pszOSServicePack,
        pRequest->dwFlags);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADPostJoinDomain(hProvider, gpLsaAdProviderState);
    BAIL_ON_LSA_ERROR(dwError);

    LSA_LOG_INFO("Joined domain: %s", pRequest->pszDomain);

cleanup:

    if (bLocked)
    {
        LsaAdProviderStateRelease(gpLsaAdProviderState);
    }

    LW_SAFE_FREE_MEMORY(pszMessage);

    if (pRequest)
    {
        lwmsg_data_free_graph(
            pDataContext,
            LsaAdIPCGetJoinDomainReqSpec(),
            pRequest);
    }

    if (pDataContext)
    {
        lwmsg_data_context_delete(pDataContext);
    }

    return dwError;

error:

    goto cleanup;
}

static
DWORD
ADPreLeaveDomain(
    HANDLE hProvider,
    PLSA_ADPROVIDER_STATE pState
    )
{
    DWORD dwError = 0;

    switch (pState->joinState)
    {
    case LSA_ADUNKNOWN:
    case LSA_ADNOT_JOINED:
        dwError = LW_ERROR_NOT_JOINED_TO_AD;
        BAIL_ON_LSA_ERROR(dwError);
        break;
    case LSA_ADJOINED:
        ADTransitionNotJoined(pState);
        break;
    }

error:

    return dwError;
}

static
DWORD
ADPostLeaveDomain(
    HANDLE hProvider,
    PLSA_ADPROVIDER_STATE pState
    )
{
    DWORD dwError = 0;

    return dwError;
}

static
DWORD
ADLeaveDomain(
    HANDLE  hProvider,
    uid_t   peerUID,
    gid_t   peerGID,
    DWORD   dwInputBufferSize,
    PVOID   pInputBuffer,
    DWORD* pdwOutputBufferSize,
    PVOID* ppOutputBuffer
    )
{
    DWORD dwError = 0;
    LWMsgDataContext* pDataContext = NULL;
    PLSA_ADIPC_LEAVE_DOMAIN_REQ pRequest = NULL;
    PSTR pszMessage = NULL;
    BOOLEAN bLocked = FALSE;

    if (peerUID != 0)
    {
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = MAP_LWMSG_ERROR(lwmsg_data_context_new(NULL, &pDataContext));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_unmarshal_flat(
                                  pDataContext,
                                  LsaAdIPCGetLeaveDomainReqSpec(),
                                  pInputBuffer,
                                  (size_t) dwInputBufferSize,
                                  OUT_PPVOID(&pRequest)));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_print_graph_alloc(
                                  pDataContext,
                                  LsaAdIPCGetLeaveDomainReqSpec(),
                                  pRequest,
                                  &pszMessage));
    BAIL_ON_LSA_ERROR(dwError);

    LSA_LOG_TRACE("Domain leave request: %s", pszMessage);

    LsaAdProviderStateAcquireWrite(gpLsaAdProviderState);
    bLocked = TRUE;

    dwError = ADPreLeaveDomain(hProvider, gpLsaAdProviderState);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaNetLeaveDomain(
        pRequest->pszUsername,
        pRequest->pszPassword);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADPostLeaveDomain(hProvider, gpLsaAdProviderState);
    BAIL_ON_LSA_ERROR(dwError);

    LSA_LOG_INFO("Left domain\n");

cleanup:

    if (bLocked)
    {
        LsaAdProviderStateRelease(gpLsaAdProviderState);
    }

    LW_SAFE_FREE_MEMORY(pszMessage);

    if (pRequest)
    {
        lwmsg_data_free_graph(
            pDataContext,
            LsaAdIPCGetLeaveDomainReqSpec(),
            pRequest);
    }

    if (pDataContext)
    {
        lwmsg_data_context_delete(pDataContext);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
ADFindGroupByName(
    IN HANDLE hProvider,
    IN PWSTR pszGroupName,
    IN LSA_FIND_FLAGS FindFlags,
    IN DWORD dwGroupInfoLevel,
    OUT PVOID* ppGroupInfo
    )
{
    DWORD dwError = 0;
    BOOLEAN bIsCacheOnlyMode = FALSE;

    LsaAdProviderStateAcquireRead(gpLsaAdProviderState);

    if (gpLsaAdProviderState->joinState != LSA_ADJOINED)
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (FindFlags & LSA_FIND_FLAGS_NSS)
    {
        bIsCacheOnlyMode = ADGetNssGroupMembersCacheOnlyEnabled();
    }

    dwError = ADFindGroupByNameWithCacheMode(
                hProvider,
                pszGroupName,
                bIsCacheOnlyMode,
                dwGroupInfoLevel,
                ppGroupInfo);
    BAIL_ON_LSA_ERROR(dwError);

error:

    LsaAdProviderStateRelease(gpLsaAdProviderState);

    return dwError;
}


static
DWORD
ADGetExpandedGroupUsersEx(
    IN HANDLE hProvider,
    IN PWSTR pszDomainName,
    IN BOOLEAN bIsOffline,
    IN PWSTR pszGroupSid,
    IN BOOLEAN bIsCacheOnlyMode,
    IN DWORD dwMaxDepth,
    OUT PBOOLEAN pbIsFullyExpanded,
    OUT size_t* psMemberUsersCount,
    OUT PLSA_SECURITY_OBJECT** pppMemberUsers
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;

	return dwError;
}
DWORD
ADFindGroupById(
    IN HANDLE hProvider,
    IN gid_t gid,
    IN LSA_FIND_FLAGS FindFlags,
    IN DWORD dwGroupInfoLevel,
    OUT PVOID* ppGroupInfo
    )
{
    DWORD dwError = 0;

	return dwError;
}


DWORD
ADGetUserGroupObjectMembership(
    IN HANDLE hProvider,
    IN PLSA_SECURITY_OBJECT pUserInfo,
    IN BOOLEAN bIsCacheOnlyMode,
    OUT size_t* psNumGroupsFound,
    OUT PLSA_SECURITY_OBJECT** pppResult
    )
{
    DWORD dwError = 0;
}

DWORD
ADGetGroupsForUser(
    IN HANDLE hProvider,
    IN OPTIONAL PWSTR pszUserName,
    IN OPTIONAL uid_t uid,
    IN LSA_FIND_FLAGS FindFlags,
    IN DWORD dwGroupInfoLevel,
    IN PDWORD pdwNumGroupsFound,
    IN PVOID** pppGroupInfoList
    )
{
	DWORD dwError = 0;

	return dwError;
}

DWORD
ADBeginEnumGroups(
    HANDLE  hProvider,
    DWORD   dwInfoLevel,
    BOOLEAN bCheckGroupMembersOnline,
    LSA_FIND_FLAGS FindFlags,
    PHANDLE phResume
    )
{
	DWORD dwError = 0;

	return dwError;
}

DWORD
ADEnumGroups(
    HANDLE  hProvider,
    HANDLE  hResume,
    DWORD   dwMaxGroups,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
    )
{
    DWORD dwError = 0;
}

VOID
ADEndEnumGroups(
    HANDLE hProvider,
    HANDLE hResume
    )
{
	return;
}

DWORD
ADChangePassword(
    HANDLE hProvider,
    PWSTR pszLoginId,
    PWSTR pszPassword,
    PWSTR pszOldPassword
    )
{

    return dwError;
}

DWORD
ADSetPassword(
    HANDLE hProvider,
    PWSTR pszLoginId,
    PWSTR pszPassword
    )
{
    return LW_ERROR_NOT_HANDLED;
}


DWORD
ADAddUser(
    HANDLE hProvider,
    DWORD  dwUserInfoLevel,
    PVOID  pUserInfo
    )
{
    return LW_ERROR_NOT_HANDLED;
}

DWORD
ADModifyUser(
    HANDLE hProvider,
    PLSA_USER_MOD_INFO pUserModInfo
    )
{
    return LW_ERROR_NOT_HANDLED;
}

DWORD
ADDeleteUser(
    HANDLE hProvider,
    uid_t  uid
    )
{
    return LW_ERROR_NOT_HANDLED;
}

DWORD
ADAddGroup(
    HANDLE hProvider,
    DWORD dwGroupInfoLevel,
    PVOID pGroupInfo
    )
{
    return LW_ERROR_NOT_HANDLED;
}

DWORD
ADModifyGroup(
    HANDLE hProvider,
    PLSA_GROUP_MOD_INFO pGroupModInfo
    )
{
    return LW_ERROR_NOT_HANDLED;
}

DWORD
ADDeleteGroup(
    HANDLE hProvider,
    gid_t  gid
    )
{
    return LW_ERROR_NOT_HANDLED;
}

DWORD
ADOpenSession(
    HANDLE hProvider,
    PWSTR  pszLoginId
    )
{
    DWORD dwError = 0;
}

DWORD
ADCloseSession(
    HANDLE hProvider,
    PWSTR  pszLoginId
    )
{
}

DWORD
ADGetNamesBySidList(
    HANDLE          hProvider,
    size_t          sCount,
    PSTR*           ppszSidList,
    PSTR**          pppszDomainNames,
    PSTR**          pppszSamAccounts,
    ADAccountType** ppTypes
    )
{
}

DWORD
ADFindNSSArtefactByKey(
    HANDLE hProvider,
    PWSTR  pszKeyName,
    PWSTR  pszMapName,
    DWORD  dwInfoLevel,
    LSA_NIS_MAP_QUERY_FLAGS dwFlags,
    PVOID* ppNSSArtefactInfo
    )
{
}

DWORD
ADBeginEnumNSSArtefacts(
    HANDLE  hProvider,
    DWORD   dwInfoLevel,
    PWSTR   pszMapName,
    LSA_NIS_MAP_QUERY_FLAGS dwFlags,
    PHANDLE phResume
    )
{
}

DWORD
ADEnumNSSArtefacts(
    HANDLE  hProvider,
    HANDLE  hResume,
    DWORD   dwMaxNSSArtefacts,
    PDWORD  pdwNSSArtefactsFound,
    PVOID** pppNSSArtefactInfoList
    )
{
}

VOID
ADEndEnumNSSArtefacts(
    HANDLE hProvider,
    HANDLE hResume
    )
{

}

DWORD
ADGetStatus(
    HANDLE hProvider,
    PLSA_AUTH_PROVIDER_STATUS* ppProviderStatus
    )
{
}



DWORD
ADRefreshConfiguration(
    HANDLE hProvider
    )
{
}

DWORD
ADProviderIoControl(
    IN HANDLE  hProvider,
    IN uid_t   peerUID,
    IN uid_t   peerGID,
    IN DWORD   dwIoControlCode,
    IN DWORD   dwInputBufferSize,
    IN PVOID   pInputBuffer,
    OUT DWORD* pdwOutputBufferSize,
    OUT PVOID* ppOutputBuffer
    )
{
}

DWORD
ADGetGroupMembershipByProvider(
    IN HANDLE     hProvider,
    IN PWSTR      pszSid,
    IN DWORD      dwGroupInfoLevel,
    OUT PDWORD    pdwGroupsCount,
    OUT PVOID   **pppMembershipInfo
    )
{
    return LW_ERROR_NOT_HANDLED;
}


DWORD
ADFindUserObjectByName(
    IN HANDLE  hProvider,
    IN PWSTR   pszLoginId,
    OUT PLSA_SECURITY_OBJECT* ppResult
    )
{
    DWORD dwError = 0;
    goto cleanup;
}

DWORD
ADFindGroupObjectByName(
    IN HANDLE  hProvider,
    IN PWSTR   pszGroupName,
    OUT PLSA_SECURITY_OBJECT* ppResult
    )
{
	DWORD dwError = 0;

	return dwError;
}

