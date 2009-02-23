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

#include "npfs.h"

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

    ntStatus = NpfsGetCCB(
                    pIrpContext->pIrp->FileHandle,
                    &pCCB
                    );
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NpfsCloseHandle(
                        pCCB
                        );
    BAIL_ON_NT_STATUS(ntStatus);

error:
    if (pCCB) {
        NpfsReleaseCCB(pCCB);
    }

    return(ntStatus);
}



NTSTATUS
NpfsCloseHandle(
    PNPFS_CCB pCCB
    )
{

    NpfsAddRefCCB(pCCB);

    NTSTATUS ntStatus = 0;

    switch (pCCB->CcbType) {
        
        case SERVER_CCB:
            ntStatus = NpfsServerCloseHandle(
                    pCCB
                    );
            BAIL_ON_NT_STATUS(ntStatus);
            break;

        case CLIENT_CCB:
            ntStatus = NpfsClientCloseHandle(
                            pCCB
                            );
            BAIL_ON_NT_STATUS(ntStatus);
            break;
    }

error:

    NpfsReleaseCCB(pCCB);

    return(ntStatus);
}

NTSTATUS
NpfsServerCloseHandle(
    PNPFS_CCB pSCB
    )
{
    NTSTATUS ntStatus = 0;
    PNPFS_PIPE pPipe = NULL;

    pPipe = pSCB->pPipe;
    ENTER_MUTEX(&pPipe->PipeMutex);

    pPipe->PipeServerState = PIPE_SERVER_CLOSED;
    pthread_cond_signal(&pPipe->PipeCondition);

    NpfsReleaseCCB(pSCB);

    if (pPipe->PipeClientState == PIPE_CLIENT_CLOSED) {
        
        ntStatus = NpfsFreePipeContext(
                        pPipe
                        );
        BAIL_ON_NT_STATUS(ntStatus);
    }

error:

    LEAVE_MUTEX(&pPipe->PipeMutex);
    return(ntStatus);
}



NTSTATUS
NpfsClientCloseHandle(
    PNPFS_CCB pCCB
    )
{
    NTSTATUS ntStatus = 0;
    PNPFS_PIPE pPipe = NULL;

    pPipe = pCCB->pPipe;
    ENTER_MUTEX(&pPipe->PipeMutex);

    pPipe->PipeClientState = PIPE_CLIENT_CLOSED;
    pthread_cond_signal(&pPipe->PipeCondition);

    NpfsReleaseCCB( pCCB);

    if (pPipe->PipeServerState == PIPE_SERVER_CLOSED) {
        
        ntStatus = NpfsFreePipeContext(
                        pPipe
                        );
        BAIL_ON_NT_STATUS(ntStatus);
    }

error:

    LEAVE_MUTEX(&pPipe->PipeMutex);
    return(ntStatus);
}
