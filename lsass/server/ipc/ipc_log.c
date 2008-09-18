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
 *        ipc_log.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 * 
 *        Inter-process communication (Server) API for Log Info
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "ipc.h"

DWORD
LsaSrvIpcSetLogInfo(
    HANDLE      hConnection,
    PLSAMESSAGE pMessage
    )
{
    DWORD  dwError = 0;
    PLSAMESSAGE pResponse = NULL;
    PLSASERVERCONNECTIONCONTEXT pContext  =
         (PLSASERVERCONNECTIONCONTEXT)hConnection;
    HANDLE hServer = (HANDLE)NULL;
    PLSA_LOG_INFO pLogInfo = NULL;
    
    dwError = LsaUnmarshalLogInfo(
                    pMessage->pData,
                    pMessage->header.messageLength,
                    &pLogInfo);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaSrvIpcOpenServer(hConnection, &hServer);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaSrvSetLogInfo(
                    hServer,
                    pLogInfo);
    if (!dwError) {
        
       dwError = LsaBuildMessage(
                    LSA_R_SET_LOGINFO,
                    0, /* Empty message */
                    1,
                    1,
                    &pResponse);
       BAIL_ON_LSA_ERROR(dwError);
       
    } else {
       
        dwError = LsaSrvIpcMarshalError(
                        dwError,
                        &pResponse);
        BAIL_ON_LSA_ERROR(dwError);
        
    }

    dwError = LsaWriteMessage(pContext->fd, pResponse);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    if (pLogInfo) {
        LsaFreeLogInfo(pLogInfo);
    }
    
    LSA_SAFE_FREE_MESSAGE(pResponse);
    
    return dwError;
    
error:

    goto cleanup;
}

DWORD
LsaSrvIpcGetLogInfo(
    HANDLE      hConnection,
    PLSAMESSAGE pMessage
    )
{
    DWORD dwError = 0;
    PLSAMESSAGE pResponse = NULL;
    DWORD dwMsgLen = 0;
    PLSASERVERCONNECTIONCONTEXT pContext =
       (PLSASERVERCONNECTIONCONTEXT)hConnection;
    HANDLE hServer = (HANDLE)NULL;
    PLSA_LOG_INFO pLogInfo = NULL;
    
    dwError = LsaSrvIpcOpenServer(hConnection, &hServer);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaSrvGetLogInfo(
                    hServer,
                    &pLogInfo);
    if (dwError) {
        
        dwError = LsaSrvIpcMarshalError(
                        dwError,
                        &pResponse);
        BAIL_ON_LSA_ERROR(dwError);
        
    } else {

       dwError = LsaMarshalLogInfo(
                       pLogInfo->maxAllowedLogLevel,
                       pLogInfo->logTarget,
                       pLogInfo->pszPath,
                       NULL,
                       &dwMsgLen);
       BAIL_ON_LSA_ERROR(dwError);
    
       dwError = LsaBuildMessage(
                        LSA_R_GET_LOGINFO,
                        dwMsgLen,
                        1,
                        1,
                        &pResponse
                        );
       BAIL_ON_LSA_ERROR(dwError);
    
       dwError = LsaMarshalLogInfo(
                       pLogInfo->maxAllowedLogLevel,
                       pLogInfo->logTarget,
                       pLogInfo->pszPath,
                       pResponse->pData,
                       &dwMsgLen);
       BAIL_ON_LSA_ERROR(dwError);
    }
    
    dwError = LsaWriteMessage(pContext->fd, pResponse);
    BAIL_ON_LSA_ERROR(dwError);
    
cleanup:

    if (pLogInfo) {
        LsaFreeLogInfo(pLogInfo);
    }
    
    LSA_SAFE_FREE_MESSAGE(pResponse);
    
    return dwError;
    
error:

    goto cleanup;
}
