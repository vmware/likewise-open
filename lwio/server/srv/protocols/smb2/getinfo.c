/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        queryinfo.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Protocols API - SMBV2
 *
 *        Query Information
 *
 * Authors: Krishna Ganugapati (kganugapati@likewise.com)
 *          Sriram Nambakam (snambakam@likewise.com)
 *
 *
 */

#include "includes.h"

static
NTSTATUS
SrvGetFileInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB_PACKET                   pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET*                  ppSmbResponse
    );

NTSTATUS
SrvGetFileBasicInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB_PACKET                   pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET*                  ppSmbResponse
    );

NTSTATUS
SrvGetFileStandardInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB_PACKET                   pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET*                  ppSmbResponse
    );

NTSTATUS
SrvGetFileInternalInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB_PACKET                   pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET*                  ppSmbResponse
    );

NTSTATUS
SrvGetFileEAInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB_PACKET                   pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET*                  ppSmbResponse
    );

NTSTATUS
SrvGetFileAccessInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB_PACKET                   pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET*                  ppSmbResponse
    );

NTSTATUS
SrvGetFilePositionInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB_PACKET                   pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET*                  ppSmbResponse
    );

NTSTATUS
SrvGetFileFullEAInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB_PACKET                   pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET*                  ppSmbResponse
    );

NTSTATUS
SrvGetFileModeInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB_PACKET                   pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET*                  ppSmbResponse
    );

NTSTATUS
SrvGetFileAlignmentInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB_PACKET                   pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET*                  ppSmbResponse
    );

NTSTATUS
SrvGetFileAllInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB_PACKET                   pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET*                  ppSmbResponse
    );

NTSTATUS
SrvGetFileAlternateNameInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB_PACKET                   pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET*                  ppSmbResponse
    );

NTSTATUS
SrvGetFileStreamInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB_PACKET                   pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET*                  ppSmbResponse
    );

NTSTATUS
SrvGetFileCompressionInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB_PACKET                   pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET*                  ppSmbResponse
    );

NTSTATUS
SrvGetFileNetworkOpenInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB_PACKET                   pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET*                  ppSmbResponse
    );

NTSTATUS
SrvGetFileAttributeTagInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB_PACKET                   pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET*                  ppSmbResponse
    );

static
NTSTATUS
SrvGetFileSystemInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB_PACKET                   pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET*                  ppSmbResponse
    );

static
NTSTATUS
SrvGetFileSystemVolumeInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB_PACKET                   pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET*                  ppSmbResponse
    );

static
NTSTATUS
SrvGetFileSystemSizeInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB_PACKET                   pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET*                  ppSmbResponse
    );

static
NTSTATUS
SrvGetFileSystemDeviceInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB_PACKET                   pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET*                  ppSmbResponse
    );

static
NTSTATUS
SrvGetFileSystemAttributeInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB_PACKET                   pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET*                  ppSmbResponse
    );

static
NTSTATUS
SrvGetFileSystemQuotaInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB_PACKET                   pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET*                  ppSmbResponse
    );

static
NTSTATUS
SrvGetFileSystemFullInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB_PACKET                   pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET*                  ppSmbResponse
    );

static
NTSTATUS
SrvGetFileSystemObjectIdInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB_PACKET                   pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET*                  ppSmbResponse
    );

static
NTSTATUS
SrvGetSecurityInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB_PACKET                   pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET*                  ppSmbResponse
    );

static
NTSTATUS
SrvGetFileSecurityBasicInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB_PACKET                   pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET*                  ppSmbResponse
    );

