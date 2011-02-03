/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

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
 *        structs.h
 *
 * Abstract:
 *
 *        Pvfs Driver internal structures
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#ifndef _PVFS_STRUCTS_H
#define _PVFS_STRUCTS_H

#define PVFS_DEFAULT_SD_RELATIVE_SIZE     512

// File System information

#define PVFS_FS_NAME     "NTFS"
#define PVFS_FILE_SYSTEM_ATTRIBUTES   \
    (FILE_CASE_SENSITIVE_SEARCH |     \
     FILE_CASE_PRESERVED_NAMES |      \
     FILE_UNICODE_ON_DISK |           \
     FILE_PERSISTENT_ACLS |           \
     FILE_SUPPORTS_SPARSE_FILES |     \
     FILE_VOLUME_QUOTAS |             \
     FILE_NAMED_STREAMS)

#define PVFS_NTFS_QUOTA_FILENAME    "$Extend\\$Quota:$Q:$INDEX_ALLOCATION"
#define PVFS_NTFS_QUOTA_FILENAME_W  \
     {'$','E','x','t','e','n','d','\\','$','Q','u','o','t','a',':','$','Q',':',\
     '$','I','N','D','E','X','_','A','L','L','O','C','A','T','I','O','N',0}
#define PVFS_NTFS_C_SHARE_W         {'C','$',0}
#define PVFS_UNIX_QUOTA_FILENAME    ".Quota.sys"

#define PVFS_STREAM_DELIMINATOR_C ':'
#define PVFS_STREAM_DELIMINATOR_S ":"

#define PVFS_STREAM_DEFAULT_TYPE_S "$DATA"
#define PVFS_STREAM_METADATA_DIR_NAME ":STREAM"

typedef DWORD PVFS_STREAM_TYPE, *PPVFS_STREAM_TYPE;

#define PVFS_STREAM_TYPE_UNKNOWN     0
#define PVFS_STREAM_TYPE_DATA        1

typedef struct _PVFS_FILE_NAME
{
    PSTR FileName;
    PSTR StreamName;
    PVFS_STREAM_TYPE Type;
    BOOLEAN ExplicitName;

} PVFS_FILE_NAME, *PPVFS_FILE_NAME;

/* HP-UX does not use blksize_t type for st_blksize
   (see stat(5))
 */
#if !defined(HAVE_BLKSIZE_T)
typedef long blksize_t;
#endif

typedef struct _PVFS_STAT_STRUCT
{
    mode_t       s_mode;
    ino_t        s_ino;
    dev_t        s_dev;
    dev_t        s_rdev;
    nlink_t      s_nlink;
    uid_t        s_uid;
    gid_t        s_gid;
    off_t        s_size;
    off_t        s_alloc;
    time_t       s_atime;
    time_t       s_ctime;
    time_t       s_mtime;
    time_t       s_crtime;     /* creation time */
    blksize_t    s_blksize;
    blkcnt_t     s_blocks;

} PVFS_STAT, *PPVFS_STAT;

typedef struct _PVFS_STATFS_STRUCT
{
    ULONG    BlockSize;
    LONG64   TotalBlocks;
    LONG64   TotalFreeBlocks;
    LONG     MaximumNameLength;

} PVFS_STATFS, *PPVFS_STATFS;

typedef enum _PVFS_INFO_TYPE {
    PVFS_QUERY = 1,
    PVFS_SET
} PVFS_INFO_TYPE, *PPVFS_INFO_TYPE;

typedef struct _PVFS_DIRECTORY_ENTRY
{
    PSTR pszFilename;
    BOOLEAN bValidStat;
    PVFS_STAT Stat;

} PVFS_DIRECTORY_ENTRY, *PPVFS_DIRECTORY_ENTRY;

typedef struct _PVFS_DIRECTORY_CONTEXT
{
    DIR *pDir;
    BOOLEAN bScanned;
    DWORD dwIndex;
    DWORD dwNumEntries;
    ULONG ulAllocated;
    PPVFS_DIRECTORY_ENTRY pDirEntries;

} PVFS_DIRECTORY_CONTEXT, *PPVFS_DIRECTORY_CONTEXT;

