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

#include "config.h"
#include "lwiosys.h"

#include "iodriver.h"
#include "ioinit.h"
#include <lw/rtlstring.h>
#include <lw/rtlgoto.h>

#include "lwlist.h"
#include "lwioutils.h"
#include "ntlogmacros.h"

#include "lwthreads.h"

#define NT_PENDING_OR_SUCCESS_OR_NOT(status) \
    (LW_NT_SUCCESS_OR_NOT(status) || (STATUS_PENDING == (status)))

typedef struct _IOP_DRIVER_CONFIG {
    PSTR pszName;
    PSTR pszPath;
    LW_LIST_LINKS Links;
} IOP_DRIVER_CONFIG, *PIOP_DRIVER_CONFIG;

typedef struct _IOP_CONFIG {
    LW_LIST_LINKS DriverConfigList;
    ULONG DriverCount;
} IOP_CONFIG, *PIOP_CONFIG;

typedef struct _IOP_CONFIG_PARSE_STATE {
    PIOP_CONFIG pConfig;
    PIOP_DRIVER_CONFIG pDriverConfig;
    NTSTATUS Status;
} IOP_CONFIG_PARSE_STATE, *PIOP_CONFIG_PARSE_STATE;

typedef struct _IOP_ROOT_STATE {
    PIOP_CONFIG Config;
    // Diagnostics Only
    ULONG DriverCount;
    LW_LIST_LINKS DriverObjectList;
    // Diagnostics Only
    ULONG DeviceCount;
    // Should really be a hash table...
    LW_LIST_LINKS DeviceObjectList;
} IOP_ROOT_STATE, *PIOP_ROOT_STATE;

typedef ULONG IO_DRIVER_OBJECT_FLAGS;

#define IO_DRIVER_OBJECT_FLAG_INITIALIZED 0x00000001
#define IO_DRIVER_OBJECT_FLAG_READY       0x00000002

struct _IO_DRIVER_OBJECT {
    LONG ReferenceCount;
    IO_DRIVER_OBJECT_FLAGS Flags;
    PIOP_ROOT_STATE Root;
    PIOP_DRIVER_CONFIG Config;

    PVOID LibraryHandle;
    PIO_DRIVER_ENTRY DriverEntry;
    struct {
        PIO_DRIVER_SHUTDOWN_CALLBACK Shutdown;
        PIO_DRIVER_DISPATCH_CALLBACK Dispatch;
    } Callback;
    PVOID Context;

    // Devices
    LW_LIST_LINKS DeviceList;
    ULONG DeviceCount;

    // For each list to which this object belongs.
    LW_LIST_LINKS RootLinks;

};

struct _IO_DEVICE_OBJECT {
    LONG ReferenceCount;
    UNICODE_STRING DeviceName;
    PIO_DRIVER_OBJECT Driver;
    PVOID Context;

    // File objects for this device.
    LW_LIST_LINKS FileObjectsList;

    // For each list to which this object belongs.
    LW_LIST_LINKS DriverLinks;
    LW_LIST_LINKS RootLinks;

    LW_RTL_MUTEX CancelMutex;
};

struct _IO_FILE_OBJECT {
    LONG ReferenceCount;
    PIO_DEVICE_OBJECT pDevice;
    PVOID pContext;

    // TODO -- Track file vs named pipe

    // IRPs for this file object.
    LW_LIST_LINKS IrpList;

    // For each list to which this object belongs.
    LW_LIST_LINKS DeviceLinks;
};

// ioinit.c

NTSTATUS
IopParse(
    IN OUT PIO_FILE_NAME pFileName,
    OUT PIO_DEVICE_OBJECT* ppDevice
    );

// ioconfig.c

VOID
IopConfigFreeConfig(
    IN OUT PIOP_CONFIG* ppConfig
    );

NTSTATUS
IopConfigParse(
    OUT PIOP_CONFIG* ppConfig,
    IN PCSTR pszConfigFilePath
    );