NTSTATUS
SrvProcessGetInfo_SMB_V2(
    PLWIO_SRV_CONNECTION pConnection,
    PSMB_PACKET          pSmbRequest,
    PSMB_PACKET*         ppSmbResponse
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader = NULL; // Do not free
    PLWIO_SRV_SESSION_2 pSession = NULL;
    PLWIO_SRV_TREE_2    pTree = NULL;
    PLWIO_SRV_FILE_2    pFile = NULL;
    PSMB_PACKET pSmbResponse = NULL;

    ntStatus = SrvConnection2FindSession(
                    pConnection,
                    pSmbRequest->pSMB2Header->ullSessionId,
                    &pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvSession2FindTree(
                    pSession,
                    pSmbRequest->pSMB2Header->ulTid,
                    &pTree);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMB2UnmarshalGetInfoRequest(pSmbRequest, &pRequestHeader);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvTree2FindFile(
                        pTree,
                        pRequestHeader->fid.ullVolatileId,
                        &pFile);
    BAIL_ON_NT_STATUS(ntStatus);

    switch (pRequestHeader->ucInfoType)
    {
        case SMB2_INFO_TYPE_FILE:

            ntStatus = SrvGetFileInfo_SMB_V2(
                            pConnection,
                            pSmbRequest,
                            pFile,
                            pRequestHeader,
                            &pSmbResponse);

            break;

        case SMB2_INFO_TYPE_FILE_SYSTEM:

            ntStatus = SrvGetFileSystemInfo_SMB_V2(
                            pConnection,
                            pSmbRequest,
                            pFile,
                            pRequestHeader,
                            &pSmbResponse);

            break;

        case SMB2_INFO_TYPE_SECURITY:

            ntStatus = SrvGetSecurityInfo_SMB_V2(
                            pConnection,
                            pSmbRequest,
                            pFile,
                            pRequestHeader,
                            &pSmbResponse);

            break;

        default:

            ntStatus = STATUS_INVALID_INFO_CLASS;

            break;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    *ppSmbResponse = pSmbResponse;

cleanup:

    if (pFile)
    {
        SrvFile2Release(pFile);
    }

    if (pTree)
    {
        SrvTree2Release(pTree);
    }

    if (pSession)
    {
        SrvSession2Release(pSession);
    }

    return ntStatus;

error:

    *ppSmbResponse = NULL;

    if (pSmbResponse)
    {
        SMBPacketFree(pConnection->hPacketAllocator, pSmbResponse);
    }

    goto cleanup;
}

static
NTSTATUS
SrvGetFileInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB_PACKET                   pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET*                  ppSmbResponse
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    switch (pRequestHeader->ucInfoClass)
    {
        case SMB2_FILE_INFO_CLASS_BASIC :

            ntStatus = SrvGetFileBasicInfo_SMB_V2(
                            pConnection,
                            pSmbRequest,
                            pFile,
                            pRequestHeader,
                            ppSmbResponse);

            break;

        case SMB2_FILE_INFO_CLASS_STANDARD :

            ntStatus = SrvGetFileStandardInfo_SMB_V2(
                            pConnection,
                            pSmbRequest,
                            pFile,
                            pRequestHeader,
                            ppSmbResponse);

            break;

        case SMB2_FILE_INFO_CLASS_INTERNAL :

            ntStatus = SrvGetFileInternalInfo_SMB_V2(
                            pConnection,
                            pSmbRequest,
                            pFile,
                            pRequestHeader,
                            ppSmbResponse);

            break;

        case SMB2_FILE_INFO_CLASS_EA :

            ntStatus = SrvGetFileEAInfo_SMB_V2(
                            pConnection,
                            pSmbRequest,
                            pFile,
                            pRequestHeader,
                            ppSmbResponse);

            break;

        case SMB2_FILE_INFO_CLASS_ACCESS :

            ntStatus = SrvGetFileAccessInfo_SMB_V2(
                            pConnection,
                            pSmbRequest,
                            pFile,
                            pRequestHeader,
                            ppSmbResponse);

            break;

        case SMB2_FILE_INFO_CLASS_POSITION :

            ntStatus = SrvGetFilePositionInfo_SMB_V2(
                            pConnection,
                            pSmbRequest,
                            pFile,
                            pRequestHeader,
                            ppSmbResponse);

            break;

        case SMB2_FILE_INFO_FULL_EA :

            ntStatus = SrvGetFileFullEAInfo_SMB_V2(
                            pConnection,
                            pSmbRequest,
                            pFile,
                            pRequestHeader,
                            ppSmbResponse);

            break;

        case SMB2_FILE_INFO_CLASS_MODE :

            ntStatus = SrvGetFileModeInfo_SMB_V2(
                            pConnection,
                            pSmbRequest,
                            pFile,
                            pRequestHeader,
                            ppSmbResponse);

            break;

        case SMB2_FILE_INFO_CLASS_ALIGNMENT :

            ntStatus = SrvGetFileAlignmentInfo_SMB_V2(
                            pConnection,
                            pSmbRequest,
                            pFile,
                            pRequestHeader,
                            ppSmbResponse);

            break;

        case SMB2_FILE_INFO_CLASS_ALL :

            ntStatus = SrvGetFileAllInfo_SMB_V2(
                            pConnection,
                            pSmbRequest,
                            pFile,
                            pRequestHeader,
                            ppSmbResponse);

            break;

        case SMB2_FILE_INFO_CLASS_ALTERNATE_NAME :

            ntStatus = SrvGetFileAlternateNameInfo_SMB_V2(
                            pConnection,
                            pSmbRequest,
                            pFile,
                            pRequestHeader,
                            ppSmbResponse);

            break;

        case SMB2_FILE_INFO_CLASS_STREAM :

            ntStatus = SrvGetFileStreamInfo_SMB_V2(
                            pConnection,
                            pSmbRequest,
                            pFile,
                            pRequestHeader,
                            ppSmbResponse);

            break;

        case SMB2_FILE_INFO_CLASS_COMPRESSION :

            ntStatus = SrvGetFileCompressionInfo_SMB_V2(
                            pConnection,
                            pSmbRequest,
                            pFile,
                            pRequestHeader,
                            ppSmbResponse);

            break;

        case SMB2_FILE_INFO_CLASS_NETWORK_OPEN :

            ntStatus = SrvGetFileNetworkOpenInfo_SMB_V2(
                            pConnection,
                            pSmbRequest,
                            pFile,
                            pRequestHeader,
                            ppSmbResponse);

            break;

        case SMB2_FILE_INFO_CLASS_ATTRIBUTE_TAG :

            ntStatus = SrvGetFileAttributeTagInfo_SMB_V2(
                            pConnection,
                            pSmbRequest,
                            pFile,
                            pRequestHeader,
                            ppSmbResponse);

            break;

        default:

            ntStatus = STATUS_INVALID_INFO_CLASS;

            break;
    }

    return ntStatus;
}

NTSTATUS
SrvGetFileBasicInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB_PACKET                   pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET*                  ppSmbResponse
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
SrvGetFileStandardInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB_PACKET                   pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET*                  ppSmbResponse
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
SrvGetFileInternalInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB_PACKET                   pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET*                  ppSmbResponse
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
SrvGetFileEAInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB_PACKET                   pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET*                  ppSmbResponse
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
SrvGetFileAccessInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB_PACKET                   pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET*                  ppSmbResponse
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
SrvGetFilePositionInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB_PACKET                   pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET*                  ppSmbResponse
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
SrvGetFileFullEAInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB_PACKET                   pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET*                  ppSmbResponse
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
SrvGetFileModeInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB_PACKET                   pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET*                  ppSmbResponse
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
SrvGetFileAlignmentInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB_PACKET                   pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET*                  ppSmbResponse
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
SrvGetFileAllInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB_PACKET                   pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET*                  ppSmbResponse
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
SrvGetFileAlternateNameInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB_PACKET                   pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET*                  ppSmbResponse
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
SrvGetFileStreamInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB_PACKET                   pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET*                  ppSmbResponse
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
SrvGetFileCompressionInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB_PACKET                   pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET*                  ppSmbResponse
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
SrvGetFileNetworkOpenInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB_PACKET                   pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET*                  ppSmbResponse
    )
{
    return STATUS_INVALID_PARAMETER;
}

