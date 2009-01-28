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

#include <lw/base.h>
#include <secdesc/secdesc.h>

#define SetFlag(Variable, Flags)   ((Variable) |= (Flags))
#define ClearFlag(Variable, Flags) ((Variable) &= ~(Flags))
#define IsSetFlag(Variable, Flags) (((Variable) & (Flags)) != 0)

typedef ULONG ACCESS_MASK;

/* Work around problem with /usr/include/arpa/nameser_compat.h */
#ifdef DELETE
# undef DELETE
#endif

#define DELETE                    0x00010000 /* right to delete object */
#define READ_CONTROL              0x00020000 /* read the object's SID not including SACL */
#define WRITE_DAC                 0x00040000 /* modify the DACL in the object's SID      */
#define WRITE_OWNER               0x00080000 /* change the owner in the object's SID     */
#define SYNCHRONIZE               0x00100000 /* use the object for synchronization       */
#define ACCESS_SYSTEM_SECURITY    0x01000000 /* get or set the SACL in an object's SID   */
#define MAXIMUM_ALLOWED           0x02000000 /* all access rights valid for the caller   */
#define GENERIC_ALL               0x10000000
#define GENERIC_EXECUTE           0x20000000
#define GENERIC_WRITE             0x40000000
#define GENERIC_READ              0x80000000

#define FILE_READ_DATA            0x00000001 /* File/NP     */
#define FILE_LIST_DIRECTORY       0x00000001 /* Dir         */
#define FILE_WRITE_DATA           0x00000002 /* File/NP     */
#define FILE_ADD_FILE             0x00000002 /* Dir         */
#define FILE_APPEND_DATA          0x00000004 /* File        */
#define FILE_ADD_SUBDIRECTORY     0x00000004 /* Dir         */
#define FILE_CREATE_PIPE_INSTANCE 0x00000004 /* NP          */
#define FILE_READ_EA              0x00000008 /* File/Dir    */
#define FILE_WRITE_EA             0x00000010 /* File/Dir    */
#define FILE_EXECUTE              0x00000020 /* File        */
#define FILE_TRAVERSE             0x00000020 /* Dir         */
#define FILE_DELETE_CHILD         0x00000020 /* Dir         */
#define FILE_READ_ATTRIBUTES      0x00000080 /* File/NP/Dir */
#define FILE_WRITE_ATTRIBUTES     0x00000100 /* File/NP/Dir */

#define STANDARD_RIGHTS_REQUIRED  0x0F0000

#define STANDARD_RIGHTS_READ      READ_CONTROL
#define STANDARD_RIGHTS_WRITE     READ_CONTROL
#define STANDARD_RIGHTS_EXECUTE   READ_CONTROL

#define STANDARD_RIGHTS_ALL       0x1F0000

#define SPECIFIC_RIGHTS_ALL       0x000FFFF

#define SHARE_READ                0x00000001
#define SHARE_WRITE               0x00000002

#define PIPE_READMODE_BYTE        0x00000000
#define PIPE_READMODE_MESSAGE     0x00000002
#define PIPE_WAIT                 0x00000000
#define PIPE_NOWAIT               0x00000001

#define FILE_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0x1FF)

#define FILE_GENERIC_READ         (STANDARD_RIGHTS_READ     |\
                                   FILE_READ_DATA           |\
                                   FILE_READ_ATTRIBUTES     |\
                                   FILE_READ_EA             |\
                                   SYNCHRONIZE)

#define FILE_GENERIC_WRITE        (STANDARD_RIGHTS_WRITE    |\
                                   FILE_WRITE_DATA          |\
                                   FILE_WRITE_ATTRIBUTES    |\
                                   FILE_WRITE_EA            |\
                                   FILE_APPEND_DATA         |\
                                   SYNCHRONIZE)

#define FILE_GENERIC_EXECUTE      (STANDARD_RIGHTS_EXECUTE  |\
                                   FILE_READ_ATTRIBUTES     |\
                                   FILE_EXECUTE             |\
                                   SYNCHRONIZE)

