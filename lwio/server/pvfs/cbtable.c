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
 *        cbtable.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        Control Block Table routines
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "pvfs.h"

#define PVFS_CB_TABLE_HASH_SIZE    127
#define PVFS_HASH_STRKEY_MULTIPLIER 31

/***********************************************************************
 **********************************************************************/

NTSTATUS
PvfsCbTableInitialize(
    PPVFS_CB_TABLE pCbTable
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;

    pthread_rwlock_init(&pCbTable->rwLock, NULL);

    ntError = PvfsHashTableCreate(
                  1021,
                  (PFN_LWRTL_RB_TREE_COMPARE)PvfsCbTableFilenameCompare,
                  (PFN_LWRTL_RB_TREE_FREE_KEY)PvfsCbTableFreeHashKey,
                  PvfsCbTableHashKey,
                  PvfsCbTableFreeHashEntry,
                  &pCbTable->pCbHashTable);
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    return ntError;

error:
    goto cleanup;
}

/***********************************************************************
 **********************************************************************/

NTSTATUS
PvfsCbTableDestroy(
    PPVFS_CB_TABLE pCbTable
    )
{
    BOOLEAN bLocked = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bLocked, &pCbTable->rwLock);
    PvfsHashTableDestroy(&pCbTable->pCbHashTable);
    LWIO_UNLOCK_RWMUTEX(bLocked, &pCbTable->rwLock);

    pthread_rwlock_destroy(&pCbTable->rwLock);

    PVFS_ZERO_MEMORY(pCbTable);

    return STATUS_SUCCESS;
}

/***********************************************************************
 **********************************************************************/

NTSTATUS
PvfsCbTableAdd_inlock(
    PPVFS_CB_TABLE_ENTRY pBucket,
    PSTR KeyString,
    PPVFS_CONTROL_BLOCK pCb
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PSTR newKeyString = NULL;

    ntError = LwRtlCStringDuplicate(&newKeyString, KeyString);
    BAIL_ON_NT_STATUS(ntError);

    ntError = LwRtlRBTreeAdd(pBucket->pTree, (PVOID)newKeyString, pCb);
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    return ntError;

error:
    if (newKeyString)
    {
        LwRtlCStringFree(&newKeyString);
    }

    goto cleanup;
}


/***********************************************************************
 **********************************************************************/

NTSTATUS
PvfsCbTableRemove_inlock(
    PPVFS_CB_TABLE_ENTRY pBucket,
    PSTR KeyString
    )
{
    return LwRtlRBTreeRemove(pBucket->pTree, (PVOID)KeyString);
}

/***********************************************************************
 **********************************************************************/

NTSTATUS
PvfsCbTableRemove(
    PPVFS_CB_TABLE_ENTRY pBucket,
    PSTR KeyString
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    BOOLEAN bLocked = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bLocked, &pBucket->rwLock);

    ntError = PvfsCbTableRemove_inlock(pBucket, KeyString);
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
PvfsCbTableLookup(
    PPVFS_CONTROL_BLOCK *ppCb,
    PPVFS_CB_TABLE_ENTRY pBucket,
    PSTR KeyString
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    BOOLEAN bLocked = FALSE;

    LWIO_LOCK_RWMUTEX_SHARED(bLocked, &pBucket->rwLock);

    ntError = PvfsCbTableLookup_inlock(ppCb, pBucket, KeyString);

    LWIO_UNLOCK_RWMUTEX(bLocked, &pBucket->rwLock);

    return ntError;
}


/***********************************************************************
 **********************************************************************/

NTSTATUS
PvfsCbTableLookup_inlock(
    PPVFS_CONTROL_BLOCK *ppCb,
    PPVFS_CB_TABLE_ENTRY pBucket,
    PCSTR KeyString
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_CONTROL_BLOCK pCb = NULL;

    ntError = LwRtlRBTreeFind(
                  pBucket->pTree,
                  (PVOID)KeyString,
                  (PVOID*)&pCb);
    if (ntError == STATUS_NOT_FOUND)
    {
        ntError = STATUS_OBJECT_NAME_NOT_FOUND;
    }
    BAIL_ON_NT_STATUS(ntError);

    *ppCb = PvfsReferenceCB(pCb);

cleanup:
    return ntError;

error:
    goto cleanup;
}

/****************************************************************************
 ***************************************************************************/

NTSTATUS
PvfsCbTableGetBucket(
    OUT PPVFS_CB_TABLE_ENTRY *ppBucket,
    IN PPVFS_CB_TABLE pCbTable,
    IN PVOID pKey
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    size_t sBucket = 0;
    PPVFS_CB_TABLE_ENTRY pBucket = NULL;
    PPVFS_HASH_TABLE pHashTable = pCbTable->pCbHashTable;

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

NTSTATUS
PvfsHashTableCreate(
    size_t sTableSize,
    PFN_LWRTL_RB_TREE_COMPARE fnCompare,
    PFN_LWRTL_RB_TREE_FREE_KEY fnFreeHashKey,
    PVFS_HASH_KEY fnHash,
    PVFS_HASH_FREE_ENTRY fnFree,
    PPVFS_HASH_TABLE* ppTable
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PPVFS_HASH_TABLE pTable = NULL;
    size_t sIndex = 0;
    PPVFS_CB_TABLE_ENTRY pNewEntry = NULL;

    ntError = PvfsAllocateMemory(
                  (PVOID*)&pTable,
                  sizeof(*pTable),
                  FALSE);
    BAIL_ON_NT_STATUS(ntError);

    pTable->sTableSize = sTableSize;
    pTable->sCount = 0;
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
                      fnCompare,
                      fnFreeHashKey,
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

    if (pNewEntry)
    {
        if (pNewEntry->pTree)
        {
            LwRtlRBTreeFree(pNewEntry->pTree);
            pNewEntry->pTree = NULL;
        }
        PvfsFreeMemory((PVOID*)&pNewEntry);
    }

    goto cleanup;
}

/****************************************************************************
 ***************************************************************************/

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

size_t
PvfsCbTableHashKey(
    PCVOID KeyString
    )
{
    size_t KeyResult = 0;
    PCSTR pszPathname = (PCSTR)KeyString;
    PCSTR pszChar = NULL;

    for (pszChar=pszPathname; pszChar && *pszChar; pszChar++)
    {
        KeyResult = (PVFS_HASH_STRKEY_MULTIPLIER * KeyResult) + (size_t)*pszChar;
    }

    return KeyResult;
}


/*****************************************************************************
 ****************************************************************************/

int
PvfsCbTableFilenameCompare(
    PCVOID a,
    PCVOID b
    )
{
    int result = 0;

    PSTR keyString1 = (PSTR)a;
    PSTR keyString2 = (PSTR)b;

    result = RtlCStringCompare(keyString1, keyString2, TRUE);

    return result;
}

VOID
PvfsCbTableFreeHashKey(
    PVOID KeyString
    )
{
    PSTR nameString = (PSTR)KeyString;

    if (nameString)
    {
        LwRtlCStringFree(&nameString);
    }

    return;
}


/*****************************************************************************
 ****************************************************************************/

VOID
PvfsCbTableFreeHashEntry(
    PPVFS_CB_TABLE_ENTRY *ppEntry
    )
{
    BOOLEAN bLocked = FALSE;
    PPVFS_CB_TABLE_ENTRY pEntry = ppEntry ? *ppEntry : NULL;

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


