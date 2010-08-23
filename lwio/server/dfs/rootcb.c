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
 *        ccb.c
 *
 * Abstract:
 *
 *        Likewise Distributed File System Driver (DFS)
 *
 *        DFS Root Control Block routineus
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "dfs.h"


/***********************************************************************
 **********************************************************************/

NTSTATUS
DfsAllocateRootCB(
    PDFS_ROOT_CONTROL_BLOCK *ppRootCB
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDFS_ROOT_CONTROL_BLOCK pRootCB = NULL;

    *ppRootCB = NULL;

    ntStatus = DfsAllocateMemory(
                   (PVOID*)&pRootCB,
                   sizeof(DFS_ROOT_CONTROL_BLOCK));
    BAIL_ON_NT_STATUS(ntStatus);

    pthread_rwlock_init(&pRootCB->RwLock, NULL);

    pRootCB->RefCount = 1;

    *ppRootCB = pRootCB;

    ntStatus = STATUS_SUCCESS;

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


/***********************************************************************
 **********************************************************************/

static
VOID
DfsFreeRootCB(
    PDFS_ROOT_CONTROL_BLOCK pRootCB
    )
{
    LwRtlCStringFree(&pRootCB->pszRootName);

    pthread_rwlock_destroy(&pRootCB->RwLock);

    DfsFreeMemory((PVOID*)&pRootCB);

    return;
}

/***********************************************************************
 **********************************************************************/

VOID
DfsReleaseRootCB(
    PDFS_ROOT_CONTROL_BLOCK pRootCB
    )
{
    if (InterlockedDecrement(&pRootCB->RefCount) == 0)
    {
        DfsFreeRootCB(pRootCB);
    }

    return;
}

/***********************************************************************
 **********************************************************************/

PDFS_ROOT_CONTROL_BLOCK
DfsReferenceRootCB(
    PDFS_ROOT_CONTROL_BLOCK pRootCB
    )
{
    InterlockedIncrement(&pRootCB->RefCount);

    return pRootCB;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
