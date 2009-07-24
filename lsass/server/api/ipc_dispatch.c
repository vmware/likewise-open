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
 *        ipc_dispatch.c
 *
 * Abstract:
 *
 *        Likewise Security and Authorization Subsystem (LSASS)
 *
 *        Server IPC dispatch table
 *
 * Authors: Brian Koropoff (bkoropoff@likewise.com)
 *
 */

#include "api.h"

static
DWORD
LsaSrvIpcRegisterHandle(
    LWMsgCall* pCall,
    PCSTR pszHandleType,
    PVOID pHandle,
    LWMsgHandleCleanupFunction pfnCleanup
    )
{
    DWORD dwError = 0;
    LWMsgSession* pSession = lwmsg_call_get_session(pCall);

    dwError = MAP_LWMSG_ERROR(lwmsg_session_register_handle(pSession, pszHandleType, pHandle, pfnCleanup));
    BAIL_ON_LSA_ERROR(dwError);

error:

    return dwError;
}

static
DWORD
LsaSrvIpcRetainHandle(
    LWMsgCall* pCall,
    PVOID pHandle
    )
{
    DWORD dwError = 0;
    LWMsgSession* pSession = lwmsg_call_get_session(pCall);

    dwError = MAP_LWMSG_ERROR(lwmsg_session_retain_handle(pSession, pHandle));
    BAIL_ON_LSA_ERROR(dwError);

error:

    return dwError;
}

static
DWORD
LsaSrvIpcUnregisterHandle(
    LWMsgCall* pCall,
    PVOID pHandle
    )
{
    DWORD dwError = 0;
    LWMsgSession* pSession = lwmsg_call_get_session(pCall);

    dwError = MAP_LWMSG_ERROR(lwmsg_session_unregister_handle(pSession, pHandle));
    BAIL_ON_LSA_ERROR(dwError);

error:

    return dwError;
}

static
HANDLE
LsaSrvIpcGetSessionData(
    LWMsgCall* pCall
    )
{
    LWMsgSession* pSession = lwmsg_call_get_session(pCall);

    return lwmsg_session_get_data(pSession);
}

static void
LsaSrvCleanupArtefactEnumHandle(
    void* pData
    )
{
    LsaSrvEndEnumNSSArtefacts(
        NULL,
        (HANDLE) pData);
}

static LWMsgStatus
LsaSrvIpcFindNSSArtefactByKey(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    // Do not free pUserInfo
    PVOID pNSSArtefactInfo = NULL;
    PVOID* ppNSSArtefactInfo = NULL;
    PLSA_NSS_ARTEFACT_INFO_LIST pResult = NULL;
    PLSA_IPC_FIND_NSSARTEFACT_BY_KEY_REQ pReq = pIn->data;
    PLSA_IPC_ERROR pError = NULL;

    dwError = LsaSrvFindNSSArtefactByKey(
                       LsaSrvIpcGetSessionData(pCall),
                       pReq->pszKeyName,
                       pReq->pszMapName,
                       pReq->dwFlags,
                       pReq->dwInfoLevel,
                       &pNSSArtefactInfo);

    if (!dwError)
    {
        dwError = LsaAllocateMemory(sizeof(*pResult),
                                    (PVOID)&pResult);
        BAIL_ON_LSA_ERROR(dwError);

        pResult->dwNssArtefactInfoLevel = pReq->dwInfoLevel;
        pResult->dwNumNssArtefacts = 1;

        dwError = LsaAllocateMemory(
                        sizeof(*ppNSSArtefactInfo) * 1,
                        (PVOID*)&ppNSSArtefactInfo);
        BAIL_ON_LSA_ERROR(dwError);

        ppNSSArtefactInfo[0] = pNSSArtefactInfo;
        pNSSArtefactInfo = NULL;

        switch (pResult->dwNssArtefactInfoLevel)
        {
            case 0:
                pResult->ppNssArtefactInfoList.ppInfoList0 = (PLSA_NSS_ARTEFACT_INFO_0*)ppNSSArtefactInfo;
                ppNSSArtefactInfo = NULL;
                break;

            default:
                dwError = LW_ERROR_INVALID_PARAMETER;
                BAIL_ON_LSA_ERROR(dwError);
        }

        pOut->tag = LSA_R_FIND_NSS_ARTEFACT_BY_KEY_SUCCESS;
        pOut->data = pResult;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA_R_FIND_NSS_ARTEFACT_BY_KEY_FAILURE;
        pOut->data = pError;
    }

cleanup:
    if (pNSSArtefactInfo)
    {
        LsaFreeNSSArtefactInfo(pReq->dwInfoLevel, pNSSArtefactInfo);
    }
    if (ppNSSArtefactInfo)
    {
        LsaFreeNSSArtefactInfoList(pReq->dwInfoLevel, ppNSSArtefactInfo, 1);
    }

    return MAP_LW_ERROR_IPC(dwError);

error:
    if(pResult)
    {
        LsaFreeIpcNssArtefactInfoList(pResult);
    }

    goto cleanup;
}

static LWMsgStatus
LsaSrvIpcBeginEnumNSSArtefacts(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PSTR pszGUID = NULL;
    PLSA_IPC_BEGIN_ENUM_NSSARTEFACT_REQ pReq = pIn->data;
    PLSA_IPC_ERROR pError = NULL;
    HANDLE hResume = NULL;

    dwError = LsaSrvBeginEnumNSSArtefacts(
                        LsaSrvIpcGetSessionData(pCall),
                        pReq->pszMapName,
                        pReq->dwFlags,
                        pReq->dwInfoLevel,
                        pReq->dwMaxNumNSSArtefacts,
                        &hResume);

    if (!dwError)
    {
        dwError = LsaSrvIpcRegisterHandle(
                                      pCall,
                                      "EnumArtefacts",
                                      hResume,
                                      LsaSrvCleanupArtefactEnumHandle);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA_R_BEGIN_ENUM_NSS_ARTEFACTS_SUCCESS;
        pOut->data = hResume;
        hResume = NULL;

        dwError = LsaSrvIpcRetainHandle(pCall, pOut->data);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA_R_BEGIN_ENUM_NSS_ARTEFACTS_FAILURE;
        pOut->data = pError;
    }

cleanup:
    LSA_SAFE_FREE_STRING(pszGUID);

    return MAP_LW_ERROR_IPC(dwError);

error:

    if(hResume)
    {
        LsaSrvCleanupArtefactEnumHandle(hResume);
    }

    goto cleanup;
}

