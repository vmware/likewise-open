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
    PSMB2_MESSAGE                 pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET                   pSmbResponse
    );

NTSTATUS
SrvGetFileBasicInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB2_MESSAGE                 pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET                   pSmbResponse
    );

NTSTATUS
SrvGetFileStandardInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB2_MESSAGE                 pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET                   pSmbResponse
    );

NTSTATUS
SrvGetFileInternalInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB2_MESSAGE                 pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET                   pSmbResponse
    );

NTSTATUS
SrvGetFileEAInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB2_MESSAGE                 pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET                   pSmbResponse
    );

NTSTATUS
SrvGetFileAccessInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB2_MESSAGE                 pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET                   pSmbResponse
    );

NTSTATUS
SrvGetFilePositionInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB2_MESSAGE                 pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET                   pSmbResponse
    );

NTSTATUS
SrvGetFileFullEAInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB2_MESSAGE                 pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET                   pSmbResponse
    );

NTSTATUS
SrvGetFileModeInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB2_MESSAGE                 pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET                   pSmbResponse
    );

NTSTATUS
SrvGetFileAlignmentInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB2_MESSAGE                 pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET                   pSmbResponse
    );

NTSTATUS
SrvGetFileAllInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB2_MESSAGE                 pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET                   pSmbResponse
    );

NTSTATUS
SrvGetFileAlternateNameInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB2_MESSAGE                 pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET                   pSmbResponse
    );

NTSTATUS
SrvGetFileStreamInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB2_MESSAGE                 pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET                   pSmbResponse
    );

NTSTATUS
SrvGetFileCompressionInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB2_MESSAGE                 pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET                   pSmbResponse
    );

NTSTATUS
SrvGetFileNetworkOpenInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB2_MESSAGE                 pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET                   pSmbResponse
    );

NTSTATUS
SrvGetFileAttributeTagInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB2_MESSAGE                 pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET                   pSmbResponse
    );

static
NTSTATUS
SrvGetFileSystemInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB2_MESSAGE                 pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET                   pSmbResponse
    );

static
NTSTATUS
SrvGetFileSystemVolumeInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB2_MESSAGE                 pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET                   pSmbResponse
    );

static
NTSTATUS
SrvGetFileSystemSizeInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB2_MESSAGE                 pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET                   pSmbResponse
    );

static
NTSTATUS
SrvGetFileSystemDeviceInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB2_MESSAGE                 pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET                   pSmbResponse
    );

static
NTSTATUS
SrvGetFileSystemAttributeInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB2_MESSAGE                 pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET                   pSmbResponse
    );

static
NTSTATUS
SrvGetFileSystemQuotaInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB2_MESSAGE                 pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET                   pSmbResponse
    );

static
NTSTATUS
SrvGetFileSystemFullInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB2_MESSAGE                 pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET                   pSmbResponse
    );

static
NTSTATUS
SrvGetFileSystemObjectIdInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB2_MESSAGE                 pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET                   pSmbResponse
    );

static
NTSTATUS
SrvGetSecurityInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB2_MESSAGE                 pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET                   pSmbResponse
    );

static
NTSTATUS
SrvGetFileSecurityBasicInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB2_MESSAGE                 pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET                   pSmbResponse
    );

