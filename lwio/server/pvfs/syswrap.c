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
 *        syswrap.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        Syscall wrapper functions
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 *          Wei Fu <wfu@likewise.com>
 */

#include "pvfs.h"

/**********************************************************
 *********************************************************/
static
NTSTATUS
PvfsSysOpen(
    int *pFd,
    PSTR pszFilename,
    int iFlags,
    mode_t Mode
    );

static
NTSTATUS
PvfsSysRemove(
    PCSTR pszPath
    );

static
BOOLEAN
PvfsSysIsEmptyDir(
    PSTR pszDirname
    );

static
NTSTATUS
PvfsSysMkDir(
    PSTR pszDirname,
    mode_t mode
    );

static
NTSTATUS
CopyUnixStatToPvfsStat(
    PPVFS_STAT pPvfsStat,
    struct stat *pSbuf
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;

    pPvfsStat->s_mode    = pSbuf->st_mode;
    pPvfsStat->s_ino     = pSbuf->st_ino;
    pPvfsStat->s_dev     = pSbuf->st_dev;
    pPvfsStat->s_rdev    = pSbuf->st_rdev;
    pPvfsStat->s_nlink   = pSbuf->st_nlink;
    pPvfsStat->s_uid     = pSbuf->st_uid;
    pPvfsStat->s_gid     = pSbuf->st_gid;
    pPvfsStat->s_size    = pSbuf->st_size;
    pPvfsStat->s_alloc   = pSbuf->st_blocks * 512;
    pPvfsStat->s_atime   = pSbuf->st_atime;
    pPvfsStat->s_ctime   = pSbuf->st_ctime;
    pPvfsStat->s_mtime   = pSbuf->st_mtime;
    pPvfsStat->s_crtime  = pSbuf->st_mtime;
    pPvfsStat->s_blksize = pSbuf->st_blksize;
    pPvfsStat->s_blocks  = pSbuf->st_blocks;

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
    NTSTATUS ntError = STATUS_SUCCESS;
    struct stat sBuf = {0};
    int unixerr = 0;

    if (stat(pszFilename, &sBuf) == -1) {
        PVFS_BAIL_ON_UNIX_ERROR(unixerr, ntError);
    }

    if (pStat) {
        ntError = CopyUnixStatToPvfsStat(pStat, &sBuf);
        BAIL_ON_NT_STATUS(ntError);
    }

cleanup:
    return ntError;

error:
    goto cleanup;
}

////////////////////////////////////////////////////////////////////////

NTSTATUS
PvfsSysStatByFileName(
    IN PPVFS_FILE_NAME FileName,
	IN OUT PPVFS_STAT Stat
	)
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PSTR fileName = NULL;

    ntError = PvfsLookupStreamDiskFileName(&fileName, FileName);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsSysStat(fileName, Stat);
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    if (fileName)
    {
        LwRtlCStringFree(&fileName);
    }

    return ntError;

error:
    goto cleanup;
}

////////////////////////////////////////////////////////////////////////

NTSTATUS
PvfsSysStatByFcb(
    IN PPVFS_FCB pFcb,
    IN OUT PPVFS_STAT pStat
    )
{
    return PvfsSysStat(pFcb->pszFilename, pStat);
}

/**********************************************************
 *********************************************************/

NTSTATUS
PvfsSysFstat(
    int fd,
	PPVFS_STAT pStat
	)
{
    NTSTATUS ntError = STATUS_SUCCESS;
    struct stat sBuf = {0};
    int unixerr = 0;

    if (fstat(fd, &sBuf) == -1) {
        PVFS_BAIL_ON_UNIX_ERROR(unixerr, ntError);
    }

    if (pStat) {
        ntError = CopyUnixStatToPvfsStat(pStat, &sBuf);
        BAIL_ON_NT_STATUS(ntError);
    }

cleanup:
    return ntError;

error:
    goto cleanup;
}


/**********************************************************
 *********************************************************/

static
NTSTATUS
PvfsSysOpen(
    int *pFd,
    PSTR pszFilename,
    int iFlags,
    mode_t Mode
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    int fd = -1;
    int unixerr = 0;

    BAIL_ON_INVALID_PTR(pszFilename, ntError);

    if ((fd = open(pszFilename, iFlags, Mode)) == -1) {
        PVFS_BAIL_ON_UNIX_ERROR(unixerr, ntError);
    }

    *pFd = fd;

cleanup:
    return ntError;

error:
    PvfsSysClose(fd);

    goto cleanup;
}

