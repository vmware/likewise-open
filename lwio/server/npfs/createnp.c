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
 *        Likewise Posix File System Driver (NPFS)
 *
 *       CreateNamedPipe Dispatch Routine
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *
 */

#include "includes.h"

NTSTATUS
NpfsCreateNamedPipe(
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

    ntStatus = NpfsCommonCreateNamedPipe(
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
NpfsCommonCreateNamedPipe(
    PNPFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;
    UNICODE_STRING  PathName = {0};
    PNPFS_FCB pFCB = NULL;
    PNPFS_PIPE pPipe = NULL;
    PNPFS_CCB pSCB = NULL;

    ENTER_WRITER_RW_LOCK(&gServerLock);

    ntStatus = NpfsValidateCreateNamedPipe(pIrpContext, &PathName);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NpfsFindFCB(&PathName, &pFCB);
    if (ntStatus == STATUS_OBJECT_NAME_NOT_FOUND)
    {
        ntStatus = NpfsCreateFCB(&PathName, &pFCB);
    }
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NpfsCreatePipe(pFCB, &pPipe);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NpfsCreateSCB(pIrpContext, pPipe, &pSCB);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NpfsSetCCB(pIrpContext->pIrp->FileHandle, pSCB);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    LEAVE_WRITER_RW_LOCK(&gServerLock);

    pIrpContext->pIrp->IoStatusBlock.Status = ntStatus;

    if (pFCB)
    {
        NpfsReleaseFCB(pFCB);
    }

    if (pPipe)
    {
        NpfsReleasePipe(pPipe);
    }

    if (pSCB)
    {
        NpfsReleaseCCB(pSCB);
    }

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
NpfsValidateCreateNamedPipe(
    PNPFS_IRP_CONTEXT pIrpContext,
    PUNICODE_STRING  pPath
    )
{
    NTSTATUS ntStatus = 0;
    PIO_ECP_NAMED_PIPE pipeParams = NULL;
    ULONG ecpSize = 0;

    if (!pIrpContext->pIrp->Args.Create.EcpList)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = IoRtlEcpListFind(
                    pIrpContext->pIrp->Args.Create.EcpList,
                    IO_ECP_TYPE_NAMED_PIPE,
                    (PVOID*)&pipeParams,
                    &ecpSize);
    if (STATUS_NOT_FOUND == ntStatus)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    if (ecpSize != sizeof(*pipeParams))
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    RtlUnicodeStringInit(pPath, pIrpContext->pIrp->Args.Create.FileName.FileName);

error:

    return ntStatus;
}