#define FILE_FLAG_WRITE_THROUGH         0x80000000
#define FILE_FLAG_OVERLAPPED            0x40000000
#define FILE_FLAG_NO_BUFFERING          0x20000000
#define FILE_FLAG_RANDOM_ACCESS         0x10000000
#define FILE_FLAG_SEQUENTIAL_SCAN       0x08000000
#define FILE_FLAG_DELETE_ON_CLOSE       0x04000000
#define FILE_FLAG_BACKUP_SEMANTICS      0x02000000
#define FILE_FLAG_POSIX_SEMANTICS       0x01000000
#define FILE_FLAG_OPEN_REPARSE_POINT    0x00200000
#define FILE_FLAG_OPEN_NO_RECALL        0x00100000
#define FILE_FLAG_FIRST_PIPE_INSTANCE   0x00080000

#define CREATE_NEW                      1
#define CREATE_ALWAYS                   2
#define OPEN_EXISTING                   3
#define OPEN_ALWAYS                     4
#define TRUNCATE_EXISTING               5


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

typedef ULONG FILE_PIPE_TYPE_MASK;

#define FILE_PIPE_BYTE_STREAM_TYPE      0x00000000
#define FILE_PIPE_MESSAGE_TYPE          0x00000001

#define FILE_PIPE_ACCEPT_REMOTE_CLIENTS 0x00000000
#define FILE_PIPE_REJECT_REMOTE_CLIENTS 0x00000002

#define FILE_PIPE_TYPE_VALID_MASK ( \
    FILE_PIPE_MESSAGE_TYPE | \
    FILE_PIPE_REJECT_REMOTE_CLIENTS | \
    0 )

typedef ULONG FILE_PIPE_READ_MODE_MASK;

#define FILE_PIPE_BYTE_STREAM_MODE      0x00000000
#define FILE_PIPE_MESSAGE_MODE          0x00000001

#define FILE_PIPE_READ_MODE_VALID_MASK ( \
    FILE_PIPE_MESSAGE_MODE | \
    0 )

typedef ULONG FILE_PIPE_COMPLETION_MODE_MASK;

#define FILE_PIPE_QUEUE_OPERATION       0x00000000
#define FILE_PIPE_COMPLETE_OPERATION    0x00000001

#define FILE_PIPE_COMPLETION_MODE_VALID_MASK ( \
    FILE_PIPE_COMPLETE_OPERATION | \
    0 )

typedef struct __LW_IO_CONTEXT LW_IO_CONTEXT, *LW_PIO_CONTEXT;
typedef struct __LW_IO_ACCESS_TOKEN LW_IO_ACCESS_TOKEN, *LW_PIO_ACCESS_TOKEN;

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

typedef struct _IO_CREATE_SECURITY_CONTEXT {
    struct {
        uid_t Uid;
        gid_t Gid;
    } Process;
    LW_PIO_ACCESS_TOKEN pAccessToken;
} IO_CREATE_SECURITY_CONTEXT, *PIO_CREATE_SECURITY_CONTEXT;

typedef ULONG FILE_INFORMATION_CLASS, *PFILE_INFORMATION_CLASS;

// FILE_INFORMATION_CLASS values are FileXxxInformation:

