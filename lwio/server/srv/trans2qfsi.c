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
SrvBuildFSAllocationInfoResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    PSMB_PACKET*        ppSmbResponse
    );

static
NTSTATUS
SrvBuildFSInfoVolumeResponse(
                PSMB_SRV_CONNECTION pConnection,
                PSMB_PACKET         pSmbRequest,
                PSMB_PACKET*        ppSmbResponse
                );

static
NTSTATUS
SrvBuildFSVolumeInfoResponse(
                PSMB_SRV_CONNECTION pConnection,
                PSMB_PACKET         pSmbRequest,
                PSMB_PACKET*        ppSmbResponse
                );

static
NTSTATUS
SrvBuildFSSizeInfoResponse(
                PSMB_SRV_CONNECTION pConnection,
                PSMB_PACKET         pSmbRequest,
                PSMB_PACKET*        ppSmbResponse
                );

static
NTSTATUS
SrvBuildFSDeviceInfoResponse(
                PSMB_SRV_CONNECTION pConnection,
                PSMB_PACKET         pSmbRequest,
                PSMB_PACKET*        ppSmbResponse
                );

static
NTSTATUS
SrvBuildFSAttributeInfoResponse(
                PSMB_SRV_CONNECTION pConnection,
                PSMB_PACKET         pSmbRequest,
                PSMB_PACKET*        ppSmbResponse
                );

static
NTSTATUS
SrvBuildFSCifsUnixInfoResponse(
                PSMB_SRV_CONNECTION pConnection,
                PSMB_PACKET         pSmbRequest,
                PSMB_PACKET*        ppSmbResponse
                );

static
NTSTATUS
SrvBuildMacFSInfoResponse(
                PSMB_SRV_CONNECTION pConnection,
                PSMB_PACKET         pSmbRequest,
                PSMB_PACKET*        ppSmbResponse
                );

NTSTATUS
SrvProcessTrans2QueryFilesystemInformation(
    PSMB_SRV_CONNECTION         pConnection,
    PSMB_PACKET                 pSmbRequest,
    PTRANSACTION_REQUEST_HEADER pRequestHeader,
    PUSHORT                     pSetup,
    PUSHORT                     pByteCount,
    PBYTE                       pParameters,
    PBYTE                       pData,
    PSMB_PACKET*                ppSmbResponse
    )
{
    NTSTATUS ntStatus = 0;
    USHORT   usInfoLevel = 0;
    PSMB_PACKET pSmbResponse = NULL;

    if ((pRequestHeader->parameterCount != 2) &&
        (pRequestHeader->parameterCount != 4))
    {
        ntStatus = STATUS_DATA_ERROR;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    usInfoLevel = *((PUSHORT)pParameters);

    switch (usInfoLevel)
    {
        case SMB_INFO_ALLOCATION:

            ntStatus = SrvBuildFSAllocationInfoResponse(
                            pConnection,
                            pSmbRequest,
                            &pSmbResponse);

            break;

        case SMB_INFO_VOLUME:

            ntStatus = SrvBuildFSInfoVolumeResponse(
                            pConnection,
                            pSmbRequest,
                            &pSmbResponse);

            break;

        case SMB_QUERY_FS_VOLUME_INFO:

            ntStatus = SrvBuildFSVolumeInfoResponse(
                            pConnection,
                            pSmbRequest,
                            &pSmbResponse);

            break;

        case SMB_QUERY_FS_SIZE_INFO:

            ntStatus = SrvBuildFSSizeInfoResponse(
                            pConnection,
                            pSmbRequest,
                            &pSmbResponse);

            break;

        case SMB_QUERY_FS_DEVICE_INFO:

            ntStatus = SrvBuildFSDeviceInfoResponse(
                            pConnection,
                            pSmbRequest,
                            &pSmbResponse);

            break;

        case SMB_QUERY_FS_ATTRIBUTE_INFO:

            ntStatus = SrvBuildFSAttributeInfoResponse(
                            pConnection,
                            pSmbRequest,
                            &pSmbResponse);

            break;


        case SMB_QUERY_CIFS_UNIX_INFO:

            ntStatus = SrvBuildFSCifsUnixInfoResponse(
                            pConnection,
                            pSmbRequest,
                            &pSmbResponse);

            break;

        case SMB_QUERY_MAC_FS_INFO:

            ntStatus = SrvBuildMacFSInfoResponse(
                            pConnection,
                            pSmbRequest,
                            &pSmbResponse);

            break;

        default:

            ntStatus = STATUS_DATA_ERROR;

            break;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    *ppSmbResponse = pSmbResponse;

cleanup:

    return ntStatus;

error:

    *ppSmbResponse = NULL;

    if (pSmbResponse)
    {
        SMBPacketFree(
            pConnection->hPacketAllocator,
            pSmbResponse);
    }

    goto cleanup;
}

static
NTSTATUS
SrvBuildFSAllocationInfoResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    PSMB_PACKET*        ppSmbResponse
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

static
NTSTATUS
SrvBuildFSInfoVolumeResponse(
                PSMB_SRV_CONNECTION pConnection,
                PSMB_PACKET         pSmbRequest,
                PSMB_PACKET*        ppSmbResponse
                )
{
    return STATUS_NOT_IMPLEMENTED;
}

static
NTSTATUS
SrvBuildFSVolumeInfoResponse(
                PSMB_SRV_CONNECTION pConnection,
                PSMB_PACKET         pSmbRequest,
                PSMB_PACKET*        ppSmbResponse
                )
{
    return STATUS_NOT_IMPLEMENTED;
}

static
NTSTATUS
SrvBuildFSSizeInfoResponse(
                PSMB_SRV_CONNECTION pConnection,
                PSMB_PACKET         pSmbRequest,
                PSMB_PACKET*        ppSmbResponse
                )
{
    return STATUS_NOT_IMPLEMENTED;
}

static
NTSTATUS
SrvBuildFSDeviceInfoResponse(
                PSMB_SRV_CONNECTION pConnection,
                PSMB_PACKET         pSmbRequest,
                PSMB_PACKET*        ppSmbResponse
                )
{
    return STATUS_NOT_IMPLEMENTED;
}

static
NTSTATUS
SrvBuildFSAttributeInfoResponse(
                PSMB_SRV_CONNECTION pConnection,
                PSMB_PACKET         pSmbRequest,
                PSMB_PACKET*        ppSmbResponse
                )
{
    return STATUS_NOT_IMPLEMENTED;
}

static
NTSTATUS
SrvBuildFSCifsUnixInfoResponse(
                PSMB_SRV_CONNECTION pConnection,
                PSMB_PACKET         pSmbRequest,
                PSMB_PACKET*        ppSmbResponse
                )
{
    return STATUS_NOT_IMPLEMENTED;
}

static
NTSTATUS
SrvBuildMacFSInfoResponse(
                PSMB_SRV_CONNECTION pConnection,
                PSMB_PACKET         pSmbRequest,
                PSMB_PACKET*        ppSmbResponse
                )
{
    return STATUS_NOT_IMPLEMENTED;
}

