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
 *        users.c
 *
 * Abstract:
 * 
 *        Likewise Security and Authentication Subsystem (LSASS)
 * 
 *        User Lookup and Management API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 */
#include "client.h"

LSASS_API
DWORD
LsaAddUser(
    HANDLE hLsaConnection,
    PVOID  pUserInfo,
    DWORD  dwUserInfoLevel
    )
{
    DWORD dwError = 0;
    DWORD dwMsgLen = 0;
    PLSAMESSAGE pMessage = NULL;
    PSTR pszError = NULL;

    if (geteuid() != 0)
    {
        dwError = EACCES;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    BAIL_ON_INVALID_HANDLE(hLsaConnection);
    BAIL_ON_INVALID_POINTER(pUserInfo);
    
    dwError = LsaValidateUserInfo(
                    pUserInfo,
                    dwUserInfoLevel);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaMarshalUserInfoList(
                    (PVOID*)&pUserInfo,
                    dwUserInfoLevel,
                    1,
                    NULL,
                    &dwMsgLen
                    );
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaBuildMessage(
                LSA_Q_ADD_USER,
                dwMsgLen,
                1,
                1,
                &pMessage);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaMarshalUserInfoList(
                    (PVOID*)&pUserInfo,
                    dwUserInfoLevel,
                    1,
                    pMessage->pData,
                    &dwMsgLen
                    );
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaSendMessage(hLsaConnection, pMessage);
    BAIL_ON_LSA_ERROR(dwError);
    
    LSA_SAFE_FREE_MESSAGE(pMessage);
    
    dwError = LsaGetNextMessage(hLsaConnection, &pMessage);
    BAIL_ON_LSA_ERROR(dwError);
        
    switch (pMessage->header.messageType)
    {
        case LSA_R_ADD_USER:
        {
            // successfully added user
            break;
        }
        case LSA_ERROR:
        {
            DWORD dwSrvError = 0;
                
            dwError = LsaUnmarshalError(
                                pMessage->pData,
                                pMessage->header.messageLength,
                                &dwSrvError,
                                &pszError);
            BAIL_ON_LSA_ERROR(dwError);
            dwError = dwSrvError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        }
        default:
        {
            dwError = LSA_ERROR_UNEXPECTED_MESSAGE;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }
    
cleanup:

    LSA_SAFE_FREE_MESSAGE(pMessage);
    LSA_SAFE_FREE_STRING(pszError);

    return dwError;
    
error:

    goto cleanup;
}

LSASS_API
DWORD
LsaModifyUser(
    HANDLE hLsaConnection,
    PLSA_USER_MOD_INFO pUserModInfo
    )
{
    DWORD dwError = 0;
    DWORD dwMsgLen = 0;
    PLSAMESSAGE pMessage = NULL;
    PSTR pszError = NULL;
    
    if (geteuid() != 0)
    {
        dwError = EACCES;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    BAIL_ON_INVALID_HANDLE(hLsaConnection);
    BAIL_ON_INVALID_POINTER(pUserModInfo);
    
    dwError = LsaMarshalUserModInfo(
                    pUserModInfo,
                    NULL,
                    &dwMsgLen);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaBuildMessage(
                LSA_Q_MODIFY_USER,
                dwMsgLen,
                1,
                1,
                &pMessage);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaMarshalUserModInfo(
                    pUserModInfo,
                    pMessage->pData,
                    &dwMsgLen);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaSendMessage(hLsaConnection, pMessage);
    BAIL_ON_LSA_ERROR(dwError);
    
    LSA_SAFE_FREE_MESSAGE(pMessage);
    
    dwError = LsaGetNextMessage(hLsaConnection, &pMessage);
    BAIL_ON_LSA_ERROR(dwError);
        
    switch (pMessage->header.messageType)
    {
        case LSA_R_MODIFY_USER:
        {
            // successfully modified user
            break;
        }
        case LSA_ERROR:
        {
            DWORD dwSrvError = 0;
                
            dwError = LsaUnmarshalError(
                                pMessage->pData,
                                pMessage->header.messageLength,
                                &dwSrvError,
                                &pszError);
            BAIL_ON_LSA_ERROR(dwError);
            dwError = dwSrvError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        }
        default:
        {
            dwError = LSA_ERROR_UNEXPECTED_MESSAGE;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }
    
cleanup:

    LSA_SAFE_FREE_MESSAGE(pMessage);
    LSA_SAFE_FREE_STRING(pszError);

    return dwError;
    
error:

    goto cleanup;
}

LSASS_API
DWORD
LsaFindUserByName(
    HANDLE hLsaConnection,
    PCSTR  pszName,
    DWORD  dwUserInfoLevel,
    PVOID* ppUserInfo
    )
{
    DWORD dwError = 0;
    PLSAMESSAGE pMessage = NULL;
    DWORD   dwMsgLen = 0;
    PSTR    pszError = NULL;
    PVOID*  ppUserInfoList = NULL;
    DWORD   dwNumUsers = 0;
    
    BAIL_ON_INVALID_HANDLE(hLsaConnection);
    BAIL_ON_INVALID_STRING(pszName);
    BAIL_ON_INVALID_POINTER(ppUserInfo);
    
    dwError = LsaValidateUserName(pszName);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaValidateUserInfoLevel(dwUserInfoLevel);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaMarshalFindUserByNameQuery(
                 pszName,
                 dwUserInfoLevel,
                 NULL,
                 &dwMsgLen
                 );
    BAIL_ON_LSA_ERROR(dwError);
        
    dwError = LsaBuildMessage(
                LSA_Q_USER_BY_NAME,
                dwMsgLen,
                1,
                1,
                &pMessage);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaMarshalFindUserByNameQuery(
                 pszName,
                 dwUserInfoLevel,
                 pMessage->pData,
                 &dwMsgLen
                 );
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaSendMessage(hLsaConnection, pMessage);
    BAIL_ON_LSA_ERROR(dwError);
    
    LSA_SAFE_FREE_MESSAGE(pMessage);
    
    dwError = LsaGetNextMessage(hLsaConnection, &pMessage);
    BAIL_ON_LSA_ERROR(dwError);
    
    switch (pMessage->header.messageType) {
        case LSA_R_USER_BY_NAME:
        {
            dwError = LsaUnmarshalUserInfoList(
                                pMessage->pData,
                                pMessage->header.messageLength,
                                &dwUserInfoLevel,
                                &ppUserInfoList,
                                &dwNumUsers
                                );
            BAIL_ON_LSA_ERROR(dwError);
            break;
        }
        case LSA_ERROR:
        {
            DWORD dwSrvError = 0;
            
            dwError = LsaUnmarshalError(
                                pMessage->pData,
                                pMessage->header.messageLength,
                                &dwSrvError,
                                &pszError
                                );
            BAIL_ON_LSA_ERROR(dwError);
            dwError = dwSrvError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        }
        default:
        {
            dwError = LSA_ERROR_UNEXPECTED_MESSAGE;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }
    
    if (dwNumUsers > 1) {
        // Login Ids must be unique
        dwError = LSA_ERROR_INTERNAL;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    *ppUserInfo = *ppUserInfoList;

cleanup:

    LSA_SAFE_FREE_MESSAGE(pMessage);
    LSA_SAFE_FREE_STRING(pszError);

    return dwError;
        
error:

    if (ppUserInfoList) {
        LsaFreeUserInfoList(dwUserInfoLevel, ppUserInfoList, dwNumUsers);
    }
    
    if (ppUserInfo) {
       *ppUserInfo = NULL;
    }

    goto cleanup;
}

LSASS_API
DWORD
LsaFindUserById(
    HANDLE hLsaConnection,
    uid_t  uid,
    DWORD  dwUserInfoLevel,
    PVOID* ppUserInfo
    )
{
    DWORD dwError = 0;
    PLSAMESSAGE pMessage = NULL;
    DWORD   dwMsgLen = 0;
    PSTR    pszError = NULL;
    PVOID*  ppUserInfoList = NULL;
    DWORD   dwNumUsers = 0;
    
    BAIL_ON_INVALID_HANDLE(hLsaConnection);
    BAIL_ON_INVALID_POINTER(ppUserInfo);
    
    dwError = LsaValidateUserInfoLevel(dwUserInfoLevel);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaMarshalFindUserByIdQuery(
                uid,
                dwUserInfoLevel,
                NULL,
                &dwMsgLen
                );
    BAIL_ON_LSA_ERROR(dwError);
        
    dwError = LsaBuildMessage(
                LSA_Q_USER_BY_ID,
                dwMsgLen,
                1,
                1,
                &pMessage);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaMarshalFindUserByIdQuery(
                uid,
                dwUserInfoLevel,
                pMessage->pData,
                &dwMsgLen
                );
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaSendMessage(hLsaConnection, pMessage);
    BAIL_ON_LSA_ERROR(dwError);
    
    LSA_SAFE_FREE_MESSAGE(pMessage);
    
    dwError = LsaGetNextMessage(hLsaConnection, &pMessage);
    BAIL_ON_LSA_ERROR(dwError);
    
    switch (pMessage->header.messageType) {
        case LSA_R_USER_BY_ID:
        {
            dwError = LsaUnmarshalUserInfoList(
                                pMessage->pData,
                                pMessage->header.messageLength,
                                &dwUserInfoLevel,
                                &ppUserInfoList,
                                &dwNumUsers
                                );
            BAIL_ON_LSA_ERROR(dwError);
            break;
        }
        case LSA_ERROR:
        {
            DWORD dwSrvError = 0;
            
            dwError = LsaUnmarshalError(
                                pMessage->pData,
                                pMessage->header.messageLength,
                                &dwSrvError,
                                &pszError
                                );
            BAIL_ON_LSA_ERROR(dwError);
            dwError = dwSrvError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        }
        default:
        {
            dwError = LSA_ERROR_UNEXPECTED_MESSAGE;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }
    
    if (dwNumUsers > 1) {
        dwError = LSA_ERROR_INTERNAL;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    *ppUserInfo = *ppUserInfoList;

cleanup:

    LSA_SAFE_FREE_MESSAGE(pMessage);
    LSA_SAFE_FREE_STRING(pszError);

    return dwError;
        
error:

    if (ppUserInfoList) {
        LsaFreeUserInfoList(dwUserInfoLevel, ppUserInfoList, dwNumUsers);
    }
    
    if (ppUserInfo) {
        *ppUserInfo = NULL;
    }
    
    goto cleanup;
}

LSASS_API
DWORD
LsaBeginEnumUsers(
    HANDLE  hLsaConnection,
    DWORD   dwUserInfoLevel,
    DWORD   dwMaxNumUsers,
    PHANDLE phResume
    )
{
    DWORD dwError = 0;
    PLSA_ENUM_USERS_INFO pInfo = NULL;
    PLSAMESSAGE pMessage = NULL;
    DWORD   dwMsgLen = 0;
    PSTR    pszError = NULL;
    PSTR    pszGUID = NULL;
    
    BAIL_ON_INVALID_HANDLE(hLsaConnection);
    BAIL_ON_INVALID_POINTER(phResume);
    
    dwError = LsaValidateUserInfoLevel(dwUserInfoLevel);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaMarshalBeginEnumRecordsQuery(
                    dwUserInfoLevel,
                    dwMaxNumUsers,
                    NULL,
                    &dwMsgLen);
    BAIL_ON_LSA_ERROR(dwError);
        
    dwError = LsaBuildMessage(
                LSA_Q_BEGIN_ENUM_USERS,
                dwMsgLen,
                1,
                1,
                &pMessage);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaMarshalBeginEnumRecordsQuery(
                    dwUserInfoLevel,
                    dwMaxNumUsers,
                    pMessage->pData,
                    &dwMsgLen);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaSendMessage(hLsaConnection, pMessage);
    BAIL_ON_LSA_ERROR(dwError);
    
    LSA_SAFE_FREE_MESSAGE(pMessage);
    
    dwError = LsaGetNextMessage(hLsaConnection, &pMessage);
    BAIL_ON_LSA_ERROR(dwError);
    
    switch (pMessage->header.messageType) {
        case LSA_R_BEGIN_ENUM_USERS:
        {
            dwError = LsaUnmarshalEnumRecordsToken(
                                pMessage->pData,
                                pMessage->header.messageLength,
                                &pszGUID);
            BAIL_ON_LSA_ERROR(dwError);
            
            break;
        }
        case LSA_ERROR:
        {
            DWORD dwSrvError = 0;
            
            dwError = LsaUnmarshalError(
                                pMessage->pData,
                                pMessage->header.messageLength,
                                &dwSrvError,
                                &pszError
                                );
            BAIL_ON_LSA_ERROR(dwError);
            dwError = dwSrvError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        }
        default:
        {
            dwError = LSA_ERROR_UNEXPECTED_MESSAGE;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }
    
    dwError = LsaAllocateMemory(
                    sizeof(LSA_ENUM_USERS_INFO),
                    (PVOID*)&pInfo);
    BAIL_ON_LSA_ERROR(dwError);
    
    pInfo->dwUserInfoLevel = dwUserInfoLevel;
    pInfo->dwNumMaxUsers = dwMaxNumUsers;
    pInfo->pszGUID = pszGUID;
    
    *phResume = (HANDLE)pInfo;
    
cleanup:

    LSA_SAFE_FREE_MESSAGE(pMessage);

    return dwError;
    
error:

    if (phResume) {
       *phResume = (HANDLE)NULL;
    }

    LSA_SAFE_FREE_STRING(pszGUID);

    goto cleanup;
}

LSASS_API
DWORD
LsaEnumUsers(
    HANDLE  hLsaConnection,
    HANDLE  hResume,
    PDWORD  pdwNumUsersFound,
    PVOID** pppUserInfoList
    )
{
    DWORD dwError = 0;
    PLSAMESSAGE pMessage = NULL;
    DWORD   dwMsgLen = 0;
    PSTR    pszError = NULL;
    DWORD   dwUserInfoLevel = 0;
    PVOID*  ppUserInfoList = NULL;
    DWORD   dwNumUsersFound = 0;    
    PLSA_ENUM_USERS_INFO pInfo = (PLSA_ENUM_USERS_INFO)hResume;
    
    BAIL_ON_INVALID_HANDLE(hLsaConnection);
    BAIL_ON_INVALID_HANDLE(hResume);
    BAIL_ON_INVALID_POINTER(pdwNumUsersFound);
    BAIL_ON_INVALID_HANDLE(pppUserInfoList);
    
    dwError = LsaMarshalEnumRecordsToken(
                    pInfo->pszGUID,
                    NULL,
                    &dwMsgLen);
    BAIL_ON_LSA_ERROR(dwError);
        
    dwError = LsaBuildMessage(
                LSA_Q_ENUM_USERS,
                dwMsgLen,
                1,
                1,
                &pMessage);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaMarshalEnumRecordsToken(
                    pInfo->pszGUID,
                    pMessage->pData,
                    &dwMsgLen);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaSendMessage(hLsaConnection, pMessage);
    BAIL_ON_LSA_ERROR(dwError);
    
    LSA_SAFE_FREE_MESSAGE(pMessage);
    
    dwError = LsaGetNextMessage(hLsaConnection, &pMessage);
    BAIL_ON_LSA_ERROR(dwError);
    
    switch (pMessage->header.messageType) {
        case LSA_R_ENUM_USERS:
        {
            dwError = LsaUnmarshalUserInfoList(
                                pMessage->pData,
                                pMessage->header.messageLength,
                                &dwUserInfoLevel,
                                &ppUserInfoList,
                                &dwNumUsersFound
                                );
            BAIL_ON_LSA_ERROR(dwError);
            break;
        }
        case LSA_ERROR:
        {
            DWORD dwSrvError = 0;
            
            dwError = LsaUnmarshalError(
                                pMessage->pData,
                                pMessage->header.messageLength,
                                &dwSrvError,
                                &pszError
                                );
            BAIL_ON_LSA_ERROR(dwError);
            dwError = dwSrvError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        }
        default:
        {
            dwError = LSA_ERROR_UNEXPECTED_MESSAGE;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }
    
    *pdwNumUsersFound = dwNumUsersFound;
    *pppUserInfoList = ppUserInfoList;

cleanup:

    LSA_SAFE_FREE_MESSAGE(pMessage);
    LSA_SAFE_FREE_STRING(pszError);

    return dwError;
        
error:

    if (ppUserInfoList) {
        LsaFreeUserInfoList(dwUserInfoLevel, ppUserInfoList, dwNumUsersFound);
    }
    
    if (pppUserInfoList) {
        *pppUserInfoList = NULL;
    }
    
    if (pdwNumUsersFound) {
        *pdwNumUsersFound = 0;
    }

    goto cleanup;
}

LSASS_API
DWORD
LsaEndEnumUsers(
    HANDLE hLsaConnection,
    HANDLE hResume
    )
{
    DWORD dwError = 0;
    PLSA_ENUM_USERS_INFO pInfo = (PLSA_ENUM_USERS_INFO)hResume;
    PLSAMESSAGE pMessage = NULL;
    DWORD   dwMsgLen = 0;
    PSTR    pszError = NULL;
    
    BAIL_ON_INVALID_HANDLE(hLsaConnection);
    BAIL_ON_INVALID_HANDLE(hResume);
    
    dwError = LsaMarshalEnumRecordsToken(
                    pInfo->pszGUID,
                    NULL,
                    &dwMsgLen);
    BAIL_ON_LSA_ERROR(dwError);
        
    dwError = LsaBuildMessage(
                LSA_Q_END_ENUM_USERS,
                dwMsgLen,
                1,
                1,
                &pMessage);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaMarshalEnumRecordsToken(
                    pInfo->pszGUID,
                    pMessage->pData,
                    &dwMsgLen);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaSendMessage(hLsaConnection, pMessage);
    BAIL_ON_LSA_ERROR(dwError);
    
    LSA_SAFE_FREE_MESSAGE(pMessage);
    
    dwError = LsaGetNextMessage(hLsaConnection, &pMessage);
    BAIL_ON_LSA_ERROR(dwError);
    
    switch (pMessage->header.messageType) {
        case LSA_R_END_ENUM_USERS:
        {
            // Success
            break;
        }
        case LSA_ERROR:
        {
            DWORD dwSrvError = 0;
            
            dwError = LsaUnmarshalError(
                                pMessage->pData,
                                pMessage->header.messageLength,
                                &dwSrvError,
                                &pszError
                                );
            BAIL_ON_LSA_ERROR(dwError);
            dwError = dwSrvError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        }
        default:
        {
            dwError = LSA_ERROR_UNEXPECTED_MESSAGE;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }
    
    LsaFreeEnumUsersInfo(pInfo);
    
cleanup:

    LSA_SAFE_FREE_MESSAGE(pMessage);

    return dwError;
    
error:

    goto cleanup;
}

LSASS_API
DWORD
LsaDeleteUserById(
    HANDLE hLsaConnection,
    uid_t  uid
    )
{
    DWORD dwError = 0;
    PLSAMESSAGE pMessage = NULL;
    DWORD   dwMsgLen = 0;
    PSTR    pszError = NULL;
    
    if (geteuid() != 0)
    {
        dwError = EACCES;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    BAIL_ON_INVALID_HANDLE(hLsaConnection);
    
    dwError = LsaMarshalDeleteUserByIdQuery(
                uid,
                NULL,
                &dwMsgLen);
    BAIL_ON_LSA_ERROR(dwError);
        
    dwError = LsaBuildMessage(
                LSA_Q_DELETE_USER,
                dwMsgLen,
                1,
                1,
                &pMessage);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaMarshalDeleteUserByIdQuery(
                uid,
                pMessage->pData,
                &dwMsgLen);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaSendMessage(hLsaConnection, pMessage);
    BAIL_ON_LSA_ERROR(dwError);
    
    LSA_SAFE_FREE_MESSAGE(pMessage);
    
    dwError = LsaGetNextMessage(hLsaConnection, &pMessage);
    BAIL_ON_LSA_ERROR(dwError);
    
    switch (pMessage->header.messageType) {
        case LSA_R_DELETE_USER:
        {
            // Success
            break;
        }
        case LSA_ERROR:
        {
            DWORD dwSrvError = 0;
            
            dwError = LsaUnmarshalError(
                                pMessage->pData,
                                pMessage->header.messageLength,
                                &dwSrvError,
                                &pszError);
            BAIL_ON_LSA_ERROR(dwError);
            dwError = dwSrvError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        }
        default:
        {
            dwError = LSA_ERROR_UNEXPECTED_MESSAGE;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

cleanup:

    LSA_SAFE_FREE_MESSAGE(pMessage);
    LSA_SAFE_FREE_STRING(pszError);

    return dwError;
        
error:

    goto cleanup;
}

LSASS_API
DWORD
LsaDeleteUserByName(
    HANDLE hLsaConnection,
    PCSTR  pszName
    )
{
    DWORD dwError = 0;
    PVOID pUserInfo = NULL;
    DWORD dwUserInfoLevel = 0;
    
    if (geteuid() != 0)
    {
        dwError = EACCES;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    BAIL_ON_INVALID_HANDLE(hLsaConnection);
    BAIL_ON_INVALID_STRING(pszName);
    
    dwError = LsaFindUserByName(
                    hLsaConnection,
                    pszName,
                    dwUserInfoLevel,
                    &pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaDeleteUserById(
                    hLsaConnection,
                    ((PLSA_USER_INFO_0)pUserInfo)->uid);
    BAIL_ON_LSA_ERROR(dwError);
    
cleanup:

    if (pUserInfo) {
        LsaFreeUserInfo(dwUserInfoLevel, pUserInfo);
    }

    return dwError;
    
error:

    goto cleanup;
}

VOID
LsaFreeEnumUsersInfo(
    PLSA_ENUM_USERS_INFO pInfo
    )
{
    LSA_SAFE_FREE_STRING(pInfo->pszGUID);
    LsaFreeMemory(pInfo);
}

LSASS_API
DWORD
LsaGetNamesBySidList(
    HANDLE          hLsaConnection,
    size_t          sCount,
    PSTR*           ppszSidList,
    PLSA_SID_INFO*  ppSIDInfoList
    )
{
    DWORD dwError = 0;
    PLSAMESSAGE pMessage = NULL;
    DWORD   dwMsgLen = 0;
    PSTR    pszError = NULL;
    size_t  sIndex = 0;
    size_t  sReplyCount = 0;
    PLSA_SID_INFO pSIDInfoList = NULL;
    
    BAIL_ON_INVALID_HANDLE(hLsaConnection);
    BAIL_ON_INVALID_POINTER(ppszSidList);
    BAIL_ON_INVALID_POINTER(ppSIDInfoList);

    for(sIndex = 0; sIndex < sCount; sIndex++)
    {
        BAIL_ON_INVALID_STRING(ppszSidList[sIndex]);
    }
    
    dwError = LsaMarshalGetNamesBySidListQuery(
                    sCount,
                    ppszSidList,
                    NULL,
                    &dwMsgLen);
    BAIL_ON_LSA_ERROR(dwError);
        
    dwError = LsaBuildMessage(
                    LSA_Q_NAMES_BY_SID_LIST,
                    dwMsgLen,
                    1,
                    1,
                    &pMessage);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaMarshalGetNamesBySidListQuery(
                    sCount,
                    ppszSidList,
                    pMessage->pData,
                    &dwMsgLen);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaSendMessage(hLsaConnection, pMessage);
    BAIL_ON_LSA_ERROR(dwError);
    
    LSA_SAFE_FREE_MESSAGE(pMessage);
    
    dwError = LsaGetNextMessage(hLsaConnection, &pMessage);
    BAIL_ON_LSA_ERROR(dwError);
    
    switch (pMessage->header.messageType) {
        case LSA_R_NAMES_BY_SID_LIST:
        {
            dwError = LsaUnmarshalGetNamesBySidListReply(
                                pMessage->pData,
                                pMessage->header.messageLength,
                                &sReplyCount,
                                &pSIDInfoList);
            BAIL_ON_LSA_ERROR(dwError);
            if(sReplyCount != sCount)
            {
                dwError = LSA_ERROR_INVALID_MESSAGE;
                BAIL_ON_LSA_ERROR(dwError);
            }
            break;
        }
        case LSA_ERROR:
        {
            DWORD dwSrvError = 0;
            
            dwError = LsaUnmarshalError(
                                pMessage->pData,
                                pMessage->header.messageLength,
                                &dwSrvError,
                                &pszError
                                );
            BAIL_ON_LSA_ERROR(dwError);
            dwError = dwSrvError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        }
        default:
        {
            dwError = LSA_ERROR_UNEXPECTED_MESSAGE;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }
    
    *ppSIDInfoList = pSIDInfoList;

cleanup:

    LSA_SAFE_FREE_MESSAGE(pMessage);
    LSA_SAFE_FREE_STRING(pszError);

    return dwError;
        
error:

    if (pSIDInfoList) {
        LsaFreeSIDInfoList(pSIDInfoList, sReplyCount);
    }
    
    *ppSIDInfoList = NULL;
    
    goto cleanup;
}
