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
 *        Likewise I/O (LWIO) - templatedriver
 *
 *        Driver
 *
 * Authors: Evgeny Popovich (epopovich@likewise.com)
 */

#include "includes.h"

static
NTSTATUS
TemplateDriverDispatch(
    IN IO_DEVICE_HANDLE hDevice,
    IN PIRP pIrp
    );

NTSTATUS
IO_DRIVER_ENTRY(srv)(
    IN IO_DRIVER_HANDLE hDriver,
    IN ULONG ulInterfaceVersion
    )
{
    NTSTATUS ntStatus = 0;
    PCSTR    pszName  = "templatedriver";
    PVOID    pDeviceContext = NULL;
    IO_DEVICE_HANDLE hDevice = NULL;

    if (IO_DRIVER_ENTRY_INTERFACE_VERSION != ulInterfaceVersion)
    {
        ntStatus = STATUS_UNSUCCESSFUL;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = IoDriverInitialize(
                    hDriver,
                    NULL,
                    TemplateDriverShutdown,
                    TemplateDriverDispatch);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = IoDeviceCreate(
                    &hDevice,
                    hDriver,
                    pszName,
                    pDeviceContext);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = TemplateDriverInitialize(hDevice);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = IoDriverRegisterRefreshCallback(
                    hDriver,
                    TemplateDriverRefresh);
    BAIL_ON_NT_STATUS(ntStatus);

    hDevice = NULL;

cleanup:

    return ntStatus;

error:

    if (hDevice)
    {
        IoDeviceDelete(&hDevice);
    }

    goto cleanup;
}

static
NTSTATUS
TemplateDriverDispatch(
    IN IO_DEVICE_HANDLE hDevice,
    IN PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;

    LWIO_LOG_TRACE("%s", __func__);

    switch (pIrp->Type)
    {
        case IRP_TYPE_CREATE:

            ntStatus = TemplateDriverProcessCreate(
                            hDevice,
                            pIrp);
            break;

        case IRP_TYPE_CLOSE:

            ntStatus = TemplateDriverProcessClose(
                            hDevice,
                            pIrp);

            break;

        case IRP_TYPE_READ:

            ntStatus = TemplateDriverProcessRead(
                            hDevice,
                            pIrp);

            break;

        case IRP_TYPE_WRITE:

            ntStatus = TemplateDriverProcessWrite(
                            hDevice,
                            pIrp);

            break;

        case IRP_TYPE_DEVICE_IO_CONTROL:

            ntStatus = TemplateDriverProcessDeviceIoControl(
                            hDevice,
                            pIrp);

            break;

        case IRP_TYPE_FS_CONTROL:

            ntStatus = TemplateDriverProcessFsControl(
                            hDevice,
                            pIrp);

            break;

        case IRP_TYPE_FLUSH_BUFFERS:

            ntStatus = TemplateDriverProcessFlushBuffers(
                            hDevice,
                            pIrp);

            break;

        case IRP_TYPE_QUERY_INFORMATION:

            ntStatus = TemplateDriverProcessQueryInformation(
                            hDevice,
                            pIrp);

            break;

        case IRP_TYPE_SET_INFORMATION:

            ntStatus = TemplateDriverProcessSetInformation(
                            hDevice,
                            pIrp);

            break;

        case IRP_TYPE_CREATE_NAMED_PIPE:

            ntStatus = TemplateDriverProcessCreateNamedPipe(
                            hDevice,
                            pIrp);

            break;

        case IRP_TYPE_CREATE_MAILSLOT:

            ntStatus = TemplateDriverProcessCreateMailslot(
                            hDevice,
                            pIrp);

            break;

        case IRP_TYPE_QUERY_DIRECTORY:

            ntStatus = TemplateDriverProcessQueryDirectory(
                            hDevice,
                            pIrp);

            break;

        case IRP_TYPE_READ_DIRECTORY_CHANGE:

            ntStatus = TemplateDriverProcessReadDirectoryChange(
                            hDevice,
                            pIrp);

            break;

        case IRP_TYPE_QUERY_VOLUME_INFORMATION:

            ntStatus = TemplateDriverProcessQueryVolumeInformation(
                            hDevice,
                            pIrp);

            break;

        case IRP_TYPE_SET_VOLUME_INFORMATION:

            ntStatus = TemplateDriverProcessSetVolumeInformation(
                            hDevice,
                            pIrp);

            break;

        case IRP_TYPE_LOCK_CONTROL:

            ntStatus = TemplateDriverProcessLockControl(
                            hDevice,
                            pIrp);

            break;

        case IRP_TYPE_QUERY_SECURITY:

            ntStatus = TemplateDriverProcessQuerySecurity(
                            hDevice,
                            pIrp);

            break;

        case IRP_TYPE_SET_SECURITY:

            ntStatus = TemplateDriverProcessSetSecurity(
                            hDevice,
                            pIrp);

            break;

        default:

            ntStatus = STATUS_UNSUCCESSFUL;
            BAIL_ON_NT_STATUS(ntStatus);
    }

cleanup:

    if (ntStatus != STATUS_PENDING)
    {
        pIrp->IoStatusBlock.Status = ntStatus;
    }

    return ntStatus;

error:

    goto cleanup;
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/

