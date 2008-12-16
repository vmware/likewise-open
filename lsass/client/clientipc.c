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
 *        ipc_client.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Inter-process Communication (Client) API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "client.h"

DWORD
LsaOpenServer(
    PHANDLE phConnection
    )
{
    DWORD dwError = 0;
    PLSA_CLIENT_CONNECTION_CONTEXT pContext = NULL;

    BAIL_ON_INVALID_POINTER(phConnection);

    dwError = LsaAllocateMemory(sizeof(LSA_CLIENT_CONNECTION_CONTEXT), (PVOID*)&pContext);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_protocol_new(NULL, &pContext->pProtocol));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_protocol_add_protocol_spec(pContext->pProtocol, LsaIPCGetProtocolSpec()));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_connection_new(pContext->pProtocol, &pContext->pAssoc));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_connection_set_endpoint(
                                  pContext->pAssoc,
                                  LWMSG_CONNECTION_MODE_LOCAL,
                                  CACHEDIR "/" LSA_SERVER_FILENAME));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaTransactOpenServer((HANDLE)pContext);
    BAIL_ON_LSA_ERROR(dwError);

    *phConnection = (HANDLE)pContext;

cleanup:
    return dwError;

error:
    if (pContext)
    {
        if (pContext->pAssoc)
        {
            lwmsg_assoc_delete(pContext->pAssoc);
        }

        if (pContext->pProtocol)
        {
            lwmsg_protocol_delete(pContext->pProtocol);
        }

        LsaFreeMemory(pContext);
    }

    if (phConnection)
    {
        *phConnection = (HANDLE)NULL;
    }

    goto cleanup;
}

DWORD
LsaCloseServer(
    HANDLE hConnection
    )
{
    DWORD dwError = 0;
    PLSA_CLIENT_CONNECTION_CONTEXT pContext =
                     (PLSA_CLIENT_CONNECTION_CONTEXT)hConnection;

    if (pContext->pAssoc)
    {
        lwmsg_assoc_close(pContext->pAssoc);
        lwmsg_assoc_delete(pContext->pAssoc);
    }

    if (pContext->pProtocol)
    {
        lwmsg_protocol_delete(pContext->pProtocol);
    }

    LsaFreeMemory(pContext);

    return dwError;
}

DWORD
LsaTransactFindGroupByName(
   HANDLE hServer,
   PCSTR pszGroupName,
   LSA_FIND_FLAGS FindFlags,
   DWORD dwGroupInfoLevel,
   PVOID* ppGroupInfo
   )
{
    DWORD dwError = 0;
    PLSA_CLIENT_CONNECTION_CONTEXT pContext =
                     (PLSA_CLIENT_CONNECTION_CONTEXT)hServer;
    LSA_IPC_FIND_OBJECT_BY_NAME_REQ findObjectByNameReq;
    // Do not free pResult and pError
    PLSA_GROUP_INFO_LIST pResultList = NULL;
    PLSA_IPC_ERROR pError = NULL;

    LWMsgMessage request = {-1, NULL};
    LWMsgMessage response = {-1, NULL};

    findObjectByNameReq.Handle = (LsaIpcServerHandle*)pContext->hServer;
    findObjectByNameReq.FindFlags = FindFlags;
    findObjectByNameReq.dwInfoLevel = dwGroupInfoLevel;
    findObjectByNameReq.pszName = pszGroupName;

    request.tag = LSA_Q_GROUP_BY_NAME;
    request.object = &findObjectByNameReq;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_send_message_transact(
                              pContext->pAssoc,
                              &request,
                              &response));
    BAIL_ON_LSA_ERROR(dwError);

    switch (response.tag)
    {
        case LSA_R_GROUP_BY_NAME_SUCCESS:
            pResultList = (PLSA_GROUP_INFO_LIST)response.object;

            if (pResultList->dwNumGroups != 1)
            {
                dwError = LSA_ERROR_INTERNAL;
                BAIL_ON_LSA_ERROR(dwError);
            }

            switch (pResultList->dwGroupInfoLevel)
            {
                case 0:
                    *ppGroupInfo = pResultList->ppGroupInfoList.ppInfoList0[0];
                    pResultList->ppGroupInfoList.ppInfoList0[0] = NULL;
                    pResultList->dwNumGroups = 0;
                    break;
                case 1:
                    *ppGroupInfo = pResultList->ppGroupInfoList.ppInfoList1[0];
                    pResultList->ppGroupInfoList.ppInfoList1[0] = NULL;
                    pResultList->dwNumGroups = 0;
                    break;
                default:
                    dwError = LSA_ERROR_INVALID_PARAMETER;
                    BAIL_ON_LSA_ERROR(dwError);
            }
            break;
        case LSA_R_GROUP_BY_NAME_FAILURE:
            pError = (PLSA_IPC_ERROR) response.object;
            dwError = pError->dwError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = EINVAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    if (response.object)
    {
        lwmsg_assoc_free_message(pContext->pAssoc, &response);
    }

    return dwError;

error:
    *ppGroupInfo = NULL;

    goto cleanup;
}

