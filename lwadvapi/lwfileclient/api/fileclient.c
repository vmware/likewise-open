/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2008
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

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *       FileClient.c
 *
 * Abstract:
 *
 *        Likewise File Browser Client
 *
 *        Public Client API
 *
 * Authors:
 */

#include "includes.h"


static DWORD gdwLastError = 0;


DWORD
GetLastError(
    )
{
    return gdwLastError;
}


static
void
SetLastError(
    DWORD dwError
    )
{
    gdwLastError = dwError;
}


BOOL
CopyFile(
    PCWSTR pszFileName,
    PCWSTR pszNewFileName,
    BOOL bOverWrite
    )
{
    DWORD dwError = 0;
    BOOL bResult = FALSE;
    IO_FILE_HANDLE hSrcHandle = NULL;
    IO_FILE_HANDLE hDestHandle = NULL;
    IO_FILE_NAME srcFilename = {0};
    IO_FILE_NAME destFilename = {0};
    IO_STATUS_BLOCK srcIoStatus = {0};
    IO_STATUS_BLOCK destIoStatus = {0};
    BYTE Buffer[MAX_BUFFER] = {0};
    PWSTR pszInternalFileName = NULL;
    PWSTR pszInternalNewFileName = NULL;

    SetLastError(dwError);

    if (pszFileName == NULL || pszNewFileName == NULL)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN_ERROR(dwError);
    }

    dwError = LwNtStatusToWin32Error(LwIoUncPathToInternalPath(pszFileName, &pszInternalFileName));
    BAIL_ON_WIN_ERROR(dwError);

    dwError = LwNtStatusToWin32Error(LwIoUncPathToInternalPath(pszNewFileName, &pszInternalNewFileName));
    BAIL_ON_WIN_ERROR(dwError);

    srcFilename.FileName = pszInternalFileName;
    destFilename.FileName = pszInternalNewFileName;

    dwError = LwNtStatusToWin32Error(LwNtCreateFile(
                &hSrcHandle,             /* File handle */
                NULL,                    /* Async control block */
                &srcIoStatus,            /* IO status block */
                &srcFilename,            /* Filename */
                NULL,                    /* Security descriptor */
                NULL,                    /* Security QOS */
                FILE_READ_DATA,          /* Desired access mask */
                0,                       /* Allocation size */
                0,                       /* File attributes */
                FILE_SHARE_READ,         /* Share access */
                FILE_OPEN,               /* Create disposition */
                FILE_NON_DIRECTORY_FILE, /* Create options */
                NULL,                    /* EA buffer */
                0,                       /* EA length */
                NULL));                  /* ECP list */
    BAIL_ON_WIN_ERROR(dwError);

    dwError = LwNtStatusToWin32Error(LwNtCreateFile(
                &hDestHandle,            /* File handle */
                NULL,                    /* Async control block */
                &destIoStatus,           /* IO status block */
                &destFilename,           /* Filename */
                NULL,                    /* Security descriptor */
                NULL,                    /* Security QOS */
                FILE_WRITE_DATA,         /* Desired access mask */
                0,                       /* Allocation size */
                0,                       /* File attributes */
                FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, /* Share access */
                bOverWrite ? FILE_OVERWRITE_IF : FILE_OPEN_IF,      /* Create disposition */
                FILE_NON_DIRECTORY_FILE, /* Create options */
                NULL,                    /* EA buffer */
                0,                       /* EA length */
                NULL));                  /* ECP list */
    BAIL_ON_WIN_ERROR(dwError);

    for(;;)
    {
        dwError = LwNtStatusToWin32Error(LwNtReadFile(
                    hSrcHandle,         // File handle
                    NULL,               // Async control block
                    &srcIoStatus,          // IO status block
                    Buffer,             // Buffer
                    (ULONG) MAX_BUFFER, // Buffer size
                    NULL,               // File offset
                    NULL));
        switch(dwError)
        {
            case ERROR_NO_MORE_MATCHES :
                dwError = 0;
                goto cleanup;

            case ERROR_HANDLE_EOF :
                dwError = 0;
                goto cleanup;

            default:
                BAIL_ON_WIN_ERROR(dwError);
        }

        dwError = LwNtStatusToWin32Error(LwNtWriteFile(
                    hDestHandle,               // File handle
                    NULL,                      // Async control block
                    &destIoStatus,             // IO status block
                    Buffer,                    // Buffer
                    (ULONG) srcIoStatus.BytesTransferred, // Buffer size
                    NULL,                      // File offset
                    NULL));                    // Key

        BAIL_ON_WIN_ERROR(dwError);
    }

    bResult = TRUE;

