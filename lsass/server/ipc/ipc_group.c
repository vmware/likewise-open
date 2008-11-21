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
 *        ipc_group.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 * 
 *        Inter-process communication (Server) API for Groups
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "ipc.h"

DWORD
LsaSrvIpcAddGroup(
    HANDLE hConnection,
    PLSAMESSAGE pMessage
    )
{
    DWORD dwError = 0;
    PVOID* ppGroupInfoList = NULL;
    DWORD dwGroupInfoLevel = 0;
    DWORD dwNumGroups = 0;
    PLSAMESSAGE pResponse = NULL;
    PLSASERVERCONNECTIONCONTEXT pContext =
        (PLSASERVERCONNECTIONCONTEXT)hConnection;
    HANDLE hServer = (HANDLE)NULL;
    
    dwError = LsaUnmarshalGroupInfoList(
                    pMessage->pData,
                    pMessage->header.messageLength,
                    &dwGroupInfoLevel,
                    &ppGroupInfoList,
                    &dwNumGroups
                    );
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaSrvIpcOpenServer(hConnection, &hServer);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaSrvAddGroup(
                    hServer,
                    dwGroupInfoLevel,
                    *ppGroupInfoList);
    if (!dwError) {
        
       dwError = LsaBuildMessage(
                    LSA_R_ADD_GROUP,
                    0, /* Empty message */
                    1,
                    1,
                    &pResponse
                    );
       BAIL_ON_LSA_ERROR(dwError);
       
    } else {
       
       dwError = LsaSrvIpcMarshalError(
                       dwError,
                       &pResponse
                       );
       BAIL_ON_LSA_ERROR(dwError);
       
    }

    dwError = LsaWriteMessage(pContext->fd, pResponse);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    if (ppGroupInfoList) {
        LsaFreeGroupInfoList(dwGroupInfoLevel, ppGroupInfoList, dwNumGroups);
    }
    
    LSA_SAFE_FREE_MESSAGE(pResponse);
    
    return dwError;
    
error:

    goto cleanup;
}

DWORD
LsaSrvIpcFindGroupByName(
    HANDLE hConnection,
    PLSAMESSAGE pMessage
    )
{
    DWORD dwError = 0;
    PSTR pszGroupName = NULL;
    PVOID pGroupInfo = NULL;
    LSA_FIND_FLAGS FindFlags = 0;
    DWORD dwGroupInfoLevel = 0;
    PLSAMESSAGE pResponse = NULL;
    DWORD dwMsgLen = 0;
    PLSASERVERCONNECTIONCONTEXT pContext =
       (PLSASERVERCONNECTIONCONTEXT)hConnection;
    HANDLE hServer = (HANDLE)NULL;
    
    dwError = LsaUnmarshalFindGroupByNameQuery(
                pMessage->pData,
                pMessage->header.messageLength,
                &pszGroupName,
                &FindFlags,
                &dwGroupInfoLevel
                );
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaSrvIpcOpenServer(hConnection, &hServer);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaSrvFindGroupByName(
                       hServer,
                       pszGroupName,
                       FindFlags,
                       dwGroupInfoLevel,
                       &pGroupInfo);
    if (dwError) {
        dwError = LsaSrvIpcMarshalError(
                        dwError,
                        &pResponse
                        );
        BAIL_ON_LSA_ERROR(dwError);
        
    } else {

       dwError = LsaMarshalGroupInfoList(
                       &pGroupInfo,
                       dwGroupInfoLevel,
                       1,
                       NULL,
                       &dwMsgLen);
       BAIL_ON_LSA_ERROR(dwError);
    
       dwError = LsaBuildMessage(
                        LSA_R_GROUP_BY_NAME,
                        dwMsgLen,
                        1,
                        1,
                        &pResponse
                        );
       BAIL_ON_LSA_ERROR(dwError);
    
       dwError = LsaMarshalGroupInfoList(
                       &pGroupInfo,
                       dwGroupInfoLevel,
                       1,
                       pResponse->pData,
                       &dwMsgLen);
       BAIL_ON_LSA_ERROR(dwError);
    }
    
    dwError = LsaWriteMessage(pContext->fd, pResponse);
    BAIL_ON_LSA_ERROR(dwError);
    
cleanup:

    if (pGroupInfo) {
        LsaFreeGroupInfo(dwGroupInfoLevel, pGroupInfo);
    }
    LSA_SAFE_FREE_STRING(pszGroupName);
    
    LSA_SAFE_FREE_MESSAGE(pResponse);
    
    return dwError;
    
error:

    goto cleanup;
}

