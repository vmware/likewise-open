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

LWMsgStatus
LsaSrvIpcAddGroup(
    LWMsgAssoc* assoc,
    const LWMsgMessage* pRequest,
    LWMsgMessage* pResponse,
    void* data
    )
{
    DWORD dwError = 0;
    PLSA_IPC_ADD_GROUP_INFO_REQ pReq = pRequest->object;
    PLSA_IPC_ERROR pError = NULL;
    // Do not free pGroupInfoList
    PLSA_GROUP_INFO_LIST pGroupInfoList = pReq->pGroupInfoList;

    switch (pGroupInfoList->dwGroupInfoLevel)
    {
        case 0:
            dwError = LsaSrvAddGroup(
                            (HANDLE)pReq->Handle,
                            0,
                            pGroupInfoList->ppGroupInfoList.ppInfoList0[0]);
            break;
        case 1:
            dwError = LsaSrvAddGroup(
                            (HANDLE)pReq->Handle,
                            1,
                            pGroupInfoList->ppGroupInfoList.ppInfoList1[0]);
            break;
        default:
            dwError = LSA_ERROR_INVALID_PARAMETER;
    }

    if (!dwError)
    {
        pResponse->tag = LSA_R_ADD_GROUP_SUCCESS;
        pResponse->object = NULL;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pResponse->tag = LSA_R_ADD_GROUP_FAILURE;;
        pResponse->object = pError;
    }

cleanup:
    return MAP_LSA_ERROR_IPC(dwError);

error:
    goto cleanup;
}

LWMsgStatus
LsaSrvIpcFindGroupByName(
    LWMsgAssoc* assoc,
    const LWMsgMessage* pRequest,
    LWMsgMessage* pResponse,
    void* data
    )
{
    DWORD dwError = 0;
    // Do not free pGroupInfo
    PVOID pGroupInfo = NULL;
    PVOID* ppGroupInfoList = NULL;
    PLSA_GROUP_INFO_LIST pResult = NULL;
    PLSA_IPC_FIND_OBJECT_BY_NAME_REQ pReq = pRequest->object;
    PLSA_IPC_ERROR pError = NULL;

    dwError = LsaAllocateMemory(sizeof(*pResult),
                                (PVOID)&pResult);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvFindGroupByName(
                       (HANDLE)pReq->Handle,
                       pReq->pszName,
                       pReq->FindFlags,
                       pReq->dwInfoLevel,
                       &pGroupInfo);

    if (!dwError)
    {
        pResult->dwGroupInfoLevel = pReq->dwInfoLevel;
        pResult->dwNumGroups = 1;
        dwError = LsaAllocateMemory(
                        sizeof(*ppGroupInfoList) * 1,
                        (PVOID*)&ppGroupInfoList);
        BAIL_ON_LSA_ERROR(dwError);

        ppGroupInfoList[0] = pGroupInfo;
        pGroupInfo = NULL;

        switch (pResult->dwGroupInfoLevel)
        {
            case 0:
                pResult->ppGroupInfoList.ppInfoList0 = (PLSA_GROUP_INFO_0*)ppGroupInfoList;
                ppGroupInfoList = NULL;
                break;
            case 1:
                pResult->ppGroupInfoList.ppInfoList1 = (PLSA_GROUP_INFO_1*)ppGroupInfoList;
                ppGroupInfoList = NULL;
                break;
            default:
                dwError = LSA_ERROR_INVALID_PARAMETER;
                BAIL_ON_LSA_ERROR(dwError);
        }

        pResponse->tag = LSA_R_GROUP_BY_NAME_SUCCESS;
        pResponse->object = pResult;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pResponse->tag = LSA_R_GROUP_BY_NAME_FAILURE;;
        pResponse->object = pError;
    }

cleanup:
    if (pGroupInfo)
    {
        LsaFreeGroupInfo(pReq->dwInfoLevel, pGroupInfo);
    }
    if(ppGroupInfoList)
    {
        LsaFreeGroupInfoList(pReq->dwInfoLevel, ppGroupInfoList, 1);
    }

    return MAP_LSA_ERROR_IPC(dwError);

error:
    if(pResult)
    {
        LsaFreeIpcGroupInfoList(pResult);
    }

    goto cleanup;
}

