/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 *        unixpath.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        POSIX filename routines
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "pvfs.h"

/* Forward declarations */

static
NTSTATUS
PvfsResolvePath(
    PSTR *ppszResolvedPath,
    PCSTR pszLookupPath
    );


/***********************************************************************
 **********************************************************************/

NTSTATUS
PvfsCanonicalPathName(
    PSTR *ppszPath,
    IO_FILE_NAME IoPath
    )
{
    PPVFS_CCB pRootCcb = NULL;
    NTSTATUS ntError = STATUS_SUCCESS;
    PSTR pszFilename = NULL;
    PSTR pszCompletePath = NULL;

    if (IoPath.FileName)
    {
        ntError = PvfsWC16CanonicalPathName(&pszFilename, IoPath.FileName);
        BAIL_ON_NT_STATUS(ntError);
    }

    if (IoPath.RootFileHandle)
    {
        ntError = PvfsAcquireCCB(IoPath.RootFileHandle, &pRootCcb);
        BAIL_ON_NT_STATUS(ntError);

        ntError = LwRtlCStringAllocatePrintf(
            &pszCompletePath,
            "%s%s%s",
            pRootCcb->pszFilename,
            (pszFilename ? (*pszFilename == '/' ? "" : "/") : ""),
            (pszFilename ? pszFilename : ""));
        BAIL_ON_NT_STATUS(ntError);
    }
    else
    {
        pszCompletePath = pszFilename;
        pszFilename = NULL;
    }

    if (!pszCompletePath)
    {
        ntError = STATUS_OBJECT_NAME_INVALID;
        BAIL_ON_NT_STATUS(ntError);
    }


    *ppszPath = pszCompletePath;

cleanup:
    LwRtlCStringFree(&pszFilename);

    if (pRootCcb)
    {
        PvfsReleaseCCB(pRootCcb);
    }

    return ntError;

error:
    LwRtlCStringFree(&pszCompletePath);

    goto cleanup;
}

/***********************************************************************
 **********************************************************************/

NTSTATUS
PvfsWC16CanonicalPathName(
    PSTR *ppszPath,
    PWSTR pwszPathname
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PSTR pszPath = NULL;
    PSTR pszCursor = NULL;
    size_t Length = 0;
    size_t Offset = 0;
    int i = 0;

    ntError = RtlCStringAllocateFromWC16String(
                  &pszPath,
                  pwszPathname);
    BAIL_ON_NT_STATUS(ntError);

    Length = RtlCStringNumChars(pszPath);

    pszCursor = pszPath;
    while (pszCursor && *pszCursor)
    {
        if ((*pszCursor == ':') ||
            (*pszCursor == '"') ||
            (*pszCursor == '*') ||
            (*pszCursor == '?') ||
            (*pszCursor == '<') ||
            (*pszCursor == '>') ||
            (*pszCursor == '|'))
        {
            ntError = STATUS_OBJECT_NAME_INVALID;
            BAIL_ON_NT_STATUS(ntError);
        }

        if (*pszCursor == '\\')
        {
            *pszCursor = '/';
        }

        /* Collapse "//" to "/" */

        if ((Offset > 0) &&
            (*pszCursor == '/') &&
            (*(pszCursor-1) == '/'))
        {
            LwRtlMoveMemory(pszCursor-1, pszCursor, Length-Offset);
            pszPath[Length-1] = '\0';
            Length--;
            continue;
        }

        Offset++;
        pszCursor = pszPath + Offset;
    }

    /* Strip trailing slashes */

    for (i=Length-1; i>0; i--)
    {
        /* break out at first non slash */
        if (pszPath[i] != '/') {
            break;
        }

        pszPath[i] = '\0';
    }

    *ppszPath = pszPath;

    ntError = STATUS_SUCCESS;

cleanup:
    return ntError;

error:
    goto cleanup;
}

/****************************************************************
 ***************************************************************/