static LWMsgStatus
LsaSrvIpcEnumNSSArtefacts(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PVOID* ppNSSArtefactInfoList = NULL;
    DWORD  dwNSSArtefactInfoLevel = 0;
    DWORD  dwNumNSSArtefactsFound = 0;
    PLSA_NSS_ARTEFACT_INFO_LIST pResult = NULL;
    PLSA_IPC_ERROR pError = NULL;

    dwError = LsaSrvEnumNSSArtefacts(
                       LsaSrvIpcGetSessionData(pCall),
                       (HANDLE) pIn->data,
                       &dwNSSArtefactInfoLevel,
                       &ppNSSArtefactInfoList,
                       &dwNumNSSArtefactsFound);

    if (!dwError)
    {
        dwError = LsaAllocateMemory(sizeof(*pResult),
                                   (PVOID)&pResult);
        BAIL_ON_LSA_ERROR(dwError);

        pResult->dwNssArtefactInfoLevel = dwNSSArtefactInfoLevel;
        pResult->dwNumNssArtefacts = dwNumNSSArtefactsFound;
        switch (pResult->dwNssArtefactInfoLevel)
        {
            case 0:
                pResult->ppNssArtefactInfoList.ppInfoList0 = (PLSA_NSS_ARTEFACT_INFO_0*)ppNSSArtefactInfoList;
                ppNSSArtefactInfoList = NULL;
                break;

            default:
                dwError = LW_ERROR_INVALID_PARAMETER;
                BAIL_ON_LSA_ERROR(dwError);
        }

        pOut->tag = LSA_R_ENUM_NSS_ARTEFACTS_SUCCESS;
        pOut->data = pResult;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA_R_ENUM_NSS_ARTEFACTS_FAILURE;;
        pOut->data = pError;
    }

cleanup:
    if(ppNSSArtefactInfoList)
    {
        LsaFreeNSSArtefactInfoList(dwNSSArtefactInfoLevel, ppNSSArtefactInfoList, dwNumNSSArtefactsFound);
    }

    return MAP_LW_ERROR_IPC(dwError);

error:
    if(pResult)
    {
        LsaFreeIpcNssArtefactInfoList(pResult);
    }

    goto cleanup;
}

static LWMsgStatus
LsaSrvIpcEndEnumNSSArtefacts(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PLSA_IPC_ERROR pError = NULL;

    dwError = LsaSrvIpcUnregisterHandle(pCall, pIn->data);
    if (!dwError)
    {
        pOut->tag = LSA_R_END_ENUM_NSS_ARTEFACTS_SUCCESS;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA_R_END_ENUM_NSS_ARTEFACTS_FAILURE;
        pOut->data = pError;
    }

cleanup:
    return MAP_LW_ERROR_IPC(dwError);

error:
    goto cleanup;
}


static LWMsgStatus
LsaSrvIpcAuthenticateUser(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PLSA_IPC_AUTH_USER_REQ pReq = pIn->data;
    PLSA_IPC_ERROR pError = NULL;

    dwError = LsaSrvAuthenticateUser(
                        LsaSrvIpcGetSessionData(pCall),
                        pReq->pszLoginName,
                        pReq->pszPassword);

    if (!dwError)
    {
        pOut->tag = LSA_R_AUTH_USER_SUCCESS;
        pOut->data = NULL;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA_R_AUTH_USER_FAILURE;
        pOut->data = pError;
    }

cleanup:
    return MAP_LW_ERROR_IPC(dwError);

error:
    goto cleanup;
}

static LWMsgStatus
LsaSrvIpcValidateUser(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PLSA_IPC_AUTH_USER_REQ pReq = pIn->data;
    PLSA_IPC_ERROR pError = NULL;

    dwError = LsaSrvValidateUser(
                        LsaSrvIpcGetSessionData(pCall),
                        pReq->pszLoginName,
                        pReq->pszPassword);

    if (!dwError)
    {
        pOut->tag = LSA_R_VALIDATE_USER_SUCCESS;
        pOut->data = NULL;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA_R_VALIDATE_USER_FAILURE;
        pOut->data = pError;
    }

cleanup:
    return MAP_LW_ERROR_IPC(dwError);

error:
    goto cleanup;
}

static LWMsgStatus
LsaSrvIpcCheckUserInList(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PLSA_IPC_CHECK_USER_IN_LIST_REQ pReq = pIn->data;
    PLSA_IPC_ERROR pError = NULL;

    dwError = LsaSrvCheckUserInList(
                        LsaSrvIpcGetSessionData(pCall),
                        pReq->pszLoginName,
                        pReq->pszListName);

    if (!dwError)
    {
        pOut->tag = LSA_R_CHECK_USER_IN_LIST_SUCCESS;
        pOut->data = NULL;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA_R_CHECK_USER_IN_LIST_FAILURE;
        pOut->data = pError;
    }

cleanup:
    return MAP_LW_ERROR_IPC(dwError);

error:
    goto cleanup;
}

static LWMsgStatus
LsaSrvIpcChangePassword(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PLSA_IPC_CHANGE_PASSWORD_REQ pReq = pIn->data;
    PLSA_IPC_ERROR pError = NULL;

    dwError = LsaSrvChangePassword(
                        LsaSrvIpcGetSessionData(pCall),
                        pReq->pszLoginName,
                        pReq->pszOldPassword,
                        pReq->pszNewPassword);

    if (!dwError)
    {
        pOut->tag = LSA_R_CHANGE_PASSWORD_SUCCESS;
        pOut->data = NULL;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA_R_CHANGE_PASSWORD_FAILURE;
        pOut->data = pError;
    }

cleanup:
    return MAP_LW_ERROR_IPC(dwError);

error:
    goto cleanup;
}

static LWMsgStatus
LsaSrvIpcSetPassword(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PLSA_IPC_SET_PASSWORD_REQ pReq = pIn->data;
    PLSA_IPC_ERROR pError = NULL;

    dwError = LsaSrvSetPassword(
                        LsaSrvIpcGetSessionData(pCall),
                        pReq->pszLoginName,
                        pReq->pszNewPassword);
    if (!dwError)
    {
        pOut->tag    = LSA_R_SET_PASSWORD_SUCCESS;
        pOut->data = NULL;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag    = LSA_R_SET_PASSWORD_FAILURE;
        pOut->data = pError;
    }

cleanup:
    return MAP_LW_ERROR_IPC(dwError);

error:
    goto cleanup;
}


static VOID
FreeAuthUserInfo(PLSA_AUTH_USER_INFO *pUserInfo)
{
    if (!pUserInfo)
        return;

    LSA_SAFE_FREE_MEMORY((*pUserInfo)->pszAccount);
    LSA_SAFE_FREE_MEMORY((*pUserInfo)->pszUserPrincipalName);
    LSA_SAFE_FREE_MEMORY((*pUserInfo)->pszFullName);
    LSA_SAFE_FREE_MEMORY((*pUserInfo)->pszDomain);
    LSA_SAFE_FREE_MEMORY((*pUserInfo)->pszDnsDomain);
    LSA_SAFE_FREE_MEMORY((*pUserInfo)->pszLogonServer);
    LSA_SAFE_FREE_MEMORY((*pUserInfo)->pszLogonScript);
    LSA_SAFE_FREE_MEMORY((*pUserInfo)->pszProfilePath);
    LSA_SAFE_FREE_MEMORY((*pUserInfo)->pszHomeDirectory);
    LSA_SAFE_FREE_MEMORY((*pUserInfo)->pszHomeDrive);

    LsaDataBlobFree(&(*pUserInfo)->pSessionKey);
    LsaDataBlobFree(&(*pUserInfo)->pLmSessionKey);

    LSA_SAFE_FREE_MEMORY((*pUserInfo)->pSidAttribList);

    LSA_SAFE_FREE_MEMORY(*pUserInfo);

    return;
}


