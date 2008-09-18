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

DWORD
LWNetSrvIpcGetDCName(
    HANDLE hConnection,
    PLWNETMESSAGE pMessage
    )
{
    DWORD dwError = 0;
    PLWNET_DC_INFO pDCInfo = NULL;
    PLWNETMESSAGE pResponse = NULL;
    DWORD dwBufferLength = 0;
    PSTR pszSiteName = NULL;
    PSTR pszDomainFQDN = NULL;
    PSTR pszServerFQDN = NULL;
    DWORD dwFlags = 0;
    
    PLWNETSERVERCONNECTIONCONTEXT pContext  =
             (PLWNETSERVERCONNECTIONCONTEXT)hConnection;
    
    dwError = LWNetUnmarshalDCNameReq(
                pMessage->pData,
                pMessage->header.messageLength,
                &pszServerFQDN,
                &pszDomainFQDN,
                &pszSiteName,
                &dwFlags
                );
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetSrvGetDCName(
                pszServerFQDN,
                pszDomainFQDN,
                pszSiteName,
                dwFlags,
                &pDCInfo
                );
    if (!dwError) {
        
        dwError = LWNetMarshalDCInfo(
                    pDCInfo,
                    NULL,
                    &dwBufferLength);
        BAIL_ON_LWNET_ERROR(dwError);
        
        dwError = LWNetBuildMessage(
                    LWNET_R_DCINFO,
                    dwBufferLength,
                    1,
                    1,
                    &pResponse);
        BAIL_ON_LWNET_ERROR(dwError);

        dwError = LWNetMarshalDCInfo(
                    pDCInfo,
                    pResponse->pData,
                    &dwBufferLength);
        BAIL_ON_LWNET_ERROR(dwError);
       
    }
    else {
       
       DWORD dwOrigErrCode = 0;
       DWORD dwMsgLen = 0;
       
       dwOrigErrCode = dwError;
       
       dwError = LWNetMarshalError(dwOrigErrCode, NULL, NULL, &dwMsgLen);
       BAIL_ON_LWNET_ERROR(dwError);
        
       dwError = LWNetBuildMessage(
                    LWNET_ERROR,
                    dwMsgLen,
                    1,
                    1,
                    &pResponse
                    );
       BAIL_ON_LWNET_ERROR(dwError);
        
       dwError = LWNetMarshalError(dwOrigErrCode, NULL, pResponse->pData, &dwMsgLen);
       BAIL_ON_LWNET_ERROR(dwError);
    }

    dwError = LWNetWriteMessage(
                pContext->fd,
                pResponse);
    BAIL_ON_LWNET_ERROR(dwError);
    
cleanup:

    LWNET_SAFE_FREE_STRING(pszServerFQDN);
    LWNET_SAFE_FREE_STRING(pszDomainFQDN);
    LWNET_SAFE_FREE_STRING(pszSiteName);
    if(pDCInfo != NULL)
    {
        LWNetFreeDCInfo(pDCInfo);
    }

    LWNET_SAFE_FREE_MESSAGE(pResponse);

    return dwError;
    
error:

    goto cleanup;
}



DWORD
LWNetSrvIpcGetDCTime(
    HANDLE hConnection,
    PLWNETMESSAGE pMessage
    )
{
    DWORD dwError = 0;
    PLWNETMESSAGE pResponse = NULL;
    DWORD dwBufferLength = 0;
    UNIX_TIME_T dcTime = 0;
    PSTR pszDomainFQDN = NULL;
    
    PLWNETSERVERCONNECTIONCONTEXT pContext  =
             (PLWNETSERVERCONNECTIONCONTEXT)hConnection;

    dwError = LWNetAllocateString(
                pMessage->pData,
                &pszDomainFQDN
                );
    BAIL_ON_LWNET_ERROR(dwError);
    
    dwError = LWNetSrvGetDCTime(
                pszDomainFQDN,
                &dcTime);

    if (!dwError) {
        
        dwBufferLength = sizeof(dcTime);
        
        dwError = LWNetBuildMessage(
                    LWNET_R_DCTIME,
                    dwBufferLength,
                    1,
                    1,
                    &pResponse);
        BAIL_ON_LWNET_ERROR(dwError);

        memcpy(pResponse->pData, &dcTime, dwBufferLength);
       
    }
    else {
       
       DWORD dwOrigErrCode = 0;
       DWORD dwMsgLen = 0;
       
       dwOrigErrCode = dwError;
       
       dwError = LWNetMarshalError(dwOrigErrCode, NULL, NULL, &dwMsgLen);
       BAIL_ON_LWNET_ERROR(dwError);
        
       dwError = LWNetBuildMessage(
                    LWNET_ERROR,
                    dwMsgLen,
                    1,
                    1,
                    &pResponse
                    );
       BAIL_ON_LWNET_ERROR(dwError);
        
       dwError = LWNetMarshalError(dwOrigErrCode, NULL, pResponse->pData, &dwMsgLen);
       BAIL_ON_LWNET_ERROR(dwError);
    }

    dwError = LWNetWriteMessage(
                pContext->fd,
                pResponse);
    BAIL_ON_LWNET_ERROR(dwError);
    
cleanup:

    LWNET_SAFE_FREE_STRING(pszDomainFQDN);

    LWNET_SAFE_FREE_MESSAGE(pResponse);

    return dwError;
    
error:

    goto cleanup;
}

