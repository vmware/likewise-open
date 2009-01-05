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
 *        ipc_metrics.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Inter-process communication (Server) API for Metrics
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "ipc.h"

LWMsgStatus
LsaSrvIpcGetMetrics(
    LWMsgAssoc* assoc,
    const LWMsgMessage* pRequest,
    LWMsgMessage* pResponse,
    void* data
    )
{
    DWORD dwError = 0;
    PVOID pMetricPack = NULL;
    PLSA_METRIC_PACK pResult = NULL;
    PLSA_IPC_ERROR pError = NULL;
    DWORD dwInfoLevel = *(PDWORD)pRequest->object;
    PVOID Handle = lwmsg_assoc_get_session_data(assoc);

    dwError = LsaSrvGetMetrics(
                        (HANDLE)Handle,
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
                dwError = LSA_ERROR_INVALID_PARAMETER;
                BAIL_ON_LSA_ERROR(dwError);
        }

        pResponse->tag = LSA_R_GET_METRICS_SUCCESS;
        pResponse->object = pResult;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pResponse->tag = LSA_R_GET_METRICS_FAILURE;
        pResponse->object = pError;
    }

cleanup:
    if(pMetricPack)
    {
        LSA_SAFE_FREE_MEMORY(pMetricPack);
    }

    return MAP_LSA_ERROR_IPC(dwError);

error:
    if(pResult)
    {
        LsaSrvFreeIpcMetriPack(pResult);
    }

    goto cleanup;
}