DWORD
LsaTransactFindGroupById(
   HANDLE hServer,
   DWORD id,
   LSA_FIND_FLAGS FindFlags,
   DWORD dwGroupInfoLevel,
   PVOID* ppGroupInfo
   )
{
    DWORD dwError = 0;
    PLSA_CLIENT_CONNECTION_CONTEXT pContext =
                     (PLSA_CLIENT_CONNECTION_CONTEXT)hServer;
    LSA_IPC_FIND_OBJECT_BY_ID_REQ findObjectByIdReq;
    // Do not free pResult and pError
    PLSA_GROUP_INFO_LIST pResultList = NULL;
    PLSA_IPC_ERROR pError = NULL;

    LWMsgMessage request = {-1, NULL};
    LWMsgMessage response = {-1, NULL};

    findObjectByIdReq.Handle = (LsaIpcServerHandle*)pContext->hServer;
    findObjectByIdReq.FindFlags = FindFlags;
    findObjectByIdReq.dwInfoLevel = dwGroupInfoLevel;
    findObjectByIdReq.id = id;

    request.tag = LSA_Q_GROUP_BY_ID;
    request.object = &findObjectByIdReq;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_send_message_transact(
                              pContext->pAssoc,
                              &request,
                              &response));
    BAIL_ON_LSA_ERROR(dwError);

    switch (response.tag)
    {
        case LSA_R_GROUP_BY_ID_SUCCESS:
            pResultList = (PLSA_GROUP_INFO_LIST)response.object;
            switch (pResultList->dwGroupInfoLevel)
            {
                case 0:
                    *ppGroupInfo = pResultList->ppGroupInfoList.ppInfoList0[0];
                    pResultList->ppGroupInfoList.ppInfoList0[0] = NULL;
                    pResultList->dwNumGroups = 0;
                    break;
                case 1:
                    *ppGroupInfo = pResultList->ppGroupInfoList.ppInfoList1[0];
                    pResultList->ppGroupInfoList.ppInfoList1[0] = NULL;
                    pResultList->dwNumGroups = 0;
                    break;
                default:
                    dwError = LSA_ERROR_INVALID_PARAMETER;
                    BAIL_ON_LSA_ERROR(dwError);
            }
            break;
        case LSA_R_GROUP_BY_ID_FAILURE:
            pError = (PLSA_IPC_ERROR) response.object;
            dwError = pError->dwError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = EINVAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    if (response.object)
    {
        lwmsg_assoc_free_message(pContext->pAssoc, &response);
    }

    return dwError;

error:
    *ppGroupInfo = NULL;

    goto cleanup;
}

DWORD
LsaTransactBeginEnumGroups(
    HANDLE hServer,
    DWORD   dwGroupInfoLevel,
    DWORD   dwMaxNumGroups,
    PHANDLE phResume
    )
{
    DWORD dwError = 0;
    PLSA_CLIENT_CONNECTION_CONTEXT pContext =
                     (PLSA_CLIENT_CONNECTION_CONTEXT)hServer;
    LSA_IPC_BEGIN_ENUM_RECORDS_REQ beginGroupEnumReq;
    // Do not free pResult and pError
    PLSA_ENUM_OBJECTS_INFO pResult = NULL;
    PLSA_IPC_ERROR pError = NULL;

    LWMsgMessage request = {-1, NULL};
    LWMsgMessage response = {-1, NULL};

    beginGroupEnumReq.Handle = (LsaIpcServerHandle*)pContext->hServer;
    beginGroupEnumReq.dwInfoLevel = dwGroupInfoLevel;
    beginGroupEnumReq.dwNumMaxRecords = dwMaxNumGroups;

    request.tag = LSA_Q_BEGIN_ENUM_GROUPS;
    request.object = &beginGroupEnumReq;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_send_message_transact(
                              pContext->pAssoc,
                              &request,
                              &response));
    BAIL_ON_LSA_ERROR(dwError);

    switch (response.tag)
    {
        case LSA_R_BEGIN_ENUM_GROUPS_SUCCESS:
            pResult = (PLSA_ENUM_OBJECTS_INFO)response.object;
            *phResume = (HANDLE)pResult;
            break;
        case LSA_R_BEGIN_ENUM_GROUPS_FAILURE:
            pError = (PLSA_IPC_ERROR) response.object;
            dwError = pError->dwError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = EINVAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    return dwError;

error:
    if (response.object)
    {
        lwmsg_assoc_free_message(pContext->pAssoc, &response);
    }
    *phResume = (HANDLE)NULL;

    goto cleanup;

}

DWORD
LsaTransactEnumGroups(
    HANDLE hServer,
    HANDLE hResume,
    PDWORD pdwNumGroupsFound,
    PVOID** pppGroupInfoList)
{
    DWORD dwError = 0;
    PLSA_CLIENT_CONNECTION_CONTEXT pContext =
                     (PLSA_CLIENT_CONNECTION_CONTEXT)hServer;
    PLSA_ENUM_OBJECTS_INFO pInfo = (PLSA_ENUM_OBJECTS_INFO)hResume;

    LSA_IPC_ENUM_RECORDS_REQ groupEnumReq;
    // Do not free pResultList and pError
    PLSA_GROUP_INFO_LIST pResultList = NULL;
    PLSA_IPC_ERROR pError = NULL;

    LWMsgMessage request = {-1, NULL};
    LWMsgMessage response = {-1, NULL};

    groupEnumReq.Handle = (LsaIpcServerHandle*)pContext->hServer;
    groupEnumReq.pszToken = pInfo->pszGUID;

    request.tag = LSA_Q_ENUM_GROUPS;
    request.object = &groupEnumReq;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_send_message_transact(
                              pContext->pAssoc,
                              &request,
                              &response));
    BAIL_ON_LSA_ERROR(dwError);

    switch (response.tag)
    {
        case LSA_R_ENUM_GROUPS_SUCCESS:
            pResultList = (PLSA_GROUP_INFO_LIST)response.object;
            *pdwNumGroupsFound = pResultList->dwNumGroups;
            switch (pResultList->dwGroupInfoLevel)
            {
                case 0:
                    *pppGroupInfoList = (PVOID*)pResultList->ppGroupInfoList.ppInfoList0;
                    pResultList->ppGroupInfoList.ppInfoList0 = NULL;
                    pResultList->dwNumGroups = 0;
                    break;
                case 1:
                    *pppGroupInfoList = (PVOID*)pResultList->ppGroupInfoList.ppInfoList1;
                    pResultList->ppGroupInfoList.ppInfoList1 = NULL;
                    pResultList->dwNumGroups = 0;
                    break;
                default:
                   dwError = LSA_ERROR_INVALID_PARAMETER;
                   BAIL_ON_LSA_ERROR(dwError);
            }
            break;
        case LSA_R_ENUM_GROUPS_FAILURE:
            pError = (PLSA_IPC_ERROR) response.object;
            dwError = pError->dwError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = EINVAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    if (response.object)
    {
        lwmsg_assoc_free_message(pContext->pAssoc, &response);
    }

    return dwError;

error:
    *pdwNumGroupsFound = 0;
    *pppGroupInfoList = NULL;

    goto cleanup;
}