static LWMsgStatus
LsaSrvIpcAuthenticateUserEx(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = LW_ERROR_NOT_IMPLEMENTED;
    PLSA_AUTH_USER_PARAMS pParams = (PLSA_AUTH_USER_PARAMS) pIn->data;
    PLSA_AUTH_USER_INFO pUserInfo = NULL;
    PLSA_IPC_ERROR pError = NULL;

    dwError = LsaSrvAuthenticateUserEx(LsaSrvIpcGetSessionData(pCall),
                                       pParams,
                                       &pUserInfo);

    if (!dwError)
    {
        pOut->tag = LSA_R_AUTH_USER_EX_SUCCESS;
        pOut->data = pUserInfo;
        pUserInfo = NULL;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA_R_AUTH_USER_EX_FAILURE;
        pOut->data = pError;
    }

cleanup:
    if (pUserInfo)
    {
       FreeAuthUserInfo(&pUserInfo);
    }

    return MAP_LW_ERROR_IPC(dwError);

error:
   goto cleanup;
}

static LWMsgStatus
LsaSrvIpcRefreshConfiguration(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PLSA_IPC_ERROR pError = NULL;

    dwError = LsaSrvRefreshConfiguration(LsaSrvIpcGetSessionData(pCall));

    if (!dwError)
    {
        pOut->tag = LSA_R_REFRESH_CONFIGURATION_SUCCESS;
        pOut->data = NULL;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA_R_REFRESH_CONFIGURATION_FAILURE;
        pOut->data = pError;
    }

cleanup:
    return MAP_LW_ERROR_IPC(dwError);

error:
    goto cleanup;
}


static LWMsgStatus
LsaSrvIpcSetMachineSid(
    LWMsgCall *pCall,
    const LWMsgParams *pIn,
    LWMsgParams *pOut,
    void *data
    )
{
    DWORD dwError = 0;
    PLSA_IPC_ERROR pError = NULL;
    PLSA_IPC_SET_MACHINE_SID pReq = pIn->data;

    dwError = LsaSrvSetMachineSid(LsaSrvIpcGetSessionData(pCall), pReq->pszSid);
    if (!dwError)
    {
        pOut->tag    = LSA_R_SET_MACHINE_SID_SUCCESS;
        pOut->data = NULL;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag    = LSA_R_SET_MACHINE_SID_FAILURE;
        pOut->data = pError;
    }

cleanup:
    return MAP_LW_ERROR_IPC(dwError);

error:
    goto cleanup;
}

static void
LsaSrvCleanupGroupEnumHandle(
    void* pData
    )
{
    LsaSrvEndEnumGroups(
        NULL,
        (HANDLE) pData);
}

static LWMsgStatus
LsaSrvIpcAddGroup(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PLSA_IPC_ERROR pError = NULL;
    // Do not free pGroupInfoList
    PLSA_GROUP_INFO_LIST pGroupInfoList = (PLSA_GROUP_INFO_LIST)pIn->data;

    switch (pGroupInfoList->dwGroupInfoLevel)
    {
        case 0:
            dwError = LsaSrvAddGroup(
                            LsaSrvIpcGetSessionData(pCall),
                            0,
                            pGroupInfoList->ppGroupInfoList.ppInfoList0[0]);
            break;
        case 1:
            dwError = LsaSrvAddGroup(
                            LsaSrvIpcGetSessionData(pCall),
                            1,
                            pGroupInfoList->ppGroupInfoList.ppInfoList1[0]);
            break;
        default:
            dwError = LW_ERROR_INVALID_PARAMETER;
    }

    if (!dwError)
    {
        pOut->tag = LSA_R_ADD_GROUP_SUCCESS;
        pOut->data = NULL;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA_R_ADD_GROUP_FAILURE;;
        pOut->data = pError;
    }

cleanup:
    return MAP_LW_ERROR_IPC(dwError);

error:
    goto cleanup;
}

static LWMsgStatus
LsaSrvIpcModifyGroup(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PLSA_IPC_ERROR pError = NULL;

    dwError = LsaSrvModifyGroup(
                    LsaSrvIpcGetSessionData(pCall),
                    (PLSA_GROUP_MOD_INFO)pIn->data);
    if (!dwError)
    {
        pOut->tag    = LSA_R_MODIFY_GROUP_SUCCESS;
        pOut->data = NULL;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag    = LSA_R_MODIFY_GROUP_FAILURE;
        pOut->data = pError;
    }

cleanup:
    return MAP_LW_ERROR_IPC(dwError);

error:
    goto cleanup;
}

static LWMsgStatus
LsaSrvIpcFindGroupByName(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    // Do not free pGroupInfo
    PVOID pGroupInfo = NULL;
    PVOID* ppGroupInfoList = NULL;
    PLSA_GROUP_INFO_LIST pResult = NULL;
    PLSA_IPC_FIND_OBJECT_BY_NAME_REQ pReq = pIn->data;
    PLSA_IPC_ERROR pError = NULL;

    dwError = LsaSrvFindGroupByName(
                       LsaSrvIpcGetSessionData(pCall),
                       pReq->pszName,
                       pReq->FindFlags,
                       pReq->dwInfoLevel,
                       &pGroupInfo);

    if (!dwError)
    {
        dwError = LsaAllocateMemory(sizeof(*pResult),
                                        (PVOID)&pResult);
        BAIL_ON_LSA_ERROR(dwError);

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
                dwError = LW_ERROR_INVALID_PARAMETER;
                BAIL_ON_LSA_ERROR(dwError);
        }

        pOut->tag = LSA_R_GROUP_BY_NAME_SUCCESS;
        pOut->data = pResult;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA_R_GROUP_BY_NAME_FAILURE;;
        pOut->data = pError;
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

    return MAP_LW_ERROR_IPC(dwError);

error:
    if(pResult)
    {
        LsaFreeIpcGroupInfoList(pResult);
    }

    goto cleanup;
}

static LWMsgStatus
LsaSrvIpcFindGroupById(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PVOID pGroupInfo = NULL;
    PVOID* ppGroupInfoList = NULL;
    PLSA_GROUP_INFO_LIST pResult = NULL;
    PLSA_IPC_FIND_OBJECT_BY_ID_REQ pReq = pIn->data;
    PLSA_IPC_ERROR pError = NULL;

    dwError = LsaSrvFindGroupById(
                       LsaSrvIpcGetSessionData(pCall),
                       pReq->id,
                       pReq->FindFlags,
                       pReq->dwInfoLevel,
                       &pGroupInfo);

    if (!dwError)
    {
        dwError = LsaAllocateMemory(sizeof(*pResult),
                                    (PVOID)&pResult);
        BAIL_ON_LSA_ERROR(dwError);

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
                dwError = LW_ERROR_INVALID_PARAMETER;
                BAIL_ON_LSA_ERROR(dwError);
        }

        pOut->tag = LSA_R_GROUP_BY_ID_SUCCESS;
        pOut->data = pResult;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA_R_GROUP_BY_ID_FAILURE;;
        pOut->data = pError;
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

    return MAP_LW_ERROR_IPC(dwError);

error:
    if(pResult)
    {
        LsaFreeIpcGroupInfoList(pResult);
    }

    goto cleanup;
}

