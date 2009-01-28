/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
 * All rights reserved.
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
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        ntipcmsg.h
 *
 * Abstract:
 *
 *        NT lwmsg IPC
 *
 * Authors: Danilo Almeida (dalmeida@likewisesoftware.com)
 */

#ifndef __NT_IPC_MSG_H__
#define __NT_IPC_MSG_H__

#include <lwmsg/lwmsg.h>
#include "lwiodef.h"
#include "lwioipc.h"
#include <lwio/io-types.h>

// TODO-Add async completion support.

// TODO-Add check in lwmsg to check for protocol tags > 16 bits.
// Apparently, the server crashed if you do that.
#define _NT_IPC_MESSAGE_TYPE_BASE 10000

//
// Protocol Message Types
//
// The message types map to message structures like this:
//
//   NT_IPC_MESSAGE_TYPE_<XXX> --> NT_IPC_MESSAGE_<XXX>
//
// Any exceptions have a comment with the corresponding structure type.
//

typedef enum _NT_IPC_MESSAGE_TYPE
{
    NT_IPC_MESSAGE_TYPE_CREATE_FILE = _NT_IPC_MESSAGE_TYPE_BASE,
    NT_IPC_MESSAGE_TYPE_CREATE_FILE_RESULT,
    NT_IPC_MESSAGE_TYPE_CLOSE_FILE,                     // NT_IPC_MESSAGE_GENERIC_FILE
    NT_IPC_MESSAGE_TYPE_CLOSE_FILE_RESULT,              // NT_IPC_MESSAGE_GENERIC_FILE_IO_RESULT
    NT_IPC_MESSAGE_TYPE_READ_FILE,
    NT_IPC_MESSAGE_TYPE_READ_FILE_RESULT,               // NT_IPC_MESSAGE_GENERIC_FILE_BUFFER_RESULT
    NT_IPC_MESSAGE_TYPE_WRITE_FILE,
    NT_IPC_MESSAGE_TYPE_WRITE_FILE_RESULT,              // NT_IPC_MESSAGE_GENERIC_FILE_IO_RESULT
    NT_IPC_MESSAGE_TYPE_DEVICE_IO_CONTROL_FILE,         // NT_IPC_MESSAGE_GENERIC_CONTROL_FILE
    NT_IPC_MESSAGE_TYPE_DEVICE_IO_CONTROL_FILE_RESULT,  // NT_IPC_MESSAGE_GENERIC_FILE_BUFFER_RESULT
    NT_IPC_MESSAGE_TYPE_FS_CONTROL_FILE,                // NT_IPC_MESSAGE_GENERIC_CONTROL_FILE
    NT_IPC_MESSAGE_TYPE_FS_CONTROL_FILE_RESULT,         // NT_IPC_MESSAGE_GENERIC_FILE_BUFFER_RESULT
    NT_IPC_MESSAGE_TYPE_FLUSH_BUFFERS_FILE,             // NT_IPC_MESSAGE_GENERIC_FILE
    NT_IPC_MESSAGE_TYPE_FLUSH_BUFFERS_FILE_RESULT,      // NT_IPC_MESSAGE_GENERIC_FILE_IO_RESULT
    NT_IPC_MESSAGE_TYPE_QUERY_INFORMATION_FILE,
    NT_IPC_MESSAGE_TYPE_QUERY_INFORMATION_FILE_RESULT,  // NT_IPC_MESSAGE_GENERIC_FILE_BUFFER_RESULT
    NT_IPC_MESSAGE_TYPE_SET_INFORMATION_FILE,
    NT_IPC_MESSAGE_TYPE_SET_INFORMATION_FILE_RESULT     // NT_IPC_MESSAGE_GENERIC_FILE_IO_RESULT
} NT_IPC_MESSAGE_TYPE, *PNT_IPC_MESSAGE_TYPE;

//
// Protocol Pseudo Types
//

// TODO-Remove pseudo-types (requires support for "optional handle" in lwmsg).
// The issue that that handles cannot be NULL.

#define _NT_IPC_USE_PSEUDO_TYPES 1