LWMsgStatus
LsaSrvIpcFindGroupById(
    LWMsgAssoc* assoc,
    const LWMsgMessage* pRequest,
    LWMsgMessage* pResponse,
    void* data
    )
{
    DWORD dwError = 0;
    PVOID pGroupInfo = NULL;
    PVOID* ppGroupInfoList = NULL;
    PLSA_GROUP_INFO_LIST pResult = NULL;
    PLSA_IPC_FIND_OBJECT_BY_ID_REQ pReq = pRequest->object;
    PLSA_IPC_ERROR pError = NULL;

    dwError = LsaAllocateMemory(sizeof(*pResult),
                                (PVOID)&pResult);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvFindGroupById(
                       (HANDLE)pReq->Handle,
                       pReq->id,
                       pReq->FindFlags,
                       pReq->dwInfoLevel,
                       &pGroupInfo);

    if (!dwError)
    {
        pResult->dwGroupInfoLevel = pReq->dwInfoLevel;
        pResult->dwNumGroups = 1;
        dwError = LsaAllocateMemory(
                        sizeof(*ppGroupInfoList) * 1,
                        (PVOID*)&ppGroupInfoList);
        BAIL_ON_LSA_ERROR(dwError);

        ppGroupInfoList[0] = pGroupInfo;
        pGroupInfo = NULL;

        switch (pResult->dwGroupInfoLevel)
        {
            case 0:
                pResult->ppGroupInfoList.ppInfoList0 = (PLSA_GROUP_INFO_0*)ppGroupInfoList;
                ppGroupInfoList = NULL;
                break;
            case 1:
                pResult->ppGroupInfoList.ppInfoList1 = (PLSA_GROUP_INFO_1*)ppGroupInfoList;
                ppGroupInfoList = NULL;
                break;
            default:
                dwError = LSA_ERROR_INVALID_PARAMETER;
                BAIL_ON_LSA_ERROR(dwError);
        }

        pResponse->tag = LSA_R_GROUP_BY_ID_SUCCESS;
        pResponse->object = pResult;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pResponse->tag = LSA_R_GROUP_BY_ID_FAILURE;;
        pResponse->object = pError;
    }

cleanup:
    if (pGroupInfo)
    {
        LsaFreeGroupInfo(pReq->dwInfoLevel, pGroupInfo);
    }
    if(ppGroupInfoList)
    {
        LsaFreeGroupInfoList(pReq->dwInfoLevel, ppGroupInfoList, 1);
    }

    return MAP_LSA_ERROR_IPC(dwError);

error:
    if(pResult)
    {
        LsaFreeIpcGroupInfoList(pResult);
    }

    goto cleanup;
}

LWMsgStatus
LsaSrvIpcGetGroupsForUser(
    LWMsgAssoc* assoc,
    const LWMsgMessage* pRequest,
    LWMsgMessage* pResponse,
    void* data
    )
{
    DWORD dwError = 0;
    PVOID* ppGroupInfoList = NULL;
    DWORD dwNumGroupsFound = 0;
    PLSA_GROUP_INFO_LIST pResult = NULL;
    PLSA_IPC_FIND_OBJECT_BY_ID_REQ pReq = pRequest->object;
    PLSA_IPC_ERROR pError = NULL;

    dwError = LsaAllocateMemory(sizeof(*pResult),
                                (PVOID)&pResult);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvGetGroupsForUser(
                       (HANDLE)pReq->Handle,
                       pReq->id,
                       pReq->FindFlags,
                       pReq->dwInfoLevel,
                       &dwNumGroupsFound,
                       &ppGroupInfoList);

    if (!dwError)
    {
        pResult->dwGroupInfoLevel = pReq->dwInfoLevel;
        pResult->dwNumGroups = dwNumGroupsFound;

        switch (pResult->dwGroupInfoLevel)
        {
            case 0:
                pResult->ppGroupInfoList.ppInfoList0 = (PLSA_GROUP_INFO_0*)ppGroupInfoList;
                ppGroupInfoList = NULL;
                break;
            case 1:
                pResult->ppGroupInfoList.ppInfoList1 = (PLSA_GROUP_INFO_1*)ppGroupInfoList;
                ppGroupInfoList = NULL;
                break;
            default:
                dwError = LSA_ERROR_INVALID_PARAMETER;
                BAIL_ON_LSA_ERROR(dwError);
        }

        pResponse->tag = LSA_R_GROUPS_FOR_USER_SUCCESS;
        pResponse->object = pResult;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pResponse->tag = LSA_R_GROUPS_FOR_USER_FAILURE;
        pResponse->object = pError;
    }

cleanup:
    if(ppGroupInfoList)
    {
        LsaFreeGroupInfoList(pReq->dwInfoLevel, ppGroupInfoList, dwNumGroupsFound);
    }

    return MAP_LSA_ERROR_IPC(dwError);

error:
    if(pResult)
    {
        LsaFreeIpcGroupInfoList(pResult);
    }

    goto cleanup;
}

