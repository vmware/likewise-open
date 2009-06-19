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
 *        main.c
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

#define _POSIX_PTHREAD_SEMANTICS 1

#include "includes.h"


static
NTSTATUS
SLocalOpenFile(
    IN PCSTR pszFileName,
    IN INT dwMode,
    IN INT dwPerms,
    OUT INT *dwHandle
    );

static
NTSTATUS
SLocalCreateDir(
    IN PCSTR pszPath,
    IN mode_t dwFileMode
    );

static
NTSTATUS
SLocalCreateDirInternal(
    IN PSTR pszPath,
    IN PSTR pszLastSlash,
    IN mode_t dwFileMode
    );

static
NTSTATUS
SLocalChangePermissions(
    IN PCSTR pszPath,
    IN mode_t dwFileMode
    );

static
NTSTATUS
SLocalCheckDirExists(
    IN PCSTR pszPath,
    IN PBOOLEAN pbDirExists
    );

static
NTSTATUS
SLocalRemoveDir(
    IN PCSTR pszPath
    );

static
NTSTATUS
SLocalRemoveFile(
    IN PCSTR pszPath
    );

static
HANDLE
SRemoteCreateFile(
    IN PCSTR pszFileName
    );

static
HANDLE
SRemoteOpenFile(
    IN PCSTR pszFileName
    );

static
NTSTATUS
SRemoteWriteFile(
    IN HANDLE hFile,
    IN PVOID pBuffer,
    IN DWORD dwNumBytesToWrite,
    OUT PDWORD pdwNumBytesWritten,
    IN OUT OPTIONAL POVERLAPPED pOverlapped
    );

static
NTSTATUS
SRemoteReadFile(
    IN HANDLE hFile,
    OUT PVOID pBuffer,
    IN DWORD dwNumberOfBytesToRead,
    OUT PDWORD pdwBytesRead,
    IN OUT OPTIONAL POVERLAPPED pOverlapped
    );

static
PFILE_BOTH_DIR_INFORMATION
GetNextDirInfo(
    PFILE_BOTH_DIR_INFORMATION pInfo
    );


