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
 *        createnp.c
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem (SMB)
 *
 *        CreateNamedPipe API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "includes.h"

NTSTATUS
DriverDispatch(
    IN IO_DEVICE_HANDLE hDevice,
    IN PIRP pIrp
    );

VOID
DriverShutdown(
    IN IO_DRIVER_HANDLE hDriver
    );

NTSTATUS
DriverEntry(
    IN IO_DRIVER_HANDLE hDriver,
    IN ULONG ulInterfaceVersion
    )
{
    NTSTATUS ntStatus = 0;

    if (IO_DRIVER_ENTRY_INTERFACE_VERSION != ulInterfaceVersion)
    {
        ntStatus = STATUS_UNSUCCESSFUL;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = IoDriverInitialize(
                    hDriver,
                    NULL,
                    DriverShutdown,
                    DriverDispatch);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBSrvListenerStart();

error:

    return ntStatus;
}

NTSTATUS
DriverDispatch(
    IN IO_DEVICE_HANDLE hDevice,
    IN PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;

    switch (pIrp->Type)
    {
        case IRP_TYPE_CREATE:

            ntStatus = SrvDeviceCreate(
                            hDevice,
                            pIrp);
            break;

        case IRP_TYPE_CLOSE:

            ntStatus = SrvDeviceClose(
                            hDevice,
                            pIrp);

            break;

        case IRP_TYPE_READ:

            ntStatus = SrvDeviceRead(
                            hDevice,
                            pIrp);

            break;

        case IRP_TYPE_WRITE:

            ntStatus = SrvDeviceWrite(
                            hDevice,
                            pIrp);

            break;

        case IRP_TYPE_IO_CONTROL:

            ntStatus = SrvDeviceControlIO(
                            hDevice,
                            pIrp);

            break;

        case IRP_TYPE_FS_CONTROL:

            ntStatus = SrvDeviceControlFS(
                            hDevice,
                            pIrp);

            break;

        case IRP_TYPE_FLUSH:

            ntStatus = SrvDeviceFlush(
                            hDevice,
                            pIrp);

            break;

        case IRP_TYPE_QUERY_INFORMATION:

            ntStatus = SrvDeviceQueryInfo(
                            hDevice,
                            pIrp);

            break;

        case IRP_TYPE_SET_INFORMATION:

            ntStatus = SrvDeviceSetInfo(
                            hDevice,
                            pIrp);

            break;

        default:

            ntStatus = STATUS_UNSUCCESSFUL;
            BAIL_ON_NT_STATUS(ntStatus);
    }

error:

    return ntStatus;
}

VOID
DriverShutdown(
    IN IO_DRIVER_HANDLE hDriver
    )
{
    NTSTATUS ntStatus = SMBSrvListenerStop();

    if (ntStatus)
    {
        SMB_LOG_ERROR("[srv] driver failed to stop. [code: %d]", ntStatus);
    }
}