DWORD
LWNetSrvIpcGetDomainController(
    HANDLE hConnection,
    PLWNETMESSAGE pMessage
    )
{
    DWORD dwError = 0;
    PLWNETMESSAGE pResponse = NULL;
    DWORD dwBufferLength = 0;
    PSTR pszDomainFQDN = NULL;
    PSTR pszDomainControllerFQDN = NULL;
    
    PLWNETSERVERCONNECTIONCONTEXT pContext  =
             (PLWNETSERVERCONNECTIONCONTEXT)hConnection;
    
    if(!pMessage->pData || *(pMessage->pData) == '\0' || 
       strlen(pMessage->pData)+1 > pMessage->header.messageLength)
    {
        dwError = LWNET_ERROR_DATA_ERROR;
        BAIL_ON_LWNET_ERROR(dwError);
    }
    
    dwError = LWNetAllocateString(
                pMessage->pData,
                &pszDomainFQDN
                );
    BAIL_ON_LWNET_ERROR(dwError);
    
    dwError = LWNetSrvGetDomainController(
                pszDomainFQDN,
                &pszDomainControllerFQDN
                );
    BAIL_ON_LWNET_ERROR(dwError);
    
    if (!dwError) {
        
        dwBufferLength = strlen(pszDomainControllerFQDN) + 1;
        
        dwError = LWNetBuildMessage(
                    LWNET_R_DC,
                    dwBufferLength,
                    1,
                    1,
                    &pResponse);
        BAIL_ON_LWNET_ERROR(dwError);

        strncpy(pResponse->pData, pszDomainControllerFQDN, dwBufferLength);
        
    }
    else {
       
       DWORD dwOrigErrCode = 0;
       DWORD dwMsgLen = 0;
       
       dwOrigErrCode = dwError;
       
       dwError = LWNetMarshalError(dwOrigErrCode, NULL, NULL, &dwMsgLen);
       BAIL_ON_LWNET_ERROR(dwError);
        
       dwError = LWNetBuildMessage(
                    LWNET_ERROR,
                    dwMsgLen,
                    1,
                    1,
                    &pMessage
                    );
       BAIL_ON_LWNET_ERROR(dwError);
        
       dwError = LWNetMarshalError(dwOrigErrCode, NULL, pMessage->pData, &dwMsgLen);
       BAIL_ON_LWNET_ERROR(dwError);
    }

    dwError = LWNetWriteMessage(
                pContext->fd,
                pResponse);
    BAIL_ON_LWNET_ERROR(dwError);
    
cleanup:

    LWNET_SAFE_FREE_STRING(pszDomainFQDN);

    LWNET_SAFE_FREE_MESSAGE(pResponse);

    return dwError;
    
error:

    goto cleanup;
}

DWORD
LWNetSrvIpcGetCurrentDomain(
    HANDLE hConnection,
    PLWNETMESSAGE pMessage
    )
{
    DWORD dwError = 0;
    PLWNETMESSAGE pResponse = NULL;
    DWORD dwBufferLength = 0;
    PSTR pszDomainFQDN = NULL;

    PLWNETSERVERCONNECTIONCONTEXT pContext  =
             (PLWNETSERVERCONNECTIONCONTEXT)hConnection;
    
    
    dwError = LWNetSrvGetCurrentDomain(
                &pszDomainFQDN
                );
    
    if (!dwError) {
        
        dwBufferLength = strlen(pszDomainFQDN) + 1;
        
        dwError = LWNetBuildMessage(
                    LWNET_R_CURRENT_DOMAIN,
                    dwBufferLength,
                    1,
                    1,
                    &pResponse);
        BAIL_ON_LWNET_ERROR(dwError);

        strncpy(pResponse->pData, pszDomainFQDN, dwBufferLength);
        
    }
    else {
       
       DWORD dwOrigErrCode = 0;
       DWORD dwMsgLen = 0;
       
       dwOrigErrCode = dwError;
       
       dwError = LWNetMarshalError(dwOrigErrCode, NULL, NULL, &dwMsgLen);
       BAIL_ON_LWNET_ERROR(dwError);
        
       dwError = LWNetBuildMessage(
                    LWNET_ERROR,
                    dwMsgLen,
                    1,
                    1,
                    &pMessage
                    );
       BAIL_ON_LWNET_ERROR(dwError);
        
       dwError = LWNetMarshalError(dwOrigErrCode, NULL, pMessage->pData, &dwMsgLen);
       BAIL_ON_LWNET_ERROR(dwError);
    }

    dwError = LWNetWriteMessage(
                pContext->fd,
                pResponse);
    BAIL_ON_LWNET_ERROR(dwError);
    
cleanup:

    LWNET_SAFE_FREE_STRING(pszDomainFQDN);

    LWNET_SAFE_FREE_MESSAGE(pResponse);

    return dwError;
    
error:

    goto cleanup;
}