cleanup:

    if (pszInternalFileName)
    {
        LwFreeString((PSTR)(pszInternalFileName));
    }

    if (pszInternalNewFileName)
    {
        LwFreeString((PSTR)(pszInternalNewFileName));
    }

    if (hSrcHandle)
    {
        LwNtCloseFile(hSrcHandle);
    }

    if (hDestHandle)
    {
        LwNtCloseFile(hDestHandle);
    }

    return bResult;

error:

    SetLastError(dwError);

    goto cleanup;
}


BOOL
DeleteFile(
    PCWSTR pszFileName
    )
{
    DWORD dwError = 0;
    BOOL bResult = FALSE;
    IO_FILE_HANDLE hFileHandle = NULL;
    IO_STATUS_BLOCK ioStatus = {0};
    IO_FILE_NAME filename = {0};
    PWSTR pszInternalFileName = NULL;

    SetLastError(dwError);

    if (pszFileName == NULL)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN_ERROR(dwError);
    }

    dwError = LwNtStatusToWin32Error(LwIoUncPathToInternalPath(pszFileName, &pszInternalFileName));
    BAIL_ON_WIN_ERROR(dwError);

    filename.FileName = pszInternalFileName;

    dwError = LwNtStatusToWin32Error(LwNtCreateFile(
                &hFileHandle,            /* File handle */
                NULL,                    /* Async control block */
                &ioStatus,               /* IO status block */
                &filename,               /* Filename */
                NULL,                    /* Security descriptor */
                NULL,                    /* Security QOS */
                DELETE,                  /* Desired access mask */
                0,                       /* Allocation size */
                0,                       /* File attributes */
                FILE_SHARE_READ |
                FILE_SHARE_WRITE |
                FILE_SHARE_DELETE,       /* Share access */
                FILE_OPEN,               /* Create disposition */
                FILE_DELETE_ON_CLOSE,    /* Create options */
                NULL,                    /* EA buffer */
                0,                       /* EA length */
                NULL));                  /* ECP list */
    BAIL_ON_WIN_ERROR(dwError);

    bResult = TRUE;

cleanup:

    if (pszInternalFileName)
    {
        LwFreeString((PSTR)(pszInternalFileName));
    }

    if (hFileHandle)
    {
        LwNtCloseFile(hFileHandle);
    }

    return bResult;

error:

    SetLastError(dwError);

    goto cleanup;
}


BOOL
MoveFile(
    PCWSTR pszFileName,
    PCWSTR pszNewFileName,
    BOOL bOverWrite
    )
{
    DWORD dwError = 0;
    BOOL bResult = FALSE;

    SetLastError(dwError);

    BAIL_ON_WIN_ERROR(dwError);

    bResult = TRUE;

cleanup:

    return bResult;

error:

    SetLastError(dwError);

    goto cleanup;
}


BOOL
CreateDirectory(
    PCWSTR pszDirectoryName
    )
{
    DWORD dwError = 0;
    BOOL bResult = FALSE;
    IO_FILE_HANDLE hFileHandle = NULL;
    IO_STATUS_BLOCK ioStatus = {0};
    IO_FILE_NAME filename = {0};
    PWSTR pszInternalFileName = NULL;

    SetLastError(dwError);

    if (pszDirectoryName == NULL)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN_ERROR(dwError);
    }

    dwError = LwNtStatusToWin32Error(LwIoUncPathToInternalPath(pszDirectoryName, &pszInternalFileName));
    BAIL_ON_WIN_ERROR(dwError);

    filename.FileName = pszInternalFileName;

    dwError = LwNtStatusToWin32Error(LwNtCreateFile(
                &hFileHandle,            /* File handle */
                NULL,                    /* Async control block */
                &ioStatus,               /* IO status block */
                &filename,               /* Filename */
                NULL,                    /* Security descriptor */
                NULL,                    /* Security QOS */
                GENERIC_WRITE,           /* Desired access mask */
                0,                       /* Allocation size */
                0,                       /* File attributes */
                FILE_SHARE_READ |
                FILE_SHARE_WRITE |
                FILE_SHARE_DELETE,       /* Share access */
                FILE_CREATE,             /* Create disposition */
                FILE_DIRECTORY_FILE,     /* Create options */
                NULL,                    /* EA buffer */
                0,                       /* EA length */
                NULL));                  /* ECP list */
    BAIL_ON_WIN_ERROR(dwError);

    bResult = TRUE;

