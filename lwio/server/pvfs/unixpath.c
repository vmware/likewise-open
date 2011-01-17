/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

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

        if (*pszCursor == '$')
        {
            if (LwRtlCStringIsEqual(pszPath, PVFS_NTFS_QUOTA_FILENAME, TRUE))
            {
                LwRtlCStringFree(&pszPath);
                ntError= LwRtlCStringDuplicate(&pszPath, PVFS_UNIX_QUOTA_FILENAME);
                BAIL_ON_NT_STATUS(ntError);

                goto cleanup;
            }
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

    ntError = STATUS_SUCCESS;

cleanup:
    *ppszPath = pszPath;

    return ntError;

error:
    if (pszPath)
    {
        LwRtlCStringFree(&pszPath);
    }

    goto cleanup;
}

/****************************************************************
 ***************************************************************/

NTSTATUS
PvfsValidatePathSCB(
    PPVFS_SCB pScb,
    PPVFS_FILE_ID pFileId
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    BOOLEAN bScbLocked = FALSE;
    PVFS_STAT Stat = {0};
    PSTR streamName = NULL;

    ntError = PvfsGetBasicStreamname(&streamName, pScb);
    BAIL_ON_NT_STATUS(ntError);

    LWIO_LOCK_RWMUTEX_SHARED(bScbLocked, &pScb->BaseControlBlock.RwLock);

    /* Verify that the dev/inode pair is the same on the pathname
       and the fd */

    ntError = PvfsSysStat(streamName, &Stat);
    BAIL_ON_NT_STATUS(ntError);

    if ((pFileId->Device != Stat.s_dev) || (pFileId->Inode != Stat.s_ino))
    {
        ntError = STATUS_FILE_RENAMED;
        BAIL_ON_NT_STATUS(ntError);
    }

cleanup:
    LWIO_UNLOCK_RWMUTEX(bScbLocked, &pScb->BaseControlBlock.RwLock);

    if (streamName)
    {
        LwRtlCStringFree(&streamName);
    }

    return ntError;

error:
    goto cleanup;
}

/****************************************************************
 ***************************************************************/

