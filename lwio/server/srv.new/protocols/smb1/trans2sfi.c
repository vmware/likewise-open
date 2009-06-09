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
SrvUnmarshallSetFileInfoParams(
    PBYTE            pParams,
    USHORT           ulBytesAvailable,
    PUSHORT          pusFid,
    PSMB_INFO_LEVEL* ppSmbInfoLevel
    );

static
NTSTATUS
SrvBuildSetFileInfoResponse(
    PLWIO_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    USHORT              usFid,
    SMB_INFO_LEVEL      smbInfoLevel,
    PBYTE               pData,
    USHORT              usDataLen,
    PSMB_PACKET*        ppSmbResponse
    );

static
NTSTATUS
SrvBuildSetFileBasicInfoResponse(
    PLWIO_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    PLWIO_SRV_FILE       pFile,
    PBYTE               pData,
    USHORT              usDataLen,
    PSMB_PACKET*        ppSmbResponse
    );

static
NTSTATUS
SrvBuildSetFileDispositionInfoResponse(
    PLWIO_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    PLWIO_SRV_FILE       pFile,
    PBYTE               pData,
    USHORT              usDataLen,
    PSMB_PACKET*        ppSmbResponse
    );

static
NTSTATUS
SrvBuildSetFileAllocationInfoResponse(
    PLWIO_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    PLWIO_SRV_FILE       pFile,
    PBYTE               pData,
    USHORT              usDataLen,
    PSMB_PACKET*        ppSmbResponse
    );

static
NTSTATUS
SrvBuildSetEndOfFileResponse(
    PLWIO_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    PLWIO_SRV_FILE       pFile,
    PBYTE               pData,
    USHORT              usDataLen,
    PSMB_PACKET*        ppSmbResponse
    );

