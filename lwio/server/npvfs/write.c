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

    //ntStatus = NpfsCommonWrite(pIrpContext, pIrp);
    BAIL_ON_NT_STATUS(ntStatus);

error:

    return ntStatus;
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
NpfsServerWriteFile(
    PNPFS_CCB pSCB,
    PNPFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntStatus = 0;
    PNPFS_PIPE pPipe = NULL;

    ENTER_READER_RW_LOCK(&gServerLock);
    pPipe = pSCB->pPipe;
    ENTER_MUTEX(&pPipe->PipeMutex);

    switch(pPipe->PipeServerState) {

        case PIPE_SERVER_CONNECTED:
                ntStatus = NpfsServerWriteFile_Connected(
                                pSCB,
                                pIrpContext
                                );
                pthread_cond_signal(&pPipe->PipeCondition);
                BAIL_ON_NT_STATUS(ntStatus);
                break;


        case PIPE_SERVER_WAITING_FOR_CONNECTION:
                break;

    }

error:

    LEAVE_MUTEX(&pPipe->PipeMutex);
    LEAVE_READER_RW_LOCK(&gServerLock);

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

    ENTER_READER_RW_LOCK(&gServerLock);
    pPipe = pCCB->pPipe;
    ENTER_MUTEX(&pPipe->Mutex);

    switch(pPipe->PipeClientState) {

        case PIPE_CLIENT_CONNECTED:
            ntStatus = NpfsClientWriteFile_Connected(
                            pCCB,
                            pIrpContext
                            );
            pthread_cond_signal(&pPipe->PipeCondition);
            BAIL_ON_NT_STATUS(ntStatus);
            break;

    }
error:

    LEAVE_MUTEX(&pPipe->Mutex);
    LEAVE_READER_RW_LOCK(&gServerLock);

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

    pPipe = pSCB->pPipe;
    pCCB = pPipe->pCCB;

    ENTER_WRITER_RW_LOCK(&pCCB->InBoundMutex);

    ntStatus = NpfsEnqueueBuffer(
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

    pPipe = pCCB->pPipe;
    pSCB = pPipe->pSCB;

    ENTER_WRITER_RW_LOCK(&pSCB->InBoundMutex);

    ntStatus = NpfsEnqueueBuffer(
                        pSCB->pMdlList,
                        pBuffer,
                        Length,
                        &pSCB->pMdlList
                        );
    BAIL_ON_NT_STATUS(ntStatus);

error:

    LEAVE_WRITER_RW_LOCK(&pSCB->InBoundMutex);

    return(ntStatus);
}
