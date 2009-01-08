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

LWMsgStatus
LsaSrvIpcAddUser(
    LWMsgAssoc* assoc,
    const LWMsgMessage* pRequest,
    LWMsgMessage* pResponse,
    void* data
    )
{
    DWORD dwError = 0;
    PLSA_IPC_ERROR pError = NULL;
    // Do not free pUserInfoList
    PLSA_USER_INFO_LIST pUserInfoList = (PLSA_USER_INFO_LIST)pRequest->object;
    PVOID Handle = NULL;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_get_session_data(assoc, (PVOID*) (PVOID) &Handle));
    BAIL_ON_LSA_ERROR(dwError);

    switch (pUserInfoList->dwUserInfoLevel)
    {
        case 0:
            dwError = LsaSrvAddUser(
                            (HANDLE)Handle,
                            0,
                            pUserInfoList->ppUserInfoList.ppInfoList0[0]);
            break;
        case 1:
            dwError = LsaSrvAddUser(
                            (HANDLE)Handle,
                            1,
                            pUserInfoList->ppUserInfoList.ppInfoList1[0]);
            break;
        case 2:
            dwError = LsaSrvAddUser(
                            (HANDLE)Handle,
                            2,
                            pUserInfoList->ppUserInfoList.ppInfoList2[0]);
            break;
        default:
            dwError = LSA_ERROR_INVALID_PARAMETER;
    }

    if (!dwError)
    {
        pResponse->tag = LSA_R_ADD_USER_SUCCESS;
        pResponse->object = NULL;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pResponse->tag = LSA_R_ADD_USER_FAILURE;;
        pResponse->object = pError;
    }

cleanup:
    return MAP_LSA_ERROR_IPC(dwError);

error:
    goto cleanup;
}

DWORD
LsaSrvIpcModifyUser(
    LWMsgAssoc* assoc,
    const LWMsgMessage* pRequest,
    LWMsgMessage* pResponse,
    void* data
    )
{
    DWORD dwError = 0;
    PLSA_IPC_ERROR pError = NULL;
    PVOID Handle = NULL;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_get_session_data(assoc, (PVOID*) (PVOID) &Handle));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvModifyUser(
                    (HANDLE)Handle,
                    (PLSA_USER_MOD_INFO)pRequest->object);

    if (!dwError)
    {
        pResponse->tag = LSA_R_MODIFY_USER_SUCCESS;
        pResponse->object = NULL;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pResponse->tag = LSA_R_MODIFY_USER_FAILURE;
        pResponse->object = pError;
    }

cleanup:
    return MAP_LSA_ERROR_IPC(dwError);

error:
    goto cleanup;
}

LWMsgStatus
LsaSrvIpcFindUserByName(
    LWMsgAssoc* assoc,
    const LWMsgMessage* pRequest,
    LWMsgMessage* pResponse,
    void* data
    )
{
    DWORD dwError = 0;
    // Do not free pUserInfo
    PVOID pUserInfo = NULL;
    PVOID* ppUserInfoList = NULL;
    PLSA_USER_INFO_LIST pResult = NULL;
    PLSA_IPC_FIND_OBJECT_BY_NAME_REQ pReq = pRequest->object;
    PLSA_IPC_ERROR pError = NULL;
    PVOID Handle = NULL;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_get_session_data(assoc, (PVOID*) (PVOID) &Handle));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvFindUserByName(
                       (HANDLE)Handle,
                       pReq->pszName,
                       pReq->dwInfoLevel,
                       &pUserInfo);

    if (!dwError)
    {
        dwError = LsaAllocateMemory(sizeof(*pResult),
                                        (PVOID)&pResult);
        BAIL_ON_LSA_ERROR(dwError);

        pResult->dwUserInfoLevel = pReq->dwInfoLevel;
        pResult->dwNumUsers = 1;
        dwError = LsaAllocateMemory(
                        sizeof(*ppUserInfoList) * 1,
                        (PVOID*)&ppUserInfoList);
        BAIL_ON_LSA_ERROR(dwError);

        ppUserInfoList[0] = pUserInfo;
        pUserInfo = NULL;

        switch (pResult->dwUserInfoLevel)
        {
            case 0:
                pResult->ppUserInfoList.ppInfoList0 = (PLSA_USER_INFO_0*)ppUserInfoList;
                ppUserInfoList = NULL;
                break;
            case 1:
                pResult->ppUserInfoList.ppInfoList1 = (PLSA_USER_INFO_1*)ppUserInfoList;
                ppUserInfoList = NULL;
                break;
            case 2:
                pResult->ppUserInfoList.ppInfoList2 = (PLSA_USER_INFO_2*)ppUserInfoList;
                ppUserInfoList = NULL;
                break;
            default:
                dwError = LSA_ERROR_INVALID_PARAMETER;
                BAIL_ON_LSA_ERROR(dwError);
        }

        pResponse->tag = LSA_R_USER_BY_NAME_SUCCESS;
        pResponse->object = pResult;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pResponse->tag = LSA_R_USER_BY_NAME_FAILURE;
        pResponse->object = pError;
    }