typedef struct _PVFS_CCB PVFS_CCB, *PPVFS_CCB;
typedef struct _PVFS_SCB PVFS_SCB, *PPVFS_SCB;
typedef struct _PVFS_FCB PVFS_FCB, *PPVFS_FCB;
typedef struct _PVFS_IRP_CONTEXT PVFS_IRP_CONTEXT, *PPVFS_IRP_CONTEXT;
typedef struct _PVFS_CCB_LIST_NODE PVFS_CCB_LIST_NODE, *PPVFS_CCB_LIST_NODE;
typedef struct _PVFS_OPLOCK_RECORD PVFS_OPLOCK_RECORD, *PPVFS_OPLOCK_RECORD;
typedef struct _PVFS_ZCT_CONTEXT PVFS_ZCT_CONTEXT, *PPVFS_ZCT_CONTEXT;

typedef DWORD PVFS_LOCK_FLAGS;

#define PVFS_LOCK_EXCLUSIVE            0x00000001
#define PVFS_LOCK_BLOCK                0x00000002

typedef DWORD PVFS_OPERATION_TYPE;

#define PVFS_OPERATION_READ            0x00000001
#define PVFS_OPERATION_WRITE           0x00000002

typedef struct _PVFS_LOCK_ENTRY
{
    BOOLEAN bFailImmediately;
    BOOLEAN bExclusive;
    ULONG Key;
    ULONG64 Offset;
    ULONG64 Length;

} PVFS_LOCK_ENTRY, *PPVFS_LOCK_ENTRY;

typedef struct _PVFS_PENDING_LOCK
{
    LW_LIST_LINKS LockList;

    PVFS_LOCK_ENTRY PendingLock;
    PPVFS_CCB pCcb;
    PPVFS_IRP_CONTEXT pIrpContext;

} PVFS_PENDING_LOCK, *PPVFS_PENDING_LOCK;


typedef LONG PVFS_SET_FILE_PROPERTY_FLAGS;

#define PVFS_SET_PROP_NONE      0x00000000
#define PVFS_SET_PROP_SECURITY  0x00000001
#define PVFS_SET_PROP_ATTRIB    0x00000002

typedef struct _PVFS_CB_TABLE_ENTRY
{
    pthread_rwlock_t rwLock;
    pthread_rwlock_t *pRwLock;
    PLWRTL_RB_TREE pTree;

} PVFS_CB_TABLE_ENTRY, *PPVFS_CB_TABLE_ENTRY;

typedef size_t (*PVFS_HASH_KEY)(PCVOID);
typedef void (*PVFS_HASH_FREE_ENTRY)(PPVFS_CB_TABLE_ENTRY*);

typedef struct _PVFS_HASH_TABLE
{
    size_t sTableSize;
    size_t sCount;
    PPVFS_CB_TABLE_ENTRY *ppEntries;
    PVFS_HASH_KEY fnHash;
    PVFS_HASH_FREE_ENTRY fnFree;
} PVFS_HASH_TABLE, *PPVFS_HASH_TABLE;

typedef struct _PVFS_CB_TABLE
{
    pthread_rwlock_t rwLock;
    PPVFS_HASH_TABLE pCbHashTable;

} PVFS_CB_TABLE, *PPVFS_CB_TABLE;

typedef struct _PVFS_PENDING_CREATE
{
    PPVFS_IRP_CONTEXT pIrpContext;
    PPVFS_FILE_NAME OriginalFileName;
    PPVFS_FILE_NAME ResolvedFileName;
    PPVFS_CCB pCcb;
    PPVFS_SCB pScb;
    ACCESS_MASK GrantedAccess;
    BOOLEAN bFileExisted;
    PVFS_SET_FILE_PROPERTY_FLAGS SetPropertyFlags;

} PVFS_PENDING_CREATE, *PPVFS_PENDING_CREATE;

typedef struct _PVFS_PENDING_READ
{
    PPVFS_IRP_CONTEXT pIrpContext;
    PPVFS_CCB pCcb;
    PPVFS_ZCT_CONTEXT pZctContext;

} PVFS_PENDING_READ, *PPVFS_PENDING_READ;


typedef struct _PVFS_PENDING_WRITE
{
    PPVFS_IRP_CONTEXT pIrpContext;
    PPVFS_CCB pCcb;
    PPVFS_ZCT_CONTEXT pZctContext;

} PVFS_PENDING_WRITE, *PPVFS_PENDING_WRITE;


typedef struct _PVFS_PENDING_SET_END_OF_FILE
{
    PPVFS_IRP_CONTEXT pIrpContext;
    PPVFS_CCB pCcb;

} PVFS_PENDING_SET_END_OF_FILE, *PPVFS_PENDING_SET_END_OF_FILE;


