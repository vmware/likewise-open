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
 *        peeknp.c
 *
 * Abstract:
 *
 *        Likewise Named Pipe File System Driver (NPFS)
 *
 *       PeekNamedPipe Dispatch Routine
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "npfs.h"

NTSTATUS
NpfsPeekNamedPipe(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;
    PNPFS_IRP_CONTEXT pIrpContext = NULL;

    ntStatus = NpfsAllocateIrpContext(pIrp, &pIrpContext);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NpfsCommonPeekNamedPipe(pIrpContext, pIrp);
    BAIL_ON_NT_STATUS(ntStatus);

error:

    return ntStatus;
}


NTSTATUS
NpfsCommonPeekNamedPipe(
    PNPFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;
    PNPFS_CCB pCCB = NULL;

    pCCB = (PNPFS_CCB)IoFileGetContext(pIrpContext->pIrp->FileHandle);

    ntStatus = NpfsPeekNamedPipeFile(pCCB, pIrpContext);
    BAIL_ON_NT_STATUS(ntStatus);

error:

    return ntStatus;
}



NTSTATUS
NpfsPeekNamedPipeFile(
    PNPFS_CCB pCCB,
    PNPFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntStatus = 0;

    switch(pCCB->CcbType) {
    case NPFS_CCB_SERVER:
        ntStatus = NpfsServerPeekNamedPipeFile(pCCB, pIrpContext);
        BAIL_ON_NT_STATUS(ntStatus);
        break;

    case NPFS_CCB_CLIENT:
        ntStatus = NpfsClientPeekNamedPipeFile(pCCB, pIrpContext);
        BAIL_ON_NT_STATUS(ntStatus);
        break;
    }

error:
    return(ntStatus);
}


NTSTATUS
NpfsServerPeekNamedPipeFile(
    PNPFS_CCB pSCB,
    PNPFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntStatus = 0;
    PNPFS_PIPE pPipe = NULL;

    pPipe = pSCB->pPipe;
    ENTER_MUTEX(&pPipe->PipeMutex);

    switch(pPipe->PipeServerState) {

        case PIPE_SERVER_CONNECTED:
            while (NpfsMdlListIsEmpty(pSCB->pMdlList) &&
                (pPipe->PipeClientState == PIPE_CLIENT_CONNECTED)) {
                pthread_cond_wait(&pPipe->PipeCondition,&pPipe->PipeMutex);
            }
            if (pPipe->PipeClientState == PIPE_CLIENT_CLOSED){
                ntStatus = STATUS_PIPE_BROKEN;
                BAIL_ON_NT_STATUS(ntStatus);
            }
            ntStatus = NpfsServerPeekNamedPipeFile_Connected( pSCB, pIrpContext);
            BAIL_ON_NT_STATUS(ntStatus);
            break;
        case PIPE_SERVER_DISCONNECTED:
        case PIPE_SERVER_INIT_STATE:
        case PIPE_SERVER_WAITING_FOR_CONNECTION:
            break;
    }

error:

    pIrpContext->pIrp->IoStatusBlock.Status = ntStatus;
    LEAVE_MUTEX(&pPipe->PipeMutex);

    return ntStatus;
}

NTSTATUS
NpfsServerPeekNamedPipeFile_Connected(
    PNPFS_CCB pSCB,
    PNPFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntStatus = 0;
    PVOID pBuffer = NULL;
    ULONG Length = 0;
    ULONG ulBytesTransferred = 0;

    pBuffer = pIrpContext->pIrp->Args.PeekNamedPipeWrite.Buffer;
    Length = pIrpContext->pIrp->Args.PeekNamedPipeWrite.Length;

    ntStatus = NpfsDequeueBuffer(
                        pSCB->pMdlList,
                        pBuffer,
                        Length,
                        &ulBytesTransferred,
                        &pSCB->pMdlList
                        );
    BAIL_ON_NT_STATUS(ntStatus);
    pIrpContext->pIrp->IoStatusBlock.BytesTransferred = ulBytesTransferred;

error:

    pIrpContext->pIrp->IoStatusBlock.Status = ntStatus;

    return ntStatus;
}

NTSTATUS
NpfsClientPeekNamedPipeFile(
    PNPFS_CCB pCCB,
    PNPFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntStatus = 0;
    PNPFS_PIPE pPipe = NULL;

    pPipe = pCCB->pPipe;
    ENTER_MUTEX(&pPipe->PipeMutex);

    switch(pPipe->PipeClientState) {

        case PIPE_CLIENT_CONNECTED:
            while (NpfsMdlListIsEmpty(pCCB->pMdlList) &&
                    (pPipe->PipeServerState == PIPE_SERVER_CONNECTED)&&
                    (pPipe->PipeClientState == PIPE_CLIENT_CONNECTED)) {
                pthread_cond_wait(&pPipe->PipeCondition, &pPipe->PipeMutex);
            }

            if (pPipe->PipeServerState == PIPE_SERVER_CLOSED) {
                ntStatus = STATUS_PIPE_BROKEN;
                BAIL_ON_NT_STATUS(ntStatus);
            }

            if (pPipe->PipeClientState == PIPE_CLIENT_CLOSED) {
                ntStatus = STATUS_PIPE_BROKEN;
                BAIL_ON_NT_STATUS(ntStatus);
            }
            ntStatus = NpfsClientPeekNamedPipeFile_Connected(
                            pCCB,
                            pIrpContext
                            );
            BAIL_ON_NT_STATUS(ntStatus);

        case PIPE_SERVER_DISCONNECTED:
            //ntStatus = STATUS_PIPE_BROKEN;
            break;
    }

error:
    pIrpContext->pIrp->IoStatusBlock.Status = ntStatus;

    LEAVE_MUTEX(&pPipe->PipeMutex);

    return(ntStatus);
}


NTSTATUS
NpfsClientPeekNamedPipeFile_Connected(
    PNPFS_CCB pCCB,
    PNPFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntStatus = 0;
    PVOID pBuffer = NULL;
    ULONG Length = 0;
    ULONG ulBytesTransferred = 0;

    pBuffer = pIrpContext->pIrp->Args.PeekNamedPipeWrite.Buffer;
    Length = pIrpContext->pIrp->Args.PeekNamedPipeWrite.Length;

    ntStatus = NpfsDequeueBuffer(
                        pCCB->pMdlList,
                        pBuffer,
                        Length,
                        &ulBytesTransferred,
                        &pCCB->pMdlList
                        );
    BAIL_ON_NT_STATUS(ntStatus);
    pIrpContext->pIrp->IoStatusBlock.BytesTransferred = ulBytesTransferred;

error:

    pIrpContext->pIrp->IoStatusBlock.Status = ntStatus;
    return(ntStatus);
}
