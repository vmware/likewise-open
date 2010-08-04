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
 *        SMB Client Redirector
 *
 *        Create Dispatch Routine
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Kaya Bekiroglu (kaya@likewisesoftware.com)
 *          Brian Koropoff (bkoropoff@likewise.com)
 */

#include "rdr.h"

static
NTSTATUS
ParseSharePath(
    PCWSTR pwszPath,
    PWSTR*  ppwszServer,
    PSTR*   ppszShare,
    PSTR*   ppszFilename
    );

static
BOOLEAN
RdrFinishCreate(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    );

static
NTSTATUS
RdrTransceiveCreate(
    PRDR_OP_CONTEXT pContext,
    PSMB_CLIENT_FILE_HANDLE pFile,
    PCWSTR pwszPath,
    ACCESS_MASK desiredAccess,
    LONG64 llAllocationSize,
    FILE_ATTRIBUTES fileAttributes,
    FILE_SHARE_FLAGS shareAccess,
    FILE_CREATE_DISPOSITION createDisposition,
    FILE_CREATE_OPTIONS createOptions
    );

static
void
RdrCancelCreate(
    PIRP pIrp,
    PVOID _pContext
    )
{
    return;
}

static
NTSTATUS
RdrCreateTreeConnect(
    PRDR_OP_CONTEXT pContext
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PIRP pIrp = pContext->pIrp;
    PIO_FILE_NAME pFileName = &pIrp->Args.Create.FileName;
    PIO_CREDS pCreds = IoSecurityGetCredentials(pIrp->Args.Create.SecurityContext);
    PIO_SECURITY_CONTEXT_PROCESS_INFORMATION pProcessInfo =
        IoSecurityGetProcessInfo(pIrp->Args.Create.SecurityContext);
    PSMB_TREE pTree = NULL;
    PWSTR pwszServer = NULL;
    PSTR pszShare = NULL;
    PSTR pszFilename = NULL;

    status = ParseSharePath(
        pFileName->FileName,
        &pwszServer,
        &pszShare,
        &pszFilename);
    BAIL_ON_NT_STATUS(status);

    status = LwRtlWC16StringAllocateFromCString(
        &pContext->State.Create.pwszFilename,
        pszFilename);
    BAIL_ON_NT_STATUS(status);

    status = RdrTreeConnect(
        pwszServer,
        pszShare,
        pCreds,
        pProcessInfo->Uid,
        pContext);
    BAIL_ON_NT_STATUS(status);

cleanup:

    RTL_FREE(&pwszServer);
    RTL_FREE(&pszShare);
    RTL_FREE(&pszFilename);

    if (status != STATUS_PENDING)
    {
        RdrContinueContext(pContext, status, pTree);
    }

    return status;

error:

    if (status != STATUS_PENDING && pTree)
    {
        SMBTreeRelease(pTree);
        pTree = NULL;
    }

    goto cleanup;
}