DWORD
LsaTransactEndEnumGroups(
    HANDLE hServer,
    HANDLE hResume
    )
{
    DWORD dwError = 0;
    PLSA_CLIENT_CONNECTION_CONTEXT pContext =
                     (PLSA_CLIENT_CONNECTION_CONTEXT)hServer;
    PLSA_ENUM_OBJECTS_INFO pInfo = (PLSA_ENUM_OBJECTS_INFO)hResume;
    LSA_IPC_ENUM_RECORDS_REQ endGroupEnumReq;
    PLSA_IPC_ERROR pError = NULL;

    LWMsgMessage request = {-1, NULL};
    LWMsgMessage response = {-1, NULL};

    endGroupEnumReq.Handle = (LsaIpcServerHandle*)pContext->hServer;
    endGroupEnumReq.pszToken = pInfo->pszGUID;

    request.tag = LSA_Q_END_ENUM_GROUPS;
    request.object = &endGroupEnumReq;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_send_message_transact(
                              pContext->pAssoc,
                              &request,
                              &response));
    BAIL_ON_LSA_ERROR(dwError);

    switch (response.tag)
    {
        case LSA_R_END_ENUM_GROUPS_SUCCESS:
            // response.object == NULL
            break;
        case LSA_R_END_ENUM_GROUPS_FAILURE:
            pError = (PLSA_IPC_ERROR) response.object;
            dwError = pError->dwError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = EINVAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    return dwError;

error:
    if (response.object)
    {
        lwmsg_assoc_free_message(pContext->pAssoc, &response);
    }

    goto cleanup;
}

DWORD
LsaTransactAddGroup(
    HANDLE hServer,
    PVOID  pGroupInfo,
    DWORD  dwGroupInfoLevel
    )
{
    DWORD dwError = 0;
    PLSA_CLIENT_CONNECTION_CONTEXT pContext =
                     (PLSA_CLIENT_CONNECTION_CONTEXT)hServer;
    LSA_IPC_ADD_GROUP_INFO_REQ addGroupinfoReq;
    PLSA_IPC_ERROR pError = NULL;

    LWMsgMessage request = {-1, NULL};
    LWMsgMessage response = {-1, NULL};

    addGroupinfoReq.Handle = (LsaIpcServerHandle*)pContext->hServer;

    dwError = LsaAllocateMemory(
                    sizeof(*(addGroupinfoReq.pGroupInfoList)),
                    (PVOID*)&addGroupinfoReq.pGroupInfoList);
    BAIL_ON_LSA_ERROR(dwError);

    addGroupinfoReq.pGroupInfoList->dwGroupInfoLevel = dwGroupInfoLevel;
    addGroupinfoReq.pGroupInfoList->dwNumGroups = 1;

    switch (dwGroupInfoLevel)
    {
        case 0:
            addGroupinfoReq.pGroupInfoList->ppGroupInfoList.ppInfoList0 = (PLSA_GROUP_INFO_0*)&pGroupInfo;
            break;
        case 1:
            addGroupinfoReq.pGroupInfoList->ppGroupInfoList.ppInfoList1 = (PLSA_GROUP_INFO_1*)&pGroupInfo;
            break;
        default:
            dwError = LSA_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
    }

    request.tag = LSA_Q_ADD_GROUP;
    request.object = &addGroupinfoReq;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_send_message_transact(
                              pContext->pAssoc,
                              &request,
                              &response));
    BAIL_ON_LSA_ERROR(dwError);

    switch (response.tag)
    {
        case LSA_R_ADD_GROUP_SUCCESS:
            // response.object == NULL
            break;
        case LSA_R_ADD_GROUP_FAILURE:
            pError = (PLSA_IPC_ERROR) response.object;
            dwError = pError->dwError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = EINVAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    return dwError;

error:
    if (response.object)
    {
        lwmsg_assoc_free_message(pContext->pAssoc, &response);
    }

    goto cleanup;
}

DWORD
LsaTransactDeleteGroupById(
    HANDLE hServer,
    gid_t  gid
    )
{
    DWORD dwError = 0;
    PLSA_CLIENT_CONNECTION_CONTEXT pContext =
                     (PLSA_CLIENT_CONNECTION_CONTEXT)hServer;
    LSA_IPC_DEL_OBJECT_INFO_REQ delGroupinfoReq;
    PLSA_IPC_ERROR pError = NULL;

    LWMsgMessage request = {-1, NULL};
    LWMsgMessage response = {-1, NULL};

    delGroupinfoReq.Handle = (LsaIpcServerHandle*)pContext->hServer;
    delGroupinfoReq.dwId = gid;

    request.tag = LSA_Q_DELETE_GROUP;
    request.object = &delGroupinfoReq;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_send_message_transact(
                              pContext->pAssoc,
                              &request,
                              &response));
    BAIL_ON_LSA_ERROR(dwError);

    switch (response.tag)
    {
        case LSA_R_DELETE_GROUP_SUCCESS:
            // response.object == NULL
            break;
        case LSA_R_DELETE_GROUP_FAILURE:
            pError = (PLSA_IPC_ERROR) response.object;
            dwError = pError->dwError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = EINVAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    return dwError;

error:
    if (response.object)
    {
        lwmsg_assoc_free_message(pContext->pAssoc, &response);
    }

    goto cleanup;
}

