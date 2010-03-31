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
 *       Fileio.c
 *
 * Abstract:
 *
 *        Likewise File Client API
 *
 *        Remote and local file enumeration and move, copy, delete operations
 *
 * Authors:
 */

#include "includes.h"


DWORD
SplitFindFilePath(
    PCWSTR pszFilePath,
    PWSTR * ppszPath,
    PWSTR * ppszFilter,
    PBOOL pbIsWildCard
    )
{
    DWORD dwError = 0;
    PWSTR pszPath = NULL;
    PWSTR pszFilter = NULL;
    size_t sPathLen = 0;
    size_t sIndex = 0;
    BOOL bIsWildCard = FALSE;

    if (pszFilePath == NULL)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN_ERROR(dwError);
    }

    dwError = LwWc16sLen(pszFilePath, &sPathLen);
    BAIL_ON_WIN_ERROR(dwError);

    sIndex = sPathLen - 1;

    /* Test for invalid search paths.
       No partial file names trailing with dot, or directory slashes */
    if (sIndex == 0 ||
        pszFilePath[sIndex] == '.' ||
        pszFilePath[sIndex] == '\\' ||
        pszFilePath[sIndex] == '/' )
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN_ERROR(dwError);
    }

    /* Walk from the end of path looking for signs of a wildcard filter in last path item */
    while (sIndex)
    {
        if (pszFilePath[sIndex] == '/' ||
            pszFilePath[sIndex] == '\\')
        {
            // Scanned back to directory/share separator
            break;
        }

        if (pszFilePath[sIndex] == '*')
        {
            bIsWildCard = TRUE;
        }

        sIndex--;
    }

    if (sIndex == 0)
    {
        /* The path as entered appears to have a search filter at the root */
        dwError = LwAllocateWc16String(&pszPath, pszFilePath);
        BAIL_ON_WIN_ERROR(dwError);

        pszPath[1] = '\0';
        *ppszPath = pszPath;
        pszPath = NULL;

        dwError = LwAllocateWc16String(&pszFilter, &pszFilePath[sIndex+1]);
        BAIL_ON_WIN_ERROR(dwError);

        *ppszFilter = pszFilter;
        pszFilter = NULL;

        *pbIsWildCard = bIsWildCard;
    }
    else
    {
        /* The path as entered appears to have a search filter for a sub path node */
        dwError = LwAllocateWc16String(&pszPath, pszFilePath);
        BAIL_ON_WIN_ERROR(dwError);

        pszPath[sIndex] = '\0';
        *ppszPath = pszPath;
        pszPath = NULL;

        dwError = LwAllocateWc16String(&pszFilter, &pszFilePath[sIndex+1]);
        BAIL_ON_WIN_ERROR(dwError);

        *ppszFilter = pszFilter;
        pszFilter = NULL;

        *pbIsWildCard = bIsWildCard;
    }

cleanup:

    return dwError;

error:

    if (pszPath)
    {
        LwFreeString((PSTR)pszPath);
    }

    if (pszFilter)
    {
        LwFreeString((PSTR)pszFilter);
    }

    goto cleanup;
}


DWORD
GetNextDirectoryEntry(
    PFIND_FILE_HANDLE hFindFile,
    DWORD dwBufferSize,
    PBYTE pBuffer,
    PFILE_BOTH_DIR_INFORMATION * ppDirectoryEntry
    )
{
    DWORD dwError = 0;
    BYTE buffer[MAX_BUFFER] = {0};
    IO_STATUS_BLOCK ioStatus = {0};

    while (TRUE)
    {
        dwError = LwNtStatusToWin32Error(LwNtQueryDirectoryFile(
                    hFindFile->hDirHandle,        /* File handle */
                    NULL,                         /* Async control block */
                    &ioStatus,                    /* IO status block */
                    buffer,                       /* Info structure */
                    dwBufferSize,                 /* Info structure size */
                    FileBothDirectoryInformation, /* Info level */
                    TRUE,                         /* Return single entry */
                    NULL,                         /* File spec */
                    FALSE));                      /* Restart scan */
        BAIL_ON_WIN_ERROR(dwError);

        if (DirectoryEntryMatchesFilter((PFILE_BOTH_DIR_INFORMATION) buffer,
                                        hFindFile->pszFilter,
                                        hFindFile->bIsWildCard))
        {
            memcpy(pBuffer, buffer, ioStatus.BytesTransferred);
            *ppDirectoryEntry = (PFILE_BOTH_DIR_INFORMATION) pBuffer;
            break;
        }
    }

cleanup:

    return dwError;

error:

    goto cleanup;
}


BOOL
DirectoryEntryMatchesFilter(
    PFILE_BOTH_DIR_INFORMATION pFileInfo,
    PWSTR pszFilter,
    BOOL bIsWildCard
    )
{
    BOOL bIsMatch = FALSE;

    if (!bIsWildCard &&
        pFileInfo->FileNameLength &&
        pszFilter &&
        !wc16scmp(pFileInfo->FileName, pszFilter))
    {
        bIsMatch = TRUE;
    }

    if (bIsWildCard)
    {
        /* TODO: Apply specific filter to current directory entry filename */
        bIsMatch = TRUE;
    }

    return bIsMatch;
}