#define FileDirectoryInformation           1 // DIR: FILE_DIRECTORY_INFORMATION
#define FileFullDirectoryInformation       2 // DIR: FILE_FULL_DIR_INFORMATION
#define FileBothDirectoryInformation       3 // DIR: FILE_BOTH_DIR_INFORMATION
#define FileBasicInformation               4 // QUERY/SET: FILE_BASIC_INFORMATION
#define FileStandardInformation            5 // QUERY: FILE_STANDARD_INFORMATION
#define FileInternalInformation            6 // QUERY: FILE_INTERNAL_INFORMATION
#define FileEaInformation                  7 // QUERY: FILE_EA_INFORMATION
#define FileAccessInformation              8 // QUERY: FILE_ACCESS_INFORMATION
#define FileNameInformation                9 // QUERY: FILE_NAME_INFORMATION
#define FileRenameInformation             10 // TODO--do rename differently (SET: FILE_RENAME_INFORMATION)
#define FileLinkInformation               11 // TODO--do create hardlink differently (SET: FILE_LINK_INFORMATION (create hardlink))
#define FileNamesInformation              12 // DIR: FILE_NAMES_INFORMATION
#define FileDispositionInformation        13 // SET: FILE_DISPOSITION_INFORMATION
#define FilePositionInformation           14 // QUERY/SET: FILE_POSITION_INFORMATION
#define FileFullEaInformation             15 // QUERY: FILE_FULL_EA_INFORMATION (all EAs?)
#define FileModeInformation               16 // QUERY: FILE_MODE_INFORMATION
#define FileAlignmentInformation          17 // QUERY: FILE_ALIGNMENT_INFORMATION
#define FileAllInformation                18 // QUERY: FILE_ALL_INFORMATION
#define FileAllocationInformation         19 // SET: FILE_ALLOCATION_INFORMATION
#define FileEndOfFileInformation          20 // SET: FILE_END_OF_FILE_INFORMATION
#define FileAlternateNameInformation      21 // QUERY: FILE_NAME_INFORMATION (query 8.3 name)
#define FileStreamInformation             22 // QUERY: FILE_STREAM_INFORMATION
#define FilePipeInformation               23 // QUERY: FILE_PIPE_INFORMATION
#define FilePipeLocalInformation          24 // QUERY: FILE_PIPE_LOCAL_INFORMATION
#define FilePipeRemoteInformation         25 // QUERY: FILE_PIPE_REMOTE_INFORMATION ????
#define FileMailslotQueryInformation      26 // unused
#define FileMailslotSetInformation        27 // unused
#define FileCompressionInformation        28 // QUERY: FILE_COMPRESSION_INFORMATION
#define FileObjectIdInformation           29 // DIR: FILE_OBJECTID_INFORMATION
#define FileCompletionInformation         30 // unused
#define FileMoveClusterInformation        31 // unused
#define FileQuotaInformation              32 // obsolete - Use IRP_TYPE_QUERY_QUOTA (DIR: FILE_QUOTA_INFORMATION)
#define FileReparsePointInformation       33 // DIR: FILE_REPARSE_POINT_INFORMATION
#define FileNetworkOpenInformation        34 // QUERY: FILE_NETWORK_OPEN_INFORMATION
#define FileAttributeTagInformation       35 // QUERY: FILE_ATTRIBUTE_TAG_INFORMATION
#define FileTrackingInformation           36 // unused
#define FileIdBothDirectoryInformation    37 // DIR: FILE_ID_BOTH_DIR_INFORMATION
#define FileIdFullDirectoryInformation    38 // DIR: FILE_ID_FULL_DIR_INFORMATION
#define FileValidDataLengthInformation    39 // unused
#define FileShortNameInformation          40 // SET: FILE_NAME_INFORMATION (set 8.3 name)
#define FileMaximumInformation            41 // SENTINEL

//
// Notes:
//
// - FileAccessInformation and FileModeInformation are done by iomgr directly
//   by returning info from the IO_FILE_HANDLE.
//

//
// General File Information
//

// QUERY/SET: FileBasicInformation
typedef struct _FILE_BASIC_INFORMATION {
    LONG64 CreationTime;
    LONG64 LastAccessTime;
    LONG64 LastWriteTime;
    LONG64 ChangeTime;
    FILE_ATTRIBUTES FileAttributes;
} FILE_BASIC_INFORMATION, *PFILE_BASIC_INFORMATION;

// QUERY: FileStandardInformation
typedef struct _FILE_STANDARD_INFORMATION {
    LONG64 AllocationSize;
    LONG64 EndOfFile;
    ULONG NumberOfLinks;
    BOOLEAN DeletePending;
    BOOLEAN Directory;
} FILE_STANDARD_INFORMATION, *PFILE_STANDARD_INFORMATION;

// QUERY: FileInternalInformation
typedef struct _FILE_INTERNAL_INFORMATION {
    LONG64 IndexNumber;
} FILE_INTERNAL_INFORMATION, *PFILE_INTERNAL_INFORMATION;

// QUERY: FileEaInformation
typedef struct _FILE_EA_INFORMATION {
    ULONG EaSize;
} FILE_EA_INFORMATION, *PFILE_EA_INFORMATION;

// QUERY: FileAccessInformation
typedef struct _FILE_ACCESS_INFORMATION {
    ACCESS_MASK AccessFlags;
} FILE_ACCESS_INFORMATION, *PFILE_ACCESS_INFORMATION;

// QUERY: FileNameInformation
// QUERY: FileAlternateNameInformation (query 8.3 name)
// SET: FileShortNameInformation (set 8.3 name)
typedef struct _FILE_NAME_INFORMATION {
    ULONG FileNameLength;
    WCHAR FileName[1];
} FILE_NAME_INFORMATION, *PFILE_NAME_INFORMATION;

// SET: FileDispositionInformation
typedef struct _FILE_DISPOSITION_INFORMATION {
    BOOLEAN DeleteFile;
} FILE_DISPOSITION_INFORMATION, *PFILE_DISPOSITION_INFORMATION;