NTSTATUS
PvfsValidatePath(
    PPVFS_FCB pFcb,
    PPVFS_FILE_ID pFileId
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    BOOLEAN bFcbLocked = FALSE;
    PVFS_STAT Stat = {0};

    LWIO_LOCK_RWMUTEX_SHARED(bFcbLocked, &pFcb->rwLock);

    /* Verify that the dev/inode pair is the same on the pathname
       and the fd */

    ntError = PvfsSysStat(pFcb->pszFilename, &Stat);
    BAIL_ON_NT_STATUS(ntError);

    if ((pFileId->Device != Stat.s_dev) || (pFileId->Inode != Stat.s_ino))
    {
        ntError = STATUS_FILE_RENAMED;
        BAIL_ON_NT_STATUS(ntError);
    }

cleanup:
    LWIO_UNLOCK_RWMUTEX(bFcbLocked, &pFcb->rwLock);

    return ntError;

error:
    goto cleanup;
}

/****************************************************************
 ***************************************************************/

NTSTATUS
PvfsFileBasename(
    PSTR *ppszFilename,
    PCSTR pszPath
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PSTR pszCursor = NULL;

    if ((pszCursor = strrchr(pszPath, '/')) != NULL)
    {
        /* Assume there is never a trailing '/' since that should
           be handled by PvfsCanonicalPathName() */

        pszCursor++;
    }

    if (pszCursor != NULL) {
        ntError = RtlCStringDuplicate(ppszFilename, pszCursor);
    } else {
        ntError = RtlCStringDuplicate(ppszFilename, pszPath);
    }
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    return ntError;

error:
    goto cleanup;
}

/****************************************************************
 ***************************************************************/

NTSTATUS
PvfsFileDirname(
    PSTR *ppszDirname,
    PCSTR pszPath
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PSTR pszCursor = NULL;
    PSTR pszNewString = NULL;
    size_t stringLength = 0;

    /* Case #1: No '/' so just return '.' */
    if (((pszCursor = strrchr(pszPath, '/')) == NULL))
    {
        ntError = RtlCStringDuplicate(ppszDirname, ".");
        goto cleanup;
    }

    /* Case #2: only one '/' (at beginning of path) */

    if (pszCursor == pszPath) {
        ntError = RtlCStringDuplicate(ppszDirname, "/");
        goto cleanup;
    }

    /* Case #3: Real dirname and file name components */

    stringLength = PVFS_PTR_DIFF(pszPath,pszCursor);

    ntError = LW_RTL_ALLOCATE_NOCLEAR(
                  &pszNewString,
                  CHAR,
                  stringLength + 1);
    BAIL_ON_NT_STATUS(ntError);

    RtlCopyMemory(pszNewString, pszPath, stringLength);
    pszNewString[stringLength] = '\0';

    *ppszDirname = pszNewString;
    ntError = STATUS_SUCCESS;

cleanup:
    return ntError;

error:
    goto cleanup;
}


/****************************************************************
 ***************************************************************/

NTSTATUS
PvfsFileSplitPath(
    PSTR *ppszDirname,
    PSTR *ppszBasename,
    PCSTR pszPath
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PSTR pszCursor = NULL;
    PSTR pszDirComponent = NULL;
    PSTR pszBaseComponent = NULL;
    size_t stringLength = 0;

    /* Case #1: No '/' so just return '.' */
    if (((pszCursor = strrchr(pszPath, '/')) == NULL))
    {
        ntError = RtlCStringDuplicate(ppszDirname, ".");
        BAIL_ON_NT_STATUS(ntError);

        ntError = RtlCStringDuplicate(ppszBasename, pszPath);
        BAIL_ON_NT_STATUS(ntError);

        goto cleanup;
    }

    /* Case #2: only one '/' (at beginning of path) */

    if (pszCursor == pszPath)
    {
        ntError = RtlCStringDuplicate(ppszDirname, "/");
        BAIL_ON_NT_STATUS(ntError);

        ntError = RtlCStringDuplicate(ppszBasename, "");
        BAIL_ON_NT_STATUS(ntError);

        goto cleanup;
    }

    /* Case #3: Real dirname and file name components */

    stringLength = PVFS_PTR_DIFF(pszPath, pszCursor);

    ntError = LW_RTL_ALLOCATE_NOCLEAR(
                  &pszDirComponent,
                  CHAR,
                  stringLength + 1);
    BAIL_ON_NT_STATUS(ntError);

    RtlCopyMemory(pszDirComponent, pszPath, stringLength);
    pszDirComponent[stringLength] = '\0';

    ntError = LwRtlCStringDuplicate(&pszBaseComponent, pszCursor+1);
    BAIL_ON_NT_STATUS(ntError);

    *ppszBasename = pszBaseComponent;
    *ppszDirname  = pszDirComponent;

cleanup:
    return ntError;

error:
    if (pszBaseComponent)
    {
        LwRtlCStringFree(&pszBaseComponent);
    }

    if (pszDirComponent)
    {
        LwRtlCStringFree(&pszDirComponent);
    }

    goto cleanup;

}


