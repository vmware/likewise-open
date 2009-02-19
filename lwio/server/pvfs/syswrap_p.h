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
    time_t       s_atime;
    time_t       s_ctime;
    time_t       s_mtime;
    time_t       s_crtime;     /* creation time */
    blksize_t    s_blksize;
    blkcnt_t     s_blocks;

} PVFS_STAT, *PPVFS_STAT;

/* Syscall wrappers */

NTSTATUS
PvfsSysStat(
	PSTR pszFilename,
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


#endif     /* _PVFS_SYSWRAP_P_H */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
