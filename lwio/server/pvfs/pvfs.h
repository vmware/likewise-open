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
 *        includes
 *
 * Abstract:
 *
 *        Likewise Posix File System (SMBSS)
 *
 *        Service Entry API
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#ifndef __PVFS_H__
#define __PVFS_H__

#include "config.h"

#if defined(HAVE_ATTR_XATTR_H) && \
    defined(HAVE_FSETXATTR) && \
    defined(HAVE_FGETXATTR) && \
    defined(HAVE_GETXATTR)
#  ifdef __LWI_LINUX__
#    define HAVE_EA_SUPPORT
#  endif
#endif


#include <lw/base.h>
#include <lw/security-types.h>

#include "lwiosys.h"
#include "lwio/lwiofsctl.h"
#include "iodriver.h"
#include "lwioutils.h"
#include "lwlist.h"

#include "structs.h"
#include "async_handler.h"
#include "threads.h"
#include "externs.h"
#include "macros.h"
#include "fileinfo_p.h"
#include "security_p.h"
#include "create_p.h"
#include "alloc_p.h"
#include "time_p.h"
#include "syswrap_p.h"
#include "ccb_p.h"
#include "fcb.h"
#include "acl.h"
#include "attrib.h"

/* Unix (POSIX) APIs */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <utime.h>

#ifdef HAVE_SYS_VFS_H
#  include <sys/vfs.h>
#endif

#ifdef HAVE_ATTR_XATTR_H
#  include <attr/xattr.h>
#endif

/* Driver defines */

#define PVFS_WORKERS_NUMBER_THREADS      4
#define PVFS_WORKERS_MAX_WORK_ITEMS      10


/* Top level APi functions */

NTSTATUS
PvfsCreate(
    PPVFS_IRP_CONTEXT  pIrpContext
    );

NTSTATUS
PvfsAllocateCreateContext(
    OUT PPVFS_PENDING_CREATE *ppCreate,
    IN  PPVFS_IRP_CONTEXT pIrpContext
    );

VOID
PvfsFreeCreateContext(
    IN OUT PVOID *ppContext
    );

NTSTATUS
PvfsCreateFileDoSysOpen(
    IN PVOID pContext
    );

NTSTATUS
PvfsDeviceIoControl(
    PPVFS_IRP_CONTEXT  pIrpContext
    );

NTSTATUS
PvfsDispatchFsIoControl(
    PPVFS_IRP_CONTEXT  pIrpContext
    );

NTSTATUS
PvfsWrite(
    PPVFS_IRP_CONTEXT  pIrpContext
    );

NTSTATUS
PvfsRead(
    PPVFS_IRP_CONTEXT  pIrpContext
    );

NTSTATUS
PvfsClose(
    PPVFS_IRP_CONTEXT  pIrpContext
    );

NTSTATUS
PvfsQueryInformationFile(
    PPVFS_IRP_CONTEXT  pIrpContext
    );

NTSTATUS
PvfsSetInformationFile(
    PPVFS_IRP_CONTEXT  pIrpContext
    );

NTSTATUS
PvfsQueryDirInformation(
    PPVFS_IRP_CONTEXT  pIrpContext
    );

NTSTATUS
PvfsQueryVolumeInformation(
    PPVFS_IRP_CONTEXT  pIrpContext
    );

NTSTATUS
PvfsSetVolumeInformation(
    PPVFS_IRP_CONTEXT  pIrpContext
    );

NTSTATUS
PvfsDispatchLockControl(
    PPVFS_IRP_CONTEXT pIrpContext
    );

NTSTATUS
PvfsLockControl(
    PPVFS_IRP_CONTEXT pIrpContext
    );

NTSTATUS
PvfsFlushBuffers(
    PPVFS_IRP_CONTEXT  pIrpContext
    );

NTSTATUS
PvfsQuerySecurityFile(
    PPVFS_IRP_CONTEXT pIrpContext
    );

NTSTATUS
PvfsSetSecurityFile(
    PPVFS_IRP_CONTEXT pIrpContext
    );


/* From errno.c */

NTSTATUS
PvfsMapUnixErrnoToNtStatus(
    int err
    );


/* From unixpath.c */

NTSTATUS
PvfsCanonicalPathName(
    PSTR *ppszPath,
    IO_FILE_NAME IoPath
    );

NTSTATUS
PvfsWC16CanonicalPathName(
    PSTR *ppszPath,
    PWSTR pwszPathname
    );

NTSTATUS
PvfsValidatePath(
    PPVFS_CCB pCcb
    );

NTSTATUS
PvfsFileBasename(
    PSTR *ppszFilename,
    PCSTR pszPath
    );
NTSTATUS
PvfsFileDirname(
    PSTR *ppszDirname,
    PCSTR pszPath
    );

NTSTATUS
PvfsFileSplitPath(
    PSTR *ppszDirname,
    PSTR *ppszBasename,
    PCSTR pszPath
    );