NTSTATUS
PvfsValidatePathFCB(
    PPVFS_FCB pFcb,
    PPVFS_FILE_ID pFileId
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    BOOLEAN bFcbLocked = FALSE;
    PVFS_STAT Stat = {0};

    LWIO_LOCK_RWMUTEX_SHARED(bFcbLocked, &pFcb->BaseControlBlock.RwLock);

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
    LWIO_UNLOCK_RWMUTEX(bFcbLocked, &pFcb->BaseControlBlock.RwLock);

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
    OUT PSTR *ppszDirname,
    OUT PSTR *ppszBasename,
    IN PCSTR pszPath
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

        ntError = RtlCStringDuplicate(ppszBasename, pszCursor+1);
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

        PvfsPathCacheRemove(pszDiskPath);   // Ignore errors
        LwRtlCStringFree(&pszDiskPath);
        pszDiskPath = NULL;
    }

    /* See if we are lucky */

    ntError = PvfsSysStat(pszPath, &Stat);
    if (ntError == STATUS_SUCCESS)
    {
        ntError = RtlCStringDuplicate(&pszDiskPath, pszPath);
        BAIL_ON_NT_STATUS(ntError);

        ntError = PvfsPathCacheAdd(pszPath);
        BAIL_ON_NT_STATUS(ntError);

        *pStat = Stat;
        *ppszDiskPath = pszDiskPath;

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
    if (pszDiskPath)
    {
        LwRtlCStringFree(&pszDiskPath);
    }

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

static
void
PvfsFreeStringArray(
    PSTR * ppStringArray,
    size_t sSize
    )
{
    size_t i = 0;

    if ( ppStringArray )
    {
        for(i = 0; i < sSize; i++)
        {
            if (ppStringArray[i])
            {
                LwRtlCStringFree(&ppStringArray[i]);
            }
        }

        LwRtlMemoryFree(ppStringArray);
    }

    return;
}

////////////////////////////////////////////////////////////////////////

NTSTATUS
PvfsParseStreamname(
    OUT PSTR *ppszOwnerFilename,
    OUT PSTR *ppszFilename, // the physical location of the stream
    OPTIONAL OUT PSTR *ppszStreamname,
    OPTIONAL OUT PVFS_STREAM_TYPE *pStreamtype,
    IN PCSTR pszFullStreamname
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PSTR pszStreamname = NULL;
    PSTR pszOwnerFilename = NULL;
    PSTR pszFilename = NULL;

    /* By default set streamtype to be $DATA */
    PVFS_STREAM_TYPE Streamtype = PVFS_STREAM_TYPE_DATA;

    PSTR pszStreamnameTmp = NULL;
    PSTR pszStreamnameTmp1 = NULL;
    PSTR* ppszTokens = NULL;
    // Do not free
    PSTR pszTmp = NULL;
    PSTR pszstrtok_rSav = NULL;
    int iTokenCount = 0;
    int i = 0;
    size_t sLen = 0;

    if (pszFullStreamname && *pszFullStreamname == PVFS_STREAM_DELIMINATOR_C)
    {
        ntError = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntError);
    }

    sLen = RtlCStringNumChars(pszFullStreamname);

    /* Check for object_name: (this violates object_name should not have ':')*/

    if (pszFullStreamname[sLen-1] == PVFS_STREAM_DELIMINATOR_C
        && pszFullStreamname[sLen-2] != PVFS_STREAM_DELIMINATOR_C)
    {
        ntError = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntError);
    }

    ntError  = RtlCStringDuplicate(&pszStreamnameTmp,
                                  pszFullStreamname);
    BAIL_ON_NT_STATUS(ntError);

    ntError  = RtlCStringDuplicate(&pszStreamnameTmp1,
                                  pszFullStreamname);
    BAIL_ON_NT_STATUS(ntError);

    pszTmp = strtok_r(pszStreamnameTmp, PVFS_STREAM_DELIMINATOR_S, &pszstrtok_rSav);
    while (pszTmp != NULL)
    {
        iTokenCount++;
        pszTmp = strtok_r(NULL, PVFS_STREAM_DELIMINATOR_S, &pszstrtok_rSav);
    }

    /* object_name -> iTokenCount == 1
     * object_name:: -> iTokenCount == 1
     * object_name::$DATA -> iTokenCount == 2
     * object_name:stream_name -> iTokenCount == 2
     * object_name:stream_name:$DATA -> iTokenCount == 3 */

    if (iTokenCount < 1 || iTokenCount > 3)
    {
        ntError = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntError);
    }

    ntError = RTL_ALLOCATE(&ppszTokens, PSTR, sizeof(*ppszTokens) * iTokenCount);
    BAIL_ON_NT_STATUS(ntError);

    pszTmp = strtok_r(pszStreamnameTmp1, PVFS_STREAM_DELIMINATOR_S, &pszstrtok_rSav);
    while (pszTmp != NULL)
    {
        ntError  = LwRtlCStringDuplicate(&ppszTokens[i++],
                                        pszTmp);
        BAIL_ON_NT_STATUS(ntError);

        pszTmp = strtok_r(NULL, PVFS_STREAM_DELIMINATOR_S, &pszstrtok_rSav);
    }

    ntError  = RtlCStringDuplicate(&pszOwnerFilename,
                                  ppszTokens[0]);
    BAIL_ON_NT_STATUS(ntError);

    if (iTokenCount == 1)
    {
        // empty stream name
        ntError = RTL_ALLOCATE(&pszStreamname, CHAR, sizeof(*pszStreamname) * 1);
        BAIL_ON_NT_STATUS(ntError);
    }
    else if (iTokenCount == 2)
    {
        if (*ppszTokens[1] == '$')
        {
            if (RtlCStringIsEqual(ppszTokens[1], "$DATA", FALSE))
            {
                // empty stream name
                ntError = RTL_ALLOCATE(&pszStreamname, CHAR, sizeof(*pszStreamname) * 1);
                BAIL_ON_NT_STATUS(ntError);
            }
            else
            {
                ntError = STATUS_NOT_SUPPORTED;
                BAIL_ON_NT_STATUS(ntError);
            }
        }
        else
        {
            ntError  = RtlCStringDuplicate(&pszStreamname,
                                           ppszTokens[1]);
            BAIL_ON_NT_STATUS(ntError);
        }
    }
    else if (iTokenCount == 3)
    {
        if (!RtlCStringIsEqual(ppszTokens[2], "$DATA", FALSE))
        {
            ntError = STATUS_NOT_SUPPORTED;
            BAIL_ON_NT_STATUS(ntError);
        }

        ntError  = RtlCStringDuplicate(&pszStreamname,
                                       ppszTokens[1]);
        BAIL_ON_NT_STATUS(ntError);
    }

    if (pszStreamname && *pszStreamname)
    {
        ntError = RtlCStringAllocatePrintf(
                      &pszFilename,
                      "%s:%s",
                      pszOwnerFilename,
                      pszStreamname);
        BAIL_ON_NT_STATUS(ntError);
    }
    else
    {
        ntError  = RtlCStringDuplicate(&pszFilename,
                                       pszOwnerFilename);
        BAIL_ON_NT_STATUS(ntError);
    }