DWORD
LsaSrvIpcFindGroupById(
    HANDLE hConnection,
    PLSAMESSAGE pQuery
    )
{
    DWORD dwError = 0;
    gid_t gid = 0;
    PVOID pGroupInfo = NULL;
    LSA_FIND_FLAGS FindFlags = 0;
    DWORD dwGroupInfoLevel = 0;
    PLSAMESSAGE pResponse = NULL;
    DWORD dwMsgLen = 0;
    PLSASERVERCONNECTIONCONTEXT pContext =
         (PLSASERVERCONNECTIONCONTEXT)hConnection;
    HANDLE hServer = (HANDLE)NULL;
    
    dwError = LsaUnmarshalFindGroupByIdQuery(
                 pQuery->pData,
                 pQuery->header.messageLength,
                 &gid,
                 &FindFlags,
                 &dwGroupInfoLevel
                 );
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaSrvIpcOpenServer(hConnection, &hServer);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaSrvFindGroupById(hServer, gid, FindFlags,
                                  dwGroupInfoLevel, &pGroupInfo);
    if (dwError) {
        
       dwError = LsaSrvIpcMarshalError(
                       dwError,
                       &pResponse
                       );
       BAIL_ON_LSA_ERROR(dwError);
       
    } else {
        
       dwError = LsaMarshalGroupInfoList(&pGroupInfo, dwGroupInfoLevel, 1, NULL, &dwMsgLen);
       BAIL_ON_LSA_ERROR(dwError);
    
       dwError = LsaBuildMessage(
                        LSA_R_GROUP_BY_ID,
                        dwMsgLen,
                        1,
                        1,
                        &pResponse
                        );
       BAIL_ON_LSA_ERROR(dwError);
    
       dwError = LsaMarshalGroupInfoList(&pGroupInfo, dwGroupInfoLevel, 1, pResponse->pData, &dwMsgLen);
       BAIL_ON_LSA_ERROR(dwError);
       
    }
    
    dwError = LsaWriteMessage(pContext->fd, pResponse);
    BAIL_ON_LSA_ERROR(dwError);
    
cleanup:

    if (pGroupInfo) {
        LsaFreeGroupInfo(dwGroupInfoLevel, pGroupInfo);
    }
    
    LSA_SAFE_FREE_MESSAGE(pResponse);
    
    return dwError;
    
error:

    goto cleanup;
}

DWORD
LsaSrvIpcGetGroupsForUser(
    HANDLE hConnection,
    PLSAMESSAGE pQuery
    )
{
    DWORD dwError = 0;
    uid_t uid = 0;
    DWORD dwGroupInfoLevel = 0;
    LSA_FIND_FLAGS FindFlags = 0;
    PVOID* ppGroupInfoList = NULL;
    DWORD  dwNumGroupsFound = 0;
    PLSAMESSAGE pResponse = NULL;
    DWORD dwMsgLen = 0;
    PLSASERVERCONNECTIONCONTEXT pContext =
         (PLSASERVERCONNECTIONCONTEXT)hConnection;
    HANDLE hServer = (HANDLE)NULL;
    
    dwError = LsaUnmarshalGetGroupsForUserQuery(
                 pQuery->pData,
                 pQuery->header.messageLength,
                 &uid,
                 &FindFlags,
                 &dwGroupInfoLevel
                 );
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaSrvIpcOpenServer(hConnection, &hServer);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaSrvGetGroupsForUser(hServer, uid, FindFlags, dwGroupInfoLevel, &dwNumGroupsFound, &ppGroupInfoList);
    if (dwError) {
        
       dwError = LsaSrvIpcMarshalError(
                       dwError,
                       &pResponse
                       );
       BAIL_ON_LSA_ERROR(dwError);
       
    } else {
        
       dwError = LsaMarshalGroupInfoList(
                           ppGroupInfoList,
                           dwGroupInfoLevel,
                           dwNumGroupsFound,
                           NULL,
                           &dwMsgLen);
       BAIL_ON_LSA_ERROR(dwError);
    
       dwError = LsaBuildMessage(
                        LSA_R_GROUPS_FOR_USER,
                        dwMsgLen,
                        1,
                        1,
                        &pResponse);
       BAIL_ON_LSA_ERROR(dwError);
    
       dwError = LsaMarshalGroupInfoList(
                        ppGroupInfoList,
                        dwGroupInfoLevel,
                        dwNumGroupsFound,
                        pResponse->pData,
                        &dwMsgLen);
       BAIL_ON_LSA_ERROR(dwError);
    }
    
    dwError = LsaWriteMessage(pContext->fd, pResponse);
    BAIL_ON_LSA_ERROR(dwError);
       
cleanup:

    if (ppGroupInfoList) {
        LsaFreeGroupInfoList(dwGroupInfoLevel, ppGroupInfoList, dwNumGroupsFound);
    }
    
    LSA_SAFE_FREE_MESSAGE(pResponse);
    
    return dwError;
    
error:

    goto cleanup;
}

