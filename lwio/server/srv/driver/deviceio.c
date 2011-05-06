/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

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
 *        deviceio.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (Srv)
 *
 *       DeviceIo Dispatch Routine
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "includes.h"

static
NTSTATUS
SrvDeviceIoCommon(
    PSRV_IRP_CONTEXT pIrpContext,
    PIRP             pIrp
    );

static
NTSTATUS
SrvDevIoControlQueryTransport(
    IN PBYTE pInputBuffer,
    IN ULONG InputBufferSize,
    OUT PBYTE pOutputBuffer,
    IN ULONG OutputBufferSize,
    OUT PULONG pBytesTransferred
    );

static
NTSTATUS
SrvDevIoControlStartTransport(
    IN PBYTE pInputBuffer,
    IN ULONG InputBufferSize,
    OUT PBYTE pOutputBuffer,
    IN ULONG OutputBufferSize,
    OUT PULONG pBytesTransferred
    );

static
NTSTATUS
SrvDevIoControlStopTransport(
    IN PBYTE pInputBuffer,
    IN ULONG InputBufferSize,
    OUT PBYTE pOutputBuffer,
    IN ULONG OutputBufferSize,
    OUT PULONG pBytesTransferred
    );

NTSTATUS
SrvDeviceControlIo(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP             pIrp
    )
{
    NTSTATUS ntStatus = 0;
    PSRV_IRP_CONTEXT pIrpContext = NULL;

    ntStatus = SrvAllocateIrpContext(pIrp, &pIrpContext);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvDeviceIoCommon(pIrpContext, pIrp);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    if (pIrpContext)
    {
        SrvFreeIrpContext(pIrpContext);
    }

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
SrvDeviceIoCommon(
    PSRV_IRP_CONTEXT pIrpContext,
    PIRP             pIrp
    )
{
    NTSTATUS ntStatus = 0;
    PBYTE pInBuffer = NULL;
    ULONG ulInBufferSize = 0;
    PBYTE pOutBuffer = NULL;
    ULONG ulOutBufferSize = 0;
    ULONG ulBytesTransferred = 0;
    ULONG ControlCode = 0;
    PSRV_CCB pCcb = IoFileGetContext(pIrp->FileHandle);

    ControlCode      = pIrp->Args.IoFsControl.ControlCode;
    pInBuffer        = pIrp->Args.IoFsControl.InputBuffer;
    ulInBufferSize   = pIrp->Args.IoFsControl.InputBufferLength;
    pOutBuffer       = pIrp->Args.IoFsControl.OutputBuffer;
    ulOutBufferSize  = pIrp->Args.IoFsControl.OutputBufferLength;

    if (pCcb->UnixUid != 0)
    {
        ntStatus = STATUS_ACCESS_DENIED;
    }
    else switch (ControlCode)
    {

      case SRV_DEVCTL_ADD_SHARE:

          ntStatus = SrvShareDevCtlAdd(
                        pInBuffer,
                        ulInBufferSize,
                        pOutBuffer,
                        ulOutBufferSize
                        );
          break;

      case SRV_DEVCTL_DELETE_SHARE:

          ntStatus = SrvShareDevCtlDelete(
                        pInBuffer,
                        ulInBufferSize,
                        pOutBuffer,
                        ulOutBufferSize
                        );
          break;

      case SRV_DEVCTL_ENUM_SHARE:

          ntStatus = SrvShareDevCtlEnum(
                        pInBuffer,
                        ulInBufferSize,
                        pOutBuffer,
                        ulOutBufferSize,
                        &ulBytesTransferred
                        );
          break;

      case SRV_DEVCTL_SET_SHARE_INFO:

          ntStatus = SrvShareDevCtlSetInfo(
                        pInBuffer,
                        ulInBufferSize,
                        pOutBuffer,
                        ulOutBufferSize
                        );
          break;

      case SRV_DEVCTL_GET_SHARE_INFO:

          ntStatus = SrvShareDevCtlGetInfo(
                        pInBuffer,
                        ulInBufferSize,
                        pOutBuffer,
                        ulOutBufferSize,
                        &ulBytesTransferred
                        );
          break;

      case SRV_DEVCTL_ENUM_SESSIONS:

          ntStatus = SrvDevCtlEnumerateSessions(
                          pInBuffer,
                          ulInBufferSize,
                          pOutBuffer,
                          ulOutBufferSize,
                          &ulBytesTransferred);

          break;

      case SRV_DEVCTL_DELETE_SESSION:

          ntStatus = SrvDevCtlDeleteSession(
                          pInBuffer,
                          ulInBufferSize,
                          pOutBuffer,
                          ulOutBufferSize,
                          &ulBytesTransferred);

          break;

      case SRV_DEVCTL_ENUM_FILES:

          ntStatus = SrvDevCtlEnumerateFiles(
                          pInBuffer,
                          ulInBufferSize,
                          pOutBuffer,
                          ulOutBufferSize,
                          &ulBytesTransferred);

          break;

      case SRV_DEVCTL_GET_FILE_INFO:

          ntStatus = SrvDevCtlGetFileInfo(
                          pInBuffer,
                          ulInBufferSize,
                          pOutBuffer,
                          ulOutBufferSize,
                          &ulBytesTransferred);

          break;

      case SRV_DEVCTL_CLOSE_FILE:

          ntStatus = SrvDevCtlCloseFile(
                          pInBuffer,
                          ulInBufferSize,
                          pOutBuffer,
                          ulOutBufferSize,
                          &ulBytesTransferred);

          break;

      case SRV_DEVCTL_ENUM_CONNECTION:

          ntStatus = SrvDevCtlEnumerateConnections(
                          pInBuffer,
                          ulInBufferSize,
                          pOutBuffer,
                          ulOutBufferSize,
                          &ulBytesTransferred);
          break;

      case SRV_DEVCTL_QUERY_TRANSPORT:

          ntStatus = SrvDevIoControlQueryTransport(
                          pInBuffer,
                          ulInBufferSize,
                          pOutBuffer,
                          ulOutBufferSize,
                          &ulBytesTransferred);
          break;

      case SRV_DEVCTL_START_TRANSPORT:

          ntStatus = SrvDevIoControlStartTransport(
                          pInBuffer,
                          ulInBufferSize,
                          pOutBuffer,
                          ulOutBufferSize,
                          &ulBytesTransferred);
          break;

      case SRV_DEVCTL_STOP_TRANSPORT:

          ntStatus = SrvDevIoControlStopTransport(
                          pInBuffer,
                          ulInBufferSize,
                          pOutBuffer,
                          ulOutBufferSize,
                          &ulBytesTransferred);
          break;

      case SRV_DEVCTL_RELOAD_SHARES:

          ntStatus = SrvShareDevCtlReloadConfiguration(
                          pInBuffer,
                          ulInBufferSize,
                          pOutBuffer,
                          ulOutBufferSize,
                          &ulBytesTransferred
                          );
          break;

      case IO_DEVICE_CTL_STATISTICS:

          ntStatus = SrvProcessStatistics(
                          pInBuffer,
                          ulInBufferSize,
                          pOutBuffer,
                          ulOutBufferSize,
                          &ulBytesTransferred);

          break;

      default:

          ntStatus = STATUS_INVALID_PARAMETER;

          break;
    }

    pIrp->IoStatusBlock.BytesTransferred = ulBytesTransferred;
    pIrp->IoStatusBlock.Status = ntStatus;

    return ntStatus;
}

static
NTSTATUS
SrvDevIoControlQueryTransport(
    IN PBYTE pInputBuffer,
    IN ULONG InputBufferSize,
    OUT PBYTE pOutputBuffer,
    IN ULONG OutputBufferSize,
    OUT PULONG pBytesTransferred
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN isStarted = FALSE;
    ULONG bytesTransferred = 0;

    // The input is NULL.  The output is a BOOLEAN indicating whether the
    // transport is started.

    //
    // Validate buffer sizes
    //

    // Input: NULL
    if (pInputBuffer || InputBufferSize)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    // Output: BOOLEAN
    if (!pOutputBuffer || (OutputBufferSize != sizeof(BOOLEAN)))
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    //
    // Call underlying functionality
    //

    isStarted = SrvProtocolIsStarted();

    *(PBOOLEAN)pOutputBuffer = isStarted;
    bytesTransferred = sizeof(isStarted);

    ntStatus = STATUS_SUCCESS;

cleanup:
    *pBytesTransferred = bytesTransferred;

    return ntStatus;

error:
    bytesTransferred = 0;

    goto cleanup;
}

static
NTSTATUS
SrvDevIoControlStartTransport(
    IN PBYTE pInputBuffer,
    IN ULONG InputBufferSize,
    OUT PBYTE pOutputBuffer,
    IN ULONG OutputBufferSize,
    OUT PULONG pBytesTransferred
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    // The input is NULL.  The output is NULL.

    //
    // Validate buffer sizes
    //

    // Input: NULL
    if (pInputBuffer || InputBufferSize)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    // Output: NULL
    if (pOutputBuffer || OutputBufferSize)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    //
    // Call underlying functionality
    //

    ntStatus = SrvProtocolStart();
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:
    *pBytesTransferred = 0;

    return ntStatus;

error:
    goto cleanup;
}

static
NTSTATUS
SrvDevIoControlStopTransport(
    IN PBYTE pInputBuffer,
    IN ULONG InputBufferSize,
    OUT PBYTE pOutputBuffer,
    IN ULONG OutputBufferSize,
    OUT PULONG pBytesTransferred
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN isForce = FALSE;
    BOOLEAN isStopped = FALSE;

    // The input is a BOOLEAN.  The output is NULL.

    //
    // Validate buffer sizes
    //

    // Input: BOOLEAN
    if (!pInputBuffer || (InputBufferSize != sizeof(BOOLEAN)))
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    // Output: NULL
    if (pOutputBuffer || OutputBufferSize)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    //
    // Capture parameters
    //

    isForce = *(PBOOLEAN)pInputBuffer ? TRUE : FALSE;

    //
    // Call underlying functionality
    //

    isStopped = SrvProtocolStop(isForce);
    if (!isStopped)
    {
        ntStatus = STATUS_CONNECTION_ACTIVE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

cleanup:
    *pBytesTransferred = 0;

    return ntStatus;

error:
    goto cleanup;
}