error:
    if (ntError)
    {
        RtlCStringFree(&pszOwnerFilename);
        RtlCStringFree(&pszStreamname);
        RtlCStringFree(&pszFilename);
    }

    RTL_FREE(&pszStreamnameTmp);
    RTL_FREE(&pszStreamnameTmp1);
    PvfsFreeStringArray(ppszTokens, iTokenCount);

    *ppszOwnerFilename = pszOwnerFilename;
    *ppszFilename = pszFilename;
    if (pStreamtype)
    {
        *pStreamtype = Streamtype;
    }
    if (ppszStreamname)
    {
        *ppszStreamname = pszStreamname;
    }
    else
    {
        RtlCStringFree(&pszStreamname);
    }

    return ntError;
}

////////////////////////////////////////////////////////////////////////

VOID
PvfsDestroyFileName(
    PPVFS_FILE_NAME pFileName
    )
{
    if (pFileName->FileName)
    {
        LwRtlCStringFree(&pFileName->FileName);
    }

    if (pFileName->StreamName)
    {
        LwRtlCStringFree(&pFileName->StreamName);
    }

    pFileName->Type = PVFS_STREAM_TYPE_UNKNOWN;
}


////////////////////////////////////////////////////////////////////////

VOID
PvfsFreeFileName(
    PPVFS_FILE_NAME pFileName
    )
{
    if (pFileName)
    {
        PvfsDestroyFileName(pFileName);
        PvfsFreeMemory((PVOID*)&pFileName);
    }

    return;
}

////////////////////////////////////////////////////////////////////////

static
NTSTATUS
PvfsParseStreamType(
    OUT PPVFS_STREAM_TYPE pType,
    IN PCSTR StreamTypeString
    );

NTSTATUS
PvfsParseFileName(
    IN OUT PPVFS_FILE_NAME pFileName,
    IN PCSTR FullFileName
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PSTR currentPosition = NULL;
    PSTR cursor = NULL;
    PSTR fileName = NULL;
    PSTR streamName = NULL;
    PSTR streamTypeString = NULL;

    BAIL_ON_INVALID_PTR(pFileName, ntError);

    pFileName->FileName = NULL;
    pFileName->StreamName = NULL;
    pFileName->Type = PVFS_STREAM_TYPE_UNKNOWN;

    ntError = LwRtlCStringDuplicate(&fileName, FullFileName);
    BAIL_ON_NT_STATUS(ntError);

    currentPosition = fileName;

    // Get the File Object name
    // Foo.txt:Summary:$DATA
    // ^

    cursor = strchr(currentPosition, PVFS_STREAM_DELIMINATOR_C);
    if (cursor == NULL)
    {
        // entire currentPosition is the file object name

        ntError = LwRtlCStringDuplicate(&pFileName->FileName, fileName);
        BAIL_ON_NT_STATUS(ntError);

        pFileName->StreamName = NULL;
        pFileName->Type = PVFS_STREAM_TYPE_DATA;

        goto cleanup;
    }

    *cursor = '\0';
    cursor++;

    if (*cursor == '\0')
    {
        // Cannot end in a trailing ':'
        ntError = STATUS_OBJECT_NAME_INVALID;
        BAIL_ON_NT_STATUS(ntError);
    }

    currentPosition = cursor;

    // Get the stream name
    // Foo.txt:Summary:$DATA
    //         ^

    cursor = strchr(currentPosition, PVFS_STREAM_DELIMINATOR_C);
    if (cursor == NULL)
    {
        // Missing StreamType
        ntError = STATUS_OBJECT_NAME_INVALID;
        BAIL_ON_NT_STATUS(ntError);
    }

    streamName = currentPosition;

    *cursor = '\0';
    cursor++;

    if (*cursor == '\0')
    {
        // Cannot end in a trailing ':'
        ntError = STATUS_OBJECT_NAME_INVALID;
        BAIL_ON_NT_STATUS(ntError);
    }

    currentPosition = cursor;
    streamTypeString = currentPosition;

    // Get the stream type
    // Foo.txt:Summary:$DATA
    //                 ^

    ntError = LwRtlCStringDuplicate(&pFileName->FileName, fileName);
    BAIL_ON_NT_STATUS(ntError);

    ntError = LwRtlCStringDuplicate(&pFileName->FileName, streamName);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsParseStreamType(&pFileName->Type, streamTypeString);
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    if (!NT_SUCCESS(ntError))
    {
        PvfsDestroyFileName(pFileName);
    }

    if (fileName)
    {
        LwRtlCStringFree(&currentPosition);
    }


    return ntError;

error:
    goto cleanup;
}