DWORD
LsaSrvIpcBeginEnumGroups(
    HANDLE hConnection,
    PLSAMESSAGE pMessage
    )
{
    DWORD dwError = 0;
    DWORD dwGroupInfoLevel = 0;
    DWORD dwNumMaxGroups = 0;
    PLSAMESSAGE pResponse = NULL;
    DWORD dwMsgLen = 0;
    PLSASERVERCONNECTIONCONTEXT pContext =
         (PLSASERVERCONNECTIONCONTEXT)hConnection;
    HANDLE hServer = (HANDLE)NULL;
    PSTR   pszGUID = NULL;
    
    dwError = LsaUnmarshalBeginEnumRecordsQuery(
                        pMessage->pData,
                        pMessage->header.messageLength,
                        &dwGroupInfoLevel,
                        &dwNumMaxGroups);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaSrvIpcOpenServer(hConnection, &hServer);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaSrvBeginEnumGroups(
                        hServer,
                        dwGroupInfoLevel,
                        dwNumMaxGroups,
                        &pszGUID);
    BAIL_ON_LSA_ERROR(dwError);
    
    if (dwError) {
        
       dwError = LsaSrvIpcMarshalError(
                       dwError,
                       &pResponse);
       BAIL_ON_LSA_ERROR(dwError);
       
    } else {
        
       dwError = LsaMarshalEnumRecordsToken(
                           pszGUID,
                           NULL,
                           &dwMsgLen);
       BAIL_ON_LSA_ERROR(dwError);
    
       dwError = LsaBuildMessage(
                        LSA_R_BEGIN_ENUM_GROUPS,
                        dwMsgLen,
                        1,
                        1,
                        &pResponse
                        );
       BAIL_ON_LSA_ERROR(dwError);
    
       dwError = LsaMarshalEnumRecordsToken(
                        pszGUID,
                        pResponse->pData,
                        &dwMsgLen);
       BAIL_ON_LSA_ERROR(dwError);
       
    }
    
    dwError = LsaWriteMessage(pContext->fd, pResponse);
    BAIL_ON_LSA_ERROR(dwError);
    
cleanup:
    
    LSA_SAFE_FREE_MESSAGE(pResponse);
    
    LSA_SAFE_FREE_STRING(pszGUID);
    
    return dwError;
    
error:

    goto cleanup;
}

DWORD
LsaSrvIpcEnumGroups(
    HANDLE hConnection,
    PLSAMESSAGE pMessage
)
{
	
    DWORD dwError = 0;
    PVOID* ppGroupInfoList = NULL;
    DWORD  dwGroupInfoLevel = 0;
    DWORD  dwNumGroupsFound = 0;
    PLSAMESSAGE pResponse = NULL;
    DWORD dwMsgLen = 0;
    PLSASERVERCONNECTIONCONTEXT pContext =
         (PLSASERVERCONNECTIONCONTEXT)hConnection;
    HANDLE hServer = (HANDLE)NULL;
    PSTR pszGUID = NULL;
    
    
    dwError = LsaUnmarshalEnumRecordsToken(
                        pMessage->pData,
                        pMessage->header.messageLength,
                        &pszGUID);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaSrvIpcOpenServer(hConnection, &hServer);
    BAIL_ON_LSA_ERROR(dwError);
    
    
    dwError = LsaSrvEnumGroups(
                    hServer,
                    pszGUID,
                    &dwGroupInfoLevel,
                    &ppGroupInfoList,
                    &dwNumGroupsFound);
                

                    
    if (dwError) {
        
       dwError = LsaSrvIpcMarshalError(
                       dwError,
                       &pResponse);
       BAIL_ON_LSA_ERROR(dwError);
       
    } else {
        
       dwError = LsaMarshalGroupInfoList(
                           ppGroupInfoList,
                           dwGroupInfoLevel,
                           dwNumGroupsFound,
                           NULL,
                           &dwMsgLen);
       BAIL_ON_LSA_ERROR(dwError);
    
        
    
       dwError = LsaBuildMessage(
                        LSA_R_ENUM_GROUPS,
                        dwMsgLen,
                        1,
                        1,
                        &pResponse);
       BAIL_ON_LSA_ERROR(dwError);
    
       dwError = LsaMarshalGroupInfoList(
                        ppGroupInfoList,
                        dwGroupInfoLevel,
                        dwNumGroupsFound,
                        pResponse->pData,
                        &dwMsgLen);
       BAIL_ON_LSA_ERROR(dwError);
       
    }
    
    
    dwError = LsaWriteMessage(pContext->fd, pResponse);
    BAIL_ON_LSA_ERROR(dwError);
    
cleanup:

    if (ppGroupInfoList) {
        LsaFreeGroupInfoList(dwGroupInfoLevel, ppGroupInfoList, dwNumGroupsFound);
    }
    
    LSA_SAFE_FREE_MESSAGE(pResponse);
    
    LSA_SAFE_FREE_STRING(pszGUID);
    
    return dwError;
    
error:

    goto cleanup;

}