static LWMsgStatus
LsaSrvIpcGetGroupsForUser(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PVOID* ppGroupInfoList = NULL;
    DWORD dwNumGroupsFound = 0;
    PLSA_GROUP_INFO_LIST pResult = NULL;
    PLSA_IPC_FIND_OBJECT_REQ pReq = pIn->data;
    PLSA_IPC_ERROR pError = NULL;

    switch (pReq->ByType)
    {
        case LSA_IPC_FIND_OBJECT_BY_TYPE_NAME:
            dwError = LsaSrvGetGroupsForUser(
                               LsaSrvIpcGetSessionData(pCall),
                               pReq->ByData.pszName,
                               0,
                               pReq->FindFlags,
                               pReq->dwInfoLevel,
                               &dwNumGroupsFound,
                               &ppGroupInfoList);
            break;
        case LSA_IPC_FIND_OBJECT_BY_TYPE_ID:
            dwError = LsaSrvGetGroupsForUser(
                               LsaSrvIpcGetSessionData(pCall),
                               NULL,
                               pReq->ByData.dwId,
                               pReq->FindFlags,
                               pReq->dwInfoLevel,
                               &dwNumGroupsFound,
                               &ppGroupInfoList);
            break;
        default:
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
    }

    if (!dwError)
    {
        dwError = LsaAllocateMemory(sizeof(*pResult),
                                        (PVOID)&pResult);
        BAIL_ON_LSA_ERROR(dwError);

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
                dwError = LW_ERROR_INVALID_PARAMETER;
                BAIL_ON_LSA_ERROR(dwError);
        }

        pOut->tag = LSA_R_GROUPS_FOR_USER_SUCCESS;
        pOut->data = pResult;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA_R_GROUPS_FOR_USER_FAILURE;
        pOut->data = pError;
    }

cleanup:
    if(ppGroupInfoList)
    {
        LsaFreeGroupInfoList(pReq->dwInfoLevel, ppGroupInfoList, dwNumGroupsFound);
    }

    return MAP_LW_ERROR_IPC(dwError);

error:
    if(pResult)
    {
        LsaFreeIpcGroupInfoList(pResult);
    }

    goto cleanup;
}

static LWMsgStatus
LsaSrvIpcBeginEnumGroups(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PLSA_IPC_BEGIN_ENUM_GROUPS_REQ pReq = pIn->data;
    PLSA_IPC_ERROR pError = NULL;
    PVOID hResume = NULL;

    dwError = LsaSrvBeginEnumGroups(
                        LsaSrvIpcGetSessionData(pCall),
                        pReq->dwInfoLevel,
                        pReq->dwNumMaxRecords,
                        pReq->bCheckGroupMembersOnline,
                        pReq->FindFlags,
                        &hResume);

    if (!dwError)
    {
        dwError = LsaSrvIpcRegisterHandle(
                                      pCall,
                                      "EnumGroups",
                                      hResume,
                                      LsaSrvCleanupGroupEnumHandle);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA_R_BEGIN_ENUM_GROUPS_SUCCESS;
        pOut->data = hResume;
        hResume = NULL;

        dwError = LsaSrvIpcRetainHandle(pCall, pOut->data);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA_R_BEGIN_ENUM_GROUPS_FAILURE;
        pOut->data = pError;
    }

cleanup:

    return MAP_LW_ERROR_IPC(dwError);

error:

    if(hResume)
    {
        LsaSrvCleanupGroupEnumHandle(hResume);
    }

    goto cleanup;
}

static LWMsgStatus
LsaSrvIpcEnumGroups(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PVOID* ppGroupInfoList = NULL;
    DWORD  dwGroupInfoLevel = 0;
    DWORD  dwNumGroupsFound = 0;
    PLSA_GROUP_INFO_LIST pResult = NULL;
    PLSA_IPC_ERROR pError = NULL;

    dwError = LsaSrvEnumGroups(
        LsaSrvIpcGetSessionData(pCall),
        (HANDLE) pIn->data,
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
                dwError = LW_ERROR_INVALID_PARAMETER;
                BAIL_ON_LSA_ERROR(dwError);
        }

        pOut->tag = LSA_R_ENUM_GROUPS_SUCCESS;
        pOut->data = pResult;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA_R_ENUM_GROUPS_FAILURE;;
        pOut->data = pError;
    }

cleanup:
    if(ppGroupInfoList)
    {
        LsaFreeGroupInfoList(dwGroupInfoLevel, ppGroupInfoList, dwNumGroupsFound);
    }

    return MAP_LW_ERROR_IPC(dwError);

error:
    if(pResult)
    {
        LsaFreeIpcGroupInfoList(pResult);
    }

    goto cleanup;
}

static LWMsgStatus
LsaSrvIpcEndEnumGroups(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PLSA_IPC_ERROR pError = NULL;

    dwError = LsaSrvIpcUnregisterHandle(pCall, pIn->data);
    if (!dwError)
    {
        pOut->tag = LSA_R_END_ENUM_GROUPS_SUCCESS;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA_R_END_ENUM_GROUPS_FAILURE;
        pOut->data = pError;
    }

cleanup:
    return MAP_LW_ERROR_IPC(dwError);

error:
    goto cleanup;
}

static LWMsgStatus
LsaSrvIpcDeleteGroup(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PLSA_IPC_ERROR pError = NULL;

    dwError = LsaSrvDeleteGroup(
                        LsaSrvIpcGetSessionData(pCall),
                        *((PDWORD)pIn->data));

    if (!dwError)
    {
        pOut->tag = LSA_R_DELETE_GROUP_SUCCESS;
        pOut->data = NULL;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA_R_DELETE_GROUP_FAILURE;
        pOut->data = pError;
    }

cleanup:
    return MAP_LW_ERROR_IPC(dwError);

error:
    goto cleanup;
}

#define ZERO_STRUCT(_s_) memset((char*)&(_s_),0,sizeof(_s_))

static
VOID
LsaNTLMGssFreeAuthMsg(
    PLSA_GSS_R_MAKE_AUTH_MSG pAuthMsgReply
    )
{
    if (pAuthMsgReply)
    {
        NTLMGssFreeSecBuffer(&pAuthMsgReply->authenticateMessage);
        LSA_SAFE_FREE_MEMORY(pAuthMsgReply);
    }
}

