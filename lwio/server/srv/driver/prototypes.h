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
 *        prototypes.h
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Driver
 *
 *        prototypes
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

// ccb.c

NTSTATUS
SrvCCBCreate(
    PSRV_IRP_CONTEXT pIrpContext,
    PSRV_CCB * ppCCB
    );

NTSTATUS
SrvCCBGet(
    IO_FILE_HANDLE FileHandle,
    PSRV_CCB*      ppCCB
    );

NTSTATUS
SrvCCBSet(
    IO_FILE_HANDLE FileHandle,
    PSRV_CCB       pCCB
    );

VOID
SrvCCBRelease(
    PSRV_CCB pCCB
    );

// config.c

NTSTATUS
SrvReadConfig(
    PCSTR            pszConfigFilePath,
    PLWIO_SRV_CONFIG pConfig
    );

NTSTATUS
SrvInitConfig(
    PLWIO_SRV_CONFIG pConfig
    );

VOID
SrvFreeConfigContents(
    PLWIO_SRV_CONFIG pConfig
    );

// devicecreate.c

NTSTATUS
SrvDeviceCreate(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    );

NTSTATUS
SrvAllocateIrpContext(
    PIRP pIrp,
    PSRV_IRP_CONTEXT * ppIrpContext
    );

VOID
SrvFreeIrpContext(
    PSRV_IRP_CONTEXT pIrpContext
    );

// device.c

NTSTATUS
SrvDeviceCreate(
    IO_DEVICE_HANDLE hDevice,
    PIRP      pIrp
    );

NTSTATUS
SrvDeviceClose(
    IO_DEVICE_HANDLE hDevice,
    PIRP pIrp
    );

NTSTATUS
SrvDeviceRead(
    IO_DEVICE_HANDLE hDevice,
    PIRP pIrp
    );

NTSTATUS
SrvDeviceWrite(
    IO_DEVICE_HANDLE hDevice,
    PIRP pIrp
    );

NTSTATUS
SrvDeviceControlIO(
    IO_DEVICE_HANDLE hDevice,
    PIRP pIrp
    );

NTSTATUS
SrvDeviceControlFS(
    IO_DEVICE_HANDLE hDevice,
    PIRP pIrp
    );

NTSTATUS
SrvDeviceFlush(
    IO_DEVICE_HANDLE hDevice,
    PIRP pIrp
    );

NTSTATUS
SrvDeviceQueryInfo(
    IO_DEVICE_HANDLE hDevice,
    PIRP pIrp
    );

NTSTATUS
SrvDeviceSetInfo(
    IO_DEVICE_HANDLE hDevice,
    PIRP pIrp
    );

// deviceio.c

NTSTATUS
SrvDeviceControlIo(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP             pIrp
    );

// srvshares.c

NTSTATUS
SrvShareGetServiceStringId(
    IN  SHARE_SERVICE  service,
    OUT PSTR*          ppszService
    );

NTSTATUS
SrvShareGetServiceId(
    IN  PCSTR          pszService,
    OUT SHARE_SERVICE* pService
    );

NTSTATUS
SrvShareDevCtlAdd(
    IN     PBYTE lpInBuffer,
    IN     ULONG ulInBufferSize,
    IN OUT PBYTE lpOutBuffer,
    IN     ULONG ulOutBufferSize
    );

NTSTATUS
SrvShareDevCtlDelete(
    IN     PBYTE lpInBuffer,
    IN     ULONG ulInBufferSize,
    IN OUT PBYTE lpOutBuffer,
    IN     ULONG ulOutBufferSize
    );

NTSTATUS
SrvShareDevCtlEnum(
    IN     PBYTE  lpInBuffer,
    IN     ULONG  ulInBufferSize,
    IN OUT PBYTE  lpOutBuffer,
    IN     ULONG  ulOutBufferSize,
    IN OUT PULONG pulBytesTransferred
    );

NTSTATUS
SrvShareDevCtlGetInfo(
    IN     PBYTE  lpInBuffer,
    IN     ULONG  ulInBufferSize,
    IN OUT PBYTE  lpOutBuffer,
    IN     ULONG  ulOutBufferSize,
    IN OUT PULONG pulBytesTransferred
    );

NTSTATUS
SrvShareDevCtlSetInfo(
    IN     PBYTE lpInBuffer,
    IN     ULONG ulInBufferSize,
    IN OUT PBYTE lpOutBuffer,
    IN     ULONG ulOutBufferSize
    );

// srvworker.c

NTSTATUS
SrvWorkerInit(
    PLWIO_SRV_WORKER      pWorker
    );

VOID
SrvWorkerIndicateStop(
    PLWIO_SRV_WORKER pWorker
    );

VOID
SrvWorkerFreeContents(
    PLWIO_SRV_WORKER pWorker
    );