DWORD
LsaTransactGetGroupsForUserById(
    HANDLE  hServer,
    uid_t   uid,
    LSA_FIND_FLAGS FindFlags,
    DWORD   dwGroupInfoLevel,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
    )
{
    DWORD dwError = 0;
    PLSA_CLIENT_CONNECTION_CONTEXT pContext =
                     (PLSA_CLIENT_CONNECTION_CONTEXT)hServer;

    LSA_IPC_FIND_OBJECT_BY_ID_REQ userGroupsReq;
    // Do not free pResultList and pError
    PLSA_GROUP_INFO_LIST pResultList = NULL;
    PLSA_IPC_ERROR pError = NULL;

    LWMsgMessage request = {-1, NULL};
    LWMsgMessage response = {-1, NULL};

    userGroupsReq.Handle = (LsaIpcServerHandle*)pContext->hServer;
    userGroupsReq.FindFlags = FindFlags;
    userGroupsReq.dwInfoLevel = dwGroupInfoLevel;
    userGroupsReq.id = uid;

    request.tag = LSA_Q_GROUPS_FOR_USER;
    request.object = &userGroupsReq;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_send_message_transact(
                              pContext->pAssoc,
                              &request,
                              &response));
    BAIL_ON_LSA_ERROR(dwError);

    switch (response.tag)
    {
        case LSA_R_GROUPS_FOR_USER_SUCCESS:
            pResultList = (PLSA_GROUP_INFO_LIST)response.object;
            *pdwGroupsFound = pResultList->dwNumGroups;
            switch (pResultList->dwGroupInfoLevel)
            {
                case 0:
                    *pppGroupInfoList = (PVOID*)pResultList->ppGroupInfoList.ppInfoList0;
                    pResultList->ppGroupInfoList.ppInfoList0 = NULL;
                    pResultList->dwNumGroups = 0;
                    break;
                case 1:
                    *pppGroupInfoList = (PVOID*)pResultList->ppGroupInfoList.ppInfoList1;
                    pResultList->ppGroupInfoList.ppInfoList1 = NULL;
                    pResultList->dwNumGroups = 0;
                    break;
                default:
                   dwError = LSA_ERROR_INVALID_PARAMETER;
                   BAIL_ON_LSA_ERROR(dwError);
            }
            break;
        case LSA_R_GROUPS_FOR_USER_FAILURE:
            pError = (PLSA_IPC_ERROR) response.object;
            dwError = pError->dwError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = EINVAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    if (response.object)
    {
        lwmsg_assoc_free_message(pContext->pAssoc, &response);
    }

    return dwError;

error:
    *pdwGroupsFound = 0;
    *pppGroupInfoList = NULL;

    goto cleanup;
}

DWORD
LsaTransactFindUserByName(
    HANDLE hServer,
    PCSTR  pszName,
    DWORD  dwUserInfoLevel,
    PVOID* ppUserInfo
    )
{
    DWORD dwError = 0;
    PLSA_CLIENT_CONNECTION_CONTEXT pContext =
                     (PLSA_CLIENT_CONNECTION_CONTEXT)hServer;
    LSA_IPC_FIND_OBJECT_BY_NAME_REQ findObjectByNameReq;
    // Do not free pResultList and pError
    PLSA_USER_INFO_LIST pResultList = NULL;
    PLSA_IPC_ERROR pError = NULL;

    LWMsgMessage request = {-1, NULL};
    LWMsgMessage response = {-1, NULL};

    findObjectByNameReq.Handle = (LsaIpcServerHandle*)pContext->hServer;
    findObjectByNameReq.dwInfoLevel = dwUserInfoLevel;
    findObjectByNameReq.pszName = pszName;

    request.tag = LSA_Q_USER_BY_NAME;
    request.object = &findObjectByNameReq;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_send_message_transact(
                              pContext->pAssoc,
                              &request,
                              &response));
    BAIL_ON_LSA_ERROR(dwError);

    switch (response.tag)
    {
        case LSA_R_USER_BY_NAME_SUCCESS:
            pResultList = (PLSA_USER_INFO_LIST)response.object;

            if (pResultList->dwNumUsers != 1)
            {
                dwError = LSA_ERROR_INTERNAL;
                BAIL_ON_LSA_ERROR(dwError);
            }

            switch (pResultList->dwUserInfoLevel)
            {
                case 0:
                    *ppUserInfo = pResultList->ppUserInfoList.ppInfoList0[0];
                    pResultList->ppUserInfoList.ppInfoList0[0] = NULL;
                    pResultList->dwNumUsers = 0;
                    break;
                case 1:
                    *ppUserInfo = pResultList->ppUserInfoList.ppInfoList1[0];
                    pResultList->ppUserInfoList.ppInfoList1[0] = NULL;
                    pResultList->dwNumUsers = 0;
                    break;
                case 2:
                    *ppUserInfo = pResultList->ppUserInfoList.ppInfoList2[0];
                    pResultList->ppUserInfoList.ppInfoList2[0] = NULL;
                    pResultList->dwNumUsers = 0;
                    break;
                default:
                    dwError = LSA_ERROR_INVALID_PARAMETER;
                    BAIL_ON_LSA_ERROR(dwError);
            }
            break;
        case LSA_R_USER_BY_NAME_FAILURE:
            pError = (PLSA_IPC_ERROR) response.object;
            dwError = pError->dwError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = EINVAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    if (response.object)
    {
        lwmsg_assoc_free_message(pContext->pAssoc, &response);
    }

    return dwError;

error:
    *ppUserInfo = NULL;

    goto cleanup;
}

DWORD
LsaTransactFindUserById(
    HANDLE hServer,
    uid_t uid,
    DWORD  dwUserInfoLevel,
    PVOID* ppUserInfo
    )
{
    DWORD dwError = 0;
    PLSA_CLIENT_CONNECTION_CONTEXT pContext =
                     (PLSA_CLIENT_CONNECTION_CONTEXT)hServer;
    LSA_IPC_FIND_OBJECT_BY_ID_REQ findObjectByIdReq;
    // Do not free pResultList and pError
    PLSA_USER_INFO_LIST pResultList = NULL;
    PLSA_IPC_ERROR pError = NULL;

    LWMsgMessage request = {-1, NULL};
    LWMsgMessage response = {-1, NULL};

    findObjectByIdReq.Handle = (LsaIpcServerHandle*)pContext->hServer;
    findObjectByIdReq.dwInfoLevel = dwUserInfoLevel;
    findObjectByIdReq.id = uid;

    request.tag = LSA_Q_USER_BY_ID;
    request.object = &findObjectByIdReq;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_send_message_transact(
                              pContext->pAssoc,
                              &request,
                              &response));
    BAIL_ON_LSA_ERROR(dwError);

    switch (response.tag)
    {
        case LSA_R_USER_BY_ID_SUCCESS:
            pResultList = (PLSA_USER_INFO_LIST)response.object;

            if (pResultList->dwNumUsers != 1)
            {
                dwError = LSA_ERROR_INTERNAL;
                BAIL_ON_LSA_ERROR(dwError);
            }

            switch (pResultList->dwUserInfoLevel)
            {
                case 0:
                    *ppUserInfo = pResultList->ppUserInfoList.ppInfoList0[0];
                    pResultList->ppUserInfoList.ppInfoList0[0] = NULL;
                    pResultList->dwNumUsers = 0;
                    break;
                case 1:
                    *ppUserInfo = pResultList->ppUserInfoList.ppInfoList1[0];
                    pResultList->ppUserInfoList.ppInfoList1[0] = NULL;
                    pResultList->dwNumUsers = 0;
                    break;
                case 2:
                    *ppUserInfo = pResultList->ppUserInfoList.ppInfoList2[0];
                    pResultList->ppUserInfoList.ppInfoList2[0] = NULL;
                    pResultList->dwNumUsers = 0;
                    break;
                default:
                    dwError = LSA_ERROR_INVALID_PARAMETER;
                    BAIL_ON_LSA_ERROR(dwError);
            }
            break;
        case LSA_R_USER_BY_ID_FAILURE:
            pError = (PLSA_IPC_ERROR) response.object;
            dwError = pError->dwError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = EINVAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    if (response.object)
    {
        lwmsg_assoc_free_message(pContext->pAssoc, &response);
    }

    return dwError;

error:
    *ppUserInfo = NULL;

    goto cleanup;
}

