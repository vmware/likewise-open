/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
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
 *        getnpinfo.c
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem (LSASS)
 *
 *        GetNamedPipeInfo API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "includes.h"

SMB_API
DWORD
SMBGetNamedPipeInfo(
    HANDLE hConnection,
    HANDLE hNamedPipe,
    PDWORD pdwFlags,
    PDWORD pdwOutBufferSize,
    PDWORD pdwInBufferSize,
    PDWORD pdwMaxInstances
    )
{
    DWORD  dwError = 0;
    PIO_CONTEXT pConnection = (PIO_CONTEXT) hConnection;
    PSMB_API_HANDLE pAPIHandle = (PSMB_API_HANDLE)hNamedPipe;
    LWMsgMessageTag replyType;
    PVOID pResponse = NULL;
    PSMB_NP_INFO pNPResponse = NULL;

    BAIL_IF_NOT_FILE_HANDLE(hNamedPipe);

    dwError = MAP_LWMSG_STATUS(lwmsg_assoc_send_transact(
                                   pConnection->pAssoc,
                                   SMB_GET_NAMED_PIPE_INFO,
                                   pAPIHandle->variant.hIPCHandle,
                                   &replyType,
                                   &pResponse));
    BAIL_ON_SMB_ERROR(dwError);
    
    switch (replyType)
    {
    case SMB_GET_NAMED_PIPE_INFO_SUCCESS:
        break;
        
    case SMB_GET_NAMED_PIPE_INFO_FAILED:
        BAIL_ON_INVALID_POINTER(pResponse);
        
        dwError = ((PSMB_STATUS_REPLY)pResponse)->dwError;
        break;
        
    default:
        dwError = EINVAL;
        
        break;
    }
    BAIL_ON_SMB_ERROR(dwError);
    
    BAIL_ON_INVALID_POINTER(pResponse);

    pNPResponse = (PSMB_NP_INFO)pResponse;
    
    if (pdwFlags)
    {
        *pdwFlags = pNPResponse->dwFlags;
    }
    
    if (pdwOutBufferSize)
    {
        *pdwOutBufferSize = pNPResponse->dwOutBufferSize;
    }

    if (pdwInBufferSize)
    {
        *pdwInBufferSize = pNPResponse->dwInBufferSize;
    }

    if (pdwMaxInstances)
    {
        *pdwMaxInstances = pNPResponse->dwMaxInstances;
    }

cleanup:

    if (pResponse)
    {
        lwmsg_assoc_free_graph(pConnection->pAssoc, replyType, pResponse);
    }

    return dwError;

error:

    if (pdwFlags)
    {
        *pdwFlags = 0;
    }

    if (pdwOutBufferSize)
    {
        *pdwOutBufferSize = 0;
    }

    if (pdwInBufferSize)
    {
        *pdwInBufferSize = 0;
    }

    if (pdwMaxInstances)
    {
        *pdwMaxInstances = 0;
    }

    goto cleanup;
}

