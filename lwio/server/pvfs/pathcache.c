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
 *        pathcache.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        Case Insensitive path cache manager
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "pvfs.h"

#define PVFS_PATH_HASH_MULTIPLIER   31

typedef struct _PVFS_PATH_CACHE_ENTRY
{
    PVFS_FILE_NAME FileName;

} PVFS_PATH_CACHE_ENTRY, *PPVFS_PATH_CACHE_ENTRY;

static
VOID
PvfsPathCacheFreeRecord(
    IN OUT PPVFS_PATH_CACHE_ENTRY pRecord
    );

////////////////////////////////////////////////////////////////////////

NTSTATUS
PvfsPathCacheAdd(
    IN PPVFS_FILE_NAME ResolvedFileName
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PPVFS_PATH_CACHE_ENTRY pCacheRecord = NULL;
    BOOLEAN cacheLock = FALSE;

    if (gpPathCache == NULL)
    {
        /* If the PathCache has been disabled or is not
           initialized, just report success and move on */

        ntError = STATUS_SUCCESS;
        goto error;
    }

    ntError = PvfsAllocateMemory(
                  (PVOID*)&pCacheRecord,
                  sizeof(PVFS_PATH_CACHE_ENTRY),
                  FALSE);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsFileNameCopy(&pCacheRecord->FileName, ResolvedFileName);
    BAIL_ON_NT_STATUS(ntError);

    LWIO_LOCK_MUTEX(cacheLock, &gPathCacheLock);
    ntError = LwioLruSetValue(
                  gpPathCache,
                  (PVOID)&pCacheRecord->FileName,
                  (PVOID)pCacheRecord);
    LWIO_UNLOCK_MUTEX(cacheLock, &gPathCacheLock);

    BAIL_ON_NT_STATUS(ntError);

error:
    if (!NT_SUCCESS(ntError))
    {
        if (pCacheRecord)
        {
            PvfsPathCacheFreeRecord(pCacheRecord);
        }
    }

    return ntError;
}

////////////////////////////////////////////////////////////////////////

NTSTATUS
PvfsPathCacheLookup(
    OUT PPVFS_FILE_NAME *ppResolvedFileName,
    IN  PPVFS_FILE_NAME pOriginalFileName
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    BOOLEAN cacheLock = FALSE;
    PPVFS_PATH_CACHE_ENTRY pCacheRecord = NULL;
    PPVFS_FILE_NAME resolvedFileName = NULL;

    if (gpPathCache == NULL)
    {
        /* If the PathCache has been disabled, just fail
           the lookup and move on */
        ntError = STATUS_OBJECT_PATH_NOT_FOUND;
        BAIL_ON_NT_STATUS(ntError);
    }

    LWIO_LOCK_MUTEX(cacheLock, &gPathCacheLock);
    ntError = LwioLruGetValue(
                  gpPathCache,
                  (PCVOID)pOriginalFileName,
                  (PVOID*)&pCacheRecord);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsFileNameDuplicate(&resolvedFileName, &pCacheRecord->FileName);
    BAIL_ON_NT_STATUS(ntError);

    *ppResolvedFileName = resolvedFileName;

error:
    LWIO_UNLOCK_MUTEX(cacheLock, &gPathCacheLock);

    if (!NT_SUCCESS(ntError))
    {
        if (resolvedFileName)
        {
            PvfsFreeFileName(resolvedFileName);
        }
    }

    return ntError;
}

////////////////////////////////////////////////////////////////////////

NTSTATUS
PvfsPathCacheRemove(
    IN PPVFS_FILE_NAME FileName
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    BOOLEAN cacheLock = FALSE;

    if (gpPathCache != NULL)
    {
        LWIO_LOCK_MUTEX(cacheLock, &gPathCacheLock);

        // Ignore errors from the remove
        ntError = LwioLruRemove(gpPathCache, (PVOID)FileName);

        LWIO_UNLOCK_MUTEX(cacheLock, &gPathCacheLock);
    }

    return STATUS_SUCCESS;
}