DWORD
LsaTransactBeginEnumUsers(
    HANDLE hServer,
    DWORD   dwUserInfoLevel,
    DWORD   dwMaxNumUsers,
    PHANDLE phResume
    )
{
    DWORD dwError = 0;
    PLSA_CLIENT_CONNECTION_CONTEXT pContext =
                     (PLSA_CLIENT_CONNECTION_CONTEXT)hServer;
    LSA_IPC_BEGIN_ENUM_RECORDS_REQ beginUserEnumReq;
    // Do not free pResult and pError
    PLSA_ENUM_OBJECTS_INFO pResult = NULL;
    PLSA_IPC_ERROR pError = NULL;

    LWMsgMessage request = {-1, NULL};
    LWMsgMessage response = {-1, NULL};

    beginUserEnumReq.Handle = (LsaIpcServerHandle*)pContext->hServer;
    beginUserEnumReq.dwInfoLevel = dwUserInfoLevel;
    beginUserEnumReq.dwNumMaxRecords = dwMaxNumUsers;

    request.tag = LSA_Q_BEGIN_ENUM_USERS;
    request.object = &beginUserEnumReq;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_send_message_transact(
                              pContext->pAssoc,
                              &request,
                              &response));
    BAIL_ON_LSA_ERROR(dwError);

    switch (response.tag)
    {
        case LSA_R_BEGIN_ENUM_USERS_SUCCESS:
            pResult = (PLSA_ENUM_OBJECTS_INFO)response.object;
            *phResume = (HANDLE)pResult;
            break;
        case LSA_R_BEGIN_ENUM_USERS_FAILURE:
            pError = (PLSA_IPC_ERROR) response.object;
            dwError = pError->dwError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = EINVAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    return dwError;

error:
    if (response.object)
    {
        lwmsg_assoc_free_message(pContext->pAssoc, &response);
    }
    *phResume = (HANDLE)NULL;

    goto cleanup;

}

DWORD
LsaTransactEnumUsers(
    HANDLE hServer,
    HANDLE hResume,
    PDWORD pdwNumUsersFound,
    PVOID** pppUserInfoList)
{
    DWORD dwError = 0;
    PLSA_CLIENT_CONNECTION_CONTEXT pContext =
                     (PLSA_CLIENT_CONNECTION_CONTEXT)hServer;
    PLSA_ENUM_OBJECTS_INFO pInfo = (PLSA_ENUM_OBJECTS_INFO)hResume;

    LSA_IPC_ENUM_RECORDS_REQ userEnumReq;
    // Do not free pResultList and pError
    PLSA_USER_INFO_LIST pResultList = NULL;
    PLSA_IPC_ERROR pError = NULL;

    LWMsgMessage request = {-1, NULL};
    LWMsgMessage response = {-1, NULL};

    userEnumReq.Handle = (LsaIpcServerHandle*)pContext->hServer;
    userEnumReq.pszToken = pInfo->pszGUID;

    request.tag = LSA_Q_ENUM_USERS;
    request.object = &userEnumReq;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_send_message_transact(
                              pContext->pAssoc,
                              &request,
                              &response));
    BAIL_ON_LSA_ERROR(dwError);

    switch (response.tag)
    {
        case LSA_R_ENUM_USERS_SUCCESS:
            pResultList = (PLSA_USER_INFO_LIST)response.object;
            *pdwNumUsersFound = pResultList->dwNumUsers;
            switch (pResultList->dwUserInfoLevel)
            {
                case 0:
                    *pppUserInfoList = (PVOID*)pResultList->ppUserInfoList.ppInfoList0;
                    pResultList->ppUserInfoList.ppInfoList0 = NULL;
                    pResultList->dwNumUsers = 0;
                    break;
                case 1:
                    *pppUserInfoList = (PVOID*)pResultList->ppUserInfoList.ppInfoList1;
                    pResultList->ppUserInfoList.ppInfoList1 = NULL;
                    pResultList->dwNumUsers = 0;
                    break;
                case 2:
                    *pppUserInfoList = (PVOID*)pResultList->ppUserInfoList.ppInfoList2;
                    pResultList->ppUserInfoList.ppInfoList2 = NULL;
                    pResultList->dwNumUsers = 0;
                    break;
                default:
                   dwError = LSA_ERROR_INVALID_PARAMETER;
                   BAIL_ON_LSA_ERROR(dwError);
            }
            break;
        case LSA_R_ENUM_USERS_FAILURE:
            pError = (PLSA_IPC_ERROR) response.object;
            dwError = pError->dwError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = EINVAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    if (response.object)
    {
        lwmsg_assoc_free_message(pContext->pAssoc, &response);
    }

    return dwError;

error:
    *pdwNumUsersFound = 0;
    *pppUserInfoList = NULL;

    goto cleanup;
}