NTSTATUS
SrvProcessGetInfo_SMB_V2(
    IN     PSMB2_CONTEXT pContext,
    IN     PSMB2_MESSAGE pSmbRequest,
    IN OUT PSMB_PACKET   pSmbResponse
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLWIO_SRV_CONNECTION pConnection = pContext->pConnection;
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader = NULL; // Do not free
    PLWIO_SRV_SESSION_2 pSession = NULL;
    PLWIO_SRV_TREE_2    pTree = NULL;
    PLWIO_SRV_FILE_2    pFile = NULL;

    ntStatus = SrvConnection2FindSession_SMB_V2(
                    pContext,
                    pConnection,
                    pSmbRequest->pHeader->ullSessionId,
                    &pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvSession2FindTree_SMB_V2(
                    pContext,
                    pSession,
                    pSmbRequest->pHeader->ulTid,
                    &pTree);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMB2UnmarshalGetInfoRequest(pSmbRequest, &pRequestHeader);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvTree2FindFile_SMB_V2(
                        pContext,
                        pTree,
                        &pRequestHeader->fid,
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
                            pSmbResponse);

            break;

        case SMB2_INFO_TYPE_FILE_SYSTEM:

            ntStatus = SrvGetFileSystemInfo_SMB_V2(
                            pConnection,
                            pSmbRequest,
                            pFile,
                            pRequestHeader,
                            pSmbResponse);

            break;

        case SMB2_INFO_TYPE_SECURITY:

            ntStatus = SrvGetSecurityInfo_SMB_V2(
                            pConnection,
                            pSmbRequest,
                            pFile,
                            pRequestHeader,
                            pSmbResponse);

            break;

        default:

            ntStatus = STATUS_INVALID_INFO_CLASS;

            break;
    }
    BAIL_ON_NT_STATUS(ntStatus);

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

    goto cleanup;
}

static
NTSTATUS
SrvGetFileInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB2_MESSAGE                 pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET                   pSmbResponse
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
                            pSmbResponse);

            break;

        case SMB2_FILE_INFO_CLASS_STANDARD :

            ntStatus = SrvGetFileStandardInfo_SMB_V2(
                            pConnection,
                            pSmbRequest,
                            pFile,
                            pRequestHeader,
                            pSmbResponse);

            break;

        case SMB2_FILE_INFO_CLASS_INTERNAL :

            ntStatus = SrvGetFileInternalInfo_SMB_V2(
                            pConnection,
                            pSmbRequest,
                            pFile,
                            pRequestHeader,
                            pSmbResponse);

            break;

        case SMB2_FILE_INFO_CLASS_EA :

            ntStatus = SrvGetFileEAInfo_SMB_V2(
                            pConnection,
                            pSmbRequest,
                            pFile,
                            pRequestHeader,
                            pSmbResponse);

            break;

        case SMB2_FILE_INFO_CLASS_ACCESS :

            ntStatus = SrvGetFileAccessInfo_SMB_V2(
                            pConnection,
                            pSmbRequest,
                            pFile,
                            pRequestHeader,
                            pSmbResponse);

            break;

        case SMB2_FILE_INFO_CLASS_POSITION :

            ntStatus = SrvGetFilePositionInfo_SMB_V2(
                            pConnection,
                            pSmbRequest,
                            pFile,
                            pRequestHeader,
                            pSmbResponse);

            break;

        case SMB2_FILE_INFO_FULL_EA :

            ntStatus = SrvGetFileFullEAInfo_SMB_V2(
                            pConnection,
                            pSmbRequest,
                            pFile,
                            pRequestHeader,
                            pSmbResponse);

            break;

        case SMB2_FILE_INFO_CLASS_MODE :

            ntStatus = SrvGetFileModeInfo_SMB_V2(
                            pConnection,
                            pSmbRequest,
                            pFile,
                            pRequestHeader,
                            pSmbResponse);

            break;

        case SMB2_FILE_INFO_CLASS_ALIGNMENT :

            ntStatus = SrvGetFileAlignmentInfo_SMB_V2(
                            pConnection,
                            pSmbRequest,
                            pFile,
                            pRequestHeader,
                            pSmbResponse);

            break;

        case SMB2_FILE_INFO_CLASS_ALL :

            ntStatus = SrvGetFileAllInfo_SMB_V2(
                            pConnection,
                            pSmbRequest,
                            pFile,
                            pRequestHeader,
                            pSmbResponse);

            break;

        case SMB2_FILE_INFO_CLASS_ALTERNATE_NAME :

            ntStatus = SrvGetFileAlternateNameInfo_SMB_V2(
                            pConnection,
                            pSmbRequest,
                            pFile,
                            pRequestHeader,
                            pSmbResponse);

            break;

        case SMB2_FILE_INFO_CLASS_STREAM :

            ntStatus = SrvGetFileStreamInfo_SMB_V2(
                            pConnection,
                            pSmbRequest,
                            pFile,
                            pRequestHeader,
                            pSmbResponse);

            break;

        case SMB2_FILE_INFO_CLASS_COMPRESSION :

            ntStatus = SrvGetFileCompressionInfo_SMB_V2(
                            pConnection,
                            pSmbRequest,
                            pFile,
                            pRequestHeader,
                            pSmbResponse);

            break;

        case SMB2_FILE_INFO_CLASS_NETWORK_OPEN :

            ntStatus = SrvGetFileNetworkOpenInfo_SMB_V2(
                            pConnection,
                            pSmbRequest,
                            pFile,
                            pRequestHeader,
                            pSmbResponse);

            break;

        case SMB2_FILE_INFO_CLASS_ATTRIBUTE_TAG :

            ntStatus = SrvGetFileAttributeTagInfo_SMB_V2(
                            pConnection,
                            pSmbRequest,
                            pFile,
                            pRequestHeader,
                            pSmbResponse);

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
    PSMB2_MESSAGE                 pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET                   pSmbResponse
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
SrvGetFileStandardInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB2_MESSAGE                 pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET                   pSmbResponse
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
SrvGetFileInternalInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB2_MESSAGE                 pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET                   pSmbResponse
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
SrvGetFileEAInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB2_MESSAGE                 pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET                   pSmbResponse
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
SrvGetFileAccessInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB2_MESSAGE                 pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET                   pSmbResponse
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
SrvGetFilePositionInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB2_MESSAGE                 pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET                   pSmbResponse
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
SrvGetFileFullEAInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB2_MESSAGE                 pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET                   pSmbResponse
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
SrvGetFileModeInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB2_MESSAGE                 pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET                   pSmbResponse
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
SrvGetFileAlignmentInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB2_MESSAGE                 pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET                   pSmbResponse
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
SrvGetFileAllInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB2_MESSAGE                 pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET                   pSmbResponse
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
SrvGetFileAlternateNameInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB2_MESSAGE                 pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET                   pSmbResponse
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
SrvGetFileStreamInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB2_MESSAGE                 pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET                   pSmbResponse
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
SrvGetFileCompressionInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB2_MESSAGE                 pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET                   pSmbResponse
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
SrvGetFileNetworkOpenInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB2_MESSAGE                 pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET                   pSmbResponse
    )
{
    return STATUS_INVALID_PARAMETER;
}

