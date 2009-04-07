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
 *        ipc_dcinfo.c
 *
 * Abstract:
 *
 *        Likewise Site Manager
 * 
 *        Inter-process communication (Server) API for querying DC Info
 *
 * Authors: Brian Dunstan (bdunstan@likewisesoftware.com)
 *         
 */
#include "includes.h"

static DWORD
LWNetSrvIpcCreateError(
    DWORD dwErrorCode,
    PCSTR pszErrorMessage,
    PLWNET_IPC_ERROR* ppError
    )
{
    DWORD dwError = 0;
    PLWNET_IPC_ERROR pError = NULL;

    dwError = LWNetAllocateMemory(sizeof(*pError), (void**) (void*) &pError);
    BAIL_ON_LWNET_ERROR(dwError);

    if (pszErrorMessage)
    {
        dwError = LWNetAllocateString(pszErrorMessage, (PSTR*) &pError->pszErrorMessage);
        BAIL_ON_LWNET_ERROR(dwError);
    }

    pError->dwError = dwErrorCode;

    *ppError = pError;

error:

    return dwError;
}

LWMsgStatus
LWNetSrvIpcGetDCName(
    LWMsgAssoc* assoc,
    const LWMsgMessage* pRequest,
    LWMsgMessage* pResponse,
    void* data
    )
{
    DWORD dwError = 0;
    PLWNET_DC_INFO pDCInfo = NULL;
    PLWNET_IPC_DCNAME_REQ pReq = pRequest->object;
    PLWNET_IPC_ERROR pError = NULL;

    dwError = LWNetSrvGetDCName(
        pReq->pszServerFQDN,
        pReq->pszDomainFQDN,
        pReq->pszSiteName,
        pReq->dwFlags,
        pReq->dwBlackListCount,
        pReq->ppszAddressBlackList,
        &pDCInfo);

    if (!dwError)
    {
        pResponse->tag = LWNET_R_DCINFO_SUCCESS;
        pResponse->object = pDCInfo;
    }
    else
    {
        dwError = LWNetSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LWNET_ERROR(dwError);
        
        pResponse->tag = LWNET_R_DCINFO_FAILURE;;
        pResponse->object = pError;
    }
    
cleanup:

    return MAP_LWNET_ERROR(dwError);

error:

    if(pDCInfo != NULL)
    {
        LWNetFreeDCInfo(pDCInfo);
    }

    goto cleanup;
}

DWORD
LWNetSrvIpcGetDCTime(
    LWMsgAssoc* assoc,
    const LWMsgMessage* pRequest,
    LWMsgMessage* pResponse,
    void* data
    )
{
    DWORD dwError = 0;
    PLWNET_IPC_DCTIME_REQ pReq = pRequest->object;
    PLWNET_IPC_DCTIME_RES pRes = NULL;
    PLWNET_IPC_ERROR pError = NULL;

    dwError = LWNetAllocateMemory(sizeof(*pRes), (void**) (void*) &pRes);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetSrvGetDCTime(
        pReq->pszDomainFQDN,
        &pRes->dcTime);

    if (!dwError)
    {
        pResponse->tag = LWNET_R_DCTIME_SUCCESS;
        pResponse->object = pRes;
    }
    else
    {
        dwError = LWNetSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LWNET_ERROR(dwError);
        
        pResponse->tag = LWNET_R_DCTIME_FAILURE;
        pResponse->object = pError;
    }

cleanup:

    /* If we are returning an error, free the response structure we allocated */
    if (pError && pRes)
    {
        LWNetFreeMemory(pRes);
    }

    return MAP_LWNET_ERROR(dwError);
    
error:

    goto cleanup;
}

DWORD
LWNetSrvIpcGetDomainController(
    LWMsgAssoc* assoc,
    const LWMsgMessage* pRequest,
    LWMsgMessage* pResponse,
    void* data
    )
{
    DWORD dwError = 0;
    PLWNET_IPC_DC_REQ pReq = pRequest->object;
    PLWNET_IPC_DC_RES pRes = NULL;
    PLWNET_IPC_ERROR pError = NULL;
    
    dwError = LWNetAllocateMemory(sizeof(*pRes), (void**) (void*) &pRes);
    BAIL_ON_LWNET_ERROR(dwError);
    
    dwError = LWNetSrvGetDomainController(
        pReq->pszDomainFQDN,
        &pRes->pszDCFQDN
        );
    
    if (!dwError)
    {
        pResponse->tag = LWNET_R_DC_SUCCESS;
        pResponse->object = pRes;
        
    }
    else
    {
        dwError = LWNetSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LWNET_ERROR(dwError);
        
        pResponse->tag = LWNET_R_DC_FAILURE;
        pResponse->object = pError;
    }

cleanup:

    /* If we are responding with an error, free the response structure we allocated */
    if (pError && pRes)
    {
        LWNetFreeMemory(pRes);
    }

    return MAP_LWNET_ERROR(dwError);
    
error:

    LWNET_SAFE_FREE_MEMORY(pRes);

    goto cleanup;
}

DWORD
LWNetSrvIpcGetCurrentDomain(
    LWMsgAssoc* assoc,
    const LWMsgMessage* pRequest,
    LWMsgMessage* pResponse,
    void* data
    )
{
    DWORD dwError = 0;
    PLWNET_IPC_CURRENT_RES pRes = NULL;
    PLWNET_IPC_ERROR pError = NULL;
    
    dwError = LWNetAllocateMemory(sizeof(*pRes), (void**) (void*) &pRes);
    BAIL_ON_LWNET_ERROR(dwError);
    
    dwError = LWNetSrvGetCurrentDomain(
        &pRes->pszDomainFQDN
        );
    
    if (!dwError)
    {
        pResponse->tag = LWNET_R_CURRENT_DOMAIN_SUCCESS;
        pResponse->object = pRes;
    }
    else
    {
        dwError = LWNetSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LWNET_ERROR(dwError);
        
        pResponse->tag = LWNET_R_CURRENT_DOMAIN_FAILURE;
        pResponse->object = pError;
    }
    
cleanup:

    if (pRes && pError)
    {
        LWNetFreeMemory(pRes);
    }

    return MAP_LWNET_ERROR(dwError);
    
error:

    LWNET_SAFE_FREE_MEMORY(pRes);

    goto cleanup;
}

