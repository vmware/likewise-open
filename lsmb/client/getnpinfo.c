/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
    PSMB_SERVER_CONNECTION pConnection = (PSMB_SERVER_CONNECTION) hConnection;
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

