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
 *        namedpipe.c
 *
 * Abstract:
 *
 *
 *
 * Authors: Krishna Ganugapati (dalmeida@likewise.com)
 */

#include "includes.h"

NTSTATUS
NpfsCreateNamedPipe(
    IN PIRP pIrp
    )
{
    NTntStatus ntStatus = STATUS_NOT_IMPLEMENTED;
    int EE = 0;
    UNICODE_STRING path = { 0 };
    UNICODE_STRING prefixPath = { 0 };
    UNICODE_STRING allowPrefix = { 0 };
    PIT_CCB pCcb = NULL;
    PIO_ECP_NAMED_PIPE pipeParams = NULL;
    ULONG ecpSize = 0;

    if (!pIrp->Args.Create.EcpList)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntntStatus);
    }

    ntStatus = IoRtlEcpListFind(
                    pIrp->Args.Create.EcpList,
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
        BAIL_ON_NT_STATUS(ntntStatus);
    }

    RtlUnicodeStringInit(&path, pIrp->Args.Create.FileName.FileName);

    ntStatus = RtlUnicodeStringAllocateFromCString(&allowPrefix, IOTEST_INTERNAL_PATH_NAMED_PIPE);
    BAIL_ON_NT_STATUS(ntStatus);

    // TODO -- Add some IoRtlPath prefix functions...
    if (path.Length <= allowPrefix.Length || !IoRtlPathIsSeparator((path.Buffer[allowPrefix.Length/sizeof(allowPrefix.Buffer[0])])))
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntntStatus);
    }

    prefixPath.Buffer = path.Buffer;
    prefixPath.Length = allowPrefix.Length;
    prefixPath.MaximumLength = prefixPath.Length;

    // Only succeed for the given prefix.
    if (!RtlUnicodeStringIsEqual(&prefixPath, &allowPrefix, FALSE))
    {
        ntStatus = STATUS_UNSUCCESSFUL;
        BAIL_ON_NT_STATUS(ntntStatus);
    }

    // Would have to check whether pipe already exists, etc.

    ntStatus = NpfsCreateCcb(&pCcb, &path);
    BAIL_ON_NT_STATUS(ntStatus);

    pCcb->IsNamedPipe = TRUE;

    ntStatus = IoFileSetContext(pIrp->FileHandle, pCcb);
    BAIL_ON_NT_STATUS(ntStatus);

    pCcb = NULL;

cleanup:
    NpfsDestroyCcb(&pCcb);
    RtlUnicodeStringFree(&allowPrefix);

    pIrp->IontStatusBlock.ntStatus = ntStatus;

    LOG_LEAVE_IF_STATUS_EE(ntStatus);
    return ntStatus;
}
