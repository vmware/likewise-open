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
 *        Likewise Posix File System Driver (NPFS)
 *
 *       Read Dispatch Routine
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "npfs.h"

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

    //ntStatus = NpfsCommonRead(pIrpContext, pIrp);
    BAIL_ON_NT_STATUS(ntStatus);

error:

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

    ntStatus = NpfsReadFile(
                    pCCB,
                    pIrpContext
                    );
    BAIL_ON_NT_STATUS(ntStatus);


error:

    return(ntStatus);
}



NTSTATUS
NpfsReadFile(
    PNPFS_CCB pCCB,
    PNPFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntStatus = 0;

    switch(pCCB->CcbType) {

        case NPFS_CCB_SERVER:
            ntStatus = NpfsServerReadFile(
                            pCCB,
                            pIrpContext
                            );
            BAIL_ON_NT_STATUS(ntStatus);
            break;

        case NPFS_CCB_CLIENT:
            ntStatus = NpfsClientReadFile(
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
NpfsServerReadFile(
    PNPFS_CCB pSCB,
    PNPFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntStatus = 0;
    PNPFS_PIPE pPipe = NULL;
    PNPFS_CCB pCCB = NULL;

    ENTER_READER_RW_LOCK(&gServerLock);
    pPipe = pCCB->pPipe;
    ENTER_READER_RW_LOCK(&pPipe->Mutex);

    switch(pPipe->PipeServerState) {

        case PIPE_SERVER_CONNECTED:
                ntStatus = NpfsServerReadFile_Connected(
                                pSCB,
                                pIrpContext
                                );
                BAIL_ON_NT_STATUS(ntStatus);
                break;


        case PIPE_SERVER_DISCONNECTED:
                break;

        case PIPE_SERVER_CREATED:
        case PIPE_SERVER_WAITING_FOR_CONNECTION:
                break;

    }

error:

    LEAVE_READER_RW_LOCK(&pPipe->Mutex);
    LEAVE_READER_RW_LOCK(&gServerLock);

    return(ntStatus);
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

    ENTER_WRITER_RW_LOCK(&pSCB->InBoundMutex);

    ntStatus = NpfsDequeueBuffer(
                        pSCB->pMdlList,
                        pBuffer,
                        Length,
                        &pSCB->pMdlList
                        );
    BAIL_ON_NT_STATUS(ntStatus);

error:

    return(ntStatus);
}

NTSTATUS
NpfsClientReadFile(
    PNPFS_CCB pCCB,
    PNPFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntStatus = 0;
    PNPFS_PIPE pPipe = NULL;

    ENTER_READER_RW_LOCK(&gServerLock);
    pPipe = pCCB->pPipe;
    ENTER_READER_RW_LOCK(&pPipe->Mutex);

    switch(pPipe->PipeClientState) {

        case PIPE_CLIENT_CONNECTED:
            ntStatus = NpfsClientReadFile_Connected(
                            pCCB,
                            pIrpContext
                            );
            BAIL_ON_NT_STATUS(ntStatus);

        case PIPE_SERVER_DISCONNECTED:
            //ntStatus = STATUS_PIPE_BROKEN;
            break;
    }

error:

    LEAVE_READER_RW_LOCK(&pPipe->Mutex);
    LEAVE_READER_RW_LOCK(&gServerLock);

    return(ntStatus);
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

    ENTER_WRITER_RW_LOCK(&pCCB->InBoundMutex);

    ntStatus = NpfsDequeueBuffer(
                        pCCB->pMdlList,
                        pBuffer,
                        Length,
                        &pCCB->pMdlList
                        );
    BAIL_ON_NT_STATUS(ntStatus);

error:

    LEAVE_WRITER_RW_LOCK(&pCCB->InBoundMutex);

    return(ntStatus);
}