NTSTATUS
SrvGetFileAttributeTagInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB_PACKET                   pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET*                  ppSmbResponse
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

static
NTSTATUS
SrvGetFileSystemInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB_PACKET                   pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET*                  ppSmbResponse
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    switch (pRequestHeader->ucInfoClass)
    {
        case SMB2_FS_INFO_CLASS_VOLUME:

            ntStatus = SrvGetFileSystemVolumeInfo_SMB_V2(
                            pConnection,
                            pSmbRequest,
                            pFile,
                            pRequestHeader,
                            ppSmbResponse);

            break;

        case SMB2_FS_INFO_CLASS_SIZE:

            ntStatus = SrvGetFileSystemSizeInfo_SMB_V2(
                            pConnection,
                            pSmbRequest,
                            pFile,
                            pRequestHeader,
                            ppSmbResponse);

            break;

        case SMB2_FS_INFO_CLASS_DEVICE:

            ntStatus = SrvGetFileSystemDeviceInfo_SMB_V2(
                            pConnection,
                            pSmbRequest,
                            pFile,
                            pRequestHeader,
                            ppSmbResponse);

            break;

        case SMB2_FS_INFO_CLASS_ATTRIBUTE:

            ntStatus = SrvGetFileSystemAttributeInfo_SMB_V2(
                            pConnection,
                            pSmbRequest,
                            pFile,
                            pRequestHeader,
                            ppSmbResponse);

            break;

        case SMB2_FS_INFO_CLASS_QUOTA:

            ntStatus = SrvGetFileSystemQuotaInfo_SMB_V2(
                            pConnection,
                            pSmbRequest,
                            pFile,
                            pRequestHeader,
                            ppSmbResponse);

            break;

        case SMB2_FS_INFO_CLASS_FULL_SIZE:

            ntStatus = SrvGetFileSystemFullInfo_SMB_V2(
                            pConnection,
                            pSmbRequest,
                            pFile,
                            pRequestHeader,
                            ppSmbResponse);

            break;

        case SMB_FS_INFO_CLASS_OBJECTID:

            ntStatus = SrvGetFileSystemObjectIdInfo_SMB_V2(
                            pConnection,
                            pSmbRequest,
                            pFile,
                            pRequestHeader,
                            ppSmbResponse);

            break;

        default:

            ntStatus = STATUS_INVALID_INFO_CLASS;

            break;
    }

    return ntStatus;
}