/****************************************************************
 ***************************************************************/

NTSTATUS
PvfsLookupPath(
    OUT PSTR *ppszDiskPath,
    IN OUT PPVFS_STAT pStat,
    IN PCSTR pszPath,
    IN BOOLEAN bCaseSensitive
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PVFS_STAT Stat = {0};
    PSTR pszDiskPath = NULL;

    /* Check the cache */

    ntError = PvfsPathCacheLookup(&pszDiskPath, pszPath);
    if (ntError == STATUS_SUCCESS)
    {
        /* Check that the path is still good.  If not fallback
           to manual checks */

        ntError = PvfsSysStat(pszDiskPath, &Stat);
        if (ntError == STATUS_SUCCESS)
        {
            *pStat = Stat;
            *ppszDiskPath = pszDiskPath;
            goto cleanup;
        }

        LwRtlCStringFree(&pszDiskPath);
        pszDiskPath = NULL;
    }

    /* See if we are lucky */

    ntError = PvfsSysStat(pszPath, &Stat);
    if (ntError == STATUS_SUCCESS)
    {
        *pStat = Stat;

        ntError = RtlCStringDuplicate(ppszDiskPath, pszPath);
        BAIL_ON_NT_STATUS(ntError);

        ntError = PvfsPathCacheAdd(pszPath);
        BAIL_ON_NT_STATUS(ntError);

        goto cleanup;
    }

    /* Done if use case sensitive matches */

    if (bCaseSensitive)
    {
        ntError = STATUS_OBJECT_NAME_NOT_FOUND;
        BAIL_ON_NT_STATUS(ntError);
    }

    /* Resolve the path */

    ntError = PvfsResolvePath(&pszDiskPath, pszPath);
    BAIL_ON_NT_STATUS(ntError);

    /* This should succeed now */
    ntError = PvfsSysStat(pszDiskPath, &Stat);
    BAIL_ON_NT_STATUS(ntError);

    *pStat = Stat;
    *ppszDiskPath = pszDiskPath;

cleanup:
    return ntError;

error:
    LwRtlCStringFree(&pszDiskPath);

    goto cleanup;
}

/****************************************************************
 ***************************************************************/

NTSTATUS
PvfsLookupFile(
    OUT PSTR *ppszDiskPath,
    IN OUT PPVFS_STAT pStat,
    IN PCSTR pszDiskDirname,
    IN PCSTR pszFilename,
    IN BOOLEAN bCaseSensitive
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PSTR pszFullPath = NULL;

    ntError = RtlCStringAllocatePrintf(
                  &pszFullPath,
                  "%s/%s",
                  pszDiskDirname,
                  pszFilename);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsLookupPath(
                  ppszDiskPath,
                  pStat,
                  pszFullPath,
                  bCaseSensitive);
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    RtlCStringFree(&pszFullPath);

    return ntError;

error:
    goto cleanup;
}

/****************************************************************
 ***************************************************************/

