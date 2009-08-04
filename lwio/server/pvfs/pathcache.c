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

#define PVFS_PATH_CACHE_SIZE        1021
#define PVFS_PATH_HASH_MULTIPLIER   31
#define PVFS_PATH_CACHE_EXPIRY      120

typedef struct _PVFS_PATH_CACHE_ENTRY
{
    PSTR pszPathname;
    time_t LastAccess;

} PVFS_PATH_CACHE_ENTRY, *PPVFS_PATH_CACHE_ENTRY;



/*****************************************************************************
 ****************************************************************************/

NTSTATUS
PvfsPathCacheAdd(
    IN PCSTR pszResolvedPath
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    DWORD dwError = LWIO_ERROR_SUCCESS;
    PSTR pszPathname = NULL;
    PPVFS_PATH_CACHE_ENTRY pCacheRecord = NULL;
    BOOLEAN bLocked = FALSE;

    if (gpPathCache == NULL)
    {
        /* If the PathCache has been disabled or is not
           initialized, just report success and move on */
        ntError = STATUS_SUCCESS;
        goto cleanup;
    }

    ntError = LwRtlCStringDuplicate(&pszPathname, pszResolvedPath);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsAllocateMemory(
                  (PVOID*)&pCacheRecord,
                  sizeof(PVFS_PATH_CACHE_ENTRY));
    BAIL_ON_NT_STATUS(ntError);

    pCacheRecord->pszPathname = pszPathname;
    pCacheRecord->LastAccess = time(NULL);

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bLocked, &gPathCacheRwLock);
    dwError = SMBHashSetValue(
                  gpPathCache,
                  (PVOID)pszPathname,
                  (PVOID)pCacheRecord);
    LWIO_UNLOCK_RWMUTEX(bLocked, &gPathCacheRwLock);

    if (dwError != LWIO_ERROR_SUCCESS) {
        ntError = STATUS_INSUFFICIENT_RESOURCES;
    }
    BAIL_ON_NT_STATUS(ntError);

    pszPathname = NULL;
    pCacheRecord = NULL;

cleanup:
    LwRtlCStringFree(&pszPathname);
    PVFS_FREE(&pCacheRecord);

    return ntError;

error:
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
    DWORD dwError = FALSE;
    BOOLEAN bLocked = FALSE;
    PPVFS_PATH_CACHE_ENTRY pCacheRecord = NULL;
    PSTR pszResolvedPath = NULL;
    time_t now = 0;

    if (gpPathCache == NULL)
    {
        /* If the PathCache has been disabled, just fail
           the lookup and move on */
        ntError = STATUS_OBJECT_PATH_NOT_FOUND;
        BAIL_ON_NT_STATUS(ntError);
    }

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bLocked, &gPathCacheRwLock);
    dwError = SMBHashGetValue(
                  gpPathCache,
                  (PCVOID)pszOriginalPath,
                  (PVOID*)&pCacheRecord);
    if (dwError != LWIO_ERROR_SUCCESS) {
        ntError = STATUS_OBJECT_PATH_NOT_FOUND;
        BAIL_ON_NT_STATUS(ntError);
    }

    now = time(NULL);

    /* Expire cache records > EXPIRE seconds old.  Check for time warps
       that sent us back in time (system clock changed) */

    if (((pCacheRecord->LastAccess + PVFS_PATH_CACHE_EXPIRY) < now) ||
        (pCacheRecord->LastAccess > now))
    {
        dwError = SMBHashRemoveKey(gpPathCache, (PCVOID)pszOriginalPath);
        /* Ignore errors from the remove */

        ntError = STATUS_OBJECT_PATH_NOT_FOUND;
        BAIL_ON_NT_STATUS(ntError);
    }

    pCacheRecord->LastAccess = now;

    ntError = LwRtlCStringDuplicate(
                  &pszResolvedPath,
                  pCacheRecord->pszPathname);
    BAIL_ON_NT_STATUS(ntError);

    *ppszResolvedPath = pszResolvedPath;
    pszResolvedPath = NULL;

cleanup:
    LWIO_UNLOCK_RWMUTEX(bLocked, &gPathCacheRwLock);

    LwRtlCStringFree(&pszResolvedPath);

    return ntError;

error:
    goto cleanup;
}


/*****************************************************************************
 ****************************************************************************/

static int
PvfsPathCachePathCompare(
    PCVOID pKey1,
    PCVOID pKey2
    );

static size_t
PvfsPathCacheKey(
    PCVOID pszPath
    );

static VOID
PvfsPathCacheFreeEntry(
    const SMB_HASH_ENTRY *pEntry
    );

NTSTATUS
PvfsPathCacheInit(
    VOID
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    DWORD dwError = 0;
    PSMB_HASH_TABLE pHashTable = NULL;

    pthread_rwlock_init(&gPathCacheRwLock, NULL);

    dwError = SMBHashCreate(
                  PVFS_PATH_CACHE_SIZE,
                  PvfsPathCachePathCompare,
                  PvfsPathCacheKey,
                  PvfsPathCacheFreeEntry,
                  &pHashTable);
    BAIL_ON_LWIO_ERROR(dwError);

    gpPathCache = pHashTable;
    pHashTable = NULL;

cleanup:
    SMBHashSafeFree(&pHashTable);

    ntError = (dwError == LWIO_ERROR_SUCCESS) ?
               STATUS_SUCCESS :
               STATUS_INSUFFICIENT_RESOURCES;

    return ntError;

error:
    goto cleanup;
}


/*****************************************************************************
 ****************************************************************************/

static size_t
PvfsPathCacheKey(
    PCVOID pszPath
    )
{
    size_t KeyResult = 0;
    PCSTR pszPathname = (PCSTR)pszPath;
    PCSTR pszChar = NULL;

    for (pszChar=pszPathname; pszChar && *pszChar; pszChar++)
    {
        KeyResult = (PVFS_PATH_HASH_MULTIPLIER * KeyResult) + (size_t)*pszChar;
    }

    return KeyResult;
}


/*****************************************************************************
 ****************************************************************************/

static VOID
PvfsPathCacheFreeEntry(
    const SMB_HASH_ENTRY *pEntry
    )
{
    if (pEntry)
    {
        PSTR pszPath = (PSTR)pEntry->pKey;

        LwRtlCStringFree(&pszPath);

        PVFS_FREE(&pEntry->pValue);
    }

    return;
}


/*****************************************************************************
 ****************************************************************************/

static int
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
