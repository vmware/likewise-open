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

#ifndef __IO_TYPES_H__
#define __IO_TYPES_H__

#include <lw/types.h>
#include <lw/attrs.h>
#include <lw/ntstatus.h>

#define SetFlag(Variable, Flags)   ((Variable) |= (Flags))
#define ClearFlag(Variable, Flags) ((Variable) &= ~(Flags))
#define IsSetFlag(Variable, Flags) (((Variable) & (Flags)) != 0)

typedef ULONG ACCESS_MASK;

typedef ULONG FILE_SHARE_FLAGS;

// Allow file sharing for read access.
#define FILE_SHARE_READ   0x1
// Allow file sharing for write access.
#define FILE_SHARE_WRITE  0x2
// Allow file sharing for delete access.
#define FILE_SHARE_DELETE 0x4

typedef ULONG FILE_CREATE_DISPOSITION;

#define FILE_SUPERSEDE    0
#define FILE_OPEN         1
#define FILE_CREATE       2
#define FILE_OPEN_IF      3
#define FILE_OVERWRITE    4
#define FILE_OVERWRITE_IF 5

typedef ULONG FILE_CREATE_OPTIONS;

#define FILE_DIRECTORY_FILE             0x00000001
#define FILE_NON_DIRECTORY_FILE         0x00000040
#define FILE_WRITE_THROUGH              0x00000002
#define FILE_SEQUENTIAL_ONLY            0x00000004
#define FILE_RANDOM_ACCESS              0x00000800
#define FILE_NO_INTERMEDIATE_BUFFERING  0x00000008
#if 0
#define FILE_SYNCHRONOUS_IO_ALERT       0x00000010
#define FILE_SYNCRHONOUS_IO_NONALERT    0x00000020
#endif
#define FILE_CREATE_TREE_CONNECTION     0x00000080
#define FILE_COMPLETE_IF_OPLOCKED       0x00000100
#define FILE_NO_EA_KNOWLEDGE            0x00000200
#define FILE_DELETE_ON_CLOSE            0x00001000
#define FILE_OPEN_BY_FILE_ID            0x00002000
#define FILE_OPEN_FOR_BACKUP_INTENT     0x00004000

typedef ULONG FILE_ATTRIBUTES;

#define FILE_ATTRIBUTE_READONLY              0x00000001
#define FILE_ATTRIBUTE_HIDDEN                0x00000002
#define FILE_ATTRIBUTE_SYSTEM                0x00000004
#define FILE_ATTRIBUTE_DIRECTORY             0x00000010
#define FILE_ATTRIBUTE_ARCHIVE               0x00000020
#define FILE_ATTRIBUTE_DEVICE                0x00000040
#define FILE_ATTRIBUTE_NORMAL                0x00000080
#define FILE_ATTRIBUTE_TEMPORARY             0x00000100
/* missing stuff */
#define FILE_ATTRIBUTE_OFFLINE               0x00001000
#define FILE_ATTRIBUTE_NOT_CONTENT_INDEXED   0x00002000
#define FILE_ATTRIBUTE_ENCRYPTED             0x00004000

#ifdef WIN32
// These are Win32 flags that get mapped to the NT-level flags.
// They should be in some Win32-level header.
#define FILE_FLAG_BACKUP_SEMANTICS
#define FILE_FLAG_DELETE_ON_CLOSE
#define FILE_FLAG_NO_BUFFERING
#define FILE_FLAG_OPEN_NO_RECALL
#define FILE_FLAG_OPEN_REPARSE_POINT
#define FILE_FLAG_OVERLAPPED
#define FILE_FLAG_POSIX_SEMANTICS
#define FILE_FLAG_RANDOM_ACCESS
#define FILE_FLAG_SEQUENTIAL_SCAN
#define FILE_FLAG_WRITE_THROUGH
#endif

typedef ULONG FILE_CREATE_RESULT;

#define FILE_SUPERSEDED       0
#define FILE_OPENED           1
#define FILE_CREATED          2
#define FILE_OVERWRITTEN      3
#define FILE_EXISTS           4
#define FILE_DOES_NOT_EXIST   5

typedef ULONG SECURITY_INFORMATION;

#define OWNER_SECURITY_INFORMATION  0x00000001
#define GROUP_SECURITY_INFORMATION  0x00000002
#define DACL_SECURITY_INFORMATION   0x00000004
#define SACL_SECURITY_INFORMATION   0x00000008

struct _IO_EVENT_OBJECT;
typedef struct _IO_EVENT_OBJECT IO_EVENT_OBJECT, *PIO_EVENT_OBJECT;
typedef IO_EVENT_OBJECT *IO_EVENT_HANDLE, **PIO_EVENT_HANDLE;

struct _IO_FILE_OBJECT;
typedef struct _IO_FILE_OBJECT IO_FILE_OBJECT, *PIO_FILE_OBJECT;
typedef IO_FILE_OBJECT *IO_FILE_HANDLE, **PIO_FILE_HANDLE;

