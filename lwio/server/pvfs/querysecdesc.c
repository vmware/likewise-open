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
 *        querysecurity.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        Query and Set Securiry Handlers
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "pvfs.h"

/* Forward declarations */

static NTSTATUS
PvfsQuerySecurityFile(
    PPVFS_IRP_CONTEXT pIrpContext
    );

static NTSTATUS
PvfsSetSecurityFile(
    PPVFS_IRP_CONTEXT pIrpContext
    );


/* File Globals */



/* Code */


/****************************************************************
 ***************************************************************/

NTSTATUS
PvfsQuerySetSecurityFile(
    PVFS_INFO_TYPE RequestType,
    IO_DEVICE_HANDLE IoDeviceHandle,
    PPVFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;

    switch(RequestType)
    {
    case PVFS_SET:
        ntError = PvfsSetSecurityFile(pIrpContext);
        break;

    case PVFS_QUERY:
        ntError = PvfsQuerySecurityFile(pIrpContext);
        break;

    default:
        ntError = STATUS_INVALID_PARAMETER;
        break;
    }
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    return ntError;

error:
    goto cleanup;
}


/****************************************************************
 ***************************************************************/

static NTSTATUS
PvfsQuerySecurityFile(
    PPVFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntError = STATUS_NOT_SUPPORTED;
    PIRP pIrp = pIrpContext->pIrp;
    PPVFS_CCB pCcb = NULL;
    PSECURITY_DESCRIPTOR_RELATIVE prReturnSecDesc = NULL;
    PSECURITY_DESCRIPTOR_ABSOLUTE paDefaultSecDesc = NULL;
    ULONG SecDescLength = 0;
    SECURITY_INFORMATION SecInfo = 0;
    IRP_ARGS_QUERY_SET_SECURITY Args = pIrpContext->pIrp->Args.QuerySetSecurity;

    /* Sanity checks */

    ntError =  PvfsAcquireCCB(pIrp->FileHandle, &pCcb);
    BAIL_ON_NT_STATUS(ntError);

    BAIL_ON_INVALID_PTR(Args.SecurityDescriptor, ntError);

    ntError = PvfsAccessCheckFileHandle(pCcb, FILE_READ_ATTRIBUTES);
    BAIL_ON_NT_STATUS(ntError);

    prReturnSecDesc = Args.SecurityDescriptor;
    SecDescLength = Args.Length;
    SecInfo  = Args.SecurityInformation;

    /* Real work starts here */

    if (pCcb->CreateOptions & FILE_DIRECTORY_FILE) {
        ntError = PvfsCreateDefaultSecDescDir(&paDefaultSecDesc);
    } else {
        ntError = PvfsCreateDefaultSecDescFile(&paDefaultSecDesc);
    }
    BAIL_ON_NT_STATUS(ntError);

    /* Ignore the SecurirityInformation for now */

    ntError = RtlAbsoluteToSelfRelativeSD(paDefaultSecDesc,
                                          prReturnSecDesc,
                                          &SecDescLength);
    BAIL_ON_NT_STATUS(ntError);

    pIrp->IoStatusBlock.BytesTransferred = SecDescLength;
    ntError = STATUS_SUCCESS;

cleanup:
    if (paDefaultSecDesc) {
        PvfsFreeAbsoluteSecurityDescriptor(paDefaultSecDesc);
    }

    if (pCcb) {
        PvfsReleaseCCB(pCcb);
    }

    return ntError;

error:
    goto cleanup;
}

/****************************************************************
 ***************************************************************/

static NTSTATUS
PvfsSetSecurityFile(
    PPVFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntError = STATUS_ACCESS_DENIED;
    PIRP pIrp = pIrpContext->pIrp;
    PPVFS_CCB pCcb = NULL;
    PSECURITY_DESCRIPTOR_RELATIVE pSecDesc = NULL;
    ULONG SecDescLength = 0;
    SECURITY_INFORMATION SecInfo = 0;
    IRP_ARGS_QUERY_SET_SECURITY Args = pIrpContext->pIrp->Args.QuerySetSecurity;

    /* Sanity checks */

    ntError =  PvfsAcquireCCB(pIrp->FileHandle, &pCcb);
    BAIL_ON_NT_STATUS(ntError);

    BAIL_ON_INVALID_PTR(Args.SecurityDescriptor, ntError);

    ntError = PvfsAccessCheckFileHandle(pCcb, FILE_WRITE_ATTRIBUTES);
    BAIL_ON_NT_STATUS(ntError);

    pSecDesc = Args.SecurityDescriptor;
    SecDescLength = Args.Length;
    SecInfo  = Args.SecurityInformation;

    /* Real work starts here */

    BAIL_ON_NT_STATUS(ntError);

    pIrp->IoStatusBlock.BytesTransferred = 0;
    ntError = STATUS_SUCCESS;

cleanup:
    if (pCcb) {
        PvfsReleaseCCB(pCcb);
    }

    return ntError;

error:
    goto cleanup;
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/