NTSTATUS
SrvProcessTrans2SetFileInformation(
    PLWIO_SRV_CONNECTION         pConnection,
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
    USHORT usFid = 0;
    PSMB_INFO_LEVEL pSmbInfoLevel = NULL; // Do not free
    PSMB_PACKET pSmbResponse = NULL;

    ntStatus = SrvUnmarshallSetFileInfoParams(
                    pParameters,
                    pRequestHeader->parameterCount,
                    &usFid,
                    &pSmbInfoLevel);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvBuildSetFileInfoResponse(
                    pConnection,
                    pSmbRequest,
                    usFid,
                    *pSmbInfoLevel,
                    pData,
                    pRequestHeader->dataCount,
                    &pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppSmbResponse = pSmbResponse;

cleanup:

    return ntStatus;

error:

    *ppSmbResponse = NULL;

    if (pSmbResponse)
    {
        SMBPacketFree(
            SrvTransportGetAllocator(pConnection),
            pSmbResponse);
    }

    goto cleanup;
}

static
NTSTATUS
SrvUnmarshallSetFileInfoParams(
    PBYTE            pParams,
    USHORT           ulBytesAvailable,
    PUSHORT          pusFid,
    PSMB_INFO_LEVEL* ppSmbInfoLevel
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_INFO_LEVEL pSmbInfoLevel = NULL;
    USHORT   usFid = 0;
    PBYTE    pDataCursor = pParams;

    // FID
    if (ulBytesAvailable < sizeof(USHORT))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    usFid = *((PUSHORT)pDataCursor);
    pDataCursor += sizeof(USHORT);
    ulBytesAvailable -= sizeof(USHORT);

    // Info level
    if (ulBytesAvailable < sizeof(SMB_INFO_LEVEL))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pSmbInfoLevel = (PSMB_INFO_LEVEL)pDataCursor;
    pDataCursor += sizeof(SMB_INFO_LEVEL);
    ulBytesAvailable -= sizeof(SMB_INFO_LEVEL);

    *pusFid = usFid;
    *ppSmbInfoLevel = pSmbInfoLevel;

cleanup:

    return ntStatus;

error:

    *pusFid = 0;
    *ppSmbInfoLevel = NULL;

    goto cleanup;
}

static
NTSTATUS
SrvBuildSetFileInfoResponse(
    PLWIO_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    USHORT              usFid,
    SMB_INFO_LEVEL      smbInfoLevel,
    PBYTE               pData,
    USHORT              usDataLen,
    PSMB_PACKET*        ppSmbResponse
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_SESSION pSession = NULL;
    PLWIO_SRV_TREE    pTree = NULL;
    PLWIO_SRV_FILE    pFile = NULL;
    PSMB_PACKET      pSmbResponse = NULL;

    ntStatus = SrvConnectionFindSession(
                    pConnection,
                    pSmbRequest->pSMBHeader->uid,
                    &pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvSessionFindTree(
                    pSession,
                    pSmbRequest->pSMBHeader->tid,
                    &pTree);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvTreeFindFile(
                    pTree,
                    usFid,
                    &pFile);
    BAIL_ON_NT_STATUS(ntStatus);

    switch (smbInfoLevel)
    {
        case SMB_INFO_STANDARD :
        case SMB_INFO_QUERY_EA_SIZE :
        case SMB_SET_FILE_UNIX_BASIC :
        case SMB_SET_FILE_UNIX_LINK :
        case SMB_SET_FILE_UNIX_HLINK :

            ntStatus = STATUS_NOT_IMPLEMENTED;

            break;

        case SMB_SET_FILE_BASIC_INFO :

            ntStatus = SrvBuildSetFileBasicInfoResponse(
                            pConnection,
                            pSmbRequest,
                            pFile,
                            pData,
                            usDataLen,
                            &pSmbResponse);

            break;

        case SMB_SET_FILE_DISPOSITION_INFO :

            ntStatus = SrvBuildSetFileDispositionInfoResponse(
                            pConnection,
                            pSmbRequest,
                            pFile,
                            pData,
                            usDataLen,
                            &pSmbResponse);

            break;

        case SMB_SET_FILE_ALLOCATION_INFO :

            ntStatus = SrvBuildSetFileAllocationInfoResponse(
                            pConnection,
                            pSmbRequest,
                            pFile,
                            pData,
                            usDataLen,
                            &pSmbResponse);

            break;

        case SMB_SET_FILE_END_OF_FILE_INFO :

            ntStatus = SrvBuildSetEndOfFileResponse(
                            pConnection,
                            pSmbRequest,
                            pFile,
                            pData,
                            usDataLen,
                            &pSmbResponse);

            break;

        default:

            ntStatus = STATUS_DATA_ERROR;

            break;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    *ppSmbResponse = pSmbResponse;

cleanup:

    if (pFile)
    {
        SrvFileRelease(pFile);
    }

    if (pTree)
    {
        SrvTreeRelease(pTree);
    }

    if (pSession)
    {
        SrvSessionRelease(pSession);
    }

    return ntStatus;

error:

    *ppSmbResponse = NULL;

    if (pSmbResponse)
    {
        SMBPacketFree(
            SrvTransportGetAllocator(pConnection),
            pSmbResponse);
    }

    goto cleanup;
}

static
NTSTATUS
SrvBuildSetFileBasicInfoResponse(
    PLWIO_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    PLWIO_SRV_FILE       pFile,
    PBYTE               pData,
    USHORT              usDataLen,
    PSMB_PACKET*        ppSmbResponse
    )
{
    NTSTATUS ntStatus = 0;
    PUSHORT  pSetup = NULL;
    BYTE     setupCount = 0;
    USHORT   usParams = 0;
    USHORT   usDataOffset = 0;
    USHORT   usParameterOffset = 0;
    USHORT   usNumPackageBytesUsed = 0;
    PFILE_BASIC_INFORMATION pFileBasicInfo = NULL;
    IO_STATUS_BLOCK ioStatusBlock = {0};
    PSMB_PACKET pSmbResponse = NULL;

    if (usDataLen < sizeof(FILE_BASIC_INFORMATION))
    {
        ntStatus = STATUS_DATA_ERROR;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pFileBasicInfo = (PFILE_BASIC_INFORMATION)pData;

    ntStatus = IoSetInformationFile(
                    pFile->hFile,
                    NULL,
                    &ioStatusBlock,
                    pFileBasicInfo,
                    sizeof(FILE_BASIC_INFORMATION),
                    FileBasicInformation);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketAllocate(
                    SrvTransportGetAllocator(pConnection),
                    &pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketBufferAllocate(
                    SrvTransportGetAllocator(pConnection),
                    64 * 1024,
                    &pSmbResponse->pRawBuffer,
                    &pSmbResponse->bufferLen);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketMarshallHeader(
                pSmbResponse->pRawBuffer,
                pSmbResponse->bufferLen,
                COM_TRANSACTION2,
                0,
                TRUE,
                pSmbRequest->pSMBHeader->tid,
                pSmbRequest->pSMBHeader->pid,
                pSmbRequest->pSMBHeader->uid,
                pSmbRequest->pSMBHeader->mid,
                pConnection->serverProperties.bRequireSecuritySignatures,
                pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    pSmbResponse->pSMBHeader->wordCount = 10 + setupCount;

    ntStatus = WireMarshallTransaction2Response(
                    pSmbResponse->pParams,
                    pSmbResponse->bufferLen - pSmbResponse->bufferUsed,
                    (PBYTE)pSmbResponse->pParams - (PBYTE)pSmbResponse->pSMBHeader,
                    pSetup,
                    setupCount,
                    (PBYTE)&usParams,
                    sizeof(usParams),
                    NULL,
                    0,
                    &usDataOffset,
                    &usParameterOffset,
                    &usNumPackageBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    pSmbResponse->bufferUsed += usNumPackageBytesUsed;

    ntStatus = SMBPacketMarshallFooter(pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppSmbResponse = pSmbResponse;

cleanup:

    return ntStatus;

error:

    *ppSmbResponse = NULL;

    if (pSmbResponse)
    {
        SMBPacketFree(
            SrvTransportGetAllocator(pConnection),
            pSmbResponse);
    }

    goto cleanup;
}

static
NTSTATUS
SrvBuildSetFileDispositionInfoResponse(
    PLWIO_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    PLWIO_SRV_FILE       pFile,
    PBYTE               pData,
    USHORT              usDataLen,
    PSMB_PACKET*        ppSmbResponse
    )
{
    NTSTATUS ntStatus = 0;
    PUSHORT  pSetup = NULL;
    BYTE     setupCount = 0;
    USHORT   usParams = 0;
    USHORT   usDataOffset = 0;
    USHORT   usParameterOffset = 0;
    USHORT   usNumPackageBytesUsed = 0;
    PFILE_DISPOSITION_INFORMATION pFileDispositionInfo = NULL;
    IO_STATUS_BLOCK ioStatusBlock = {0};
    PSMB_PACKET pSmbResponse = NULL;

    if (usDataLen < sizeof(FILE_DISPOSITION_INFORMATION))
    {
        ntStatus = STATUS_DATA_ERROR;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pFileDispositionInfo = (PFILE_DISPOSITION_INFORMATION)pData;

    ntStatus = IoSetInformationFile(
                    pFile->hFile,
                    NULL,
                    &ioStatusBlock,
                    pFileDispositionInfo,
                    sizeof(FILE_DISPOSITION_INFORMATION),
                    FileDispositionInformation);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketAllocate(
                    SrvTransportGetAllocator(pConnection),
                    &pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketBufferAllocate(
                    SrvTransportGetAllocator(pConnection),
                    64 * 1024,
                    &pSmbResponse->pRawBuffer,
                    &pSmbResponse->bufferLen);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketMarshallHeader(
                pSmbResponse->pRawBuffer,
                pSmbResponse->bufferLen,
                COM_TRANSACTION2,
                0,
                TRUE,
                pSmbRequest->pSMBHeader->tid,
                pSmbRequest->pSMBHeader->pid,
                pSmbRequest->pSMBHeader->uid,
                pSmbRequest->pSMBHeader->mid,
                pConnection->serverProperties.bRequireSecuritySignatures,
                pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    pSmbResponse->pSMBHeader->wordCount = 10 + setupCount;

    ntStatus = WireMarshallTransaction2Response(
                    pSmbResponse->pParams,
                    pSmbResponse->bufferLen - pSmbResponse->bufferUsed,
                    (PBYTE)pSmbResponse->pParams - (PBYTE)pSmbResponse->pSMBHeader,
                    pSetup,
                    setupCount,
                    (PBYTE)&usParams,
                    sizeof(usParams),
                    NULL,
                    0,
                    &usDataOffset,
                    &usParameterOffset,
                    &usNumPackageBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    pSmbResponse->bufferUsed += usNumPackageBytesUsed;

    ntStatus = SMBPacketMarshallFooter(pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppSmbResponse = pSmbResponse;

cleanup:

    return ntStatus;

error:

    *ppSmbResponse = NULL;

    if (pSmbResponse)
    {
        SMBPacketFree(
            SrvTransportGetAllocator(pConnection),
            pSmbResponse);
    }

    goto cleanup;
}

static
NTSTATUS
SrvBuildSetFileAllocationInfoResponse(
    PLWIO_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    PLWIO_SRV_FILE       pFile,
    PBYTE               pData,
    USHORT              usDataLen,
    PSMB_PACKET*        ppSmbResponse
    )
{
    NTSTATUS ntStatus = 0;
    PUSHORT  pSetup = NULL;
    BYTE     setupCount = 0;
    USHORT   usParams = 0;
    USHORT   usDataOffset = 0;
    USHORT   usParameterOffset = 0;
    USHORT   usNumPackageBytesUsed = 0;
    PFILE_ALLOCATION_INFORMATION pFileAllocationInfo = NULL;
    IO_STATUS_BLOCK ioStatusBlock = {0};
    PSMB_PACKET pSmbResponse = NULL;

    if (usDataLen < sizeof(FILE_ALLOCATION_INFORMATION))
    {
        ntStatus = STATUS_DATA_ERROR;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pFileAllocationInfo = (PFILE_ALLOCATION_INFORMATION)pData;

    ntStatus = IoSetInformationFile(
                    pFile->hFile,
                    NULL,
                    &ioStatusBlock,
                    pFileAllocationInfo,
                    sizeof(FILE_ALLOCATION_INFORMATION),
                    FileAllocationInformation);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketAllocate(
                    SrvTransportGetAllocator(pConnection),
                    &pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketBufferAllocate(
                    SrvTransportGetAllocator(pConnection),
                    64 * 1024,
                    &pSmbResponse->pRawBuffer,
                    &pSmbResponse->bufferLen);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketMarshallHeader(
                pSmbResponse->pRawBuffer,
                pSmbResponse->bufferLen,
                COM_TRANSACTION2,
                0,
                TRUE,
                pSmbRequest->pSMBHeader->tid,
                pSmbRequest->pSMBHeader->pid,
                pSmbRequest->pSMBHeader->uid,
                pSmbRequest->pSMBHeader->mid,
                pConnection->serverProperties.bRequireSecuritySignatures,
                pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    pSmbResponse->pSMBHeader->wordCount = 10 + setupCount;

    ntStatus = WireMarshallTransaction2Response(
                    pSmbResponse->pParams,
                    pSmbResponse->bufferLen - pSmbResponse->bufferUsed,
                    (PBYTE)pSmbResponse->pParams - (PBYTE)pSmbResponse->pSMBHeader,
                    pSetup,
                    setupCount,
                    (PBYTE)&usParams,
                    sizeof(usParams),
                    NULL,
                    0,
                    &usDataOffset,
                    &usParameterOffset,
                    &usNumPackageBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    pSmbResponse->bufferUsed += usNumPackageBytesUsed;

    ntStatus = SMBPacketMarshallFooter(pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppSmbResponse = pSmbResponse;

cleanup:

    return ntStatus;

error:

    *ppSmbResponse = NULL;

    if (pSmbResponse)
    {
        SMBPacketFree(
            SrvTransportGetAllocator(pConnection),
            pSmbResponse);
    }

    goto cleanup;
}

static
NTSTATUS
SrvBuildSetEndOfFileResponse(
    PLWIO_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    PLWIO_SRV_FILE       pFile,
    PBYTE               pData,
    USHORT              usDataLen,
    PSMB_PACKET*        ppSmbResponse
    )
{
    NTSTATUS ntStatus = 0;
    PUSHORT  pSetup = NULL;
    BYTE     setupCount = 0;
    USHORT   usParams = 0;
    USHORT   usDataOffset = 0;
    USHORT   usParameterOffset = 0;
    USHORT   usNumPackageBytesUsed = 0;
    PFILE_END_OF_FILE_INFORMATION pFileEofInfo = NULL;
    IO_STATUS_BLOCK ioStatusBlock = {0};
    PSMB_PACKET pSmbResponse = NULL;

    if (usDataLen < sizeof(FILE_END_OF_FILE_INFORMATION))
    {
        ntStatus = STATUS_DATA_ERROR;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pFileEofInfo = (PFILE_END_OF_FILE_INFORMATION)pData;

    ntStatus = IoSetInformationFile(
                    pFile->hFile,
                    NULL,
                    &ioStatusBlock,
                    pFileEofInfo,
                    sizeof(FILE_END_OF_FILE_INFORMATION),
                    FileEndOfFileInformation);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketAllocate(
                    SrvTransportGetAllocator(pConnection),
                    &pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketBufferAllocate(
                    SrvTransportGetAllocator(pConnection),
                    64 * 1024,
                    &pSmbResponse->pRawBuffer,
                    &pSmbResponse->bufferLen);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketMarshallHeader(
                pSmbResponse->pRawBuffer,
                pSmbResponse->bufferLen,
                COM_TRANSACTION2,
                0,
                TRUE,
                pSmbRequest->pSMBHeader->tid,
                pSmbRequest->pSMBHeader->pid,
                pSmbRequest->pSMBHeader->uid,
                pSmbRequest->pSMBHeader->mid,
                pConnection->serverProperties.bRequireSecuritySignatures,
                pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    pSmbResponse->pSMBHeader->wordCount = 10 + setupCount;

    ntStatus = WireMarshallTransaction2Response(
                    pSmbResponse->pParams,
                    pSmbResponse->bufferLen - pSmbResponse->bufferUsed,
                    (PBYTE)pSmbResponse->pParams - (PBYTE)pSmbResponse->pSMBHeader,
                    pSetup,
                    setupCount,
                    (PBYTE)&usParams,
                    sizeof(usParams),
                    NULL,
                    0,
                    &usDataOffset,
                    &usParameterOffset,
                    &usNumPackageBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    pSmbResponse->bufferUsed += usNumPackageBytesUsed;

    ntStatus = SMBPacketMarshallFooter(pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppSmbResponse = pSmbResponse;

cleanup:

    return ntStatus;

error:

    *ppSmbResponse = NULL;

    if (pSmbResponse)
    {
        SMBPacketFree(
            SrvTransportGetAllocator(pConnection),
            pSmbResponse);
    }

    goto cleanup;
}
