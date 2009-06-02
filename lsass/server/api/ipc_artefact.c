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
 *        Inter-process communication (Server) API for NSSArtefacts
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "api.h"

static void
LsaSrvCleanupArtefactEnumHandle(
    void* pData
    )
{
    LsaSrvEndEnumNSSArtefacts(
        NULL,
        (HANDLE) pData);
}

LWMsgStatus
LsaSrvIpcFindNSSArtefactByKey(
    LWMsgAssoc* assoc,
    const LWMsgMessage* pRequest,
    LWMsgMessage* pResponse,
    void* data
    )
{
    DWORD dwError = 0;
    // Do not free pUserInfo
    PVOID pNSSArtefactInfo = NULL;
    PVOID* ppNSSArtefactInfo = NULL;
    PLSA_NSS_ARTEFACT_INFO_LIST pResult = NULL;
    PLSA_IPC_FIND_NSSARTEFACT_BY_KEY_REQ pReq = pRequest->object;
    PLSA_IPC_ERROR pError = NULL;
    PVOID Handle = NULL;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_get_session_data(assoc, (PVOID*) (PVOID) &Handle));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvFindNSSArtefactByKey(
                       (HANDLE)Handle,
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
                dwError = LSA_ERROR_INVALID_PARAMETER;
                BAIL_ON_LSA_ERROR(dwError);
        }

        pResponse->tag = LSA_R_FIND_NSS_ARTEFACT_BY_KEY_SUCCESS;
        pResponse->object = pResult;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pResponse->tag = LSA_R_FIND_NSS_ARTEFACT_BY_KEY_FAILURE;
        pResponse->object = pError;
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

    return MAP_LSA_ERROR_IPC(dwError);

error:
    if(pResult)
    {
        LsaFreeIpcNssArtefactInfoList(pResult);
    }

    goto cleanup;
}

LWMsgStatus
LsaSrvIpcBeginEnumNSSArtefacts(
    LWMsgAssoc* assoc,
    const LWMsgMessage* pRequest,
    LWMsgMessage* pResponse,
    void* data
    )
{
    DWORD dwError = 0;
    PSTR pszGUID = NULL;
    PLSA_IPC_BEGIN_ENUM_NSSARTEFACT_REQ pReq = pRequest->object;
    PLSA_IPC_ERROR pError = NULL;
    PVOID Handle = NULL;
    HANDLE hResume = NULL;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_get_session_data(assoc, (PVOID*) (PVOID) &Handle));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvBeginEnumNSSArtefacts(
                        (HANDLE)Handle,
                        pReq->pszMapName,
                        pReq->dwFlags,
                        pReq->dwInfoLevel,
                        pReq->dwMaxNumNSSArtefacts,
                        &hResume);

    if (!dwError)
    {
        dwError = MAP_LWMSG_ERROR(lwmsg_assoc_register_handle(
                                      assoc,
                                      "EnumArtefacts",
                                      hResume,
                                      LsaSrvCleanupArtefactEnumHandle));
        BAIL_ON_LSA_ERROR(dwError);

        pResponse->tag = LSA_R_BEGIN_ENUM_NSS_ARTEFACTS_SUCCESS;
        pResponse->object = hResume;
        hResume = NULL;

        dwError = MAP_LWMSG_ERROR(lwmsg_assoc_retain_handle(assoc, pResponse->object));
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pResponse->tag = LSA_R_BEGIN_ENUM_NSS_ARTEFACTS_FAILURE;
        pResponse->object = pError;
    }

cleanup:
    LSA_SAFE_FREE_STRING(pszGUID);

    return MAP_LSA_ERROR_IPC(dwError);

error:

    if(hResume)
    {
        LsaSrvCleanupArtefactEnumHandle(hResume);
    }

    goto cleanup;
}

LWMsgStatus
LsaSrvIpcEnumNSSArtefacts(
    LWMsgAssoc* assoc,
    const LWMsgMessage* pRequest,
    LWMsgMessage* pResponse,
    void* data
    )
{
    DWORD dwError = 0;
    PVOID* ppNSSArtefactInfoList = NULL;
    DWORD  dwNSSArtefactInfoLevel = 0;
    DWORD  dwNumNSSArtefactsFound = 0;
    PLSA_NSS_ARTEFACT_INFO_LIST pResult = NULL;
    PLSA_IPC_ERROR pError = NULL;
    PVOID Handle = NULL;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_get_session_data(assoc, (PVOID*) (PVOID) &Handle));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvEnumNSSArtefacts(
                       Handle,
                       (HANDLE) pRequest->object,
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
                dwError = LSA_ERROR_INVALID_PARAMETER;
                BAIL_ON_LSA_ERROR(dwError);
        }

        pResponse->tag = LSA_R_ENUM_NSS_ARTEFACTS_SUCCESS;
        pResponse->object = pResult;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pResponse->tag = LSA_R_ENUM_NSS_ARTEFACTS_FAILURE;;
        pResponse->object = pError;
    }

cleanup:
    if(ppNSSArtefactInfoList)
    {
        LsaFreeNSSArtefactInfoList(dwNSSArtefactInfoLevel, ppNSSArtefactInfoList, dwNumNSSArtefactsFound);
    }

    return MAP_LSA_ERROR_IPC(dwError);

error:
    if(pResult)
    {
        LsaFreeIpcNssArtefactInfoList(pResult);
    }

    goto cleanup;
}

LWMsgStatus
LsaSrvIpcEndEnumNSSArtefacts(
    LWMsgAssoc* assoc,
    const LWMsgMessage* pRequest,
    LWMsgMessage* pResponse,
    void* data
    )
{
    DWORD dwError = 0;
    PLSA_IPC_ERROR pError = NULL;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_unregister_handle(assoc, pRequest->object));
    if (!dwError)
    {
        pResponse->tag = LSA_R_END_ENUM_NSS_ARTEFACTS_SUCCESS;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pResponse->tag = LSA_R_END_ENUM_NSS_ARTEFACTS_FAILURE;
        pResponse->object = pError;
    }

cleanup:
    return MAP_LSA_ERROR_IPC(dwError);

error:
    goto cleanup;
}