DWORD
LsaTransactEndEnumUsers(
    HANDLE hServer,
    HANDLE hResume
    )
{
    DWORD dwError = 0;
    PLSA_CLIENT_CONNECTION_CONTEXT pContext =
                     (PLSA_CLIENT_CONNECTION_CONTEXT)hServer;
    PLSA_ENUM_OBJECTS_INFO pInfo = (PLSA_ENUM_OBJECTS_INFO)hResume;
    LSA_IPC_ENUM_RECORDS_REQ endUserEnumReq;
    PLSA_IPC_ERROR pError = NULL;

    LWMsgMessage request = {-1, NULL};
    LWMsgMessage response = {-1, NULL};

    endUserEnumReq.Handle = (LsaIpcServerHandle*)pContext->hServer;
    endUserEnumReq.pszToken = pInfo->pszGUID;

    request.tag = LSA_Q_END_ENUM_USERS;
    request.object = &endUserEnumReq;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_send_message_transact(
                              pContext->pAssoc,
                              &request,
                              &response));
    BAIL_ON_LSA_ERROR(dwError);

    switch (response.tag)
    {
        case LSA_R_END_ENUM_USERS_SUCCESS:
            // response.object == NULL
            break;
        case LSA_R_END_ENUM_USERS_FAILURE:
            pError = (PLSA_IPC_ERROR) response.object;
            dwError = pError->dwError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = EINVAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    return dwError;

error:
    if (response.object)
    {
        lwmsg_assoc_free_message(pContext->pAssoc, &response);
    }

    goto cleanup;
}

DWORD
LsaTransactAddUser(
    HANDLE hServer,
    PVOID  pUserInfo,
    DWORD  dwUserInfoLevel
    )
{
    DWORD dwError = 0;
    PLSA_CLIENT_CONNECTION_CONTEXT pContext =
                     (PLSA_CLIENT_CONNECTION_CONTEXT)hServer;
    LSA_IPC_ADD_USER_INFO_REQ addUserinfoReq;
    PLSA_IPC_ERROR pError = NULL;

    LWMsgMessage request = {-1, NULL};
    LWMsgMessage response = {-1, NULL};

    addUserinfoReq.Handle = (LsaIpcServerHandle*)pContext->hServer;

    dwError = LsaAllocateMemory(
                    sizeof(*(addUserinfoReq.pUserInfoList)),
                    (PVOID*)&addUserinfoReq.pUserInfoList);
    BAIL_ON_LSA_ERROR(dwError);

    addUserinfoReq.pUserInfoList->dwUserInfoLevel = dwUserInfoLevel;
    addUserinfoReq.pUserInfoList->dwNumUsers = 1;

    switch (dwUserInfoLevel)
    {
        case 0:
            addUserinfoReq.pUserInfoList->ppUserInfoList.ppInfoList0 = (PLSA_USER_INFO_0*)&pUserInfo;
            break;
        case 1:
            addUserinfoReq.pUserInfoList->ppUserInfoList.ppInfoList1 = (PLSA_USER_INFO_1*)&pUserInfo;
            break;
        case 2:
            addUserinfoReq.pUserInfoList->ppUserInfoList.ppInfoList2 = (PLSA_USER_INFO_2*)&pUserInfo;
            break;
        default:
            dwError = LSA_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
    }

    request.tag = LSA_Q_ADD_USER;
    request.object = &addUserinfoReq;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_send_message_transact(
                              pContext->pAssoc,
                              &request,
                              &response));
    BAIL_ON_LSA_ERROR(dwError);

    switch (response.tag)
    {
        case LSA_R_ADD_USER_SUCCESS:
            // response.object == NULL
            break;
        case LSA_R_ADD_USER_FAILURE:
            pError = (PLSA_IPC_ERROR) response.object;
            dwError = pError->dwError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = EINVAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    return dwError;

error:
    if (response.object)
    {
        lwmsg_assoc_free_message(pContext->pAssoc, &response);
    }

    goto cleanup;
}

DWORD
LsaTransactDeleteUserById(
    HANDLE hServer,
    uid_t  uid
    )
{
    DWORD dwError = 0;
    PLSA_CLIENT_CONNECTION_CONTEXT pContext =
                     (PLSA_CLIENT_CONNECTION_CONTEXT)hServer;
    LSA_IPC_DEL_OBJECT_INFO_REQ delUserinfoReq;
    PLSA_IPC_ERROR pError = NULL;

    LWMsgMessage request = {-1, NULL};
    LWMsgMessage response = {-1, NULL};

    delUserinfoReq.Handle = (LsaIpcServerHandle*)pContext->hServer;
    delUserinfoReq.dwId = uid;

    request.tag = LSA_Q_DELETE_USER;
    request.object = &delUserinfoReq;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_send_message_transact(
                              pContext->pAssoc,
                              &request,
                              &response));
    BAIL_ON_LSA_ERROR(dwError);

    switch (response.tag)
    {
        case LSA_R_DELETE_USER_SUCCESS:
            // response.object == NULL
            break;
        case LSA_R_DELETE_USER_FAILURE:
            pError = (PLSA_IPC_ERROR) response.object;
            dwError = pError->dwError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = EINVAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    return dwError;

error:
    if (response.object)
    {
        lwmsg_assoc_free_message(pContext->pAssoc, &response);
    }

    goto cleanup;
}

DWORD
LsaTransactAuthenticateUser(
    HANDLE hServer,
    PCSTR  pszLoginName,
    PCSTR  pszPassword
    )
{
    DWORD dwError = 0;
    PLSA_CLIENT_CONNECTION_CONTEXT pContext =
                     (PLSA_CLIENT_CONNECTION_CONTEXT)hServer;
    LSA_IPC_AUTH_USER_REQ authUserReq;
    PLSA_IPC_ERROR pError = NULL;

    LWMsgMessage request = {-1, NULL};
    LWMsgMessage response = {-1, NULL};

    authUserReq.Handle = (LsaIpcServerHandle*)pContext->hServer;
    authUserReq.pszLoginName = pszLoginName;
    authUserReq.pszPassword = pszPassword;

    request.tag = LSA_Q_AUTH_USER;
    request.object = &authUserReq;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_send_message_transact(
                              pContext->pAssoc,
                              &request,
                              &response));
    BAIL_ON_LSA_ERROR(dwError);

    switch (response.tag)
    {
        case LSA_R_AUTH_USER_SUCCESS:
            // response.object == NULL
            break;
        case LSA_R_AUTH_USER_FAILURE:
            pError = (PLSA_IPC_ERROR) response.object;
            dwError = pError->dwError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = EINVAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    return dwError;

error:
    if (response.object)
    {
        lwmsg_assoc_free_message(pContext->pAssoc, &response);
    }

    goto cleanup;
}

