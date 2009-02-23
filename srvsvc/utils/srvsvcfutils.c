/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software 2007
 * All rights reserved.
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 * Server Service file utilities
 *
 */
#include "includes.h"


DWORD
SRVSVCRemoveFile(
    PCSTR pszPath
    )
{
    DWORD dwError = 0;

    while (1) {
        if (unlink(pszPath) < 0) {
            if (errno == EINTR) {
                continue;
            }
            dwError = errno;
            BAIL_ON_SRVSVC_ERROR(dwError);
        } else {
            break;
        }
    }

error:

    return dwError;
}

DWORD
SRVSVCCheckFileExists(
    PCSTR pszPath,
    PBOOLEAN pbFileExists
    )
{
    DWORD dwError = 0;

    struct stat statbuf;

    memset(&statbuf, 0, sizeof(struct stat));

    while (1) {
        if (stat(pszPath, &statbuf) < 0) {
            if (errno == EINTR) {
                continue;
            } else if (errno == ENOENT) {
             *pbFileExists = 0;
             break;
            }
            dwError = errno;
            BAIL_ON_SRVSVC_ERROR(dwError);
        } else {
            *pbFileExists = 1;
            break;
        }
    }

error:

    return dwError;
}

DWORD
SRVSVCGetFileSize(
    PCSTR pszPath,
    PDWORD pdwFileSize
    )
{
    DWORD dwError = 0;

    struct stat statbuf;

    memset(&statbuf, 0, sizeof(struct stat));

    while (1) {
        if (stat(pszPath, &statbuf) < 0) {
            if (errno == EINTR) {
                continue;
            } else if (errno == ENOENT) {
             *pdwFileSize = 0;
             break;
            }
            dwError = errno;
            BAIL_ON_SRVSVC_ERROR(dwError);
        } else {
            *pdwFileSize = statbuf.st_size;
            break;
        }
    }

error:

    return dwError;
}

DWORD
SRVSVCMoveFile(
    PCSTR pszSrcPath,
    PCSTR pszDstPath
    )
{
    DWORD dwError = 0;

    if (rename(pszSrcPath, pszDstPath) < 0) {
        dwError = errno;
    }

    return dwError;
}

DWORD
SRVSVCChangePermissions(
    PCSTR pszPath,
    mode_t dwFileMode
    )
{
    DWORD dwError = 0;

    while (1) {
        if (chmod(pszPath, dwFileMode) < 0) {
            if (errno == EINTR) {
                continue;
            }
            dwError = errno;
            BAIL_ON_SRVSVC_ERROR(dwError);
        } else {
            break;
        }
    }

error:

    return dwError;
}

DWORD
SRVSVCChangeOwner(
    PCSTR pszPath,
    uid_t uid,
    gid_t gid
    )
{
    DWORD dwError = 0;

    while (1) {
        if (chown(pszPath, uid, gid) < 0) {
            if (errno == EINTR) {
                continue;
            }
            dwError = errno;
            BAIL_ON_SRVSVC_ERROR(dwError);
        } else {
            break;
        }
    }

error:

    return dwError;
}

DWORD
SRVSVCChangeOwnerAndPermissions(
    PCSTR pszPath,
    uid_t uid,
    gid_t gid,
    mode_t dwFileMode
    )
{
    DWORD dwError = 0;

    dwError = SRVSVCChangeOwner(pszPath, uid, gid);
    BAIL_ON_SRVSVC_ERROR(dwError);

    dwError = SRVSVCChangePermissions(pszPath, dwFileMode);
    BAIL_ON_SRVSVC_ERROR(dwError);

error:

    return dwError;
}

DWORD
SRVSVCChangeDirectory(
    PCSTR pszPath
    )
{
    if (pszPath == NULL || *pszPath == '\0')
        return EINVAL;

    if (chdir(pszPath) < 0)
        return errno;

    return 0;
}

/*
// TODO: Check access and removability before actual deletion
*/
DWORD
SRVSVCRemoveDirectory(
    PCSTR pszPath
    )
{
    DWORD dwError = 0;
    DIR* pDir = NULL;
    struct dirent* pDirEntry = NULL;
    struct stat statbuf;
    CHAR szBuf[PATH_MAX+1];

    if ((pDir = opendir(pszPath)) == NULL) {
        dwError = errno;
        BAIL_ON_SRVSVC_ERROR(dwError);
    }

    while ((pDirEntry = readdir(pDir)) != NULL) {

        if (!strcmp(pDirEntry->d_name, "..") ||
            !strcmp(pDirEntry->d_name, "."))
            continue;

        sprintf(szBuf, "%s/%s", pszPath, pDirEntry->d_name);

        memset(&statbuf, 0, sizeof(struct stat));

        if (stat(szBuf, &statbuf) < 0) {
            dwError = errno;
            BAIL_ON_SRVSVC_ERROR(dwError);
        }

        if ((statbuf.st_mode & S_IFMT) == S_IFDIR) {
            dwError = SRVSVCRemoveDirectory(szBuf);
            BAIL_ON_SRVSVC_ERROR(dwError);

            if (rmdir(szBuf) < 0) {
                dwError = errno;
                BAIL_ON_SRVSVC_ERROR(dwError);
            }

        } else {

            dwError = SRVSVCRemoveFile(szBuf);
            BAIL_ON_SRVSVC_ERROR(dwError);

        }
    }

error:

    if (pDir)
        closedir(pDir);

    return dwError;
}