NTSTATUS
CopyFileFromNt(
    IN PCSTR pszSourcePath,
    IN PCSTR pszTargetPath
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    HANDLE hRemoteFile = (HANDLE)NULL;
    int hLocalFile = -1;

    hRemoteFile = SRemoteOpenFile( (PCSTR)pszSourcePath);
    if (hRemoteFile == INVALID_HANDLE_VALUE)
    {
        fprintf(stderr, "Failed to open file  %s \n", pszSourcePath);
        goto error;
    }

    status = SLocalOpenFile(
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

        status = SRemoteReadFile(
                        hRemoteFile,
                        szBuff,
                        sizeof(szBuff),
                        &dwRead,  // number of bytes read
                        NULL);    // not overlapped
        if (!dwRead)
        {
            status = STATUS_SUCCESS;
            break;
        }

        BAIL_ON_NT_STATUS(status);

        if ((dwWrote = write(hLocalFile, szBuff, dwRead)) == -1)
        {
            fprintf(stderr, "Write failed! (%s)\n", strerror(errno));
            status = STATUS_UNSUCCESSFUL;
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
CopyDirFromNt(
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

    status = LwRtlCStringAllocatePrintf(
        &pszRemoteFileName,
        "/rdr%s/",
        pszSourcePath);
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

    status = SLocalCreateDir(
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

        for (pInfo = (PFILE_BOTH_DIR_INFORMATION) buffer; pInfo; pInfo = GetNextDirInfo(pInfo))
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

                status = CopyDirFromNt(
                            pszRemotePath,
                            pszLocalPath);
                BAIL_ON_NT_STATUS(status);
            }
            else
            {
                status = CopyFileFromNt(
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

static
PFILE_BOTH_DIR_INFORMATION
GetNextDirInfo(
    PFILE_BOTH_DIR_INFORMATION pInfo
    )
{
    if (pInfo->NextEntryOffset)
    {
        return (PFILE_BOTH_DIR_INFORMATION) (((PBYTE) pInfo) + pInfo->NextEntryOffset);
    }
    else
    {
        return NULL;
    }
}

NTSTATUS
CopyFileToNt(
    IN PCSTR pszSourcePath,
    IN PCSTR pszTargetPath
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    HANDLE hRemoteFile = (HANDLE)NULL;
    int hLocalFile = -1;
    DWORD dwBytesRead = 0;
    CHAR szBuf[BUFF_SIZE];

    status = SLocalOpenFile(
                (PCSTR)pszSourcePath,
                O_RDONLY,
                0,
                &hLocalFile);

    BAIL_ON_NT_STATUS(status);

    hRemoteFile = SRemoteCreateFile( (PCSTR)pszTargetPath);
    if (hRemoteFile == INVALID_HANDLE_VALUE)
    {
        fprintf(stderr, "Failed to open file  %s \n", pszTargetPath);
        goto error;
    }

    do
    {
        DWORD dwWritten = 0;

        memset (szBuf,0,BUFF_SIZE);

        if ((dwBytesRead = read(hLocalFile, szBuf, sizeof(szBuf))) == -1)
        {
            fprintf(stderr, "Read failed! (%s)\n", strerror(errno));
            status = STATUS_UNSUCCESSFUL;
            BAIL_ON_NT_STATUS(status);
        }

        if (dwBytesRead == 0)
        {
            break;
        }

        status  = SRemoteWriteFile(
                            hRemoteFile,
                            szBuf,
                            dwBytesRead,
                            &dwWritten,
                            NULL);

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
CopyDirToNt(
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

    if ((pDir = opendir(pszSourcePath)) == NULL)
    {
        status = LwUnixErrnoToNtStatus(errno);
        BAIL_ON_NT_STATUS(status);
    }

    status = LwRtlCStringAllocatePrintf(
        &pszRemoteFileName,
        "/rdr%s/",
        pszTargetPath);
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
            status = CopyDirToNt(
                            pszLocalPath,
                            pszRemotePath);
            BAIL_ON_NT_STATUS(status);
        }
        else
        {
            status = CopyFileToNt(
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

static
HANDLE
SRemoteOpenFile(
    IN PCSTR pszFileName
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    IO_FILE_NAME filename = {0};
    IO_FILE_HANDLE handle = NULL;
    IO_STATUS_BLOCK ioStatus ;
    PSTR pszRemoteFileName = NULL;

    status = LwRtlCStringAllocatePrintf(
        &pszRemoteFileName,
        "/rdr%s",
        pszFileName);
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


cleanup:

    RTL_FREE(&pszRemoteFileName);
    return handle;

error:

    goto cleanup;
}

static
NTSTATUS
SLocalOpenFile(
    IN PCSTR pszFileName,
    IN INT  dwMode,
    IN INT dwPerms,
    OUT INT *dwHandle
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int fd = -1;

    if ((fd = open(pszFileName, dwMode, dwPerms)) == -1)
    {
        fprintf(stderr, "Failed to open local file \"%s\" for copy (%s).\n",
                pszFileName, strerror(errno));

        status = STATUS_UNSUCCESSFUL;
        BAIL_ON_NT_STATUS(status);
    }


error:
    *dwHandle = fd;
    return status;

}

static
NTSTATUS
SLocalCreateDir(
    IN PCSTR pszPath,
    IN mode_t dwFileMode
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PSTR pszTmpPath = NULL;

    if (pszPath == NULL)
    {
        status = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(status);
    }

    status = SMBAllocateString(
                pszPath,
                &pszTmpPath);
    BAIL_ON_NT_STATUS(status);

    status = SLocalCreateDirInternal(
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

static
NTSTATUS
SLocalCreateDirInternal(
    IN PSTR pszPath,
    IN PSTR pszLastSlash,
    IN mode_t dwFileMode
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PSTR pszSlash = NULL;
    BOOLEAN bDirExists = FALSE;
    BOOLEAN bDirCreated = FALSE;

    pszSlash = pszLastSlash ? strchr(pszLastSlash + 1, '/') : strchr(pszPath, '/');

    if (pszSlash)
    {
        *pszSlash = '\0';
    }

    if (pszPath[0])
    {
        status = SLocalCheckDirExists(pszPath, &bDirExists);
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
        status = SLocalCreateDirInternal(pszPath, pszSlash, dwFileMode);
        BAIL_ON_NT_STATUS(status);
    }

    if (pszSlash)
    {
        *pszSlash = '\0';
    }

    if (bDirCreated)
    {
        status = SLocalChangePermissions(pszPath, dwFileMode);
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
        SLocalRemoveDir(pszPath);
    }

    if (pszSlash)
    {
        *pszSlash = '/';
    }

    goto cleanup;
}

static
NTSTATUS
SLocalChangePermissions(
    IN PCSTR pszPath,
    IN mode_t dwFileMode
    )
{
    NTSTATUS status = STATUS_SUCCESS;

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

static
NTSTATUS
SLocalCheckDirExists(
    IN PCSTR pszPath,
    IN PBOOLEAN pbDirExists
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    struct stat statbuf;

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

static
NTSTATUS
SLocalRemoveDir(
    IN PCSTR pszPath
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    DIR* pDir = NULL;
    struct dirent* pDirEntry = NULL;
    struct stat statbuf;
    CHAR szBuf[BUFF_SIZE+1];

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
            status = SLocalRemoveDir(szBuf);
            BAIL_ON_NT_STATUS(status);

            if (rmdir(szBuf) < 0)
            {
                status = LwUnixErrnoToNtStatus(status);
                BAIL_ON_NT_STATUS(status);
            }
        }
        else
        {
            status = SLocalRemoveFile(szBuf);
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

static
NTSTATUS
SLocalRemoveFile(
    IN PCSTR pszPath
    )
{
    NTSTATUS status = STATUS_SUCCESS;

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

static
NTSTATUS
SRemoteReadFile(
    IN HANDLE hFile,
    OUT PVOID pBuffer,
    IN DWORD dwNumberOfBytesToRead,
    OUT PDWORD pdwBytesRead,
    IN OUT OPTIONAL POVERLAPPED pOverlapped
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
    goto cleanup;
}

static
HANDLE
SRemoteCreateFile(
    IN PCSTR pszFileName
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    IO_FILE_NAME filename = {0};
    IO_FILE_HANDLE handle = NULL;
    IO_STATUS_BLOCK ioStatus ;
    PSTR pszRemoteFileName = NULL;

    status = LwRtlCStringAllocatePrintf(
        &pszRemoteFileName,
        "/rdr%s",
        pszFileName);
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


cleanup:

    RTL_FREE(&pszRemoteFileName);
    return handle;

error:

    goto cleanup;
}

static
NTSTATUS
SRemoteWriteFile(
    IN HANDLE hFile,
    IN PVOID pBuffer,
    IN DWORD dwNumBytesToWrite,
    OUT PDWORD pdwNumBytesWritten,
    IN OUT OPTIONAL POVERLAPPED pOverlapped
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    IO_STATUS_BLOCK ioStatus ;

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
    goto cleanup;

}