cleanup:

    if (pszInternalFileName)
    {
        LwFreeString((PSTR)(pszInternalFileName));
    }

    if (hFileHandle)
    {
        LwNtCloseFile(hFileHandle);
    }

    return bResult;

error:

    SetLastError(dwError);

    goto cleanup;
}


BOOL
RemoveDirectory(
    PCWSTR pszDirectoryName
    )
{
    DWORD dwError = 0;
    BOOL bResult = FALSE;
    IO_FILE_HANDLE hFileHandle = NULL;
    IO_STATUS_BLOCK ioStatus = {0};
    IO_FILE_NAME filename = {0};
    PWSTR pszInternalFileName = NULL;

    SetLastError(dwError);

    if (pszDirectoryName == NULL)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN_ERROR(dwError);
    }

    dwError = LwNtStatusToWin32Error(LwIoUncPathToInternalPath(pszDirectoryName, &pszInternalFileName));
    BAIL_ON_WIN_ERROR(dwError);

    filename.FileName = pszInternalFileName;

    dwError = LwNtStatusToWin32Error(LwNtCreateFile(
                &hFileHandle,            /* File handle */
                NULL,                    /* Async control block */
                &ioStatus,               /* IO status block */
                &filename,               /* Filename */
                NULL,                    /* Security descriptor */
                NULL,                    /* Security QOS */
                DELETE,                  /* Desired access mask */
                0,                       /* Allocation size */
                0,                       /* File attributes */
                FILE_SHARE_READ |
                FILE_SHARE_WRITE |
                FILE_SHARE_DELETE,       /* Share access */
                FILE_OPEN,               /* Create disposition */
                FILE_DIRECTORY_FILE |
                FILE_DELETE_ON_CLOSE,    /* Create options */
                NULL,                    /* EA buffer */
                0,                       /* EA length */
                NULL));                  /* ECP list */
    BAIL_ON_WIN_ERROR(dwError);

    bResult = TRUE;

cleanup:

    if (pszInternalFileName)
    {
        LwFreeString((PSTR)(pszInternalFileName));
    }

    if (hFileHandle)
    {
        LwNtCloseFile(hFileHandle);
    }

    return bResult;

error:

    SetLastError(dwError);

    goto cleanup;
}


