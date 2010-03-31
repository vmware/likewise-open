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
 *       Fileio
 *
 * Abstract:
 *
 *        Likewise File Browser Client
 *
 *
 *
 * Authors:
 */

#ifndef __FILE_IO_H__
#define __FILE_IO_H__


#define MAX_BUFFER 4096
#define TIME_SEC_CONVERSION_CONSTANT 11644473600LL


#define BAIL_ON_WIN_ERROR(err)    \
    if ((err) != ERROR_SUCCESS) { \
        goto error;               \
    }

typedef struct _FIND_FILE_HANDLE
{
    PWSTR pszPath;
    PWSTR pszFilter;
    BOOL  bIsWildCard;
    IO_FILE_HANDLE hDirHandle;
} FIND_FILE_HANDLE, *PFIND_FILE_HANDLE;


DWORD
SplitFindFilePath(
    PCWSTR pszFilePath,
    PWSTR * ppszPath,
    PWSTR * ppszFilter,
    PBOOL pbIsWildCard
    );

DWORD
GetNextDirectoryEntry(
    PFIND_FILE_HANDLE hFindFile,
    DWORD dwBufferSize,
    PBYTE pBuffer,
    PFILE_BOTH_DIR_INFORMATION * ppDirectoryEntry
    );

BOOL
DirectoryEntryMatchesFilter(
    PFILE_BOTH_DIR_INFORMATION pFileInfo,
    PWSTR pszFilter,
    BOOL bIsWildCard
    );

DWORD
ConvertDirectoryEntryToWin32FileData(
    PFIND_FILE_HANDLE hFindFile,
    PFILE_BOTH_DIR_INFORMATION pDirectoryEntry,
    PLIKEWISE_FIND_DATA pFindData
    );

DWORD
IsPathToDirectory(
    PWSTR pszFilePath,
    PBOOL pbIsDirectory
    );

DWORD
WinTimeToUnixTime(
    LONG64 WinTime,
    time_t * pUnixTime
    );


#endif