// QUERY/SET: FilePositionInformation
// Access: FILE_READ_DATA or FILE_WRITE_DATA
typedef struct _FILE_POSITION_INFORMATION {
    LONG64 CurrentByteOffset;
} FILE_POSITION_INFORMATION, *PFILE_POSITION_INFORMATION;

// QUERY: FileFullEaInformation (All EAs?)
// IRP_TYPE_QUERY_EA (OUT)
typedef struct _FILE_FULL_EA_INFORMATION {
    ULONG NextEntryOffset;
    UCHAR Flags;
    UCHAR EaNameLength;
    USHORT EaValueLength;
    CHAR EaName[1];
} FILE_FULL_EA_INFORMATION, *PFILE_FULL_EA_INFORMATION;

// QUERY: FileModeInformation
typedef struct _FILE_MODE_INFORMATION {
    FILE_CREATE_OPTIONS Mode;
} FILE_MODE_INFORMATION, *PFILE_MODE_INFORMATION;

// QUERY: FileAlignmentInformation
typedef struct _FILE_ALIGNMENT_INFORMATION {
    ULONG AlignmentRequirement;
} FILE_ALIGNMENT_INFORMATION, *PFILE_ALIGNMENT_INFORMATION;

// QUERY: FileAllInformation
typedef struct _FILE_ALL_INFORMATION {
    FILE_BASIC_INFORMATION BasicInformation;
    FILE_STANDARD_INFORMATION StandardInformation;
    FILE_INTERNAL_INFORMATION InternalInformation;
    FILE_EA_INFORMATION EaInformation;
    FILE_ACCESS_INFORMATION AccessInformation;
    FILE_POSITION_INFORMATION PositionInformation;
    FILE_MODE_INFORMATION ModeInformation;
    FILE_ALIGNMENT_INFORMATION AlignmentInformation;
    FILE_NAME_INFORMATION NameInformation;
} FILE_ALL_INFORMATION, *PFILE_ALL_INFORMATION;

// SET: FileAllocationInformation
typedef struct _FILE_ALLOCATION_INFORMATION {
    LONG64 AllocationSize;
} FILE_ALLOCATION_INFORMATION, *PFILE_ALLOCATION_INFORMATION;

// SET: FileEndOfFileInformation
typedef struct _FILE_END_OF_FILE_INFORMATION {
    LONG64 EndOfFile;
} FILE_END_OF_FILE_INFORMATION;

// QUERY: FileStreamInformation
typedef struct _FILE_STREAM_INFORMATION {
    ULONG NextEntryOffset;
    ULONG StreamNameLength;
    LONG64 StreamSize;
    LONG64 StreamAllocationSize;
    WCHAR StreamName[1];
} FILE_STREAM_INFORMATION, *PFILE_STREAM_INFORMATION;

// QUERY: FileCompressionInformation
typedef struct _FILE_COMPRESSION_INFORMATION {
    LONG64 CompressedFileSize;
    USHORT CompressionFormat;
    UCHAR CompressionUnitShift;
    UCHAR ChunkShift;
    UCHAR ClusterShift;
    UCHAR Reserved[3];
} FILE_COMPRESSION_INFORMATION, *PFILE_COMPRESSION_INFORMATION;

// QUERY: FileNetworkOpenInformation
typedef struct _FILE_NETWORK_OPEN_INFORMATION {
    LONG64 CreationTime;
    LONG64 LastAccessTime;
    LONG64 LastWriteTime;
    LONG64 ChangeTime;
    LONG64 AllocationSize;
    LONG64 EndOfFile;
    FILE_ATTRIBUTES FileAttributes;
} FILE_NETWORK_OPEN_INFORMATION, *PFILE_NETWORK_OPEN_INFORMATION;

// QUERY: FileAttributeTagInformation
typedef struct _FILE_ATTRIBUTE_TAG_INFORMATION {
    FILE_ATTRIBUTES FileAttributes;
    ULONG ReparseTag;
} FILE_ATTRIBUTE_TAG_INFORMATION, *PFILE_ATTRIBUTE_TAG_INFORMATION;

//
// Directory Information
//

// DIR: FileDirectoryInformation
typedef struct _FILE_DIRECTORY_INFORMATION {
    ULONG NextEntryOffset;
    ULONG FileIndex;
    LONG64 CreationTime;
    LONG64 LastAccessTime;
    LONG64 LastWriteTime;
    LONG64 ChangeTime;
    LONG64 EndOfFile;
    LONG64 AllocationSize;
    FILE_ATTRIBUTES FileAttributes;
    ULONG FileNameLength;
    WCHAR FileName[1];
} FILE_DIRECTORY_INFORMATION, *PFILE_DIRECTORY_INFORMATION;