DWORD
ConvertDirectoryEntryToWin32FileData(
    PFIND_FILE_HANDLE hFindFile,
    PFILE_BOTH_DIR_INFORMATION pDirectoryEntry,
    PLIKEWISE_FIND_DATA pFindData
    )
{
    DWORD dwError = STATUS_SUCCESS;

    if (pDirectoryEntry == NULL || pFindData == NULL)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN_ERROR(dwError);
    }

    memset(pFindData, 0, sizeof(LIKEWISE_FIND_DATA));

    pFindData->FileAttributes = pDirectoryEntry->FileAttributes;
    WinTimeToUnixTime(pDirectoryEntry->CreationTime, &pFindData->ftCreationTime);
    WinTimeToUnixTime(pDirectoryEntry->LastAccessTime, &pFindData->ftLastAccessTime);
    WinTimeToUnixTime(pDirectoryEntry->LastWriteTime, &pFindData->ftLastWriteTime);

    if ((pFindData->FileAttributes & FILE_LIST_DIRECTORY) ==
        FILE_LIST_DIRECTORY)
    {
        pFindData->nFileSizeLow = 0;
        pFindData->nFileSizeHigh = 0;
    }
    else
    {
        pFindData->nFileSizeLow = pDirectoryEntry->EndOfFile;
        pFindData->nFileSizeHigh = 0;
    }

    if (pDirectoryEntry->FileNameLength > 0 &&
        pDirectoryEntry->FileNameLength < MAX_FILENAME_PATH)
    {
        dwError = LwAllocateMemory((pDirectoryEntry->FileNameLength + 1) * sizeof(WCHAR), (PVOID*)&pFindData->FileName);
        BAIL_ON_WIN_ERROR(dwError);

        dwError = LwWc16snCpy(pFindData->FileName,
                              pDirectoryEntry->FileName,
                              pDirectoryEntry->FileNameLength);
        BAIL_ON_WIN_ERROR(dwError);
    }
    else
    {
        dwError = ERROR_INVALID_NAME;
        BAIL_ON_WIN_ERROR(dwError);
    }

    if (pDirectoryEntry->ShortNameLength > 0 &&
        pDirectoryEntry->ShortNameLength < MAX_ALTERNATE_NAME)
    {
        dwError = LwAllocateMemory((pDirectoryEntry->ShortNameLength + 1) * sizeof(WCHAR), (PVOID*)&pFindData->Alternate);
        BAIL_ON_WIN_ERROR(dwError);

        dwError = LwWc16snCpy(pFindData->Alternate,
                              pDirectoryEntry->ShortName,
                              pDirectoryEntry->ShortNameLength);
        BAIL_ON_WIN_ERROR(dwError);
    }

cleanup:

    return dwError;

error:

    goto cleanup;
}


DWORD
IsPathToDirectory(
    PWSTR pszFilePath,
    PBOOL pbIsDirectory
    )
{
    DWORD dwError = 0;
    IO_FILE_HANDLE hDirHandle = NULL;
    IO_FILE_NAME filename = {0};
    IO_STATUS_BLOCK ioStatus = {0};
    FILE_STANDARD_INFORMATION fileStdInfo = {0};

    if (pszFilePath == NULL)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN_ERROR(dwError);
    }

    filename.FileName = pszFilePath;

    dwError = LwNtStatusToWin32Error(LwNtCreateFile(
                &hDirHandle,     /* File handle */
                NULL,            /* Async control block */
                &ioStatus,       /* IO status block */
                &filename,       /* Filename */
                NULL,            /* Security descriptor */
                NULL,            /* Security QOS */
                FILE_READ_DATA,  /* Desired access mask */
                0,               /* Allocation size */
                0,               /* File attributes */
                FILE_SHARE_READ, /* Share access */
                FILE_OPEN,       /* Create disposition */
                0,               /* Create options */
                NULL,            /* EA buffer */
                0,               /* EA length */
                NULL));          /* ECP list */
    BAIL_ON_WIN_ERROR(dwError);

    dwError = LwNtStatusToWin32Error(LwNtQueryInformationFile(
                hDirHandle,
                NULL,
                &ioStatus,
                (PVOID*)&fileStdInfo,
                sizeof(fileStdInfo),
                FileStandardInformation));
    BAIL_ON_WIN_ERROR(dwError);

    if (fileStdInfo.Directory)
    {
        *pbIsDirectory = TRUE;
    }
    else
    {
        *pbIsDirectory = FALSE;
    }

cleanup:

    if (hDirHandle)
    {
        LwNtCloseFile(hDirHandle);
    }

    return dwError;

error:

    goto cleanup;
}


DWORD
WinTimeToUnixTime(
    LONG64 WinTime,
    time_t * pUnixTime
    )
{
    DWORD dwError = 0;

    if (pUnixTime == NULL)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN_ERROR(dwError);
    }

    *pUnixTime = (WinTime / 10000000LL) - TIME_SEC_CONVERSION_CONSTANT;

cleanup:

    return dwError;

error:

    goto cleanup;
}


