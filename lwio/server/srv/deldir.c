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

#include "includes.h"

static
NTSTATUS
SrvUnmarshallDeleteDirectoryRequest(
    PSMB_PACKET pSmbRequest
    );

static
NTSTATUS
SrvDeleteFile(
    PSMB_SRV_FILE pFile
    );

static
NTSTATUS
SrvBuildDeleteDirectoryResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    PSMB_PACKET*        ppSmbResponse
    );

NTSTATUS
SrvProcessDeleteDirectory(
    PLWIO_SRV_CONTEXT pContext
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_SRV_CONNECTION pConnection = pContext->pConnection;
    PSMB_PACKET pSmbRequest = pContext->pRequest;
    PSMB_PACKET pSmbResponse = NULL;
    PSMB_SRV_FILE pFile = NULL;

    ntStatus = SrvUnmarshallDeleteDirectoryRequest(
                    pSmbRequest);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvDeleteFile(pFile);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvBuildDeleteDirectoryResponse(
                        pConnection,
                        pSmbRequest,
                        &pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvConnectionWriteMessage(
                        pConnection,
                        pContext->ulRequestSequence + 1,
                        pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    if (pSmbResponse)
    {
        SMBPacketFree(
            pConnection->hPacketAllocator,
            pSmbResponse);
    }

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
SrvUnmarshallDeleteDirectoryRequest(
    PSMB_PACKET pSmbRequest
    )
{
    NTSTATUS ntStatus = 0;

    return (ntStatus);
}

static
NTSTATUS
SrvDeleteFile(
    PSMB_SRV_FILE pFile
    )
{
    NTSTATUS ntStatus = 0;

    return ntStatus;
}

static
NTSTATUS
SrvBuildDeleteDirectoryResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    PSMB_PACKET*        ppSmbResponse
    )
{
    NTSTATUS ntStatus = 0;

    return ntStatus;
}

