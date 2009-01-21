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

#include "iop.h"

VOID
IopRootFree(
    IN OUT PIOP_ROOT_STATE* ppRoot
    )
{
    PIOP_ROOT_STATE pRoot = *ppRoot;

    if (pRoot)
    {
        // Unload drivers in reverse load order
        while (!LwListIsEmpty(&pRoot->DriverObjectList))
        {
            PLW_LIST_LINKS pLinks = LwListRemoveTail(&pRoot->DriverObjectList);
            PIO_DRIVER_OBJECT pDriverObject = LW_STRUCT_FROM_FIELD(pLinks, IO_DRIVER_OBJECT, RootLinks);

            IopDriverUnload(&pDriverObject);
        }

        IopConfigFreeConfig(&pRoot->Config);
        IoMemoryFree(pRoot);
        *ppRoot = NULL;
    }
}

NTSTATUS
IopRootCreate(
    OUT PIOP_ROOT_STATE* ppRoot,
    IN PCSTR pszConfigFilePath
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    PIOP_ROOT_STATE pRoot = NULL;

    status = IO_ALLOCATE(&pRoot, IOP_ROOT_STATE, sizeof(*pRoot));
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    LwListInit(&pRoot->DriverObjectList);
    LwListInit(&pRoot->DeviceObjectList);

    status = IopConfigParse(&pRoot->Config, pszConfigFilePath);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

cleanup:
    if (status)
    {
        IopRootFree(&pRoot);
    }

    *ppRoot = pRoot;

    IO_LOG_LEAVE_ON_STATUS_EE(status, EE);
    return status;
}

NTSTATUS
IopRootLoadDrivers(
    IN PIOP_ROOT_STATE pRoot
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    PLW_LIST_LINKS pLinks = NULL;
    PIO_DRIVER_OBJECT pDriverObject = NULL;

    for (pLinks = pRoot->Config->DriverConfigList.Next;
         pLinks != &pRoot->Config->DriverConfigList;
         pLinks = pLinks->Next)
    {
        PIOP_DRIVER_CONFIG pDriverConfig = LW_STRUCT_FROM_FIELD(pLinks, IOP_DRIVER_CONFIG, Links);

        status = IopDriverLoad(&pDriverObject, pRoot, pDriverConfig);
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }

cleanup:
    IO_LOG_LEAVE_ON_STATUS_EE(status, EE);
    return status;
}

PIO_DEVICE_OBJECT
IopRootFindDevice(
    IN PIOP_ROOT_STATE pRoot,
    IN PUNICODE_STRING pDeviceName
    )
{
    PLW_LIST_LINKS pLinks = NULL;
    PIO_DEVICE_OBJECT pFoundDevice = NULL;

    for (pLinks = pRoot->DeviceObjectList.Next;
         pLinks != &pRoot->DeviceObjectList;
         pLinks = pLinks->Next)
    {
        PIO_DEVICE_OBJECT pDevice = LW_STRUCT_FROM_FIELD(pLinks, IO_DEVICE_OBJECT, RootLinks);
        if (RtlUnicodeStringIsEqual(pDeviceName, &pDevice->DeviceName, FALSE))
        {
            pFoundDevice = pDevice;
            break;
        }
    }

    return pFoundDevice;
}

VOID
IopRootInsertDriver(
    IN PIOP_ROOT_STATE pRoot,
    IN PLW_LIST_LINKS pDriverRootLinks
    )
{
    LwListInsertTail(&pRoot->DriverObjectList,
                     pDriverRootLinks);
    pRoot->DriverCount++;
}

VOID
IopRootRemoveDriver(
    IN PIOP_ROOT_STATE pRoot,
    IN PLW_LIST_LINKS pDriverRootLinks
    )
{
    LwListRemove(pDriverRootLinks);
    pRoot->DriverCount--;
}



VOID
IopRootInsertDevice(
    IN PIOP_ROOT_STATE pRoot,
    IN PLW_LIST_LINKS pDeviceRootLinks
    )
{
    LwListInsertTail(&pRoot->DeviceObjectList,
                     pDeviceRootLinks);
    pRoot->DeviceCount++;
}

VOID
IopRootRemoveDevice(
    IN PIOP_ROOT_STATE pRoot,
    IN PLW_LIST_LINKS pDeviceRootLinks
    )
{
    LwListRemove(pDeviceRootLinks);
    pRoot->DeviceCount--;
}

NTSTATUS
IopRootParse(
    IN PIOP_ROOT_STATE pRoot,
    IN OUT PIO_FILE_NAME pFileName,
    OUT PIO_DEVICE_OBJECT* ppDevice
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    PWSTR pszCurrent = NULL;
    UNICODE_STRING deviceName = { 0 };
    PIO_DEVICE_OBJECT pDevice = NULL;

    if (pFileName->RootFileHandle)
    {
        // Relative path -- not yet supported.
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_EE(EE);
    }

    // Absolute path.

    if (!pFileName->FileName)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_EE(EE);
    }

    if (!IoRtlPathIsSeparator(pFileName->FileName[0]))
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_EE(EE);
    }

    pszCurrent = pFileName->FileName + 1;
    while (pszCurrent[0] && !IoRtlPathIsSeparator(pszCurrent[0]))
    {
        pszCurrent++;
    }

    deviceName.Buffer = (PWSTR) pFileName->FileName + 1;
    deviceName.Length = (pszCurrent - deviceName.Buffer) * sizeof(deviceName.Buffer[0]);
    deviceName.MaximumLength = deviceName.Length;

    pDevice = IopRootFindDevice(pRoot, &deviceName);
    if (!pDevice)
    {
        status = STATUS_OBJECT_PATH_NOT_FOUND;
        GOTO_CLEANUP_EE(EE);
    }

    pFileName->FileName = pszCurrent;

cleanup:
    *ppDevice = pDevice;

    IO_LOG_LEAVE_ON_STATUS_EE(status, EE);
    return status;
}