BOOL
RenameFile(
    PCWSTR pszFileName,
    PCWSTR pszNewFileName
    )
{
    DWORD dwError = 0;
    BOOL bResult = FALSE;
    IO_FILE_HANDLE hFileHandle = NULL;
    IO_STATUS_BLOCK ioStatus = {0};
    IO_FILE_NAME filename = {0};
    PWSTR pszInternalFileName = NULL;
    PWSTR pszInternalNewFileName = NULL;

    PFILE_RENAME_INFORMATION pRenameInfo = NULL;
    PWSTR pwszNewPath = NULL;
    size_t newPathLength = 0;

    SetLastError(dwError);

    if (pszFileName == NULL || pszNewFileName == NULL)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN_ERROR(dwError);
    }

    // Original name (/rdr/server/share/foo/cat)
    dwError = LwNtStatusToWin32Error(LwIoUncPathToInternalPath(pszFileName, &pszInternalFileName));
    BAIL_ON_WIN_ERROR(dwError);

    filename.FileName = pszInternalFileName;

    // New name (/rdr/server/share/foo/dog)
    dwError = LwNtStatusToWin32Error(LwIoUncPathToInternalPath(pszNewFileName, &pszInternalNewFileName));
    BAIL_ON_WIN_ERROR(dwError);

    // Lop off the lwio driver prefix (/share/foo/dog)
    for (pwszNewPath = pszInternalNewFileName + 1;
         *pwszNewPath && *pwszNewPath != '/';
         pwszNewPath++);

    dwError = LwWc16sLen(pwszNewPath, &newPathLength);
    BAIL_ON_WIN_ERROR(dwError);

    dwError = LwAllocateMemory(sizeof(FILE_RENAME_INFORMATION) + (newPathLength + 1) * sizeof(WCHAR),
                               (PVOID*) &pRenameInfo);
    BAIL_ON_WIN_ERROR(dwError);

    pRenameInfo->ReplaceIfExists = TRUE;
    pRenameInfo->RootDirectory = NULL;
    pRenameInfo->FileNameLength = newPathLength;

    memcpy(pRenameInfo->FileName, pwszNewPath, newPathLength * sizeof(WCHAR));

    dwError = LwNtStatusToWin32Error(LwNtCreateFile(
                &hFileHandle,            /* File handle */
                NULL,                    /* Async control block */
                &ioStatus,               /* IO status block */
                &filename,               /* Filename */
                NULL,                    /* Security descriptor */
                NULL,                    /* Security QOS */
                DELETE,                  /* Desired access mask */
                0,                       /* Allocation size */
                0,                       /* File attributes */
                FILE_SHARE_READ |
                FILE_SHARE_WRITE |
                FILE_SHARE_DELETE,       /* Share access */
                FILE_OPEN,               /* Create disposition */
                0,                       /* Create options */
                NULL,                    /* EA buffer */
                0,                       /* EA length */
                NULL));                  /* ECP list */
    BAIL_ON_WIN_ERROR(dwError);

    dwError = LwNtStatusToWin32Error(LwNtSetInformationFile(
                hFileHandle,             /* File handle */
                NULL,                    /* Async control block */
                &ioStatus,               /* IO status block */
                pRenameInfo,             /* File information */
                sizeof(FILE_RENAME_INFORMATION) + (newPathLength + 1) * sizeof(WCHAR), /* File information size */
                FileRenameInformation));/* Information class */
    BAIL_ON_WIN_ERROR(dwError);

    bResult = TRUE;

cleanup:

    if (pszInternalFileName)
    {
        LwFreeString((PSTR)(pszInternalFileName));
    }

    if (pszInternalNewFileName)
    {
        LwFreeString((PSTR)(pszInternalNewFileName));
    }

    if (hFileHandle)
    {
        LwNtCloseFile(hFileHandle);
    }

    if (pRenameInfo)
    {
        LwFreeMemory(pRenameInfo);
    }

    return bResult;

error:

    SetLastError(dwError);

    goto cleanup;
}


HANDLE
FindFirstFile(
    PCWSTR pszFilePath,
    PLIKEWISE_FIND_DATA pFindFileData
    )
{
    DWORD dwError = 0;
    PFIND_FILE_HANDLE hFindFile = NULL;
    IO_FILE_NAME filename = {0};
    IO_STATUS_BLOCK ioStatus = {0};
    PFILE_BOTH_DIR_INFORMATION pDirectoryEntry = NULL;
    PWSTR pszInternalPath = NULL;
    BYTE Buffer[MAX_BUFFER] = {0};

    SetLastError(dwError);

    if (pszFilePath == NULL)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN_ERROR(dwError);
    }

    dwError = LwAllocateMemory(sizeof(FIND_FILE_HANDLE), (PVOID*)&hFindFile);
    BAIL_ON_WIN_ERROR(dwError);

    dwError = SplitFindFilePath(pszFilePath,
                                &hFindFile->pszPath,
                                &hFindFile->pszFilter,
                                &hFindFile->bIsWildCard);
    BAIL_ON_WIN_ERROR(dwError);

    dwError = LwNtStatusToWin32Error(LwIoUncPathToInternalPath(hFindFile->pszPath, &pszInternalPath));
    BAIL_ON_WIN_ERROR(dwError);

    filename.FileName = pszInternalPath;

    dwError = LwNtStatusToWin32Error(LwNtCreateFile(
                &hFindFile->hDirHandle,  /* File handle */
                NULL,                    /* Async control block */
                &ioStatus,               /* IO status block */
                &filename,               /* Filename */
                NULL,                    /* Security descriptor */
                NULL,                    /* Security QOS */
                FILE_LIST_DIRECTORY,     /* Desired access mask */
                0,                       /* Allocation size */
                0,                       /* File attributes */
                FILE_SHARE_READ,         /* Share access */
                FILE_OPEN,               /* Create disposition */
                FILE_DIRECTORY_FILE,     /* Create options */
                NULL,                    /* EA buffer */
                0,                       /* EA length */
                NULL));                  /* ECP list */
    BAIL_ON_WIN_ERROR(dwError);

    dwError = GetNextDirectoryEntry(hFindFile,
                                    MAX_BUFFER,
                                    Buffer,
                                    &pDirectoryEntry);
    BAIL_ON_WIN_ERROR(dwError);

    dwError = ConvertDirectoryEntryToWin32FileData(hFindFile,
                                                   pDirectoryEntry,
                                                   pFindFileData);
    BAIL_ON_WIN_ERROR(dwError);

