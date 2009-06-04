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
 *        globals.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Server API Globals
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "api.h"

time_t gServerStartTime = 0;

PLSA_AUTH_PROVIDER gpAuthProviderList = NULL;
pthread_rwlock_t gpAuthProviderList_rwlock;

PLSA_RPC_SERVER gpRpcServerList = NULL;
pthread_rwlock_t gpRpcServerList_rwlock;

pthread_rwlock_t gPerfCounters_rwlock;
UINT64 gPerfCounters[LsaMetricSentinel];

pthread_mutex_t    gAPIConfigLock     = PTHREAD_MUTEX_INITIALIZER;
PSTR               gpszConfigFilePath = NULL;
LSA_SRV_API_CONFIG gAPIConfig = {0};

DWORD
LsaSrvApiInit(
    PCSTR pszConfigFilePath
    )
{
    DWORD dwError = 0;
    LSA_SRV_API_CONFIG apiConfig = {0};

    gServerStartTime = time(NULL);

    pthread_rwlock_init(&gPerfCounters_rwlock, NULL);

    memset(&gPerfCounters[0], 0, sizeof(gPerfCounters));

    pthread_rwlock_init(&gpAuthProviderList_rwlock, NULL);

    dwError = LsaSrvApiInitConfig(&gAPIConfig);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvApiReadConfig(
                    pszConfigFilePath,
                    &apiConfig);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvApiTransferConfigContents(
                    &apiConfig,
                    &gAPIConfig);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvInitAuthProviders(pszConfigFilePath);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaInitRpcServers(pszConfigFilePath);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAllocateString(
                    pszConfigFilePath,
                    &gpszConfigFilePath);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LsaSrvApiFreeConfigContents(&apiConfig);

    return 0;

error:

    goto cleanup;
}

DWORD
LsaSrvApiShutdown(
    VOID
    )
{
    LsaSrvFreeAuthProviders();

    pthread_mutex_lock(&gAPIConfigLock);

    LSA_SAFE_FREE_STRING(gpszConfigFilePath);

    LsaSrvApiFreeConfigContents(&gAPIConfig);

    pthread_mutex_unlock(&gAPIConfigLock);

    return 0;


}



