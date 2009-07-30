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
 *        fsctl.c
 *
 * Abstract:
 *
 *        Likewise Named Pipe File System Driver (NPFS)
 *
 *        Device I/O Function
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "npfs.h"

static
NTSTATUS
NpfsCommonGetSessionKey(
    PNPFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

static
NTSTATUS
NpfsCommonGetPeerPrincipal(
    PNPFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

static
NTSTATUS
NpfsCommonGetPeerAddress(
    PNPFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

static
NTSTATUS
NpfsCommonTransceive(
    PNPFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

NTSTATUS
NpfsFsCtl(
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

    ntStatus = NpfsCommonFsCtl(pIrpContext, pIrp);
    BAIL_ON_NT_STATUS(ntStatus);

error:

    if (pIrpContext && ntStatus != STATUS_PENDING)
    {
        NpfsFreeIrpContext(pIrpContext);
    }

    return ntStatus;
}


NTSTATUS
NpfsCommonFsCtl(
    PNPFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;

    switch (pIrpContext->pIrp->Args.IoFsControl.ControlCode)
    {
    case IO_NPFS_FSCTL_CONNECT_NAMED_PIPE:
        ntStatus = NpfsAsyncConnectNamedPipe(pIrpContext, pIrp);
        break;
    case IO_FSCTL_SMB_GET_SESSION_KEY:
        ntStatus = NpfsCommonGetSessionKey(pIrpContext, pIrp);
        break;
    case IO_FSCTL_SMB_GET_PEER_PRINCIPAL:
        ntStatus = NpfsCommonGetPeerPrincipal(pIrpContext, pIrp);
        break;
    case IO_FSCTL_SMB_GET_PEER_ADDRESS:
        ntStatus = NpfsCommonGetPeerAddress(pIrpContext, pIrp);
        break;
    case IO_FSCTL_PIPE_TRANSCEIVE:
        ntStatus = NpfsCommonTransceive(pIrpContext, pIrp);
        break;
    default:
        ntStatus = STATUS_NOT_SUPPORTED;
        break;
    }

    return ntStatus;
}

static
NTSTATUS
NpfsCommonGetSessionKey(
    PNPFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PVOID pOutBuffer = pIrp->Args.IoFsControl.OutputBuffer;
    ULONG OutLength = pIrp->Args.IoFsControl.OutputBufferLength;
    PNPFS_CCB pCCB = NULL;
    PNPFS_PIPE pPipe = NULL;
    BOOL bReleasePipeLock = FALSE;
    ULONG ulSessionKeyLength = 0;

    ntStatus = NpfsGetCCB(pIrpContext->pIrp->FileHandle, &pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    pPipe = pCCB->pPipe;

    ENTER_MUTEX(&pPipe->PipeMutex);
    bReleasePipeLock = TRUE;

    /* Ensure we actually have a session key */
    if (pPipe->pSessionKey != NULL)
    {
        ulSessionKeyLength = pPipe->ulSessionKeyLength;

        /* Ensure there is enough space in the output buffer for the
           session key structure */
        if (ulSessionKeyLength > OutLength)
        {
            ntStatus = STATUS_BUFFER_TOO_SMALL;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        memcpy(pOutBuffer, pPipe->pSessionKey, ulSessionKeyLength);

        pIrp->IoStatusBlock.BytesTransferred = ulSessionKeyLength;
    }
    else
    {
        /* Return 0 byte result to indicate no session key */
        pIrp->IoStatusBlock.BytesTransferred = 0;
    }

cleanup:

    if (bReleasePipeLock)
    {
        LEAVE_MUTEX(&pPipe->PipeMutex);
    }

    pIrp->IoStatusBlock.Status = ntStatus;

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
NpfsCommonGetPeerPrincipal(
    PNPFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PVOID pOutBuffer = pIrp->Args.IoFsControl.OutputBuffer;
    ULONG OutLength = pIrp->Args.IoFsControl.OutputBufferLength;
    PNPFS_CCB pCCB = NULL;
    PNPFS_PIPE pPipe = NULL;
    BOOL bReleasePipeLock = FALSE;
    ULONG ulPrincipalLength = 0;

    ntStatus = NpfsGetCCB(pIrpContext->pIrp->FileHandle, &pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    pPipe = pCCB->pPipe;

    ENTER_MUTEX(&pPipe->PipeMutex);
    bReleasePipeLock = TRUE;

    /* Ensure we actually have a client principal */
    if (pPipe->pszClientPrincipalName != NULL)
    {
        ulPrincipalLength = strlen(pPipe->pszClientPrincipalName) + 1;

        /* Ensure there is enough space in the output buffer */
        if (ulPrincipalLength > OutLength)
        {
            ntStatus = STATUS_BUFFER_TOO_SMALL;
            BAIL_ON_NT_STATUS(ntStatus);
        }

           memcpy(pOutBuffer, pPipe->pszClientPrincipalName, ulPrincipalLength);

        pIrp->IoStatusBlock.BytesTransferred = ulPrincipalLength;
    }
    else
    {
        /* Return 0 byte result to indicate no principal name */
        pIrp->IoStatusBlock.BytesTransferred = 0;
    }

cleanup:

    if (bReleasePipeLock)
    {
        LEAVE_MUTEX(&pPipe->PipeMutex);
    }

    pIrp->IoStatusBlock.Status = ntStatus;

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
NpfsCommonGetPeerAddress(
    PNPFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PVOID pOutBuffer = pIrp->Args.IoFsControl.OutputBuffer;
    ULONG OutLength = pIrp->Args.IoFsControl.OutputBufferLength;
    PNPFS_CCB pCCB = NULL;
    PNPFS_PIPE pPipe = NULL;
    BOOL bReleasePipeLock = FALSE;
    ULONG ulAddrLength = 0;

    ntStatus = NpfsGetCCB(pIrpContext->pIrp->FileHandle, &pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    pPipe = pCCB->pPipe;

    ENTER_MUTEX(&pPipe->PipeMutex);
    bReleasePipeLock = TRUE;

    /* Ensure we actually have an address */
    if (pPipe->ulClientAddress != 0)
    {
        ulAddrLength = sizeof(pPipe->ulClientAddress);

        /* Ensure there is enough space in the output buffer */
        if (ulAddrLength > OutLength)
        {
            ntStatus = STATUS_BUFFER_TOO_SMALL;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        memcpy(pOutBuffer, &pPipe->ulClientAddress, ulAddrLength);

        pIrp->IoStatusBlock.BytesTransferred = ulAddrLength;
    }
    else
    {
        /* Return 0 byte result to indicate no address */
        pIrp->IoStatusBlock.BytesTransferred = 0;
    }

cleanup:

    if (bReleasePipeLock)
    {
        LEAVE_MUTEX(&pPipe->PipeMutex);
    }

    pIrp->IoStatusBlock.Status = ntStatus;

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
NpfsCommonTransceive(
    PNPFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    ntStatus = NpfsCommonWrite(
                    pIrpContext,
                    pIrp);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NpfsCommonRead(
                    pIrpContext,
                    pIrp);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    pIrp->IoStatusBlock.Status = ntStatus;

    return ntStatus;

error:

    goto cleanup;
}
