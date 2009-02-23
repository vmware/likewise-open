/*
 * Copyright (c) Likewise Software.  All rights Reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Module Name:
 *
 *        iodriver.h
 *
 * Abstract:
 *
 *        IO Manager Driver Header File
 *
 * Authors: Danilo Almeida (dalmeida@likewisesoftware.com)
 *
 */

#ifndef __IODRIVER_H__
#define __IODRIVER_H__

#include <lwio/lwio.h>

struct _IO_DRIVER_OBJECT;
typedef struct _IO_DRIVER_OBJECT IO_DRIVER_OBJECT, *PIO_DRIVER_OBJECT;
typedef IO_DRIVER_OBJECT *IO_DRIVER_HANDLE, **PIO_DRIVER_HANDLE;

struct _IO_DEVICE_OBJECT;
typedef struct _IO_DEVICE_OBJECT IO_DEVICE_OBJECT, *PIO_DEVICE_OBJECT;
typedef IO_DEVICE_OBJECT *IO_DEVICE_HANDLE, **PIO_DEVICE_HANDLE;

typedef ULONG IRP_TYPE;

#define IRP_TYPE_UNKNOWN                   0
#define IRP_TYPE_CREATE                    1
#define IRP_TYPE_CLOSE                     2
#define IRP_TYPE_READ                      3
#define IRP_TYPE_WRITE                     4
#define IRP_TYPE_DEVICE_IO_CONTROL         5
#define IRP_TYPE_FS_CONTROL                6
#define IRP_TYPE_FLUSH_BUFFERS             7
#define IRP_TYPE_QUERY_INFORMATION         8
#define IRP_TYPE_SET_INFORMATION           9
#define IRP_TYPE_CREATE_NAMED_PIPE        10
#define IRP_TYPE_CREATE_MAILSLOT          11
// In Windows, DIRECTORY_CONTROL has two subtypes:
// - NOTIFY_CHANGE_DIRECTORY
// - QUERY_DIRECTORY
// Here, however, two separate IRPs are used.
#define IRP_TYPE_QUERY_DIRECTORY          12
#define IRP_TYPE_QUERY_VOLUME_INFORMATION 17
#if 0
#define IRP_TYPE_NOTIFY_CHANGE_DIRECTORY  13
#define IRP_TYPE_LOCK_CONTROL             14
#define IPP_TYPE_QUERY_EA                 15
#define IPP_TYPE_SET_EA                   16
#define IRP_TYPE_SET_VOLUME_INFORMATION   18
#define IRP_TYPE_QUERY_QUOTA              19
#define IRP_TYPE_SET_QUOTA                20
#define IRP_TYPE_QUERY_SECURITY           21
#define IRP_TYPE_SET_SECURITY             22

#define IRP_TYPE_QUERY_FULL_ATTRIBUTES    23
#define IRP_TYPE_RENAME                   24
#define IRP_TYPE_LINK                     25
#define IRP_TYPE_UNLINK                   26
#endif

#if 0
#define IO_LOCK_CONTROL_LOCK 1
#define IO_LOCK_CONTROL_UNLOCK 2
#define IO_LOCK_CONTROL_UNLOCK_ALL 4
#define IO_LOCK_CONTROL_UNLOCK_ALL_BY_KEY 3
#endif

typedef struct _IRP_ARGS_CREATE {
    IN PIO_CREATE_SECURITY_CONTEXT SecurityContext;
    IN IO_FILE_NAME FileName;
    IN ACCESS_MASK DesiredAccess;
    IN OPTIONAL LONG64 AllocationSize;
    IN FILE_ATTRIBUTES FileAttributes;
    IN FILE_SHARE_FLAGS ShareAccess;
    IN FILE_CREATE_DISPOSITION CreateDisposition;
    IN FILE_CREATE_OPTIONS CreateOptions;
    IN OPTIONAL PFILE_FULL_EA_INFORMATION EaBuffer;
    IN ULONG EaLength;
    IN OPTIONAL PVOID SecurityDescriptor; // TBD
    IN OPTIONAL PVOID SecurityQualityOfService; // TBD
    IN OPTIONAL PIO_ECP_LIST EcpList;
} IRP_ARGS_CREATE, *PIRP_ARGS_CREATE;

