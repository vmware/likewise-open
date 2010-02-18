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

#define PVFS_FS_NAME     "NTFS"


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
    LONG   BlockSize;
    LONG   TotalBlocks;
    LONG   TotalFreeBlocks;
    LONG   MaximumNameLength;

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
typedef struct _PVFS_FCB PVFS_FCB, *PPVFS_FCB;
typedef struct _PVFS_IRP_CONTEXT PVFS_IRP_CONTEXT, *PPVFS_IRP_CONTEXT;
typedef struct _PVFS_CCB_LIST_NODE PVFS_CCB_LIST_NODE, *PPVFS_CCB_LIST_NODE;
typedef struct _PVFS_OPLOCK_RECORD PVFS_OPLOCK_RECORD, *PPVFS_OPLOCK_RECORD;

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
    LONG64 Offset;
    LONG64 Length;

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

typedef struct _PVFS_PENDING_CREATE
{
    PPVFS_IRP_CONTEXT pIrpContext;
    PSTR pszOriginalFilename;
    PSTR pszDiskFilename;
    PPVFS_CCB pCcb;
    PPVFS_FCB pFcb;
    ACCESS_MASK GrantedAccess;
    BOOLEAN bFileExisted;
    PVFS_SET_FILE_PROPERTY_FLAGS SetPropertyFlags;

} PVFS_PENDING_CREATE, *PPVFS_PENDING_CREATE;

typedef struct _PVFS_PENDING_READ
{
    PPVFS_IRP_CONTEXT pIrpContext;
    PPVFS_CCB pCcb;

} PVFS_PENDING_READ, *PPVFS_PENDING_READ;


typedef struct _PVFS_PENDING_WRITE
{
    PPVFS_IRP_CONTEXT pIrpContext;
    PPVFS_CCB pCcb;

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
    PPVFS_FCB pFcb;
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

#define PVFS_FCB_MAX_PENDING_LOCKS       50
#define PVFS_FCB_MAX_PENDING_OPERATIONS  50
#define PVFS_FCB_MAX_PENDING_NOTIFY      50

typedef struct _PVFS_FILE_ID
{
    dev_t Device;
    ino_t Inode;

} PVFS_FILE_ID, *PPVFS_FILE_ID;


struct _PVFS_FCB
{
    LONG RefCount;

    /* ControlBlock */
    pthread_mutex_t ControlBlock;   /* For ensuring atomic operations
                                       on an individual FCB */
    PPVFS_FCB pParentFcb;
    PVFS_FILE_ID FileId;
    LONG64 LastWriteTime;          /* Saved mode time from SET_FILE_INFO */
    BOOLEAN bDeleteOnClose;
    /* End ControlBlock */

    /* rwlockFileName */
    pthread_rwlock_t rwFileName;

    PSTR pszFilename;
    /* End rwlockFileName */

    /* rwCcbLock */
    pthread_rwlock_t rwCcbLock;     /* For managing the CCB list */
    PPVFS_LIST pCcbList;
    /* End rwCcbLock */


    /* rwBrlLock */
    pthread_rwlock_t rwBrlLock;     /* For managing the LockTable in
                                       the CCB list, the pendingLockqueue,
                                       and the LastFailedLock entry */
    PPVFS_LIST pPendingLockQueue;
    /* End rwBrlLock */


    /* mutexOplock */
    pthread_mutex_t  mutexOplock;   /* Managing oplock lists */
    BOOLEAN bOplockBreakInProgress;

    PPVFS_LIST pOplockList;

    PPVFS_LIST pOplockPendingOpsQueue;
    PPVFS_LIST pOplockReadyOpsQueue;
    /* End mutexOplock */

    /* Change Notify */
    pthread_mutex_t mutexNotify;

    PPVFS_LIST pNotifyListIrp;
    PPVFS_LIST pNotifyListBuffer;
    /* Change Notify */
};

typedef struct _PVFS_FCB_TABLE
{
    pthread_rwlock_t rwLock;

    PLWRTL_RB_TREE pFcbTree;

} PVFS_FCB_TABLE;

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

struct _PVFS_CCB
{
    LW_LIST_LINKS FcbList;

    pthread_mutex_t FileMutex;      /* Use for fd buffer operations */
    pthread_mutex_t ControlBlock;   /* Use for CCB SetFileInfo operations */

    LONG RefCount;
    BOOLEAN bPendingDeleteHandle;
    BOOLEAN bCloseInProgress;

    /* Open fd to the File or Directory */
    int fd;
    PVFS_FILE_ID FileId;

    /* Pointer to the shared PVFS FileHandle */
    PPVFS_FCB pFcb;

    /* Save parameters from the CreateFile() */
    PSTR pszFilename;
    FILE_CREATE_OPTIONS CreateOptions;
    FILE_SHARE_FLAGS ShareFlags;
    ACCESS_MASK AccessGranted;

    PACCESS_TOKEN pUserToken;

    /* Handle for Directory enumeraqtion */
    PPVFS_DIRECTORY_CONTEXT pDirContext;

    PVFS_LOCK_TABLE LockTable;

    PVFS_OPLOCK_STATE OplockState;
    ULONG OplockBreakResult;

    FILE_NOTIFY_CHANGE ChangeEvent;
    LONG64 FileSize;
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


struct _PVFS_IRP_CONTEXT
{
    pthread_mutex_t Mutex;
    pthread_cond_t  Event;    /* synchronize point for worker threads */

    BOOLEAN bIsCancelled;
    BOOLEAN bIsPended;
    BOOLEAN bInProgress;
    PVFS_QUEUE_TYPE QueueType;

    PPVFS_FCB pFcb;
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

typedef struct _PVFS_WORK_CONTEXT
{
    LW_LIST_LINKS WorkList;

    BOOLEAN bIsIrpContext;
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


#endif    /* _PVFS_STRUCTS_H */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
