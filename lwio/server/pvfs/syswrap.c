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
 *        syswrap.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        Syscall wrapper functions
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "pvfs.h"


/**********************************************************
 *********************************************************/

static NTSTATUS
CopyUnixStatToPvfsStat(
    PPVFS_STAT pPvfsStat,
    struct stat *pSbuf
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;

    pPvfsStat->s_mode    = pSbuf->st_mode;
    pPvfsStat->s_ino     = pSbuf->st_ino;
    pPvfsStat->s_dev     = pSbuf->st_dev;
    pPvfsStat->s_rdev    = pSbuf->st_rdev;
    pPvfsStat->s_nlink   = pSbuf->st_nlink;
    pPvfsStat->s_uid     = pSbuf->st_uid;
    pPvfsStat->s_gid     = pSbuf->st_gid;
    pPvfsStat->s_size    = pSbuf->st_size;
    pPvfsStat->s_alloc   = pSbuf->st_blocks * pSbuf->st_blksize;
    pPvfsStat->s_atime   = pSbuf->st_atime;
    pPvfsStat->s_ctime   = pSbuf->st_ctime;
    pPvfsStat->s_mtime   = pSbuf->st_mtime;
    pPvfsStat->s_crtime  = pSbuf->st_mtime;
    pPvfsStat->s_blksize = pSbuf->st_blksize;
    pPvfsStat->s_blocks  = pSbuf->st_blocks;

    ntError = STATUS_SUCCESS;

    return ntError;
}

/**********************************************************
 *********************************************************/

NTSTATUS
PvfsSysStat(
	PCSTR pszFilename,
	PPVFS_STAT pStat
	)
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    struct stat sBuf = {0};
    int unixerr = 0;

    if (stat(pszFilename, &sBuf) == -1) {
        PVFS_BAIL_ON_UNIX_ERROR(unixerr, ntError);
    }

    if (pStat) {
        ntError = CopyUnixStatToPvfsStat(pStat, &sBuf);
        BAIL_ON_NT_STATUS(ntError);
    }

    ntError = STATUS_SUCCESS;

cleanup:
    return ntError;

error:
    goto cleanup;
}

/**********************************************************
 *********************************************************/

NTSTATUS
PvfsSysFstat(
    int fd,
	PPVFS_STAT pStat
	)
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    struct stat sBuf = {0};
    int unixerr = 0;

    if (fstat(fd, &sBuf) == -1) {
        PVFS_BAIL_ON_UNIX_ERROR(unixerr, ntError);
    }

    if (pStat) {
        ntError = CopyUnixStatToPvfsStat(pStat, &sBuf);
        BAIL_ON_NT_STATUS(ntError);
    }

    ntError = STATUS_SUCCESS;

cleanup:
    return ntError;

error:
    goto cleanup;
}


/**********************************************************
 *********************************************************/

NTSTATUS
PvfsSysOpen(
    int *pFd,
    PSTR pszFilename,
    int iFlags,
    mode_t Mode
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    int fd = -1;
    int unixerr = 0;

    BAIL_ON_INVALID_PTR(pszFilename, ntError);

    if ((fd = open(pszFilename, iFlags, Mode)) == -1) {
        PVFS_BAIL_ON_UNIX_ERROR(unixerr, ntError);
    }

    *pFd = fd;
    ntError = STATUS_SUCCESS;

cleanup:
    return ntError;

error:
    goto cleanup;
}

/**********************************************************
 *********************************************************/

NTSTATUS
PvfsSysMkDir(
    PSTR pszDirname,
    mode_t mode
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    int unixerr = 0;

    if ((mkdir(pszDirname, mode)) == -1) {
        PVFS_BAIL_ON_UNIX_ERROR(unixerr, ntError);
    }

    ntError = STATUS_SUCCESS;

cleanup:
    return ntError;

error:
    goto cleanup;
}


/**********************************************************
 *********************************************************/

NTSTATUS
PvfsSysOpenDir(
    PCSTR pszDirname,
    DIR **ppDir
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    int unixerr = 0;
    DIR *pDir = NULL;

    if ((pDir = opendir(pszDirname)) == NULL) {
        PVFS_BAIL_ON_UNIX_ERROR(unixerr, ntError);
    }

    *ppDir = pDir;
    ntError = STATUS_SUCCESS;

cleanup:
    return ntError;

error:
    goto cleanup;
}

/**********************************************************
 *********************************************************/

NTSTATUS
PvfsSysDirFd(
    PPVFS_CCB pCcb,
    int *pFd
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    int unixerr = 0;
    int fd = -1;

#ifdef HAVE_DIRFD
    if ((fd = dirfd(pCcb->pDirContext->pDir)) == -1) {
        PVFS_BAIL_ON_UNIX_ERROR(unixerr, ntError);
    }
#else
    if ((fd = open(pCcb->pszFilename, 0, 0)) == -1) {
        PVFS_BAIL_ON_UNIX_ERROR(unixerr, ntError);
    }
#endif

    *pFd = fd;
    ntError = STATUS_SUCCESS;

cleanup:
    return ntError;

error:
    goto cleanup;
}