typedef struct _PVFS_PENDING_SET_ALLOCATION
{
    PPVFS_IRP_CONTEXT pIrpContext;
    PPVFS_CCB pCcb;

} PVFS_PENDING_SET_ALLOCATION, *PPVFS_PENDING_SET_ALLOCATION;


typedef NTSTATUS (*PPVFS_OPLOCK_PENDING_COMPLETION_CALLBACK)(
    IN PVOID pContext
    );

typedef VOID (*PPVFS_OPLOCK_PENDING_COMPLETION_FREE_CTX)(
    IN PVOID *ppContext
    );

typedef struct _PVFS_PENDING_OPLOCK_BREAK_TEST
{
    PPVFS_SCB pScb;
    PPVFS_IRP_CONTEXT pIrpContext;
    PPVFS_CCB pCcb;
    PPVFS_OPLOCK_PENDING_COMPLETION_CALLBACK pfnCompletion;
    PPVFS_OPLOCK_PENDING_COMPLETION_FREE_CTX pfnFreeContext;
    PVOID pCompletionContext;

} PVFS_PENDING_OPLOCK_BREAK_TEST, *PPVFS_PENDING_OPLOCK_BREAK_TEST;


typedef struct _PVFS_OPLOCK_PENDING_OPERATION
{
    LW_LIST_LINKS PendingOpList;

    PPVFS_IRP_CONTEXT pIrpContext;

    PPVFS_OPLOCK_PENDING_COMPLETION_CALLBACK pfnCompletion;
    PPVFS_OPLOCK_PENDING_COMPLETION_FREE_CTX pfnFreeContext;
    PVOID pCompletionContext;

} PVFS_OPLOCK_PENDING_OPERATION, *PPVFS_OPLOCK_PENDING_OPERATION;

#define PVFS_SCB_MAX_PENDING_LOCKS       50
#define PVFS_SCB_MAX_PENDING_OPERATIONS  50
#define PVFS_SCB_MAX_PENDING_NOTIFY      50

#define PVFS_CLEAR_FILEID(x)   \
    do {                       \
        (x).Device = 0;        \
        (x).Inode = 0;         \
    } while(0)

typedef struct _PVFS_FILE_ID
{
    dev_t Device;
    ino_t Inode;

} PVFS_FILE_ID, *PPVFS_FILE_ID;

typedef struct _PVFS_CONTROL_BLOCK
{
    LONG RefCount;
    PPVFS_CB_TABLE_ENTRY pBucket;
    pthread_mutex_t Mutex;
    pthread_rwlock_t RwLock;
    BOOLEAN Removed;

} PVFS_CONTROL_BLOCK, *PPVFS_CONTROL_BLOCK;

struct _PVFS_SCB
{
    PVFS_CONTROL_BLOCK BaseControlBlock;

    LW_LIST_LINKS FcbList;

    // Base Mutex
    PVFS_FILE_ID FileId;
    BOOLEAN bDeleteOnClose;

    BOOLEAN bOplockBreakInProgress;
    PPVFS_LIST pOplockList;
    PPVFS_LIST pOplockPendingOpsQueue;
    PPVFS_LIST pOplockReadyOpsQueue;
    // End BaseMutex


    // Base.RwLock
    PPVFS_FCB pOwnerFcb;

    PSTR pszStreamname;
    PVFS_STREAM_TYPE StreamType;
    // End Base.RwLock


    /* rwCcbLock */
    pthread_rwlock_t rwCcbLock;     /* For managing the CCB list */
    LONG OpenHandleCount;
    PPVFS_LIST pCcbList;
    /* End rwCcbLock */


    /* rwBrlLock */
    pthread_rwlock_t rwBrlLock;     /* For managing the LockTable in
                                       the CCB list, the pendingLockqueue,
                                       and the LastFailedLock entry */
    PPVFS_LIST pPendingLockQueue;
    /* End rwBrlLock */
};

struct _PVFS_FCB
{
    PVFS_CONTROL_BLOCK BaseControlBlock;

    // BaseMutex
    PVFS_FILE_ID FileId;
    LONG64 LastWriteTime;          /* Saved mode time from SET_FILE_INFO */
    BOOLEAN bDeleteOnClose;

