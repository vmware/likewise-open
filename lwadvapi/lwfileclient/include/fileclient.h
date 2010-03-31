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
 *       FileClient.h
 *
 * Abstract:
 *
 *        Likewise File Client library
 *
 *        Public Client API
 *
 * Authors:
 */


#ifndef __LWFILECLIENT_H__
#define __LWFILECLIENT_H__

#if HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#include <krb5.h>
#include "lwio/lwio.h"
#include "lwio/ntfileapi.h"
#include "lwnet.h"
#include <lw/types.h>
#include <lw/attrs.h>


#define MAX_FILENAME_PATH 260
#define MAX_ALTERNATE_NAME 14

typedef struct
{
    FILE_ATTRIBUTES FileAttributes;
    time_t ftCreationTime;
    time_t ftLastAccessTime;
    time_t ftLastWriteTime;
    DWORD nFileSizeHigh;
    DWORD nFileSizeLow;
    DWORD dwReserved0;
    DWORD dwReserved1;
    PWCHAR FileName;  // Size MAX_FILENAME_PATH
    PWCHAR Alternate; // Size MAX_ALTERNATE_NAME
}LIKEWISE_FIND_DATA, *PLIKEWISE_FIND_DATA;

typedef struct
{
    WORD wYear;
    WORD wMonth;
    WORD wDayOfWeek;
    WORD wDay;
    WORD wHour;
    WORD wMinute;
    WORD wSecond;
    WORD wMilliseconds;
}SYSTEM_TIME, *PSYSTEM_TIME;


DWORD
GetLastError(
    );

BOOL
CopyFile(
    PCWSTR pszFileName,
    PCWSTR pszNewFileName,
    BOOL bOverWrite
    );

BOOL
DeleteFile(
    PCWSTR pszFileName
    );

BOOL
MoveFile(
    PCWSTR pszFileName,
    PCWSTR pszNewFileName,
    BOOL bOverWrite
    );

BOOL
CreateDirectory(
    PCWSTR pszDirectoryName
    );

BOOL
RemoveDirectory(
    PCWSTR pszDirectoryName
    );

BOOL
RenameFile(
    PCWSTR pszFileName,
    PCWSTR pszNewFileName
    );

HANDLE
FindFirstFile(
    PCWSTR pszFilePath,
    PLIKEWISE_FIND_DATA pFindData
    );

BOOL
FindNextFile(
    HANDLE hFindHandle,
    PLIKEWISE_FIND_DATA pFindData
    );

BOOL
FindClose(
    HANDLE hFindHandle
    );

DWORD
FileTimeToSystemTime(
    const time_t * pFileTime,
    SYSTEM_TIME *  pSystemTime
    );


#endif
