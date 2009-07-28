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
 *        error.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Protocols API - SMBV2
 *
 *        Error
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

NTSTATUS
SrvBuildErrorResponse_SMB_V2(
    PLWIO_SRV_CONNECTION pConnection,
    PSMB2_HEADER         pSmbRequestHeader,
    NTSTATUS             errorStatus,
    PBYTE                pMessage,
    ULONG                ulMessageLength,
    PSMB_PACKET          pSmbResponse
    )
{
    NTSTATUS ntStatus = 0;
    PSMB2_HEADER pSMB2Header = NULL; // Do not free
    PBYTE pOutBufferRef = pSmbResponse->pRawBuffer + pSmbResponse->bufferUsed;
    ULONG ulTotalBytesUsed = 0;
    ULONG ulBytesAvailable = pSmbResponse->bufferLen - pSmbResponse->bufferUsed;
    ULONG ulOffset    = pSmbResponse->bufferUsed - sizeof(NETBIOS_HEADER);
    ULONG ulBytesUsed = 0;
    PBYTE pOutBuffer = pOutBufferRef;

    ntStatus = SMB2MarshalHeader(
                pOutBuffer,
                ulOffset,
                ulBytesAvailable,
                pSmbRequestHeader->command,
                pSmbRequestHeader->usEpoch,
                pSmbRequestHeader->usCredits, /* TODO: Figure out this one */
                pSmbRequestHeader->ulPid,
                pSmbRequestHeader->ullCommandSequence,
                pSmbRequestHeader->ulTid,
                pSmbRequestHeader->ullSessionId,
                errorStatus,
                TRUE,
                pSmbRequestHeader->ulFlags & SMB2_FLAGS_RELATED_OPERATION,
                &pSMB2Header,
                &ulBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    pSMB2Header->error = errorStatus;

    ulTotalBytesUsed += ulBytesUsed;
    pOutBuffer += ulBytesUsed;
    ulOffset += ulBytesUsed;
    ulBytesAvailable -= ulBytesUsed;

    ntStatus = SMB2MarshalError(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    pMessage,
                    ulMessageLength,
                    &ulBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    ulTotalBytesUsed += ulBytesUsed;
    // pOutBuffer += ulBytesUsed;
    // ulOffset += ulBytesUsed;
    // ulBytesAvailable -= ulBytesUsed;

    pSmbResponse->bufferUsed += ulTotalBytesUsed;

cleanup:

    return ntStatus;

error:

    if (ulTotalBytesUsed)
    {
        memset(pOutBufferRef, 0, ulTotalBytesUsed);
    }

    goto cleanup;
}
