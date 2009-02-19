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
 *        pipe.c
 *
 * Abstract:
 *
 *        Likewise Named Pipe File System Driver (NPFS)
 *
 *        Pipe Management Routines
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "npfs.h"

NTSTATUS
NpfsFindAvailablePipe(
    PNPFS_FCB pFCB,
    PNPFS_PIPE * ppPipe
    )
{
    NTSTATUS ntStatus = 0;
    PNPFS_PIPE pPipe = NULL;

    ENTER_WRITER_RW_LOCK(&pFCB->PipeListRWLock);

    pPipe = pFCB->pPipes;


    while(pPipe) {

        ENTER_MUTEX(&pPipe->PipeMutex);

        if (pPipe->PipeServerState == PIPE_SERVER_WAITING_FOR_CONNECTION){
            *ppPipe = pPipe;

            LEAVE_MUTEX(&pPipe->PipeMutex);
            LEAVE_WRITER_RW_LOCK(&pFCB->PipeListRWLock);

            return(ntStatus);
        }
        
        LEAVE_MUTEX(&pPipe->PipeMutex);
        pPipe = pPipe->pNext;
    }

    LEAVE_WRITER_RW_LOCK(&pFCB->PipeListRWLock);

    *ppPipe = NULL;
    return(STATUS_PIPE_NOT_AVAILABLE);
}

NTSTATUS
NpfsCreatePipe(
    PNPFS_FCB pFCB,
    PNPFS_PIPE * ppPipe
    )
{
    NTSTATUS ntStatus = 0;
    PNPFS_PIPE pPipe = NULL;

    ENTER_WRITER_RW_LOCK(&pFCB->PipeListRWLock);

    ntStatus = NpfsAllocateMemory(
                    sizeof(NPFS_PIPE),
                    &pPipe
                    );
    BAIL_ON_NT_STATUS(ntStatus);

    pthread_cond_init(&pPipe->PipeCondition,NULL);
    pthread_mutex_init(&pPipe->PipeMutex, NULL);

    pPipe->PipeServerState = PIPE_SERVER_INIT_STATE;
    pPipe->PipeClientState = PIPE_CLIENT_INIT_STATE;
    pPipe->pNext = pFCB->pPipes;
    pFCB->pPipes = pPipe;
    pFCB->CurrentNumberOfInstances++;

    *ppPipe = pPipe;

cleanup:

    LEAVE_WRITER_RW_LOCK(&pFCB->PipeListRWLock);

    return(ntStatus);

error:

    *ppPipe = NULL;
    goto cleanup;

}


NTSTATUS
NpfsFreePipeContext(
    PNPFS_PIPE pPipe
    )
{
    NTSTATUS ntStatus = 0;

    return (ntStatus);
}


VOID
NpfsReleasePipe(
    PNPFS_PIPE pPipe
    )
{
    NpfsInterlockedDecrement(&pPipe->cRef);
    if (!NpfsInterlockedCounter(&pPipe->cRef)) {

        NpfsFreePipe(pPipe);
    }
    return;
}

VOID
NpfsAddRefPipe(
    PNPFS_PIPE pPipe
    )
{
    NpfsInterlockedIncrement(&pPipe->cRef);
    return;
}


NTSTATUS
NpfsFreePipe(
    PNPFS_PIPE pPipe
    )
{
    NTSTATUS ntStatus = 0;
    PNPFS_FCB pFCB = NULL;

    pFCB = pPipe->pFCB;

    NpfsRemovePipeFromFCB(
            pFCB,
            pPipe
            );

    NpfsReleaseFCB(pFCB);

    if (pPipe)
    {
        RTL_FREE(&pPipe->pSessionKey);
        RTL_FREE(&pPipe->pszClientPrincipalName);
        NpfsFreeMemory(pPipe);
    }

    return(ntStatus);
}

VOID
NpfsRemovePipeFromFCB(
    PNPFS_FCB pFCB,
    PNPFS_PIPE pPipe
    )
{
    ENTER_WRITER_RW_LOCK(&pFCB->PipeListRWLock);

    //
    // Now remove the Pipe entry
    //

    LEAVE_WRITER_RW_LOCK(&pFCB->PipeListRWLock);
}