typedef struct _IRP_ARGS_READ_WRITE {
    // IN for write, OUT for read
    IN OUT PVOID Buffer;
    IN ULONG Length;
    IN OPTIONAL PLONG64 ByteOffset;
    IN OPTIONAL PULONG Key;
} IRP_ARGS_READ_WRITE, *PIRP_ARGS_READ_WRITE;

typedef struct _IRP_ARGS_IO_FS_CONTROL {
    IN ULONG ControlCode;
    IN PVOID InputBuffer;
    IN ULONG InputBufferLength;
    OUT PVOID OutputBuffer;
    IN ULONG OutputBufferLength;
} IRP_ARGS_IO_FS_CONTROL, *PIRP_ARGS_IO_FS_CONTROL;

typedef struct _IRP_ARGS_QUERY_SET_INFORMATION {
    IN OUT PVOID FileInformation;
    IN ULONG Length;
    IN FILE_INFORMATION_CLASS FileInformationClass;
} IRP_ARGS_QUERY_SET_INFORMATION, *PIRP_ARGS_QUERY_SET_INFORMATION;

typedef struct _IRP_ARGS_QUERY_DIRECTORY {
    OUT PVOID FileInformation;
    IN ULONG Length;
    IN FILE_INFORMATION_CLASS FileInformationClass;
    IN BOOLEAN ReturnSingleEntry;
    IN OPTIONAL PIO_FILE_SPEC FileSpec;
    IN BOOLEAN RestartScan;
} IRP_ARGS_QUERY_DIRECTORY, *PIRP_ARGS_QUERY_DIRECTORY;

typedef struct _IRP_ARGS_QUERY_VOLUME {
    OUT PVOID FsInformation;
    IN ULONG Length;
    IN FS_INFORMATION_CLASS FsInformationClass;
} IRP_ARGS_QUERY_VOLUME, *PIRP_ARGS_QUERY_VOLUME;

typedef struct _IRP {
    IN IRP_TYPE Type;
    OUT IO_STATUS_BLOCK IoStatusBlock;
    IN IO_DRIVER_HANDLE DriverHandle;
    IN IO_DEVICE_HANDLE DeviceHandle;
    IN IO_FILE_HANDLE FileHandle;
    union {
        // IRP_TYPE_CREATE
        IRP_ARGS_CREATE Create;
        // IRP_TYPE_READ, IRP_TYPE_WRITE
        IRP_ARGS_READ_WRITE ReadWrite;
        // IRP_TYPE_IO_CONTROL, IRP_TYPE_FS_CONTROL
        IRP_ARGS_IO_FS_CONTROL IoFsControl;
        // IRP_TYPE_QUERY_INFORMATION, IRP_TYPE_SET_INFORMATION
        IRP_ARGS_QUERY_SET_INFORMATION QuerySetInformation;
        // IRP_TYPE_QUERY_DIRECTORY
        IRP_ARGS_QUERY_DIRECTORY QueryDirectory;
        // IRP_TYPE_QUERY_VOLUME
        IRP_ARGS_QUERY_VOLUME QueryVolume;
        // No args for IRP_TYPE_CLOSE, IRP_TYPE_FLUSH
    } Args;
    // Internal data at the end...
} IRP, *PIRP;

typedef NTSTATUS (*PIO_IRP_CALLBACK)(
    IN PVOID CallbackContext,
    IN PIRP Irp
    );

typedef VOID (*PIO_DRIVER_SHUTDOWN_CALLBACK)(
    IN IO_DRIVER_HANDLE DriverHandle
    );

typedef NTSTATUS (*PIO_DRIVER_DISPATCH_CALLBACK)(
    IN IO_DEVICE_HANDLE DeviceHandle,
    IN PIRP Irp
    );

typedef NTSTATUS (*PIO_DRIVER_ENTRY)(
    IN IO_DRIVER_HANDLE DriverHandle,
    IN ULONG InterfaceVersion
    );