/**********************************************************
 *********************************************************/

NTSTATUS
PvfsSysOpenByFileName(
    OUT int *pFd,
    OUT PBOOLEAN pbCreateOwnerFile,
    IN PPVFS_FILE_NAME pFileName,
    IN int iFlags,
    IN mode_t Mode
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    int fd = -1;
    int unixerr = 0;
    PSTR pszFilename = NULL;
    PVFS_STAT Stat = {0};
    int Ownerfd = -1;
    PSTR pszStreamDirectoryName = NULL;
    PSTR pszMetadataPath = NULL;
    BOOLEAN bCreateOwnerFile = FALSE;

    // Need make sure the object (file/directory) exists
    // Before non-default stream objects can be created
    if (!PvfsIsDefaultStreamName(pFileName))
    {
        ntError = PvfsSysStat(pFileName->FileName, &Stat);
        if (LW_STATUS_OBJECT_NAME_NOT_FOUND == ntError)
        {
            ntError = PvfsSysOpen(
                      &Ownerfd,
                      pFileName->FileName,
                      iFlags,
                      Mode);
            if (STATUS_SUCCESS == ntError)
            {
                bCreateOwnerFile = TRUE;
            }
        }
        BAIL_ON_NT_STATUS(ntError);

        ntError = PvfsLookupStreamDirectoryPath(&pszStreamDirectoryName,
                                                pFileName);
        BAIL_ON_NT_STATUS(ntError);

        ntError = PvfsFileDirname(&pszMetadataPath,
                                  pszStreamDirectoryName);
        BAIL_ON_NT_STATUS(ntError);

        ntError = PvfsSysOpenDir(pszMetadataPath, NULL);
        if (LW_STATUS_OBJECT_NAME_NOT_FOUND == ntError)
        {
            // create meta data directory
            ntError = PvfsSysMkDir(
                      pszMetadataPath,
                      (mode_t)gPvfsDriverConfig.CreateDirectoryMode);
            BAIL_ON_NT_STATUS(ntError);
        }
        BAIL_ON_NT_STATUS(ntError);

        ntError = PvfsSysOpenDir(pszStreamDirectoryName, NULL);
        if (LW_STATUS_OBJECT_NAME_NOT_FOUND == ntError)
        {
            // create stream directory for an object
            ntError = PvfsSysMkDir(
                          pszStreamDirectoryName,
                          (mode_t)gPvfsDriverConfig.CreateDirectoryMode);
            BAIL_ON_NT_STATUS(ntError);
        }
        BAIL_ON_NT_STATUS(ntError);

        ntError = LwRtlCStringAllocatePrintf(
                      &pszFilename,
                      "%s/%s",
                      pszStreamDirectoryName,
                      PvfsGetCStringBaseStreamName(pFileName));
        BAIL_ON_NT_STATUS(ntError);
    }
    else
    {
        ntError = LwRtlCStringDuplicate(&pszFilename,
                                        PvfsGetCStringBaseFileName(pFileName));
        BAIL_ON_NT_STATUS(ntError);
    }

    if ((fd = open(pszFilename, iFlags, Mode)) == -1) {
        PVFS_BAIL_ON_UNIX_ERROR(unixerr, ntError);
    }

    *pFd = fd;
    *pbCreateOwnerFile = bCreateOwnerFile;

cleanup:

    if (pszFilename)
    {
        LwRtlCStringFree(&pszFilename);
    }
    if (pszStreamDirectoryName)
    {
        LwRtlCStringFree(&pszStreamDirectoryName);
    }
    if (pszMetadataPath)
    {
        LwRtlCStringFree(&pszMetadataPath);
    }

    PvfsSysClose(Ownerfd);

    return ntError;

error:
    PvfsSysClose(fd);

    goto cleanup;
}


/**********************************************************
 *********************************************************/
static
NTSTATUS
PvfsSysMkDir(
    PSTR pszDirname,
    mode_t mode
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    int unixerr = 0;

    if ((mkdir(pszDirname, mode)) == -1) {
        PVFS_BAIL_ON_UNIX_ERROR(unixerr, ntError);
    }

cleanup:
    return ntError;

error:
    goto cleanup;
}


////////////////////////////////////////////////////////////////////////