static
BOOLEAN
RdrCreateTreeConnected(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    )
{
    PSMB_TREE pTree = pParam;
    PIRP pIrp = pContext->pIrp;
    ACCESS_MASK DesiredAccess = pIrp->Args.Create.DesiredAccess;
    LONG64 AllocationSize = pIrp->Args.Create.AllocationSize;
    FILE_SHARE_FLAGS ShareAccess = pIrp->Args.Create.ShareAccess;
    FILE_CREATE_DISPOSITION CreateDisposition = pIrp->Args.Create.CreateDisposition;
    FILE_CREATE_OPTIONS CreateOptions = pIrp->Args.Create.CreateOptions;
    FILE_ATTRIBUTES FileAttributes =  pIrp->Args.Create.FileAttributes;
    PSMB_CLIENT_FILE_HANDLE pFile = NULL;

    BAIL_ON_NT_STATUS(status);

    status = SMBAllocateMemory(
        sizeof(SMB_CLIENT_FILE_HANDLE),
        (PVOID*)&pFile);
    BAIL_ON_NT_STATUS(status);

    status = LwErrnoToNtStatus(pthread_mutex_init(&pFile->mutex, NULL));
    BAIL_ON_NT_STATUS(status);

    pFile->pMutex = &pFile->mutex;
    pFile->pTree = pTree;

    pContext->Continue = RdrFinishCreate;

    pContext->State.Create.pFile = pFile;

    status = RdrTransceiveCreate(
        pContext,
        pFile,
        pContext->State.Create.pwszFilename,
        DesiredAccess,
        AllocationSize,
        FileAttributes,
        ShareAccess,
        CreateDisposition,
        CreateOptions);
    BAIL_ON_NT_STATUS(status);


cleanup:

    RTL_FREE(&pContext->State.Create.pwszFilename);

    if (status != STATUS_PENDING)
    {
        RdrFreeContext(pContext);
        pIrp->IoStatusBlock.Status = status;
        IoIrpComplete(pIrp);
    }

    return FALSE;

error:

    if (status != STATUS_PENDING && pFile)
    {
        RdrReleaseFile(pFile);
    }

    goto cleanup;
}

static
BOOLEAN
RdrFinishCreate(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    )
{
    PSMB_CLIENT_FILE_HANDLE pFile = pContext->State.Create.pFile;
    PSMB_PACKET pPacket = pParam;
    PCREATE_RESPONSE_HEADER pResponseHeader = NULL;

    BAIL_ON_NT_STATUS(status);

    status = pPacket->pSMBHeader->error;
    BAIL_ON_NT_STATUS(status);

    status = WireUnmarshallSMBResponseCreate(
                pPacket->pParams,
                pPacket->bufferLen - pPacket->bufferUsed,
                &pResponseHeader);
    BAIL_ON_NT_STATUS(status);

    pFile->fid = pResponseHeader->fid;
    pFile->usFileType = pResponseHeader->fileType;

    status = IoFileSetContext(pContext->pIrp->FileHandle, pFile);
    BAIL_ON_NT_STATUS(status);

cleanup:

    if (pPacket)
    {
        SMBPacketRelease(
            pFile->pTree->pSession->pSocket->hPacketAllocator,
            pPacket);
    }

    pContext->pIrp->IoStatusBlock.Status = status;
    IoIrpComplete(pContext->pIrp);
    RdrFreeContext(pContext);

    return FALSE;

error:

    RdrReleaseFile(pFile);

    goto cleanup;
}

NTSTATUS
RdrCreate(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PRDR_OP_CONTEXT pContext = NULL;
    PIO_CREDS pCreds = IoSecurityGetCredentials(pIrp->Args.Create.SecurityContext);

    status = RdrCreateContext(pIrp, &pContext);
    BAIL_ON_NT_STATUS(status);

    IoIrpMarkPending(pIrp, RdrCancelCreate, pContext);

    if (!pCreds)
    {
        status = STATUS_ACCESS_DENIED;
        BAIL_ON_NT_STATUS(status);
    }

    pContext->Continue = RdrCreateTreeConnected;

    status = RdrCreateTreeConnect(pContext);
    BAIL_ON_NT_STATUS(status);

cleanup:

    if (status != STATUS_PENDING)
    {
        RdrFreeContext(pContext);
        pIrp->IoStatusBlock.Status = status;
        IoIrpComplete(pIrp);
    }

    return status;

error:

    goto cleanup;
}

