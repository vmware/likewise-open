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
 *        create.c
 *
 * Abstract:
 *
 *        Likewise Named Pipe File System Driver (NPFS)
 *
 *       Create Dispatch Routine
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *
 */

#include "includes.h"

static
NTSTATUS
NpfsCommonProcessCreateEcp(
    PNPFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp,
    PNPFS_CCB pCCB
    );

NTSTATUS
NpfsCreate(
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

    ntStatus = NpfsCommonCreate(pIrpContext, pIrp);
    BAIL_ON_NT_STATUS(ntStatus);

error:

    if(pIrpContext) {
        NpfsFreeIrpContext(pIrpContext);
    }
    return ntStatus;
}



NTSTATUS
NpfsAllocateIrpContext(
    PIRP pIrp,
    PNPFS_IRP_CONTEXT * ppIrpContext
    )
{
    NTSTATUS ntStatus = 0;
    PNPFS_IRP_CONTEXT pIrpContext = NULL;

    ntStatus = IO_ALLOCATE(&pIrpContext, NPFS_IRP_CONTEXT, sizeof(*pIrpContext));
    BAIL_ON_NT_STATUS(ntStatus);

    LwListInit(&pIrpContext->Link);

    pIrpContext->pIrp = pIrp;

    *ppIrpContext = pIrpContext;

    return(ntStatus);

error:

    *ppIrpContext = NULL;
    return(ntStatus);
}



NTSTATUS
NpfsFreeIrpContext(
    PNPFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntStatus = 0;

    IO_FREE(&pIrpContext);

    return(ntStatus);
}