#ifdef _NT_IPC_USE_PSEUDO_TYPES
typedef union __NT_IPC_PSEUDO_OPTIONAL_IO_FILE_HANDLE_VARIANT {
#if 0
    struct {} Invalid;
#else
    // Dummy field
    BYTE Invalid;
#endif
    IO_FILE_HANDLE FileHandle;
} _NT_IPC_PSEUDO_OPTIONAL_IO_FILE_HANDLE_VARIANT;

typedef struct _NT_IPC_PSEUDO_OPTIONAL_IO_FILE_HANDLE {
    BOOLEAN IsValid;
    _NT_IPC_PSEUDO_OPTIONAL_IO_FILE_HANDLE_VARIANT Variant;
} NT_IPC_PSEUDO_OPTIONAL_IO_FILE_HANDLE, *PNT_IPC_PSEUDO_OPTIONALIO_FILE_HANDLE;

// For IO_FILE_NAME
typedef struct _NT_IPC_PSEUDO_IO_FILE_NAME {
    OPTIONAL NT_IPC_PSEUDO_OPTIONAL_IO_FILE_HANDLE RootFileHandle;
    PWSTR FileName;
    IO_NAME_OPTIONS IoNameOptions;
} NT_IPC_PSEUDO_IO_FILE_NAME, *PNT_IPC_PSEUDO_IO_FILE_NAME;
#endif

//
// Protocol Helper Types
//

typedef struct _NT_IPC_HELPER_ECP {
    IN PCSTR pszType;
    IN PVOID pData;
    IN ULONG Size;
} NT_IPC_HELPER_ECP, *PNT_IPC_HELPER_ECP;

//
// Protocol Messages
//

typedef struct _NT_IPC_MESSAGE_GENERIC_FILE {
    IN IO_FILE_HANDLE FileHandle;
} NT_IPC_MESSAGE_GENERIC_FILE, *PNT_IPC_MESSAGE_GENERIC_FILE;

typedef struct _NT_IPC_MESSAGE_GENERIC_CONTROL_FILE {
    IN IO_FILE_HANDLE FileHandle;
    IN ULONG ControlCode;
    IN PVOID InputBuffer;
    IN ULONG InputBufferLength;
    IN ULONG OutputBufferLength;
} NT_IPC_MESSAGE_GENERIC_CONTROL_FILE, *PNT_IPC_MESSAGE_GENERIC_CONTROL_FILE;

typedef struct _NT_IPC_MESSAGE_GENERIC_FILE_IO_RESULT {
    // From IO_STATUS_BLOCK (which uses context-based union):
    OUT NTSTATUS Status;
    OUT ULONG BytesTransferred;
} NT_IPC_MESSAGE_GENERIC_FILE_IO_RESULT, *PNT_IPC_MESSAGE_GENERIC_FILE_IO_RESULT;

typedef struct _NT_IPC_MESSAGE_GENERIC_FILE_BUFFER_RESULT {
    // Break up IO_STATUS_BLOCK into Status and BytesTransferred so that the
    // IPC marshaling layer can use BytesTransferred for the length of Buffer.

    // From IO_STATUS_BLOCK (which uses context-based union):
    OUT NTSTATUS Status;
    OUT ULONG BytesTransferred;

    // Length is BytesTransferred.
    OUT PVOID Buffer;
} NT_IPC_MESSAGE_GENERIC_FILE_BUFFER_RESULT, *PNT_IPC_MESSAGE_GENERIC_FILE_BUFFER_RESULT;

//
// NtCreateFile
//
// IN TAG:  NT_IPC_MESSAGE_TYPE_CREATE_FILE
// OUT TAG: NT_IPC_MESSAGE_TYPE_CREATE_FILE_RESULT
//
// IN:  NT_IPC_MESSAGE_CREATE_FILE
// OUT: NT_IPC_MESSAGE_CREATE_FILE_RESULT
//