// DIR: FileFullDirectoryInformation
typedef struct _FILE_FULL_DIR_INFORMATION {
    ULONG NextEntryOffset;
    ULONG FileIndex;
    LONG64 CreationTime;
    LONG64 LastAccessTime;
    LONG64 LastWriteTime;
    LONG64 ChangeTime;
    LONG64 EndOfFile;
    LONG64 AllocationSize;
    FILE_ATTRIBUTES FileAttributes;
    ULONG FileNameLength;
    ULONG EaSize;
    WCHAR FileName[1];
} FILE_FULL_DIR_INFORMATION, *PFILE_FULL_DIR_INFORMATION;

// DIR: FileIdFullDirectoryInformation
typedef struct _FILE_ID_FULL_DIR_INFORMATION {
    ULONG NextEntryOffset;
    ULONG FileIndex;
    LONG64 CreationTime;
    LONG64 LastAccessTime;
    LONG64 LastWriteTime;
    LONG64 ChangeTime;
    LONG64 EndOfFile;
    LONG64 AllocationSize;
    FILE_ATTRIBUTES FileAttributes;
    ULONG FileNameLength;
    ULONG EaSize;
    LONG64 FileId;
    WCHAR FileName[1];
} FILE_ID_FULL_DIR_INFORMATION, *PFILE_ID_FULL_DIR_INFORMATION;

// DIR: FileBothDirectoryInformation
typedef struct _FILE_BOTH_DIR_INFORMATION {
    ULONG NextEntryOffset;
    ULONG FileIndex;
    LONG64 CreationTime;
    LONG64 LastAccessTime;
    LONG64 LastWriteTime;
    LONG64 ChangeTime;
    LONG64 EndOfFile;
    LONG64 AllocationSize;
    FILE_ATTRIBUTES FileAttributes;
    ULONG FileNameLength;
    ULONG EaSize;
    UINT8 ShortNameLength; // TODO-real one is char...is that a problem?
    WCHAR ShortName[12];
    WCHAR FileName[1];
} FILE_BOTH_DIR_INFORMATION, *PFILE_BOTH_DIR_INFORMATION;

// DIR: FileIdBothDirectoryInformation
typedef struct _FILE_ID_BOTH_DIR_INFORMATION {
    ULONG NextEntryOffset;
    ULONG FileIndex;
    LONG64 CreationTime;
    LONG64 LastAccessTime;
    LONG64 LastWriteTime;
    LONG64 ChangeTime;
    LONG64 EndOfFile;
    LONG64 AllocationSize;
    FILE_ATTRIBUTES FileAttributes;
    ULONG FileNameLength;
    ULONG EaSize;
    UINT8 ShortNameLength; // TODO-real one is char...is that a problem?
    WCHAR ShortName[12];
    LONG64 FileId;
    WCHAR FileName[1];
} FILE_ID_BOTH_DIR_INFORMATION, *PFILE_ID_BOTH_DIR_INFORMATION;

// DIR: FileNamesInformation
typedef struct _FILE_NAMES_INFORMATION {
    ULONG NextEntryOffset;
    ULONG FileIndex;
    ULONG FileNameLength;
    WCHAR FileName[1];
} FILE_NAMES_INFORMATION, *PFILE_NAMES_INFORMATION;

// DIR: FileObjectIdInformation
typedef struct _FILE_OBJECTID_INFORMATION {
    LONG64 FileReference;
    UCHAR ObjectId[16];
    union {
        struct {
            UCHAR BirthVolumeId[16];
            UCHAR BirthObjectId[16];
            UCHAR DomainId[16];
        };
        UCHAR ExtendedInfo[48];
    };
} FILE_OBJECTID_INFORMATION, *PFILE_OBJECTID_INFORMATION;

// DIR: FileReparsePointInformation
typedef struct _FILE_REPARSE_POINT_INFORMATION {
    LONG64 FileReference;
    ULONG Tag;
} FILE_REPARSE_POINT_INFORMATION, *PFILE_REPARSE_POINT_INFORMATION;

//
// Pipe Information
//

// QUERY: FilePipeInformation
typedef struct _FILE_PIPE_INFORMATION {
    ULONG ReadMode;
    ULONG CompletionMode;
} FILE_PIPE_INFORMATION, *PFILE_PIPE_INFORMATION;

