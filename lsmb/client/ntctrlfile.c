/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        ntreadfile.c
 *
 * Abstract:
 *
 *        Likewise NT Subsystem (NT)
 *
 *        NTFsControlFile API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "includes.h"


NT_API
NTSTATUS
NTFsControlFile(
    IN HANDLE      hConnection,
    IN HANDLE      hFile,
    IN PVOID       InputBuffer,
    IN ULONG       InputBufferLength,
    OUT PVOID      OutputBuffer,
    IN ULONG       OutBufferBufferLength
    )
{
    DWORD  ntStatus = 0;
    PNT_SERVER_CONNECTION pConnection = (PNT_SERVER_CONNECTION)hConnection;
    PNT_API_HANDLE pAPIHandle = (PNT_API_HANDLE)hFile;
    NT_CONTROL_FILE_REQUEST request = {0};
    LWMsgMessageTag replyType;
    PVOID pResponse = NULL;
    PNT_CONTROL_FILE_RESPONSE pNPResponse = NULL;

    BAIL_ON_INVALID_POINTER(pBuffer);
    BAIL_ON_INVALID_POINTER(pdwBytesFsControl);
    BAIL_IF_NOT_FILE_HANDLE(hFile);

    request.dwBytesToFsControl = dwNumberOfBytesToFsControl;
    request.hFile = pAPIHandle->variant.hIPCHandle;

    ntStatus = MAP_LWMSG_STATUS(lwmsg_assoc_send_transact(
                    pConnection->pAssoc,
                    NT_CONTROL_FILE,
                    &request,
                    &replyType,
                    &pResponse));
    BAIL_ON_NT_STATUS(ntStatus);

    switch (replyType)
    {
        case NT_CONTROL_FILE_SUCCESS:

            break;

        case NT_CONTROL_FILE_FAILED:

            BAIL_ON_INVALID_POINTER(pResponse);

            ntStatus = ((PNT_STATUS_REPLY)pResponse)->ntStatus;

            break;

        default:

            ntStatus = EINVAL;

            break;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    BAIL_ON_INVALID_POINTER(pResponse);

    pNPResponse = (PNT_CONTROL_FILE_RESPONSE)pResponse;

    if (pNPResponse->dwBytesFsControl)
    {
        memcpy(pBuffer, pNPResponse->pBuffer, pNPResponse->dwBytesFsControl);
    }

    *pdwBytesFsControl = pNPResponse->dwBytesFsControl;

cleanup:

    if (pResponse)
    {
        lwmsg_assoc_free_graph(pConnection->pAssoc, replyType, pResponse);
    }

    return ntStatus;

error:

    if (pdwBytesFsControl)
    {
        *pdwBytesFsControl = 0;
    }

    goto cleanup;
}

