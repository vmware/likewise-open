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
 *        groups.c
 *
 * Abstract:
 * 
 *        Likewise Security and Authentication Subsystem (LSASS)
 * 
 *        Group Lookup and Management API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 */
#include "client.h"

LSASS_API
DWORD
LsaAddGroup(
    HANDLE hLsaConnection,
    PVOID  pGroupInfo,
    DWORD  dwGroupInfoLevel
    )
{
    DWORD dwError = 0;
    DWORD dwMsgLen = 0;
    PLSAMESSAGE pMessage = NULL;
    PSTR  pszError = NULL;

    if (geteuid() != 0)
    {
        dwError = EACCES;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    BAIL_ON_INVALID_HANDLE(hLsaConnection);
    BAIL_ON_INVALID_POINTER(pGroupInfo);
    
    dwError = LsaValidateGroupInfo(
                    pGroupInfo,
                    dwGroupInfoLevel);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaMarshalGroupInfoList(
                    (PVOID*)&pGroupInfo,
                    dwGroupInfoLevel,
                    1,
                    NULL,
                    &dwMsgLen
                    );
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaBuildMessage(
                LSA_Q_ADD_GROUP,
                dwMsgLen,
                1,
                1,
                &pMessage);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaMarshalGroupInfoList(
                    (PVOID*)&pGroupInfo,
                    dwGroupInfoLevel,
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
        case LSA_R_ADD_GROUP:
        {
            // successfully added group
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
LsaFindGroupByName(
    HANDLE hLsaConnection,
    PCSTR  pszGroupName,
    DWORD  dwGroupInfoLevel,
    PVOID* ppGroupInfo
    )
{
    DWORD dwError = 0;
    PLSAMESSAGE pMessage = NULL;
    DWORD   dwMsgLen = 0;
    PSTR    pszError = NULL;
    PVOID*  ppGroupInfoList = NULL;
    DWORD   dwNumGroups = 0;
    
    BAIL_ON_INVALID_HANDLE(hLsaConnection);
    BAIL_ON_INVALID_STRING(pszGroupName);
    BAIL_ON_INVALID_POINTER(ppGroupInfo);
    
    dwError = LsaValidateGroupName(pszGroupName);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaValidateGroupInfoLevel(dwGroupInfoLevel);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaMarshalFindGroupByNameQuery(
                pszGroupName,
                dwGroupInfoLevel,
                NULL,
                &dwMsgLen
                );
    BAIL_ON_LSA_ERROR(dwError);
        
    dwError = LsaBuildMessage(
                LSA_Q_GROUP_BY_NAME,
                dwMsgLen,
                1,
                1,
                &pMessage);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaMarshalFindGroupByNameQuery(
                pszGroupName,
                dwGroupInfoLevel,
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
        case LSA_R_GROUP_BY_NAME:
        {
            dwError = LsaUnmarshalGroupInfoList(
                                    pMessage->pData,
                                    pMessage->header.messageLength,
                                    &dwGroupInfoLevel,
                                    &ppGroupInfoList,
                                    &dwNumGroups
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
    
    if (dwNumGroups > 1) {
        dwError = LSA_ERROR_INTERNAL;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    *ppGroupInfo = *ppGroupInfoList;

cleanup:

    LSA_SAFE_FREE_MESSAGE(pMessage);
    LSA_SAFE_FREE_STRING(pszError);

    return dwError;
        
error:

    if (ppGroupInfoList) {
        LsaFreeGroupInfoList(dwGroupInfoLevel, ppGroupInfoList, dwNumGroups);
    }
    
    if (ppGroupInfo) {
        *ppGroupInfo = NULL;
    }

    goto cleanup;
}

LSASS_API
DWORD
LsaFindGroupById(
    HANDLE hLsaConnection,
    gid_t  gid,
    DWORD  dwGroupInfoLevel,
    PVOID* ppGroupInfo
    )
{
    DWORD dwError = 0;
    PLSAMESSAGE pMessage = NULL;
    DWORD   dwMsgLen = 0;
    PSTR    pszError = NULL;
    PVOID*  ppGroupInfoList = NULL;
    DWORD   dwNumGroups = 0;
    
    BAIL_ON_INVALID_HANDLE(hLsaConnection);
    BAIL_ON_INVALID_POINTER(ppGroupInfo);
    
    dwError = LsaValidateGroupInfoLevel(dwGroupInfoLevel);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaMarshalFindGroupByIdQuery(
                gid,
                dwGroupInfoLevel,
                NULL,
                &dwMsgLen
                );
    BAIL_ON_LSA_ERROR(dwError);
        
    dwError = LsaBuildMessage(
                LSA_Q_GROUP_BY_ID,
                dwMsgLen,
                1,
                1,
                &pMessage);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaMarshalFindGroupByIdQuery(
                gid,
                dwGroupInfoLevel,
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
        case LSA_R_GROUP_BY_ID:
        {
            dwError = LsaUnmarshalGroupInfoList(
                                pMessage->pData,
                                pMessage->header.messageLength,
                                &dwGroupInfoLevel,
                                &ppGroupInfoList,
                                &dwNumGroups
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
    
    if (dwNumGroups > 1) {
        dwError = LSA_ERROR_INTERNAL;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    *ppGroupInfo = *ppGroupInfoList;

cleanup:

    LSA_SAFE_FREE_MESSAGE(pMessage);
    LSA_SAFE_FREE_STRING(pszError);

    return dwError;
        
error:

    if (ppGroupInfoList) {
        LsaFreeGroupInfoList(dwGroupInfoLevel, ppGroupInfoList, dwNumGroups);
    }
    
    if (ppGroupInfo) {
        *ppGroupInfo = NULL;
    }

    goto cleanup;
}

LSASS_API
DWORD
LsaBeginEnumGroups(
    HANDLE  hLsaConnection,
    DWORD   dwGroupInfoLevel,
    DWORD   dwMaxNumGroups,
    PHANDLE phResume
    )
{
    DWORD dwError = 0;
    PLSA_ENUM_GROUPS_INFO pInfo = NULL;
    PLSAMESSAGE pMessage = NULL;
    DWORD   dwMsgLen = 0;
    PSTR    pszError = NULL;
    PSTR    pszGUID = NULL;
    
    BAIL_ON_INVALID_HANDLE(hLsaConnection);
    BAIL_ON_INVALID_POINTER(phResume);
    
    dwError = LsaValidateGroupInfoLevel(dwGroupInfoLevel);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaMarshalBeginEnumRecordsQuery(
                    dwGroupInfoLevel,
                    dwMaxNumGroups,
                    NULL,
                    &dwMsgLen);
    BAIL_ON_LSA_ERROR(dwError);
        
    dwError = LsaBuildMessage(
                LSA_Q_BEGIN_ENUM_GROUPS,
                dwMsgLen,
                1,
                1,
                &pMessage);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaMarshalBeginEnumRecordsQuery(
                    dwGroupInfoLevel,
                    dwMaxNumGroups,
                    pMessage->pData,
                    &dwMsgLen);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaSendMessage(hLsaConnection, pMessage);
    BAIL_ON_LSA_ERROR(dwError);
    
    LSA_SAFE_FREE_MESSAGE(pMessage);
    
    dwError = LsaGetNextMessage(hLsaConnection, &pMessage);
    BAIL_ON_LSA_ERROR(dwError);
    
    switch (pMessage->header.messageType) {
        case LSA_R_BEGIN_ENUM_GROUPS:
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
                    sizeof(LSA_ENUM_GROUPS_INFO),
                    (PVOID*)&pInfo);
    BAIL_ON_LSA_ERROR(dwError);
    
    pInfo->dwGroupInfoLevel = dwGroupInfoLevel;
    pInfo->dwNumMaxGroups = dwMaxNumGroups;
    pInfo->pszGUID = pszGUID;
    
    *phResume = (HANDLE)pInfo;
    
cleanup:

    LSA_SAFE_FREE_MESSAGE(pMessage);

    return dwError;
    
error:

    *phResume = (HANDLE)NULL;

    LSA_SAFE_FREE_STRING(pszGUID);

    goto cleanup;
}

LSASS_API
DWORD
LsaEnumGroups(
    HANDLE  hLsaConnection,
    HANDLE  hResume,
    PDWORD  pdwNumGroupsFound,
    PVOID** pppGroupInfoList
    )
{
    DWORD dwError = 0;
    PLSAMESSAGE pMessage = NULL;
    DWORD   dwMsgLen = 0;
    PSTR    pszError = NULL;
    PVOID*  ppGroupInfoList = NULL;
    DWORD   dwNumGroupsFound = 0;
    DWORD   dwGroupInfoLevel = 0;
    PLSA_ENUM_GROUPS_INFO pInfo = (PLSA_ENUM_GROUPS_INFO)hResume;
    
    BAIL_ON_INVALID_HANDLE(hLsaConnection);
    BAIL_ON_INVALID_HANDLE(hResume);
    BAIL_ON_INVALID_POINTER(pdwNumGroupsFound);
    BAIL_ON_INVALID_POINTER(pppGroupInfoList);
    
    dwGroupInfoLevel = pInfo->dwGroupInfoLevel;

    dwError = LsaMarshalEnumRecordsToken(
                    pInfo->pszGUID,
                    NULL,
                    &dwMsgLen);
    BAIL_ON_LSA_ERROR(dwError);
        
    dwError = LsaBuildMessage(
                LSA_Q_ENUM_GROUPS,
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
        case LSA_R_ENUM_GROUPS:
        {
            dwError = LsaUnmarshalGroupInfoList(
                                pMessage->pData,
                                pMessage->header.messageLength,
                                &dwGroupInfoLevel,
                                &ppGroupInfoList,
                                &dwNumGroupsFound
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
    
    *pdwNumGroupsFound = dwNumGroupsFound;
    *pppGroupInfoList = ppGroupInfoList;

cleanup:

    LSA_SAFE_FREE_MESSAGE(pMessage);
    LSA_SAFE_FREE_STRING(pszError);

    return dwError;
        
error:

    if (ppGroupInfoList) {
        LsaFreeGroupInfoList(dwGroupInfoLevel, ppGroupInfoList, dwNumGroupsFound);
    }
    
    if (pppGroupInfoList) {
        *pppGroupInfoList = NULL;
    }
    
    if (pdwNumGroupsFound) {
        *pdwNumGroupsFound = 0;
    }

    goto cleanup;
}

LSASS_API
DWORD
LsaEndEnumGroups(
    HANDLE hLsaConnection,
    HANDLE hResume
    )
{
    DWORD dwError = 0;
    PLSA_ENUM_GROUPS_INFO pInfo = (PLSA_ENUM_GROUPS_INFO)hResume;
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
                LSA_Q_END_ENUM_GROUPS,
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
        case LSA_R_END_ENUM_GROUPS:
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
    
    LsaFreeEnumGroupsInfo(pInfo);
    
cleanup:

    LSA_SAFE_FREE_MESSAGE(pMessage);

    return dwError;
    
error:

    goto cleanup;
}

LSASS_API
DWORD
LsaDeleteGroupById(
    HANDLE hLsaConnection,
    gid_t  gid
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
    
    dwError = LsaMarshalDeleteGroupByIdQuery(
                gid,
                NULL,
                &dwMsgLen);
    BAIL_ON_LSA_ERROR(dwError);
        
    dwError = LsaBuildMessage(
                LSA_Q_DELETE_GROUP,
                dwMsgLen,
                1,
                1,
                &pMessage);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaMarshalDeleteGroupByIdQuery(
                gid,
                pMessage->pData,
                &dwMsgLen);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaSendMessage(hLsaConnection, pMessage);
    BAIL_ON_LSA_ERROR(dwError);
    
    LSA_SAFE_FREE_MESSAGE(pMessage);
    
    dwError = LsaGetNextMessage(hLsaConnection, &pMessage);
    BAIL_ON_LSA_ERROR(dwError);
    
    switch (pMessage->header.messageType) {
        case LSA_R_DELETE_GROUP:
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
LsaDeleteGroupByName(
    HANDLE hLsaConnection,
    PCSTR  pszName
    )
{
    DWORD dwError = 0;
    PVOID pGroupInfo = NULL;
    DWORD dwGroupInfoLevel = 0;

    if (geteuid() != 0)
    {
        dwError = EACCES;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    BAIL_ON_INVALID_HANDLE(hLsaConnection);
    BAIL_ON_INVALID_STRING(pszName);
    
    dwError = LsaValidateGroupName(pszName);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaFindGroupByName(
                    hLsaConnection,
                    pszName,
                    dwGroupInfoLevel,
                    &pGroupInfo);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaDeleteGroupById(
                    hLsaConnection,
                    ((PLSA_GROUP_INFO_0)pGroupInfo)->gid);
    BAIL_ON_LSA_ERROR(dwError);
    
cleanup:

    if (pGroupInfo) {
        LsaFreeGroupInfo(dwGroupInfoLevel, pGroupInfo);
    }

    return dwError;
    
error:

    goto cleanup;
}

LSASS_API
DWORD
LsaGetGroupsForUserName(
    HANDLE  hLsaConnection,
    PCSTR   pszUserName,
    PDWORD  pdwGroupFound,
    gid_t** ppGidResults
    )
{    
    DWORD dwError = 0;
    PVOID pUserInfo = NULL;
    DWORD dwUserInfoLevel = 0;
    DWORD dwGroupInfoLevel = 0;
    DWORD dwGroupFound = 0;
    gid_t* pGidResult = NULL;
    PLSAMESSAGE pMessage = NULL;
    DWORD dwMsgLen = 0;
    PSTR  pszError = NULL;
    PVOID*  ppGroupInfoList = NULL;
    DWORD iGroup = 0;

    BAIL_ON_INVALID_HANDLE(hLsaConnection);    
    BAIL_ON_INVALID_STRING(pszUserName);   
    BAIL_ON_INVALID_POINTER(ppGidResults);
    
    dwError = LsaValidateGroupInfoLevel(dwGroupInfoLevel);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaValidateUserName(pszUserName);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaFindUserByName(
                  hLsaConnection,
                  pszUserName,
                  dwUserInfoLevel,
                  &pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);  
    
    dwError = LsaMarshalGetGroupsForUserQuery(
                ((PLSA_USER_INFO_0)pUserInfo)->uid,
                dwGroupInfoLevel,
                NULL,
                &dwMsgLen);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaBuildMessage(
                LSA_Q_GROUPS_FOR_USER,
                dwMsgLen,
                1,
                1,
                &pMessage);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaMarshalGetGroupsForUserQuery(
                ((PLSA_USER_INFO_0)pUserInfo)->uid,
                dwGroupInfoLevel,
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
        case LSA_R_GROUPS_FOR_USER:
        {
            dwError = LsaUnmarshalGroupInfoList(
                                pMessage->pData,
                                pMessage->header.messageLength,
                                &dwGroupInfoLevel,
                                &ppGroupInfoList,
                                &dwGroupFound
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
    
    dwError = LsaAllocateMemory(
                    sizeof(gid_t) * dwGroupFound,
                    (PVOID*)&pGidResult);
    BAIL_ON_LSA_ERROR(dwError);
    
    switch(dwGroupInfoLevel)
    {
        case 0:
            
            for (iGroup = 0; iGroup < dwGroupFound; iGroup++) {
                   *(pGidResult+iGroup) = ((PLSA_GROUP_INFO_0)(*(ppGroupInfoList+iGroup)))->gid;                
            }
            
            break;
            
        default:
            dwError = LSA_ERROR_UNSUPPORTED_GROUP_LEVEL;
            BAIL_ON_LSA_ERROR(dwError);
            break;
    }
    
    *ppGidResults = pGidResult;
    *pdwGroupFound = dwGroupFound;

cleanup:

    if (pUserInfo) {
        LsaFreeUserInfo(dwUserInfoLevel, pUserInfo);
    }
    
    if (ppGroupInfoList) {
        LsaFreeGroupInfoList(dwGroupInfoLevel, (PVOID*)ppGroupInfoList, dwGroupFound);
    }
    
    LSA_SAFE_FREE_MESSAGE(pMessage);
    LSA_SAFE_FREE_STRING(pszError);

    return dwError;
        
error:

    *ppGidResults = NULL;
    *pdwGroupFound = 0;

    goto cleanup;   
}

VOID
LsaFreeEnumGroupsInfo(
    PLSA_ENUM_GROUPS_INFO pInfo
    )
{
    LSA_SAFE_FREE_STRING(pInfo->pszGUID);
    LsaFreeMemory(pInfo);
}
