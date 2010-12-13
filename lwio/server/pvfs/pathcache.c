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
    PSTR pszPathname;

} PVFS_PATH_CACHE_ENTRY, *PPVFS_PATH_CACHE_ENTRY;

/*****************************************************************************
 ****************************************************************************/

NTSTATUS
PvfsPathCacheAdd(
    IN PCSTR pszResolvedPath
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PPVFS_PATH_CACHE_ENTRY pCacheRecord = NULL;
    BOOLEAN bLocked = FALSE;

    if (gpPathCache == NULL)
    {
        /* If the PathCache has been disabled or is not
           initialized, just report success and move on */

        ntError = STATUS_SUCCESS;
        goto cleanup;
    }

    ntError = PvfsAllocateMemory(
                  (PVOID*)&pCacheRecord,
                  sizeof(PVFS_PATH_CACHE_ENTRY),
                  FALSE);
    BAIL_ON_NT_STATUS(ntError);

    ntError = LwRtlCStringDuplicate(
                  &pCacheRecord->pszPathname,
                  pszResolvedPath);
    BAIL_ON_NT_STATUS(ntError);

    LWIO_LOCK_MUTEX(bLocked, &gPathCacheLock);
    ntError = LwioLruSetValue(
                  gpPathCache,
                  (PVOID)pCacheRecord->pszPathname,
                  (PVOID)pCacheRecord);
    LWIO_UNLOCK_MUTEX(bLocked, &gPathCacheLock);

    BAIL_ON_NT_STATUS(ntError);

cleanup:
    return ntError;

error:
    if (pCacheRecord)
    {
        if (pCacheRecord->pszPathname)
        {
            LwRtlCStringFree(&pCacheRecord->pszPathname);
        }

        PVFS_FREE(&pCacheRecord);
    }

    goto cleanup;
}

/*****************************************************************************
 ****************************************************************************/

NTSTATUS
PvfsPathCacheLookup(
    OUT PSTR *ppszResolvedPath,
    IN  PCSTR pszOriginalPath
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    BOOLEAN bLocked = FALSE;
    PPVFS_PATH_CACHE_ENTRY pCacheRecord = NULL;
    PSTR pszResolvedPath = NULL;

    if (gpPathCache == NULL)
    {
        /* If the PathCache has been disabled, just fail
           the lookup and move on */
        ntError = STATUS_OBJECT_PATH_NOT_FOUND;
        BAIL_ON_NT_STATUS(ntError);
    }

    LWIO_LOCK_MUTEX(bLocked, &gPathCacheLock);
    ntError = LwioLruGetValue(
                  gpPathCache,
                  (PCVOID)pszOriginalPath,
                  (PVOID*)&pCacheRecord);
    BAIL_ON_NT_STATUS(ntError);

    ntError = LwRtlCStringDuplicate(
                  &pszResolvedPath,
                  pCacheRecord->pszPathname);
    BAIL_ON_NT_STATUS(ntError);

    *ppszResolvedPath = pszResolvedPath;

cleanup:
    LWIO_UNLOCK_MUTEX(bLocked, &gPathCacheLock);

    return ntError;

error:
    if (pszResolvedPath)
    {
        LwRtlCStringFree(&pszResolvedPath);
    }
    goto cleanup;
}


/*****************************************************************************
 ****************************************************************************/

NTSTATUS
PvfsPathCacheLookup2(
    OUT PSTR pszBuffer,
    IN  size_t sBufferLength,
    IN  PCSTR pszOriginalPath
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    BOOLEAN bLocked = FALSE;
    PPVFS_PATH_CACHE_ENTRY pCacheRecord = NULL;

    if (gpPathCache == NULL)
    {
        /* If the PathCache has been disabled, just fail
           the lookup and move on */
        ntError = STATUS_OBJECT_PATH_NOT_FOUND;
        BAIL_ON_NT_STATUS(ntError);
    }

    LWIO_LOCK_MUTEX(bLocked, &gPathCacheLock);
    ntError = LwioLruGetValue(
                  gpPathCache,
                  (PCVOID)pszOriginalPath,
                  (PVOID*)&pCacheRecord);
    BAIL_ON_NT_STATUS(ntError);

    strncpy(pszBuffer, pCacheRecord->pszPathname, sBufferLength-1);
    pszBuffer[sBufferLength-1] = '\0';

    ntError = STATUS_SUCCESS;

cleanup:
    LWIO_UNLOCK_MUTEX(bLocked, &gPathCacheLock);

    return ntError;

error:
    goto cleanup;
}

/*****************************************************************************
 ****************************************************************************/

NTSTATUS
PvfsPathCacheRemove(
    PCSTR pszPathname
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    BOOLEAN bLocked = FALSE;

    if (gpPathCache != NULL)
    {
        LWIO_LOCK_MUTEX(bLocked, &gPathCacheLock);

        /* Ignore errors from the remove */
        ntError = LwioLruRemove(gpPathCache, (PVOID)pszPathname);

        LWIO_UNLOCK_MUTEX(bLocked, &gPathCacheLock);
    }

    return STATUS_SUCCESS;
}


/*****************************************************************************
 ****************************************************************************/

static
LONG
PvfsPathCachePathCompare(
    PCVOID pKey1,
    PCVOID pKey2
    );

static
ULONG
PvfsPathCacheKey(
    PCVOID pszPath
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
    BOOLEAN bLocked = FALSE;

    if (gpPathCache == NULL)
    {
        goto cleanup;
    }

    LWIO_LOCK_MUTEX(bLocked, &gPathCacheLock);

    LwioLruSafeFree(&gpPathCache);

    LWIO_UNLOCK_MUTEX(bLocked, &gPathCacheLock);

    pthread_mutex_destroy(&gPathCacheLock);
    gpPathCacheLock = NULL;

cleanup:

    return;
}


/*****************************************************************************
 ****************************************************************************/

static
ULONG
PvfsPathCacheKey(
    PCVOID pszPath
    )
{
    ULONG KeyResult = 0;
    PCSTR pszPathname = (PCSTR)pszPath;
    PCSTR pszChar = NULL;

    for (pszChar=pszPathname; pszChar && *pszChar; pszChar++)
    {
        KeyResult = (PVFS_PATH_HASH_MULTIPLIER * KeyResult) +
                    (ULONG)tolower(*pszChar);
    }

    return KeyResult;
}


/*****************************************************************************
 ****************************************************************************/

static
VOID
PvfsPathCacheFreeEntry(
    LWIO_LRU_ENTRY entry
    )
{
    PPVFS_PATH_CACHE_ENTRY pRecord = NULL;

    pRecord = (PPVFS_PATH_CACHE_ENTRY)entry.pValue;

    LwRtlCStringFree(&pRecord->pszPathname);
    PVFS_FREE(&pRecord);

    /* The Key was a pointer to the pszPathname in the record */

    return;
}


/*****************************************************************************
 ****************************************************************************/

static
LONG
PvfsPathCachePathCompare(
    PCVOID pKey1,
    PCVOID pKey2
    )
{
    PSTR pszPath1 = (PSTR)pKey1;
    PSTR pszPath2 = (PSTR)pKey2;

    return strcasecmp(pszPath1, pszPath2);
}




/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