static LWMsgStatus
LsaSrvIpcBuildAuthMessage(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PLSA_IPC_MAKE_AUTH_MSG_REQ pReq = pIn->data;
    PLSA_IPC_ERROR pError = NULL;
    PLSA_GSS_R_MAKE_AUTH_MSG pAuthMsgReply = NULL;
    uid_t peerUID = 0;

    dwError = LsaAllocateMemory(sizeof(*pAuthMsgReply),
                                (PVOID*)&pAuthMsgReply);
    BAIL_ON_LSA_ERROR(dwError);

    ZERO_STRUCT(pAuthMsgReply->authenticateMessage);
    ZERO_STRUCT(pAuthMsgReply->baseSessionKey);

    LsaSrvGetUid(LsaSrvIpcGetSessionData(pCall), &peerUID);

    pAuthMsgReply->msgError = NTLMGssBuildAuthenticateMessage(
                        pReq->negotiateFlags,
                        peerUID,
                        &pReq->credentials,
                        &pReq->serverChallenge,
                        &pReq->targetInfo,
                        &pAuthMsgReply->authenticateMessage,
                        &pAuthMsgReply->baseSessionKey);

    pOut->tag = LSA_R_GSS_MAKE_AUTH_MSG_SUCCESS;
    pOut->data = pAuthMsgReply;

cleanup:
    return MAP_LW_ERROR_IPC(dwError);

error:
    dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);

    pOut->tag = LSA_R_GSS_MAKE_AUTH_MSG_FAILURE;
    pOut->data = pError;

    if(pAuthMsgReply)
    {
        LsaNTLMGssFreeAuthMsg(pAuthMsgReply);
    }

    goto cleanup;
}

static LWMsgStatus
LsaSrvIpcCheckAuthMessage(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PLSA_IPC_CHECK_AUTH_MSG_REQ pReq = pIn->data;
    PLSA_IPC_ERROR pError = NULL;
    PLSA_GSS_R_CHECK_AUTH_MSG pCheckAuthMsgReply = NULL;

    dwError = LsaAllocateMemory(sizeof(*pCheckAuthMsgReply),
                                (PVOID*)&pCheckAuthMsgReply);
    BAIL_ON_LSA_ERROR(dwError);

    ZERO_STRUCT(pCheckAuthMsgReply->baseSessionKey);

    pCheckAuthMsgReply->msgError = NTLMGssCheckAuthenticateMessage(
                                        pReq->negotiateFlags,
                                        &pReq->serverChallenge,
                                        &pReq->targetInfo,
                                        &pReq->authenticateMessage,
                                        &pCheckAuthMsgReply->baseSessionKey);

    pOut->tag = LSA_R_GSS_CHECK_AUTH_MSG_SUCCESS;
    pOut->data = pCheckAuthMsgReply;

cleanup:
    return MAP_LW_ERROR_IPC(dwError);

error:
    dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);

    pOut->tag = LSA_R_GSS_CHECK_AUTH_MSG_FAILURE;
    pOut->data = pError;

    if(pCheckAuthMsgReply)
    {
        LSA_SAFE_FREE_MEMORY(pCheckAuthMsgReply);
    }

    goto cleanup;
}

static LWMsgStatus
LsaSrvIpcSetLogInfo(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PLSA_IPC_ERROR pError = NULL;

    dwError = LsaSrvSetLogInfo(LsaSrvIpcGetSessionData(pCall),
                                (PLSA_LOG_INFO)pIn->data);

    if (!dwError)
    {
        pOut->tag = LSA_R_SET_LOGINFO_SUCCESS;
        pOut->data = NULL;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA_R_SET_LOGINFO_FAILURE;
        pOut->data = pError;
    }

cleanup:
    return MAP_LW_ERROR_IPC(dwError);

error:
    goto cleanup;
}

static LWMsgStatus
LsaSrvIpcGetLogInfo(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PLSA_LOG_INFO pLogInfo = NULL;
    PLSA_IPC_ERROR pError = NULL;

    dwError = LsaSrvGetLogInfo(LsaSrvIpcGetSessionData(pCall),
                               &pLogInfo);

    if (!dwError)
    {
        pOut->tag = LSA_R_GET_LOGINFO_SUCCESS;
        pOut->data = pLogInfo;
        pLogInfo = NULL;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA_R_GET_LOGINFO_FAILURE;
        pOut->data = pError;
    }

cleanup:
    if (pLogInfo)
    {
        LsaFreeLogInfo(pLogInfo);
    }
    return MAP_LW_ERROR_IPC(dwError);

error:
    goto cleanup;
}

static LWMsgStatus
LsaSrvIpcGetMetrics(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PVOID pMetricPack = NULL;
    PLSA_METRIC_PACK pResult = NULL;
    PLSA_IPC_ERROR pError = NULL;
    DWORD dwInfoLevel = *(PDWORD)pIn->data;

    dwError = LsaSrvGetMetrics(
                        LsaSrvIpcGetSessionData(pCall),
                        dwInfoLevel,
                        &pMetricPack);

    if (!dwError)
    {
        dwError = LsaAllocateMemory(sizeof(*pResult),
                                    (PVOID)&pResult);
        BAIL_ON_LSA_ERROR(dwError);

        pResult->dwInfoLevel = dwInfoLevel;

        switch (pResult->dwInfoLevel)
        {
            case 0:
                pResult->pMetricPack.pMetricPack0 = (PLSA_METRIC_PACK_0)pMetricPack;
                pMetricPack = NULL;
                break;

            case 1:
                pResult->pMetricPack.pMetricPack1 = (PLSA_METRIC_PACK_1)pMetricPack;
                pMetricPack = NULL;
                break;

            default:
                dwError = LW_ERROR_INVALID_PARAMETER;
                BAIL_ON_LSA_ERROR(dwError);
        }

        pOut->tag = LSA_R_GET_METRICS_SUCCESS;
        pOut->data = pResult;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA_R_GET_METRICS_FAILURE;
        pOut->data = pError;
    }

cleanup:
    if(pMetricPack)
    {
        LSA_SAFE_FREE_MEMORY(pMetricPack);
    }

    return MAP_LW_ERROR_IPC(dwError);

error:
    if(pResult)
    {
        LsaSrvFreeIpcMetriPack(pResult);
    }

    goto cleanup;
}

static LWMsgStatus
LsaSrvIpcProviderIoControl(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PLSA_IPC_ERROR pError = NULL;
    // Do not free pProviderIoControlReq
    PLSA_IPC_PROVIDER_IO_CONTROL_REQ pProviderIoControlReq =
        (PLSA_IPC_PROVIDER_IO_CONTROL_REQ)pIn->data;
    DWORD dwOutputBufferSize = 0;
    PVOID pOutputBuffer = NULL;
    PLSA_DATA_BLOB pBlob = NULL;

    dwError = LsaSrvProviderIoControl(
                  LsaSrvIpcGetSessionData(pCall),
                  pProviderIoControlReq->pszProvider,
                  pProviderIoControlReq->dwIoControlCode,
                  pProviderIoControlReq->dwDataLen,
                  pProviderIoControlReq->pData,
                  &dwOutputBufferSize,
                  &pOutputBuffer);

    if (!dwError)
    {
        if ( dwOutputBufferSize )
        {
            pOut->tag = LSA_R_PROVIDER_IO_CONTROL_SUCCESS_DATA;
            dwError = LsaDataBlobStore(
                          &pBlob,
                          dwOutputBufferSize,
                          pOutputBuffer);
            BAIL_ON_LSA_ERROR(dwError);
            pOut->data = pBlob;
        }
        else
        {
            pOut->tag = LSA_R_PROVIDER_IO_CONTROL_SUCCESS;
            pOut->data = NULL;
        }
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA_R_PROVIDER_IO_CONTROL_FAILURE;;
        pOut->data = pError;
    }

cleanup:
    if ( pOutputBuffer )
    {
        LsaFreeMemory(pOutputBuffer);
    }

    return MAP_LW_ERROR_IPC(dwError);

error:

    LsaDataBlobFree( &pBlob );

    goto cleanup;
}