typedef struct _NT_IPC_MESSAGE_CREATE_FILE {
    IN PIO_ACCESS_TOKEN pSecurityToken;
#ifdef _NT_IPC_USE_PSEUDO_TYPES
    IN NT_IPC_PSEUDO_IO_FILE_NAME FileName;
#else
    IN IO_FILE_NAME FileName;
#endif
    IN ACCESS_MASK DesiredAccess;
    IN OPTIONAL LONG64 AllocationSize;
    IN FILE_ATTRIBUTES FileAttributes;
    IN FILE_SHARE_FLAGS ShareAccess;
    IN FILE_CREATE_DISPOSITION CreateDisposition;
    IN FILE_CREATE_OPTIONS CreateOptions;
    IN OPTIONAL PVOID EaBuffer; // FILE_FULL_EA_INFORMATION
    IN ULONG EaLength;
    // TODO -- Add stuff for SDs, etc.
#if 0
    IN OPTIONAL PVOID SecurityDescriptor; // TBD
    IN OPTIONAL PVOID SecurityQualityOfService; // TBD
#endif
    IN OPTIONAL PNT_IPC_HELPER_ECP EcpList;
    IN ULONG EcpCount;
} NT_IPC_MESSAGE_CREATE_FILE, *PNT_IPC_MESSAGE_CREATE_FILE;

typedef struct _NT_IPC_MESSAGE_CREATE_FILE_RESULT {
#ifdef _NT_IPC_USE_PSEUDO_TYPES
    OUT NT_IPC_PSEUDO_OPTIONAL_IO_FILE_HANDLE FileHandle;
#else
    OUT IO_FILE_HANDLE FileHandle;
#endif

    // From IO_STATUS_BLOCK (which uses context-based union):
    OUT NTSTATUS Status;
    OUT FILE_CREATE_RESULT CreateResult;
} NT_IPC_MESSAGE_CREATE_FILE_RESULT, *PNT_IPC_MESSAGE_CREATE_FILE_RESULT;

//
// NtCloseFile
//
// IN TAG:  NT_IPC_MESSAGE_TYPE_CLOSE_FILE
// OUT TAG: NT_IPC_MESSAGE_TYPE_CLOSE_FILE_RESULT
//
// IN:  NT_IPC_MESSAGE_GENERIC_FILE
// OUT: NT_IPC_MESSAGE_GENERIC_FILE_IO_RESULT
//

//
// NtReadFile
//
// IN TAG:  NT_IPC_MESSAGE_TYPE_READ_FILE
// OUT TAG: NT_IPC_MESSAGE_TYPE_READ_FILE_RESULT
//
// IN:  NT_IPC_MESSAGE_READ_FILE
// OUT: NT_IPC_MESSAGE_GENERIC_FILE_BUFFER_RESULT
//

typedef struct _NT_IPC_MESSAGE_READ_FILE {
    IN IO_FILE_HANDLE FileHandle;
    IN ULONG Length;
    IN OPTIONAL PLONG64 ByteOffset;
    IN OPTIONAL PULONG Key;
} NT_IPC_MESSAGE_READ_FILE, *PNT_IPC_MESSAGE_READ_FILE;

//
// NtWriteFile
//
// IN TAG:  NT_IPC_MESSAGE_TYPE_WRITE_FILE
// OUT TAG: NT_IPC_MESSAGE_TYPE_WRITE_FILE_RESULT
//
// IN:  NT_IPC_MESSAGE_READ_FILE
// OUT: NT_IPC_MESSAGE_GENERIC_FILE_IO_RESULT
//

typedef struct _NT_IPC_MESSAGE_WRITE_FILE {
    IN IO_FILE_HANDLE FileHandle;
    IN PVOID Buffer;
    IN ULONG Length;
    IN OPTIONAL PLONG64 ByteOffset;
    IN OPTIONAL PULONG Key;
} NT_IPC_MESSAGE_WRITE_FILE, *PNT_IPC_MESSAGE_WRITE_FILE;

//
// NtDeviceIoControlFile
//
// IN TAG:  NT_IPC_MESSAGE_TYPE_DEVICE_IO_CONTROL_FILE
// OUT TAG: NT_IPC_MESSAGE_TYPE_DEVICE_IO_CONTROL_FILE_RESULT
//
// IN:  NT_IPC_MESSAGE_GENERIC_CONTROL_FILE
// OUT: NT_IPC_MESSAGE_GENERIC_FILE_BUFFER_RESULT
//

