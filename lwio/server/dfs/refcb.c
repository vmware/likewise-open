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
 *        Create Control Block routineus
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "dfs.h"


/***********************************************************************
 **********************************************************************/

NTSTATUS
DfsAllocateCCB(
    PDFS_CREATE_CONTROL_BLOCK *ppCcb
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDFS_CREATE_CONTROL_BLOCK pCcb = NULL;

    *ppCcb = NULL;

    ntStatus = DfsAllocateMemory(
                   (PVOID*)&pCcb,
                   sizeof(DFS_CREATE_CONTROL_BLOCK));
    BAIL_ON_NT_STATUS(ntStatus);

    pthread_mutex_init(&pCcb->Mutex, NULL);

    pCcb->RefCount = 1;

    *ppCcb = pCcb;

    InterlockedIncrement(&gDfsObjectCounter.CcbCount);

    ntStatus = STATUS_SUCCESS;

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


/***********************************************************************
 **********************************************************************/

VOID
DfsFreeCCB(
    PDFS_CREATE_CONTROL_BLOCK pCcb
    )
{
    LwRtlCStringFree(&pCcb->pszFilename);

    pthread_mutex_destroy(&pCcb->Mutex);

    DfsFreeMemory((PVOID*)&pCcb);

    InterlockedDecrement(&gDfsObjectCounter.CcbCount);

    return;
}

/***********************************************************************
 **********************************************************************/

VOID
DfsReleaseCCB(
    PDFS_CREATE_CONTROL_BLOCK pCcb
    )
{
    if (InterlockedDecrement(&pCcb->RefCount) == 0)
    {
        DfsFreeCCB(pCcb);
    }

    return;
}

/***********************************************************************
 **********************************************************************/

PDFS_CREATE_CONTROL_BLOCK
DfsReferenceCCB(
    PDFS_CREATE_CONTROL_BLOCK pCcb
    )
{
    InterlockedIncrement(&pCcb->RefCount);

    return pCcb;
}


/***********************************************************************
 **********************************************************************/

static NTSTATUS
DfsAcquireCCBInternal(
    IO_FILE_HANDLE FileHandle,
    PDFS_CREATE_CONTROL_BLOCK * ppCcb,
    BOOLEAN bIncRef
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDFS_CREATE_CONTROL_BLOCK pCcb = (PDFS_CREATE_CONTROL_BLOCK)IoFileGetContext(FileHandle);

    // DFS_BAIL_ON_INVALID_CCB(pCcb, ntStatus);

    if (bIncRef)
    {
        InterlockedIncrement(&pCcb->RefCount);
    }

    *ppCcb = pCcb;

    return ntStatus;
}

/***********************************************************************
 **********************************************************************/

NTSTATUS
DfsAcquireCCB(
    IO_FILE_HANDLE FileHandle,
    PDFS_CREATE_CONTROL_BLOCK * ppCcb
    )
{
    return DfsAcquireCCBInternal(FileHandle, ppCcb, TRUE);
}


/***********************************************************************
 **********************************************************************/

NTSTATUS
DfsAcquireCCBClose(
    IO_FILE_HANDLE FileHandle,
    PDFS_CREATE_CONTROL_BLOCK * ppCcb
    )
{
    return DfsAcquireCCBInternal(FileHandle, ppCcb, FALSE);
}


/***********************************************************************
 **********************************************************************/

NTSTATUS
DfsStoreCCB(
    IO_FILE_HANDLE FileHandle,
    PDFS_CREATE_CONTROL_BLOCK pCcb
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    ntStatus = IoFileSetContext(FileHandle, (PVOID)pCcb);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:
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