DWORD
LsaSrvIpcEndEnumGroups(
    HANDLE hConnection,
    PLSAMESSAGE pMessage
    )
{
    DWORD dwError = 0;
    PLSAMESSAGE pResponse = NULL;
    DWORD dwMsgLen = 0;
    PLSASERVERCONNECTIONCONTEXT pContext =
         (PLSASERVERCONNECTIONCONTEXT)hConnection;
    HANDLE hServer = (HANDLE)NULL;
    PSTR   pszGUID = NULL;
    
    dwError = LsaUnmarshalEnumRecordsToken(
                        pMessage->pData,
                        pMessage->header.messageLength,
                        &pszGUID);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaSrvIpcOpenServer(hConnection, &hServer);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaSrvEndEnumGroups(hServer, pszGUID);
    BAIL_ON_LSA_ERROR(dwError);
    
    if (dwError) {
        
       dwError = LsaSrvIpcMarshalError(
                       dwError,
                       &pResponse);
       BAIL_ON_LSA_ERROR(dwError);
       
    } else {
    
       dwError = LsaBuildMessage(
                        LSA_R_END_ENUM_GROUPS,
                        dwMsgLen,
                        1,
                        1,
                        &pResponse
                        );
       BAIL_ON_LSA_ERROR(dwError);
       
    }
    
    dwError = LsaWriteMessage(pContext->fd, pResponse);
    BAIL_ON_LSA_ERROR(dwError);
    
cleanup:
    
    LSA_SAFE_FREE_MESSAGE(pResponse);
    
    LSA_SAFE_FREE_STRING(pszGUID);
    
    return dwError;
    
error:

    goto cleanup;
}

DWORD
LsaSrvIpcDeleteGroup(
    HANDLE hConnection,
    PLSAMESSAGE pMessage
    )
{
    DWORD  dwError = 0;
    HANDLE hServer = (HANDLE)NULL;
    PLSAMESSAGE pResponse = NULL;
    DWORD dwMsgLen = 0;
    PLSASERVERCONNECTIONCONTEXT pContext =
         (PLSASERVERCONNECTIONCONTEXT)hConnection;
    gid_t  gid = 0;
    
    dwError = LsaSrvIpcOpenServer(hConnection, &hServer);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaUnmarshalDeleteGroupByIdQuery(
                    pMessage->pData,
                    pMessage->header.messageLength,
                    &gid);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaSrvDeleteGroup(
                    hServer,
                    gid);
    BAIL_ON_LSA_ERROR(dwError);

    if (dwError) {
        
       dwError = LsaSrvIpcMarshalError(
                       dwError,
                       &pResponse);
       BAIL_ON_LSA_ERROR(dwError);
       
    } else {
    
       dwError = LsaBuildMessage(
                        LSA_R_DELETE_GROUP,
                        dwMsgLen,
                        1,
                        1,
                        &pResponse
                        );
       BAIL_ON_LSA_ERROR(dwError);
       
    }
    
    dwError = LsaWriteMessage(pContext->fd, pResponse);
    BAIL_ON_LSA_ERROR(dwError);
    
cleanup:

    LSA_SAFE_FREE_MESSAGE(pResponse);
    
    return dwError;
    
error:

    goto cleanup;
}