DWORD
SRVSVCCheckDirectoryExists(
    PCSTR pszPath,
    PBOOLEAN pbDirExists
    )
{
    DWORD dwError = 0;

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
            dwError = errno;
            BAIL_ON_SRVSVC_ERROR(dwError);

        }

        /*
            The path exists. Is it a directory?
         */

        *pbDirExists = (((statbuf.st_mode & S_IFMT) == S_IFDIR) ? TRUE : FALSE);
        break;
    }

error:

    return dwError;
}

DWORD
SRVSVCGetCurrentDirectoryPath(
    PSTR* ppszPath
    )
{
    DWORD dwError = 0;
    CHAR szBuf[PATH_MAX+1];
    PSTR pszPath = NULL;

    if (getcwd(szBuf, PATH_MAX) == NULL) {
        dwError = errno;
        BAIL_ON_SRVSVC_ERROR(dwError);
    }

    dwError = SRVSVCAllocateString(szBuf, &pszPath);
    BAIL_ON_SRVSVC_ERROR(dwError);

    *ppszPath = pszPath;

    return dwError;

error:

    if (pszPath) {
        SRVSVCFreeString(pszPath);
    }

    return dwError;
}

static
DWORD
SRVSVCCreateDirectoryRecursive(
    PCSTR pszCurDirPath,
    PSTR pszTmpPath,
    PSTR *ppszTmp,
    DWORD dwFileMode,
    DWORD dwWorkingFileMode,
    int  iPart
    )
{
    DWORD dwError = 0;
    PSTR pszDirPath = NULL;
    BOOLEAN bDirCreated = FALSE;
    BOOLEAN bDirExists = FALSE;
    CHAR szDelimiters[] = "/";

    PSTR pszToken = strtok_r((iPart ? NULL : pszTmpPath), szDelimiters, ppszTmp);

    if (pszToken != NULL) {

        dwError = SRVSVCAllocateMemory(strlen(pszCurDirPath)+strlen(pszToken)+2,
                                    (PVOID*)&pszDirPath);
        BAIL_ON_SRVSVC_ERROR(dwError);

        sprintf(pszDirPath,
                "%s/%s",
                (!strcmp(pszCurDirPath, "/") ? "" : pszCurDirPath),
                pszToken);

        dwError = SRVSVCCheckDirectoryExists(pszDirPath, &bDirExists);
        BAIL_ON_SRVSVC_ERROR(dwError);

        if (!bDirExists) {
            if (mkdir(pszDirPath, dwWorkingFileMode) < 0) {
                dwError = errno;
                BAIL_ON_SRVSVC_ERROR(dwError);
            }
            bDirCreated = TRUE;
        }

        dwError = SRVSVCChangeDirectory(pszDirPath);
        BAIL_ON_SRVSVC_ERROR(dwError);

        dwError = SRVSVCCreateDirectoryRecursive(
            pszDirPath,
            pszTmpPath,
            ppszTmp,
            dwFileMode,
            dwWorkingFileMode,
            iPart+1
            );
        BAIL_ON_SRVSVC_ERROR(dwError);
    }

    if (bDirCreated && (dwFileMode != dwWorkingFileMode)) {
        dwError = SRVSVCChangePermissions(pszDirPath, dwFileMode);
        BAIL_ON_SRVSVC_ERROR(dwError);
    }
    if (pszDirPath) {
        SRVSVCFreeMemory(pszDirPath);
    }

    return dwError;

error:

    if (bDirCreated) {
        SRVSVCRemoveDirectory(pszDirPath);
    }

    if (pszDirPath) {
        SRVSVCFreeMemory(pszDirPath);
    }

    return dwError;
}

DWORD
SRVSVCCreateDirectory(
    PCSTR pszPath,
    mode_t dwFileMode
    )
{
    DWORD dwError = 0;
    PSTR pszCurDirPath = NULL;
    PSTR pszTmpPath = NULL;
    PSTR pszTmp = NULL;
    mode_t dwWorkingFileMode;

    if (pszPath == NULL || *pszPath == '\0') {
        dwError = EINVAL;
        BAIL_ON_SRVSVC_ERROR(dwError);
    }

    dwWorkingFileMode = dwFileMode;
    if (!(dwFileMode & S_IXUSR)) {
        /*
         * This is so that we can navigate the folders
         * when we are creating the subfolders
         */
        dwWorkingFileMode |= S_IXUSR;
    }

    dwError = SRVSVCGetCurrentDirectoryPath(&pszCurDirPath);
    BAIL_ON_SRVSVC_ERROR(dwError);

    dwError = SRVSVCAllocateString(pszPath, &pszTmpPath);
    BAIL_ON_SRVSVC_ERROR(dwError);

    if (*pszPath == '/') {
        dwError = SRVSVCChangeDirectory("/");
        BAIL_ON_SRVSVC_ERROR(dwError);

        dwError = SRVSVCCreateDirectoryRecursive("/", pszTmpPath, &pszTmp, dwFileMode, dwWorkingFileMode, 0);
        BAIL_ON_SRVSVC_ERROR(dwError);

    } else {

        dwError = SRVSVCCreateDirectoryRecursive(pszCurDirPath, pszTmpPath, &pszTmp, dwFileMode, dwWorkingFileMode, 0);
        BAIL_ON_SRVSVC_ERROR(dwError);

    }

error:

    if (pszCurDirPath) {

        SRVSVCChangeDirectory(pszCurDirPath);

        SRVSVCFreeMemory(pszCurDirPath);

    }

    if (pszTmpPath) {
        SRVSVCFreeMemory(pszTmpPath);
    }

    return dwError;
}