static
NTSTATUS
ParseSharePath(
    PCWSTR pwszPath,
    PWSTR*  ppwszServer,
    PSTR*   ppszShare,
    PSTR*   ppszFilename
    )
{
    NTSTATUS status = 0;
    PSTR  pszPath = NULL;
    PSTR  pszIndex = NULL;
    PSTR  pszServer = NULL;
    PSTR  pszCanonical = NULL;
    PSTR  pszShare  = NULL;
    PSTR  pszFilename = NULL;
    PSTR  pszCursor = NULL;
    size_t sLen = 0;
    size_t i = 0;
    struct in_addr ipAddr;

    memset(&ipAddr, 0, sizeof(ipAddr));
    status = SMBWc16sToMbs(
                    pwszPath,
                    &pszPath);
    BAIL_ON_NT_STATUS(status);

    SMBStripWhitespace(pszPath, TRUE, TRUE);

    pszIndex = pszPath;

    // Skip optional initial decoration
    if (!strncmp(pszIndex, "/", sizeof("/") - 1) ||
        !strncmp(pszIndex, "\\", sizeof("\\") - 1))
    {
        pszIndex += 1;
    }

    if (IsNullOrEmptyString(pszIndex))
    {
        status = LWIO_ERROR_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(status);
    }

    // Seek server name
    sLen = strcspn(pszIndex, "\\/");
    if (!sLen)
    {
        status = LWIO_ERROR_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(status);
    }

    status = SMBStrndup(
                    pszIndex,
                    sLen,
                    &pszServer);
    BAIL_ON_NT_STATUS(status);

    status = SMBStrndup(
                    pszIndex,
                    sLen,
                    &pszCanonical);
    BAIL_ON_NT_STATUS(status);

    for (pszCursor = pszCanonical; *pszCursor; pszCursor++)
    {
        if (*pszCursor == '@')
        {
            *pszCursor = '\0';
            break;
        }
    }

    pszIndex += sLen;

    // Skip delimiter
    sLen = strspn(pszIndex, "\\/");
    if (!sLen)
    {
        status = LWIO_ERROR_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(status);
    }

    pszIndex += sLen;

    // Seek share name
    sLen = strcspn(pszIndex, "\\/");
    if (!sLen)
    {
        status = LWIO_ERROR_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(status);
    }

    status = SMBAllocateMemory(
        sizeof("\\\\") - 1 + strlen(pszCanonical) + sizeof("\\") - 1 + sLen + 1,
        (PVOID*)&pszShare);
    BAIL_ON_NT_STATUS(status);

    sprintf(pszShare, "\\\\%s\\", pszCanonical);
    strncat(pszShare, pszIndex, sLen);

    pszIndex += sLen;

    // Skip delimiter
    sLen = strspn(pszIndex, "\\/");
    if (!sLen)
    {
        SMBAllocateMemory(
            strlen("\\") + 1,
            (PVOID*)&pszFilename);
        BAIL_ON_NT_STATUS(status);

        pszFilename[0] = '\\';
        pszFilename[1] = '\0';
    }
    else
    {
        pszIndex += sLen;

        SMBAllocateMemory(
            strlen(pszIndex) + 2,
            (PVOID*)&pszFilename);
        BAIL_ON_NT_STATUS(status);

        pszFilename[0] = '\\';

        for (i = 0; pszIndex[i]; i++)
        {
            switch (pszIndex[i])
            {
            case '/':
                pszFilename[1 + i] = '\\';
                break;
            default:
                pszFilename[1 + i] = pszIndex[i];
            }
        }

        pszFilename[1 + i] = '\0';
    }

    status = LwRtlWC16StringAllocateFromCString(ppwszServer, pszServer);
    BAIL_ON_NT_STATUS(status);

    *ppszShare  = pszShare;
    *ppszFilename = pszFilename;

cleanup:

    LWIO_SAFE_FREE_STRING(pszCanonical);
    LWIO_SAFE_FREE_STRING(pszServer);
    LWIO_SAFE_FREE_STRING(pszPath);

    return status;

error:

    LWIO_SAFE_FREE_STRING(pszServer);
    LWIO_SAFE_FREE_STRING(pszShare);
    LWIO_SAFE_FREE_STRING(pszFilename);

    *ppwszServer = NULL;
    *ppszShare = NULL;
    *ppszFilename = NULL;

    goto cleanup;
}

