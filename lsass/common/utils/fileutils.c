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
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        lsafutils.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS) File Utilities
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */

#include "includes.h"

DWORD
LsaRemoveFile(
    PCSTR pszPath
    )
{
    DWORD dwError = 0;

    while (1) {
        if (unlink(pszPath) < 0) {
            if (errno == EINTR) {
                continue;
            }
            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_LSA_ERROR(dwError);
        } else {
            break;
        }
    }

error:

    return dwError;
}

DWORD
LsaCheckFileExists(
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
           dwError = LwMapErrnoToLwError(errno);
           BAIL_ON_LSA_ERROR(dwError);
        } else {
          *pbFileExists = 1;
          break;
        }
    }

error:

    return dwError;
}

DWORD
LsaCheckSockExists(
    PSTR pszPath,
    PBOOLEAN pbSockExists
    )
{
    DWORD dwError = 0;

    struct stat statbuf;

    memset(&statbuf, 0, sizeof(struct stat));

    while (1) {
        if (stat(pszPath, &statbuf) < 0) {
           if (errno == EINTR) {
              continue;
           } else if (errno == ENOENT || errno == ENOTDIR) {
             *pbSockExists = 0;
             break;
           }
           dwError = LwMapErrnoToLwError(errno);
           BAIL_ON_LSA_ERROR(dwError);
        } else {
          *pbSockExists = (((statbuf.st_mode & S_IFMT) == S_IFSOCK) ? TRUE : FALSE);
          break;
        }
    }

error:

    return dwError;
}

DWORD
LsaCheckLinkExists(
    PSTR pszPath,
    PBOOLEAN pbLinkExists
    )
{
    DWORD dwError = 0;
    BOOLEAN bExists = FALSE;

    struct stat statbuf;

    memset(&statbuf, 0, sizeof(struct stat));

    while (1)
    {
        if (stat(pszPath, &statbuf) < 0)
        {
           if (errno == EINTR)
           {
              continue;
           }
           else if (errno == ENOENT || errno == ENOTDIR)
           {
             *pbLinkExists = 0;
             break;
           }

           dwError = LwMapErrnoToLwError(errno);
           BAIL_ON_LSA_ERROR(dwError);
        }
        else
        {
           if (((statbuf.st_mode & S_IFMT) == S_IFLNK))
           {
               bExists = TRUE;
           }
          break;
        }
    }

error:

    *pbLinkExists = bExists;

    return dwError;
}

DWORD
LsaCheckFileOrLinkExists(
    PSTR pszPath,
    PBOOLEAN pbExists
    )
{
    DWORD dwError = 0;
    BOOLEAN bExists = FALSE;

    struct stat statbuf;

    memset(&statbuf, 0, sizeof(struct stat));

    while (1)
    {
        if (stat(pszPath, &statbuf) < 0)
        {
           if (errno == EINTR)
           {
              continue;
           }
           else if (errno == ENOENT || errno == ENOTDIR)
           {
             break;
           }

           dwError = LwMapErrnoToLwError(errno);
           BAIL_ON_LSA_ERROR(dwError);
        }
        else
        {
          if (((statbuf.st_mode & S_IFMT) == S_IFLNK) ||
              ((statbuf.st_mode & S_IFMT) == S_IFREG))
          {
              bExists = TRUE;
          }
          break;
        }
    }

error:

    *pbExists = bExists;

    return dwError;
}

DWORD
LsaMoveFile(
    PCSTR pszSrcPath,
    PCSTR pszDstPath
    )
{
    DWORD dwError = 0;

    if (rename(pszSrcPath, pszDstPath) < 0) {
        dwError = LwMapErrnoToLwError(errno);
    }

    return dwError;
}

DWORD
LsaChangePermissions(
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
            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_LSA_ERROR(dwError);
        } else {
            break;
        }
    }

error:

    return dwError;
}

