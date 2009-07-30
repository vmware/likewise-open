/*
 * Copyright (c) Likewise Software.  All rights reserved.
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
 * license@likewise.com
 */

#include "npfs.h"


NTSTATUS
NpfsCreateSCB(
    PNPFS_IRP_CONTEXT pIrpContext,
    PNPFS_PIPE pPipe,
    PNPFS_CCB * ppSCB
    )
{
    NTSTATUS ntStatus = 0;
    PNPFS_CCB pSCB = NULL;

    ntStatus = NpfsAllocateMemory(
                    sizeof(*pSCB),
                    OUT_PPVOID(&pSCB)
                    );
    BAIL_ON_NT_STATUS(ntStatus);

    LwListInit(&pSCB->link);
    LwListInit(&pSCB->mdlList);

    pSCB->CcbType = NPFS_CCB_SERVER;
    pSCB->lRefCount = 1;
    pSCB->pPipe = pPipe;
    pPipe->pSCB = pSCB;

    NpfsAddRefPipe(pPipe);

    *ppSCB = pSCB;

cleanup:

    return ntStatus;

error:

    *ppSCB = NULL;

    goto cleanup;
}

NTSTATUS
NpfsCreateCCB(
    PNPFS_IRP_CONTEXT pIrpContext,
    PNPFS_PIPE pPipe,
    PNPFS_CCB * ppCCB
    )
{
    NTSTATUS ntStatus = 0;
    PNPFS_CCB pCCB = NULL;

    ntStatus = NpfsAllocateMemory(
                    sizeof(*pCCB),
                    OUT_PPVOID(&pCCB)
                    );
    BAIL_ON_NT_STATUS(ntStatus);

    LwListInit(&pCCB->link);
    LwListInit(&pCCB->mdlList);

    pCCB->CcbType = NPFS_CCB_CLIENT;
    pCCB->lRefCount = 1;
    pCCB->pPipe = pPipe;
    pPipe->pCCB = pCCB;

    NpfsAddRefPipe(pPipe);

    *ppCCB = pCCB;

cleanup:

    return ntStatus;

error:

    *ppCCB = NULL;

    goto cleanup;
}


VOID
NpfsReleaseCCB(
    PNPFS_CCB pCCB
    )
{
    if (InterlockedDecrement(&pCCB->lRefCount) == 0)
    {
        NpfsFreeCCB(pCCB);
    }
    return;
}

VOID
NpfsAddRefCCB(
    PNPFS_CCB pCCB
    )
{
    InterlockedIncrement(&pCCB->lRefCount);
}

VOID
NpfsFreeCCB(
    PNPFS_CCB pCCB
    )
{
    NpfsFreeMdlList(&pCCB->mdlList);
    NpfsReleasePipe(pCCB->pPipe);
    NpfsFreeMemory(pCCB);
}

NTSTATUS
NpfsGetCCB(
    IO_FILE_HANDLE FileHandle,
    PNPFS_CCB * ppCCB
    )
{
    NTSTATUS ntStatus = 0;
    PNPFS_CCB pCCB = NULL;

    pCCB = (PNPFS_CCB)IoFileGetContext(FileHandle);
    if (!pCCB)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    NpfsAddRefCCB(pCCB);

    *ppCCB = pCCB;

cleanup:

    return ntStatus;

error:
    *ppCCB = NULL;

    goto cleanup;

}

NTSTATUS
NpfsSetCCB(
    IO_FILE_HANDLE FileHandle,
    PNPFS_CCB pCCB
    )
{
    NTSTATUS ntStatus = 0;

    ntStatus = IoFileSetContext(FileHandle, pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    NpfsAddRefCCB(pCCB);

error:

    return (ntStatus);
}
