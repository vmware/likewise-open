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

#include "npfs.h"

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
    //IO_FILE_HANDLE FileHandle;
    UNICODE_STRING PipeName = {0};
    PNPFS_FCB pFCB = NULL;
    PNPFS_PIPE pPipe = NULL;
    PNPFS_CCB pCCB = NULL;
    BOOLEAN bReleaseLock = FALSE;

    ntStatus = NpfsValidateCreate(
                    pIrpContext,
                    &PipeName
                    );
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NpfsFindFCB(
                    &PipeName,
                    &pFCB
                    );
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NpfsFindAvailablePipe(
                    pFCB,
                    &pPipe
                    );
    BAIL_ON_NT_STATUS(ntStatus);

    ENTER_MUTEX(&pPipe->PipeMutex);
    bReleaseLock = TRUE;

    if (pPipe->PipeServerState != PIPE_SERVER_WAITING_FOR_CONNECTION)
    {
        ntStatus = STATUS_INVALID_SERVER_STATE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = NpfsCreateCCB(
                    pIrpContext,
                    &pCCB
                    );
    BAIL_ON_NT_STATUS(ntStatus);

    pPipe->pCCB = pCCB;
    pPipe->PipeClientState =  PIPE_CLIENT_CONNECTED;
    pCCB->pPipe = pPipe;

    //
    // This is the Add Reference for the Pipe for the CCB
    //
    NpfsAddRefPipe(pPipe);

    ntStatus = NpfsCommonProcessCreateEcp(
        pIrpContext,
        pIrp,
        pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    pthread_cond_signal(&pPipe->PipeCondition);

    LEAVE_MUTEX(&pPipe->PipeMutex);
    bReleaseLock = FALSE;

    ntStatus = NpfsSetCCB(
                        pIrpContext->pIrp->FileHandle,
                        pCCB
                        );
    BAIL_ON_NT_STATUS(ntStatus);

    pIrpContext->pIrp->IoStatusBlock.CreateResult = FILE_OPENED;

cleanup:

    if (bReleaseLock)
    {
        LEAVE_MUTEX(&pPipe->PipeMutex);
    }

    if (pPipe) {
        NpfsReleasePipe(pPipe);
    }

    if (pFCB) {
        NpfsReleaseFCB(pFCB);
    }

    pIrpContext->pIrp->IoStatusBlock.Status = ntStatus;

    return(ntStatus);

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

    return(ntStatus);

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
    PSTR pszClientPrincipalName = NULL;
    ULONG ulPrincipalLength = 0;
    PULONG pulAddr = NULL;
    ULONG ulAddrLen = 0;

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
        IO_ECP_TYPE_PEER_PRINCIPAL,
        OUT_PPVOID(&pszClientPrincipalName),
        &ulPrincipalLength);

    if (ntStatus != STATUS_NOT_FOUND)
    {
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = LwRtlCStringDuplicate(
            &pPipe->pszClientPrincipalName,
            pszClientPrincipalName);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = IoRtlEcpListFind(
        pIrp->Args.Create.EcpList,
        IO_ECP_TYPE_PEER_ADDRESS,
        OUT_PPVOID(&pulAddr),
        &ulAddrLen);

    if (ntStatus != STATUS_NOT_FOUND)
    {
        BAIL_ON_NT_STATUS(ntStatus);

        if (ulAddrLen > sizeof(pPipe->ulClientAddress))
        {
            /* Only 4-byte (IPv4) address supported */
            ntStatus = STATUS_NOT_SUPPORTED;
        }

        pPipe->ulClientAddress = *pulAddr;
    }

cleanup:

    return ntStatus;

error:

    goto cleanup;
}