static
NTSTATUS
SrvGetFileSystemVolumeInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB_PACKET                   pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET*                  ppSmbResponse
    )
{
    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    PBYTE                       pResponseBuffer = NULL;
    size_t                      sAllocatedSize = 0;
    ULONG                       ulResponseBufferLen = 0;
    IO_STATUS_BLOCK             ioStatusBlock = {0};
    PFILE_FS_VOLUME_INFORMATION pFSVolInfo = NULL;
    PSMB2_GET_INFO_RESPONSE_HEADER pGetInfoResponseHeader = NULL;
    PSMB_FS_VOLUME_INFO_HEADER  pFSVolInfoHeader = NULL;
    USHORT                      usVolumeLabelLen = 0;
    ULONG                       ulBytesAvailable = 0;
    ULONG                       ulOffset = 0;
    PBYTE                       pBuffer = NULL;
    PSMB_PACKET                 pSmbResponse = NULL;

    ntStatus = SMBPacketBufferAllocate(
                    pConnection->hPacketAllocator,
                    pRequestHeader->ulOutputBufferLen,
                    &pResponseBuffer,
                    &sAllocatedSize);
    BAIL_ON_NT_STATUS(ntStatus);

    ulResponseBufferLen = sAllocatedSize;

    ntStatus = IoQueryVolumeInformationFile(
                            pFile->hFile,
                            NULL,
                            &ioStatusBlock,
                            pResponseBuffer,
                            ulResponseBufferLen,
                            FileFsVolumeInformation);
    BAIL_ON_NT_STATUS(ntStatus);

    ulResponseBufferLen = ioStatusBlock.BytesTransferred;

    ntStatus = SMBPacketAllocate(
                    pConnection->hPacketAllocator,
                    &pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketBufferAllocate(
                    pConnection->hPacketAllocator,
                    64 * 1024,
                    &pSmbResponse->pRawBuffer,
                    &pSmbResponse->bufferLen);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMB2MarshalHeader(
                    pSmbResponse,
                    COM2_GETINFO,
                    0,
                    1,
                    pSmbRequest->pSMB2Header->ulPid,
                    pSmbRequest->pSMB2Header->ullCommandSequence,
                    pSmbRequest->pSMB2Header->ulTid,
                    pSmbRequest->pSMB2Header->ullSessionId,
                    STATUS_SUCCESS,
                    TRUE,
                    TRUE);
    BAIL_ON_NT_STATUS(ntStatus);

    ulBytesAvailable = pSmbResponse->bufferLen - pSmbResponse->bufferUsed;
    pBuffer = pSmbResponse->pParams;
    ulOffset = (PBYTE)pSmbResponse->pParams - (PBYTE)pSmbResponse->pSMB2Header;

    if (ulBytesAvailable < sizeof(SMB2_GET_INFO_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pGetInfoResponseHeader = (PSMB2_GET_INFO_RESPONSE_HEADER)pBuffer;

    ulBytesAvailable -= sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    pBuffer += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulOffset += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    pSmbResponse->bufferUsed += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);

    pGetInfoResponseHeader->usLength = sizeof(SMB2_GET_INFO_RESPONSE_HEADER)+1;
    pGetInfoResponseHeader->usOutBufferOffset = ulOffset;

    pFSVolInfo = (PFILE_FS_VOLUME_INFORMATION)pResponseBuffer;
    usVolumeLabelLen = wc16slen(pFSVolInfo->VolumeLabel) * sizeof(wchar16_t);

    pGetInfoResponseHeader->ulOutBufferLength = sizeof(SMB_FS_VOLUME_INFO_HEADER);
    pGetInfoResponseHeader->ulOutBufferLength += usVolumeLabelLen;

    if (ulBytesAvailable < pGetInfoResponseHeader->ulOutBufferLength)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pFSVolInfoHeader = (PSMB_FS_VOLUME_INFO_HEADER)pBuffer;

    ulBytesAvailable -= sizeof(SMB_FS_VOLUME_INFO_HEADER);
    pBuffer += sizeof(SMB_FS_VOLUME_INFO_HEADER);

    pFSVolInfoHeader->bSupportsObjects = pFSVolInfo->SupportsObjects;
    pFSVolInfoHeader->llVolumeCreationTime = pFSVolInfo->VolumeCreationTime;
    pFSVolInfoHeader->ulVolumeSerialNumber = pFSVolInfo->VolumeSerialNumber;
    pFSVolInfoHeader->ulVolumeLabelLength = usVolumeLabelLen;

    if (usVolumeLabelLen)
    {
        memcpy(pBuffer, (PBYTE)pFSVolInfo->VolumeLabel, usVolumeLabelLen);
    }

    pSmbResponse->bufferUsed += pGetInfoResponseHeader->ulOutBufferLength;

    ntStatus = SMB2MarshalFooter(pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppSmbResponse = pSmbResponse;

cleanup:

    if (pResponseBuffer)
    {
        SMBPacketBufferFree(
                pConnection->hPacketAllocator,
                pResponseBuffer,
                sAllocatedSize);
    }

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
SrvGetFileSystemSizeInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB_PACKET                   pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET*                  ppSmbResponse
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

static
NTSTATUS
SrvGetFileSystemDeviceInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB_PACKET                   pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET*                  ppSmbResponse
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

static
NTSTATUS
SrvGetFileSystemAttributeInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB_PACKET                   pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET*                  ppSmbResponse
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

static
NTSTATUS
SrvGetFileSystemQuotaInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB_PACKET                   pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET*                  ppSmbResponse
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

static
NTSTATUS
SrvGetFileSystemFullInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB_PACKET                   pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET*                  ppSmbResponse
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

static
NTSTATUS
SrvGetFileSystemObjectIdInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB_PACKET                   pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET*                  ppSmbResponse
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

static
NTSTATUS
SrvGetSecurityInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB_PACKET                   pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET*                  ppSmbResponse
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    switch (pRequestHeader->ucInfoClass)
    {
        case SMB2_SEC_INFO_CLASS_BASIC:

            ntStatus = SrvGetFileSecurityBasicInfo_SMB_V2(
                            pConnection,
                            pSmbRequest,
                            pFile,
                            pRequestHeader,
                            ppSmbResponse);

            break;

        default:

            ntStatus = STATUS_INVALID_INFO_CLASS;

            break;
    }

    return ntStatus;
}

static
NTSTATUS
SrvGetFileSecurityBasicInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB_PACKET                   pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET*                  ppSmbResponse
    )
{
    return STATUS_NOT_IMPLEMENTED;
}