#define IO_DRIVER_ENTRY_FUNCTION_NAME "DriverEntry"
#define IO_DRIVER_ENTRY_INTERFACE_VERSION 1

// Driver functions

// Can only be called in DriverEntry.
NTSTATUS
IoDriverInitialize(
    IN OUT IO_DRIVER_HANDLE DriverHandle,
    IN OPTIONAL PVOID DriverContext,
    IN PIO_DRIVER_SHUTDOWN_CALLBACK ShutdownCallback,
    IN PIO_DRIVER_DISPATCH_CALLBACK DispatchCallback
    );

PCSTR
IoDriverGetName(
    IN IO_DRIVER_HANDLE DriverHandle
    );

PVOID
IoDriverGetContext(
    IN IO_DRIVER_HANDLE DriverHandle
    );


// Device functions

NTSTATUS
IoDeviceCreate(
    OUT PIO_DEVICE_HANDLE pDeviceHandle,
    IN IO_DRIVER_HANDLE DriverHandle,
    IN PCSTR pszName,
    IN OPTIONAL PVOID DeviceContext
    );

VOID
IoDeviceDelete(
    IN OUT PIO_DEVICE_HANDLE pDeviceHandle
    );

PVOID
IoDeviceGetContext(
    IN IO_DEVICE_HANDLE DeviceHandle
    );

// File functions

NTSTATUS
IoFileSetContext(
    IN IO_FILE_HANDLE FileHandle,
    IN PVOID FileContext
    );

PVOID
IoFileGetContext(
    IN IO_FILE_HANDLE FileHandle
    );

// IRP functions for async processing

NTSTATUS
IoIrpSetCompletionCallback(
    IN PIRP Irp,
    IN PIO_IRP_CALLBACK Callback,
    IN PVOID CallbackContext
    );

NTSTATUS
IoIrpSetCancelCallback(
    IN PIRP Irp,
    IN PIO_IRP_CALLBACK Callback,
    IN PVOID CallbackContext
    );

// must have set IO status block in IRP.
NTSTATUS
IoIrpComplete(
    IN PIRP Irp
    );

// Drivrer memory

VOID
IoMemoryZero(
    IN OUT PVOID pMemory,
    IN size_t Size
    );

PVOID
IoMemoryAllocate(
    IN size_t Size
    );

VOID
IoMemoryFree(
    IN OUT PVOID pMemory
    );

#define IO_ALLOCATE(ppMemory, Type, Size) \
    ( (*(ppMemory)) = (Type*) IoMemoryAllocate(Size), (*(ppMemory)) ? STATUS_SUCCESS : STATUS_INSUFFICIENT_RESOURCES )

#define IO_FREE(ppMemory) \
    do { \
        if (*(ppMemory)) \
        { \
            IoMemoryFree(*(ppMemory)); \
            (*(ppMemory)) = NULL; \
        } \
    } while (0)

#define IO_LOG_ENTER(Format, ...) \
    SMB_LOG_DEBUG("ENTER: " Format, ## __VA_ARGS__)

#define IO_LOG_LEAVE(Format, ...) \
    SMB_LOG_DEBUG("LEAVE: " Format, ## __VA_ARGS__)

#define IO_LOG_ENTER_LEAVE(Format, ...) \
    SMB_LOG_DEBUG("ENTER/LEAVE: " Format, ## __VA_ARGS__)

#define IO_LOG_LEAVE_ON_STATUS_EE(status, EE) \
    do { \
        if (EE || status) \
        { \
            IO_LOG_LEAVE("-> 0x%08x (EE = %d)", status, EE); \
        } \
    } while (0)

#define IO_LOG_ENTER_LEAVE_STATUS_EE(status, EE) \
    IO_LOG_ENTER_LEAVE("-> 0x%08x (EE = %d)", status, EE)

#define IO_LOG_ENTER_LEAVE_STATUS_EE_EX(status, EE, Format, ...) \
    IO_LOG_ENTER_LEAVE(Format " -> 0x%08x (EE = %d)", ## __VA_ARGS__, status, EE)

#endif /* __IODRIVER_H__ */
