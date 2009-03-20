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
 *        createnp.c
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem (SMB)
 *
 *        CreateNamedPipe API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "rdr.h"

static
NTSTATUS
ParseSharePath(
    PCWSTR pwszPath,
    PSTR*   ppszServer,
    PSTR*   ppszShare,
    PSTR*   ppszFilename
    );

static
NTSTATUS
RdrTransactCreateFile(
    SMB_TREE *pTree,
    PCWSTR pwszPath,
    ACCESS_MASK desiredAccess,
    LONG64 llAllocationSize,
    FILE_ATTRIBUTES fileAttributes,
    FILE_SHARE_FLAGS shareAccess,
    FILE_CREATE_DISPOSITION createDisposition,
    FILE_CREATE_OPTIONS createOptions,
    PUSHORT pusFid
    );

NTSTATUS
RdrCreateFileEx(
    PIO_ACCESS_TOKEN pSecurityToken,
    PCWSTR pwszPath,
    ACCESS_MASK desiredAccess,
    LONG64 llAllocationSize,
    FILE_ATTRIBUTES fileAttributes,
    FILE_SHARE_FLAGS shareAccess,
    FILE_CREATE_DISPOSITION createDisposition,
    FILE_CREATE_OPTIONS createOptions,
    PHANDLE phFile
    )
{
    NTSTATUS ntStatus = 0;
    PSTR   pszServer = NULL;
    PSTR   pszShare = NULL;
    PSTR   pszFilename = NULL;
    PWSTR  pwszFilename = NULL;
    PSMB_CLIENT_FILE_HANDLE pFile = NULL;

    if (!pSecurityToken ||
        pSecurityToken->type != IO_ACCESS_TOKEN_TYPE_KRB5)
    {
        ntStatus = STATUS_ACCESS_DENIED;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SMBAllocateMemory(
        sizeof(SMB_CLIENT_FILE_HANDLE),
        (PVOID*)&pFile);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = pthread_mutex_init(&pFile->mutex, NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    pFile->pMutex = &pFile->mutex;

    ntStatus = ParseSharePath(
        pwszPath,
        &pszServer,
        &pszShare,
        &pszFilename);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBWc16sToMbs(
        pSecurityToken->payload.krb5.pwszPrincipal,
        &pFile->pszPrincipal);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBWc16sToMbs(
                    pSecurityToken->payload.krb5.pwszCachePath,
                    &pFile->pszCachePath);
    BAIL_ON_NT_STATUS(ntStatus);

    SMB_LOG_DEBUG("Principal [%s] Cache Path [%s]",
                  SMB_SAFE_LOG_STRING(pFile->pszPrincipal),
                  SMB_SAFE_LOG_STRING(pFile->pszCachePath));

    ntStatus = SMBKrb5SetDefaultCachePath(
                    pFile->pszCachePath,
                    NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBSrvClientTreeOpen(
                    pszServer,
                    pFile->pszPrincipal,
                    pszShare,
                    &pFile->pTree);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBMbsToWc16s(
                    pszFilename,
                    &pwszFilename);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = RdrTransactCreateFile(
        pFile->pTree,
        pwszFilename,
        desiredAccess,
        llAllocationSize,
        fileAttributes,
        shareAccess,
        createDisposition,
        createOptions,
        &pFile->fid);
    BAIL_ON_NT_STATUS(ntStatus);

    pFile->pwszPath = pwszFilename;
    pwszFilename = NULL;

    *phFile = (HANDLE)pFile;

cleanup:

    SMB_SAFE_FREE_STRING(pszServer);
    SMB_SAFE_FREE_STRING(pszShare);
    SMB_SAFE_FREE_STRING(pszFilename);
    SMB_SAFE_FREE_MEMORY(pwszFilename);

    return ntStatus;

error:

    if (pFile)
    {
        RdrReleaseFile(pFile);
    }

    *phFile = NULL;

    goto cleanup;
}

static
NTSTATUS
ParseSharePath(
    PCWSTR pwszPath,
    PSTR*   ppszServer,
    PSTR*   ppszShare,
    PSTR*   ppszFilename
    )
{
    NTSTATUS ntStatus = 0;
    PSTR  pszPath = NULL;
    PSTR  pszIndex = NULL;
    PSTR  pszServer = NULL;
    PSTR  pszShare  = NULL;
    PSTR  pszFilename = NULL;
    size_t sLen = 0;
    size_t i = 0;

    ntStatus = SMBWc16sToMbs(
                    pwszPath,
                    &pszPath);
    BAIL_ON_NT_STATUS(ntStatus);

    SMBStripWhitespace(pszPath, TRUE, TRUE);

    pszIndex = pszPath;

    // Skip optional initial decoration
    if (!strncmp(pszIndex, "/", sizeof("/") - 1) ||
        !strncmp(pszIndex, "\\", sizeof("\\") - 1))
    {
        pszIndex += 1;
    }

    if (IsNullOrEmptyString(pszIndex) || !isalpha((int)*pszIndex))
    {
        ntStatus = SMB_ERROR_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    // Seek server name
    sLen = strcspn(pszIndex, "\\/");
    if (!sLen)
    {
        ntStatus = SMB_ERROR_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SMBStrndup(
                    pszIndex,
                    sLen,
                    &pszServer);
    BAIL_ON_NT_STATUS(ntStatus);

    pszIndex += sLen;

    // Skip delimiter
    sLen = strspn(pszIndex, "\\/");
    if (!sLen)
    {
        ntStatus = SMB_ERROR_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pszIndex += sLen;

    // Seek share name
    sLen = strcspn(pszIndex, "\\/");
    if (!sLen)
    {
        ntStatus = SMB_ERROR_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SMBAllocateMemory(
        sizeof("\\\\") - 1 + strlen(pszServer) + sizeof("\\") - 1 + sLen + 1,
        (PVOID*)&pszShare);
    BAIL_ON_NT_STATUS(ntStatus);
    
    sprintf(pszShare, "\\\\%s\\", pszServer);
    strncat(pszShare, pszIndex, sLen);

    pszIndex += sLen;

    // Skip delimiter
    sLen = strspn(pszIndex, "\\/");
    if (!sLen)
    {
        ntStatus = SMB_ERROR_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pszIndex += sLen;

    SMBAllocateMemory(
        strlen(pszIndex) + 2,
        (PVOID*)&pszFilename);
    BAIL_ON_NT_STATUS(ntStatus);

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

    *ppszServer = pszServer;
    *ppszShare  = pszShare;
    *ppszFilename = pszFilename;

cleanup:

    SMB_SAFE_FREE_STRING(pszPath);

    return ntStatus;

error:

    SMB_SAFE_FREE_STRING(pszServer);
    SMB_SAFE_FREE_STRING(pszShare);
    SMB_SAFE_FREE_STRING(pszFilename);

    *ppszServer = NULL;
    *ppszShare = NULL;
    *ppszFilename = NULL;

    goto cleanup;
}

static
NTSTATUS
RdrTransactCreateFile(
    SMB_TREE *pTree,
    PCWSTR pwszPath,
    ACCESS_MASK desiredAccess,
    LONG64 llAllocationSize,
    FILE_ATTRIBUTES fileAttributes,
    FILE_SHARE_FLAGS shareAccess,
    FILE_CREATE_DISPOSITION createDisposition,
    FILE_CREATE_OPTIONS createOptions,
    PUSHORT pusFid
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    SMB_PACKET packet = {0};
    uint32_t packetByteCount = 0;
    CREATE_REQUEST_HEADER *pHeader = NULL;
    CREATE_RESPONSE_HEADER *pResponseHeader = NULL;
    SMB_RESPONSE *pResponse = NULL;
    PSMB_PACKET pResponsePacket = NULL;
    USHORT usMid = 0;

    /* @todo: make initial length configurable */
    ntStatus = SMBPacketBufferAllocate(
                    pTree->pSession->pSocket->hPacketAllocator,
                    1024*64,
                    &packet.pRawBuffer,
                    &packet.bufferLen);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBTreeAcquireMid(
                    pTree,
                    &usMid);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketMarshallHeader(
                packet.pRawBuffer,
                packet.bufferLen,
                COM_NT_CREATE_ANDX,
                0,
                0,
                pTree->tid,
                0,
                pTree->pSession->uid,
                usMid,
                TRUE,
                &packet);
    BAIL_ON_NT_STATUS(ntStatus);

    packet.pData = packet.pParams + sizeof(CREATE_REQUEST_HEADER);

    /* @todo: handle size restart */
    packet.bufferUsed += sizeof(CREATE_REQUEST_HEADER);

    /* If most commands have word counts which are easy to compute, this
       should be folded into a parameter to SMBPacketMarshallHeader() */
    packet.pSMBHeader->wordCount = 24;

    pHeader = (CREATE_REQUEST_HEADER *) packet.pParams;

    pHeader->reserved = 0;
    /* @todo: does the length include alignment padding? */
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

    ntStatus = WireMarshallCreateRequestData(
                packet.pData,
                packet.bufferLen - packet.bufferUsed,
                (packet.pData - (uint8_t *) packet.pSMBHeader) % 2,
                &packetByteCount,
                pwszPath);
    BAIL_ON_NT_STATUS(ntStatus);

    assert(packetByteCount <= UINT16_MAX);
    pHeader->byteCount = (uint16_t) packetByteCount;
    packet.bufferUsed += packetByteCount;

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

    ntStatus = SMBPacketMarshallFooter(&packet);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBResponseCreate(usMid, &pResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBSrvClientTreeAddResponse(pTree, pResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    /* @todo: on send packet error, the response must be removed from the
       tree.*/
    ntStatus = SMBSocketSend(pTree->pSession->pSocket, &packet);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBTreeReceiveResponse(
                    pTree,
                    packet.haveSignature,
                    packet.sequence + 1,
                    pResponse,
                    &pResponsePacket);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = pResponsePacket->pSMBHeader->error;
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = WireUnmarshallSMBResponseCreate(
                pResponsePacket->pParams,
                pResponsePacket->bufferLen - pResponsePacket->bufferUsed,
                &pResponseHeader);
    BAIL_ON_NT_STATUS(ntStatus);

    *pusFid = pResponseHeader->fid;

cleanup:

    if (pResponsePacket)
    {
        SMBPacketFree(
            pTree->pSession->pSocket->hPacketAllocator,
            pResponsePacket);
    }

    if (packet.bufferLen)
    {
        SMBPacketBufferFree(pTree->pSession->pSocket->hPacketAllocator,
                            packet.pRawBuffer,
                            packet.bufferLen);
    }

    if (pResponse)
    {
        SMBResponseFree(pResponse);
    }

    return ntStatus;

error:

    *pusFid = 0;

    goto cleanup;
}
