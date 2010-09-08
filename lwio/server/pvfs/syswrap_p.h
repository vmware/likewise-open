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
 *        syswrap_p.h
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        syscall wrappers
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */


#ifndef _PVFS_SYSWRAP_P_H
#define _PVFS_SYSWRAP_P_H

#include "config.h"

#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>


/* Syscall wrappers */

NTSTATUS
PvfsSysStat(
	PCSTR pszFilename,
	PPVFS_STAT pStat
	);

NTSTATUS
PvfsSysFstat(
	int fd,
	PPVFS_STAT pStat
	);

NTSTATUS
PvfsSysOpen(
    int *pFd,
    PSTR pszFilename,
    int iFlags,
    mode_t Mode
    );

NTSTATUS
PvfsSysClose(
    int fd
    );


NTSTATUS
PvfsSysMkDir(
    PSTR pszDirname,
    mode_t mode
    );

NTSTATUS
PvfsSysOpenDir(
    PCSTR pszDirname,
    DIR **ppDir
    );

NTSTATUS
PvfsSysDirFd(
    PPVFS_CCB pCcb,
    int *pFd
    );

NTSTATUS
PvfsSysReadDir(
    DIR *pDir,
    struct dirent *pDirEntry,
    struct dirent **ppDirEntry
    );

NTSTATUS
PvfsSysCloseDir(
    DIR *pDir
    );

NTSTATUS
PvfsSysLseek(
    int fd,
    off_t offset,
    int whence,
    off_t *pNewOffset
    );

NTSTATUS
PvfsSysFtruncate(
    int fd,
    off_t offset
    );


NTSTATUS
PvfsSysUtime(
    PSTR pszPathname,
    LONG64 LastWriteTime,
    LONG64 LastAccessTime
    );

NTSTATUS
PvfsSysFstatFs(
    PPVFS_CCB pCcb,
    PPVFS_STATFS pStatFs
    );

NTSTATUS
PvfsSysRemove(
    PCSTR pszPath
    );

NTSTATUS
PvfsSysRead(
    PPVFS_CCB pCcb,
    PVOID pBuffer,
    ULONG pBufLen,
    PLONG64 pOffset,
    PULONG pBytesRead
    );

NTSTATUS
PvfsSysWrite(
    PPVFS_CCB pCcb,
    PVOID pBuffer,
    ULONG pBufLen,
    PLONG64 pOffset,
    PULONG pBytesWritten
    );

NTSTATUS
PvfsSysChown(
    PPVFS_CCB pCcb,
    uid_t uid,
    gid_t gid
    );

NTSTATUS
PvfsSysFchmod(
    PPVFS_CCB pCcb,
    mode_t Mode
    );

NTSTATUS
PvfsSysRename(
    PCSTR pszOldname,
    PCSTR pszNewname
    );

NTSTATUS
PvfsSysFsync(
    PPVFS_CCB pCcb
    );

NTSTATUS
PvfsSysNanoSleep(
    const struct timespec *pRequestedTime,
    struct timespec *pRemainingTime
    );

NTSTATUS
PvfsSysPipe(
    int PipeFds[2]
    );

NTSTATUS
PvfsSysSetNonBlocking(
    int Fd
    );

#ifdef HAVE_SPLICE
NTSTATUS
PvfsSysSplice(
    int FromFd,
    PLONG64 pFromOffset,
    int ToFd,
    PLONG64 pToOffset,
    ULONG Length,
    unsigned int Flags,
    PULONG pBytesSpliced
    );
#endif

#endif     /* _PVFS_SYSWRAP_P_H */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