NTSTATUS
PvfsSysMkDirByFileName(
    IN PPVFS_FILE_NAME DirectoryName,
    mode_t mode
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PSTR directoryName = NULL;

    PVFS_BAIL_ON_INVALID_FILENAME(DirectoryName, ntError);

    // Data stream is not directory
    if (!PvfsIsDefaultStreamName(DirectoryName))
    {
        ntError = STATUS_NOT_A_DIRECTORY;
        BAIL_ON_NT_STATUS(ntError);
    }

    ntError = PvfsLookupStreamDiskFileName(&directoryName, DirectoryName);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsSysMkDir(directoryName, mode);
    BAIL_ON_NT_STATUS(ntError);

error:
    if (directoryName)
    {
        LwRtlCStringFree(&directoryName);
    }

    return ntError;
}

/**********************************************************
 *********************************************************/

NTSTATUS
PvfsSysOpenDir(
    IN PCSTR pszDirname,
    OUT OPTIONAL DIR **ppDir
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    int unixerr = 0;
    DIR *pDir = NULL;

    if ((pDir = opendir(pszDirname)) == NULL) {
        PVFS_BAIL_ON_UNIX_ERROR(unixerr, ntError);
    }

error:
    if (ntError || !ppDir)
    {
        if (pDir)
        {
            PvfsSysCloseDir(pDir);
        }
    }

    if (ppDir)
    {
        *ppDir = pDir;
    }

    return ntError;
}

/**********************************************************
 *********************************************************/