cleanup:
    if (pUserInfo)
    {
        LsaFreeUserInfo(pReq->dwInfoLevel, pUserInfo);
    }
    if(ppUserInfoList)
    {
        LsaFreeUserInfoList(pReq->dwInfoLevel, ppUserInfoList, 1);
    }

    return MAP_LSA_ERROR_IPC(dwError);

error:
    if(pResult)
    {
        LsaFreeIpcUserInfoList(pResult);
    }

    goto cleanup;
}

LWMsgStatus
LsaSrvIpcFindUserById(
    LWMsgAssoc* assoc,
    const LWMsgMessage* pRequest,
    LWMsgMessage* pResponse,
    void* data
    )
{
    DWORD dwError = 0;
    // Do not free pUserInfo
    PVOID pUserInfo = NULL;
    PVOID* ppUserInfoList = NULL;
    PLSA_USER_INFO_LIST pResult = NULL;
    PLSA_IPC_FIND_OBJECT_BY_ID_REQ pReq = pRequest->object;
    PLSA_IPC_ERROR pError = NULL;
    PVOID Handle = NULL;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_get_session_data(assoc, (PVOID*) (PVOID) &Handle));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvFindUserById(
                       (HANDLE)Handle,
                       pReq->id,
                       pReq->dwInfoLevel,
                       &pUserInfo);

    if (!dwError)
    {
        dwError = LsaAllocateMemory(sizeof(*pResult),
                                        (PVOID)&pResult);
        BAIL_ON_LSA_ERROR(dwError);

        pResult->dwUserInfoLevel = pReq->dwInfoLevel;
        pResult->dwNumUsers = 1;
        dwError = LsaAllocateMemory(
                        sizeof(*ppUserInfoList) * 1,
                        (PVOID*)&ppUserInfoList);
        BAIL_ON_LSA_ERROR(dwError);

        ppUserInfoList[0] = pUserInfo;
        pUserInfo = NULL;

        switch (pResult->dwUserInfoLevel)
        {
            case 0:
                pResult->ppUserInfoList.ppInfoList0 = (PLSA_USER_INFO_0*)ppUserInfoList;
                ppUserInfoList = NULL;
                break;
            case 1:
                pResult->ppUserInfoList.ppInfoList1 = (PLSA_USER_INFO_1*)ppUserInfoList;
                ppUserInfoList = NULL;
                break;
            case 2:
                pResult->ppUserInfoList.ppInfoList2 = (PLSA_USER_INFO_2*)ppUserInfoList;
                ppUserInfoList = NULL;
                break;
            default:
                dwError = LSA_ERROR_INVALID_PARAMETER;
                BAIL_ON_LSA_ERROR(dwError);
        }

        pResponse->tag = LSA_R_USER_BY_ID_SUCCESS;
        pResponse->object = pResult;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pResponse->tag = LSA_R_USER_BY_ID_FAILURE;
        pResponse->object = pError;
    }

cleanup:
    if (pUserInfo)
    {
        LsaFreeUserInfo(pReq->dwInfoLevel, pUserInfo);
    }
    if(ppUserInfoList)
    {
        LsaFreeUserInfoList(pReq->dwInfoLevel, ppUserInfoList, 1);
    }

    return MAP_LSA_ERROR_IPC(dwError);

error:
    if(pResult)
    {
        LsaFreeIpcUserInfoList(pResult);
    }

    goto cleanup;
}