// Available namespaces
#define IO_NS_NATIVE  "/Device"
#define IO_NS_WIN32  "/Win32"

typedef ULONG IO_NAME_OPTIONS;

#define IO_NAME_OPTION_CASE_SENSITIVE 0x00000001

// TODO-Use UNICODE_STRING
typedef struct _IO_FILE_NAME {
    OPTIONAL IO_FILE_HANDLE RootFileHandle;
    PWSTR FileName;
    IO_NAME_OPTIONS IoNameOptions;
} IO_FILE_NAME, *PIO_FILE_NAME;

typedef ULONG IO_FILE_SPEC_TYPE;

#define IO_FILE_SPEC_TYPE_UNKNOWN 0
#define IO_FILE_SPEC_TYPE_WIN32   1

// TODO-Use UNICODE_STRING
typedef struct _IO_FILE_SPEC {
    PWSTR FileName;
    IO_FILE_SPEC_TYPE Type;
    IO_NAME_OPTIONS IoNameOptions;
} IO_FILE_SPEC, *PIO_FILE_SPEC;

typedef struct _IO_STATUS_BLOCK {
    NTSTATUS Status;
    // NOTE: If the union below is changed, the IPC layer may need
    //       to be changed as well.
    union {
        ULONG BytesTransferred;
        FILE_CREATE_RESULT CreateResult;
    };
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

typedef VOID (*PIO_COMPLETION_CALLBACK)(
    IN PVOID CallbackContext,
    IN PIO_STATUS_BLOCK IoStatusBlock
    );

typedef struct _IO_ASYNC_CONTROL_BLOCK {
    OPTIONAL IO_EVENT_HANDLE Event;
    OPTIONAL PIO_COMPLETION_CALLBACK Callback;
    OPTIONAL PVOID CallbackContext;
} IO_ASYNC_CONROL_BLOCK, *PIO_ASYNC_CONTROL_BLOCK;

typedef UCHAR IO_CREATE_SECURITY_CONTEXT_IMPERSONATION_TYPE;

#define IO_CREATE_SECURITY_CONTEXT_IMPERSONATION_TYPE_NONE     0
#define IO_CREATE_SECURITY_CONTEXT_IMPERSONATION_TYPE_PASSWORD 1
#define IO_CREATE_SECURITY_CONTEXT_IMPERSONATION_TYPE_KERBEROS 2

typedef struct _IO_CREATE_SECURITY_CONTEXT {
    IO_CREATE_SECURITY_CONTEXT_IMPERSONATION_TYPE ImpersonationType;
    struct {
        uid_t Uid;
        gid_t Gid;
    } Process;
    union {
        struct {
            PWSTR Username;
            PWSTR Password;
        } Password;
        struct {
            PWSTR Principal;
            PWSTR CachePath;
        } Kerberos;
    } Impersonation;
} IO_CREATE_SECURITY_CONTEXT, *PIO_CREATE_SECURITY_CONTEXT;

// TDB:
#if 1
typedef struct _IO_EA_BUFFER IO_EA_BUFFER, *PIO_EA_BUFFER;
#else
// This structure is to simplify EA buffer processing
typedef struct _IO_EA_BUFFER {
    BOOLEAN IsParsed;
    union {
        struct {
            // Really a FILE_FULL_EA_INFORMATION
            PVOID pBuffer;
            ULONG Length;
        } Raw;
        struct {
            // TBD
        } Parsed;
    };
} IO_EA_BUFFER, *PIO_EA_BUFFER;
#endif

#if 0
typedef struct _FILE_FULL_EA_INFORMATION {
    ULONG NextEntryOffset;
    UCHAR Flags;
    UCHAR EaNameLength;
    USHORT EaValueLength;
    CHAR EaName[1];
} FILE_FULL_EA_INFORMATION, *PFILE_FULL_EA_INFORMATION;
#endif

typedef struct __LW_IO_CONTEXT LW_IO_CONTEXT, *LW_PIO_CONTEXT;
typedef struct __LW_IO_ACCESS_TOKEN LW_IO_ACCESS_TOKEN, *LW_PIO_ACCESS_TOKEN;

#ifndef PSECURITY_DESCRIPTOR_DEFINED

typedef PVOID PSECURITY_DESCRIPTOR;

#define PSECURITY_DESCRIPTOR_DEFINED
#endif

typedef PVOID PSID;
typedef PVOID PFILE_NETWORK_OPEN_INFORMATION;

typedef ULONG FILE_INFORMATION_CLASS;
typedef ULONG FS_INFORMATION_CLASS;

#ifndef LW_STRICT_NAMESPACE

typedef LW_IO_CONTEXT IO_CONTEXT;
typedef LW_PIO_CONTEXT PIO_CONTEXT;
typedef LW_IO_ACCESS_TOKEN IO_ACCESS_TOKEN;
typedef LW_PIO_ACCESS_TOKEN PIO_ACCESS_TOKEN;

#endif /* ! LW_STRICT_NAMESPACE */

#endif