//
// NtFsControlFile
//
// IN TAG:  NT_IPC_MESSAGE_TYPE_FS_CONTROL_FILE
// OUT TAG: NT_IPC_MESSAGE_TYPE_FS_CONTROL_FILE_RESULT
//
// IN:  NT_IPC_MESSAGE_GENERIC_CONTROL_FILE
// OUT: NT_IPC_MESSAGE_GENERIC_FILE_BUFFER_RESULT
//

//
// NtFlushBuffersFile
//
// IN TAG:  NT_IPC_MESSAGE_TYPE_FLUSH_BUFFERS_FILE
// OUT TAG: NT_IPC_MESSAGE_TYPE_FLUSH_BUFFERS_FILE_RESULT
//
// IN:  NT_IPC_MESSAGE_GENERIC_FILE
// OUT: NT_IPC_MESSAGE_GENERIC_FILE_IO_RESULT
//

//
// NtQueryInformationFile
//
// IN TAG:  NT_IPC_MESSAGE_TYPE_QUERY_INFORMATION_FILE
// OUT TAG: NT_IPC_MESSAGE_TYPE_QUERY_INFORMATION_FILE_RESULT
//
// IN:  NT_IPC_MESSAGE_QUERY_INFORMATION_FILE
// OUT: NT_IPC_MESSAGE_GENERIC_FILE_BUFFER_RESULT
//

typedef struct _NT_IPC_MESSAGE_QUERY_INFORMATION_FILE {
    IN IO_FILE_HANDLE FileHandle;
    IN ULONG Length;
    IN FILE_INFORMATION_CLASS FileInformationClass;
} NT_IPC_MESSAGE_QUERY_INFORMATION_FILE, *PNT_IPC_MESSAGE_QUERY_INFORMATION_FILE;

//
// NtSetInformationFile
//
// IN TAG:  NT_IPC_MESSAGE_TYPE_SET_INFORMATION_FILE
// OUT TAG: NT_IPC_MESSAGE_TYPE_SET_INFORMATION_FILE_RESULT
//
// IN:  NT_IPC_MESSAGE_SET_INFORMATION_FILE
// OUT: NT_IPC_MESSAGE_GENERIC_FILE_IO_RESULT
//

typedef struct _NT_IPC_MESSAGE_SET_INFORMATION_FILE {
    IN IO_FILE_HANDLE FileHandle;
    IN PVOID FileInformation;
    IN ULONG Length;
    IN FILE_INFORMATION_CLASS FileInformationClass;
} NT_IPC_MESSAGE_SET_INFORMATION_FILE, *PNT_IPC_MESSAGE_SET_INFORMATION_FILE;

//
// Functions
//

NTSTATUS
NtIpcLWMsgStatusToNtStatus(
    IN LWMsgStatus LwMsgStatus
    );

LWMsgStatus
NtIpcNtStatusToLWMsgStatus(
    IN NTSTATUS Status
    );

NTSTATUS
NtIpcAddProtocolSpec(
    IN OUT LWMsgProtocol* pProtocol
    );

NTSTATUS
NtIpcUnregisterFileHandle(
    IN LWMsgAssoc* pAssoc,
    IN IO_FILE_HANDLE FileHandle
    );

#ifdef _NT_IPC_USE_PSEUDO_TYPES
VOID
NtIpcRealToPseudoIoFileHandle(
    IN OPTIONAL IO_FILE_HANDLE FileHandle,
    OUT PNT_IPC_PSEUDO_OPTIONALIO_FILE_HANDLE pPseudoFileHandle
    );

VOID
NtIpcRealFromPseudoIoFileHandle(
    OUT PIO_FILE_HANDLE pFileHandle,
    IN PNT_IPC_PSEUDO_OPTIONALIO_FILE_HANDLE pPseudoFileHandle
    );

VOID
NtIpcRealToPseudoIoFileName(
    IN PIO_FILE_NAME FileName,
    OUT PNT_IPC_PSEUDO_IO_FILE_NAME pPseudoFileName
    );

VOID
NtIpcRealFromPseudoIoFileName(
    OUT PIO_FILE_NAME FileName,
    IN PNT_IPC_PSEUDO_IO_FILE_NAME pPseudoFileName
    );
#endif

#endif /* __NT_IPC_MSG_H__ */