static NTSTATUS
PvfsResolvePath(
    PSTR *ppszResolvedPath,
    PCSTR pszLookupPath
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PSTR pszCursor = NULL;
    PSTR pszComponent = NULL;
    PSTR pszPath = NULL;
    PVFS_STAT Stat = {0};
    PSTR pszResolvedPath = NULL;
    PSTR pszResWorkingPath = NULL;
    CHAR pszWorkingPath[PATH_MAX] = { 0 };
    DWORD Length = PATH_MAX;
    DIR *pDir = NULL;
    struct dirent *pDirEntry = NULL;

    if (*pszLookupPath != '/') {
        ntError = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntError);
    }

    ntError = RTL_ALLOCATE(&pszResolvedPath, CHAR, Length);
    BAIL_ON_NT_STATUS(ntError);

    ntError = RtlCStringDuplicate(&pszPath, pszLookupPath);
    BAIL_ON_NT_STATUS(ntError);

    pszComponent = pszPath + 1;

    while (pszComponent && (Length > 0))
    {
        pDir = NULL;

        if ((pszCursor = strchr(pszComponent, '/')) != NULL) {
            *pszCursor = '\0';
        }

        snprintf(pszWorkingPath, PATH_MAX-1, "%s/%s",
                 pszResolvedPath, pszComponent);

        /* Try cache first */

        ntError = PvfsPathCacheLookup(&pszResWorkingPath, pszWorkingPath);
        if(ntError == STATUS_SUCCESS)
        {
            strncpy(pszResolvedPath, pszResWorkingPath, PATH_MAX-1);
            Length = PATH_MAX - RtlCStringNumChars(pszResolvedPath);
            RtlCStringFree(&pszResWorkingPath);
        }

        /* Maybe an exact match on disk? */

        else if (PvfsSysStat(pszWorkingPath, &Stat) == STATUS_SUCCESS)
        {
            strncpy(pszResolvedPath, pszWorkingPath, PATH_MAX-1);
            Length = PATH_MAX - RtlCStringNumChars(pszResolvedPath);
            RtlCStringFree(&pszResWorkingPath);

            ntError = PvfsPathCacheAdd(pszResolvedPath);
            BAIL_ON_NT_STATUS(ntError);
        }

        /* Do the work ourselves */

        else
        {
            /* Enumerate directory entries and look for a match */

            ntError = PvfsSysOpenDir(pszResolvedPath, &pDir);
            if (ntError == STATUS_NOT_A_DIRECTORY)
            {
                ntError = STATUS_OBJECT_PATH_NOT_FOUND;
            }
            BAIL_ON_NT_STATUS(ntError);

            for(ntError = PvfsSysReadDir(pDir, &pDirEntry);
                pDirEntry;
                ntError = PvfsSysReadDir(pDir, &pDirEntry))
            {
                /* First check the error return */
                BAIL_ON_NT_STATUS(ntError);

                if (RtlCStringIsEqual(pszComponent, pDirEntry->d_name, FALSE)) {
                    break;
                }
            }

            /* Did we find a match? */

            if (!pDirEntry) {
                /* Return code depends on whether the last component was
                   not found or if an intermediate component was invalid */

                if (pszCursor == NULL)
                {
                    ntError = STATUS_OBJECT_NAME_NOT_FOUND;
                }
                else
                {
                    ntError = STATUS_OBJECT_PATH_NOT_FOUND;
                }

                BAIL_ON_NT_STATUS(ntError);
            }

            strncat(pszResolvedPath, "/", Length-1);
            Length -= 1;
            strncat(pszResolvedPath, pDirEntry->d_name, Length-1);
            Length -= RtlCStringNumChars(pDirEntry->d_name);

            ntError = PvfsSysCloseDir(pDir);
            pDir = NULL;
            BAIL_ON_NT_STATUS(ntError);

            ntError = PvfsPathCacheAdd(pszResolvedPath);
            BAIL_ON_NT_STATUS(ntError);

        }

        /* Cleanup for next loop */


        if (pszCursor) {
            *pszCursor = '/';
        }

        if ((pszComponent = strchr(pszComponent, '/')) != NULL) {
            pszComponent++;
        }
    }

    /* Did we finish? */

    if ((Length <= 0) && (pszComponent != NULL)) {
        ntError = STATUS_NAME_TOO_LONG;
        BAIL_ON_NT_STATUS(ntError);
    }

    *ppszResolvedPath = pszResolvedPath;
    pszResolvedPath = NULL;

    ntError = STATUS_SUCCESS;

cleanup:
    RtlCStringFree(&pszPath);
    RtlCStringFree(&pszResWorkingPath);
    RtlCStringFree(&pszResolvedPath);

    if (pDir) {
        PvfsSysCloseDir(pDir);
    }

    return ntError;

error:
    goto cleanup;
}



/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