DWORD
LsaTransactAuthenticateUserEx(
    IN HANDLE hServer,
    IN LSA_AUTH_USER_PARAMS* pParams,
    //pUserInfo is allocated by the caller
    OUT LSA_AUTH_USER_INFO* pUserInfo
    )
{
    DWORD dwError = 0;
    PLSA_CLIENT_CONNECTION_CONTEXT pContext =
                     (PLSA_CLIENT_CONNECTION_CONTEXT)hServer;
    LSA_IPC_AUTH_USER_EX_REQ authUserExReq;
    PLSA_AUTH_USER_INFO pResult = NULL;
    PLSA_IPC_ERROR pError = NULL;

    LWMsgMessage request = {-1, NULL};
    LWMsgMessage response = {-1, NULL};

    authUserExReq.Handle = (LsaIpcServerHandle*)pContext->hServer;
    authUserExReq.pParams = pParams;

    request.tag = LSA_Q_AUTH_USER_EX;
    request.object = &authUserExReq;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_send_message_transact(
                              pContext->pAssoc,
                              &request,
                              &response));
    BAIL_ON_LSA_ERROR(dwError);

    switch (response.tag)
    {
        case LSA_R_AUTH_USER_EX_SUCCESS:
            pResult = (PLSA_AUTH_USER_INFO) response.object;
            pUserInfo->dwDummy = pResult->dwDummy;
            break;

        case LSA_R_AUTH_USER_EX_FAILURE:
            pError = (PLSA_IPC_ERROR) response.object;
            dwError = pError->dwError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = EINVAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    return dwError;

error:
    if (response.object)
    {
        lwmsg_assoc_free_message(pContext->pAssoc, &response);
    }

    goto cleanup;
}

DWORD
LsaTransactValidateUser(
    HANDLE hServer,
    PCSTR  pszLoginName,
    PCSTR  pszPassword
    )
{
    DWORD dwError = 0;
    PLSA_CLIENT_CONNECTION_CONTEXT pContext =
                     (PLSA_CLIENT_CONNECTION_CONTEXT)hServer;
    LSA_IPC_AUTH_USER_REQ validateUserReq;
    PLSA_IPC_ERROR pError = NULL;

    LWMsgMessage request = {-1, NULL};
    LWMsgMessage response = {-1, NULL};

    validateUserReq.Handle = (LsaIpcServerHandle*)pContext->hServer;
    validateUserReq.pszLoginName = pszLoginName;
    validateUserReq.pszPassword = pszPassword;

    request.tag = LSA_Q_VALIDATE_USER;
    request.object = &validateUserReq;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_send_message_transact(
                              pContext->pAssoc,
                              &request,
                              &response));
    BAIL_ON_LSA_ERROR(dwError);

    switch (response.tag)
    {
        case LSA_R_VALIDATE_USER_SUCCESS:
            // response.object == NULL
            break;
        case LSA_R_VALIDATE_USER_FAILURE:
            pError = (PLSA_IPC_ERROR) response.object;
            dwError = pError->dwError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = EINVAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    return dwError;

error:
    if (response.object)
    {
        lwmsg_assoc_free_message(pContext->pAssoc, &response);
    }

    goto cleanup;
}

DWORD
LsaTransactChangePassword(
    HANDLE hServer,
    PCSTR  pszLoginName,
    PCSTR  pszOldPassword,
    PCSTR  pszNewPassword
    )
{
    DWORD dwError = 0;
    PLSA_CLIENT_CONNECTION_CONTEXT pContext =
                     (PLSA_CLIENT_CONNECTION_CONTEXT)hServer;
    LSA_IPC_CHANGE_PASSWORD_REQ changePasswordReq;
    PLSA_IPC_ERROR pError = NULL;

    LWMsgMessage request = {-1, NULL};
    LWMsgMessage response = {-1, NULL};

    changePasswordReq.Handle = (LsaIpcServerHandle*)pContext->hServer;
    changePasswordReq.pszLoginName = pszLoginName;
    changePasswordReq.pszOldPassword = pszOldPassword;
    changePasswordReq.pszNewPassword = pszNewPassword;

    request.tag = LSA_Q_CHANGE_PASSWORD;
    request.object = &changePasswordReq;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_send_message_transact(
                              pContext->pAssoc,
                              &request,
                              &response));
    BAIL_ON_LSA_ERROR(dwError);

    switch (response.tag)
    {
        case LSA_R_CHANGE_PASSWORD_SUCCESS:
            // response.object == NULL
            break;
        case LSA_R_CHANGE_PASSWORD_FAILURE:
            pError = (PLSA_IPC_ERROR) response.object;
            dwError = pError->dwError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = EINVAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    return dwError;

error:
    if (response.object)
    {
        lwmsg_assoc_free_message(pContext->pAssoc, &response);
    }

    goto cleanup;
}

DWORD
LsaTransactOpenSession(
    HANDLE hServer,
    PCSTR pszLoginId
    )
{
    DWORD dwError = 0;
    PLSA_CLIENT_CONNECTION_CONTEXT pContext =
                     (PLSA_CLIENT_CONNECTION_CONTEXT)hServer;
    LSA_IPC_OPEN_OR_CLOSE_SESSION_REQ openSessionReq;
    PLSA_IPC_ERROR pError = NULL;

    LWMsgMessage request = {-1, NULL};
    LWMsgMessage response = {-1, NULL};

    openSessionReq.Handle = (LsaIpcServerHandle*)pContext->hServer;
    openSessionReq.pszLoginId = pszLoginId;

    request.tag = LSA_Q_OPEN_SESSION;
    request.object = &openSessionReq;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_send_message_transact(
                              pContext->pAssoc,
                              &request,
                              &response));
    BAIL_ON_LSA_ERROR(dwError);

    switch (response.tag)
    {
        case LSA_R_OPEN_SESSION_SUCCESS:
            // response.object == NULL
            break;
        case LSA_R_OPEN_SESSION_FAILURE:
            pError = (PLSA_IPC_ERROR) response.object;
            dwError = pError->dwError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = EINVAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    return dwError;

error:
    if (response.object)
    {
        lwmsg_assoc_free_message(pContext->pAssoc, &response);
    }

    goto cleanup;
}

