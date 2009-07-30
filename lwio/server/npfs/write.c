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
 *        write.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (NPFS)
 *
 *       Write Dispatch Routine
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "npfs.h"

NTSTATUS
NpfsWrite(
    IO_DEVICE_HANDLE IoDeviceHandle,
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


    ntStatus = NpfsCommonWrite(pIrpContext, pIrp);
    BAIL_ON_NT_STATUS(ntStatus);

error:

    if(pIrpContext) {
        NpfsFreeIrpContext(pIrpContext);
    }

    return ntStatus;
}



NTSTATUS
NpfsCommonWrite(
    PNPFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;
    PNPFS_CCB pCCB = NULL;

    ntStatus = NpfsGetCCB(
                    pIrpContext->pIrp->FileHandle,
                    &pCCB
                    );
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NpfsWriteFile(
                    pCCB,
                    pIrpContext
                    );
    BAIL_ON_NT_STATUS(ntStatus);


error:

    if (pCCB) {
        NpfsReleaseCCB(pCCB);
    }
    return(ntStatus);
}



NTSTATUS
NpfsWriteFile(
    PNPFS_CCB pCCB,
    PNPFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntStatus = 0;

    switch(pCCB->CcbType) {

        case NPFS_CCB_SERVER:
            ntStatus = NpfsServerWriteFile(
                            pCCB,
                            pIrpContext
                            );
            BAIL_ON_NT_STATUS(ntStatus);
            break;

        case NPFS_CCB_CLIENT:
            ntStatus = NpfsClientWriteFile(
                            pCCB,
                            pIrpContext
                            );
            BAIL_ON_NT_STATUS(ntStatus);
            break;
    }

error:

    return(ntStatus);
}



NTSTATUS
NpfsServerWriteFile(
    PNPFS_CCB pSCB,
    PNPFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntStatus = 0;
    PNPFS_PIPE pPipe = NULL;

    NpfsAddRefCCB(pSCB);

    pPipe = pSCB->pPipe;
    ENTER_MUTEX(&pPipe->PipeMutex);

    switch(pPipe->PipeServerState)
    {
    case PIPE_SERVER_CONNECTED:
        ntStatus = NpfsServerWriteFile_Connected(
            pSCB,
            pIrpContext
            );
        pthread_cond_signal(&pPipe->PipeCondition);
        BAIL_ON_NT_STATUS(ntStatus);
        break;
    case PIPE_SERVER_WAITING_FOR_CONNECTION:
    case PIPE_SERVER_INIT_STATE:
    case PIPE_SERVER_DISCONNECTED:
    case PIPE_SERVER_CREATED:
    case PIPE_SERVER_CLOSED:
        break;

    }

error:
    pIrpContext->pIrp->IoStatusBlock.Status = ntStatus;
    LEAVE_MUTEX(&pPipe->PipeMutex);

    NpfsReleaseCCB(pSCB);

    return(ntStatus);
}


NTSTATUS
NpfsClientWriteFile(
    PNPFS_CCB pCCB,
    PNPFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntStatus = 0;
    PNPFS_PIPE pPipe = NULL;

    NpfsAddRefCCB(pCCB);

    pPipe = pCCB->pPipe;
    ENTER_MUTEX(&pPipe->PipeMutex);

    switch(pPipe->PipeClientState)
    {
    case PIPE_CLIENT_CONNECTED:
        ntStatus = NpfsClientWriteFile_Connected(
            pCCB,
            pIrpContext);
        pthread_cond_signal(&pPipe->PipeCondition);
        BAIL_ON_NT_STATUS(ntStatus);
        break;
    case PIPE_CLIENT_INIT_STATE:
    case PIPE_CLIENT_CLOSED:
        break;
    }
error:

    pIrpContext->pIrp->IoStatusBlock.Status = ntStatus;
    LEAVE_MUTEX(&pPipe->PipeMutex);

    NpfsReleaseCCB(pCCB);

    return(ntStatus);
}



NTSTATUS
NpfsServerWriteFile_Connected(
    PNPFS_CCB pSCB,
    PNPFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntStatus = 0;
    PNPFS_PIPE pPipe = NULL;
    PNPFS_CCB pCCB = NULL;
    PVOID pBuffer = NULL;
    ULONG Length = 0;
    ULONG ulBytesTransferred = 0;

    switch (pIrpContext->pIrp->Type)
    {
        case IRP_TYPE_FS_CONTROL:

            pBuffer = pIrpContext->pIrp->Args.IoFsControl.InputBuffer;
            Length = pIrpContext->pIrp->Args.IoFsControl.InputBufferLength;

            break;

        default:

            pBuffer = pIrpContext->pIrp->Args.ReadWrite.Buffer;
            Length = pIrpContext->pIrp->Args.ReadWrite.Length;

            break;
    }

    pPipe = pSCB->pPipe;
    pCCB = pPipe->pCCB;

    ntStatus = NpfsEnqueueBuffer(
                        &pCCB->mdlList,
                        pBuffer,
                        Length,
                        &ulBytesTransferred
                        );
    BAIL_ON_NT_STATUS(ntStatus);

    pIrpContext->pIrp->IoStatusBlock.BytesTransferred = ulBytesTransferred;
error:

    pIrpContext->pIrp->IoStatusBlock.Status = ntStatus;

    return(ntStatus);
}

NTSTATUS
NpfsClientWriteFile_Connected(
    PNPFS_CCB pCCB,
    PNPFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntStatus = 0;
    PNPFS_PIPE pPipe = NULL;
    PNPFS_CCB pSCB = NULL;
    PVOID pBuffer = NULL;
    ULONG Length = 0;
    ULONG ulBytesTransferred = 0;

    switch (pIrpContext->pIrp->Type)
    {
        case IRP_TYPE_FS_CONTROL:

            pBuffer = pIrpContext->pIrp->Args.IoFsControl.InputBuffer;
            Length = pIrpContext->pIrp->Args.IoFsControl.InputBufferLength;

            break;

        default:

            pBuffer = pIrpContext->pIrp->Args.ReadWrite.Buffer;
            Length = pIrpContext->pIrp->Args.ReadWrite.Length;

            break;
    }

    pPipe = pCCB->pPipe;
    pSCB = pPipe->pSCB;


    ntStatus = NpfsEnqueueBuffer(
                        &pSCB->mdlList,
                        pBuffer,
                        Length,
                        &ulBytesTransferred);
    BAIL_ON_NT_STATUS(ntStatus);

    pIrpContext->pIrp->IoStatusBlock.BytesTransferred = ulBytesTransferred;

error:

    pIrpContext->pIrp->IoStatusBlock.Status = ntStatus;

    return(ntStatus);
}
