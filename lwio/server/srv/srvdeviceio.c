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

NTSTATUS
SrvDeviceControlIo(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP             pIrp
    )
{
    NTSTATUS ntStatus = 0;
    PSRV_IRP_CONTEXT pIrpContext = NULL;

    ntStatus = SrvAllocateIrpContext(
                        pIrp,
                        &pIrpContext
                        );
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvDeviceIoCommon(pIrpContext, pIrp);
    BAIL_ON_NT_STATUS(ntStatus);

error:

    return ntStatus;
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

    ControlCode      = pIrp->Args.IoFsControl.ControlCode;
    pInBuffer        = pIrp->Args.IoFsControl.InputBuffer;
    ulInBufferSize   = pIrp->Args.IoFsControl.InputBufferLength;
    pOutBuffer       = pIrp->Args.IoFsControl.OutputBuffer;
    ulOutBufferSize  = pIrp->Args.IoFsControl.OutputBufferLength;

    switch (ControlCode)
    {

      case SRV_DEVCTL_ADD_SHARE:

          ntStatus = SrvDevCtlAddShare(
                        pInBuffer,
                        ulInBufferSize,
                        pOutBuffer,
                        ulOutBufferSize
                        );
          break;

      case SRV_DEVCTL_DELETE_SHARE:

          ntStatus = SrvDevCtlDeleteShare(
                        pInBuffer,
                        ulInBufferSize,
                        pOutBuffer,
                        ulOutBufferSize
                        );
          break;

      case SRV_DEVCTL_ENUM_SHARE:

          ntStatus = SrvDevCtlEnumShares(
                        pInBuffer,
                        ulInBufferSize,
                        pOutBuffer,
                        ulOutBufferSize,
                        &ulBytesTransferred
                        );
          break;

      case SRV_DEVCTL_SET_SHARE_INFO:

          ntStatus = SrvDevCtlSetShareInfo(
                        pInBuffer,
                        ulInBufferSize,
                        pOutBuffer,
                        ulOutBufferSize
                        );
          break;

      case SRV_DEVCTL_GET_SHARE_INFO:

          ntStatus = SrvDevCtlGetShareInfo(
                        pInBuffer,
                        ulInBufferSize,
                        pOutBuffer,
                        ulOutBufferSize
                        );
          break;

      default:

          ntStatus = STATUS_INVALID_PARAMETER;

          break;
    }

    pIrp->IoStatusBlock.BytesTransferred = ulBytesTransferred;
    pIrp->IoStatusBlock.Status = ntStatus;

    return ntStatus;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