// ioroot.c

VOID
IopRootFree(
    IN OUT PIOP_ROOT_STATE* ppRoot
    );

NTSTATUS
IopRootCreate(
    OUT PIOP_ROOT_STATE* ppRoot,
    IN PCSTR pszConfigFilePath
    );

NTSTATUS
IopRootLoadDrivers(
    IN PIOP_ROOT_STATE pRoot
    );

PIO_DEVICE_OBJECT
IopRootFindDevice(
    IN PIOP_ROOT_STATE pRoot,
    IN PUNICODE_STRING pDeviceName
    );

VOID
IopRootInsertDriver(
    IN PIOP_ROOT_STATE pRoot,
    IN PLW_LIST_LINKS pDriverRootLinks
    );

VOID
IopRootRemoveDriver(
    IN PIOP_ROOT_STATE pRoot,
    IN PLW_LIST_LINKS pDriverRootLinks
    );

VOID
IopRootInsertDevice(
    IN PIOP_ROOT_STATE pRoot,
    IN PLW_LIST_LINKS pDeviceRootLinks
    );

VOID
IopRootRemoveDevice(
    IN PIOP_ROOT_STATE pRoot,
    IN PLW_LIST_LINKS pDeviceRootLinks
    );

NTSTATUS
IopRootParse(
    IN PIOP_ROOT_STATE pRoot,
    IN OUT PIO_FILE_NAME pFileName,
    OUT PIO_DEVICE_OBJECT* ppDevice
    );

// iodriver.c

VOID
IopDriverUnload(
    IN OUT PIO_DRIVER_OBJECT* ppDriverObject
    );

NTSTATUS
IopDriverLoad(
    OUT PIO_DRIVER_OBJECT* ppDriverObject,
    IN PIOP_ROOT_STATE pRoot,
    IN PIOP_DRIVER_CONFIG pDriverConfig
    );

VOID
IopDriverInsertDevice(
    IN PIO_DRIVER_OBJECT pDriver,
    IN PLW_LIST_LINKS pDeviceDriverLinks
    );

VOID
IopDriverRemoveDevice(
    IN PIO_DRIVER_OBJECT pDriver,
    IN PLW_LIST_LINKS pDeviceDriverLinks
    );

// iodevice.c

NTSTATUS
IopDeviceCallDriver(
    IN IO_DEVICE_HANDLE DeviceHandle,
    IN OUT PIRP pIrp
    );

// ioirp.c

NTSTATUS
IopIrpCreate(
    OUT PIRP* ppIrp,
    IN IRP_TYPE Type,
    IN PIO_FILE_OBJECT pFileObject
    );

VOID
IopIrpReference(
    IN PIRP Irp
    );

VOID
IopIrpDereference(
    IN OUT PIRP* Irp
    );

PIRP
IopIrpGetIrpFromAsyncCancelContext(
    IN PIO_ASYNC_CANCEL_CONTEXT Context
    );

BOOLEAN
IopIrpCancel(
    IN PIRP pIrp
    );

NTSTATUS
IopIrpDispatch(
    IN PIRP pIrp,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    IN OPTIONAL PIO_STATUS_BLOCK pIoStatusBlock,
    IN OPTIONAL PIO_FILE_HANDLE pCreateFileHandle
    );

// iofile.c

VOID
IopFileObjectReference(
    IN PIO_FILE_OBJECT pFileObject
    );

VOID
IopFileObjectDereference(
    IN OUT PIO_FILE_OBJECT* ppFileObject
    );

NTSTATUS
IopFileObjectAllocate(
    OUT PIO_FILE_OBJECT* ppFileObject,
    IN PIO_DEVICE_OBJECT pDevice
    );

VOID
IopFileObjectFree(
    IN OUT PIO_FILE_OBJECT* ppFileObject
    );

// iosecurity.c

VOID
IopSecurityReferenceSecurityContext(
    IN PIO_CREATE_SECURITY_CONTEXT SecurityContext
    );