    PPVFS_LIST pNotifyListIrp;
    PPVFS_LIST pNotifyListBuffer;
    // End BaseMutex

    // Base.RwLock
    PPVFS_FCB pParentFcb;
    PSTR pszFilename;
    // End Base.RwLock

    // rwScbLock
    pthread_rwlock_t rwScbLock;     /* For managing the SCB list */
    LONG OpenHandleCount;
    PPVFS_LIST pScbList;
    // End rwScbLock
};

typedef struct _PVFS_LOCK_LIST
{
    ULONG NumberOfLocks;
    ULONG ListSize;
    PPVFS_LOCK_ENTRY pLocks;

} PVFS_LOCK_LIST, *PPVFS_LOCK_LIST;


typedef struct _PVFS_LOCK_TABLE
{
    PVFS_LOCK_LIST ExclusiveLocks;
    PVFS_LOCK_LIST SharedLocks;

} PVFS_LOCK_TABLE, *PPVFS_LOCK_TABLE;

typedef enum _PVFS_OPLOCK_STATE
{
    PVFS_OPLOCK_STATE_NONE = 0,
    PVFS_OPLOCK_STATE_GRANTED,
    PVFS_OPLOCK_STATE_BREAK_IN_PROGRESS

} PVFS_OPLOCK_STATE, *PPVFS_OPLOCK_STATE;


#define PVFS_ECP_ENABLE_ABE      0x00000001

struct _PVFS_CCB
{
    LW_LIST_LINKS ScbList;

    pthread_mutex_t ControlBlock;

    LONG RefCount;
    BOOLEAN bPendingDeleteHandle;
    BOOLEAN bCloseInProgress;

    /* Open fd to the File or Directory */
    int fd;
    PVFS_FILE_ID FileId;

    /* Pointer to the shared PVFS FileHandle */
    PPVFS_SCB pScb;

    /* Save parameters from the CreateFile() */
    PSTR pszFilename;
    PWSTR pwszShareName;
    FILE_CREATE_OPTIONS CreateOptions;
    FILE_SHARE_FLAGS ShareFlags;
    ACCESS_MASK AccessGranted;
    BOOLEAN bQuotaFile;

    PACCESS_TOKEN pUserToken;

    /* Handle for Directory enumeraqtion */
    PPVFS_DIRECTORY_CONTEXT pDirContext;
    DWORD EcpFlags;

    PVFS_LOCK_TABLE LockTable;

    PVFS_OPLOCK_STATE OplockState;
    ULONG OplockBreakResult;

    FILE_NOTIFY_CHANGE ChangeEvent;
    LONG64 FileSize;

    PPVFS_LIST pZctContextList;
};

typedef enum
{
    PVFS_QUEUE_TYPE_NONE = 0,
    PVFS_QUEUE_TYPE_IO,
    PVFS_QUEUE_TYPE_OPLOCK,
    PVFS_QUEUE_TYPE_PENDING_OPLOCK_BREAK,
    PVFS_QUEUE_TYPE_PENDING_LOCK,
    PVFS_QUEUE_TYPE_NOTIFY
} PVFS_QUEUE_TYPE;


#define PVFS_IRP_CTX_FLAG_NONE             0x0000
#define PVFS_IRP_CTX_FLAG_CANCELLED        0x0001
#define PVFS_IRP_CTX_FLAG_PENDED           0x0002
#define PVFS_IRP_CTX_FLAG_ACTIVE           0x0004
#define PVFS_IRP_CTX_FLAG_COMPLETE         0x0008
#define PVFS_IRP_CTX_FLAG_REQUEST_CANCEL   0x0010

struct _PVFS_IRP_CONTEXT
{
    LONG RefCount;

    USHORT Flags;

    PVFS_QUEUE_TYPE QueueType;

    PPVFS_SCB pScb;
    PIRP pIrp;
};

/* Used for Query/Set level handlers */

struct _InfoLevelDispatchEntry {
    FILE_INFORMATION_CLASS Level;
    NTSTATUS (*fn)(PVFS_INFO_TYPE RequestType,
                   PPVFS_IRP_CONTEXT pIrpContext);
};

struct _PVFS_OPLOCK_RECORD
{
    LW_LIST_LINKS OplockList;

    ULONG OplockType;
    PPVFS_CCB pCcb;
    PPVFS_IRP_CONTEXT pIrpContext;

};