LWMsgStatus
LsaSrvIpcBeginEnumUsers(
    LWMsgAssoc* assoc,
    const LWMsgMessage* pRequest,
    LWMsgMessage* pResponse,
    void* data
    )
{
    DWORD dwError = 0;
    PSTR pszGUID = NULL;
    PLSA_ENUM_OBJECTS_INFO pResult = NULL;
    PLSA_IPC_BEGIN_ENUM_USERS_REQ pReq = pRequest->object;
    PLSA_IPC_ERROR pError = NULL;
    PVOID Handle = NULL;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_get_session_data(assoc, (PVOID*) (PVOID) &Handle));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvBeginEnumUsers(
                        (HANDLE)Handle,
                        (HANDLE)pReq->Handle,
                        pReq->dwInfoLevel,
                        pReq->dwNumMaxRecords,
                        &pszGUID);

    if (!dwError)
    {
        dwError = LsaAllocateMemory(sizeof(*pResult),
                                        (PVOID)&pResult);
        BAIL_ON_LSA_ERROR(dwError);

        pResult->dwObjectInfoLevel = pReq->dwInfoLevel;
        pResult->dwNumMaxObjects = pReq->dwNumMaxRecords;
        pResult->pszGUID = pszGUID;
        pszGUID = NULL;

        pResponse->tag = LSA_R_BEGIN_ENUM_USERS_SUCCESS;
        pResponse->object = pResult;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pResponse->tag = LSA_R_BEGIN_ENUM_USERS_FAILURE;
        pResponse->object = pError;
    }

cleanup:
    LSA_SAFE_FREE_STRING(pszGUID);

    return MAP_LSA_ERROR_IPC(dwError);

error:
    if(pResult)
    {
        LsaFreeEnumObjectsInfo(pResult);
    }

    goto cleanup;
}

LWMsgStatus
LsaSrvIpcEnumUsers(
    LWMsgAssoc* assoc,
    const LWMsgMessage* pRequest,
    LWMsgMessage* pResponse,
    void* data
    )
{
    DWORD dwError = 0;
    PVOID* ppUserInfoList = NULL;
    DWORD  dwUserInfoLevel = 0;
    DWORD  dwNumUsersFound = 0;
    PLSA_USER_INFO_LIST pResult = NULL;
    PLSA_IPC_ENUM_RECORDS_REQ pReq = pRequest->object;
    PLSA_IPC_ERROR pError = NULL;

    dwError = LsaSrvEnumUsers(
                       (HANDLE)pReq->Handle,
                       pReq->pszToken,
                       &dwUserInfoLevel,
                       &ppUserInfoList,
                       &dwNumUsersFound);

    if (!dwError)
    {
        dwError = LsaAllocateMemory(sizeof(*pResult),
                                   (PVOID)&pResult);
        BAIL_ON_LSA_ERROR(dwError);

        pResult->dwUserInfoLevel = dwUserInfoLevel;
        pResult->dwNumUsers = dwNumUsersFound;
        switch (pResult->dwUserInfoLevel)
        {
            case 0:
                pResult->ppUserInfoList.ppInfoList0 = (PLSA_USER_INFO_0*)ppUserInfoList;
                ppUserInfoList = NULL;
                break;

            case 1:
                pResult->ppUserInfoList.ppInfoList1 = (PLSA_USER_INFO_1*)ppUserInfoList;
                ppUserInfoList = NULL;
                break;

            case 2:
                pResult->ppUserInfoList.ppInfoList2 = (PLSA_USER_INFO_2*)ppUserInfoList;
                ppUserInfoList = NULL;
                break;

            default:
                dwError = LSA_ERROR_INVALID_PARAMETER;
                BAIL_ON_LSA_ERROR(dwError);
        }

        pResponse->tag = LSA_R_ENUM_USERS_SUCCESS;
        pResponse->object = pResult;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pResponse->tag = LSA_R_ENUM_USERS_FAILURE;;
        pResponse->object = pError;
    }

cleanup:
    if(ppUserInfoList)
    {
        LsaFreeUserInfoList(dwUserInfoLevel, ppUserInfoList, dwNumUsersFound);
    }

    return MAP_LSA_ERROR_IPC(dwError);

error:
    if(pResult)
    {
        LsaFreeIpcUserInfoList(pResult);
    }

    goto cleanup;
}

