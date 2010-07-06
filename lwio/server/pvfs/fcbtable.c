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
 *        fcbtable.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        File Control Block Table routines
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "pvfs.h"

#define PVFS_FCB_TABLE_HASH_SIZE    127
#define PVFS_HASH_STRKEY_MULTIPLIER 31

static
NTSTATUS
PvfsHashTableCreate(
    size_t sTableSize,
    PVFS_HASH_KEY_COMPARE fnCompare,
    PVFS_HASH_KEY fnHash,
    PVFS_HASH_FREE_ENTRY fnFree,
    PPVFS_HASH_TABLE* ppHashTable
    );

static
VOID
PvfsHashTableDestroy(
    PVFS_HASH_TABLE** ppTable
    );

NTSTATUS
PvfsHashTableGetValue(
    PPVFS_HASH_TABLE pTable,
    PVOID  pKey,
    PPVFS_FCB *ppFcb
    );

NTSTATUS
PvfsHashTableSetValue(
    PPVFS_HASH_TABLE pTable,
    PVOID  pKey,
    PPVFS_FCB pFcb
    );

NTSTATUS
PvfsHashTableRemoveKey(
    PVFS_HASH_TABLE *pTable,
    PVOID  pKey
    );

static
size_t
PvfsFcbTableHashKey(
    PCVOID pszPath
    );

static
VOID
PvfsFcbTableFreeHashEntry(
    PPVFS_FCB_TABLE_ENTRY *ppEntry
    );

static
int
PvfsFcbTableFilenameCompare(
    PCVOID a,
    PCVOID b
    );

/*****************************************************************************
 ****************************************************************************/


NTSTATUS
PvfsFcbTableInitialize(
    VOID
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;

    pthread_rwlock_init(&gFcbTable.rwLock, NULL);

    ntError = PvfsHashTableCreate(
                  1021,
                  PvfsFcbTableFilenameCompare,
                  PvfsFcbTableHashKey,
                  PvfsFcbTableFreeHashEntry,
                  &gFcbTable.pFcbTable);
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    return ntError;

error:
    goto cleanup;
}


/*****************************************************************************
 ****************************************************************************/

NTSTATUS
PvfsFcbTableDestroy(
    VOID
    )
{
    BOOLEAN bLocked = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bLocked, &gFcbTable.rwLock);
    PvfsHashTableDestroy(&gFcbTable.pFcbTable);
    LWIO_UNLOCK_RWMUTEX(bLocked, &gFcbTable.rwLock);

    pthread_rwlock_destroy(&gFcbTable.rwLock);

    PVFS_ZERO_MEMORY(&gFcbTable);

    return STATUS_SUCCESS;
}


/***********************************************************************
 **********************************************************************/

NTSTATUS
PvfsFcbTableAdd_inlock(
    PPVFS_FCB_TABLE_ENTRY pBucket,
    PPVFS_FCB pFcb
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;

    ntError = LwRtlRBTreeAdd(
                  pBucket->pTree,
                  (PVOID)pFcb->pszFilename,
                  (PVOID)pFcb);
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    return ntError;

error:
    goto cleanup;
}


/***********************************************************************
 **********************************************************************/

NTSTATUS
PvfsFcbTableRemove_inlock(
    PPVFS_FCB_TABLE_ENTRY pBucket,
    PPVFS_FCB pFcb
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;

    ntError = LwRtlRBTreeRemove(
                  pBucket->pTree,
                  (PVOID)pFcb->pszFilename);
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    return ntError;

error:
    goto cleanup;
}

/***********************************************************************
 **********************************************************************/

NTSTATUS
PvfsFcbTableRemove(
    PPVFS_FCB_TABLE_ENTRY pBucket,
    PPVFS_FCB pFcb
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    BOOLEAN bLocked = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bLocked, &pBucket->rwLock);

    ntError = PvfsFcbTableRemove_inlock(pBucket, pFcb);
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    LWIO_UNLOCK_RWMUTEX(bLocked, &pBucket->rwLock);

    return ntError;

error:
    goto cleanup;
}


/***********************************************************************
 **********************************************************************/

NTSTATUS
PvfsFcbTableLookup(
    PPVFS_FCB *ppFcb,
    PPVFS_FCB_TABLE_ENTRY pBucket,
    PSTR pszFilename
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    BOOLEAN bLocked = FALSE;

    LWIO_LOCK_RWMUTEX_SHARED(bLocked, &pBucket->rwLock);

    ntError = PvfsFcbTableLookup_inlock(ppFcb, pBucket, pszFilename);

    LWIO_UNLOCK_RWMUTEX(bLocked, &pBucket->rwLock);

    return ntError;
}


/***********************************************************************
 **********************************************************************/

NTSTATUS
PvfsFcbTableLookup_inlock(
    PPVFS_FCB *ppFcb,
    PPVFS_FCB_TABLE_ENTRY pBucket,
    PCSTR pszFilename
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_FCB pFcb = NULL;

    ntError = LwRtlRBTreeFind(
                  pBucket->pTree,
                  (PVOID)pszFilename,
                  (PVOID*)&pFcb);
    if (ntError == STATUS_NOT_FOUND)
    {
        ntError = STATUS_OBJECT_NAME_NOT_FOUND;
    }
    BAIL_ON_NT_STATUS(ntError);

    *ppFcb = PvfsReferenceFCB(pFcb);
    ntError = STATUS_SUCCESS;

cleanup:
    return ntError;

error:
    goto cleanup;
}

/****************************************************************************
 ***************************************************************************/

