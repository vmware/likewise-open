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
 *        ipc_client.c
 *
 * Abstract:
 *
 *        Likewise Site Manager
 *
 *        Inter-process Communication (Client) API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 * 
 */
#include "includes.h"

DWORD
LWNetOpenServer(
    PHANDLE phConnection
    )
{
    DWORD dwError = 0;
    PLWNET_CLIENT_CONNECTION_CONTEXT pContext = NULL;

    BAIL_ON_INVALID_POINTER(phConnection);

    dwError = LWNetAllocateMemory(sizeof(LWNET_CLIENT_CONNECTION_CONTEXT), (PVOID*)&pContext);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_protocol_new(NULL, &pContext->pProtocol));
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_protocol_add_protocol_spec(pContext->pProtocol, LWNetIPCGetProtocolSpec()));
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_client_new(NULL, pContext->pProtocol, &pContext->pClient));
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_client_set_endpoint(
                                  pContext->pClient,
                                  LWMSG_CONNECTION_MODE_LOCAL,
                                  LWNET_CACHE_DIR "/" LWNET_SERVER_FILENAME));
    BAIL_ON_LWNET_ERROR(dwError);

    *phConnection = (HANDLE)pContext;

cleanup:

    return dwError;

error:

    if (pContext)
    {
        if (pContext->pClient)
        {
            lwmsg_client_delete(pContext->pClient);
        }

        if (pContext->pProtocol)
        {
            lwmsg_protocol_delete(pContext->pProtocol);
        }

        LWNetFreeMemory(pContext);
    }

    if (phConnection)
    {
        *phConnection = NULL;
    }

    goto cleanup;
}

DWORD
LWNetCloseServer(
    HANDLE hConnection
    )
{
    DWORD dwError = 0;
    PLWNET_CLIENT_CONNECTION_CONTEXT pContext =
                     (PLWNET_CLIENT_CONNECTION_CONTEXT)hConnection;

    if (pContext->pClient)
    {
        lwmsg_client_delete(pContext->pClient);
    }

    if (pContext->pProtocol)
    {
        lwmsg_protocol_delete(pContext->pProtocol);
    }

    LWNetFreeMemory(pContext);

    return dwError;
}

DWORD
LWNetAcquireCall(
    HANDLE hConnection,
    LWMsgCall** ppCall
    )
{
    DWORD dwError = 0;
    PLWNET_CLIENT_CONNECTION_CONTEXT pContext = hConnection;

    dwError = MAP_LWMSG_ERROR(lwmsg_client_acquire_call(pContext->pClient, ppCall));
    BAIL_ON_LWNET_ERROR(dwError);

error:

    return dwError;
}

DWORD
LWNetTransactGetDCName(
    HANDLE hConnection,
    PCSTR pszServerFQDN,
    PCSTR pszDomainFQDN,
    PCSTR pszSiteName,
    DWORD dwFlags,
    DWORD dwBlackListCount,
    PSTR* ppszAddressBlackList,
    PLWNET_DC_INFO* ppDCInfo
    )
{
    DWORD dwError = 0;
    LWNET_IPC_DCNAME_REQ dcReq;
    PLWNET_IPC_ERROR pError = NULL;

    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    dwError = LWNetAcquireCall(hConnection, &pCall);
    BAIL_ON_LWNET_ERROR(dwError);

    dcReq.pszServerFQDN = pszServerFQDN;
    dcReq.pszDomainFQDN = pszDomainFQDN;
    dcReq.pszSiteName = pszSiteName;
    dcReq.dwFlags = dwFlags;
    dcReq.dwBlackListCount = dwBlackListCount;
    dcReq.ppszAddressBlackList = ppszAddressBlackList;

    in.tag = LWNET_Q_DCINFO;
    in.data = &dcReq;

    dwError = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_LWNET_ERROR(dwError);

    switch (out.tag)
    {
    case LWNET_R_DCINFO_SUCCESS:
        *ppDCInfo = (PLWNET_DC_INFO) out.data;
        out.data = NULL;
        break;
    case LWNET_R_DCINFO_FAILURE:
        pError = (PLWNET_IPC_ERROR) out.data;
        dwError = pError->dwError;
        BAIL_ON_LWNET_ERROR(dwError);
        break;
    default:
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_LWNET_ERROR(dwError);
    }

cleanup:

    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:

    if (ppDCInfo)
    {
        *ppDCInfo = NULL;
    }

    goto cleanup;
}