////////////////////////////////////////////////////////////////////////

static
LONG
PvfsPathCachePathCompare(
    PCVOID pKey1,
    PCVOID pKey2
    );

static
ULONG
PvfsPathCacheKey(
    PCVOID Key
    );

static
VOID
PvfsPathCacheFreeEntry(
    LWIO_LRU_ENTRY entry
    );

NTSTATUS
PvfsPathCacheInit(
    VOID
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PLWIO_LRU pLru = NULL;

    pthread_mutex_init(&gPathCacheLock, NULL);
    gpPathCacheLock = &gPathCacheLock;

    ntError = LwioLruCreate(
                gPvfsDriverConfig.PathCacheSize,
                0,
                PvfsPathCachePathCompare,
                PvfsPathCacheKey,
                PvfsPathCacheFreeEntry,
                &pLru);
    BAIL_ON_NT_STATUS(ntError);

    gpPathCache = pLru;
    pLru = NULL;

cleanup:
    LwioLruSafeFree(&pLru);

    return ntError;

error:
    goto cleanup;
}

VOID
PvfsPathCacheShutdown(
    VOID
    )
{
    BOOLEAN cacheLock = FALSE;

    if (gpPathCache == NULL)
    {
        goto cleanup;
    }

    LWIO_LOCK_MUTEX(cacheLock, &gPathCacheLock);

    LwioLruSafeFree(&gpPathCache);

    LWIO_UNLOCK_MUTEX(cacheLock, &gPathCacheLock);

    pthread_mutex_destroy(&gPathCacheLock);
    gpPathCacheLock = NULL;

cleanup:

    return;
}

////////////////////////////////////////////////////////////////////////

static
ULONG
PvfsPathCacheKey(
    PCVOID Key
    )
{
    ULONG KeyResult = 0;
    PPVFS_FILE_NAME FileNameKey = (PPVFS_FILE_NAME)Key;
    PCSTR cursor = NULL;

    for (cursor=FileNameKey->FileName; cursor && *cursor; cursor++)
    {
        KeyResult = (PVFS_PATH_HASH_MULTIPLIER * KeyResult) +
                    (ULONG)tolower(*cursor);
    }

    if (FileNameKey->StreamName)
    {
        for (cursor=FileNameKey->FileName; cursor && *cursor; cursor++)
        {
            KeyResult = (PVFS_PATH_HASH_MULTIPLIER * KeyResult) +
                        (ULONG)tolower(*cursor);
        }

        KeyResult = (PVFS_PATH_HASH_MULTIPLIER * KeyResult) + FileNameKey->Type;
    }

    return KeyResult;
}

////////////////////////////////////////////////////////////////////////


static
VOID
PvfsPathCacheFreeRecord(
    IN OUT PPVFS_PATH_CACHE_ENTRY pRecord
    )
{
    PvfsDestroyFileName(&pRecord->FileName);

    PVFS_FREE(&pRecord);

    return;
}


////////////////////////////////////////////////////////////////////////

static
VOID
PvfsPathCacheFreeEntry(
    LWIO_LRU_ENTRY entry
    )
{
    PPVFS_PATH_CACHE_ENTRY pRecord = NULL;

    pRecord = (PPVFS_PATH_CACHE_ENTRY)entry.pValue;

    if (pRecord)
    {
        PvfsPathCacheFreeRecord(pRecord);
        entry.pValue = NULL;
    }

    /* The Key was a pointer to the pszPathname in the record */

    return;
}

////////////////////////////////////////////////////////////////////////

static
LONG
PvfsPathCachePathCompare(
    PCVOID pKey1,
    PCVOID pKey2
    )
{
    PPVFS_FILE_NAME FileName1 = (PPVFS_FILE_NAME)pKey1;
    PPVFS_FILE_NAME FileName2 = (PPVFS_FILE_NAME)pKey2;

    return PvfsFileNameCompare(FileName1, FileName2, FALSE);
}