static LWMsgDispatchSpec gMessageHandlers[] =
{
    LWMSG_DISPATCH(LSA_Q_GROUP_BY_NAME, LsaSrvIpcFindGroupByName),
    LWMSG_DISPATCH(LSA_Q_GROUP_BY_ID, LsaSrvIpcFindGroupById),
    LWMSG_DISPATCH(LSA_Q_BEGIN_ENUM_GROUPS, LsaSrvIpcBeginEnumGroups),
    LWMSG_DISPATCH(LSA_Q_ENUM_GROUPS, LsaSrvIpcEnumGroups),
    LWMSG_DISPATCH(LSA_Q_END_ENUM_GROUPS, LsaSrvIpcEndEnumGroups),
    LWMSG_DISPATCH(LSA_Q_USER_BY_NAME, LsaSrvIpcFindUserByName),
    LWMSG_DISPATCH(LSA_Q_USER_BY_ID, LsaSrvIpcFindUserById),
    LWMSG_DISPATCH(LSA_Q_BEGIN_ENUM_USERS, LsaSrvIpcBeginEnumUsers),
    LWMSG_DISPATCH(LSA_Q_ENUM_USERS, LsaSrvIpcEnumUsers),
    LWMSG_DISPATCH(LSA_Q_END_ENUM_USERS, LsaSrvIpcEndEnumUsers),
    LWMSG_DISPATCH(LSA_Q_AUTH_USER, LsaSrvIpcAuthenticateUser),
    LWMSG_DISPATCH(LSA_Q_AUTH_USER_EX, LsaSrvIpcAuthenticateUserEx),
    LWMSG_DISPATCH(LSA_Q_VALIDATE_USER, LsaSrvIpcValidateUser),
    LWMSG_DISPATCH(LSA_Q_CHANGE_PASSWORD, LsaSrvIpcChangePassword),
    LWMSG_DISPATCH(LSA_Q_SET_PASSWORD, LsaSrvIpcSetPassword),
    LWMSG_DISPATCH(LSA_Q_OPEN_SESSION, LsaSrvIpcOpenSession),
    LWMSG_DISPATCH(LSA_Q_CLOSE_SESSION, LsaSrvIpcCloseSession),
    LWMSG_DISPATCH(LSA_Q_MODIFY_USER, LsaSrvIpcModifyUser),
    LWMSG_DISPATCH(LSA_Q_NAMES_BY_SID_LIST, LsaSrvIpcGetNamesBySidList),
    LWMSG_DISPATCH(LSA_Q_GSS_MAKE_AUTH_MSG, LsaSrvIpcBuildAuthMessage),
    LWMSG_DISPATCH(LSA_Q_GSS_CHECK_AUTH_MSG, LsaSrvIpcCheckAuthMessage),
    LWMSG_DISPATCH(LSA_Q_ADD_GROUP, LsaSrvIpcAddGroup),
    LWMSG_DISPATCH(LSA_Q_MODIFY_GROUP, LsaSrvIpcModifyGroup),
    LWMSG_DISPATCH(LSA_Q_DELETE_GROUP, LsaSrvIpcDeleteGroup),
    LWMSG_DISPATCH(LSA_Q_ADD_USER, LsaSrvIpcAddUser),
    LWMSG_DISPATCH(LSA_Q_DELETE_USER, LsaSrvIpcDeleteUser),
    LWMSG_DISPATCH(LSA_Q_GROUPS_FOR_USER, LsaSrvIpcGetGroupsForUser),
    LWMSG_DISPATCH(LSA_Q_GET_METRICS, LsaSrvIpcGetMetrics),
    LWMSG_DISPATCH(LSA_Q_SET_LOGINFO, LsaSrvIpcSetLogInfo),
    LWMSG_DISPATCH(LSA_Q_GET_LOGINFO, LsaSrvIpcGetLogInfo),
    LWMSG_DISPATCH(LSA_Q_GET_STATUS, LsaSrvIpcGetStatus),
    LWMSG_DISPATCH(LSA_Q_REFRESH_CONFIGURATION, LsaSrvIpcRefreshConfiguration),
    LWMSG_DISPATCH(LSA_Q_CHECK_USER_IN_LIST, LsaSrvIpcCheckUserInList),
    LWMSG_DISPATCH(LSA_Q_BEGIN_ENUM_NSS_ARTEFACTS, LsaSrvIpcBeginEnumNSSArtefacts),
    LWMSG_DISPATCH(LSA_Q_ENUM_NSS_ARTEFACTS, LsaSrvIpcEnumNSSArtefacts),
    LWMSG_DISPATCH(LSA_Q_END_ENUM_NSS_ARTEFACTS, LsaSrvIpcEndEnumNSSArtefacts),
    LWMSG_DISPATCH(LSA_Q_FIND_NSS_ARTEFACT_BY_KEY, LsaSrvIpcFindNSSArtefactByKey),
    LWMSG_DISPATCH(LSA_Q_SET_TRACE_INFO, LsaSrvIpcSetTraceInfo),
    LWMSG_DISPATCH(LSA_Q_GET_TRACE_INFO, LsaSrvIpcGetTraceInfo),
    LWMSG_DISPATCH(LSA_Q_ENUM_TRACE_INFO, LsaSrvIpcEnumTraceInfo),
    LWMSG_DISPATCH(LSA_Q_PROVIDER_IO_CONTROL, LsaSrvIpcProviderIoControl),
    LWMSG_DISPATCH_END
};

LWMsgDispatchSpec*
LsaSrvGetDispatchSpec(
    void
    )
{
    return gMessageHandlers;
}


pthread_t gRpcSrvWorker;