DWORD
LsaTransactCloseSession(
    HANDLE hServer,
    PCSTR pszLoginId
    )
{
    DWORD dwError = 0;
    PLSA_CLIENT_CONNECTION_CONTEXT pContext =
                     (PLSA_CLIENT_CONNECTION_CONTEXT)hServer;
    LSA_IPC_OPEN_OR_CLOSE_SESSION_REQ closeSessionReq;
    PLSA_IPC_ERROR pError = NULL;

    LWMsgMessage request = {-1, NULL};
    LWMsgMessage response = {-1, NULL};

    closeSessionReq.Handle = (LsaIpcServerHandle*)pContext->hServer;
    closeSessionReq.pszLoginId = pszLoginId;

    request.tag = LSA_Q_CLOSE_SESSION;
    request.object = &closeSessionReq;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_send_message_transact(
                              pContext->pAssoc,
                              &request,
                              &response));
    BAIL_ON_LSA_ERROR(dwError);

    switch (response.tag)
    {
        case LSA_R_CLOSE_SESSION_SUCCESS:
            // response.object == NULL
            break;
        case LSA_R_CLOSE_SESSION_FAILURE:
            pError = (PLSA_IPC_ERROR) response.object;
            dwError = pError->dwError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = EINVAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    return dwError;

error:
    if (response.object)
    {
        lwmsg_assoc_free_message(pContext->pAssoc, &response);
    }

    goto cleanup;
}

DWORD
LsaTransactModifyUser(
    HANDLE hServer,
    PLSA_USER_MOD_INFO pUserModInfo
    )
{
    DWORD dwError = 0;
    PLSA_CLIENT_CONNECTION_CONTEXT pContext =
                     (PLSA_CLIENT_CONNECTION_CONTEXT)hServer;
    LSA_IPC_MOD_USER_INFO_REQ modUserReq;
    PLSA_IPC_ERROR pError = NULL;

    LWMsgMessage request = {-1, NULL};
    LWMsgMessage response = {-1, NULL};

    modUserReq.Handle = (LsaIpcServerHandle*)pContext->hServer;
    modUserReq.pUserModInfo = pUserModInfo;

    request.tag = LSA_Q_MODIFY_USER;
    request.object = &modUserReq;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_send_message_transact(
                              pContext->pAssoc,
                              &request,
                              &response));
    BAIL_ON_LSA_ERROR(dwError);

    switch (response.tag)
    {
        case LSA_R_MODIFY_USER_SUCCESS:
            // response.object == NULL
            break;
        case LSA_R_MODIFY_USER_FAILURE:
            pError = (PLSA_IPC_ERROR) response.object;
            dwError = pError->dwError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = EINVAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    return dwError;

error:
    if (response.object)
    {
        lwmsg_assoc_free_message(pContext->pAssoc, &response);
    }

    goto cleanup;
}

DWORD
LsaTransactGetNamesBySidList(
    IN HANDLE hServer,
    IN size_t sCount,
    IN PSTR* ppszSidList,
    OUT PLSA_SID_INFO* ppSIDInfoList,
    OUT OPTIONAL CHAR *pchDomainSeparator
    )
{
    DWORD dwError = 0;
    PLSA_CLIENT_CONNECTION_CONTEXT pContext =
                     (PLSA_CLIENT_CONNECTION_CONTEXT)hServer;
    LSA_IPC_NAMES_BY_SIDS_REQ getNamesBySidsReq;
    PLSA_FIND_NAMES_BY_SIDS pResult = NULL;
    PLSA_IPC_ERROR pError = NULL;

    LWMsgMessage request = {-1, NULL};
    LWMsgMessage response = {-1, NULL};

    getNamesBySidsReq.Handle = (LsaIpcServerHandle*)pContext->hServer;
    getNamesBySidsReq.sCount = sCount;
    getNamesBySidsReq.ppszSidList = ppszSidList;

    request.tag = LSA_Q_NAMES_BY_SID_LIST;
    request.object = &getNamesBySidsReq;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_send_message_transact(
                              pContext->pAssoc,
                              &request,
                              &response));
    BAIL_ON_LSA_ERROR(dwError);

    switch (response.tag)
    {
        case LSA_R_NAMES_BY_SID_LIST_SUCCESS:
            pResult = (PLSA_FIND_NAMES_BY_SIDS)response.object;
            *ppSIDInfoList = pResult->pSIDInfoList;
            pResult->pSIDInfoList = NULL;
            if (pchDomainSeparator)
            {
                *pchDomainSeparator = pResult->chDomainSeparator;
            }

            break;
        case LSA_R_NAMES_BY_SID_LIST_FAILURE:
            pError = (PLSA_IPC_ERROR) response.object;
            dwError = pError->dwError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = EINVAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    if (response.object)
    {
        lwmsg_assoc_free_message(pContext->pAssoc, &response);
    }
    return dwError;

error:
    goto cleanup;
}

DWORD
LsaTransactOpenServer(
   IN OUT HANDLE hServer
   )
{
    DWORD dwError = 0;
    PLSA_CLIENT_CONNECTION_CONTEXT pContext =
                     (PLSA_CLIENT_CONNECTION_CONTEXT)hServer;
    PLSA_IPC_ERROR pError = NULL;

    LWMsgMessage request = {-1, NULL};
    LWMsgMessage response = {-1, NULL};

    request.tag = LSA_Q_OPEN_SERVER;
    request.object = NULL;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_send_message_transact(
                                  pContext->pAssoc,
                                  &request,
                                  &response));
    BAIL_ON_LSA_ERROR(dwError);

    switch (response.tag)
    {
    case LSA_R_OPEN_SERVER_SUCCESS:
        pContext->hServer = (HANDLE) response.object;
        break;
    case LSA_R_OPEN_SERVER_FAILURE:
        pError = (PLSA_IPC_ERROR) response.object;
        dwError = pError->dwError;
        BAIL_ON_LSA_ERROR(dwError);
        break;
    default:
        dwError = EINVAL;
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    return dwError;

error:
    if (response.object)
    {
        lwmsg_assoc_free_message(pContext->pAssoc, &response);
    }

    goto cleanup;
}
