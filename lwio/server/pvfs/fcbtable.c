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

/*****************************************************************************
 ****************************************************************************/

static
int
FcbTableFilenameCompare(
    PVOID a,
    PVOID b
    );

NTSTATUS
PvfsFcbTableInitialize(
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

static
int
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

/*****************************************************************************
 ****************************************************************************/

NTSTATUS
PvfsFcbTableDestroy(
    VOID
    )
{
    BOOLEAN bLocked = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bLocked, &gFcbTable.rwLock);

    /* Need a traversal to close all open CCBs first.  Then free the
       RBTree */
    LwRtlRBTreeFree(gFcbTable.pFcbTree);
    gFcbTable.pFcbTree = NULL;

    LWIO_UNLOCK_RWMUTEX(bLocked, &gFcbTable.rwLock);

    pthread_rwlock_destroy(&gFcbTable.rwLock);

    PVFS_ZERO_MEMORY(&gFcbTable);

    return STATUS_SUCCESS;
}


/***********************************************************************
 **********************************************************************/

NTSTATUS
PvfsFcbTableAdd(
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


/***********************************************************************
 **********************************************************************/

NTSTATUS
PvfsFcbTableRemove(
    PPVFS_FCB pFcb
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;

    /* We must have the mutex locked exclusively coming
       into this */

    ntError = LwRtlRBTreeRemove(gFcbTable.pFcbTree,
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
PvfsFcbTableLookup(
    PPVFS_FCB *ppFcb,
    PSTR pszFilename
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    BOOLEAN bLocked = FALSE;

    LWIO_LOCK_RWMUTEX_SHARED(bLocked, &gFcbTable.rwLock);

    ntError = PvfsFcbTableLookup_inlock(ppFcb, pszFilename);

    LWIO_UNLOCK_RWMUTEX(bLocked, &gFcbTable.rwLock);

    return ntError;
}


/***********************************************************************
 **********************************************************************/

NTSTATUS
PvfsFcbTableLookup_inlock(
    PPVFS_FCB *ppFcb,
    PCSTR pszFilename
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_FCB pFcb = NULL;

    ntError = LwRtlRBTreeFind(gFcbTable.pFcbTree,
                              (PVOID)pszFilename,
                              (PVOID*)&pFcb);
    if (ntError == STATUS_NOT_FOUND) {
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


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