static LWMsgStatus
LsaSrvIpcOpenSession(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PLSA_IPC_ERROR pError = NULL;

    dwError = LsaSrvOpenSession(
                    LsaSrvIpcGetSessionData(pCall),
                    (PSTR)pIn->data);

    if (!dwError)
    {
        pOut->tag = LSA_R_OPEN_SESSION_SUCCESS;
        pOut->data = NULL;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA_R_OPEN_SESSION_FAILURE;
        pOut->data = pError;
    }

cleanup:
    return MAP_LW_ERROR_IPC(dwError);

error:
    goto cleanup;
}

static LWMsgStatus
LsaSrvIpcCloseSession(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PLSA_IPC_ERROR pError = NULL;

    dwError = LsaSrvCloseSession(
                    LsaSrvIpcGetSessionData(pCall),
                    (PSTR)pIn->data);

    if (!dwError)
    {
        pOut->tag = LSA_R_CLOSE_SESSION_SUCCESS;
        pOut->data = NULL;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA_R_CLOSE_SESSION_FAILURE;
        pOut->data = pError;
    }

cleanup:
    return MAP_LW_ERROR_IPC(dwError);

error:
    goto cleanup;
}

static LWMsgStatus
LsaSrvIpcGetStatus(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PLSASTATUS pLsaStatus = NULL;
    PLSA_IPC_ERROR pError = NULL;

    dwError = LsaSrvGetStatus(
                    LsaSrvIpcGetSessionData(pCall),
                    &pLsaStatus);

    if (!dwError)
    {
        pOut->tag = LSA_R_GET_STATUS_SUCCESS;
        pOut->data = pLsaStatus;
        pLsaStatus = NULL;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA_R_GET_STATUS_FAILURE;
        pOut->data = pError;
    }

cleanup:
    if(pLsaStatus)
    {
        LsaFreeStatus(pLsaStatus);
    }
    return MAP_LW_ERROR_IPC(dwError);

error:
    goto cleanup;
}

static LWMsgStatus
LsaSrvIpcSetTraceInfo(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PLSA_IPC_SET_TRACE_INFO_REQ pReq = pIn->data;
    PLSA_IPC_ERROR pError = NULL;

    dwError = LsaSrvSetTraceFlags(
                        LsaSrvIpcGetSessionData(pCall),
                        pReq->pTraceFlagArray,
                        pReq->dwNumFlags);

    if (!dwError)
    {
        pOut->tag = LSA_R_SET_TRACE_INFO_SUCCESS;
        pOut->data = NULL;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA_R_SET_TRACE_INFO_FAILURE;
        pOut->data = pError;
    }

cleanup:
    return MAP_LW_ERROR_IPC(dwError);

error:
    goto cleanup;
}

static LWMsgStatus
LsaSrvIpcGetTraceInfo(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PLSA_TRACE_INFO_LIST pResult = NULL;
    PLSA_TRACE_INFO pTraceInfo = NULL;
    PLSA_IPC_ERROR pError = NULL;

    dwError = LsaSrvGetTraceInfo(
                        LsaSrvIpcGetSessionData(pCall),
                        *(PDWORD)pIn->data,
                        &pTraceInfo);

    if (!dwError)
    {
        dwError = LsaAllocateMemory(sizeof(*pResult),
                                    (PVOID)&pResult);
        BAIL_ON_LSA_ERROR(dwError);

        pResult->dwNumFlags = 1;
        pResult->pTraceInfoArray = pTraceInfo;
        pTraceInfo = NULL;

        pOut->tag = LSA_R_GET_TRACE_INFO_SUCCESS;
        pOut->data = pResult;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA_R_GET_TRACE_INFO_FAILURE;
        pOut->data = pError;
    }

cleanup:
    LSA_SAFE_FREE_MEMORY(pTraceInfo);

    return MAP_LW_ERROR_IPC(dwError);

error:
    if (pResult)
    {
        LSA_SAFE_FREE_MEMORY(pResult->pTraceInfoArray);
        LsaFreeMemory(pResult);
    }

    goto cleanup;
}

static LWMsgStatus
LsaSrvIpcEnumTraceInfo(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PLSA_TRACE_INFO_LIST pResult = NULL;
    PLSA_IPC_ERROR pError = NULL;

    dwError = LsaAllocateMemory(sizeof(*pResult),
                               (PVOID)&pResult);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvEnumTraceFlags(
                       LsaSrvIpcGetSessionData(pCall),
                       &pResult->pTraceInfoArray,
                       &pResult->dwNumFlags);

    if (!dwError)
    {
        pOut->tag = LSA_R_ENUM_TRACE_INFO_SUCCESS;
        pOut->data = pResult;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA_R_ENUM_TRACE_INFO_FAILURE;;
        pOut->data = pError;
    }

cleanup:
    return MAP_LW_ERROR_IPC(dwError);

error:
    if (pResult)
    {
        LSA_SAFE_FREE_MEMORY(pResult->pTraceInfoArray);
        LsaFreeMemory(pResult);
    }

    goto cleanup;
}

static void
LsaSrvCleanupUserEnumHandle(
    void* pData
    )
{
    LsaSrvEndEnumUsers(
        NULL,
        (HANDLE) pData);
}

static LWMsgStatus
LsaSrvIpcAddUser(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PLSA_IPC_ERROR pError = NULL;
    // Do not free pUserInfoList
    PLSA_USER_INFO_LIST pUserInfoList = (PLSA_USER_INFO_LIST)pIn->data;

    switch (pUserInfoList->dwUserInfoLevel)
    {
        case 0:
            dwError = LsaSrvAddUser(
                            LsaSrvIpcGetSessionData(pCall),
                            0,
                            pUserInfoList->ppUserInfoList.ppInfoList0[0]);
            break;
        case 1:
            dwError = LsaSrvAddUser(
                            LsaSrvIpcGetSessionData(pCall),
                            1,
                            pUserInfoList->ppUserInfoList.ppInfoList1[0]);
            break;
        case 2:
            dwError = LsaSrvAddUser(
                            LsaSrvIpcGetSessionData(pCall),
                            2,
                            pUserInfoList->ppUserInfoList.ppInfoList2[0]);
            break;
        default:
            dwError = LW_ERROR_INVALID_PARAMETER;
    }

    if (!dwError)
    {
        pOut->tag = LSA_R_ADD_USER_SUCCESS;
        pOut->data = NULL;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA_R_ADD_USER_FAILURE;;
        pOut->data = pError;
    }

cleanup:
    return MAP_LW_ERROR_IPC(dwError);

error:
    goto cleanup;
}

