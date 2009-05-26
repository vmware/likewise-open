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
 *        callnp.c
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem (SMB)
 *
 *        CloseHandle API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "includes.h"

static
DWORD
RdrTransactCloseFile(
    PSMB_TREE pTree,
    uint16_t usFid
    );

DWORD
RdrCloseFileEx(
    HANDLE hFile
    )
{
    DWORD dwError = 0;
    PSMB_CLIENT_FILE_HANDLE pFile = (PSMB_CLIENT_FILE_HANDLE)hFile;

    if (pFile->fid != 0)
    {
        /* Ignore error sending close message and proceed to tear down
           local resources */
        RdrTransactCloseFile(
            pFile->pTree,
            pFile->fid);
    }

    if (pFile->pTree)
    {
        SMBTreeRelease(pFile->pTree);
    }

    SMB_SAFE_FREE_STRING(pFile->pszCachePath);
    SMB_SAFE_FREE_STRING(pFile->pszPrincipal);

    if (pFile->pMutex)
    {
        pthread_mutex_destroy(pFile->pMutex);
    }

    SMBFreeMemory(pFile);

    return dwError;
}

static
DWORD
RdrTransactCloseFile(
    PSMB_TREE pTree,
    uint16_t usFid
    )
{
    DWORD dwError = STATUS_SUCCESS;
    SMB_PACKET packet = {0};
    CLOSE_REQUEST_HEADER *pHeader = NULL;
    SMB_RESPONSE *pResponse = NULL;
    PSMB_PACKET pResponsePacket = NULL;
    USHORT usMid = 0;


    dwError = SMBSocketBufferAllocate(
        pTree->pSession->pSocket,
        1024*64,
        &packet.pRawBuffer,
        &packet.bufferLen);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBTreeAcquireMid(
        pTree,
        &usMid);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBPacketMarshallHeader(
        packet.pRawBuffer,
        packet.bufferLen,
        COM_CLOSE,
        0,
        0,
        pTree->tid,
        0,
        pTree->pSession->uid,
        usMid,
        TRUE,
        &packet);
    BAIL_ON_SMB_ERROR(dwError);

    packet.pData = packet.pParams + sizeof(CLOSE_REQUEST_HEADER);

    packet.bufferUsed += sizeof(CLOSE_REQUEST_HEADER);

    packet.pSMBHeader->wordCount = 3;

    pHeader = (CLOSE_REQUEST_HEADER*) packet.pParams;

    pHeader->fid = SMB_HTOL16(usFid);
    pHeader->lastWriteTime = SMB_HTOL64(0);
    pHeader->byteCount = SMB_HTOL16(0);

    dwError = SMBPacketMarshallFooter(&packet);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBResponseCreate(usMid, &pResponse);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBSrvClientTreeAddResponse(pTree, pResponse);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBPacketSend(pTree->pSession->pSocket, &packet);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBTreeReceiveResponse(
        pTree,
        packet.haveSignature,
        packet.sequence + 1,
        pResponse,
        &pResponsePacket);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = pResponsePacket->pSMBHeader->error;
    BAIL_ON_SMB_ERROR(dwError);

cleanup:

    if (pResponsePacket)
    {
        SMBSocketPacketFree(
            pTree->pSession->pSocket,
            pResponsePacket);
    }

    if (packet.bufferLen)
    {
        SMBSocketBufferFree(pTree->pSession->pSocket,
                            packet.pRawBuffer,
                            packet.bufferLen);
    }

    if (pResponse)
    {
        SMBResponseFree(pResponse);
    }

    return dwError;

error:

    goto cleanup;
}