NTSTATUS
PvfsLookupPath(
    PSTR *ppszDiskPath,
    PCSTR pszPath,
    BOOLEAN bCaseSensitive
    );

NTSTATUS
PvfsLookupFile(
    PSTR *ppszDiskPath,
    PCSTR pszDiskDirname,
    PCSTR pszFilename,
    BOOLEAN bCaseSensitive
    );

/* From pathcache.c */

NTSTATUS
PvfsPathCacheAdd(
    IN PCSTR pszResolvedPath
    );


NTSTATUS
PvfsPathCacheLookup(
    OUT PSTR *ppszResolvedPath,
    IN  PCSTR pszOriginalPath
    );


/* From wildcard.c */

BOOLEAN
PvfsWildcardMatch(
    IN PSTR pszPathname,
    IN PSTR pszPattern,
    IN BOOLEAN bCaseSensitive
    );


/* From util_open.c */

NTSTATUS
MapPosixOpenFlags(
    int *unixFlags,
    ACCESS_MASK Access,
    IRP_ARGS_CREATE CreateArgs
    );


/* From string.c */

VOID
PvfsCStringUpper(
	PSTR pszString
	);

/* From sharemode.c */

NTSTATUS
PvfsCheckShareMode(
    IN PSTR pszFilename,
    IN FILE_SHARE_FLAGS ShareAccess,
    IN ACCESS_MASK DesiredAccess,
    OUT PPVFS_FCB *ppFcb
    );

NTSTATUS
PvfsEnforceShareMode(
    IN PPVFS_FCB pFcb,
    IN FILE_SHARE_FLAGS ShareAccess,
    IN ACCESS_MASK DesiredAccess
    );

/* From locking.c */

NTSTATUS
PvfsLockFile(
    PPVFS_IRP_CONTEXT pIrpCtx,
    PPVFS_CCB pCcb,
    ULONG Key,
    LONG64 Offset,
    LONG64 Length,
    PVFS_LOCK_FLAGS Flags
    );

NTSTATUS
PvfsUnlockFile(
    PPVFS_CCB pCcb,
    BOOLEAN bUnlockAll,
    ULONG Key,
    LONG64 Offset,
    LONG64 Length
    );

NTSTATUS
PvfsCheckLockedRegion(
    IN PPVFS_CCB pCcb,
    IN PVFS_OPERATION_TYPE Operation,
    IN ULONG Key,
    IN LONG64 Offset,
    IN ULONG Length
    );

NTSTATUS
PvfsCreateLockContext(
    OUT PPVFS_PENDING_LOCK *ppLockContext,
    IN  PPVFS_IRP_CONTEXT pIrpContext,
    IN  PPVFS_CCB pCcb,
    IN  ULONG Key,
    IN  LONG64 Offset,
    IN  LONG64 Length,
    IN  PVFS_LOCK_FLAGS Flags
    );

VOID
PvfsFreeLockContext(
    OUT PVOID *ppContext
    );

NTSTATUS
PvfsLockFileWithContext(
    PVOID pContext
    );

BOOLEAN
PvfsFileHasOpenByteRangeLocks(
    PPVFS_FCB pFcb
    );


/* From oplock.c */

NTSTATUS
PvfsOplockRequest(
    IN  PPVFS_IRP_CONTEXT pIrpContext,
    IN  PVOID InputBuffer,
    IN  ULONG InputBufferLength,
    OUT PVOID OutputBuffer,
    IN  ULONG OutputBufferLength
    );

NTSTATUS
PvfsOplockBreakAck(
    IN  PPVFS_IRP_CONTEXT pIrpContext,
    IN  PVOID InputBuffer,
    IN  ULONG InputBufferLength,
    OUT PVOID OutputBuffer,
    IN  ULONG OutputBufferLength
    );

NTSTATUS
PvfsOplockBreakIfLocked(
    IN PPVFS_IRP_CONTEXT pIrpContext,
    IN PPVFS_CCB pCcb,
    IN PPVFS_FCB pFcb
    );

NTSTATUS
PvfsOplockPendingBreakIfLocked(
    IN PPVFS_PENDING_OPLOCK_BREAK_TEST pTestContext
    );

NTSTATUS
PvfsCreateOplockBreakTestContext(
    OUT PPVFS_PENDING_OPLOCK_BREAK_TEST *ppTestContext,
    IN  PPVFS_FCB pFcb,
    IN  PPVFS_IRP_CONTEXT pIrpContext,
    IN  PPVFS_CCB pCcb,
    IN  PPVFS_OPLOCK_PENDING_COMPLETION_CALLBACK pfnCompletion,
    IN  PPVFS_OPLOCK_PENDING_COMPLETION_FREE_CTX pfnFreeContext,
    IN  PVOID pCompletionContext
    );

VOID
PvfsFreeOplockBreakTestContext(
    IN OUT PPVFS_PENDING_OPLOCK_BREAK_TEST *ppContext
    );



#endif /* __PVFS_H__ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
