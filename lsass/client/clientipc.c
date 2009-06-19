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
 *        clientipc.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Inter-process Communication (Client) API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Wei Fu (wfu@likewisesoftware.com)
 */
#include "client.h"

DWORD
LsaOpenServer(
    PHANDLE phConnection
    )
{
    DWORD dwError = 0;
    PLSA_CLIENT_CONNECTION_CONTEXT pContext = NULL;
    static LWMsgTime connectTimeout = {2, 0};

    BAIL_ON_INVALID_POINTER(phConnection);

    dwError = LsaAllocateMemory(sizeof(LSA_CLIENT_CONNECTION_CONTEXT), (PVOID*)&pContext);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_protocol_new(NULL, &pContext->pProtocol));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_protocol_add_protocol_spec(pContext->pProtocol, LsaIPCGetProtocolSpec()));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_connection_new(NULL, pContext->pProtocol, &pContext->pAssoc));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_connection_set_endpoint(
                                  pContext->pAssoc,
                                  LWMSG_CONNECTION_MODE_LOCAL,
                                  CACHEDIR "/" LSA_SERVER_FILENAME));
    BAIL_ON_LSA_ERROR(dwError);

    /* Attempt to automatically restore the connection if the server closes it.
       This allows us to transparently recover from lsassd being restarted
       as long as:

       1. No operation is attempted during the window that lsassd is down
       2. The shutdown did not wipe out any state such as enumeration handles

       lwmsg will handle reconnecting for us if these conditions are met.
    */

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_set_action(
                                  pContext->pAssoc,
                                  LWMSG_STATUS_PEER_RESET,
                                  LWMSG_ASSOC_ACTION_RESET_AND_RETRY));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_set_action(
                                  pContext->pAssoc,
                                  LWMSG_STATUS_PEER_CLOSE,
                                  LWMSG_ASSOC_ACTION_RESET_AND_RETRY));
    BAIL_ON_LSA_ERROR(dwError);

    if (getenv("LW_DISABLE_CONNECT_TIMEOUT") == NULL)
    {
        /* Give up connecting within 2 seconds in case lsassd
           is unresponsive (e.g. it's being traced in a debugger) */
        dwError = MAP_LWMSG_ERROR(lwmsg_assoc_set_timeout(
                                      pContext->pAssoc,
                                      LWMSG_TIMEOUT_ESTABLISH,
                                      &connectTimeout));
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_establish(pContext->pAssoc));
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
    DWORD dwGroupInfoLevel,
    DWORD dwMaxNumGroups,
    BOOLEAN bCheckGroupMembersOnline,
    LSA_FIND_FLAGS FindFlags,
    PHANDLE phResume
    )
{
    DWORD dwError = 0;
    PLSA_CLIENT_CONNECTION_CONTEXT pContext =
                     (PLSA_CLIENT_CONNECTION_CONTEXT)hServer;
    LSA_IPC_BEGIN_ENUM_GROUPS_REQ beginGroupEnumReq;
    // Do not free pResult and pError
    PLSA_IPC_ERROR pError = NULL;

    LWMsgMessage request = {-1, NULL};
    LWMsgMessage response = {-1, NULL};

    beginGroupEnumReq.dwInfoLevel = dwGroupInfoLevel;
    beginGroupEnumReq.dwNumMaxRecords = dwMaxNumGroups;
    beginGroupEnumReq.bCheckGroupMembersOnline = bCheckGroupMembersOnline;
    beginGroupEnumReq.FindFlags = FindFlags;

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
            *phResume = (HANDLE)response.object;
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
    PVOID** pppGroupInfoList
    )
{
    DWORD dwError = 0;
    PLSA_CLIENT_CONNECTION_CONTEXT pContext =
                     (PLSA_CLIENT_CONNECTION_CONTEXT)hServer;

    // Do not free pResultList and pError
    PLSA_GROUP_INFO_LIST pResultList = NULL;
    PLSA_IPC_ERROR pError = NULL;

    LWMsgMessage request = {-1, NULL};
    LWMsgMessage response = {-1, NULL};

    request.tag = LSA_Q_ENUM_GROUPS;
    request.object = hResume;

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
    PLSA_IPC_ERROR pError = NULL;

    LWMsgMessage request = {-1, NULL};
    LWMsgMessage response = {-1, NULL};

    request.tag = LSA_Q_END_ENUM_GROUPS;
    request.object = hResume;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_send_message_transact(
                              pContext->pAssoc,
                              &request,
                              &response));
    BAIL_ON_LSA_ERROR(dwError);

    switch (response.tag)
    {
        case LSA_R_END_ENUM_GROUPS_SUCCESS:
            dwError = MAP_LWMSG_ERROR(lwmsg_assoc_release_handle(
                                          pContext->pAssoc,
                                          hResume));
            BAIL_ON_LSA_ERROR(dwError);
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
    PLSA_IPC_ERROR pError = NULL;
    LSA_GROUP_INFO_LIST addGroupInfoReq;

    LWMsgMessage request = {-1, NULL};
    LWMsgMessage response = {-1, NULL};

    addGroupInfoReq.dwGroupInfoLevel = dwGroupInfoLevel;
    addGroupInfoReq.dwNumGroups = 1;

    switch (dwGroupInfoLevel)
    {
        case 0:
            addGroupInfoReq.ppGroupInfoList.ppInfoList0 = (PLSA_GROUP_INFO_0*)&pGroupInfo;
            break;
        case 1:
            addGroupInfoReq.ppGroupInfoList.ppInfoList1 = (PLSA_GROUP_INFO_1*)&pGroupInfo;
            break;
        default:
            dwError = LSA_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
    }

    request.tag = LSA_Q_ADD_GROUP;
    request.object = &addGroupInfoReq;

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
    PLSA_IPC_ERROR pError = NULL;

    LWMsgMessage request = {-1, NULL};
    LWMsgMessage response = {-1, NULL};

    request.tag = LSA_Q_DELETE_GROUP;
    request.object = &gid;

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
LsaTransactGetGroupsForUser(
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
    PLSA_CLIENT_CONNECTION_CONTEXT pContext =
                     (PLSA_CLIENT_CONNECTION_CONTEXT)hServer;

    LSA_IPC_FIND_OBJECT_REQ userGroupsReq;
    // Do not free pResultList and pError
    PLSA_GROUP_INFO_LIST pResultList = NULL;
    PLSA_IPC_ERROR pError = NULL;

    LWMsgMessage request = {-1, NULL};
    LWMsgMessage response = {-1, NULL};

    userGroupsReq.FindFlags = FindFlags;
    userGroupsReq.dwInfoLevel = dwGroupInfoLevel;

    if (pszUserName)
    {
        userGroupsReq.ByType = LSA_IPC_FIND_OBJECT_BY_TYPE_NAME;
        userGroupsReq.ByData.pszName = pszUserName;
    }
    else
    {
        userGroupsReq.ByType = LSA_IPC_FIND_OBJECT_BY_TYPE_ID;
        userGroupsReq.ByData.dwId = uid;
    }

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

    findObjectByNameReq.dwInfoLevel = dwUserInfoLevel;
    findObjectByNameReq.pszName = pszName;
    findObjectByNameReq.FindFlags = 0;

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
    LSA_FIND_FLAGS FindFlags,
    PHANDLE phResume
    )
{
    DWORD dwError = 0;
    PLSA_CLIENT_CONNECTION_CONTEXT pContext =
                     (PLSA_CLIENT_CONNECTION_CONTEXT)hServer;
    LSA_IPC_BEGIN_ENUM_USERS_REQ beginUserEnumReq;
    // Do not free pResult and pError
    PLSA_IPC_ERROR pError = NULL;

    LWMsgMessage request = {-1, NULL};
    LWMsgMessage response = {-1, NULL};

    beginUserEnumReq.dwInfoLevel = dwUserInfoLevel;
    beginUserEnumReq.dwNumMaxRecords = dwMaxNumUsers;
    beginUserEnumReq.FindFlags = FindFlags;

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
            *phResume = response.object;
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
    PVOID** pppUserInfoList
    )
{
    DWORD dwError = 0;
    PLSA_CLIENT_CONNECTION_CONTEXT pContext =
                     (PLSA_CLIENT_CONNECTION_CONTEXT)hServer;

    // Do not free pResultList and pError
    PLSA_USER_INFO_LIST pResultList = NULL;
    PLSA_IPC_ERROR pError = NULL;

    LWMsgMessage request = {-1, NULL};
    LWMsgMessage response = {-1, NULL};

    request.tag = LSA_Q_ENUM_USERS;
    request.object = hResume;

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
    PLSA_IPC_ERROR pError = NULL;

    LWMsgMessage request = {-1, NULL};
    LWMsgMessage response = {-1, NULL};

    request.tag = LSA_Q_END_ENUM_USERS;
    request.object = hResume;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_send_message_transact(
                              pContext->pAssoc,
                              &request,
                              &response));
    BAIL_ON_LSA_ERROR(dwError);

    switch (response.tag)
    {
        case LSA_R_END_ENUM_USERS_SUCCESS:
            dwError = MAP_LWMSG_ERROR(lwmsg_assoc_release_handle(
                                          pContext->pAssoc,
                                          hResume));
            BAIL_ON_LSA_ERROR(dwError);
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
    LSA_USER_INFO_LIST addUserInfoReq;
    PLSA_IPC_ERROR pError = NULL;

    LWMsgMessage request = {-1, NULL};
    LWMsgMessage response = {-1, NULL};

    addUserInfoReq.dwUserInfoLevel = dwUserInfoLevel;
    addUserInfoReq.dwNumUsers = 1;

    switch (dwUserInfoLevel)
    {
        case 0:
            addUserInfoReq.ppUserInfoList.ppInfoList0 = (PLSA_USER_INFO_0*)&pUserInfo;
            break;
        case 1:
            addUserInfoReq.ppUserInfoList.ppInfoList1 = (PLSA_USER_INFO_1*)&pUserInfo;
            break;
        case 2:
            addUserInfoReq.ppUserInfoList.ppInfoList2 = (PLSA_USER_INFO_2*)&pUserInfo;
            break;
        default:
            dwError = LSA_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
    }

    request.tag = LSA_Q_ADD_USER;
    request.object = &addUserInfoReq;

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
    PLSA_IPC_ERROR pError = NULL;

    LWMsgMessage request = {-1, NULL};
    LWMsgMessage response = {-1, NULL};

    request.tag = LSA_Q_DELETE_USER;
    request.object = &uid;

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
    OUT PLSA_AUTH_USER_INFO* ppUserInfo
    )
{
    DWORD dwError = 0;
    PLSA_CLIENT_CONNECTION_CONTEXT pContext =
                     (PLSA_CLIENT_CONNECTION_CONTEXT)hServer;
    LSA_AUTH_USER_PARAMS authUserExReq;
    PLSA_IPC_ERROR pError = NULL;

    LWMsgMessage request = {-1, NULL};
    LWMsgMessage response = {-1, NULL};

    authUserExReq.AuthType = pParams->AuthType;
    authUserExReq.pass = pParams->pass;
    authUserExReq.pszAccountName = pParams->pszAccountName;
    authUserExReq.pszDomain = pParams->pszDomain;
    authUserExReq.pszWorkstation = pParams->pszWorkstation;

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
            *ppUserInfo = (PLSA_AUTH_USER_INFO) response.object;
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
LsaTransactSetPassword(
    HANDLE hServer,
    PCSTR  pszLoginName,
    PCSTR  pszNewPassword
    )
{
    DWORD dwError = 0;
    PLSA_CLIENT_CONNECTION_CONTEXT pContext =
                     (PLSA_CLIENT_CONNECTION_CONTEXT)hServer;
    LSA_IPC_SET_PASSWORD_REQ setPasswordReq;
    PLSA_IPC_ERROR pError = NULL;

    LWMsgMessage request = {-1, NULL};
    LWMsgMessage response = {-1, NULL};

    setPasswordReq.pszLoginName   = pszLoginName;
    setPasswordReq.pszNewPassword = pszNewPassword;

    request.tag    = LSA_Q_SET_PASSWORD;
    request.object = &setPasswordReq;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_send_message_transact(
                              pContext->pAssoc,
                              &request,
                              &response));
    BAIL_ON_LSA_ERROR(dwError);

    switch (response.tag)
    {
        case LSA_R_SET_PASSWORD_SUCCESS:
            break;

        case LSA_R_SET_PASSWORD_FAILURE:
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
    PLSA_IPC_ERROR pError = NULL;

    LWMsgMessage request = {-1, NULL};
    LWMsgMessage response = {-1, NULL};

    request.tag = LSA_Q_MODIFY_USER;
    request.object = pUserModInfo;

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
LsaTransactModifyGroup(
    HANDLE hServer,
    PLSA_GROUP_MOD_INFO pGroupModInfo
    )
{
    DWORD dwError = 0;
    PLSA_CLIENT_CONNECTION_CONTEXT pContext =
                     (PLSA_CLIENT_CONNECTION_CONTEXT)hServer;
    PLSA_IPC_ERROR pError = NULL;

    LWMsgMessage request = {-1, NULL};
    LWMsgMessage response = {-1, NULL};

    request.tag    = LSA_Q_MODIFY_GROUP;
    request.object = pGroupModInfo;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_send_message_transact(
                              pContext->pAssoc,
                              &request,
                              &response));
    BAIL_ON_LSA_ERROR(dwError);

    switch (response.tag) {
    case LSA_R_MODIFY_GROUP_SUCCESS:
        break;

    case LSA_R_MODIFY_GROUP_FAILURE:
        pError = (PLSA_IPC_ERROR)response.object;
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
    if (response.object) {
        lwmsg_assoc_free_message(pContext->pAssoc, &response);
    }

    goto cleanup;
}

DWORD
LsaTransactProviderIoControl(
    IN HANDLE  hServer,
    IN PCSTR   pszProvider,
    IN DWORD   dwIoControlCode,
    IN DWORD   dwInputBufferSize,
    IN PVOID   pInputBuffer,
    OUT DWORD* pdwOutputBufferSize,
    OUT PVOID* ppOutputBuffer
    )
{
    DWORD dwError = 0;
    PLSA_CLIENT_CONNECTION_CONTEXT pContext =
        (PLSA_CLIENT_CONNECTION_CONTEXT)hServer;
    LSA_IPC_PROVIDER_IO_CONTROL_REQ providerIoControlReq;
    // Do not free pResultBuffer and pError
    PLSA_DATA_BLOB pResultBuffer = NULL;
    PLSA_IPC_ERROR pError = NULL;

    LWMsgMessage request = { -1, NULL};
    LWMsgMessage response = {-1, NULL};

    providerIoControlReq.pszProvider = pszProvider;
    providerIoControlReq.dwIoControlCode = dwIoControlCode;
    providerIoControlReq.dwDataLen = dwInputBufferSize;
    providerIoControlReq.pData = pInputBuffer;

    request.tag = LSA_Q_PROVIDER_IO_CONTROL;
    request.object = &providerIoControlReq;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_send_message_transact(
                              pContext->pAssoc,
                              &request,
                              &response));
    BAIL_ON_LSA_ERROR(dwError);

    switch (response.tag)
    {
        case LSA_R_PROVIDER_IO_CONTROL_SUCCESS:
            *pdwOutputBufferSize = 0;
            *ppOutputBuffer = NULL;
            break;
        case LSA_R_PROVIDER_IO_CONTROL_SUCCESS_DATA:
            pResultBuffer = (PLSA_DATA_BLOB)response.object;
            *pdwOutputBufferSize = pResultBuffer->dwLen;
            *ppOutputBuffer = (PVOID)(pResultBuffer->pData);
            pResultBuffer->pData = NULL;
            break;
        case LSA_R_PROVIDER_IO_CONTROL_FAILURE:
            pError = (PLSA_IPC_ERROR) response.object;
            dwError = pError->dwError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = EINVAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    if ( response.object )
    {
        lwmsg_assoc_free_message(
            pContext->pAssoc,
            &response);
    }

    return dwError;

error:

    *pdwOutputBufferSize = 0;
    *ppOutputBuffer = NULL;

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