LWMsgStatus
LsaSrvIpcEndEnumUsers(
    LWMsgAssoc* assoc,
    const LWMsgMessage* pRequest,
    LWMsgMessage* pResponse,
    void* data
    )
{
    DWORD dwError = 0;
    PLSA_IPC_ENUM_RECORDS_REQ pReq = pRequest->object;
    PLSA_IPC_ERROR pError = NULL;

    dwError = LsaSrvEndEnumUsers(
                        (HANDLE)pReq->Handle,
                        pReq->pszToken);

    if (!dwError)
    {
        pResponse->tag = LSA_R_END_ENUM_USERS_SUCCESS;
        pResponse->object = NULL;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pResponse->tag = LSA_R_END_ENUM_USERS_FAILURE;
        pResponse->object = pError;
    }

cleanup:
    return MAP_LSA_ERROR_IPC(dwError);

error:
    goto cleanup;
}

LWMsgStatus
LsaSrvIpcDeleteUser(
    LWMsgAssoc* assoc,
    const LWMsgMessage* pRequest,
    LWMsgMessage* pResponse,
    void* data
    )
{
    DWORD dwError = 0;
    PLSA_IPC_ERROR pError = NULL;
    PVOID Handle = NULL;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_get_session_data(assoc, (PVOID*) (PVOID) &Handle));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvDeleteUser(
                        (HANDLE)Handle,
                        *((PDWORD)pRequest->object));

    if (!dwError)
    {
        pResponse->tag = LSA_R_DELETE_USER_SUCCESS;
        pResponse->object = NULL;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pResponse->tag = LSA_R_DELETE_USER_FAILURE;
        pResponse->object = pError;
    }

cleanup:
    return MAP_LSA_ERROR_IPC(dwError);

error:
    goto cleanup;
}

DWORD
LsaSrvIpcGetNamesBySidList(
    LWMsgAssoc* assoc,
    const LWMsgMessage* pRequest,
    LWMsgMessage* pResponse,
    void* data
    )
{
    DWORD dwError = 0;
    PSTR* ppszDomainNames = NULL;
    PSTR* ppszSamAccounts = NULL;
    ADAccountType* pTypes = NULL;
    CHAR chDomainSeparator = 0;
    PLSA_FIND_NAMES_BY_SIDS pResult = NULL;
    PLSA_IPC_NAMES_BY_SIDS_REQ pReq = pRequest->object;
    PLSA_IPC_ERROR pError = NULL;
    DWORD i = 0;
    PVOID Handle = NULL;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_get_session_data(assoc, (PVOID*) (PVOID) &Handle));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvGetNamesBySidList(
                    (HANDLE)Handle,
                    pReq->sCount,
                    pReq->ppszSidList,
                    &ppszDomainNames,
                    &ppszSamAccounts,
                    &pTypes,
                    &chDomainSeparator);

    if (!dwError)
    {
        dwError = LsaAllocateMemory(sizeof(*pResult),
                                    (PVOID)&pResult);
        BAIL_ON_LSA_ERROR(dwError);

        pResult->sCount = pReq->sCount;
        pResult->chDomainSeparator = chDomainSeparator;

        dwError = LsaAllocateMemory(sizeof(*(pResult->pSIDInfoList)) * pResult->sCount,
                                    (PVOID*)&pResult->pSIDInfoList);
        BAIL_ON_LSA_ERROR(dwError);

        for (i = 0; i < pResult->sCount; i++)
        {
            pResult->pSIDInfoList[i].accountType = pTypes[i];
            pResult->pSIDInfoList[i].pszDomainName = ppszDomainNames[i];
            ppszDomainNames[i] = NULL;
            pResult->pSIDInfoList[i].pszSamAccountName = ppszSamAccounts[i];
            ppszSamAccounts[i] = NULL;
        }

        pResponse->tag = LSA_R_NAMES_BY_SID_LIST_SUCCESS;
        pResponse->object = pResult;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pResponse->tag = LSA_R_NAMES_BY_SID_LIST_FAILURE;
        pResponse->object = pError;
    }

cleanup:
    LsaFreeStringArray(ppszDomainNames, pResult->sCount);
    LsaFreeStringArray(ppszSamAccounts, pResult->sCount);
    LSA_SAFE_FREE_MEMORY(pTypes);

    return MAP_LSA_ERROR_IPC(dwError);

error:
    if(pResult)
    {
        LsaFreeIpcNameSidsList(pResult);
    }

    goto cleanup;
}
