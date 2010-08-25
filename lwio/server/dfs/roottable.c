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
 *        roottable.c
 *
 * Abstract:
 *
 *        Likewise Distributed File System Driver (DFS)
 *
 *        DFS Root Control Block Table routineus
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "dfs.h"

static
int
DfsRootCtrlBlockTableCompare(
    PVOID pKey1,
    PVOID pKey2
    );

static
VOID
DfsRootCtrlBlockTableFreeKey(
    PVOID pKey
    );

static
VOID
DfsRootCtrlBlockTableFreeData(
    PVOID pData
    );


/***********************************************************************
 **********************************************************************/

NTSTATUS
DfsRootCtrlBlockTableInitialize(
    PDFS_ROOT_CONTROL_BLOCK_TABLE pRootCtrlBlockTable
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(pRootCtrlBlockTable, ntStatus);

    pthread_rwlock_init(&pRootCtrlBlockTable->RwLock, NULL);
    pRootCtrlBlockTable->pRwLock = &pRootCtrlBlockTable->RwLock;

    ntStatus = LwRtlRBTreeCreate(
                   DfsRootCtrlBlockTableCompare,
                   DfsRootCtrlBlockTableFreeKey,
                   DfsRootCtrlBlockTableFreeData,
                   &pRootCtrlBlockTable->pTable);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:
    return ntStatus;

error:
    if (pRootCtrlBlockTable)
    {
        if (pRootCtrlBlockTable->pRwLock)
        {
            pthread_rwlock_destroy(&pRootCtrlBlockTable->RwLock);
            pRootCtrlBlockTable->pRwLock = NULL;
        }

        if (pRootCtrlBlockTable->pTable)
        {
            LwRtlRBTreeFree(pRootCtrlBlockTable->pTable);
            pRootCtrlBlockTable->pTable = NULL;
        }
    }

    goto cleanup;
}


/***********************************************************************
 **********************************************************************/

static
int
DfsRootCtrlBlockTableCompare(
    PVOID pKey1,
    PVOID pKey2
    )
{
    return wc16scasecmp((const wchar16_t*)pKey1, (const wchar16_t*)pKey2);
}


/***********************************************************************
 **********************************************************************/

static
VOID
DfsRootCtrlBlockTableFreeKey(
    PVOID pKey
    )
{
    return;
}



/***********************************************************************
 **********************************************************************/

static
VOID
DfsRootCtrlBlockTableFreeData(
    PVOID pData
    )
{
    return;
}


/***********************************************************************
 **********************************************************************/

static
NTSTATUS
DfsRootCtrlBlockTableAdd_inlock(
    PDFS_ROOT_CONTROL_BLOCK_TABLE pRootTable,
    PDFS_ROOT_CONTROL_BLOCK pRootCB
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    ntStatus = LwRtlRBTreeAdd(
                   pRootTable->pTable,
                   pRootCB->pwszRootName,
                   pRootCB);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:
    return ntStatus;

error:
    goto cleanup;
}

/***********************************************************************
 **********************************************************************/

NTSTATUS
DfsRootCtrlBlockTableAdd(
    PDFS_ROOT_CONTROL_BLOCK_TABLE pRootTable,
    PDFS_ROOT_CONTROL_BLOCK pRootCB
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN bLocked = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bLocked, pRootTable->pRwLock);

    ntStatus = DfsRootCtrlBlockTableAdd_inlock(pRootTable, pRootCB);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:
    LWIO_UNLOCK_RWMUTEX(bLocked, pRootTable->pRwLock);

    return ntStatus;

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