NTSTATUS
NpfsCommonCreate(
    PNPFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;
    UNICODE_STRING PipeName = {0};
    PNPFS_FCB pFCB = NULL;
    PNPFS_PIPE pPipe = NULL;
    PNPFS_CCB pCCB = NULL;
    BOOLEAN bReleaseLock = FALSE;
    PNPFS_IRP_CONTEXT pConnectContext = NULL;

    ntStatus = NpfsValidateCreate(pIrpContext, &PipeName);
    BAIL_ON_NT_STATUS(ntStatus);

    ENTER_READER_RW_LOCK(&gServerLock);
    ntStatus = NpfsFindFCB(&PipeName, &pFCB);
    LEAVE_READER_RW_LOCK(&gServerLock);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NpfsFindAvailablePipe(pFCB, &pPipe);
    BAIL_ON_NT_STATUS(ntStatus);

    ENTER_MUTEX(&pPipe->PipeMutex);
    bReleaseLock = TRUE;

    if (pPipe->PipeServerState != PIPE_SERVER_WAITING_FOR_CONNECTION)
    {
        ntStatus = STATUS_INVALID_SERVER_STATE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = NpfsCreateCCB(pIrpContext, pPipe, &pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    pPipe->PipeClientState = PIPE_CLIENT_CONNECTED;

    pPipe->pClientAccessToken = IoSecurityGetAccessToken(pIrp->Args.Create.SecurityContext);
    RtlReferenceAccessToken(pPipe->pClientAccessToken);

    ntStatus = NpfsCommonProcessCreateEcp(pIrpContext, pIrp, pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    /* Wake up blocking pipe waiters */
    pthread_cond_signal(&pPipe->PipeCondition);

    /* If there is a pending connect IRP, grab it
       to complete once we leave the pipe mutex */
    if (pPipe->pPendingServerConnect)
    {
        pConnectContext = pPipe->pPendingServerConnect;
        pPipe->pPendingServerConnect = NULL;
        pPipe->PipeServerState = PIPE_SERVER_CONNECTED;
    }

    LEAVE_MUTEX(&pPipe->PipeMutex);
    bReleaseLock = FALSE;

    if (pConnectContext)
    {
        pConnectContext->pIrp->IoStatusBlock.Status = STATUS_SUCCESS;
        IoIrpComplete(pConnectContext->pIrp);
        IO_FREE(&pConnectContext);
    }

    ntStatus = NpfsSetCCB(pIrpContext->pIrp->FileHandle, pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    pIrpContext->pIrp->IoStatusBlock.CreateResult = FILE_OPENED;

cleanup:

    if (bReleaseLock)
    {
        LEAVE_MUTEX(&pPipe->PipeMutex);
    }

    if (pFCB)
    {
        NpfsReleaseFCB(pFCB);
    }

    if (pPipe)
    {
        NpfsReleasePipe(pPipe);
    }

    if (pCCB)
    {
        NpfsReleaseCCB(pCCB);
    }

    pIrpContext->pIrp->IoStatusBlock.Status = ntStatus;

    return ntStatus;

error:

    pIrpContext->pIrp->IoStatusBlock.CreateResult = FILE_DOES_NOT_EXIST;

    goto cleanup;
}

NTSTATUS
NpfsValidateCreate(
    PNPFS_IRP_CONTEXT pIrpContext,
    PUNICODE_STRING pPipeName
    )
{
    NTSTATUS ntStatus = 0;

    RtlUnicodeStringInit(
            pPipeName,
            pIrpContext->pIrp->Args.Create.FileName.FileName
            );
    BAIL_ON_NT_STATUS(ntStatus);

error:

    return(ntStatus);
}

static
NTSTATUS
NpfsCommonProcessCreateEcp(
    PNPFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp,
    PNPFS_CCB pCCB
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PNPFS_PIPE pPipe = pCCB->pPipe;
    PBYTE pSessionKey = NULL;
    ULONG ulSessionKeyLength = 0;
    PBYTE pAddr = NULL;
    ULONG ulEcpSize = 0;
    PFILE_NETWORK_OPEN_INFORMATION pNetworkOpenInfo = NULL;
    PFILE_PIPE_INFORMATION pPipeInfo = NULL;
    PFILE_PIPE_LOCAL_INFORMATION pPipeLocalInfo = NULL;

    ntStatus = IoRtlEcpListFind(
        pIrp->Args.Create.EcpList,
        IO_ECP_TYPE_SESSION_KEY,
        OUT_PPVOID(&pSessionKey),
        &ulSessionKeyLength);

    if (ntStatus != STATUS_NOT_FOUND)
    {
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = RTL_ALLOCATE(&pPipe->pSessionKey, BYTE, ulSessionKeyLength);
        BAIL_ON_NT_STATUS(ntStatus);

        memcpy(pPipe->pSessionKey, pSessionKey, ulSessionKeyLength);
        pPipe->ulSessionKeyLength = ulSessionKeyLength;
    }

    ntStatus = IoRtlEcpListFind(
        pIrp->Args.Create.EcpList,
        IO_ECP_TYPE_PEER_ADDRESS,
        OUT_PPVOID(&pAddr),
        &ulEcpSize);

    if (ntStatus != STATUS_NOT_FOUND)
    {
        BAIL_ON_NT_STATUS(ntStatus);

        if (ulEcpSize > sizeof(pPipe->ClientAddress))
        {
            ntStatus = STATUS_INVALID_PARAMETER;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        pPipe->usClientAddressLen = (USHORT) ulEcpSize;
        memcpy(pPipe->ClientAddress, pAddr, ulEcpSize);
    }

    ntStatus = IoRtlEcpListFind(
                    pIrp->Args.Create.EcpList,
                    SRV_ECP_TYPE_NET_OPEN_INFO,
                    OUT_PPVOID(&pNetworkOpenInfo),
                    &ulEcpSize);

    if (ntStatus == STATUS_SUCCESS)
    {
        if (ulEcpSize != sizeof(FILE_NETWORK_OPEN_INFORMATION))
        {
            ntStatus = STATUS_INVALID_PARAMETER;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        ntStatus = NpfsQueryCcbFileNetworkOpenInfo(pCCB, pNetworkOpenInfo);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = IoRtlEcpListAcknowledge(
                        pIrp->Args.Create.EcpList,
                        SRV_ECP_TYPE_NET_OPEN_INFO);
        BAIL_ON_NT_STATUS(ntStatus);
    }
    else
    {
        ntStatus = STATUS_SUCCESS;
    }

    ntStatus = IoRtlEcpListFind(
                    pIrp->Args.Create.EcpList,
                    SRV_ECP_TYPE_PIPE_INFO,
                    OUT_PPVOID(&pPipeInfo),
                    &ulEcpSize);

    if (ntStatus == STATUS_SUCCESS)
    {
        if (ulEcpSize != sizeof(FILE_PIPE_INFORMATION))
        {
            ntStatus = STATUS_INVALID_PARAMETER;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        ntStatus = NpfsQueryCcbFilePipeInfo(pCCB, pPipeInfo);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = IoRtlEcpListAcknowledge(
                        pIrp->Args.Create.EcpList,
                        SRV_ECP_TYPE_PIPE_INFO);
        BAIL_ON_NT_STATUS(ntStatus);
    }
    else
    {
        ntStatus = STATUS_SUCCESS;
    }

    ntStatus = IoRtlEcpListFind(
                    pIrp->Args.Create.EcpList,
                    SRV_ECP_TYPE_PIPE_LOCAL_INFO,
                    OUT_PPVOID(&pPipeLocalInfo),
                    &ulEcpSize);

    if (ntStatus == STATUS_SUCCESS)
    {
        if (ulEcpSize != sizeof(FILE_PIPE_LOCAL_INFORMATION))
        {
            ntStatus = STATUS_INVALID_PARAMETER;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        ntStatus = NpfsQueryCcbFilePipeLocalInfo(pCCB, pPipeLocalInfo);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = IoRtlEcpListAcknowledge(
                        pIrp->Args.Create.EcpList,
                        SRV_ECP_TYPE_PIPE_LOCAL_INFO);
        BAIL_ON_NT_STATUS(ntStatus);
    }
    else
    {
        ntStatus = STATUS_SUCCESS;
    }

cleanup:

    return ntStatus;

error:

    goto cleanup;
}