// QUERY: FilePipeLocalInformation
typedef struct _FILE_PIPE_LOCAL_INFORMATION {
    ULONG NamedPipeType;
    ULONG NamedPipeConfiguration;
    ULONG MaximumInstances;
    ULONG CurrentInstances;
    ULONG InboundQuota;
    ULONG ReadDataAvailable;
    ULONG OutboundQuota;
    ULONG WriteQuotaAvailable;
    ULONG NamedPipeState;
    ULONG NamedPipeEnd;
} FILE_PIPE_LOCAL_INFORMATION, *PFILE_PIPE_LOCAL_INFORMATION;

// QUERY: FilePipeRemoteInformation
typedef struct _FILE_PIPE_REMOTE_INFORMATION {
    LONG64 CollectDataTime;
    ULONG MaximumCollectionCount;
} FILE_PIPE_REMOTE_INFORMATION, *PFILE_PIPE_REMOTE_INFORMATION;

#if 0
// TODO--Use new IRP_TYPE instead of FileRenameInformation

// SET: FileRenameInformation
typedef struct _FILE_RENAME_INFORMATION {
    BOOLEAN ReplaceIfExists;
    IO_FILE_HANDLE RootDirectory;
    ULONG FileNameLength;
    WCHAR FileName[1];
} FILE_RENAME_INFORMATION, *PFILE_RENAME_INFORMATION;

// TODO--Use new IRP_TYPE instead of FileLinkInformation

// SET: FileLinkInformation (create hardlink)
typedef struct _FILE_LINK_INFORMATION {
    BOOLEAN ReplaceIfExists;
    IO_FILE_HANDLE RootDirectory;
    ULONG FileNameLength;
    WCHAR FileName[1];
} FILE_LINK_INFORMATION, *PFILE_LINK_INFORMATION;

// TODO-EA

// IRP_TYPE_QUERY_EA (IN)
typedef struct _FILE_GET_EA_INFORMATION {
    ULONG NextEntryOffset;
    UCHAR EaNameLength;
    CHAR EaName[1];
} FILE_GET_EA_INFORMATION, *PFILE_GET_EA_INFORMATION;

// TODO-Quota

// IRP_TYPE_QUERY_QUOTA (IN)
typedef struct _FILE_GET_QUOTA_INFORMATION {
    ULONG NextEntryOffset;
    ULONG SidLength;
    BYTE Sid[1];
} FILE_GET_QUOTA_INFORMATION, *PFILE_GET_QUOTA_INFORMATION;

// IRP_TYPE_SET_QUOTA (OUT)
typedef struct _FILE_QUOTA_INFORMATION {
    ULONG NextEntryOffset;
    ULONG SidLength;
    LONG64 ChangeTime;
    LONG64 QuotaUsed;
    LONG64 QuotaThreshold;
    LONG64 QuotaLimit;
    BYTE Sid[1];
} FILE_QUOTA_INFORMATION, *PFILE_QUOTA_INFORMATION;
#endif

typedef ULONG FS_INFORMATION_CLASS, *PFS_INFORMATION_CLASS;

typedef struct _IO_ECP_LIST *PIO_ECP_LIST;

// TODO-Move named pipe ECP stuff to internal header.

#define IO_ECP_TYPE_NAMED_PIPE "Likewise.IO.NamedPipe"

typedef struct _IO_ECP_NAMED_PIPE {
    FILE_PIPE_TYPE_MASK NamedPipeType;
    FILE_PIPE_READ_MODE_MASK ReadMode;
    FILE_PIPE_COMPLETION_MODE_MASK CompletionMode;
    ULONG MaximumInstances;
    ULONG InboundQuota;
    ULONG OutboundQuota;
    LONG64 DefaultTimeout;
    BOOLEAN HaveDefaultTimeout;
} IO_ECP_NAMED_PIPE, *PIO_ECP_NAMED_PIPE;

#ifndef LW_STRICT_NAMESPACE

typedef LW_IO_CONTEXT IO_CONTEXT;
typedef LW_PIO_CONTEXT PIO_CONTEXT;
typedef LW_IO_ACCESS_TOKEN IO_ACCESS_TOKEN;
typedef LW_PIO_ACCESS_TOKEN PIO_ACCESS_TOKEN;

#endif /* ! LW_STRICT_NAMESPACE */

#endif