/**********************************************************
 *********************************************************/

NTSTATUS
PvfsSysReadDir(
    DIR *pDir,
    struct dirent **ppDirEntry
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    int unixerr = 0;
    struct dirent *pDirEntry = NULL;

    if ((pDirEntry = readdir(pDir)) == NULL)
    {
        /* Handle errno a little differently here.  Assume
           we can only get EBADF in the case on an error.
           Otherwise, assume NULL means end-of-file. */

        unixerr = errno;
        if (unixerr == EBADF) {
            ntError = PvfsMapUnixErrnoToNtStatus(unixerr);
            BAIL_ON_NT_STATUS(ntError);
        }
    }

    *ppDirEntry = pDirEntry;
    ntError = STATUS_SUCCESS;

cleanup:
    return ntError;

error:
    goto cleanup;

}

/**********************************************************
 *********************************************************/

NTSTATUS
PvfsSysCloseDir(
    DIR *pDir
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    int unixerr = 0;

    if (closedir(pDir) == -1)
    {
        PVFS_BAIL_ON_UNIX_ERROR(unixerr, ntError);
    }

cleanup:
    return ntError;

error:
    goto cleanup;
}

/**********************************************************
 *********************************************************/

NTSTATUS
PvfsSysClose(
    int fd
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    int unixerr = 0;

    if (close(fd) == -1) {
        PVFS_BAIL_ON_UNIX_ERROR(unixerr, ntError);
    }

cleanup:
    return ntError;

error:
    goto cleanup;
}


/**********************************************************
 *********************************************************/

NTSTATUS
PvfsSysLseek(
    IN int fd,
    IN off_t offset,
    IN int whence,
    OUT OPTIONAL off_t *pNewOffset
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    int unixerr = 0;
    off_t newOffset = 0;

    if ((newOffset = lseek(fd, offset, whence)) == (off_t)-1) {
        PVFS_BAIL_ON_UNIX_ERROR(unixerr, ntError);
    }

    if (pNewOffset) {
        *pNewOffset = newOffset;
    }

cleanup:
    return ntError;

error:
    goto cleanup;
}

/**********************************************************
 *********************************************************/

NTSTATUS
PvfsSysFtruncate(
    int fd,
    off_t offset
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    int unixerr = 0;

    if (ftruncate(fd, offset) == -1) {
        PVFS_BAIL_ON_UNIX_ERROR(unixerr, ntError);
    }

cleanup:
    return ntError;

error:
    goto cleanup;
}

/**********************************************************
 *********************************************************/

NTSTATUS
PvfsSysUtime(
    PSTR pszPathname,
    LONG64 LastWriteTime,
    LONG64 LastAccessTime
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    int unixerr;
    time_t tWrite = 0;
    time_t tAccess = 0;
    struct utimbuf TimeBuf = {0};

    PVFS_ZERO_MEMORY(&TimeBuf);

    if (LastWriteTime != 0) {
        ntError = PvfsWinToUnixTime(&tWrite, LastWriteTime);
        BAIL_ON_NT_STATUS(ntError);
    }

    if (LastAccessTime != 0) {
        ntError = PvfsWinToUnixTime(&tAccess, LastAccessTime);
        BAIL_ON_NT_STATUS(ntError);
    }

    TimeBuf.actime = tAccess;
    TimeBuf.modtime = tWrite;

    if (utime(pszPathname, &TimeBuf) == -1) {
        PVFS_BAIL_ON_UNIX_ERROR(unixerr, ntError);
    }

cleanup:
    return ntError;

error:
    goto cleanup;
}

/**********************************************************
 *********************************************************/

NTSTATUS
PvfsSysFstatFs(
    PPVFS_CCB pCcb,
    PPVFS_STATFS pStatFs
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;

#if defined(HAVE_FSTATFS) && defined(__LWI_LINUX__)
    {
        struct statfs sFsBuf;
        int unixerr = 0;

        if (fstatfs(pCcb->fd, &sFsBuf) == -1)
        {
            PVFS_BAIL_ON_UNIX_ERROR(unixerr, ntError);
        }

        pStatFs->BlockSize         = sFsBuf.f_bsize;
        pStatFs->TotalBlocks       = sFsBuf.f_blocks;
        pStatFs->TotalFreeBlocks   = sFsBuf.f_bavail;
        pStatFs->MaximumNameLength = sFsBuf.f_namelen;

        ntError = STATUS_SUCCESS;
    }
#else
    /* Make up some numbers */
    pStatFs->BlockSize         = 4096;
    pStatFs->TotalBlocks       = 1024*128;
    pStatFs->TotalFreeBlocks   = 1024*64;
    pStatFs->MaximumNameLength = 255;

    ntError = STATUS_SUCCESS;
    BAIL_ON_NT_STATUS(ntError);
#endif

cleanup:
    return ntError;

error:
    goto cleanup;
}

