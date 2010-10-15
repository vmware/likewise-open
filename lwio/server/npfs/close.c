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
 *        close.c
 *
 * Abstract:
 *
 *        Likewise Named Pipe File System Driver (NPFS)
 *
 *        Close Dispatch Function
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "includes.h"

NTSTATUS
NpfsClose(
    IO_DEVICE_HANDLE DeviceHandle,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;
    PNPFS_IRP_CONTEXT pIrpContext = NULL;

    ntStatus = NpfsAllocateIrpContext(
                        pIrp,
                        &pIrpContext
                        );
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NpfsCommonClose(
                        pIrpContext,
                        pIrp);
    BAIL_ON_NT_STATUS(ntStatus);

error:

    if(pIrpContext) {
        NpfsFreeIrpContext(pIrpContext);
    }

    return ntStatus;
}


NTSTATUS
NpfsCommonClose(
    PNPFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;
    PNPFS_CCB pCCB = NULL;

    ntStatus = NpfsGetCCB(pIrpContext->pIrp->FileHandle, &pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NpfsCloseHandle(pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

error:

    return(ntStatus);
}



NTSTATUS
NpfsCloseHandle(
    PNPFS_CCB pCCB
    )
{
    NTSTATUS ntStatus = 0;

    switch (pCCB->CcbType) {

    case NPFS_CCB_SERVER:
        ntStatus = NpfsServerCloseHandle(pCCB);
        BAIL_ON_NT_STATUS(ntStatus);
        break;

    case NPFS_CCB_CLIENT:
        ntStatus = NpfsClientCloseHandle(pCCB);
        BAIL_ON_NT_STATUS(ntStatus);
        break;
    }

error:

    return(ntStatus);
}

NTSTATUS
NpfsServerCloseHandle(
    PNPFS_CCB pSCB
    )
{
    NTSTATUS ntStatus = 0;
    PNPFS_PIPE pPipe = NULL;
    PNPFS_CCB pCCB = NULL;
    PLW_LIST_LINKS pLink = NULL;
    PNPFS_IRP_CONTEXT pReadContext = NULL;

    pPipe = pSCB->pPipe;

    ENTER_MUTEX(&pPipe->PipeMutex);

    pCCB = pPipe->pCCB;

    pPipe->PipeServerState = PIPE_SERVER_CLOSED;

    while (pCCB && !LwListIsEmpty(&pCCB->ReadIrpList))
    {
        pLink = pCCB->ReadIrpList.Next;
        LwListRemove(pLink);

        pReadContext = LW_STRUCT_FROM_FIELD(pLink, NPFS_IRP_CONTEXT, Link);

        NpfsClientCompleteReadFile(pCCB, pReadContext);
    }

    pthread_cond_signal(&pPipe->PipeCondition);

    if (pPipe->PipeClientState == PIPE_CLIENT_CLOSED)
    {
        ntStatus = NpfsFreePipeContext(pPipe);
        BAIL_ON_NT_STATUS(ntStatus);
    }

error:

    pPipe->pSCB = NULL;

    LEAVE_MUTEX(&pPipe->PipeMutex);

    NpfsReleaseCCB(pSCB);

    return ntStatus;
}



NTSTATUS
NpfsClientCloseHandle(
    PNPFS_CCB pCCB
    )
{
    NTSTATUS ntStatus = 0;
    PNPFS_PIPE pPipe = NULL;
    PNPFS_CCB pSCB = NULL;
    PLW_LIST_LINKS pLink = NULL;
    PNPFS_IRP_CONTEXT pReadContext = NULL;

    pPipe = pCCB->pPipe;

    ENTER_MUTEX(&pPipe->PipeMutex);

    pSCB = pPipe->pSCB;

    pPipe->PipeClientState = PIPE_CLIENT_CLOSED;

    while (pSCB && !LwListIsEmpty(&pSCB->ReadIrpList))
    {
        pLink = pSCB->ReadIrpList.Next;
        LwListRemove(pLink);

        pReadContext = LW_STRUCT_FROM_FIELD(pLink, NPFS_IRP_CONTEXT, Link);

        NpfsServerCompleteReadFile(pSCB, pReadContext);
    }

    pthread_cond_signal(&pPipe->PipeCondition);

    if (pPipe->PipeServerState == PIPE_SERVER_CLOSED)
    {
        ntStatus = NpfsFreePipeContext(pPipe);
        BAIL_ON_NT_STATUS(ntStatus);
    }

error:

    pPipe->pCCB = NULL;

    LEAVE_MUTEX(&pPipe->PipeMutex);

    NpfsReleaseCCB(pCCB);

    return(ntStatus);
}
