/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 *        driver.c
 *
 * Abstract:
 *
 *        Likewise Named Pipe File System Driver (PVFS)
 *
 *        Driver Entry Function
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Danilo Almeida (dalmeida@likewisesoftware.com)
 */

#include "npfs.h"

VOID
NpfsDriverShutdown(
    IN IO_DRIVER_HANDLE DriverHandle
    )
{
    IO_LOG_ENTER_LEAVE("");
}

NTSTATUS
NpfsDriverDispatch(
    IN IO_DEVICE_HANDLE DeviceHandle,
    IN PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;
    int EE = 0;

    switch (pIrp->Type)
    {

        case IRP_TYPE_CREATE:
            ntStatus = NpfsCreate(
                            DeviceHandle,
                            pIrp
                            );
            break;

        case IRP_TYPE_CREATE_NAMED_PIPE:
            ntStatus = NpfsCreateNamedPipe(
                            DeviceHandle,
                            pIrp
                            );
            break;

        case IRP_TYPE_CLOSE:
            ntStatus = NpfsClose(
                            DeviceHandle,
                            pIrp
                            );
            break;


        case IRP_TYPE_READ:
            ntStatus = NpfsRead(
                            DeviceHandle,
                            pIrp
                            );
             break;

        case IRP_TYPE_WRITE:
            ntStatus = NpfsWrite(
                            DeviceHandle,
                            pIrp
                            );
            break;

        case IRP_TYPE_DEVICE_IO_CONTROL:
            ntStatus = NpfsDeviceIo(
                            DeviceHandle,
                            pIrp);
            break;

        case IRP_TYPE_FS_CONTROL:
            ntStatus = NpfsFsCtl(
                            DeviceHandle,
                            pIrp
                            );
            break;
        case IRP_TYPE_FLUSH_BUFFERS:
            ntStatus = STATUS_NOT_IMPLEMENTED;
            break;
        case IRP_TYPE_QUERY_INFORMATION:
            ntStatus = NpfsQueryInformation(
                            DeviceHandle,
                            pIrp
                            );
            break;
        case IRP_TYPE_SET_INFORMATION:
            ntStatus = NpfsSetInformation(
                            DeviceHandle,
                            pIrp
                            );
            break;
    default:
        ntStatus = STATUS_UNSUCCESSFUL;
        pIrp->IoStatusBlock.Status = ntStatus;
        GOTO_CLEANUP_ON_STATUS_EE(ntStatus, EE);
    }

cleanup:
    IO_LOG_ENTER_LEAVE_STATUS_EE_EX(ntStatus, EE, "Type = %u", pIrp->Type);
    return ntStatus;
}

NTSTATUS
DriverEntry(
    IN IO_DRIVER_HANDLE DriverHandle,
    IN ULONG InterfaceVersion
    )
{
    NTSTATUS ntStatus = 0;
    int EE = 0;
    IO_DEVICE_HANDLE deviceHandle = NULL;

    if (IO_DRIVER_ENTRY_INTERFACE_VERSION != InterfaceVersion)
    {
        ntStatus = STATUS_UNSUCCESSFUL;
        GOTO_CLEANUP_ON_STATUS_EE(ntStatus, EE);
    }

    pthread_rwlock_init(&gServerLock, NULL);

    ntStatus = IoDriverInitialize(DriverHandle,
                                  NULL,
                                  NpfsDriverShutdown,
                                  NpfsDriverDispatch);
    GOTO_CLEANUP_ON_STATUS_EE(ntStatus, EE);

    ntStatus = IoDeviceCreate(&deviceHandle,
                              DriverHandle,
                              "npfs",
                              NULL);
    GOTO_CLEANUP_ON_STATUS_EE(ntStatus, EE);

cleanup:
    IO_LOG_ENTER_LEAVE_STATUS_EE(ntStatus, EE);
    return ntStatus;
}