////////////////////////////////////////////////////////////////////////

static
NTSTATUS
PvfsParseStreamType(
    OUT PPVFS_STREAM_TYPE pStreamType,
    IN PCSTR StreamTypeString
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PVFS_STREAM_TYPE streamType = PVFS_STREAM_TYPE_UNKNOWN;

    BAIL_ON_INVALID_PTR(pStreamType, ntError);

    if (!StreamTypeString ||
        (*StreamTypeString == '\0') ||
        (*StreamTypeString != '$'))
    {
        ntError = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntError);
    }

    if (LwRtlCStringIsEqual(StreamTypeString, "$DATA", FALSE))
    {
        streamType = PVFS_STREAM_TYPE_DATA;
    }
    else
    {
        ntError = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntError);
    }

    *pStreamType = streamType;

cleanup:
    return ntError;

error:
    goto cleanup;

}


////////////////////////////////////////////////////////////////////////

NTSTATUS
PvfsAllocateFileName(
    OUT PPVFS_FILE_NAME *ppFileName,
    IN PSTR SourceFileName
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PPVFS_FILE_NAME pFileName = NULL;

    ntError = PvfsAllocateMemory((PVOID*)&pFileName, sizeof(*pFileName), TRUE);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsParseFileName(pFileName, SourceFileName);
    BAIL_ON_NT_STATUS(ntError);

    *ppFileName = pFileName;
    pFileName = NULL;

error:
    if (pFileName)
    {
        PvfsFreeFileName(pFileName);
    }

    return ntError;
}



/****************************************************************
 ***************************************************************/

