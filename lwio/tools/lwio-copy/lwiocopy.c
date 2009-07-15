/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
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
 *        lwiocopy.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO)
 *
 *        Tool to copy files/directories
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "includes.h"

static
BOOLEAN
IsPathRemote(
    PCSTR pszPath
    );

static
NTSTATUS
CopyFile_RemoteToRemote(
    PCSTR   pszSrcPath,
    PCSTR   pszDestPath,
    BOOLEAN bCopyRecursive
    );

static
NTSTATUS
CopyFile_RemoteToLocal(
    PCSTR   pszSrcPath,
    PCSTR   pszDestPath,
    BOOLEAN bCopyRecursive
    );

static
NTSTATUS
CopyFile_LocalToRemote(
    PCSTR   pszSrcPath,
    PCSTR   pszDestPath,
    BOOLEAN bCopyRecursive
    );

static
NTSTATUS
CopyFile_LocalToLocal(
    PCSTR   pszSrcPath,
    PCSTR   pszDestPath,
    BOOLEAN bCopyRecursive
    );

NTSTATUS
CopyFile(
    PCSTR   pszSrcPath,
    PCSTR   pszDestPath,
    BOOLEAN bCopyRecursive
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (IsPathRemote(pszSrcPath))
    {
        if (IsPathRemote(pszDestPath))
        {
            // Remote to Remote
            ntStatus = CopyFile_RemoteToRemote(
                            pszSrcPath,
                            pszDestPath,
                            bCopyRecursive);
        }
        else
        {
            // Remote to Local
            ntStatus = CopyFile_RemoteToLocal(
                            pszSrcPath,
                            pszDestPath,
                            bCopyRecursive);
        }
    }
    else
    {
        if (IsPathRemote(pszDestPath))
        {
            // Local to Remote
            ntStatus = CopyFile_LocalToRemote(
                            pszSrcPath,
                            pszDestPath,
                            bCopyRecursive);
        }
        else
        {
            // Local to Local
            ntStatus = CopyFile_LocalToLocal(
                            pszSrcPath,
                            pszDestPath,
                            bCopyRecursive);
        }
    }

    return ntStatus;
}

static
BOOLEAN
IsPathRemote(
    PCSTR pszPath
    )
{
    BOOLEAN bIsRemote = FALSE;

    if (!strncmp(pszPath, "//", sizeof("//")-1) ||
        !strncmp(pszPath, "\\\\", sizeof("\\\\")-1))
    {
        bIsRemote = TRUE;
    }

    return bIsRemote;
}