static
NTSTATUS
RdrTransceiveCreate(
    PRDR_OP_CONTEXT pContext,
    PSMB_CLIENT_FILE_HANDLE pFile,
    PCWSTR pwszPath,
    ACCESS_MASK desiredAccess,
    LONG64 llAllocationSize,
    FILE_ATTRIBUTES fileAttributes,
    FILE_SHARE_FLAGS shareAccess,
    FILE_CREATE_DISPOSITION createDisposition,
    FILE_CREATE_OPTIONS createOptions
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    uint32_t packetByteCount = 0;
    CREATE_REQUEST_HEADER *pHeader = NULL;

    status = RdrAllocateContextPacket(
        pContext,
        1024*64);
    BAIL_ON_NT_STATUS(status);

    status = SMBPacketMarshallHeader(
                pContext->Packet.pRawBuffer,
                pContext->Packet.bufferLen,
                COM_NT_CREATE_ANDX,
                0,
                0,
                pFile->pTree->tid,
                gRdrRuntime.SysPid,
                pFile->pTree->pSession->uid,
                0,
                TRUE,
                &pContext->Packet);
    BAIL_ON_NT_STATUS(status);

    pContext->Packet.pData = pContext->Packet.pParams + sizeof(CREATE_REQUEST_HEADER);

    pContext->Packet.bufferUsed += sizeof(CREATE_REQUEST_HEADER);

    pContext->Packet.pSMBHeader->wordCount = 24;

    pHeader = (CREATE_REQUEST_HEADER *) pContext->Packet.pParams;

    pHeader->reserved = 0;
    pHeader->nameLength = (wc16slen(pwszPath) + 1) * sizeof(wchar16_t);
    pHeader->flags = 0;
    pHeader->rootDirectoryFid = 0;
    pHeader->desiredAccess = desiredAccess;
    pHeader->allocationSize = llAllocationSize;
    pHeader->extFileAttributes = fileAttributes;
    pHeader->shareAccess = shareAccess;
    pHeader->createDisposition = createDisposition;
    pHeader->createOptions = createOptions;
    pHeader->impersonationLevel = 0x2; /* FIXME */

    status = WireMarshallCreateRequestData(
                pContext->Packet.pData,
                pContext->Packet.bufferLen - pContext->Packet.bufferUsed,
                (pContext->Packet.pData - (uint8_t *) pContext->Packet.pSMBHeader) % 2,
                &packetByteCount,
                pwszPath);
    BAIL_ON_NT_STATUS(status);

    assert(packetByteCount <= UINT16_MAX);
    pHeader->byteCount = (uint16_t) packetByteCount;
    pContext->Packet.bufferUsed += packetByteCount;

    // byte order conversions
    SMB_HTOL8_INPLACE(pHeader->reserved);
    SMB_HTOL16_INPLACE(pHeader->nameLength);
    SMB_HTOL32_INPLACE(pHeader->flags);
    SMB_HTOL32_INPLACE(pHeader->rootDirectoryFid);
    SMB_HTOL32_INPLACE(pHeader->desiredAccess);
    SMB_HTOL64_INPLACE(pHeader->allocationSize);
    SMB_HTOL32_INPLACE(pHeader->extFileAttributes);
    SMB_HTOL32_INPLACE(pHeader->shareAccess);
    SMB_HTOL32_INPLACE(pHeader->createDisposition);
    SMB_HTOL32_INPLACE(pHeader->createOptions);
    SMB_HTOL32_INPLACE(pHeader->impersonationLevel);
    SMB_HTOL8_INPLACE(pHeader->securityFlags);
    SMB_HTOL16_INPLACE(pHeader->byteCount);

    status = SMBPacketMarshallFooter(&pContext->Packet);
    BAIL_ON_NT_STATUS(status);

    status = RdrSocketTransceive(pFile->pTree->pSession->pSocket, pContext);
    BAIL_ON_NT_STATUS(status);

cleanup:

    return status;

error:

    goto cleanup;
}