NTSTATUS
SrvGetFileAttributeTagInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB2_MESSAGE                 pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET                   pSmbResponse
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

static
NTSTATUS
SrvGetFileSystemInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB2_MESSAGE                 pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET                   pSmbResponse
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
                            pSmbResponse);

            break;

        case SMB2_FS_INFO_CLASS_SIZE:

            ntStatus = SrvGetFileSystemSizeInfo_SMB_V2(
                            pConnection,
                            pSmbRequest,
                            pFile,
                            pRequestHeader,
                            pSmbResponse);

            break;

        case SMB2_FS_INFO_CLASS_DEVICE:

            ntStatus = SrvGetFileSystemDeviceInfo_SMB_V2(
                            pConnection,
                            pSmbRequest,
                            pFile,
                            pRequestHeader,
                            pSmbResponse);

            break;

        case SMB2_FS_INFO_CLASS_ATTRIBUTE:

            ntStatus = SrvGetFileSystemAttributeInfo_SMB_V2(
                            pConnection,
                            pSmbRequest,
                            pFile,
                            pRequestHeader,
                            pSmbResponse);

            break;

        case SMB2_FS_INFO_CLASS_QUOTA:

            ntStatus = SrvGetFileSystemQuotaInfo_SMB_V2(
                            pConnection,
                            pSmbRequest,
                            pFile,
                            pRequestHeader,
                            pSmbResponse);

            break;

        case SMB2_FS_INFO_CLASS_FULL_SIZE:

            ntStatus = SrvGetFileSystemFullInfo_SMB_V2(
                            pConnection,
                            pSmbRequest,
                            pFile,
                            pRequestHeader,
                            pSmbResponse);

            break;

        case SMB_FS_INFO_CLASS_OBJECTID:

            ntStatus = SrvGetFileSystemObjectIdInfo_SMB_V2(
                            pConnection,
                            pSmbRequest,
                            pFile,
                            pRequestHeader,
                            pSmbResponse);

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
    PSMB2_MESSAGE                 pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET                   pSmbResponse
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
    PBYTE pOutBufferRef = pSmbResponse->pRawBuffer + pSmbResponse->bufferUsed;
    PBYTE pOutBuffer       = pOutBufferRef;
    ULONG ulBytesAvailable = pSmbResponse->bufferLen - pSmbResponse->bufferUsed;
    ULONG ulOffset         = 0;
    ULONG ulBytesUsed      = 0;
    ULONG ulTotalBytesUsed = 0;

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

    ntStatus = SMB2MarshalHeader(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM2_GETINFO,
                    0,
                    1,
                    pSmbRequest->pHeader->ulPid,
                    pSmbRequest->pHeader->ullCommandSequence,
                    pSmbRequest->pHeader->ulTid,
                    pSmbRequest->pHeader->ullSessionId,
                    STATUS_SUCCESS,
                    TRUE,
                    pSmbRequest->pHeader->ulFlags & SMB2_FLAGS_RELATED_OPERATION,
                    NULL,
                    &ulBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    ulTotalBytesUsed += ulBytesUsed;
    pOutBuffer += ulBytesUsed;
    ulOffset += ulBytesUsed;
    ulBytesAvailable -= ulBytesUsed;

    if (ulBytesAvailable < sizeof(SMB2_GET_INFO_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pGetInfoResponseHeader = (PSMB2_GET_INFO_RESPONSE_HEADER)pOutBuffer;

    ulTotalBytesUsed += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulBytesAvailable -= sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    pOutBuffer += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulOffset += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);

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

    pFSVolInfoHeader = (PSMB_FS_VOLUME_INFO_HEADER)pOutBuffer;

    ulTotalBytesUsed += sizeof(SMB_FS_VOLUME_INFO_HEADER);
    ulBytesAvailable -= sizeof(SMB_FS_VOLUME_INFO_HEADER);
    pOutBuffer += sizeof(SMB_FS_VOLUME_INFO_HEADER);

    pFSVolInfoHeader->bSupportsObjects = pFSVolInfo->SupportsObjects;
    pFSVolInfoHeader->llVolumeCreationTime = pFSVolInfo->VolumeCreationTime;
    pFSVolInfoHeader->ulVolumeSerialNumber = pFSVolInfo->VolumeSerialNumber;
    pFSVolInfoHeader->ulVolumeLabelLength = usVolumeLabelLen;

    if (usVolumeLabelLen)
    {
        memcpy(pOutBuffer, (PBYTE)pFSVolInfo->VolumeLabel, usVolumeLabelLen);
    }

    ulTotalBytesUsed += pGetInfoResponseHeader->ulOutBufferLength;

    pSmbResponse->bufferUsed += ulTotalBytesUsed;

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

    if (ulTotalBytesUsed)
    {
        memset(pOutBufferRef, 0, ulTotalBytesUsed);
    }

    goto cleanup;
}

static
NTSTATUS
SrvGetFileSystemSizeInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB2_MESSAGE                 pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET                   pSmbResponse
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

static
NTSTATUS
SrvGetFileSystemDeviceInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB2_MESSAGE                 pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET                   pSmbResponse
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

static
NTSTATUS
SrvGetFileSystemAttributeInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB2_MESSAGE                 pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET                   pSmbResponse
    )
{
    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    PBYTE                       pResponseBuffer = NULL;
    size_t                      sAllocatedSize = 0;
    ULONG                       ulResponseBufferLen = 0;
    IO_STATUS_BLOCK             ioStatusBlock = {0};
    PFILE_FS_ATTRIBUTE_INFORMATION pFSAttrInfo = NULL;
    PSMB2_GET_INFO_RESPONSE_HEADER pGetInfoResponseHeader = NULL;
    PSMB_FS_ATTRIBUTE_INFO_HEADER  pFSAttrInfoHeader = NULL;
    USHORT                         usVolumeLabelLen = 0;
    PBYTE pOutBufferRef = pSmbResponse->pRawBuffer + pSmbResponse->bufferUsed;
    PBYTE pOutBuffer       = pOutBufferRef;
    ULONG ulBytesAvailable = pSmbResponse->bufferLen - pSmbResponse->bufferUsed;
    ULONG ulOffset         = 0;
    ULONG ulBytesUsed      = 0;
    ULONG ulTotalBytesUsed = 0;

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
                            FileFsAttributeInformation);
    BAIL_ON_NT_STATUS(ntStatus);

    ulResponseBufferLen = ioStatusBlock.BytesTransferred;

    ntStatus = SMB2MarshalHeader(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM2_GETINFO,
                    0,
                    1,
                    pSmbRequest->pHeader->ulPid,
                    pSmbRequest->pHeader->ullCommandSequence,
                    pSmbRequest->pHeader->ulTid,
                    pSmbRequest->pHeader->ullSessionId,
                    STATUS_SUCCESS,
                    TRUE,
                    pSmbRequest->pHeader->ulFlags & SMB2_FLAGS_RELATED_OPERATION,
                    NULL,
                    &ulBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    ulTotalBytesUsed += ulBytesUsed;
    pOutBuffer += ulBytesUsed;
    ulOffset += ulBytesUsed;
    ulBytesAvailable -= ulBytesUsed;

    if (ulBytesAvailable < sizeof(SMB2_GET_INFO_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pGetInfoResponseHeader = (PSMB2_GET_INFO_RESPONSE_HEADER)pOutBuffer;

    ulTotalBytesUsed += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulBytesAvailable -= sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    pOutBuffer += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulOffset += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);

    pGetInfoResponseHeader->usLength = sizeof(SMB2_GET_INFO_RESPONSE_HEADER)+1;
    pGetInfoResponseHeader->usOutBufferOffset = ulOffset;

    pFSAttrInfo = (PFILE_FS_ATTRIBUTE_INFORMATION)pResponseBuffer;
    usVolumeLabelLen = wc16slen(pFSAttrInfo->FileSystemName) * sizeof(wchar16_t);

    pGetInfoResponseHeader->ulOutBufferLength = sizeof(SMB_FS_ATTRIBUTE_INFO_HEADER);
    pGetInfoResponseHeader->ulOutBufferLength += usVolumeLabelLen;

    if (ulBytesAvailable < pGetInfoResponseHeader->ulOutBufferLength)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pFSAttrInfoHeader = (PSMB_FS_ATTRIBUTE_INFO_HEADER)pOutBuffer;

    ulTotalBytesUsed += sizeof(SMB_FS_ATTRIBUTE_INFO_HEADER);
    ulBytesAvailable -= sizeof(SMB_FS_ATTRIBUTE_INFO_HEADER);
    pOutBuffer += sizeof(SMB_FS_ATTRIBUTE_INFO_HEADER);

    pFSAttrInfoHeader->ulFSAttributes = pFSAttrInfo->FileSystemAttributes;
    pFSAttrInfoHeader->lMaxFilenameLen = pFSAttrInfo->MaximumComponentNameLength;
    pFSAttrInfoHeader->ulFileSystemNameLen = usVolumeLabelLen;

    if (usVolumeLabelLen)
    {
        memcpy(pOutBuffer, (PBYTE)pFSAttrInfo->FileSystemName, usVolumeLabelLen);
    }

    ulTotalBytesUsed += pGetInfoResponseHeader->ulOutBufferLength;

    pSmbResponse->bufferUsed += ulTotalBytesUsed;

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

    if (ulTotalBytesUsed)
    {
        memset(pOutBufferRef, 0, ulTotalBytesUsed);
    }

    goto cleanup;
}

static
NTSTATUS
SrvGetFileSystemQuotaInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB2_MESSAGE                 pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET                   pSmbResponse
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

static
NTSTATUS
SrvGetFileSystemFullInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB2_MESSAGE                 pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET                   pSmbResponse
    )
{
    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    IO_STATUS_BLOCK             ioStatusBlock = {0};
    FILE_FS_SIZE_INFORMATION    fSSizeInfo = {0};
    PSMB2_GET_INFO_RESPONSE_HEADER pGetInfoResponseHeader = NULL;
    PSMB_FS_FULL_INFO_HEADER       pFSFullInfoHeader = NULL;
    PBYTE pOutBufferRef = pSmbResponse->pRawBuffer + pSmbResponse->bufferUsed;
    PBYTE pOutBuffer       = pOutBufferRef;
    ULONG ulBytesAvailable = pSmbResponse->bufferLen - pSmbResponse->bufferUsed;
    ULONG ulOffset         = 0;
    ULONG ulBytesUsed      = 0;
    ULONG ulTotalBytesUsed = 0;

    ntStatus = IoQueryVolumeInformationFile(
                        pFile->hFile,
                        NULL,
                        &ioStatusBlock,
                        &fSSizeInfo,
                        sizeof(fSSizeInfo),
                        FileFsSizeInformation);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMB2MarshalHeader(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM2_GETINFO,
                    0,
                    1,
                    pSmbRequest->pHeader->ulPid,
                    pSmbRequest->pHeader->ullCommandSequence,
                    pSmbRequest->pHeader->ulTid,
                    pSmbRequest->pHeader->ullSessionId,
                    STATUS_SUCCESS,
                    TRUE,
                    pSmbRequest->pHeader->ulFlags & SMB2_FLAGS_RELATED_OPERATION,
                    NULL,
                    &ulBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    ulTotalBytesUsed += ulBytesUsed;
    pOutBuffer += ulBytesUsed;
    ulOffset += ulBytesUsed;
    ulBytesAvailable -= ulBytesUsed;

    if (ulBytesAvailable < sizeof(SMB2_GET_INFO_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pGetInfoResponseHeader = (PSMB2_GET_INFO_RESPONSE_HEADER)pOutBuffer;

    ulTotalBytesUsed += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulBytesAvailable -= sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    pOutBuffer += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulOffset += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);

    pGetInfoResponseHeader->usLength = sizeof(SMB2_GET_INFO_RESPONSE_HEADER)+1;
    pGetInfoResponseHeader->usOutBufferOffset = ulOffset;

    pGetInfoResponseHeader->ulOutBufferLength = sizeof(SMB_FS_FULL_INFO_HEADER);

    if (ulBytesAvailable < pGetInfoResponseHeader->ulOutBufferLength)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pFSFullInfoHeader = (PSMB_FS_FULL_INFO_HEADER)pOutBuffer;

    ulTotalBytesUsed += sizeof(SMB_FS_FULL_INFO_HEADER);
    ulBytesAvailable -= sizeof(SMB_FS_FULL_INFO_HEADER);
    pOutBuffer += sizeof(SMB_FS_FULL_INFO_HEADER);

    // TODO: Fill in the AllocationSize
    pFSFullInfoHeader->ullAllocationSize = 0;
    pFSFullInfoHeader->ullAvailableAllocationUnits = fSSizeInfo.AvailableAllocationUnits;
    pFSFullInfoHeader->ullTotalAllocationUnits = fSSizeInfo.TotalAllocationUnits;
    pFSFullInfoHeader->ulSectorsPerAllocationUnit = fSSizeInfo.SectorsPerAllocationUnit;
    pFSFullInfoHeader->ulBytesPerSector = fSSizeInfo.BytesPerSector;

    ulTotalBytesUsed += pGetInfoResponseHeader->ulOutBufferLength;

    pSmbResponse->bufferUsed += ulTotalBytesUsed;

cleanup:

    return ntStatus;

error:

    if (ulTotalBytesUsed)
    {
        memset(pOutBufferRef, 0, ulTotalBytesUsed);
    }

    goto cleanup;
}

static
NTSTATUS
SrvGetFileSystemObjectIdInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB2_MESSAGE                 pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET                   pSmbResponse
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

static
NTSTATUS
SrvGetSecurityInfo_SMB_V2(
    PLWIO_SRV_CONNECTION          pConnection,
    PSMB2_MESSAGE                 pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET                   pSmbResponse
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
                            pSmbResponse);

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
    PSMB2_MESSAGE                 pSmbRequest,
    PLWIO_SRV_FILE_2              pFile,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PSMB_PACKET                   pSmbResponse
    )
{
    return STATUS_NOT_IMPLEMENTED;
}
