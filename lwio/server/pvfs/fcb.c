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
 *        fcb.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        File Control Block routines
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "pvfs.h"

/* Forward declarations */


/* File Globals */

typedef struct _PVFS_FCB_TABLE
{
    pthread_rwlock_t rwLock;

    PLWRTL_RB_TREE pFcbTree;

} PVFS_FCB_TABLE;

static PVFS_FCB_TABLE gFcbTable;


/* Code */

/***********************************************************
 **********************************************************/

NTSTATUS
PvfsAllocateFCB(
    PPVFS_FCB *ppFcb
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_FCB pFcb = NULL;
    LONG NewRefCount = 0;

    *ppFcb = NULL;

    ntError = PvfsAllocateMemory((PVOID*)&pFcb, sizeof(PVFS_FCB));
    BAIL_ON_NT_STATUS(ntError);

    /* Initialize mutexes and refcounts */

    pthread_mutex_init(&pFcb->ControlBlock, NULL);

    /* Add initial ref count */

    pFcb->RefCount = 0;
    NewRefCount = InterlockedIncrement(&pFcb->RefCount);

    *ppFcb = pFcb;

    ntError = STATUS_SUCCESS;

cleanup:
    return ntError;

error:
    goto cleanup;
}

/*******************************************************
 ******************************************************/

static NTSTATUS
PvfsRemoveFCB(
    PPVFS_FCB pFcb
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;

    ntError= LwRtlRBTreeRemove(gFcbTable.pFcbTree,
                               (PVOID)pFcb->pszFilename);
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    return ntError;

error:
    goto cleanup;
}

/*******************************************************
 ******************************************************/

static NTSTATUS
PvfsFreeFCB(
    PPVFS_FCB pFcb
    )
{
    if (pFcb->pszFilename) {
        RtlCStringFree(&pFcb->pszFilename);
    }

    pthread_mutex_destroy(&pFcb->ControlBlock);

    PvfsFreeMemory(pFcb);

    return STATUS_SUCCESS;
}

/*******************************************************
 ******************************************************/

VOID
PvfsReleaseFCB(
    PPVFS_FCB pFcb
    )
{
    if (InterlockedDecrement(&pFcb->RefCount) == 0)
    {
        PvfsRemoveFCB(pFcb);
        PvfsFreeFCB(pFcb);
    }

    return;
}

/*******************************************************
 ******************************************************/

static int
FcbTableFilenameCompare(
    PVOID a,
    PVOID b
    )
{
    int iReturn = 0;

    PSTR pszFilename1 = (PSTR)a;
    PSTR pszFilename2 = (PSTR)b;

    iReturn = RtlCStringCompare(pszFilename1, pszFilename2, TRUE);

    return iReturn;
}

/*******************************************************
 ******************************************************/

NTSTATUS
PvfsInitializeFCBTable(
    VOID
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;

    pthread_rwlock_init(&gFcbTable.rwLock, NULL);

    ntError = LwRtlRBTreeCreate(&FcbTableFilenameCompare,
                                NULL,
                                NULL,
                                &gFcbTable.pFcbTree);
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    return ntError;

error:
    goto cleanup;
}

/*******************************************************
 ******************************************************/

NTSTATUS
PvfsDestroyFCBTable(
    VOID
    )
{
    /* Need to add tree traversal here and remove
       data */

    LwRtlRBTreeFree(gFcbTable.pFcbTree);
    pthread_rwlock_destroy(&gFcbTable.rwLock);

    PVFS_ZERO_MEMORY(&gFcbTable);

    return STATUS_SUCCESS;
}

/*******************************************************
 ******************************************************/

NTSTATUS
PvfsFindFCB(
    PPVFS_FCB *ppFcb,
    PSTR pszFilename
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_FCB pFcb = NULL;
    LONG NewRefCount = 0;

    /* LOCK_READ_ACCESS(gTable) */

    ntError = LwRtlRBTreeFind(gFcbTable.pFcbTree,
                              (PVOID)pszFilename,
                              (PVOID*)&pFcb);
    if (ntError == STATUS_NOT_FOUND) {
        ntError = STATUS_OBJECT_NAME_NOT_FOUND;
    }
    BAIL_ON_NT_STATUS(ntError);

    NewRefCount = InterlockedIncrement(&pFcb->RefCount);

    *ppFcb = pFcb;
    ntError = STATUS_SUCCESS;

cleanup:
    /* UNLOCK_READ_ACCESS(gTable) */

    return ntError;

error:
    goto cleanup;
}

/*******************************************************
 ******************************************************/

static NTSTATUS
PvfsAddFCB(
    PPVFS_FCB pFcb
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;

    ntError = LwRtlRBTreeAdd(gFcbTable.pFcbTree,
                             (PVOID)pFcb->pszFilename,
                             (PVOID)pFcb);
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    return ntError;

error:
    goto cleanup;
}

/*******************************************************
 ******************************************************/

NTSTATUS
PvfsCreateFCB(
    OUT PPVFS_FCB *ppFcb,
    IN  PSTR pszFilename,
    IN  FILE_SHARE_FLAGS ShareAccess
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_FCB pFcb = NULL;

    ntError = PvfsAllocateFCB(&pFcb);
    BAIL_ON_NT_STATUS(ntError);

    ntError = RtlCStringDuplicate(&pFcb->pszFilename, pszFilename);
    BAIL_ON_NT_STATUS(ntError);

    pFcb->ShareAccess = ShareAccess;

    /* Add to the file handle table */

    ntError = PvfsAddFCB(pFcb);
    BAIL_ON_NT_STATUS(ntError);

    /* Return a reference to the FCB */

    *ppFcb = pFcb;
    ntError = STATUS_SUCCESS;

cleanup:
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
