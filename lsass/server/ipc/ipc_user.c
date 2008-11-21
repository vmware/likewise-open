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
 *        ipc_user.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 * 
 *        Inter-process communication (Server) API for Users
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "ipc.h"

DWORD
LsaSrvIpcAddUser(
    HANDLE      hConnection,
    PLSAMESSAGE pMessage
    )
{
    DWORD  dwError = 0;
    PVOID* ppUserInfoList = NULL;
    DWORD  dwUserInfoLevel = 0;
    DWORD  dwNumUsers = 0;
    PLSAMESSAGE pResponse = NULL;
    PLSASERVERCONNECTIONCONTEXT pContext  =
         (PLSASERVERCONNECTIONCONTEXT)hConnection;
    HANDLE hServer = (HANDLE)NULL;
    
    dwError = LsaUnmarshalUserInfoList(
                    pMessage->pData,
                    pMessage->header.messageLength,
                    &dwUserInfoLevel,
                    &ppUserInfoList,
                    &dwNumUsers);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaSrvIpcOpenServer(hConnection, &hServer);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaSrvAddUser(
                    hServer,
                    dwUserInfoLevel,
                    *ppUserInfoList
                    );
    if (!dwError) {
        
       dwError = LsaBuildMessage(
                    LSA_R_ADD_USER,
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

    if (ppUserInfoList) {
        LsaFreeUserInfoList(dwUserInfoLevel, ppUserInfoList, dwNumUsers);
    }
    
    LSA_SAFE_FREE_MESSAGE(pResponse);
    
    return dwError;
    
error:

    goto cleanup;
}

DWORD
LsaSrvIpcModifyUser(
    HANDLE hConnection,
    PLSAMESSAGE pMessage
    )
{
    DWORD  dwError = 0;
    PLSA_USER_MOD_INFO pUserModInfo = NULL;
    PLSAMESSAGE pResponse = NULL;
    PLSASERVERCONNECTIONCONTEXT pContext  =
         (PLSASERVERCONNECTIONCONTEXT)hConnection;
    HANDLE hServer = (HANDLE)NULL;
    
    dwError = LsaUnmarshalUserModInfo(
                    pMessage->pData,
                    pMessage->header.messageLength,
                    &pUserModInfo);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaSrvIpcOpenServer(hConnection, &hServer);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaSrvModifyUser(
                    hServer,
                    pUserModInfo);
    if (!dwError) {
        
       dwError = LsaBuildMessage(
                    LSA_R_MODIFY_USER,
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

    if (pUserModInfo) {
        LsaFreeUserModInfo(pUserModInfo);
    }
    
    LSA_SAFE_FREE_MESSAGE(pResponse);
    
    return dwError;
    
error:

    goto cleanup;
}

DWORD
LsaSrvIpcFindUserByName(
    HANDLE hConnection,
    PLSAMESSAGE pQuery
    )
{
    DWORD dwError = 0;
    PSTR pszUserName = NULL;
    PVOID pUserInfo = NULL;
    DWORD dwUserInfoLevel = 0;
    PLSAMESSAGE pResponse = NULL;
    DWORD dwMsgLen = 0;
    PLSASERVERCONNECTIONCONTEXT pContext =
       (PLSASERVERCONNECTIONCONTEXT)hConnection;
    HANDLE hServer = (HANDLE)NULL;
    
    dwError = LsaUnmarshalFindUserByNameQuery(
                    pQuery->pData,
                    pQuery->header.messageLength,
                    &pszUserName,
                    &dwUserInfoLevel
                    );
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaSrvIpcOpenServer(hConnection, &hServer);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaSrvFindUserByName(
                    hServer,
                    pszUserName,
                    dwUserInfoLevel,
                    &pUserInfo);
    if (dwError) {
        dwError = LsaSrvIpcMarshalError(
                        dwError,
                        &pResponse
                        );
        BAIL_ON_LSA_ERROR(dwError);
        
    } else {

       dwError = LsaMarshalUserInfoList(
                       &pUserInfo,
                       dwUserInfoLevel,
                       1,
                       NULL,
                       &dwMsgLen);
       BAIL_ON_LSA_ERROR(dwError);
    
       dwError = LsaBuildMessage(
                        LSA_R_USER_BY_NAME,
                        dwMsgLen,
                        1,
                        1,
                        &pResponse
                        );
       BAIL_ON_LSA_ERROR(dwError);
    
       dwError = LsaMarshalUserInfoList(
                       &pUserInfo,
                       dwUserInfoLevel,
                       1,
                       pResponse->pData,
                       &dwMsgLen);
       BAIL_ON_LSA_ERROR(dwError);
    }
    
    dwError = LsaWriteMessage(pContext->fd, pResponse);
    BAIL_ON_LSA_ERROR(dwError);
    
cleanup:

    if (pUserInfo) {
        LsaFreeUserInfo(dwUserInfoLevel, pUserInfo);
    }
    LSA_SAFE_FREE_STRING(pszUserName);
    
    LSA_SAFE_FREE_MESSAGE(pResponse);
    
    return dwError;
    
error:

    goto cleanup;
}

DWORD
LsaSrvIpcFindUserById(
    HANDLE hConnection,
    PLSAMESSAGE pQuery
    )
{
    DWORD dwError = 0;
    uid_t uid = 0;
    PVOID pUserInfo = NULL;
    DWORD dwUserInfoLevel = 0;
    PLSAMESSAGE pResponse = NULL;
    DWORD dwMsgLen = 0;
    PLSASERVERCONNECTIONCONTEXT pContext =
         (PLSASERVERCONNECTIONCONTEXT)hConnection;
    HANDLE hServer = (HANDLE)NULL;
    
    dwError = LsaUnmarshalFindUserByIdQuery(
                    pQuery->pData,
                    pQuery->header.messageLength,
                    &uid,
                    &dwUserInfoLevel
                    );
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaSrvIpcOpenServer(hConnection, &hServer);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaSrvFindUserById(
                    hServer,
                    uid,
                    dwUserInfoLevel,
                    &pUserInfo);
    if (dwError) {
    
        dwError = LsaSrvIpcMarshalError(
                        dwError,
                        &pResponse
                        );
        BAIL_ON_LSA_ERROR(dwError);
       
    } else {

        dwError = LsaMarshalUserInfoList(&pUserInfo, dwUserInfoLevel, 1, NULL, &dwMsgLen);
        BAIL_ON_LSA_ERROR(dwError);
     
        dwError = LsaBuildMessage(
                         LSA_R_USER_BY_ID,
                         dwMsgLen,
                         1,
                         1,
                         &pResponse
                         );
        BAIL_ON_LSA_ERROR(dwError);
     
        dwError = LsaMarshalUserInfoList(&pUserInfo, dwUserInfoLevel, 1, pResponse->pData, &dwMsgLen);
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    dwError = LsaWriteMessage(pContext->fd, pResponse);
    BAIL_ON_LSA_ERROR(dwError);
    
cleanup:

    if (pUserInfo) {
        LsaFreeUserInfo(dwUserInfoLevel, pUserInfo);
    }
    
    LSA_SAFE_FREE_MESSAGE(pResponse);
    
    return dwError;
    
error:

    goto cleanup;
}

DWORD
LsaSrvIpcBeginEnumUsers(
    HANDLE hConnection,
    PLSAMESSAGE pMessage
    )
{
    DWORD dwError = 0;
    DWORD dwUserInfoLevel = 0;
    DWORD dwNumMaxUsers = 0;
    PLSAMESSAGE pResponse = NULL;
    DWORD dwMsgLen = 0;
    PLSASERVERCONNECTIONCONTEXT pContext =
         (PLSASERVERCONNECTIONCONTEXT)hConnection;
    HANDLE hServer = (HANDLE)NULL;
    PSTR   pszGUID = NULL;
    
    dwError = LsaUnmarshalBeginEnumRecordsQuery(
                        pMessage->pData,
                        pMessage->header.messageLength,
                        &dwUserInfoLevel,
                        &dwNumMaxUsers);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaSrvIpcOpenServer(hConnection, &hServer);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaSrvBeginEnumUsers(
                        hServer,
                        dwUserInfoLevel,
                        dwNumMaxUsers,
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
                        LSA_R_BEGIN_ENUM_USERS,
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
LsaSrvIpcEnumUsers(
    HANDLE hConnection,
    PLSAMESSAGE pMessage
)
{
    DWORD dwError = 0;
    PVOID* ppUserInfoList = NULL;
    DWORD  dwUserInfoLevel = 0;
    DWORD  dwNumUsersFound = 0;
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
    
    dwError = LsaSrvEnumUsers(
                    hServer,
                    pszGUID,
                    &dwUserInfoLevel,
                    &ppUserInfoList,                    
                    &dwNumUsersFound);
    if (dwError) {
        
       dwError = LsaSrvIpcMarshalError(
                       dwError,
                       &pResponse);
       BAIL_ON_LSA_ERROR(dwError);
       
    } else {
        
       dwError = LsaMarshalUserInfoList(
                           ppUserInfoList,
                           dwUserInfoLevel,
                           dwNumUsersFound,
                           NULL,
                           &dwMsgLen);
       BAIL_ON_LSA_ERROR(dwError);
    
       dwError = LsaBuildMessage(
                        LSA_R_ENUM_USERS,
                        dwMsgLen,
                        1,
                        1,
                        &pResponse
                        );
       BAIL_ON_LSA_ERROR(dwError);
    
       dwError = LsaMarshalUserInfoList(
                        ppUserInfoList,
                        dwUserInfoLevel,
                        dwNumUsersFound,
                        pResponse->pData,
                        &dwMsgLen);
       BAIL_ON_LSA_ERROR(dwError);
       
    }
    
    dwError = LsaWriteMessage(pContext->fd, pResponse);
    BAIL_ON_LSA_ERROR(dwError);
    
cleanup:

    if (ppUserInfoList) {
        LsaFreeUserInfoList(dwUserInfoLevel, ppUserInfoList, dwNumUsersFound);
    }
    
    LSA_SAFE_FREE_MESSAGE(pResponse);
    
    LSA_SAFE_FREE_STRING(pszGUID);
    
    return dwError;
    
error:

    goto cleanup;
}

DWORD
LsaSrvIpcEndEnumUsers(
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
    
    dwError = LsaSrvEndEnumUsers(hServer, pszGUID);
    BAIL_ON_LSA_ERROR(dwError);
    
    if (dwError) {
        
       dwError = LsaSrvIpcMarshalError(
                       dwError,
                       &pResponse);
       BAIL_ON_LSA_ERROR(dwError);
       
    } else {
    
       dwError = LsaBuildMessage(
                        LSA_R_END_ENUM_USERS,
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
LsaSrvIpcDeleteUser(
    HANDLE hConnection,
    PLSAMESSAGE pMessage
    )
{
    DWORD dwError = 0;
    uid_t uid = 0;
    HANDLE hServer = (HANDLE)NULL;
    PLSAMESSAGE pResponse = NULL;
    DWORD dwMsgLen = 0;
    PLSASERVERCONNECTIONCONTEXT pContext  =
             (PLSASERVERCONNECTIONCONTEXT)hConnection;
        
    dwError = LsaSrvIpcOpenServer(hConnection, &hServer);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaUnmarshalDeleteUserByIdQuery(
                    pMessage->pData,
                    pMessage->header.messageLength,
                    &uid);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaSrvDeleteUser(
                    hServer,
                    uid);
    
    if (dwError) {
            
        dwError = LsaSrvIpcMarshalError(
                           dwError,
                           &pResponse);
        BAIL_ON_LSA_ERROR(dwError);
           
    } else {
        
        dwError = LsaBuildMessage(
                           LSA_R_DELETE_USER,
                           dwMsgLen,
                           1,
                           1,
                           &pResponse);
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

DWORD
LsaSrvIpcGetNamesBySidList(
    HANDLE hConnection,
    const PLSAMESSAGE pMessage)
{
    DWORD dwError = 0;
    size_t sCount = 0;
    PSTR* ppszSidList = NULL;
    PSTR* ppszDomainNames = NULL;
    PSTR* ppszSamAccounts = NULL;
    ADAccountType* pTypes = NULL;
    PLSAMESSAGE pResponse = NULL;
    DWORD dwMsgLen = 0;
    PLSASERVERCONNECTIONCONTEXT pContext =
       (PLSASERVERCONNECTIONCONTEXT)hConnection;
    HANDLE hServer = (HANDLE)NULL;
    CHAR chDomainSeparator = 0;
    
    dwError = LsaUnmarshalGetNamesBySidListQuery(
                    pMessage->pData,
                    pMessage->header.messageLength,
                    &sCount,
                    &ppszSidList);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaSrvIpcOpenServer(hConnection, &hServer);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaSrvGetNamesBySidList(
                    hServer,
                    sCount,
                    ppszSidList,
                    &ppszDomainNames,
                    &ppszSamAccounts,
                    &pTypes,
                    &chDomainSeparator);
    if (dwError) {
        dwError = LsaSrvIpcMarshalError(
                        dwError,
                        &pResponse
                        );
        BAIL_ON_LSA_ERROR(dwError);
        
    } else {

       dwError = LsaMarshalGetNamesBySidListReply(
               sCount,
               ppszDomainNames,
               ppszSamAccounts,
               pTypes,
               chDomainSeparator,
               NULL,
               &dwMsgLen);
       BAIL_ON_LSA_ERROR(dwError);
    
       dwError = LsaBuildMessage(
                        LSA_R_NAMES_BY_SID_LIST,
                        dwMsgLen,
                        1,
                        1,
                        &pResponse);
       BAIL_ON_LSA_ERROR(dwError);
    
       dwError = LsaMarshalGetNamesBySidListReply(
               sCount,
               ppszDomainNames,
               ppszSamAccounts,
               pTypes,
               chDomainSeparator,
               pResponse->pData,
               &dwMsgLen);
       BAIL_ON_LSA_ERROR(dwError);
    }
    
    dwError = LsaWriteMessage(pContext->fd, pResponse);
    BAIL_ON_LSA_ERROR(dwError);
    
cleanup:

    LsaFreeStringArray(ppszSidList, sCount);
    LsaFreeStringArray(ppszDomainNames, sCount);
    LsaFreeStringArray(ppszSamAccounts, sCount);
    LSA_SAFE_FREE_MEMORY(pTypes);
    
    LSA_SAFE_FREE_MESSAGE(pResponse);
    
    return dwError;
    
error:

    goto cleanup;
}