cleanup:

    if (hFindFile)
    {
        return (HANDLE) hFindFile;
    }
    else
    {
        return (HANDLE) -1;
    }

error:

    // Clean up any resources the FindFile handle contains by calling FindClose
    FindClose((HANDLE)hFindFile);
    hFindFile = NULL;

    SetLastError(dwError);

    goto cleanup;
}


BOOL
FindNextFile(
    HANDLE hFindHandle,
    PLIKEWISE_FIND_DATA pFindFileData
    )
{
    DWORD dwError = 0;
    BOOL bResult = FALSE;
    PFIND_FILE_HANDLE hFindFile = (PFIND_FILE_HANDLE) hFindHandle;
    PFILE_BOTH_DIR_INFORMATION pDirectoryEntry = NULL;
    BYTE Buffer[MAX_BUFFER];

    SetLastError(dwError);

    if (hFindFile == NULL)
    {
        dwError = ERROR_INVALID_HANDLE;
        BAIL_ON_WIN_ERROR(dwError);
    }

    dwError = GetNextDirectoryEntry(hFindFile,
                                    MAX_BUFFER,
                                    Buffer,
                                    &pDirectoryEntry);
    BAIL_ON_WIN_ERROR(dwError);

    dwError = ConvertDirectoryEntryToWin32FileData(hFindFile,
                                                   pDirectoryEntry,
                                                   pFindFileData);
    BAIL_ON_WIN_ERROR(dwError);

    bResult = TRUE;

cleanup:

    return bResult;

error:

    SetLastError(dwError);

    goto cleanup;
}


BOOL
FindClose(
    HANDLE hFindHandle
    )
{
    DWORD dwError = 0;
    PFIND_FILE_HANDLE hFindFile = (PFIND_FILE_HANDLE) hFindHandle;
    BOOL bResult = FALSE;

    SetLastError(dwError);

    if (hFindFile == NULL)
    {
        dwError = ERROR_INVALID_HANDLE;
        BAIL_ON_WIN_ERROR(dwError);
    }

    if (hFindFile->hDirHandle)
    {
        LwNtCloseFile(hFindFile->hDirHandle);
    }

    if (hFindFile->pszPath)
    {
        LwFreeString((PSTR)(hFindFile->pszPath));
    }

    if (hFindFile->pszFilter)
    {
        LwFreeString((PSTR)(hFindFile->pszFilter));
    }

    LwFreeMemory(hFindFile);

    bResult = TRUE;

cleanup:

    return bResult;

error:

    SetLastError(dwError);

    goto cleanup;
}


DWORD
FileTimeToSystemTime(
    const time_t * pFileTime,
    SYSTEM_TIME * pSystemTime
    )
{
    DWORD dwError = 0;
    struct tm * tmFileTime = gmtime(pFileTime);

    if (tmFileTime == NULL)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN_ERROR(dwError);
    }

    // Adjust for DST
    tmFileTime->tm_hour += tmFileTime->tm_isdst;

    pSystemTime->wYear = tmFileTime->tm_year + 1900;
    pSystemTime->wMonth = tmFileTime->tm_mon + 1;
    pSystemTime->wDay = tmFileTime->tm_mday;
    pSystemTime->wHour = tmFileTime->tm_hour;
    pSystemTime->wMinute = tmFileTime->tm_min;
    pSystemTime->wSecond = tmFileTime->tm_sec;
    pSystemTime->wMilliseconds = 0;

cleanup:

    return dwError;

error:

    goto cleanup;
}


