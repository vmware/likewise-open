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

    dwError = MAP_LWMSG_ERROR(lwmsg_connection_new(pContext->pProtocol, &pContext->pAssoc));
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_connection_set_endpoint(
                                  pContext->pAssoc, 
                                  LWMSG_CONNECTION_MODE_LOCAL,
                                  LWNET_CACHE_DIR "/" LWNET_SERVER_FILENAME));
    BAIL_ON_LWNET_ERROR(dwError);
    
    *phConnection = (HANDLE)pContext;

cleanup:

    return dwError;

error:

    if (pContext)
    {
        if (pContext->pAssoc)
        {
            lwmsg_assoc_delete(pContext->pAssoc);
        }

        if (pContext->pProtocol)
        {
            lwmsg_protocol_delete(pContext->pProtocol);
        }

        LWNetFreeMemory(pContext);
    }

    if (phConnection) 
    {
        *phConnection = (HANDLE)NULL;
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

    if (pContext->pAssoc)
    {
        lwmsg_assoc_close(pContext->pAssoc);
        lwmsg_assoc_delete(pContext->pAssoc);
    }

    if (pContext->pProtocol)
    {
        lwmsg_protocol_delete(pContext->pProtocol);
    }

    LWNetFreeMemory(pContext);

    return dwError;
}


DWORD
LWNetTransactGetDCName(
    HANDLE hConnection,
    PCSTR pszServerFQDN,
    PCSTR pszDomainFQDN,
    PCSTR pszSiteName,
    DWORD dwFlags,
    PLWNET_DC_INFO* ppDCInfo
    )
{
    DWORD dwError = 0;
    PLWNET_CLIENT_CONNECTION_CONTEXT pContext =
                     (PLWNET_CLIENT_CONNECTION_CONTEXT)hConnection;
    LWNET_IPC_DCNAME_REQ dcReq;
    PLWNET_IPC_ERROR pError = NULL;

    LWMsgMessage request = {-1, NULL};
    LWMsgMessage response = {-1, NULL};

    dcReq.pszServerFQDN = pszServerFQDN;
    dcReq.pszDomainFQDN = pszDomainFQDN;
    dcReq.pszSiteName = pszSiteName;
    dcReq.dwFlags = dwFlags;

    request.tag = LWNET_Q_DCINFO;
    request.object = &dcReq;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_send_message_transact(
                                  pContext->pAssoc,
                                  &request,
                                  &response));
    BAIL_ON_LWNET_ERROR(dwError);
    
    switch (response.tag)
    {
    case LWNET_R_DCINFO_SUCCESS:
        *ppDCInfo = (PLWNET_DC_INFO) response.object;
        break;
    case LWNET_R_DCINFO_FAILURE:
        pError = (PLWNET_IPC_ERROR) response.object;
        dwError = pError->dwError;
        BAIL_ON_LWNET_ERROR(dwError);
        break;
    default:
        dwError = EINVAL;
        BAIL_ON_LWNET_ERROR(dwError);
    }

cleanup:

    return dwError;

error:

    if (response.object)
    {
        lwmsg_assoc_free_message(pContext->pAssoc, &response);
    }

    if (ppDCInfo)
    {
        *ppDCInfo = NULL;
    }
                    
    goto cleanup;
}