LWMsgStatus
LsaSrvIpcBeginEnumGroups(
    LWMsgAssoc* assoc,
    const LWMsgMessage* pRequest,
    LWMsgMessage* pResponse,
    void* data
    )
{
    DWORD dwError = 0;
    PSTR pszGUID = NULL;
    PLSA_ENUM_OBJECTS_INFO pResult = NULL;
    PLSA_IPC_BEGIN_ENUM_RECORDS_REQ pReq = pRequest->object;
    PLSA_IPC_ERROR pError = NULL;

    dwError = LsaAllocateMemory(sizeof(*pResult),
                                (PVOID)&pResult);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvBeginEnumGroups(
                        (HANDLE)pReq->Handle,
                        pReq->dwInfoLevel,
                        pReq->dwNumMaxRecords,
                        &pszGUID);

    if (!dwError)
    {
        pResult->dwObjectInfoLevel = pReq->dwInfoLevel;
        pResult->dwNumMaxObjects = pReq->dwNumMaxRecords;
        pResult->pszGUID = pszGUID;
        pszGUID = NULL;

        pResponse->tag = LSA_R_BEGIN_ENUM_GROUPS_SUCCESS;
        pResponse->object = pResult;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pResponse->tag = LSA_R_BEGIN_ENUM_GROUPS_FAILURE;
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
LsaSrvIpcEnumGroups(
    LWMsgAssoc* assoc,
    const LWMsgMessage* pRequest,
    LWMsgMessage* pResponse,
    void* data
    )
{
    DWORD dwError = 0;
    PVOID* ppGroupInfoList = NULL;
    DWORD  dwGroupInfoLevel = 0;
    DWORD  dwNumGroupsFound = 0;
    PLSA_GROUP_INFO_LIST pResult = NULL;
    PLSA_IPC_ENUM_RECORDS_REQ pReq = pRequest->object;
    PLSA_IPC_ERROR pError = NULL;

    dwError = LsaSrvEnumGroups(
                       (HANDLE)pReq->Handle,
                       pReq->pszToken,
                       &dwGroupInfoLevel,
                       &ppGroupInfoList,
                       &dwNumGroupsFound);

    if (!dwError)
    {
        dwError = LsaAllocateMemory(sizeof(*pResult),
                                    (PVOID)&pResult);
        BAIL_ON_LSA_ERROR(dwError);

        pResult->dwGroupInfoLevel = dwGroupInfoLevel;
        pResult->dwNumGroups = dwNumGroupsFound;
        switch (pResult->dwGroupInfoLevel)
        {
            case 0:
                pResult->ppGroupInfoList.ppInfoList0 = (PLSA_GROUP_INFO_0*)ppGroupInfoList;
                ppGroupInfoList = NULL;
                break;

            case 1:
                pResult->ppGroupInfoList.ppInfoList1 = (PLSA_GROUP_INFO_1*)ppGroupInfoList;
                ppGroupInfoList = NULL;
                break;

            default:
                dwError = LSA_ERROR_INVALID_PARAMETER;
                BAIL_ON_LSA_ERROR(dwError);
        }

        pResponse->tag = LSA_R_ENUM_GROUPS_SUCCESS;
        pResponse->object = pResult;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pResponse->tag = LSA_R_ENUM_GROUPS_FAILURE;;
        pResponse->object = pError;
    }

cleanup:
    if(ppGroupInfoList)
    {
        LsaFreeGroupInfoList(dwGroupInfoLevel, ppGroupInfoList, dwNumGroupsFound);
    }

    return MAP_LSA_ERROR_IPC(dwError);

error:
    if(pResult)
    {
        LsaFreeIpcGroupInfoList(pResult);
    }

    goto cleanup;
}

LWMsgStatus
LsaSrvIpcEndEnumGroups(
    LWMsgAssoc* assoc,
    const LWMsgMessage* pRequest,
    LWMsgMessage* pResponse,
    void* data
    )
{
    DWORD dwError = 0;
    PLSA_IPC_ENUM_RECORDS_REQ pReq = pRequest->object;
    PLSA_IPC_ERROR pError = NULL;

    dwError = LsaSrvEndEnumGroups(
                        (HANDLE)pReq->Handle,
                        pReq->pszToken);

    if (!dwError)
    {
        pResponse->tag = LSA_R_END_ENUM_GROUPS_SUCCESS;
        pResponse->object = NULL;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pResponse->tag = LSA_R_END_ENUM_GROUPS_FAILURE;
        pResponse->object = pError;
    }

cleanup:
    return MAP_LSA_ERROR_IPC(dwError);

error:
    goto cleanup;
}

LWMsgStatus
LsaSrvIpcDeleteGroup(
    LWMsgAssoc* assoc,
    const LWMsgMessage* pRequest,
    LWMsgMessage* pResponse,
    void* data
    )
{
    DWORD dwError = 0;
    PLSA_IPC_DEL_OBJECT_INFO_REQ pReq = pRequest->object;
    PLSA_IPC_ERROR pError = NULL;

    dwError = LsaSrvDeleteGroup(
                        (HANDLE)pReq->Handle,
                        pReq->dwId);

    if (!dwError)
    {
        pResponse->tag = LSA_R_DELETE_GROUP_SUCCESS;
        pResponse->object = NULL;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pResponse->tag = LSA_R_DELETE_GROUP_FAILURE;
        pResponse->object = pError;
    }

cleanup:
    return MAP_LSA_ERROR_IPC(dwError);

error:
    goto cleanup;
}