/**********************************************************
 *********************************************************/

NTSTATUS
PvfsSysRemove(
    PSTR pszPath
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    int unixerr = 0;

    if (remove(pszPath) == -1) {
        PVFS_BAIL_ON_UNIX_ERROR(unixerr, ntError);
    }

cleanup:
    return ntError;

error:
    goto cleanup;
}

/**********************************************************
 *********************************************************/

NTSTATUS
PvfsSysRead(
    PPVFS_CCB pCcb,
    PVOID pBuffer,
    ULONG pBufLen,
    PLONG64 pOffset,
    PULONG pBytesRead
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    ssize_t bytesRead = 0;
    int unixerr = 0;

    /* Use pread() if we have an offset, otherwise fall back
       to read() */

    if (pOffset)
    {
        off_t offset = 0;

        bytesRead = pread(pCcb->fd, pBuffer, pBufLen, *pOffset);

        if (bytesRead > 0)
        {
            /* pread() and pwrite() don't update the file offset */
            ntError = PvfsSysLseek(pCcb->fd, bytesRead, SEEK_CUR, &offset);
            BAIL_ON_NT_STATUS(ntError);
        }
    }
    else
    {
        bytesRead = read(pCcb->fd, pBuffer, pBufLen);
    }

    if (bytesRead == -1) {
        PVFS_BAIL_ON_UNIX_ERROR(unixerr, ntError);
    }

    *pBytesRead = (ULONG)bytesRead;
    ntError = STATUS_SUCCESS;


cleanup:
    return ntError;

error:
    goto cleanup;
}

/**********************************************************
 *********************************************************/

NTSTATUS
PvfsSysWrite(
    PPVFS_CCB pCcb,
    PVOID pBuffer,
    ULONG pBufLen,
    PLONG64 pOffset,
    PULONG pBytesWritten
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    ssize_t bytesWritten = 0;
    int unixerr = 0;

    /* Use pwrite() if we have an offset, otherwise fall back
       to write() */

    if (pOffset)
    {
        off_t offset = 0;

        bytesWritten = pwrite(pCcb->fd, pBuffer, pBufLen, *pOffset);

        if (bytesWritten > 0)
        {
            /* pread() and pwrite() don't update the file offset */
            ntError = PvfsSysLseek(pCcb->fd, bytesWritten, SEEK_CUR, &offset);
            BAIL_ON_NT_STATUS(ntError);
        }
    }
    else
    {
         bytesWritten = write(pCcb->fd, pBuffer, pBufLen);
    }

    if (bytesWritten == -1) {
        PVFS_BAIL_ON_UNIX_ERROR(unixerr, ntError);
    }

    *pBytesWritten = (ULONG)bytesWritten;
    ntError = STATUS_SUCCESS;

cleanup:
    return ntError;

error:
    goto cleanup;
}

/**********************************************************
 *********************************************************/

NTSTATUS
PvfsSysChown(
    PPVFS_CCB pCcb,
    uid_t uid,
    gid_t gid
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    int unixerr = 0;

    if (chown(pCcb->pszFilename, uid, gid) == -1) {
        PVFS_BAIL_ON_UNIX_ERROR(unixerr, ntError);
    }

    ntError = STATUS_SUCCESS;

cleanup:
    return ntError;

error:
    goto cleanup;
}


/**********************************************************
 *********************************************************/

NTSTATUS
PvfsSysRename(
    PCSTR pszOldname,
    PCSTR pszNewname
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    int unixerr = 0;

    if (rename(pszOldname, pszNewname) == -1 ) {
        PVFS_BAIL_ON_UNIX_ERROR(unixerr, ntError);
    }

    ntError = STATUS_SUCCESS;

cleanup:
    return ntError;

error:
    goto cleanup;
}

/**********************************************************
 *********************************************************/

NTSTATUS
PvfsSysFsync(
    PPVFS_CCB pCcb
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    int unixerr = 0;

#if defined(HAVE_FDATASYNC)
    if (fdatasync(pCcb->fd) == -1 ) {
        PVFS_BAIL_ON_UNIX_ERROR(unixerr, ntError);
    }
#elif defined(HAVE_FSYNC)
    if (fsync(pCcb->fd) == -1 ) {
        PVFS_BAIL_ON_UNIX_ERROR(unixerr, ntError);
    }
#else
    ntError = STATUS_NOT_SUPPORTED;
    BAIL_ON_NT_STATUS(ntError);
#endif

cleanup:
    return ntError;

error:
    goto cleanup;
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
