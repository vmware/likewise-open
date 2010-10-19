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
 *        read.c
 *
 * Abstract:
 *
 *        Likewise Named Pipe File System Driver (NPFS)
 *
 *       Read Dispatch Routine
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "includes.h"

NTSTATUS
NpfsRead(
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

    ntStatus = NpfsCommonRead(pIrpContext, pIrp);
    BAIL_ON_NT_STATUS(ntStatus);

error:

    if (pIrpContext && ntStatus != STATUS_PENDING)
    {
        NpfsFreeIrpContext(pIrpContext);
    }

    return ntStatus;
}


NTSTATUS
NpfsCommonRead(
    PNPFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;
    PNPFS_CCB pCCB = NULL;

    ntStatus = NpfsGetCCB(pIrpContext->pIrp->FileHandle, &pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NpfsReadFile(pCCB, pIrpContext);
    BAIL_ON_NT_STATUS(ntStatus);


error:

    return ntStatus;
}



NTSTATUS
NpfsReadFile(
    PNPFS_CCB pCCB,
    PNPFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntStatus = 0;

    switch(pCCB->CcbType)
    {
    case NPFS_CCB_SERVER:
        ntStatus = NpfsServerReadFile(pCCB, pIrpContext);
        BAIL_ON_NT_STATUS(ntStatus);
        break;

    case NPFS_CCB_CLIENT:
        ntStatus = NpfsClientReadFile(pCCB, pIrpContext);
        BAIL_ON_NT_STATUS(ntStatus);
        break;
    }

error:

    return ntStatus;
}

static
VOID
NpfsCancelReadFile(
    IN PIRP pIrp,
    IN PVOID pCallbackContext
    )
{
    NTSTATUS ntStatus = 0;
    PNPFS_PIPE pPipe = NULL;
    PNPFS_CCB pSCB = NULL;
    PNPFS_IRP_CONTEXT pIrpContext = pCallbackContext;
    BOOLEAN bDoComplete = FALSE;

    ntStatus = NpfsGetCCB(pIrpContext->pIrp->FileHandle, &pSCB);
    if (ntStatus == STATUS_SUCCESS)
    {
        LWIO_LOG_DEBUG("ReadFile() cancelled");

        pPipe = pSCB->pPipe;

        ENTER_MUTEX(&pPipe->PipeMutex);

        /* Check if the operation needs to be completed */
        if (!LwListIsEmpty(&pIrpContext->Link))
        {
            LwListRemove(&pIrpContext->Link);
            bDoComplete = TRUE;
        }

        LEAVE_MUTEX(&pPipe->PipeMutex);

        if (bDoComplete)
        {
            pIrpContext->pIrp->IoStatusBlock.Status = STATUS_CANCELLED;
            IoIrpComplete(pIrpContext->pIrp);
            NpfsFreeIrpContext(pIrpContext);
        }
    }
}

VOID
NpfsServerCompleteReadFile(
    PNPFS_CCB pSCB,
    PNPFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntStatus = 0;
    PNPFS_PIPE pPipe = NULL;

    pPipe = pSCB->pPipe;

    if ((pPipe->PipeClientState == PIPE_CLIENT_CLOSED)
        && (NpfsMdlListIsEmpty(&pSCB->mdlList)))
    {
        ntStatus = STATUS_END_OF_FILE;
        BAIL_ON_NT_STATUS(ntStatus);
    }
    else if ((pPipe->PipeClientState == PIPE_CLIENT_CLOSED)
             && (!NpfsMdlListIsEmpty(&pSCB->mdlList)))
    {
        ntStatus = NpfsServerReadFile_Connected(pSCB, pIrpContext);
        BAIL_ON_NT_STATUS(ntStatus);
    }
    else if ((pPipe->PipeClientState == PIPE_CLIENT_CONNECTED)
             && (!NpfsMdlListIsEmpty(&pSCB->mdlList)))
    {
        ntStatus = NpfsServerReadFile_Connected(pSCB, pIrpContext);
        BAIL_ON_NT_STATUS(ntStatus);
    }

error:

    pIrpContext->pIrp->IoStatusBlock.Status = ntStatus;

    IoIrpComplete(pIrpContext->pIrp);

    NpfsFreeIrpContext(pIrpContext);
}

NTSTATUS
NpfsServerReadFile(
    PNPFS_CCB pSCB,
    PNPFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntStatus = 0;
    PNPFS_PIPE pPipe = NULL;

    pPipe = pSCB->pPipe;
    ENTER_MUTEX(&pPipe->PipeMutex);

    switch(pPipe->PipeServerState)
    {
    case PIPE_SERVER_CONNECTED:
        if (pPipe->PipeClientState == PIPE_CLIENT_CLOSED)
        {
            if (NpfsMdlListIsEmpty(&pSCB->mdlList))
            {
                ntStatus = STATUS_END_OF_FILE;
                BAIL_ON_NT_STATUS(ntStatus);
            }
            else
            {
                ntStatus = NpfsServerReadFile_Connected(pSCB, pIrpContext);
                BAIL_ON_NT_STATUS(ntStatus);
            }
        }
        else if (pPipe->PipeClientState == PIPE_CLIENT_CONNECTED)
        {
            if (NpfsMdlListIsEmpty(&pSCB->mdlList))
            {
                LwListInsertBefore(&pSCB->ReadIrpList, &pIrpContext->Link);

                IoIrpMarkPending(
                    pIrpContext->pIrp,
                    NpfsCancelReadFile,
                    pIrpContext);

                ntStatus = STATUS_PENDING;
                BAIL_ON_NT_STATUS(ntStatus);
            }
            else
            {
                ntStatus = NpfsServerReadFile_Connected(pSCB, pIrpContext);
                BAIL_ON_NT_STATUS(ntStatus);
            }
        }
        else
        {
            ntStatus = STATUS_INTERNAL_ERROR;
            BAIL_ON_NT_STATUS(ntStatus);
        }
        break;

    case PIPE_SERVER_INIT_STATE:
    case PIPE_SERVER_WAITING_FOR_CONNECTION:
    case PIPE_SERVER_CREATED:
    case PIPE_SERVER_CLOSED:
    case PIPE_SERVER_DISCONNECTED:
        ntStatus = STATUS_PIPE_DISCONNECTED;
        BAIL_ON_NT_STATUS(ntStatus);
        break;
    }

error:

    pIrpContext->pIrp->IoStatusBlock.Status = ntStatus;
    LEAVE_MUTEX(&pPipe->PipeMutex);

    return ntStatus;
}

NTSTATUS
NpfsServerReadFile_Connected(
    PNPFS_CCB pSCB,
    PNPFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntStatus = 0;
    PVOID pBuffer = NULL;
    ULONG Length = 0;
    ULONG ulBytesTransferred = 0;

    switch (pIrpContext->pIrp->Type)
    {
        case IRP_TYPE_FS_CONTROL:

            pBuffer = pIrpContext->pIrp->Args.IoFsControl.OutputBuffer;
            Length = pIrpContext->pIrp->Args.IoFsControl.OutputBufferLength;

            break;

        default:

            pBuffer = pIrpContext->pIrp->Args.ReadWrite.Buffer;
            Length = pIrpContext->pIrp->Args.ReadWrite.Length;

            break;
    }

    ntStatus = NpfsDequeueBuffer(
                        &pSCB->mdlList,
                        pBuffer,
                        Length,
                        &ulBytesTransferred
                        );
    BAIL_ON_NT_STATUS(ntStatus);
    pIrpContext->pIrp->IoStatusBlock.BytesTransferred = ulBytesTransferred;

error:

    pIrpContext->pIrp->IoStatusBlock.Status = ntStatus;

    return ntStatus;
}

VOID
NpfsClientCompleteReadFile(
    PNPFS_CCB pCCB,
    PNPFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntStatus = 0;
    PNPFS_PIPE pPipe = NULL;

    pPipe = pCCB->pPipe;

    if ((pPipe->PipeServerState == PIPE_SERVER_CLOSED)
        && (NpfsMdlListIsEmpty(&pCCB->mdlList)))
    {
        ntStatus = STATUS_END_OF_FILE;
        BAIL_ON_NT_STATUS(ntStatus);
    }
    else if ((pPipe->PipeServerState == PIPE_SERVER_CLOSED)
             && (!NpfsMdlListIsEmpty(&pCCB->mdlList)))
    {
        ntStatus = NpfsClientReadFile_Connected(pCCB, pIrpContext);
        BAIL_ON_NT_STATUS(ntStatus);
    }
    else if ((pPipe->PipeServerState == PIPE_SERVER_CONNECTED)
             && (!NpfsMdlListIsEmpty(&pCCB->mdlList)))
    {
        ntStatus = NpfsClientReadFile_Connected(pCCB, pIrpContext);
        BAIL_ON_NT_STATUS(ntStatus);
    }

error:

    pIrpContext->pIrp->IoStatusBlock.Status = ntStatus;

    IoIrpComplete(pIrpContext->pIrp);

    NpfsFreeIrpContext(pIrpContext);
}

NTSTATUS
NpfsClientReadFile(
    PNPFS_CCB pCCB,
    PNPFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntStatus = 0;
    PNPFS_PIPE pPipe = NULL;

    pPipe = pCCB->pPipe;
    ENTER_MUTEX(&pPipe->PipeMutex);

    switch(pPipe->PipeClientState)
    {
    case PIPE_CLIENT_CONNECTED:
        if (pPipe->PipeServerState == PIPE_SERVER_CLOSED)
        {
            if (NpfsMdlListIsEmpty(&pCCB->mdlList))
            {
                ntStatus = STATUS_END_OF_FILE;
                BAIL_ON_NT_STATUS(ntStatus);
            }
            else
            {
                ntStatus = NpfsClientReadFile_Connected(pCCB, pIrpContext);
                BAIL_ON_NT_STATUS(ntStatus);
            }
        }
        else if (pPipe->PipeServerState == PIPE_SERVER_CONNECTED)
        {
            if (NpfsMdlListIsEmpty(&pCCB->mdlList))
            {
                LwListInsertBefore(&pCCB->ReadIrpList, &pIrpContext->Link);

                IoIrpMarkPending(
                    pIrpContext->pIrp,
                    NpfsCancelReadFile,
                    pIrpContext);

                ntStatus = STATUS_PENDING;
                BAIL_ON_NT_STATUS(ntStatus);
            }
            else
            {
                ntStatus = NpfsClientReadFile_Connected(pCCB, pIrpContext);
                BAIL_ON_NT_STATUS(ntStatus);
            }
        }
        else
        {
            ntStatus = STATUS_INTERNAL_ERROR;
            BAIL_ON_NT_STATUS(ntStatus);
        }
        break;
    case PIPE_CLIENT_INIT_STATE:
    case PIPE_CLIENT_CLOSED:
        ntStatus = STATUS_PIPE_DISCONNECTED;
        BAIL_ON_NT_STATUS(ntStatus);
        break;
    }

error:

    pIrpContext->pIrp->IoStatusBlock.Status = ntStatus;

    LEAVE_MUTEX(&pPipe->PipeMutex);

    return ntStatus;
}


NTSTATUS
NpfsClientReadFile_Connected(
    PNPFS_CCB pCCB,
    PNPFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntStatus = 0;
    PVOID pBuffer = NULL;
    ULONG Length = 0;
    ULONG ulBytesTransferred = 0;

    switch (pIrpContext->pIrp->Type)
    {
        case IRP_TYPE_FS_CONTROL:

            pBuffer = pIrpContext->pIrp->Args.IoFsControl.OutputBuffer;
            Length = pIrpContext->pIrp->Args.IoFsControl.OutputBufferLength;

            break;

        default:

            pBuffer = pIrpContext->pIrp->Args.ReadWrite.Buffer;
            Length = pIrpContext->pIrp->Args.ReadWrite.Length;

            break;
    }

    ntStatus = NpfsDequeueBuffer(
                        &pCCB->mdlList,
                        pBuffer,
                        Length,
                        &ulBytesTransferred);
    BAIL_ON_NT_STATUS(ntStatus);
    pIrpContext->pIrp->IoStatusBlock.BytesTransferred = ulBytesTransferred;

error:

    pIrpContext->pIrp->IoStatusBlock.Status = ntStatus;

    return ntStatus;
}