typedef NTSTATUS (*PPVFS_WORK_CONTEXT_CALLBACK)(
    IN PVOID pContext
    );

typedef VOID (*PPVFS_WORK_CONTEXT_FREE_CTX)(
    IN PVOID *ppContext
    );

#define PVFS_WORK_CTX_FLAG_IRP_CONTEXT    0x00000001
#define PVFS_WORK_CTX_FLAG_TERMINATE      0x00000002

typedef struct _PVFS_WORK_CONTEXT
{
    LW_LIST_LINKS WorkList;

    LONG Flags;
    PVOID pContext;

    PPVFS_WORK_CONTEXT_CALLBACK pfnCompletion;
    PPVFS_WORK_CONTEXT_FREE_CTX pfnFreeContext;

} PVFS_WORK_CONTEXT, *PPVFS_WORK_CONTEXT;


typedef struct _PVFS_OPEN_FILE_INFO
{
    ULONG Level;
    ULONG BytesAvailable;
    ULONG Offset;
    PVOID pData;
    PVOID pPreviousEntry;

} PVFS_OPEN_FILE_INFO, *PPVFS_OPEN_FILE_INFO;


typedef struct _PVFS_NOTIFY_FILTER_BUFFER
{
    PVOID pData;
    ULONG Length;
    ULONG Offset;
    PFILE_NOTIFY_INFORMATION pNotify;
    NTSTATUS Status;

} PVFS_NOTIFY_FILTER_BUFFER, *PPVFS_NOTIFY_FILTER_BUFFER;

typedef struct _PVFS_NOTIFY_FILTER_RECORD
{
    LW_LIST_LINKS NotifyList;

    PPVFS_IRP_CONTEXT pIrpContext;
    PVFS_NOTIFY_FILTER_BUFFER Buffer;
    PPVFS_CCB pCcb;
    FILE_NOTIFY_CHANGE NotifyFilter;
    BOOLEAN bWatchTree;

} PVFS_NOTIFY_FILTER_RECORD, *PPVFS_NOTIFY_FILTER_RECORD;

typedef struct _PVFS_NOTIFY_REPORT_RECORD
{
    PPVFS_FCB pFcb;
    FILE_NOTIFY_CHANGE Filter;
    FILE_ACTION Action;
    PSTR pszFilename;

} PVFS_NOTIFY_REPORT_RECORD, *PPVFS_NOTIFY_REPORT_RECORD;


/* SID/UID/GID caches */

typedef struct _PVFS_ID_CACHE
{
    union {
        uid_t Uid;
        gid_t Gid;
    } UnixId;
    PSID pSid;
} PVFS_ID_CACHE, *PPVFS_ID_CACHE;

typedef enum {
    PVFS_ZCT_MODE_DISABLED = 0,
    PVFS_ZCT_MODE_MEMORY,
    PVFS_ZCT_MODE_SPLICE
} PVFS_ZCT_MODE, *PPVFS_ZCT_MODE;

typedef struct _PVFS_DRIVER_CONFIG
{
    pthread_rwlock_t rwLock;

    DWORD     CreateFileMode;
    DWORD     CreateDirectoryMode;

    BOOLEAN   EnableOplocks;
    BOOLEAN   EnableDriverDebug;
    PVFS_ZCT_MODE ZctMode;

    uid_t    VirtualUid;
    gid_t    VirtualGid;

    DWORD     WorkerThreadPoolSize;
    DWORD     PathCacheSize;

} PVFS_DRIVER_CONFIG, *PPVFS_DRIVER_CONFIG;


/* ZCT */

struct _PVFS_ZCT_CONTEXT
{
    PVFS_ZCT_MODE Mode;
    PPVFS_CCB pCcb;
    LW_LIST_LINKS CcbLinks;
    union {
        PVOID pBuffer;
#ifdef HAVE_SPLICE
        int PipeFds[2];
#endif
    };
};

/* Quota */

typedef struct _PVFS_QUOTA_ENTRY
{
    ULONG SidLength;
    LONG64 ChangeTime;
    LONG64 QuotaUsed;
    LONG64 QuotaThreshold;
    LONG64 QuotaLimit;
    BYTE Sid[SID_MAX_SIZE];
    struct _PVFS_QUOTA_ENTRY* pNext;
} PVFS_QUOTA_ENTRY, *PPVFS_QUOTA_ENTRY;


#endif    /* _PVFS_STRUCTS_H */


