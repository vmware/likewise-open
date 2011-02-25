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
 *        dispatch.h
 *
 * Abstract:
 *
 *        Likewise I/O (LWIO) - templatedriver
 *
 *        Dispatch header
 *
 * Authors: Evgeny Popovich (epopovich@likewise.com)
 */

#ifndef __DISPATCH_H__
#define __DISPATCH_H__

NTSTATUS
TemplateDriverProcessCreate(
    IO_DEVICE_HANDLE hDevice,
    PIRP pIrp
    );

NTSTATUS
TemplateDriverProcessClose(
    IO_DEVICE_HANDLE hDevice,
    PIRP pIrp
    );

NTSTATUS
TemplateDriverProcessRead(
    IO_DEVICE_HANDLE hDevice,
    PIRP pIrp
    );

NTSTATUS
TemplateDriverProcessWrite(
    IO_DEVICE_HANDLE hDevice,
    PIRP pIrp
    );

NTSTATUS
TemplateDriverProcessDeviceIoControl(
    IO_DEVICE_HANDLE hDevice,
    PIRP pIrp
    );

NTSTATUS
TemplateDriverProcessFsControl(
    IO_DEVICE_HANDLE hDevice,
    PIRP pIrp
    );

NTSTATUS
TemplateDriverProcessFlushBuffers(
    IO_DEVICE_HANDLE hDevice,
    PIRP pIrp
    );

NTSTATUS
TemplateDriverProcessQueryInformation(
    IO_DEVICE_HANDLE hDevice,
    PIRP pIrp
    );

NTSTATUS
TemplateDriverProcessSetInformation(
    IO_DEVICE_HANDLE hDevice,
    PIRP pIrp
    );

NTSTATUS
TemplateDriverProcessCreateNamedPipe(
    IO_DEVICE_HANDLE hDevice,
    PIRP pIrp
    );

NTSTATUS
TemplateDriverProcessCreateMailslot(
    IO_DEVICE_HANDLE hDevice,
    PIRP pIrp
    );

NTSTATUS
TemplateDriverProcessQueryDirectory(
    IO_DEVICE_HANDLE hDevice,
    PIRP pIrp
    );

NTSTATUS
TemplateDriverProcessReadDirectoryChange(
    IO_DEVICE_HANDLE hDevice,
    PIRP pIrp
    );

NTSTATUS
TemplateDriverProcessQueryVolumeInformation(
    IO_DEVICE_HANDLE hDevice,
    PIRP pIrp
    );

NTSTATUS
TemplateDriverProcessSetVolumeInformation(
    IO_DEVICE_HANDLE hDevice,
    PIRP pIrp
    );

NTSTATUS
TemplateDriverProcessLockControl(
    IO_DEVICE_HANDLE hDevice,
    PIRP pIrp
    );

NTSTATUS
TemplateDriverProcessQuerySecurity(
    IO_DEVICE_HANDLE hDevice,
    PIRP pIrp
    );

NTSTATUS
TemplateDriverProcessSetSecurity(
    IO_DEVICE_HANDLE hDevice,
    PIRP pIrp
    );

#endif  // __DISPATCH_H__

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/

