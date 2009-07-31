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
 *        connectnp.c
 *
 * Abstract:
 *
 *        Likewise Named Pipe File System Driver (NPFS)
 *
 *       ConnectNamedPipe Dispatch Routine
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *
 */

#include "npfs.h"

static
VOID
NpfsCancelConnectNamedPipe(
    IN PIRP pIrp,
    IN PVOID pCallbackContext
    );

/*****/

NTSTATUS
NpfsAsyncConnectNamedPipe(
    PNPFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;
    PNPFS_PIPE pPipe = NULL;
    PNPFS_CCB pSCB = NULL;

    ntStatus = NpfsGetCCB(pIrpContext->pIrp->FileHandle, &pSCB);
    BAIL_ON_NT_STATUS(ntStatus);

    pPipe = pSCB->pPipe;

    ENTER_MUTEX(&pPipe->PipeMutex);

    if (pPipe->PipeServerState != PIPE_SERVER_INIT_STATE)
    {
        LEAVE_MUTEX(&pPipe->PipeMutex);
        ntStatus = STATUS_INVALID_SERVER_STATE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pPipe->PipeServerState = PIPE_SERVER_WAITING_FOR_CONNECTION;

    if (pPipe->PipeClientState != PIPE_CLIENT_CONNECTED)
    {
        pPipe->pPendingServerConnect = pIrpContext;
        IoIrpMarkPending(pIrp, NpfsCancelConnectNamedPipe, pIrpContext);

        ntStatus = STATUS_PENDING;
    }
    else
    {
        pPipe->PipeServerState = PIPE_SERVER_CONNECTED;
        pIrpContext->pIrp->IoStatusBlock.Status = ntStatus;
    }

    LEAVE_MUTEX(&pPipe->PipeMutex);

cleanup:

    pIrpContext->pIrp->IoStatusBlock.Status = ntStatus;

    return ntStatus;

error:

    goto cleanup;
}

static
VOID
NpfsCancelConnectNamedPipe(
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
        LWIO_LOG_DEBUG("ConnectNamedPipe() cancelled");

        pPipe = pSCB->pPipe;

        ENTER_MUTEX(&pPipe->PipeMutex);

        /* Check if the operation was already completed */
        if (pPipe->pPendingServerConnect == pIrpContext)
        {
            bDoComplete = TRUE;
            pPipe->PipeServerState = PIPE_SERVER_INIT_STATE;
            pPipe->pPendingServerConnect = NULL;
        }

        LEAVE_MUTEX(&pPipe->PipeMutex);

        if (bDoComplete)
        {
            pIrpContext->pIrp->IoStatusBlock.Status = STATUS_CANCELLED;
            IoIrpComplete(pIrpContext->pIrp);
            IO_FREE(&pIrpContext);
        }
    }
}
