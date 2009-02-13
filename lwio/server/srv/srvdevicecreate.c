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
 *        Likewise Posix File System Driver (SRV)
 *
 *       Create Dispatch Routine
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "includes.h"

static
NTSTATUS
SrvCommonCreate(
    PSRV_IRP_CONTEXT pIrpContext,
    PIRP             pIrp
    );

static
NTSTATUS
SrvValidateCreate(
    PSRV_IRP_CONTEXT pIrpContext,
    PUNICODE_STRING  pDeviceName
    );

NTSTATUS
SrvDeviceCreate(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP             pIrp
    )
{
    NTSTATUS ntStatus = 0;
    PSRV_IRP_CONTEXT pIrpContext = NULL;

    ntStatus = SrvAllocateIrpContext(
                        pIrp,
                        &pIrpContext);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvCommonCreate(
                    pIrpContext,
                    pIrp);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    // TODO: Should we cleanup pIrpContext?

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
SrvAllocateIrpContext(
    PIRP              pIrp,
    PSRV_IRP_CONTEXT* ppIrpContext
    )
{
    NTSTATUS ntStatus = 0;
    PSRV_IRP_CONTEXT pIrpContext = NULL;

    ntStatus = IO_ALLOCATE(&pIrpContext, SRV_IRP_CONTEXT, sizeof(*pIrpContext));
    BAIL_ON_NT_STATUS(ntStatus);

    *ppIrpContext = pIrpContext;

cleanup:

    return ntStatus;

error:

    *ppIrpContext = NULL;

    goto cleanup;
}

static
NTSTATUS
SrvCommonCreate(
    PSRV_IRP_CONTEXT pIrpContext,
    PIRP             pIrp
    )
{
    NTSTATUS ntStatus = 0;
    UNICODE_STRING DeviceName = {0};
    PSRV_CCB pCCB = NULL;

    ntStatus = SrvValidateCreate(
                    pIrpContext,
                    &DeviceName);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvCCBCreate(
                    pIrpContext,
                    &pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvCCBSet(
                    pIrpContext->pIrp->FileHandle,
                    pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    pIrpContext->pIrp->IoStatusBlock.CreateResult = FILE_OPENED;

cleanup:

    pIrpContext->pIrp->IoStatusBlock.Status = ntStatus;

    return ntStatus;

error:

    pIrpContext->pIrp->IoStatusBlock.CreateResult = FILE_DOES_NOT_EXIST;

    goto cleanup;
}

static
NTSTATUS
SrvValidateCreate(
    PSRV_IRP_CONTEXT pIrpContext,
    PUNICODE_STRING  pDeviceName
    )
{
    NTSTATUS ntStatus = 0;

    RtlUnicodeStringInit(
            pDeviceName,
            pIrpContext->pIrp->Args.Create.FileName.FileName);

    return ntStatus;
}