DWORD
LWNetTransactGetDCTime(
    HANDLE hConnection,
    PCSTR pszDomainFQDN,
    PUNIX_TIME_T pDCTime
    )
{
    DWORD dwError = 0;
    PLWNET_CLIENT_CONNECTION_CONTEXT pContext =
                     (PLWNET_CLIENT_CONNECTION_CONTEXT)hConnection;
    PLWNET_IPC_ERROR pError = NULL;
    LWNET_IPC_DCTIME_REQ dcTimeReq;
    LWMsgMessage request = {-1, NULL};
    LWMsgMessage response = {-1, NULL};

    dcTimeReq.pszDomainFQDN = pszDomainFQDN;
    request.tag = LWNET_Q_DCTIME;
    request.object = &dcTimeReq;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_send_message_transact(
                                  pContext->pAssoc,
                                  &request,
                                  &response));
    BAIL_ON_LWNET_ERROR(dwError);
    
    switch (response.tag)
    {
    case LWNET_R_DCTIME_SUCCESS:
        *pDCTime = ((PLWNET_IPC_DCTIME_RES) response.object)->dcTime;
        break;
    case LWNET_R_DCTIME_FAILURE:
        pError = (PLWNET_IPC_ERROR) response.object;
        dwError = pError->dwError;
        BAIL_ON_LWNET_ERROR(dwError);
        break;
    default:
        dwError = EINVAL;
        BAIL_ON_LWNET_ERROR(dwError);
    }

cleanup:

    if (response.object)
    {
        lwmsg_assoc_free_message(pContext->pAssoc, &response);
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
    PLWNET_CLIENT_CONNECTION_CONTEXT pContext =
        (PLWNET_CLIENT_CONNECTION_CONTEXT)hConnection;
    PLWNET_IPC_ERROR pError = NULL;
    LWNET_IPC_DC_REQ dcReq;
    PLWNET_IPC_DC_RES dcRes = NULL;
    LWMsgMessage request = {-1, NULL};
    LWMsgMessage response = {-1, NULL};

    dcReq.pszDomainFQDN = pszDomainFQDN;
    request.tag = LWNET_Q_DC;
    request.object = &dcReq;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_send_message_transact(
                                  pContext->pAssoc,
                                  &request,
                                  &response));
    BAIL_ON_LWNET_ERROR(dwError);
    
    switch (response.tag)
    {
    case LWNET_R_DC_SUCCESS:
        dcRes = response.object;
        *ppszDomainControllerFQDN = dcRes->pszDCFQDN;
        /* NULL out the field so it does not get freed */
        dcRes->pszDCFQDN = NULL;
        break;
    case LWNET_R_DC_FAILURE:
        pError = (PLWNET_IPC_ERROR) response.object;
        dwError = pError->dwError;
        BAIL_ON_LWNET_ERROR(dwError);
        break;
    default:
        dwError = EINVAL;
        BAIL_ON_LWNET_ERROR(dwError);
    }

cleanup:

    if (response.object)
    {
        lwmsg_assoc_free_message(pContext->pAssoc, &response);
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
    PLWNET_CLIENT_CONNECTION_CONTEXT pContext =
        (PLWNET_CLIENT_CONNECTION_CONTEXT)hConnection;
    PLWNET_IPC_ERROR pError = NULL;
    PLWNET_IPC_CURRENT_RES pCurRes = NULL;

    LWMsgMessage request = {-1, NULL};
    LWMsgMessage response = {-1, NULL};

    request.tag = LWNET_Q_CURRENT_DOMAIN;
    request.object = NULL;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_send_message_transact(
                                  pContext->pAssoc,
                                  &request,
                                  &response));
    BAIL_ON_LWNET_ERROR(dwError);
    
    switch (response.tag)
    {
    case LWNET_R_CURRENT_DOMAIN_SUCCESS:
        pCurRes = response.object;
        *ppszDomainFQDN = pCurRes->pszDomainFQDN;
        pCurRes->pszDomainFQDN = NULL;
        break;
    case LWNET_R_CURRENT_DOMAIN_FAILURE:
        pError = (PLWNET_IPC_ERROR) response.object;
        dwError = pError->dwError;
        BAIL_ON_LWNET_ERROR(dwError);
        break;
    default:
        dwError = EINVAL;
        BAIL_ON_LWNET_ERROR(dwError);
    }

cleanup:

    if (response.object)
    {
        lwmsg_assoc_free_message(pContext->pAssoc, &response);
    }

    return dwError;

error:

    goto cleanup;
}
