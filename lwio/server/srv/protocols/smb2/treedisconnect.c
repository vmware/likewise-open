/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software
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
 *        treedisconnect.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Protocols API - SMBV2
 *
 *        Tree disconnect
 *
 * Authors: Krishna Ganugapati (kganugapati@likewise.com)
 *          Sriram Nambakam (snambakam@likewise.com)
 *
 *
 */
#include "includes.h"

NTSTATUS
SrvProcessTreeDisconnect_SMB_V2(
    IN OUT PSMB2_CONTEXT pContext,
    IN     PSMB2_MESSAGE pSmbRequest,
    IN OUT PSMB_PACKET   pSmbResponse
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLWIO_SRV_CONNECTION pConnection = pContext->pConnection;
    PLWIO_SRV_SESSION_2  pSession = NULL;
    PLWIO_SRV_TREE_2     pTree = NULL;
    PSMB2_TREE_DISCONNECT_REQUEST_HEADER pTreeDisconnectHeader = NULL; // Do not free
    PBYTE pOutBufferRef = pSmbResponse->pRawBuffer + pSmbResponse->bufferUsed;
    PBYTE pOutBuffer       = pOutBufferRef;
    ULONG ulBytesAvailable = pSmbResponse->bufferLen - pSmbResponse->bufferUsed;
    ULONG ulOffset         = pSmbResponse->bufferUsed - sizeof(NETBIOS_HEADER);
    ULONG ulBytesUsed      = 0;
    ULONG ulTotalBytesUsed = 0;

    ntStatus = SrvConnection2FindSession_SMB_V2(
                    pContext,
                    pConnection,
                    pSmbRequest->pHeader->ullSessionId,
                    &pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMB2UnmarshalTreeDisconnectRequest(
                    pSmbRequest,
                    &pTreeDisconnectHeader);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvSession2FindTree_SMB_V2(
                    pContext,
                    pSession,
                    pSmbRequest->pHeader->ulTid,
                    &pTree);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvSession2RemoveTree(
                        pSession,
                        pSmbRequest->pHeader->ulTid);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMB2MarshalHeader(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM2_TREE_DISCONNECT,
                    0,
                    1,
                    pSmbRequest->pHeader->ulPid,
                    pSmbRequest->pHeader->ullCommandSequence,
                    pTree->ulTid,
                    pSession->ullUid,
                    STATUS_SUCCESS,
                    TRUE,
                    pSmbRequest->pHeader->ulFlags & SMB2_FLAGS_RELATED_OPERATION,
                    NULL,
                    &ulBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    ulTotalBytesUsed += ulBytesUsed;
    pOutBuffer += ulBytesUsed;
    ulOffset += ulBytesUsed;
    ulBytesAvailable -= ulBytesUsed;

    ntStatus = SMB2MarshalTreeDisconnectResponse(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    &ulBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    ulTotalBytesUsed += ulBytesUsed;
    // pOutBuffer += ulBytesUsed;
    // ulOffset += ulBytesUsed;
    // ulBytesAvailable -= ulBytesUsed;

    pSmbResponse->bufferUsed += ulTotalBytesUsed;

    if (pContext->pTree)
    {
        SrvTree2Release(pContext->pTree);
        pContext->pTree = NULL;
    }

cleanup:

    if (pSession)
    {
        SrvSession2Release(pSession);
    }

    if (pTree)
    {
        SrvTree2Release(pTree);
    }

    return ntStatus;

error:

    if (ulTotalBytesUsed)
    {
        memset(pOutBufferRef, 0, ulTotalBytesUsed);
    }

    goto cleanup;
}