static
NTSTATUS
CopyFile_RemoteToRemote(
    PCSTR   pszSrcPath,
    PCSTR   pszDestPath,
    BOOLEAN bCopyRecursive
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

static
NTSTATUS
CopyFile_RemoteToLocal(
    PCSTR   pszSrcPath,
    PCSTR   pszDestPath,
    BOOLEAN bCopyRecursive
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN bIsDirectory = FALSE;

    ntStatus = LwioCheckRemotePathIsDirectory(pszSrcPath, &bIsDirectory);
    BAIL_ON_NT_STATUS(ntStatus);

    if (bIsDirectory)
    {
        ntStatus = LwioCopyDirFromRemote(pszSrcPath, pszDestPath);
    }
    else
    {
        ntStatus = LwioCopyFileFromRemote(pszSrcPath, pszDestPath);
    }
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
CopyFile_LocalToRemote(
    PCSTR   pszSrcPath,
    PCSTR   pszDestPath,
    BOOLEAN bCopyRecursive
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN bExists = FALSE;

    ntStatus = LwioCheckDirectoryExists(pszSrcPath, &bExists);
    BAIL_ON_NT_STATUS(ntStatus);

    if (bExists)
    {
        ntStatus = LwioCopyDirToRemote(pszSrcPath, pszDestPath);
        BAIL_ON_NT_STATUS(ntStatus);

        goto done;
    }

    ntStatus = LwioCheckFileExists(pszSrcPath, &bExists);
    BAIL_ON_NT_STATUS(ntStatus);

    if (bExists)
    {
        ntStatus = LwioCopyFileToRemote(pszSrcPath, pszDestPath);
        BAIL_ON_NT_STATUS(ntStatus);
    }
    else
    {
        ntStatus = STATUS_NO_SUCH_FILE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

done:
cleanup:

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
CopyFile_LocalToLocal(
    PCSTR   pszSrcPath,
    PCSTR   pszDestPath,
    BOOLEAN bCopyRecursive
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
LwioCheckRemotePathIsDirectory(
    IN     PCSTR    pszPath,
    IN OUT PBOOLEAN pbIsDirectory
    )
{
    NTSTATUS        ntStatus = STATUS_SUCCESS;
    PSTR            pszRemoteFileName = NULL;
    IO_FILE_NAME    filename = {0};
    IO_FILE_HANDLE  hFile = NULL;
    IO_STATUS_BLOCK ioStatusBlock;
    FILE_STANDARD_INFORMATION fileStdInfo;

    ntStatus = LwRtlCStringAllocatePrintf(
                    &pszRemoteFileName,
                    "/rdr%s/",
                    pszPath+1);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwRtlWC16StringAllocateFromCString(
                    &filename.FileName,
                    pszRemoteFileName);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwNtCreateFile(
                    &hFile,                /* File handle */
                    NULL,                  /* Async control block */
                    &ioStatusBlock,        /* IO status block */
                    &filename,             /* Filename */
                    NULL,                  /* Security descriptor */
                    NULL,                  /* Security QOS */
                    FILE_READ_ATTRIBUTES,  /* Desired access mask */
                    0,                     /* Allocation size */
                    0,                     /* File attributes */
                    FILE_SHARE_READ,       /* Share access */
                    FILE_OPEN,             /* Create disposition */
                    0,                     /* Create options */
                    NULL,                  /* EA buffer */
                    0,                     /* EA length */
                    NULL);                 /* ECP list */
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwNtQueryInformationFile(
                    hFile,
                    NULL,
                    &ioStatusBlock,
                    (PVOID*)&fileStdInfo,
                    sizeof(fileStdInfo),
                    FileStandardInformation);
    BAIL_ON_NT_STATUS(ntStatus);

    *pbIsDirectory = fileStdInfo.Directory;

cleanup:

    if (hFile)
    {
        LwNtCloseFile(hFile);
    }

    return ntStatus;

error:

    *pbIsDirectory = FALSE;

    goto cleanup;
}

NTSTATUS
LwioCopyFileFromRemote(
    IN PCSTR pszSourcePath,
    IN PCSTR pszTargetPath
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    IO_FILE_HANDLE hRemoteFile = NULL;
    int hLocalFile = -1;

    BAIL_ON_NULL_POINTER(pszSourcePath);
    BAIL_ON_NULL_POINTER(pszTargetPath);

    status = LwioRemoteOpenFile(pszSourcePath, &hRemoteFile);
    BAIL_ON_NT_STATUS(status);

    status = LwioLocalOpenFile(
                (PCSTR)pszTargetPath,
                O_WRONLY|O_TRUNC|O_CREAT,
                0666,
                &hLocalFile);

    BAIL_ON_NT_STATUS(status);

    do
    {
        BYTE  szBuff[BUFF_SIZE];
        DWORD dwRead = 0;
        DWORD dwWrote = 0;

        status = LwioRemoteReadFile(
                        hRemoteFile,
                        szBuff,
                        sizeof(szBuff),
                        &dwRead);  // number of bytes read
        if (!dwRead)
        {
            status = STATUS_SUCCESS;
            break;
        }

        BAIL_ON_NT_STATUS(status);

        if ((dwWrote = write(hLocalFile, szBuff, dwRead)) == -1)
        {
            status = LwUnixErrnoToNtStatus(errno);
            BAIL_ON_NT_STATUS(status);
        }

    } while(1);


cleanup:

    if (hRemoteFile)
    {
        LwNtCloseFile(hRemoteFile);
    }

    close(hLocalFile);

    return (status);

error:

    goto cleanup;

}

NTSTATUS
LwioCopyDirFromRemote(
    IN PCSTR pszSourcePath,
    IN PCSTR pszTargetPath
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOL bRestart = TRUE;
    IO_FILE_NAME filename = {0};
    IO_FILE_HANDLE handle = NULL;
    IO_STATUS_BLOCK ioStatus ;
    PSTR pszRemoteFileName = NULL;
    PSTR pszEntryFilename = NULL;
    BYTE buffer[MAX_BUFFER];
    PFILE_BOTH_DIR_INFORMATION pInfo = NULL;
    PSTR pszLocalPath = NULL;
    PSTR pszRemotePath = NULL;

    BAIL_ON_NULL_POINTER(pszSourcePath);
    BAIL_ON_NULL_POINTER(pszTargetPath);

    status = LwRtlCStringAllocatePrintf(
        &pszRemoteFileName,
        "/rdr%s/",
        pszSourcePath+1);
    BAIL_ON_NT_STATUS(status);

    status = LwRtlWC16StringAllocateFromCString(
        &filename.FileName,
        pszRemoteFileName);
    BAIL_ON_NT_STATUS(status);

    status = LwNtCreateFile(
        &handle,               /* File handle */
        NULL,                  /* Async control block */
        &ioStatus,             /* IO status block */
        &filename,             /* Filename */
        NULL,                  /* Security descriptor */
        NULL,                  /* Security QOS */
        FILE_LIST_DIRECTORY,   /* Desired access mask */
        0,                     /* Allocation size */
        0,                     /* File attributes */
        FILE_SHARE_READ |
        FILE_SHARE_WRITE |
        FILE_SHARE_DELETE,     /* Share access */
        FILE_OPEN,             /* Create disposition */
        FILE_DIRECTORY_FILE,   /* Create options */
        NULL,                  /* EA buffer */
        0,                     /* EA length */
        NULL);                 /* ECP list */
    BAIL_ON_NT_STATUS(status);

    status = LwioLocalCreateDir(
                pszTargetPath,
                S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);
    BAIL_ON_NT_STATUS(status);

    for (;;)
    {
        status = LwNtQueryDirectoryFile(
            handle,                             /* File handle */
            NULL,                               /* Async control block */
            &ioStatus,                             /* IO status block */
            buffer,                             /* Info structure */
            sizeof(buffer),                     /* Info structure size */
            FileBothDirectoryInformation,         /* Info level */
            FALSE,                                 /* Do not return single entry */
            NULL,                                 /* File spec */
            bRestart);                             /* Restart scan */

        switch (status)
        {
        case STATUS_NO_MORE_MATCHES:
            status = STATUS_SUCCESS;
            goto cleanup;
        default:
            BAIL_ON_NT_STATUS(status);
        }

        bRestart = FALSE;

        for (pInfo = (PFILE_BOTH_DIR_INFORMATION) buffer; pInfo;
                   pInfo = (pInfo->NextEntryOffset)?(PFILE_BOTH_DIR_INFORMATION) (((PBYTE) pInfo) + pInfo->NextEntryOffset):NULL)
        {
            RTL_FREE(&pszEntryFilename);
            RTL_FREE(&pszRemotePath);
            RTL_FREE(&pszLocalPath);

            status = LwRtlCStringAllocateFromWC16String(
                        &pszEntryFilename,
                        pInfo->FileName
                        );
            BAIL_ON_NT_STATUS(status);

            if (!strcmp(pszEntryFilename, "..") ||
                !strcmp(pszEntryFilename, "."))
                continue;

            status = LwRtlCStringAllocatePrintf(
                        &pszRemotePath,
                        "%s/%s",
                        pszSourcePath,
                        pszEntryFilename);
            BAIL_ON_NT_STATUS(status);

            status = LwRtlCStringAllocatePrintf(
                        &pszLocalPath,
                        "%s/%s",
                        pszTargetPath,
                        pszEntryFilename);
            BAIL_ON_NT_STATUS(status);

            if(pInfo->FileAttributes == FILE_ATTRIBUTE_DIRECTORY)
            {

                status = LwioCopyDirFromRemote(
                            pszRemotePath,
                            pszLocalPath);
                BAIL_ON_NT_STATUS(status);
            }
            else
            {
                status = LwioCopyFileFromRemote(
                            pszRemotePath,
                            pszLocalPath);
                BAIL_ON_NT_STATUS(status);
          }
        }
    }

cleanup:

    if (handle)
    {
        LwNtCloseFile(handle);
    }

    RTL_FREE(&pszLocalPath);
    RTL_FREE(&pszRemotePath);
    RTL_FREE(&pszEntryFilename);
    RTL_FREE(&filename.FileName);

    return status;

error:

    goto cleanup;
}


NTSTATUS
LwioCopyFileToRemote(
    IN PCSTR pszSourcePath,
    IN PCSTR pszTargetPath
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    IO_FILE_HANDLE hRemoteFile = NULL;
    int hLocalFile = -1;
    DWORD dwBytesRead = 0;
    CHAR szBuf[BUFF_SIZE];

    BAIL_ON_NULL_POINTER(pszSourcePath);
    BAIL_ON_NULL_POINTER(pszTargetPath);

    status = LwioLocalOpenFile(
                (PCSTR)pszSourcePath,
                O_RDONLY,
                0,
                &hLocalFile);

    BAIL_ON_NT_STATUS(status);

    status = LwioRemoteCreateFile(pszTargetPath, &hRemoteFile);
    BAIL_ON_NT_STATUS(status);

    do
    {
        DWORD dwWritten = 0;

        memset (szBuf,0,BUFF_SIZE);

        if ((dwBytesRead = read(hLocalFile, szBuf, sizeof(szBuf))) == -1)
        {
            status = LwUnixErrnoToNtStatus(errno);
            BAIL_ON_NT_STATUS(status);
        }

        if (dwBytesRead == 0)
        {
            break;
        }

        status  = LwioRemoteWriteFile(
                            hRemoteFile,
                            szBuf,
                            dwBytesRead,
                            &dwWritten);

        BAIL_ON_NT_STATUS(status);

    } while (dwBytesRead != 0);

cleanup:

    if (hRemoteFile)
    {
        LwNtCloseFile(hRemoteFile);
    }

    close(hLocalFile);

    return (status);

error:

    goto cleanup;

}

NTSTATUS
LwioCopyDirToRemote(
    IN PCSTR pszSourcePath,
    IN PCSTR pszTargetPath
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    DIR* pDir = NULL;
    struct dirent* pDirEntry = NULL;
    struct stat statbuf;
    IO_STATUS_BLOCK ioStatus ;
    IO_FILE_NAME filename = {0};
    IO_FILE_HANDLE handle = NULL;
    PSTR pszRemoteFileName = NULL;
    PSTR pszLocalPath = NULL;
    PSTR pszRemotePath = NULL;
    CHAR szBuf[BUFF_SIZE];

    BAIL_ON_NULL_POINTER(pszSourcePath);
    BAIL_ON_NULL_POINTER(pszTargetPath);

    if ((pDir = opendir(pszSourcePath)) == NULL)
    {
        status = LwUnixErrnoToNtStatus(errno);
        BAIL_ON_NT_STATUS(status);
    }

    status = LwRtlCStringAllocatePrintf(
        &pszRemoteFileName,
        "/rdr%s/",
        pszTargetPath+1);
    BAIL_ON_NT_STATUS(status);

    status = LwRtlWC16StringAllocateFromCString(
        &filename.FileName,
        pszRemoteFileName);
    BAIL_ON_NT_STATUS(status);

    status = LwNtCreateFile(
        &handle,               /* File handle */
        NULL,                  /* Async control block */
        &ioStatus,             /* IO status block */
        &filename,             /* Filename */
        NULL,                  /* Security descriptor */
        NULL,                  /* Security QOS */
        FILE_LIST_DIRECTORY,   /* Desired access mask */
        0,                     /* Allocation size */
        0,                     /* File attributes */
        FILE_SHARE_READ |
        FILE_SHARE_WRITE |
        FILE_SHARE_DELETE,     /* Share access */
        FILE_CREATE,           /* Create disposition */
        FILE_DIRECTORY_FILE,   /* Create options */
        NULL,                  /* EA buffer */
        0,                     /* EA length */
        NULL);                 /* ECP list */
    BAIL_ON_NT_STATUS(status);

    while ((pDirEntry = readdir(pDir)) != NULL)
    {
        RTL_FREE(&pszRemotePath);
        RTL_FREE(&pszLocalPath);

        if (!strcmp(pDirEntry->d_name, "..") ||
            !strcmp(pDirEntry->d_name, "."))
            continue;

        sprintf(szBuf, "%s/%s", pszSourcePath, pDirEntry->d_name);

        memset(&statbuf, 0, sizeof(struct stat));

        if (stat(szBuf, &statbuf) < 0)
        {
            status = LwUnixErrnoToNtStatus(errno);
            BAIL_ON_NT_STATUS(status);
        }

        status = LwRtlCStringAllocatePrintf(
                    &pszLocalPath,
                    "%s/%s",
                    pszSourcePath,
                    pDirEntry->d_name);
        BAIL_ON_NT_STATUS(status);

        status = LwRtlCStringAllocatePrintf(
                    &pszRemotePath,
                    "%s/%s",
                    pszTargetPath,
                    pDirEntry->d_name);
        BAIL_ON_NT_STATUS(status);

        if ((statbuf.st_mode & S_IFMT) == S_IFDIR)
        {
            status = LwioCopyDirToRemote(
                            pszLocalPath,
                            pszRemotePath);
            BAIL_ON_NT_STATUS(status);
        }
        else
        {
            status = LwioCopyFileToRemote(
                            pszLocalPath,
                            pszRemotePath);
            BAIL_ON_NT_STATUS(status);
        }
    }

    if(closedir(pDir) < 0)
    {
        pDir = NULL;
        status = LwUnixErrnoToNtStatus(status);
        BAIL_ON_NT_STATUS(status);
    }

    pDir = NULL;

cleanup:
    if (handle)
        LwNtCloseFile(handle);

    if (pDir)
        closedir(pDir);

    RTL_FREE(&pszLocalPath);
    RTL_FREE(&pszRemotePath);
    return status;

error:
    goto cleanup;

}


NTSTATUS
LwioRemoteOpenFile(
    IN PCSTR        pszFileName,
    PIO_FILE_HANDLE phFile
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    IO_FILE_NAME filename = {0};
    IO_FILE_HANDLE handle = NULL;
    IO_STATUS_BLOCK ioStatus ;
    PSTR pszRemoteFileName = NULL;

    BAIL_ON_NULL_POINTER(pszFileName);

    status = LwRtlCStringAllocatePrintf(
        &pszRemoteFileName,
        "/rdr%s",
        pszFileName+1);
    BAIL_ON_NT_STATUS(status);

    status = LwRtlWC16StringAllocateFromCString(
        &filename.FileName,
        pszRemoteFileName);
    BAIL_ON_NT_STATUS(status);

    status = LwNtCreateFile(
        &handle,               /* File handle */
        NULL,                  /* Async control block */
        &ioStatus,             /* IO status block */
        &filename,             /* Filename */
        NULL,                  /* Security descriptor */
        NULL,                  /* Security QOS */
        GENERIC_READ | GENERIC_WRITE,            /* Desired access mask */
        0,                     /* Allocation size */
        0,                     /* File attributes */
        FILE_SHARE_READ |
        FILE_SHARE_WRITE |
        FILE_SHARE_DELETE,     /* Share access */
        FILE_OPEN,           /* Create disposition */
        FILE_NON_DIRECTORY_FILE,               /* Create options */
        NULL,                  /* EA buffer */
        0,                     /* EA length */
        NULL);                 /* ECP list */
    BAIL_ON_NT_STATUS(status);

    *phFile = handle;

cleanup:

    RTL_FREE(&pszRemoteFileName);

    return status;

error:

    *phFile = NULL;

    goto cleanup;
}


NTSTATUS
LwioLocalOpenFile(
    IN PCSTR pszFileName,
    IN INT  dwMode,
    IN INT dwPerms,
    OUT INT *dwHandle
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int fd = -1;

    BAIL_ON_NULL_POINTER(pszFileName);

    if ((fd = open(pszFileName, dwMode, dwPerms)) == -1)
    {
        status = LwUnixErrnoToNtStatus(errno);
        BAIL_ON_NT_STATUS(status);
    }


error:

    *dwHandle = fd;

    return status;

}


NTSTATUS
LwioLocalCreateDir(
    IN PCSTR pszPath,
    IN mode_t dwFileMode
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PSTR pszTmpPath = NULL;

    BAIL_ON_NULL_POINTER(pszPath);

    status = SMBAllocateString(
                pszPath,
                &pszTmpPath);
    BAIL_ON_NT_STATUS(status);

    status = LwioLocalCreateDirInternal(
                pszTmpPath,
                NULL,
                dwFileMode);
    BAIL_ON_NT_STATUS(status);

cleanup:

    LWIO_SAFE_FREE_STRING(pszTmpPath);
    return status;

error:
    goto cleanup;
}


NTSTATUS
LwioLocalCreateDirInternal(
    IN PSTR pszPath,
    IN PSTR pszLastSlash,
    IN mode_t dwFileMode
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PSTR pszSlash = NULL;
    BOOLEAN bDirExists = FALSE;
    BOOLEAN bDirCreated = FALSE;

    BAIL_ON_NULL_POINTER(pszPath);

    pszSlash = pszLastSlash ? strchr(pszLastSlash + 1, '/') : strchr(pszPath, '/');

    if (pszSlash)
    {
        *pszSlash = '\0';
    }

    if (pszPath[0])
    {
        status = LwioLocalCheckDirExists(pszPath, &bDirExists);
        BAIL_ON_NT_STATUS(status);

        if (!bDirExists)
        {
            if (mkdir(pszPath, S_IRWXU) != 0)
            {
                status = LwUnixErrnoToNtStatus(errno);
                BAIL_ON_NT_STATUS(status);
            }
            bDirCreated = TRUE;
        }
    }

    if (pszSlash)
    {
        *pszSlash = '/';
    }

    if (pszSlash)
    {
        status = LwioLocalCreateDirInternal(pszPath, pszSlash, dwFileMode);
        BAIL_ON_NT_STATUS(status);
    }

    if (pszSlash)
    {
        *pszSlash = '\0';
    }

    if (bDirCreated)
    {
        status = LwioLocalChangePermissions(pszPath, dwFileMode);
        BAIL_ON_NT_STATUS(status);
    }

    if (pszSlash)
    {
        *pszSlash = '/';
    }

cleanup:

    return status;

error:

    if (pszSlash)
    {
        *pszSlash = '\0';
    }

    if (bDirCreated)
    {
        LwioLocalRemoveDir(pszPath);
    }

    if (pszSlash)
    {
        *pszSlash = '/';
    }

    goto cleanup;
}


NTSTATUS
LwioLocalChangePermissions(
    IN PCSTR pszPath,
    IN mode_t dwFileMode
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    BAIL_ON_NULL_POINTER(pszPath);

    while (1)
    {
        if (chmod(pszPath, dwFileMode) < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            status = LwUnixErrnoToNtStatus(errno);
            BAIL_ON_NT_STATUS(status);
        }
        else
        {
            break;
        }
    }

error:
    return status;
}


NTSTATUS
LwioLocalCheckDirExists(
    IN PCSTR pszPath,
    IN PBOOLEAN pbDirExists
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    struct stat statbuf;

    BAIL_ON_NULL_POINTER(pszPath);

    while (1)
    {
        memset(&statbuf, 0, sizeof(struct stat));

        if (stat(pszPath, &statbuf) < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            else if (errno == ENOENT || errno == ENOTDIR)
            {
                *pbDirExists = FALSE;
                break;
            }
            status = LwUnixErrnoToNtStatus(errno);
            BAIL_ON_NT_STATUS(status);
        }

        *pbDirExists = (((statbuf.st_mode & S_IFMT) == S_IFDIR) ? TRUE : FALSE);
        break;
    }

error:
    return status;
}


NTSTATUS
LwioLocalRemoveDir(
    IN PCSTR pszPath
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    DIR* pDir = NULL;
    struct dirent* pDirEntry = NULL;
    struct stat statbuf;
    CHAR szBuf[BUFF_SIZE+1];

    BAIL_ON_NULL_POINTER(pszPath);

    if ((pDir = opendir(pszPath)) == NULL)
    {
        status = LwUnixErrnoToNtStatus(errno);
        BAIL_ON_NT_STATUS(status);
    }

    while ((pDirEntry = readdir(pDir)) != NULL)
    {

        if (!strcmp(pDirEntry->d_name, "..") ||
            !strcmp(pDirEntry->d_name, "."))
            continue;

        sprintf(szBuf, "%s/%s", pszPath, pDirEntry->d_name);

        memset(&statbuf, 0, sizeof(struct stat));

        if (stat(szBuf, &statbuf) < 0)
        {
            status = LwUnixErrnoToNtStatus(errno);
            BAIL_ON_NT_STATUS(status);
        }

        if ((statbuf.st_mode & S_IFMT) == S_IFDIR)
        {
            status = LwioLocalRemoveDir(szBuf);
            BAIL_ON_NT_STATUS(status);

            if (rmdir(szBuf) < 0)
            {
                status = LwUnixErrnoToNtStatus(status);
                BAIL_ON_NT_STATUS(status);
            }
        }
        else
        {
            status = LwioLocalRemoveFile(szBuf);
            BAIL_ON_NT_STATUS(status);

        }
    }

    if(closedir(pDir) < 0)
    {
        pDir = NULL;
        status = LwUnixErrnoToNtStatus(status);
        BAIL_ON_NT_STATUS(status);
    }

    pDir = NULL;

    if (rmdir(pszPath) < 0)
    {
        status = LwUnixErrnoToNtStatus(status);
        BAIL_ON_NT_STATUS(status);
    }

error:

    if (pDir)
        closedir(pDir);

    return status;
}


NTSTATUS
LwioLocalRemoveFile(
    IN PCSTR pszPath
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    BAIL_ON_NULL_POINTER(pszPath);

    while (1)
    {
        if (unlink(pszPath) < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            status = LwUnixErrnoToNtStatus(errno);
            BAIL_ON_NT_STATUS(status);
        }
        else
        {
            break;
        }
    }

error:

    return status;
}


NTSTATUS
LwioRemoteReadFile(
    IN HANDLE hFile,
    OUT PVOID pBuffer,
    IN DWORD dwNumberOfBytesToRead,
    OUT PDWORD pdwBytesRead
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    IO_STATUS_BLOCK ioStatus;

    status = LwNtReadFile(
        hFile,                               // File handle
        NULL,                                // Async control block
        &ioStatus,                           // IO status block
        pBuffer,                             // Buffer
        (ULONG) dwNumberOfBytesToRead,       // Buffer size
        NULL,                                // File offset
        NULL);                               // Key
    BAIL_ON_NT_STATUS(status);

    *pdwBytesRead = (int) ioStatus.BytesTransferred;

cleanup:

    return status;

error:

    *pdwBytesRead = 0;
    pBuffer = NULL;

    goto cleanup;
}


NTSTATUS
LwioRemoteCreateFile(
    IN PCSTR        pszFileName,
    PIO_FILE_HANDLE phFile
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    IO_FILE_NAME filename = {0};
    IO_FILE_HANDLE handle = NULL;
    IO_STATUS_BLOCK ioStatus ;
    PSTR pszRemoteFileName = NULL;

    BAIL_ON_NULL_POINTER(pszFileName);

    status = LwRtlCStringAllocatePrintf(
        &pszRemoteFileName,
        "/rdr%s",
        pszFileName+1);
    BAIL_ON_NT_STATUS(status);

    status = LwRtlWC16StringAllocateFromCString(
        &filename.FileName,
        pszRemoteFileName);
    BAIL_ON_NT_STATUS(status);

    status = LwNtCreateFile(
        &handle,                 /* File handle */
        NULL,                    /* Async control block */
        &ioStatus,               /* IO status block */
        &filename,               /* Filename */
        NULL,                    /* Security descriptor */
        NULL,                    /* Security QOS */
        FILE_WRITE_DATA,         /* Desired access mask */
        0,                       /* Allocation size */
        0,                       /* File attributes */
        FILE_SHARE_READ |
        FILE_SHARE_WRITE |
        FILE_SHARE_DELETE,       /* Share access */
        FILE_CREATE,             /* Create disposition */
        FILE_NON_DIRECTORY_FILE, /* Create options */
        NULL,                    /* EA buffer */
        0,                       /* EA length */
        NULL);                   /* ECP list */
    BAIL_ON_NT_STATUS(status);

    *phFile = handle;

cleanup:

    RTL_FREE(&pszRemoteFileName);
    return status;

error:

    *phFile = NULL;

    goto cleanup;
}


NTSTATUS
LwioRemoteWriteFile(
    IN HANDLE hFile,
    IN PVOID pBuffer,
    IN DWORD dwNumBytesToWrite,
    OUT PDWORD pdwNumBytesWritten
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    IO_STATUS_BLOCK ioStatus ;

    BAIL_ON_NULL_POINTER(pBuffer);

    status = LwNtWriteFile(
        hFile,                                 // File handle
        NULL,                                 // Async control block
        &ioStatus,                             // IO status block
        pBuffer,                             // Buffer
        (ULONG) dwNumBytesToWrite,             // Buffer size
        NULL,                                 // File offset
        NULL);                                 // Key
    BAIL_ON_NT_STATUS(status);

    *pdwNumBytesWritten = (int) ioStatus.BytesTransferred;

cleanup:

    return status;

error:

    *pdwNumBytesWritten = 0;

    goto cleanup;

}

NTSTATUS
LwioCheckFileExists(
    PCSTR pszPath,
    PBOOLEAN pbFileExists
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    struct stat statbuf;

    BAIL_ON_NULL_POINTER(pszPath);

    memset(&statbuf, 0, sizeof(struct stat));

    while (1) {
        if (stat(pszPath, &statbuf) < 0) {
            if (errno == EINTR)
            {
                continue;
            }
            else if (errno == ENOENT || errno == ENOTDIR)
            {
                *pbFileExists = FALSE;
                break;
            }

            status = LwUnixErrnoToNtStatus(errno);
            BAIL_ON_NT_STATUS(status);

        } else {
            *pbFileExists = (((statbuf.st_mode & S_IFMT) == S_IFREG) ? TRUE : FALSE);
            break;
        }
    }

cleanup:

    return status;

error:

    *pbFileExists = FALSE;

    goto cleanup;
}

NTSTATUS
LwioCheckDirectoryExists(
    PCSTR pszPath,
    PBOOLEAN pbDirExists
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    struct stat statbuf;

    while (1) {

        memset(&statbuf, 0, sizeof(struct stat));

        if (stat(pszPath, &statbuf) < 0) {

            if (errno == EINTR) {
                continue;
            }
            else if (errno == ENOENT || errno == ENOTDIR) {
                *pbDirExists = FALSE;
                break;
            }

            status = LwUnixErrnoToNtStatus(errno);
            BAIL_ON_NT_STATUS(status);

        }

        *pbDirExists = (((statbuf.st_mode & S_IFMT) == S_IFDIR) ? TRUE : FALSE);
        break;
    }

cleanup:

    return status;

error:

    *pbDirExists = FALSE;

    goto cleanup;
}