NTSTATUS
PvfsFcbTableGetBucket(
    OUT PPVFS_FCB_TABLE_ENTRY *ppBucket,
    IN PPVFS_FCB_TABLE pFcbTable,
    IN PVOID pKey
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    size_t sBucket = 0;
    PPVFS_FCB_TABLE_ENTRY pBucket = NULL;
    PPVFS_HASH_TABLE pHashTable = pFcbTable->pFcbTable;

    if (pHashTable->sTableSize > 0)
    {
        sBucket = pHashTable->fnHash(pKey) % pHashTable->sTableSize;
        pBucket = pHashTable->ppEntries[sBucket];
    }

    if (pBucket == NULL)
    {
        ntError = STATUS_NOT_FOUND;
        BAIL_ON_NT_STATUS(ntError);
    }

    *ppBucket = pBucket;

cleanup:
    return ntError;

error:
    goto cleanup;
}


/****************************************************************************
 ***************************************************************************/

static
NTSTATUS
PvfsHashTableCreate(
    size_t sTableSize,
    PVFS_HASH_KEY_COMPARE fnCompare,
    PVFS_HASH_KEY fnHash,
    PVFS_HASH_FREE_ENTRY fnFree,
    PPVFS_HASH_TABLE* ppTable
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PPVFS_HASH_TABLE pTable = NULL;
    size_t sIndex = 0;
    PPVFS_FCB_TABLE_ENTRY pNewEntry = NULL;

    ntError = PvfsAllocateMemory(
                  (PVOID*)&pTable,
                  sizeof(*pTable),
                  FALSE);
    BAIL_ON_NT_STATUS(ntError);

    pTable->sTableSize = sTableSize;
    pTable->sCount = 0;
    pTable->fnCompare = fnCompare;
    pTable->fnHash = fnHash;
    pTable->fnFree = fnFree;

    ntError = PvfsAllocateMemory(
                  (PVOID*)&pTable->ppEntries,
                  sizeof(*pTable->ppEntries) * sTableSize,
                  TRUE);
    BAIL_ON_NT_STATUS(ntError);

    // Allocating the entire hash table of buckets saves a required
    // exclusive lock on the entire hash table later when adding
    // new entries

    for (sIndex=0; sIndex < pTable->sTableSize; sIndex++)
    {
        ntError = PvfsAllocateMemory(
                      (PVOID*)&pNewEntry,
                      sizeof(*pNewEntry),
                      FALSE);
        BAIL_ON_NT_STATUS(ntError);

        pthread_rwlock_init(&pNewEntry->rwLock, NULL);
        pNewEntry->pRwLock = &pNewEntry->rwLock;

        ntError = LwRtlRBTreeCreate(
                      (PFN_LWRTL_RB_TREE_COMPARE)pTable->fnCompare,
                      NULL,
                      NULL,
                      &pNewEntry->pTree);
        BAIL_ON_NT_STATUS(ntError);

        pTable->ppEntries[sIndex] = pNewEntry;
        pNewEntry = NULL;
    }

    *ppTable = pTable;

cleanup:
    return ntError;

error:
    PvfsHashTableDestroy(&pTable);

    goto cleanup;
}

/****************************************************************************
 ***************************************************************************/

static
VOID
PvfsHashTableDestroy(
    PPVFS_HASH_TABLE *ppTable
    )
{
    DWORD dwIndex = 0;
    PPVFS_HASH_TABLE pTable = NULL;

    if (ppTable && *ppTable)
    {
        pTable = *ppTable;

        for (dwIndex=0; dwIndex < pTable->sTableSize; dwIndex++)
        {
            if (pTable->ppEntries[dwIndex] == NULL)
            {
                continue;
            }

            pTable->fnFree(&pTable->ppEntries[dwIndex]);
        }

        PVFS_FREE(&pTable->ppEntries);
        PVFS_FREE(&pTable);

        *ppTable = NULL;
    }

    return;
}


/*****************************************************************************
 ****************************************************************************/

static
size_t
PvfsFcbTableHashKey(
    PCVOID pszPath
    )
{
    size_t KeyResult = 0;
    PCSTR pszPathname = (PCSTR)pszPath;
    PCSTR pszChar = NULL;

    for (pszChar=pszPathname; pszChar && *pszChar; pszChar++)
    {
        KeyResult = (PVFS_HASH_STRKEY_MULTIPLIER * KeyResult) + (size_t)*pszChar;
    }

    return KeyResult;
}


/*****************************************************************************
 ****************************************************************************/

static
int
PvfsFcbTableFilenameCompare(
    PCVOID a,
    PCVOID b
    )
{
    int iReturn = 0;

    PSTR pszFilename1 = (PSTR)a;
    PSTR pszFilename2 = (PSTR)b;

    iReturn = RtlCStringCompare(pszFilename1, pszFilename2, TRUE);

    return iReturn;
}


/*****************************************************************************
 ****************************************************************************/

static
VOID
PvfsFcbTableFreeHashEntry(
    PPVFS_FCB_TABLE_ENTRY *ppEntry
    )
{
    BOOLEAN bLocked = FALSE;
    PPVFS_FCB_TABLE_ENTRY pEntry = ppEntry ? *ppEntry : NULL;

    if (pEntry)
    {
        if (pEntry->pRwLock)
        {
            LWIO_LOCK_RWMUTEX_EXCLUSIVE(bLocked, &pEntry->rwLock);
            if (pEntry->pTree)
            {
                LwRtlRBTreeFree(pEntry->pTree);
            }
            LWIO_UNLOCK_RWMUTEX(bLocked, &pEntry->rwLock);

            pthread_rwlock_destroy(&pEntry->rwLock);
        }


        PVFS_FREE(&pEntry);

        *ppEntry = NULL;
    }

    return;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