static LWMsgStatus
LsaSrvIpcModifyUser(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PLSA_IPC_ERROR pError = NULL;

    dwError = LsaSrvModifyUser(
                    LsaSrvIpcGetSessionData(pCall),
                    (PLSA_USER_MOD_INFO)pIn->data);

    if (!dwError)
    {
        pOut->tag = LSA_R_MODIFY_USER_SUCCESS;
        pOut->data = NULL;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA_R_MODIFY_USER_FAILURE;
        pOut->data = pError;
    }

cleanup:
    return MAP_LW_ERROR_IPC(dwError);

error:
    goto cleanup;
}

static LWMsgStatus
LsaSrvIpcFindUserByName(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    // Do not free pUserInfo
    PVOID pUserInfo = NULL;
    PVOID* ppUserInfoList = NULL;
    PLSA_USER_INFO_LIST pResult = NULL;
    PLSA_IPC_FIND_OBJECT_BY_NAME_REQ pReq = pIn->data;
    PLSA_IPC_ERROR pError = NULL;

    dwError = LsaSrvFindUserByName(
                       LsaSrvIpcGetSessionData(pCall),
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
                dwError = LW_ERROR_INVALID_PARAMETER;
                BAIL_ON_LSA_ERROR(dwError);
        }

        pOut->tag = LSA_R_USER_BY_NAME_SUCCESS;
        pOut->data = pResult;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA_R_USER_BY_NAME_FAILURE;
        pOut->data = pError;
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

    return MAP_LW_ERROR_IPC(dwError);

error:
    if(pResult)
    {
        LsaFreeIpcUserInfoList(pResult);
    }

    goto cleanup;
}

static LWMsgStatus
LsaSrvIpcFindUserById(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    // Do not free pUserInfo
    PVOID pUserInfo = NULL;
    PVOID* ppUserInfoList = NULL;
    PLSA_USER_INFO_LIST pResult = NULL;
    PLSA_IPC_FIND_OBJECT_BY_ID_REQ pReq = pIn->data;
    PLSA_IPC_ERROR pError = NULL;

    dwError = LsaSrvFindUserById(
                       LsaSrvIpcGetSessionData(pCall),
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
                dwError = LW_ERROR_INVALID_PARAMETER;
                BAIL_ON_LSA_ERROR(dwError);
        }

        pOut->tag = LSA_R_USER_BY_ID_SUCCESS;
        pOut->data = pResult;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA_R_USER_BY_ID_FAILURE;
        pOut->data = pError;
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

    return MAP_LW_ERROR_IPC(dwError);

error:
    if(pResult)
    {
        LsaFreeIpcUserInfoList(pResult);
    }

    goto cleanup;
}

static LWMsgStatus
LsaSrvIpcBeginEnumUsers(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PLSA_IPC_BEGIN_ENUM_USERS_REQ pReq = pIn->data;
    PLSA_IPC_ERROR pError = NULL;
    HANDLE hResume = NULL;

    dwError = LsaSrvBeginEnumUsers(
        LsaSrvIpcGetSessionData(pCall),
        pReq->dwInfoLevel,
        pReq->dwNumMaxRecords,
        pReq->FindFlags,
        &hResume);

    if (!dwError)
    {
        dwError = LsaSrvIpcRegisterHandle(
                                      pCall,
                                      "EnumUsers",
                                      hResume,
                                      LsaSrvCleanupUserEnumHandle);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA_R_BEGIN_ENUM_USERS_SUCCESS;
        pOut->data = hResume;
        hResume = NULL;

        dwError = LsaSrvIpcRetainHandle(pCall, pOut->data);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA_R_BEGIN_ENUM_USERS_FAILURE;
        pOut->data = pError;
    }

cleanup:

    return MAP_LW_ERROR_IPC(dwError);

error:

    if(hResume)
    {
        LsaSrvCleanupUserEnumHandle(hResume);
    }

    goto cleanup;
}

static LWMsgStatus
LsaSrvIpcEnumUsers(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PVOID* ppUserInfoList = NULL;
    DWORD  dwUserInfoLevel = 0;
    DWORD  dwNumUsersFound = 0;
    PLSA_USER_INFO_LIST pResult = NULL;
    PLSA_IPC_ERROR pError = NULL;

    dwError = LsaSrvEnumUsers(
                       LsaSrvIpcGetSessionData(pCall),
                       pIn->data,
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
                dwError = LW_ERROR_INVALID_PARAMETER;
                BAIL_ON_LSA_ERROR(dwError);
        }

        pOut->tag = LSA_R_ENUM_USERS_SUCCESS;
        pOut->data = pResult;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA_R_ENUM_USERS_FAILURE;;
        pOut->data = pError;
    }

cleanup:
    if(ppUserInfoList)
    {
        LsaFreeUserInfoList(dwUserInfoLevel, ppUserInfoList, dwNumUsersFound);
    }

    return MAP_LW_ERROR_IPC(dwError);

error:
    if(pResult)
    {
        LsaFreeIpcUserInfoList(pResult);
    }

    goto cleanup;
}

static LWMsgStatus
LsaSrvIpcEndEnumUsers(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PLSA_IPC_ERROR pError = NULL;

    dwError = LsaSrvIpcUnregisterHandle(pCall, pIn->data);
    if (!dwError)
    {
        pOut->tag = LSA_R_END_ENUM_USERS_SUCCESS;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA_R_END_ENUM_USERS_FAILURE;
        pOut->data = pError;
    }

cleanup:
    return MAP_LW_ERROR_IPC(dwError);

error:
    goto cleanup;
}

static LWMsgStatus
LsaSrvIpcDeleteUser(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PLSA_IPC_ERROR pError = NULL;

    dwError = LsaSrvDeleteUser(
                        LsaSrvIpcGetSessionData(pCall),
                        *((PDWORD)pIn->data));

    if (!dwError)
    {
        pOut->tag = LSA_R_DELETE_USER_SUCCESS;
        pOut->data = NULL;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA_R_DELETE_USER_FAILURE;
        pOut->data = pError;
    }

cleanup:
    return MAP_LW_ERROR_IPC(dwError);

error:
    goto cleanup;
}