DWORD
LsaChangeOwner(
    PCSTR pszPath,
    uid_t uid,
    gid_t gid
    )
{
    DWORD dwError = 0;
    struct stat statbuf = {0};

    if (lstat(pszPath, &statbuf) < 0) {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_LSA_ERROR(dwError);
    }

    while (1) {

        if (S_ISLNK(statbuf.st_mode)) {

            if (lchown(pszPath, uid, gid) < 0) {
                if (errno == EINTR) {
                    continue;
                }
                dwError = LwMapErrnoToLwError(errno);
                BAIL_ON_LSA_ERROR(dwError);
            } else {
                break;
            }

        } else {

            if (chown(pszPath, uid, gid) < 0) {
                if (errno == EINTR) {
                    continue;
                }
                dwError = LwMapErrnoToLwError(errno);
                BAIL_ON_LSA_ERROR(dwError);
            } else {
                break;
            }
        }
    }

error:

    return dwError;
}

DWORD
LsaChangeOwnerAndPermissions(
    PCSTR pszPath,
    uid_t uid,
    gid_t gid,
    mode_t dwFileMode
    )
{
    DWORD dwError = 0;

    dwError = LsaChangeOwner(pszPath, uid, gid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaChangePermissions(pszPath, dwFileMode);
    BAIL_ON_LSA_ERROR(dwError);

error:

    return dwError;
}

DWORD
LsaChangeDirectory(
    PSTR pszPath
    )
{
    if (pszPath == NULL || *pszPath == '\0')
        return LW_ERROR_INVALID_PARAMETER;

    if (chdir(pszPath) < 0)
        return LwMapErrnoToLwError(errno);

    return 0;
}

/*
// TODO: Check access and removability before actual deletion
*/
DWORD
LsaRemoveDirectory(
    PSTR pszPath
    )
{
    DWORD dwError = 0;
    DIR* pDir = NULL;
    struct dirent* pDirEntry = NULL;
    struct stat statbuf;
    CHAR szBuf[PATH_MAX+1];

    if ((pDir = opendir(pszPath)) == NULL) {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_LSA_ERROR(dwError);
    }

    while ((pDirEntry = readdir(pDir)) != NULL) {

        if (!strcmp(pDirEntry->d_name, "..") ||
            !strcmp(pDirEntry->d_name, "."))
            continue;

        sprintf(szBuf, "%s/%s", pszPath, pDirEntry->d_name);

        memset(&statbuf, 0, sizeof(struct stat));

        if (stat(szBuf, &statbuf) < 0) {
            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_LSA_ERROR(dwError);
        }

        if ((statbuf.st_mode & S_IFMT) == S_IFDIR) {
            dwError = LsaRemoveDirectory(szBuf);
            BAIL_ON_LSA_ERROR(dwError);

            if (rmdir(szBuf) < 0) {
                dwError = LwMapErrnoToLwError(errno);
                BAIL_ON_LSA_ERROR(dwError);
            }

        } else {

            dwError = LsaRemoveFile(szBuf);
            BAIL_ON_LSA_ERROR(dwError);

        }
    }

error:

    if (pDir)
        closedir(pDir);

    return dwError;
}

DWORD
LsaCheckDirectoryExists(
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
            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_LSA_ERROR(dwError);

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
LsaGetCurrentDirectoryPath(
    PSTR* ppszPath
    )
{
    DWORD dwError = 0;
    CHAR szBuf[PATH_MAX+1];
    PSTR pszPath = NULL;

    if (getcwd(szBuf, PATH_MAX) == NULL) {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwAllocateString(szBuf, &pszPath);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszPath = pszPath;

    return dwError;

error:

    if (pszPath) {
        LwFreeString(pszPath);
    }

    return dwError;
}

static
DWORD
LsaCreateDirectoryRecursive(
    PSTR pszCurDirPath,
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

        dwError = LwAllocateMemory(strlen(pszCurDirPath)+strlen(pszToken)+2,
                                   (PVOID*)&pszDirPath);
        BAIL_ON_LSA_ERROR(dwError);

        sprintf(pszDirPath,
                "%s/%s",
                (!strcmp(pszCurDirPath, "/") ? "" : pszCurDirPath),
                pszToken);


        dwError = LsaCheckDirectoryExists(pszDirPath, &bDirExists);
        BAIL_ON_LSA_ERROR(dwError);

        if (!bDirExists) {
            if (mkdir(pszDirPath, dwWorkingFileMode) < 0) {
                dwError = LwMapErrnoToLwError(errno);
                BAIL_ON_LSA_ERROR(dwError);
            }
            bDirCreated = TRUE;
        }

        dwError = LsaChangeDirectory(pszDirPath);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaCreateDirectoryRecursive(
            pszDirPath,
            pszTmpPath,
            ppszTmp,
            dwFileMode,
            dwWorkingFileMode,
            iPart+1
            );
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (bDirCreated && (dwFileMode != dwWorkingFileMode)) {
        dwError = LsaChangePermissions(pszDirPath, dwFileMode);
        BAIL_ON_LSA_ERROR(dwError);
    }
    if (pszDirPath) {
        LwFreeMemory(pszDirPath);
    }

    return dwError;

error:

    if (bDirCreated) {
        LsaRemoveDirectory(pszDirPath);
    }

    if (pszDirPath) {
        LwFreeMemory(pszDirPath);
    }

    return dwError;
}

DWORD
LsaCreateDirectory(
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
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwWorkingFileMode = dwFileMode;
    if (!(dwFileMode & S_IXUSR)) {
        /*
         * This is so that we can navigate the folders
         * when we are creating the subfolders
         */
        dwWorkingFileMode |= S_IXUSR;
    }

    dwError = LsaGetCurrentDirectoryPath(&pszCurDirPath);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateString(pszPath, &pszTmpPath);
    BAIL_ON_LSA_ERROR(dwError);

    if (*pszPath == '/') {
        dwError = LsaChangeDirectory("/");
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaCreateDirectoryRecursive("/", pszTmpPath, &pszTmp, dwFileMode, dwWorkingFileMode, 0);
        BAIL_ON_LSA_ERROR(dwError);

    } else {

        dwError = LsaCreateDirectoryRecursive(pszCurDirPath, pszTmpPath, &pszTmp, dwFileMode, dwWorkingFileMode, 0);
        BAIL_ON_LSA_ERROR(dwError);

    }

error:

    if (pszCurDirPath) {

        LsaChangeDirectory(pszCurDirPath);

        LwFreeMemory(pszCurDirPath);

    }

    if (pszTmpPath) {
        LwFreeMemory(pszTmpPath);
    }

    return dwError;
}

DWORD
LsaGetDirectoryFromPath(
    IN PCSTR pszPath,
    OUT PSTR* ppszDir
    )
{
    PCSTR pszLastSlash = NULL;
    PSTR pszDir = NULL;
    DWORD dwError = 0;

    BAIL_ON_INVALID_POINTER(pszPath);

    pszLastSlash = strrchr(pszPath, '/');
    if (pszLastSlash == pszPath)
    {
        //Include the trailing / since this is the root directory ( / )
        pszLastSlash++;
    }

    if (pszLastSlash == NULL)
    {
        dwError = LwAllocateString(
                        ".",
                        &pszDir);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        dwError = LwStrndup(
                        pszPath,
                        pszLastSlash - pszPath,
                        &pszDir);
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppszDir = pszDir;

cleanup:

    return dwError;

error:

    LW_SAFE_FREE_STRING(pszDir);
    *ppszDir = NULL;
    goto cleanup;
}

DWORD
LsaGetOwnerAndPermissions(
    PCSTR pszSrcPath,
    uid_t * uid,
    gid_t * gid,
    mode_t * mode
    )
{
    DWORD dwError = 0;
    struct stat statbuf;

    memset(&statbuf, 0, sizeof(struct stat));

    if (stat(pszSrcPath, &statbuf) < 0) {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_LSA_ERROR(dwError);
    }

    *uid = statbuf.st_uid;
    *gid = statbuf.st_gid;
    *mode = statbuf.st_mode;

error:

    return dwError;
}

DWORD
LsaCopyFileWithPerms(
    PCSTR pszSrcPath,
    PCSTR pszDstPath,
    mode_t dwPerms
    )
{
    DWORD dwError = 0;
    PCSTR pszTmpSuffix = ".tmp_likewise_lsass";
    PSTR pszTmpPath = NULL;
    BOOLEAN bRemoveFile = FALSE;
    CHAR szBuf[1024+1];
    int  iFd = -1;
    int  oFd = -1;
    DWORD dwBytesRead = 0;

    if (LW_IS_NULL_OR_EMPTY_STR(pszSrcPath) ||
        LW_IS_NULL_OR_EMPTY_STR(pszDstPath)) {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwAllocateMemory(strlen(pszDstPath)+strlen(pszTmpSuffix)+2,
                               (PVOID*)&pszTmpPath);
    BAIL_ON_LSA_ERROR(dwError);

    strcpy(pszTmpPath, pszDstPath);
    strcat(pszTmpPath, pszTmpSuffix);

    if ((iFd = open(pszSrcPath, O_RDONLY, S_IRUSR)) < 0) {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if ((oFd = open(pszTmpPath, O_WRONLY|O_TRUNC|O_CREAT, S_IRUSR|S_IWUSR)) < 0) {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_LSA_ERROR(dwError);
    }

    bRemoveFile = TRUE;

    while (1) {
        if ((dwBytesRead = read(iFd, szBuf, 1024)) < 0) {

            if (errno == EINTR)
                continue;

            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_LSA_ERROR(dwError);
        }

        if (dwBytesRead == 0)
            break;

        if (write(oFd, szBuf, dwBytesRead) != dwBytesRead) {

            if (errno == EINTR)
                continue;

            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_LSA_ERROR(dwError);

        }

    }

    close(iFd); iFd = -1;
    close(oFd); oFd = -1;

    dwError = LsaMoveFile(pszTmpPath, pszDstPath);
    BAIL_ON_LSA_ERROR(dwError);

    bRemoveFile = FALSE;

    dwError = LsaChangePermissions(pszDstPath, dwPerms);
    BAIL_ON_LSA_ERROR(dwError);

error:

    if (iFd >= 0)
        close(iFd);

    if (oFd >= 0)
        close(oFd);


    if (bRemoveFile) {
        LsaRemoveFile(pszTmpPath);
    }

    LW_SAFE_FREE_STRING (pszTmpPath);

    return dwError;
}


DWORD
LsaCopyFileWithOriginalPerms(
    PCSTR pszSrcPath,
    PCSTR pszDstPath
    )
{
    DWORD dwError = 0;
    uid_t uid;
    gid_t gid;
    mode_t mode;

    dwError = LsaGetOwnerAndPermissions(pszSrcPath, &uid, &gid, &mode);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaCopyFileWithPerms(pszSrcPath, pszDstPath, mode);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaChangeOwnerAndPermissions(pszDstPath, uid, gid, mode);
    BAIL_ON_LSA_ERROR(dwError);

error:

    return dwError;
}

DWORD
LsaBackupFile(
    PCSTR pszPath
    )
{
    DWORD dwError = 0;
    BOOLEAN bExists = FALSE;
    CHAR    szBackupPath[PATH_MAX];

    dwError = LsaCheckFileExists(pszPath, &bExists);
    BAIL_ON_LSA_ERROR(dwError);

    if (!bExists)
    {
        /* Do not need to backup, since the file does not yet exist. */
        goto done;
    }

    sprintf(szBackupPath, "%s.likewise_lsass.orig", pszPath);

    dwError = LsaCheckFileExists(szBackupPath, &bExists);
    BAIL_ON_LSA_ERROR(dwError);

    if (bExists)
    {
       sprintf(szBackupPath, "%s.likewise_lsass.bak", pszPath);
    }

    dwError = LsaCopyFileWithOriginalPerms(pszPath, szBackupPath);
    BAIL_ON_LSA_ERROR(dwError);

done:
error:

    return dwError;
}

DWORD
LsaGetSymlinkTarget(
   PCSTR pszPath,
   PSTR* ppszTargetPath
   )
{
    DWORD dwError = 0;
    CHAR szBuf[PATH_MAX+1];
    ssize_t len = 0;
    PSTR pszTargetPath = NULL;

    memset(szBuf, 0, sizeof(szBuf));

    while (1) {

       if ((len = readlink(pszPath, szBuf, PATH_MAX)) < 0) {
          if (errno == EINTR)
             continue;

          dwError = LwMapErrnoToLwError(errno);
          BAIL_ON_LSA_ERROR(dwError);
       }

       break;
    }
    szBuf[len] = '\0';
    dwError = LwAllocateString(
                    szBuf,
                    &pszTargetPath);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszTargetPath = pszTargetPath;

cleanup:

    return dwError;

error:

    *ppszTargetPath = NULL;

    LW_SAFE_FREE_STRING(pszTargetPath);

    goto cleanup;
}

DWORD
LsaCreateSymlink(
   PCSTR pszOldPath,
   PCSTR pszNewPath
   )
{
    if (symlink(pszOldPath, pszNewPath) < 0)
        return LwMapErrnoToLwError(errno);
    else
        return ERROR_SUCCESS;
}

DWORD
LsaCopySkeletonDirectory(
    PCSTR pszSourceDirPath,
    uid_t ownerUid,
    gid_t ownerGid,
    PCSTR pszDestDirPath
    )
{
    DWORD dwError = 0;
    DIR*  pDir = NULL;
    struct dirent* pDirEntry = NULL;
    struct stat statbuf = {0};
    CHAR  szSrcPath[PATH_MAX+1];
    CHAR  szDstPath[PATH_MAX+1];
    PSTR  pszTargetPath = NULL;

    if (NULL == (pDir = opendir(pszSourceDirPath))) {
       dwError = LwMapErrnoToLwError(errno);
       BAIL_ON_LSA_ERROR(dwError);
    }

    while (NULL != (pDirEntry = readdir(pDir)))
    {
        if (!strcmp(pDirEntry->d_name, ".") ||
            !strcmp(pDirEntry->d_name, "..")||
            !strcmp(pDirEntry->d_name, "lost+found"))
            continue;

        memset(&statbuf, 0, sizeof(statbuf));

        sprintf(szSrcPath, "%s/%s", pszSourceDirPath, pDirEntry->d_name);

        if (lstat(szSrcPath, &statbuf) < 0) {
            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_LSA_ERROR(dwError);
        }

#ifdef __LWI_FREEBSD__
        if (!strncmp(pDirEntry->d_name, "dot.", 4) && pDirEntry->d_name[4] != '\0' )
        {
            sprintf(szDstPath, "%s/%s", pszDestDirPath, pDirEntry->d_name + 3);
        }
        else
        {
            sprintf(szDstPath, "%s/%s", pszDestDirPath, pDirEntry->d_name);
        }
#else
        sprintf(szDstPath, "%s/%s", pszDestDirPath, pDirEntry->d_name);
#endif

        if (S_ISDIR(statbuf.st_mode)) {

            dwError = LsaCreateDirectory(
                            szDstPath,
                            statbuf.st_mode);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaChangeOwner(
                            szDstPath,
                            ownerUid,
                            ownerGid);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaCopySkeletonDirectory(
                            szSrcPath,
                            ownerUid,
                            ownerGid,
                            szDstPath);
            BAIL_ON_LSA_ERROR(dwError);

        } else if (S_ISREG(statbuf.st_mode)) {

            dwError = LsaCopyFileWithOriginalPerms(
                            szSrcPath,
                            szDstPath);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaChangeOwner(
                            szDstPath,
                            ownerUid,
                            ownerGid);
            BAIL_ON_LSA_ERROR(dwError);

        } else if (S_ISLNK(statbuf.st_mode)) {

            dwError = LsaGetSymlinkTarget(
                            szSrcPath,
                            &pszTargetPath);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaCreateSymlink(
                            pszTargetPath,
                            szDstPath);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaChangeOwner(
                            szDstPath,
                            ownerUid,
                            ownerGid);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

cleanup:

    if (pDir) {
        closedir(pDir);
    }

    LW_SAFE_FREE_STRING(pszTargetPath);

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaGetMatchingFilePathsInFolder(
    PCSTR pszDirPath,
    PCSTR pszFileNameRegExp,
    PSTR** pppszHostFilePaths,
    PDWORD pdwNPaths
    )
{
    typedef struct __PATHNODE
    {
        PSTR pszPath;
        struct __PATHNODE *pNext;
    } PATHNODE, *PPATHNODE;

    DWORD dwError = 0;
    DIR* pDir = NULL;
    struct dirent*  pDirEntry = NULL;
    regex_t rx;
    BOOLEAN rxAllocated = FALSE;
    regmatch_t* pResult = NULL;
    size_t nMatch = 1;
    DWORD  dwNPaths = 0;
    DWORD  iPath = 0;
    PSTR*  ppszHostFilePaths = NULL;
    CHAR   szBuf[PATH_MAX+1];
    struct stat statbuf;
    PPATHNODE pPathList = NULL;
    PPATHNODE pPathNode = NULL;
    BOOLEAN bDirExists = FALSE;

    dwError = LsaCheckDirectoryExists(pszDirPath, &bDirExists);
    BAIL_ON_LSA_ERROR(dwError);

    if(!bDirExists) {
        dwError = ERROR_FILE_NOT_FOUND;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (regcomp(&rx, pszFileNameRegExp, REG_EXTENDED) != 0) {
        dwError = LW_ERROR_REGEX_COMPILE_FAILED;
        BAIL_ON_LSA_ERROR(dwError);
    }
    rxAllocated = TRUE;

    dwError = LwAllocateMemory(sizeof(regmatch_t), (PVOID*)&pResult);
    BAIL_ON_LSA_ERROR(dwError);

    pDir = opendir(pszDirPath);
    if (!pDir) {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_LSA_ERROR(dwError);
    }

    while ((pDirEntry = readdir(pDir)) != NULL) {

        int copied = snprintf(
                            szBuf,
                            sizeof(szBuf),
                            "%s/%s",
                            pszDirPath,
                            pDirEntry->d_name);
        if (copied >= sizeof(szBuf))
        {
            //Skip pathnames that are too long
            continue;
        }
        memset(&statbuf, 0, sizeof(struct stat));
        if (lstat(szBuf, &statbuf) < 0) {
            if(errno == ENOENT)
            {
                //This occurs when there is a symbolic link pointing to a
                //location that doesn't exist, because stat looks at the final
                //file, not the link. Since this file doesn't exist anyway,
                //just skip it.
                continue;
            }
            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_LSA_ERROR(dwError);
        }

        /*
         * For now, we are searching only for regular files
         * This may be enhanced in the future to support additional
         * file system entry types
         */
        if (((statbuf.st_mode & S_IFMT) == S_IFREG) &&
            (regexec(&rx, pDirEntry->d_name, nMatch, pResult, 0) == 0)) {
            dwNPaths++;

            dwError = LwAllocateMemory(sizeof(PATHNODE), (PVOID*)&pPathNode);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LwAllocateString(szBuf, &pPathNode->pszPath);
            BAIL_ON_LSA_ERROR(dwError);

            pPathNode->pNext = pPathList;
            pPathList = pPathNode;
            pPathNode = NULL;
        }
    }

    if (pPathList) {
        dwError = LwAllocateMemory(sizeof(PSTR)*dwNPaths,
                                    (PVOID*)&ppszHostFilePaths);
        BAIL_ON_LSA_ERROR(dwError);
        /*
         *  The linked list is in reverse.
         *  Assign values in reverse to get it right
         */
        iPath = dwNPaths-1;
        pPathNode = pPathList;
        while (pPathNode) {
            *(ppszHostFilePaths+iPath) = pPathNode->pszPath;
            pPathNode->pszPath = NULL;
            pPathNode = pPathNode->pNext;
            iPath--;
        }
    }

    *pppszHostFilePaths = ppszHostFilePaths;
    *pdwNPaths = dwNPaths;

cleanup:

    if (pPathNode) {
        LW_SAFE_FREE_STRING(pPathNode->pszPath);
        LwFreeMemory(pPathNode);
    }

    while(pPathList) {
        pPathNode = pPathList;
        pPathList = pPathList->pNext;
        LW_SAFE_FREE_STRING(pPathNode->pszPath);
        LwFreeMemory(pPathNode);
    }

    if (rxAllocated) {
        regfree(&rx);
    }

    LW_SAFE_FREE_MEMORY(pResult);

    if (pDir) {
        closedir(pDir);
    }

    return dwError;

error:

    if (ppszHostFilePaths) {
       LwFreeStringArray(ppszHostFilePaths, dwNPaths);
    }

    goto cleanup;
}
