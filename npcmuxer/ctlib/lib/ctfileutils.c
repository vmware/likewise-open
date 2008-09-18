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

/* ex: set tabstop=4 expandtab shiftwidth=4: */

#include <ctfileutils.h>
#include <dirent.h>
#include <unistd.h>
#include <ctgoto.h>
#include <ctmemory.h>
#include <ctstring.h>
#include <string.h>


#define _CT_FILE_CALL(status, sysCall) \
    while (1) \
    { \
        if ((sysCall) < 0) \
        { \
            if (errno == EINTR) \
            { \
                continue; \
            } \
            status = CT_ERRNO_TO_STATUS(errno); \
        } \
        else \
        { \
            status = CT_STATUS_SUCCESS; \
        } \
        break; \
    }

CT_STATUS
CtFileStat(
    IN const char* Path,
    OUT struct stat* Info
    )
{
    CT_STATUS status;
    _CT_FILE_CALL(status, stat(Path, Info));
    return status;
}


CT_STATUS
CtFileUnlink(
    IN const char* Path
    )
{
    CT_STATUS status;
    _CT_FILE_CALL(status, unlink(Path));
    return status;
}


CT_STATUS
CtFileRemoveDirectory(
    IN const char* Path
    )
{
    CT_STATUS status;
    _CT_FILE_CALL(status, rmdir(Path));
    return status;
}

/*
// TODO: Check access and removability before actual deletion
*/
CT_STATUS
CtFileRemoveTree(
    IN const char* Path
    )
{
    CT_STATUS status = CT_STATUS_SUCCESS;
    DIR* dir = NULL;
    struct dirent* entry = NULL;
    char* subPath = NULL;
    struct stat statBuffer;

    dir = opendir(Path);
    if (!dir)
    {
        status = CT_ERRNO_TO_STATUS(errno);
        GOTO_CLEANUP();
    }

    for (;;)
    {
        CT_SAFE_FREE(subPath);

        entry = readdir(dir);
        if (!entry)
        {
            break;
        }

        if (!strcmp(entry->d_name, "..") ||
            !strcmp(entry->d_name, "."))
        {
            continue;
        }

        status = CtAllocateStringPrintf(&subPath, "%s/%s",
                                        Path, entry->d_name);
        GOTO_CLEANUP_ON_STATUS(status);

        status = CtFileStat(subPath, &statBuffer);
        GOTO_CLEANUP_ON_STATUS(status);

        if ((statBuffer.st_mode & S_IFMT) == S_IFDIR)
        {
            /* TODO: Remove recursion */
            status = CtFileRemoveTree(subPath);
            GOTO_CLEANUP_ON_STATUS(status);

            status = CtFileRemoveDirectory(subPath);
            GOTO_CLEANUP_ON_STATUS(status);
        }
        else
        {
            status = CtFileUnlink(subPath);
            GOTO_CLEANUP_ON_STATUS(status);
        }
    }

cleanup:
    if (dir)
    {
        closedir(dir);
    }

    CT_SAFE_FREE(subPath);

    return status;
}

CT_STATUS
CtFileSetOwner(
    IN const char* Path,
    IN uid_t Uid,
    IN gid_t Gid
    )
{
    CT_STATUS status;
    _CT_FILE_CALL(status, chown(Path, Uid, Gid));
    return status;
}

CT_STATUS
CtFileSetMode(
    IN const char* Path,
    IN mode_t Mode
    )
{
    CT_STATUS status;
    _CT_FILE_CALL(status, chmod(Path, Mode));
    return status;
}

CT_STATUS
CtFileSetOwnerMode(
    IN const char* Path,
    IN uid_t Uid,
    IN gid_t Gid,
    IN mode_t Mode
    )
{
    CT_STATUS status;

    status = CtFileSetOwner(Path, Uid, Gid);
    GOTO_CLEANUP_ON_STATUS(status);

    status = CtFileSetMode(Path, Mode);
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:
    return status;
}

CT_STATUS
CtFileGetOwnerMode(
    IN const char* Path,
    OUT OPTIONAL uid_t* Uid,
    OUT OPTIONAL gid_t* Gid,
    OUT OPTIONAL mode_t* Mode
    )
{
    CT_STATUS status;
    struct stat statBuffer;
    uid_t uid = -1;
    gid_t gid = -1;
    mode_t mode = 0;

    status = CtFileStat(Path, &statBuffer);
    GOTO_CLEANUP_ON_STATUS(status);

    uid = statBuffer.st_uid;
    gid = statBuffer.st_gid;
    mode = statBuffer.st_mode;

cleanup:
    if (Uid)
    {
        *Uid = statBuffer.st_uid;
    }
    if (Gid)
    {
        *Gid = statBuffer.st_gid;
    }
    if (Mode)
    {
        *Mode = statBuffer.st_mode;
    }
    return status;
}