static LWMsgStatus
LsaSrvIpcGetNamesBySidList(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PSTR* ppszDomainNames = NULL;
    PSTR* ppszSamAccounts = NULL;
    ADAccountType* pTypes = NULL;
    CHAR chDomainSeparator = 0;
    PLSA_FIND_NAMES_BY_SIDS pResult = NULL;
    PLSA_IPC_NAMES_BY_SIDS_REQ pReq = pIn->data;
    PLSA_IPC_ERROR pError = NULL;
    DWORD i = 0;

    dwError = LsaSrvGetNamesBySidList(
                    LsaSrvIpcGetSessionData(pCall),
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

        pOut->tag = LSA_R_NAMES_BY_SID_LIST_SUCCESS;
        pOut->data = pResult;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = LSA_R_NAMES_BY_SID_LIST_FAILURE;
        pOut->data = pError;
    }

cleanup:

    if (ppszDomainNames)
    {
        LsaFreeStringArray(ppszDomainNames, pReq->sCount);
    }

    if (ppszSamAccounts)
    {
        LsaFreeStringArray(ppszSamAccounts, pReq->sCount);
    }
    LSA_SAFE_FREE_MEMORY(pTypes);

    return MAP_LW_ERROR_IPC(dwError);

error:
    if(pResult)
    {
        LsaFreeIpcNameSidsList(pResult);
    }

    goto cleanup;
}

static LWMsgDispatchSpec gMessageHandlers[] =
{
    LWMSG_DISPATCH_BLOCK(LSA_Q_GROUP_BY_NAME, LsaSrvIpcFindGroupByName),
    LWMSG_DISPATCH_BLOCK(LSA_Q_GROUP_BY_ID, LsaSrvIpcFindGroupById),
    LWMSG_DISPATCH_BLOCK(LSA_Q_BEGIN_ENUM_GROUPS, LsaSrvIpcBeginEnumGroups),
    LWMSG_DISPATCH_BLOCK(LSA_Q_ENUM_GROUPS, LsaSrvIpcEnumGroups),
    LWMSG_DISPATCH_BLOCK(LSA_Q_END_ENUM_GROUPS, LsaSrvIpcEndEnumGroups),
    LWMSG_DISPATCH_BLOCK(LSA_Q_USER_BY_NAME, LsaSrvIpcFindUserByName),
    LWMSG_DISPATCH_BLOCK(LSA_Q_USER_BY_ID, LsaSrvIpcFindUserById),
    LWMSG_DISPATCH_BLOCK(LSA_Q_BEGIN_ENUM_USERS, LsaSrvIpcBeginEnumUsers),
    LWMSG_DISPATCH_BLOCK(LSA_Q_ENUM_USERS, LsaSrvIpcEnumUsers),
    LWMSG_DISPATCH_BLOCK(LSA_Q_END_ENUM_USERS, LsaSrvIpcEndEnumUsers),
    LWMSG_DISPATCH_BLOCK(LSA_Q_AUTH_USER, LsaSrvIpcAuthenticateUser),
    LWMSG_DISPATCH_BLOCK(LSA_Q_AUTH_USER_EX, LsaSrvIpcAuthenticateUserEx),
    LWMSG_DISPATCH_BLOCK(LSA_Q_VALIDATE_USER, LsaSrvIpcValidateUser),
    LWMSG_DISPATCH_BLOCK(LSA_Q_CHANGE_PASSWORD, LsaSrvIpcChangePassword),
    LWMSG_DISPATCH_BLOCK(LSA_Q_SET_PASSWORD, LsaSrvIpcSetPassword),
    LWMSG_DISPATCH_BLOCK(LSA_Q_OPEN_SESSION, LsaSrvIpcOpenSession),
    LWMSG_DISPATCH_BLOCK(LSA_Q_CLOSE_SESSION, LsaSrvIpcCloseSession),
    LWMSG_DISPATCH_BLOCK(LSA_Q_MODIFY_USER, LsaSrvIpcModifyUser),
    LWMSG_DISPATCH_BLOCK(LSA_Q_NAMES_BY_SID_LIST, LsaSrvIpcGetNamesBySidList),
    LWMSG_DISPATCH_BLOCK(LSA_Q_GSS_MAKE_AUTH_MSG, LsaSrvIpcBuildAuthMessage),
    LWMSG_DISPATCH_BLOCK(LSA_Q_GSS_CHECK_AUTH_MSG, LsaSrvIpcCheckAuthMessage),
    LWMSG_DISPATCH_BLOCK(LSA_Q_ADD_GROUP, LsaSrvIpcAddGroup),
    LWMSG_DISPATCH_BLOCK(LSA_Q_MODIFY_GROUP, LsaSrvIpcModifyGroup),
    LWMSG_DISPATCH_BLOCK(LSA_Q_DELETE_GROUP, LsaSrvIpcDeleteGroup),
    LWMSG_DISPATCH_BLOCK(LSA_Q_ADD_USER, LsaSrvIpcAddUser),
    LWMSG_DISPATCH_BLOCK(LSA_Q_DELETE_USER, LsaSrvIpcDeleteUser),
    LWMSG_DISPATCH_BLOCK(LSA_Q_GROUPS_FOR_USER, LsaSrvIpcGetGroupsForUser),
    LWMSG_DISPATCH_BLOCK(LSA_Q_GET_METRICS, LsaSrvIpcGetMetrics),
    LWMSG_DISPATCH_BLOCK(LSA_Q_SET_LOGINFO, LsaSrvIpcSetLogInfo),
    LWMSG_DISPATCH_BLOCK(LSA_Q_GET_LOGINFO, LsaSrvIpcGetLogInfo),
    LWMSG_DISPATCH_BLOCK(LSA_Q_GET_STATUS, LsaSrvIpcGetStatus),
    LWMSG_DISPATCH_BLOCK(LSA_Q_REFRESH_CONFIGURATION, LsaSrvIpcRefreshConfiguration),
    LWMSG_DISPATCH_BLOCK(LSA_Q_CHECK_USER_IN_LIST, LsaSrvIpcCheckUserInList),
    LWMSG_DISPATCH_BLOCK(LSA_Q_BEGIN_ENUM_NSS_ARTEFACTS, LsaSrvIpcBeginEnumNSSArtefacts),
    LWMSG_DISPATCH_BLOCK(LSA_Q_ENUM_NSS_ARTEFACTS, LsaSrvIpcEnumNSSArtefacts),
    LWMSG_DISPATCH_BLOCK(LSA_Q_END_ENUM_NSS_ARTEFACTS, LsaSrvIpcEndEnumNSSArtefacts),
    LWMSG_DISPATCH_BLOCK(LSA_Q_FIND_NSS_ARTEFACT_BY_KEY, LsaSrvIpcFindNSSArtefactByKey),
    LWMSG_DISPATCH_BLOCK(LSA_Q_SET_TRACE_INFO, LsaSrvIpcSetTraceInfo),
    LWMSG_DISPATCH_BLOCK(LSA_Q_GET_TRACE_INFO, LsaSrvIpcGetTraceInfo),
    LWMSG_DISPATCH_BLOCK(LSA_Q_ENUM_TRACE_INFO, LsaSrvIpcEnumTraceInfo),
    LWMSG_DISPATCH_BLOCK(LSA_Q_PROVIDER_IO_CONTROL, LsaSrvIpcProviderIoControl),
    LWMSG_DISPATCH_BLOCK(LSA_Q_SET_MACHINE_SID, LsaSrvIpcSetMachineSid),
    LWMSG_DISPATCH_END
};

LWMsgDispatchSpec*
LsaSrvGetDispatchSpec(
    void
    )
{
    return gMessageHandlers;
}