static
NTSTATUS
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
    PSTR pszCurrentResolvedPath = NULL;
    PSTR pszResWorkingPath = NULL;
    PSTR pszResWorkingPath2 = NULL;
    CHAR pszWorkingPath[PATH_MAX] = { 0 };
    DWORD Length = PATH_MAX;
    DIR *pDir = NULL;
    struct dirent *pDirEntry = NULL;
    struct dirent dirEntry = { 0 };

    if (*pszLookupPath != '/')
    {
        ntError = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntError);
    }

    ntError = RTL_ALLOCATE(&pszResolvedPath, CHAR, Length);
    BAIL_ON_NT_STATUS(ntError);

    pszCurrentResolvedPath = pszResolvedPath;

    ntError = RtlCStringDuplicate(&pszPath, pszLookupPath);
    BAIL_ON_NT_STATUS(ntError);

    pszComponent = pszPath + 1;

    while (pszComponent && (Length > 0))
    {
        pDir = NULL;

        if ((pszCursor = strchr(pszComponent, '/')) != NULL)
        {
            *pszCursor = '\0';
        }

        /* If pszWorkingPath and pszCurrentResolvedPath overlap (i.e. are the same
           pointer), we cannot use snprintf() */

        if (pszCurrentResolvedPath == pszWorkingPath)
        {
            size_t sWorkingLen = RtlCStringNumChars(pszWorkingPath);

            strncat(pszWorkingPath, "/", PATH_MAX-sWorkingLen-1);
            sWorkingLen++;
            strncat(pszWorkingPath, pszComponent, PATH_MAX-sWorkingLen-1);
        }
        else
        {
            snprintf(
                pszWorkingPath,
                PATH_MAX-1,
                "%s/%s",
                pszCurrentResolvedPath,
                pszComponent);
        }

        /* Try cache first */

        ntError = PvfsPathCacheLookup(&pszResWorkingPath2, pszWorkingPath);
        if (ntError == STATUS_SUCCESS)
        {
            if (pszResWorkingPath)
            {
                LwRtlCStringFree(&pszResWorkingPath);
            }

            pszResWorkingPath = pszResWorkingPath2;
            pszResWorkingPath2 = NULL;

            pszCurrentResolvedPath = pszResWorkingPath;
        }

        /* Maybe an exact match on disk? */

        else if (PvfsSysStat(pszWorkingPath, &Stat) == STATUS_SUCCESS)
        {
            pszCurrentResolvedPath = pszWorkingPath;

            ntError = PvfsPathCacheAdd(pszCurrentResolvedPath);
            BAIL_ON_NT_STATUS(ntError);
        }

        /* Do the work ourselves */

        else
        {
            /* Enumerate directory entries and look for a match */

            ntError = PvfsSysOpenDir(pszCurrentResolvedPath, &pDir);
            if (ntError == STATUS_NOT_A_DIRECTORY)
            {
                ntError = STATUS_OBJECT_PATH_NOT_FOUND;
            }
            BAIL_ON_NT_STATUS(ntError);

            for(ntError = PvfsSysReadDir(pDir, &dirEntry, &pDirEntry);
                pDirEntry;
                ntError = PvfsSysReadDir(pDir, &dirEntry, &pDirEntry))
            {
                /* First check the error return */
                BAIL_ON_NT_STATUS(ntError);

                if (RtlCStringIsEqual(pszComponent, pDirEntry->d_name, FALSE))
                {
                    break;
                }
            }

            /* Did we find a match? */

            if (!pDirEntry)
            {
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

            if (pszCurrentResolvedPath == pszResolvedPath)
            {
                size_t sResolvedLen = RtlCStringNumChars(pszResolvedPath);

                strncat(pszResolvedPath, "/", PATH_MAX-sResolvedLen-1);
                sResolvedLen++;
                strncat(pszResolvedPath, pDirEntry->d_name, PATH_MAX-sResolvedLen-1);
            }
            else
            {
                snprintf(
                    pszResolvedPath,
                    PATH_MAX-1,
                    "%s/%s",
                    pszCurrentResolvedPath,
                    pDirEntry->d_name);
            }

            pszCurrentResolvedPath = pszResolvedPath;

            ntError = PvfsSysCloseDir(pDir);
            pDir = NULL;
            BAIL_ON_NT_STATUS(ntError);

            ntError = PvfsPathCacheAdd(pszResolvedPath);
            BAIL_ON_NT_STATUS(ntError);

        }

        Length = PATH_MAX - RtlCStringNumChars(pszCurrentResolvedPath);

        /* Cleanup for next loop */


        if (pszCursor)
        {
            *pszCursor = '/';
        }

        if ((pszComponent = strchr(pszComponent, '/')) != NULL)
        {
            pszComponent++;
        }
    }

    /* Did we finish? */

    if ((Length <= 0) && (pszComponent != NULL))
    {
        ntError = STATUS_NAME_TOO_LONG;
        BAIL_ON_NT_STATUS(ntError);
    }

    if (pszCurrentResolvedPath != pszResolvedPath)
    {
        strncpy(pszResolvedPath, pszCurrentResolvedPath, PATH_MAX-1);
        pszResolvedPath[PATH_MAX-1] = '\0';
        pszCurrentResolvedPath = NULL;
    }

    *ppszResolvedPath = pszResolvedPath;
    pszResolvedPath = NULL;

    ntError = STATUS_SUCCESS;

cleanup:
    if (pszPath)
    {
        LwRtlCStringFree(&pszPath);
    }

    if (pszResolvedPath)
    {
        LwRtlCStringFree(&pszResolvedPath);
    }

    if (pszResWorkingPath)
    {
        LwRtlCStringFree(&pszResWorkingPath);
    }

    if (pszResWorkingPath2)
    {
        LwRtlCStringFree(&pszResWorkingPath2);
    }

    if (pDir)
    {
        PvfsSysCloseDir(pDir);
    }

    return ntError;

error:
    goto cleanup;
}