NTSTATUS
PvfsSysReadDir(
    DIR *pDir,
    struct dirent *pDirEntry,
    struct dirent **ppDirEntry
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    int unixerr = 0;

    unixerr = readdir_r(pDir, pDirEntry, ppDirEntry);
    if (unixerr != 0)
    {
        ntError = LwErrnoToNtStatus(unixerr);
    }
    BAIL_ON_NT_STATUS(ntError);

error:
    return ntError;
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

////////////////////////////////////////////////////////////////////////

NTSTATUS
PvfsSysUtimeByFcb(
    IN PPVFS_FCB pFcb,
    IN LONG64 LastWriteTime,
    IN LONG64 LastAccessTime
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    int unixerr;
    time_t tWrite = 0;
    time_t tAccess = 0;
    struct utimbuf TimeBuf = {0};

    if (LastWriteTime != 0)
    {
        ntError = PvfsWinToUnixTime(&tWrite, LastWriteTime);
        BAIL_ON_NT_STATUS(ntError);
    }

    if (LastAccessTime != 0)
    {
        ntError = PvfsWinToUnixTime(&tAccess, LastAccessTime);
        BAIL_ON_NT_STATUS(ntError);
    }

    TimeBuf.actime = tAccess;
    TimeBuf.modtime = tWrite;

    if (utime(pFcb->pszFilename, &TimeBuf) == -1)
    {
        PVFS_BAIL_ON_UNIX_ERROR(unixerr, ntError);
    }

error:
    return ntError;
}

/**********************************************************
 *********************************************************/

NTSTATUS
PvfsSysFstatFs(
    PPVFS_CCB pCcb,
    PPVFS_STATFS pStatFs
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;

#if defined(HAVE_FSTATFS) && defined(__LWI_LINUX__)
    {
        struct statfs sFsBuf;
        int unixerr = 0;

        if (fstatfs(pCcb->fd, &sFsBuf) == -1)
        {
            PVFS_BAIL_ON_UNIX_ERROR(unixerr, ntError);
        }

        pStatFs->BlockSize         =
                sFsBuf.f_bsize > UINT32_MAX ? UINT32_MAX : sFsBuf.f_bsize;
        pStatFs->TotalBlocks       = sFsBuf.f_blocks;
        pStatFs->TotalFreeBlocks   = sFsBuf.f_bavail;
        pStatFs->MaximumNameLength =
                sFsBuf.f_namelen > INT32_MAX ? INT32_MAX : sFsBuf.f_namelen;

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
static
BOOLEAN
PvfsSysIsEmptyDir(
    PSTR pszDirname
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    BOOLEAN isEmptyDir = TRUE;
    DIR *pDir = NULL;
    struct dirent *pDirEntry = NULL;
    struct dirent dirEntry = { 0 };

    ntError = PvfsSysOpenDir(pszDirname, &pDir);
    BAIL_ON_NT_STATUS(ntError);

    for (ntError = PvfsSysReadDir(pDir, &dirEntry, &pDirEntry);
         pDirEntry;
         ntError = PvfsSysReadDir(pDir, &dirEntry, &pDirEntry))
    {
        /* First check the error return */
        BAIL_ON_NT_STATUS(ntError);

        if (!LwRtlCStringIsEqual(pDirEntry->d_name, ".", FALSE) &&
            !LwRtlCStringIsEqual(pDirEntry->d_name, "..", FALSE))
        {
            isEmptyDir = FALSE;
            break;
        }
    }

error:
    if (pDir)
    {
        PvfsSysCloseDir(pDir);
    }

    return NT_SUCCESS(ntError) ? isEmptyDir : FALSE;
}

/**********************************************************
 *********************************************************/

static
NTSTATUS
PvfsSysRemoveDir(
    PSTR pszDirname
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    DIR *pDir = NULL;
    struct dirent *pDirEntry = NULL;
    struct dirent dirEntry = { 0 };
    PSTR pszPath = NULL;
    PVFS_STAT streamDirStat = { 0 };

    ntError = PvfsSysOpenDir(pszDirname, &pDir);
    BAIL_ON_NT_STATUS(ntError);

    for (ntError = PvfsSysReadDir(pDir, &dirEntry, &pDirEntry);
         pDirEntry;
         ntError = PvfsSysReadDir(pDir, &dirEntry, &pDirEntry))
    {
        /* First check the error return */
        BAIL_ON_NT_STATUS(ntError);

        memset(&streamDirStat, 0, sizeof(PVFS_STAT));
        if (pszPath)
        {
            LwRtlCStringFree(&pszPath);
        }

        ntError = LwRtlCStringAllocatePrintf(
                          &pszPath,
                          "%s/%s",
                          pszDirname,
                          pDirEntry->d_name);
        BAIL_ON_NT_STATUS(ntError);

        ntError = PvfsSysStat(pszPath, &streamDirStat);
        if (ntError == STATUS_SUCCESS)
        {
            if(S_ISDIR(streamDirStat.s_mode))
            {
                if (!LwRtlCStringIsEqual(pDirEntry->d_name, ".", FALSE) &&
                    !LwRtlCStringIsEqual(pDirEntry->d_name, "..", FALSE))
                {
                    ntError = PvfsSysRemoveDir(pszPath);
                    BAIL_ON_NT_STATUS(ntError);
                }
            }
            else
            {
                ntError = PvfsSysRemove(pszPath);
                BAIL_ON_NT_STATUS(ntError);
            }
        }
        else
        {
            // Squash the error is the stream directory did not exist
            ntError = STATUS_SUCCESS;
        }
    }

    ntError = PvfsSysCloseDir(pDir);
    BAIL_ON_NT_STATUS(ntError);
    pDir = NULL;

    ntError = PvfsSysRemove(pszDirname);
    BAIL_ON_NT_STATUS(ntError);

error:
    if (pszPath)
    {
        LwRtlCStringFree(&pszPath);
    }

    if (pDir)
    {
        PvfsSysCloseDir(pDir);
    }

    return ntError;
}

static
NTSTATUS
PvfsSysRemove(
    PCSTR pszPath
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    int unixerr = 0;

    if (remove(pszPath) == -1) {
        PVFS_BAIL_ON_UNIX_ERROR(unixerr, ntError);
    }

cleanup:
    return ntError;

error:
    goto cleanup;
}

////////////////////////////////////////////////////////////////////////

NTSTATUS
PvfsSysRemoveByFileName(
    IN PPVFS_FILE_NAME FileName
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PSTR fileName = NULL;
    PSTR streamDir = NULL;
    PVFS_STAT streamDirStat = { 0 };
    PVFS_STAT fileStat = { 0 };
    PSTR pszMetaDataPath = NULL;

    ntError = PvfsLookupStreamDiskFileName(&fileName, FileName);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsSysStat(fileName, &fileStat);
    if (ntError == STATUS_SUCCESS)
    {
        if(S_ISDIR(fileStat.s_mode))
        {
            ntError = LwRtlCStringAllocatePrintf(
                          &pszMetaDataPath,
                          "%s/%s",
                          fileName,
                          PVFS_STREAM_METADATA_DIR_NAME);
            BAIL_ON_NT_STATUS(ntError);

            if (PvfsSysIsEmptyDir(pszMetaDataPath))
            {
                ntError = PvfsSysRemove(pszMetaDataPath);
                BAIL_ON_NT_STATUS(ntError);
            }
        }
    }
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsSysRemove(fileName);
    BAIL_ON_NT_STATUS(ntError);

    if (PvfsIsDefaultStreamName(FileName))
    {
        // Have to remove the stream directory as well

        ntError = PvfsLookupStreamDirectoryPath(&streamDir, FileName);
        BAIL_ON_NT_STATUS(ntError);

        ntError = PvfsSysStat(streamDir, &streamDirStat);
        if (ntError == STATUS_SUCCESS)
        {
            ntError = PvfsSysRemoveDir(streamDir) ;
            BAIL_ON_NT_STATUS(ntError);
        }
        else
        {
            // Squash the error is the stream directory did not exist
            ntError = STATUS_SUCCESS;
        }
        BAIL_ON_NT_STATUS(ntError);
    }

error:
    if (fileName)
    {
        LwRtlCStringFree(&fileName);
    }
    if (streamDir)
    {
        LwRtlCStringFree(&streamDir);
    }
    if (pszMetaDataPath)
    {
        LwRtlCStringFree(&pszMetaDataPath);
    }

    return ntError;
}

/**********************************************************
 *********************************************************/

NTSTATUS
PvfsSysRead(
    PPVFS_CCB pCcb,
    PVOID pBuffer,
    ULONG pBufLen,
    PULONG64 pOffset,
    PULONG pBytesRead
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    ssize_t bytesRead = 0;
    int unixerr = 0;

    /* Use pread() if we have an offset, otherwise fall back
       to read() */

    if (pOffset)
    {
        off_t offset = 0;

        bytesRead = pread(pCcb->fd, pBuffer, pBufLen, (off_t)*pOffset);

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
    PULONG64 pOffset,
    PULONG pBytesWritten
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    ssize_t bytesWritten = 0;
    int unixerr = 0;

    /* Use pwrite() if we have an offset, otherwise fall back
       to write() */

    if (pOffset)
    {
        off_t offset = 0;

        bytesWritten = pwrite(pCcb->fd, pBuffer, pBufLen, (off_t)*pOffset);

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

cleanup:
    return ntError;

error:
    goto cleanup;
}

////////////////////////////////////////////////////////////////////////

NTSTATUS
PvfsSysChownByFileName(
    IN PPVFS_FILE_NAME pFileName,
    uid_t uid,
    gid_t gid
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    int unixerr = 0;
    PSTR pszFilename = NULL;

    ntError = PvfsLookupStreamDiskFileName(&pszFilename, pFileName);
    BAIL_ON_NT_STATUS(ntError);

    if (chown(pszFilename, uid, gid) == -1)
    {
        PVFS_BAIL_ON_UNIX_ERROR(unixerr, ntError);
    }

error:
    if (pszFilename)
    {
        LwRtlCStringFree(&pszFilename);
    }

    return ntError;
}


/**********************************************************
 *********************************************************/

NTSTATUS
PvfsSysFchmod(
    PPVFS_CCB pCcb,
    mode_t Mode
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    int unixerr = 0;

    if (fchmod(pCcb->fd, Mode) == -1) {
        PVFS_BAIL_ON_UNIX_ERROR(unixerr, ntError);
    }

cleanup:
    return ntError;

error:
    goto cleanup;
}

////////////////////////////////////////////////////////////////////////

NTSTATUS
PvfsSysRenameByFileName(
    IN PPVFS_FILE_NAME OriginalFileName,
    IN PPVFS_FILE_NAME NewFileName
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    int unixerr = 0;
    PSTR oldFilename = NULL;
    PSTR newFilename = NULL;
    PSTR oldStreamDir = NULL;
    PSTR newStreamDir = NULL;
    PSTR newStreamDirParent = NULL;
    PVFS_STAT streamDirStat = { 0 };

    ntError = PvfsLookupStreamDiskFileName(&oldFilename, OriginalFileName);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsLookupStreamDiskFileName(&newFilename, NewFileName);
    BAIL_ON_NT_STATUS(ntError);

    if (rename(oldFilename, newFilename) == -1 )
    {
        PVFS_BAIL_ON_UNIX_ERROR(unixerr, ntError);
    }

    if (PvfsIsDefaultStreamName(OriginalFileName))
    {
        // Have to rename the stream directory as well

        ntError = PvfsLookupStreamDirectoryPath(&oldStreamDir, OriginalFileName);
        BAIL_ON_NT_STATUS(ntError);

        ntError = PvfsLookupStreamDirectoryPath(&newStreamDir, NewFileName);
        BAIL_ON_NT_STATUS(ntError);

        ntError = PvfsSysStat(oldStreamDir, &streamDirStat);
        if (ntError == STATUS_SUCCESS)
        {
            ntError = PvfsFileDirname(&newStreamDirParent, newStreamDir);
            BAIL_ON_NT_STATUS(ntError);

            LwRtlZeroMemory(&streamDirStat, sizeof(streamDirStat));

            ntError = PvfsSysStat(newStreamDirParent, &streamDirStat);
            switch (ntError)
            {
                case STATUS_SUCCESS:
                    if (!S_ISDIR(streamDirStat.s_mode))
                    {
                        ntError = STATUS_NOT_A_DIRECTORY;
                    }
                    break;

                case STATUS_OBJECT_NAME_NOT_FOUND:
                    ntError = PvfsSysMkDir(
                                  newStreamDirParent,
                                  (mode_t)gPvfsDriverConfig.CreateDirectoryMode);
                    break;

                default:
                    break;
            }
            BAIL_ON_NT_STATUS(ntError);

            if (rename(oldStreamDir, newStreamDir) == -1)
            {
                // We are now in an inconstsent state -- What should we do?
                // We cannot safely roll back to the original name
                // as someone else may have it.  Let's BAIL for now although
                // in the future we may decide that this should just log an error
                // but not fail
                PVFS_BAIL_ON_UNIX_ERROR(unixerr, ntError);
            }
        }

        // Squash the error is the stream directory did not exist
        ntError = STATUS_SUCCESS;
    }

error:
    if (oldFilename)
    {
        LwRtlCStringFree(&oldFilename);
    }
    if (newFilename)
    {
        LwRtlCStringFree(&newFilename);
    }
    if (oldStreamDir)
    {
        LwRtlCStringFree(&oldStreamDir);
    }
    if (newStreamDir)
    {
        LwRtlCStringFree(&newStreamDir);
    }
    if (newStreamDirParent)
    {
        LwRtlCStringFree(&newStreamDirParent);
    }


    return ntError;
}

/**********************************************************
 *********************************************************/

NTSTATUS
PvfsSysLinkByFileName(
    IN PPVFS_FILE_NAME pOldname,
    IN PPVFS_FILE_NAME pNewname
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    int unixerr = 0;

    if (link(pOldname->FileName, pNewname->FileName) == -1 ) {
        PVFS_BAIL_ON_UNIX_ERROR(unixerr, ntError);
    }

cleanup:
    return ntError;

error:
    goto cleanup;
}

/**********************************************************
 *********************************************************/

#define PVFS_DISABLE_FSYNC   1

NTSTATUS
PvfsSysFsync(
    PPVFS_CCB pCcb
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    int unixerr = 0;

#ifdef PVFS_DISABLE_FSYNC
    ntError = STATUS_SUCCESS;
    unixerr = 0;
    BAIL_ON_NT_STATUS(ntError);
#elif defined(HAVE_FDATASYNC)
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


/**********************************************************
 *********************************************************/

NTSTATUS
PvfsSysNanoSleep(
    const struct timespec *pRequestedTime,
    struct timespec *pRemainingTime
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    int unixerr = 0;

    if (nanosleep(pRequestedTime, pRemainingTime) == -1)
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
PvfsSysPipe(
    int PipeFds[2]
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    int unixerr = 0;

    if (pipe(PipeFds) == -1)
    {
        PVFS_BAIL_ON_UNIX_ERROR(unixerr, ntError);
    }

cleanup:
    return ntError;

error:
    PipeFds[0] = -1;
    PipeFds[1] = -1;

    goto cleanup;
}

/**********************************************************
 *********************************************************/

NTSTATUS
PvfsSysSetNonBlocking(
    int Fd
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    int unixerr = 0;

    if (fcntl(Fd, F_SETFL, O_NONBLOCK) == -1)
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
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    long result = 0;
    int unixerr = 0;

    result = splice(FromFd, pFromOffset, ToFd, pToOffset, Length, Flags);
    if (result == -1)
    {
        PVFS_BAIL_ON_UNIX_ERROR(unixerr, ntError);
    }

cleanup:
    *pBytesSpliced = result;

    return ntError;

error:
    result = 0;
    goto cleanup;
}
#endif
