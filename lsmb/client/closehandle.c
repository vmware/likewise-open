/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        callnp.c
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem (SMB)
 *
 *        CloseHandle API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "includes.h"

SMB_API
DWORD
SMBCloseHandle(
    HANDLE hConnection,
    HANDLE hFile
    )
{
    DWORD  dwError = 0;
    PSMB_SERVER_CONNECTION pConnection = (PSMB_SERVER_CONNECTION) hConnection;
    PSMB_API_HANDLE pAPIHandle = (PSMB_API_HANDLE) hFile;
    LWMsgMessageTag replyType;
    PVOID pResponse = NULL;

    BAIL_ON_INVALID_POINTER(pAPIHandle);

    switch(pAPIHandle->type)
    {
    case SMB_API_HANDLE_FILE:
        /* The check for an invalid hConnection must go here because
           hConnection may be NULL if we are closing API handles with
           no server-side state */
        BAIL_ON_INVALID_POINTER(hConnection);

        dwError = MAP_LWMSG_STATUS(lwmsg_assoc_send_transact(
                                       pConnection->pAssoc,
                                       SMB_CLOSE_FILE,
                                       pAPIHandle->variant.hIPCHandle,
                                       &replyType,
                                       &pResponse));
        BAIL_ON_SMB_ERROR(dwError);
        
        switch (replyType)
        {
        case SMB_CLOSE_FILE_SUCCESS:
            break;
            
        case SMB_CLOSE_FILE_FAILED:
            
            BAIL_ON_INVALID_POINTER(pResponse);
            
            dwError = ((PSMB_STATUS_REPLY)pResponse)->dwError;
            
            break;
            
        default:
            
            dwError = EINVAL;
            
            break;
        }
        BAIL_ON_SMB_ERROR(dwError);
    
        /* The handle will no longer be used, so unregister it from the assoc */
        dwError = MAP_LWMSG_STATUS(lwmsg_assoc_unregister_handle(
                                       pConnection->pAssoc,
                                       pAPIHandle->variant.hIPCHandle,
                                       LWMSG_FALSE));
        BAIL_ON_SMB_ERROR(dwError);
        
        /* Free the handle wrapper */
        SMB_SAFE_FREE_MEMORY(pAPIHandle);
        break;
    case SMB_API_HANDLE_ACCESS:
        dwError = SMBAPIHandleFreeSecurityToken(pAPIHandle);
        BAIL_ON_SMB_ERROR(dwError);
        SMB_SAFE_FREE_MEMORY(pAPIHandle);
        break;
    }

cleanup:

    return dwError;

error:

    goto cleanup;
}
