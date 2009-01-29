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
SrvBuildFilePath(
    PWSTR          pwszPrefix,
    PWSTR          pwszSuffix,
    PIO_FILE_NAME* ppFilename
    );

static
NTSTATUS
SrvBuildNTCreateResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    PIO_STATUS_BLOCK    pIoStatusBlock,
    PSMB_SRV_FILE       pFile,
    PSMB_PACKET*        ppSmbResponse
    );

NTSTATUS
SrvProcessNTCreateAndX(
    PLWIO_SRV_CONTEXT pContext
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_SRV_CONNECTION pConnection = pContext->pConnection;
    PSMB_PACKET         pSmbRequest = pContext->pRequest;
    PSMB_PACKET         pSmbResponse = NULL;
    PSMB_SRV_SESSION    pSession = NULL;
    PSMB_SRV_TREE       pTree = NULL;
    PSMB_SRV_FILE       pFile = NULL;
    PCREATE_REQUEST_HEADER pRequestHeader = NULL; // Do not free
    PWSTR               pwszFilename = NULL; // Do not free
    IO_FILE_HANDLE      hFile = NULL;
    PIO_ASYNC_CONTROL_BLOCK pAsyncControlBlock = NULL;
    IO_STATUS_BLOCK     ioStatusBlock = {0};
    PIO_CREATE_SECURITY_CONTEXT pSecurityContext = NULL;
    PVOID               pSecurityDescriptor = NULL;
    PVOID               pSecurityQOS = NULL;
    PIO_FILE_NAME       pFilename = NULL;

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

    ntStatus = WireUnmarshallCreateFileRequest(
                    pSmbRequest->pParams,
                    pSmbRequest->bufferLen - pSmbRequest->bufferUsed,
                    (PBYTE)pSmbRequest->pParams - (PBYTE)pSmbRequest->pSMBHeader,
                    &pRequestHeader,
                    &pwszFilename);
    BAIL_ON_NT_STATUS(ntStatus);

    // TODO: Handle root fids
    ntStatus = SrvBuildFilePath(
                    pTree->pShareInfo->pwszPath,
                    pwszFilename,
                    &pFilename);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = IoCreateFile(
                    &hFile,
                    pAsyncControlBlock,
                    &ioStatusBlock,
                    pSecurityContext,
                    pFilename,
                    pSecurityDescriptor,
                    pSecurityQOS,
                    pRequestHeader->desiredAccess,
                    pRequestHeader->allocationSize,
                    pRequestHeader->extFileAttributes,
                    pRequestHeader->shareAccess,
                    pRequestHeader->createDisposition,
                    pRequestHeader->createOptions,
                    NULL, /* EA Buffer */
                    0,    /* EA Length */
                    NULL  /* ECP List  */
                    );
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = ioStatusBlock.Status;
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvTreeCreateFile(
                    pTree,
                    &hFile,
                    &pFilename,
                    pRequestHeader->desiredAccess,
                    pRequestHeader->allocationSize,
                    pRequestHeader->extFileAttributes,
                    pRequestHeader->shareAccess,
                    pRequestHeader->createDisposition,
                    pRequestHeader->createOptions,
                    &pFile);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvBuildNTCreateResponse(
                    pConnection,
                    pSmbRequest,
                    &ioStatusBlock,
                    pFile,
                    &pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvConnectionWriteMessage(
                    pConnection,
                    pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

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

    if (pSmbResponse)
    {
        SMBPacketFree(
            pConnection->hPacketAllocator,
            pSmbResponse);
    }

    return (ntStatus);

error:

    if (pFilename)
    {
        SMB_SAFE_FREE_MEMORY(pFilename->FileName);
        SMBFreeMemory(pFilename);
    }

    if (hFile)
    {
        IoCloseFile(hFile);
    }

    goto cleanup;
}

static
NTSTATUS
SrvBuildFilePath(
    PWSTR          pwszPrefix,
    PWSTR          pwszSuffix,
    PIO_FILE_NAME* ppFilename
    )
{
    NTSTATUS ntStatus = 0;
    size_t              len_prefix = 0;
    size_t              len_suffix = 0;
    size_t              len_separator = 0;
    PWSTR               pDataCursor = NULL;
    PIO_FILE_NAME       pFilename = NULL;
    wchar16_t           wszFwdSlash;
    wchar16_t           wszBackSlash;

    wcstowc16s(&wszFwdSlash, L"/", 1);
    wcstowc16s(&wszBackSlash, L"\\", 1);

    if (*pwszSuffix && (*pwszSuffix != wszFwdSlash) && (*pwszSuffix != wszBackSlash))
    {
#ifdef _WIN32
        len_separator = sizeof(wszBackSlash);
#else
        len_separator = sizeof(wszFwdSlash);
#endif
    }

    ntStatus = SMBAllocateMemory(
                    sizeof(IO_FILE_NAME),
                    (PVOID*)&pFilename);
    BAIL_ON_NT_STATUS(ntStatus);

    len_prefix = wc16slen(pwszPrefix);
    len_suffix = wc16slen(pwszSuffix);

    ntStatus = SMBAllocateMemory(
                    (len_prefix + len_suffix + len_separator + 1 ) * sizeof(wchar16_t),
                    (PVOID*)&pFilename->FileName);
    BAIL_ON_NT_STATUS(ntStatus);

    pDataCursor = pFilename->FileName;
    while (pwszPrefix && *pwszPrefix)
    {
        *pDataCursor++ = *pwszPrefix++;
    }

    if (len_separator)
    {
#ifdef _WIN32
        *pDataCursor++ = wszBackSlash;
#else
        *pDataCursor++ = wszFwdSlash;
#endif
    }

    while (pwszSuffix && *pwszSuffix)
    {
        *pDataCursor++ = *pwszSuffix++;
    }

    pDataCursor = pFilename->FileName;
    while (pDataCursor && *pDataCursor)
    {
#ifdef _WIN32
        if (*pDataCursor == wszFwdSlash)
        {
            *pDataCursor = wszBackSlash;
        }
#else
        if (*pDataCursor == wszBackSlash)
        {
            *pDataCursor = wszFwdSlash;
        }
#endif
        pDataCursor++;
    }

    *ppFilename = pFilename;

cleanup:

    return ntStatus;

error:

    *ppFilename = NULL;

    if (pFilename)
    {
        SMB_SAFE_FREE_MEMORY(pFilename->FileName);
        SMBFreeMemory(pFilename);
    }

    goto cleanup;
}

static
NTSTATUS
SrvBuildNTCreateResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    PIO_STATUS_BLOCK    pIoStatusBlock,
    PSMB_SRV_FILE       pFile,
    PSMB_PACKET*        ppSmbResponse
    )
{
    NTSTATUS    ntStatus = 0;
    PSMB_PACKET pSmbResponse = NULL;
    PCREATE_RESPONSE_HEADER pResponseHeader = NULL;

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

    ntStatus = SMBPacketMarshallHeader(
                pSmbResponse->pRawBuffer,
                pSmbResponse->bufferLen,
                COM_NT_CREATE_ANDX,
                0,
                TRUE,
                pSmbRequest->pSMBHeader->tid,
                getpid(),
                pSmbRequest->pSMBHeader->uid,
                pSmbRequest->pSMBHeader->mid,
                pConnection->serverProperties.bRequireSecuritySignatures,
                pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    pSmbResponse->pSMBHeader->wordCount = 26;

    pResponseHeader = (PCREATE_RESPONSE_HEADER)pSmbResponse->pParams;
    pSmbResponse->pData = pSmbResponse->pParams + sizeof(CREATE_RESPONSE_HEADER);
    pSmbResponse->bufferUsed += sizeof(CREATE_RESPONSE_HEADER);

    pResponseHeader->fid = pFile->fid;
    pResponseHeader->createAction = pIoStatusBlock->CreateResult;
    // TODO: Fill in other file attributes

    pSmbResponse->pByteCount = &pResponseHeader->byteCount;
    *pSmbResponse->pByteCount = 0;

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
            pConnection->hPacketAllocator,
            pSmbResponse);
    }

    goto cleanup;
}