DWORD
LWNetTransactGetDCList(
    IN HANDLE hConnection,
    IN PCSTR pszDomainFQDN,
    IN PCSTR pszSiteName,
    IN DWORD dwFlags,
    OUT PLWNET_DC_ADDRESS* ppDcList,
    OUT LW_PDWORD pdwDcCount
    )
{
    DWORD dwError = 0;
    LWNET_IPC_DCNAME_REQ dcReq = { 0 };
    PLWNET_IPC_ERROR pError = NULL;

    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    dwError = LWNetAcquireCall(hConnection, &pCall);
    BAIL_ON_LWNET_ERROR(dwError);

    dcReq.pszDomainFQDN = pszDomainFQDN;
    dcReq.pszSiteName = pszSiteName;
    dcReq.dwFlags = dwFlags;

    in.tag = LWNET_Q_DCLIST;
    in.data = &dcReq;

    dwError = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_LWNET_ERROR(dwError);

    switch (out.tag)
    {
    case LWNET_R_DCLIST_SUCCESS:
    {
        PLWNET_IPC_DCLIST_RES pResult = (PLWNET_IPC_DCLIST_RES) out.data;
        *ppDcList = pResult->pDcList;
        pResult->pDcList = NULL;
        *pdwDcCount = pResult->dwDcCount;
        break;
    }
    case LWNET_R_DCLIST_FAILURE:
        pError = (PLWNET_IPC_ERROR) out.data;
        dwError = pError->dwError;
        BAIL_ON_LWNET_ERROR(dwError);
        break;
    default:
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_LWNET_ERROR(dwError);
    }

cleanup:

    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:

    *ppDcList = NULL;
    *pdwDcCount = 0;

    goto cleanup;

}

DWORD
LWNetTransactGetDCTime(
    HANDLE hConnection,
    PCSTR pszDomainFQDN,
    PLWNET_UNIX_TIME_T pDCTime
    )
{
    DWORD dwError = 0;
    PLWNET_IPC_ERROR pError = NULL;
    LWNET_IPC_DCTIME_REQ dcTimeReq;
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    dwError = LWNetAcquireCall(hConnection, &pCall);
    BAIL_ON_LWNET_ERROR(dwError);

    dcTimeReq.pszDomainFQDN = pszDomainFQDN;
    in.tag = LWNET_Q_DCTIME;
    in.data = &dcTimeReq;

    dwError = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_LWNET_ERROR(dwError);

    switch (out.tag)
    {
    case LWNET_R_DCTIME_SUCCESS:
        *pDCTime = ((PLWNET_IPC_DCTIME_RES) out.data)->dcTime;
        break;
    case LWNET_R_DCTIME_FAILURE:
        pError = (PLWNET_IPC_ERROR) out.data;
        dwError = pError->dwError;
        BAIL_ON_LWNET_ERROR(dwError);
        break;
    default:
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_LWNET_ERROR(dwError);
    }

cleanup:

    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
LWNetTransactGetDomainController(
    HANDLE hConnection,
    PCSTR pszDomainFQDN,
    PSTR* ppszDomainControllerFQDN
    )
{
    DWORD dwError = 0;
    PLWNET_IPC_ERROR pError = NULL;
    LWNET_IPC_DC_REQ dcReq;
    PLWNET_IPC_DC_RES dcRes = NULL;
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    dwError = LWNetAcquireCall(hConnection, &pCall);
    BAIL_ON_LWNET_ERROR(dwError);

    dcReq.pszDomainFQDN = pszDomainFQDN;
    in.tag = LWNET_Q_DC;
    in.data = &dcReq;

    dwError = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_LWNET_ERROR(dwError);

    switch (out.tag)
    {
    case LWNET_R_DC_SUCCESS:
        dcRes = out.data;
        *ppszDomainControllerFQDN = dcRes->pszDCFQDN;
        /* NULL out the field so it does not get freed */
        dcRes->pszDCFQDN = NULL;
        break;
    case LWNET_R_DC_FAILURE:
        pError = (PLWNET_IPC_ERROR) out.data;
        dwError = pError->dwError;
        BAIL_ON_LWNET_ERROR(dwError);
        break;
    default:
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_LWNET_ERROR(dwError);
    }

cleanup:

    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
LWNetTransactGetCurrentDomain(
    HANDLE hConnection,
    PSTR* ppszDomainFQDN
    )
{
    DWORD dwError = 0;
    PLWNET_IPC_ERROR pError = NULL;
    PLWNET_IPC_CURRENT_RES pCurRes = NULL;

    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    dwError = LWNetAcquireCall(hConnection, &pCall);
    BAIL_ON_LWNET_ERROR(dwError);

    in.tag = LWNET_Q_CURRENT_DOMAIN;
    in.data = NULL;

    dwError = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_LWNET_ERROR(dwError);

    switch (out.tag)
    {
    case LWNET_R_CURRENT_DOMAIN_SUCCESS:
        pCurRes = out.data;
        *ppszDomainFQDN = pCurRes->pszDomainFQDN;
        pCurRes->pszDomainFQDN = NULL;
        break;
    case LWNET_R_CURRENT_DOMAIN_FAILURE:
        pError = (PLWNET_IPC_ERROR) out.data;
        dwError = pError->dwError;
        BAIL_ON_LWNET_ERROR(dwError);
        break;
    default:
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_LWNET_ERROR(dwError);
    }

cleanup:

    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:

    goto cleanup;
}
