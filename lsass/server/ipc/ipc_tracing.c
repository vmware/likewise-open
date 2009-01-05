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
 *        ipc_tracing.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Inter-process communication (Server) API for Tracing Info
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "ipc.h"

LWMsgStatus
LsaSrvIpcSetTraceInfo(
    LWMsgAssoc* assoc,
    const LWMsgMessage* pRequest,
    LWMsgMessage* pResponse,
    void* data
    )
{
    DWORD dwError = 0;
    PLSA_IPC_SET_TRACE_INFO_REQ pReq = pRequest->object;
    PLSA_IPC_ERROR pError = NULL;
    PVOID Handle = lwmsg_assoc_get_session_data(assoc);

    dwError = LsaSrvSetTraceFlags(
                        (HANDLE)Handle,
                        pReq->pTraceFlagArray,
                        pReq->dwNumFlags);

    if (!dwError)
    {
        pResponse->tag = LSA_R_SET_TRACE_INFO_SUCCESS;
        pResponse->object = NULL;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pResponse->tag = LSA_R_SET_TRACE_INFO_FAILURE;
        pResponse->object = pError;
    }

cleanup:
    return MAP_LSA_ERROR_IPC(dwError);

error:
    goto cleanup;
}

LWMsgStatus
LsaSrvIpcGetTraceInfo(
    LWMsgAssoc* assoc,
    const LWMsgMessage* pRequest,
    LWMsgMessage* pResponse,
    void* data
    )
{
    DWORD dwError = 0;
    PLSA_TRACE_INFO_LIST pResult = NULL;
    PLSA_TRACE_INFO pTraceInfo = NULL;
    PLSA_IPC_ERROR pError = NULL;
    PVOID Handle = lwmsg_assoc_get_session_data(assoc);

    dwError = LsaSrvGetTraceInfo(
                        (HANDLE)Handle,
                        *(PDWORD)pRequest->object,
                        &pTraceInfo);

    if (!dwError)
    {
        dwError = LsaAllocateMemory(sizeof(*pResult),
                                    (PVOID)&pResult);
        BAIL_ON_LSA_ERROR(dwError);

        pResult->dwNumFlags = 1;
        pResult->pTraceInfoArray = pTraceInfo;
        pTraceInfo = NULL;

        pResponse->tag = LSA_R_GET_TRACE_INFO_SUCCESS;
        pResponse->object = pResult;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pResponse->tag = LSA_R_GET_TRACE_INFO_FAILURE;
        pResponse->object = pError;
    }

cleanup:
    LSA_SAFE_FREE_MEMORY(pTraceInfo);

    return MAP_LSA_ERROR_IPC(dwError);

error:
    if (pResult)
    {
        LSA_SAFE_FREE_MEMORY(pResult->pTraceInfoArray);
        LsaFreeMemory(pResult);
    }

    goto cleanup;
}

LWMsgStatus
LsaSrvIpcEnumTraceInfo(
    LWMsgAssoc* assoc,
    const LWMsgMessage* pRequest,
    LWMsgMessage* pResponse,
    void* data
    )
{
    DWORD dwError = 0;
    PLSA_TRACE_INFO_LIST pResult = NULL;
    PLSA_IPC_ERROR pError = NULL;
    PVOID Handle = lwmsg_assoc_get_session_data(assoc);

    dwError = LsaAllocateMemory(sizeof(*pResult),
                               (PVOID)&pResult);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvEnumTraceFlags(
                       (HANDLE)Handle,
                       &pResult->pTraceInfoArray,
                       &pResult->dwNumFlags);

    if (!dwError)
    {
        pResponse->tag = LSA_R_ENUM_TRACE_INFO_SUCCESS;
        pResponse->object = pResult;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pResponse->tag = LSA_R_ENUM_TRACE_INFO_FAILURE;;
        pResponse->object = pError;
    }

cleanup:
    return MAP_LSA_ERROR_IPC(dwError);

error:
    if (pResult)
    {
        LSA_SAFE_FREE_MEMORY(pResult->pTraceInfoArray);
        LsaFreeMemory(pResult);
    }

    goto cleanup;
}
