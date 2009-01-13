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
 *        session_setup.c
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem (LWIO)
 *
 *        SMB Client Session Setup Handler
 *
 * Author: Kaya Bekiroglu (kaya@likewisesoftware.com)
 *
 * @todo: add error logging code
 * @todo: switch to NT error codes where appropriate
 */

#include "rdr.h"

/* @todo: clean up */
/* @todo: the first session on a socket is always VC 0; increment
thereafter */
DWORD
SessionSetup(
    PSMB_SOCKET pSocket,
    BOOLEAN     bSignMessages,
    PBYTE       pPrimerSessionKey,
    DWORD       dwPrimerSessionKeyLen,
    uint16_t   *pUID,
    PBYTE*      ppSessionKey,
    PDWORD      pdwSessionKeyLength,
    PHANDLE     phGssContext
    )
{
    DWORD dwError = 0;
    SMB_PACKET packet = {0};

    uint32_t packetByteCount = 0;

    HANDLE   hSMBGSSContext = NULL;

    wchar16_t nativeOS[1024];
    wchar16_t nativeLanMan[1024];
    wchar16_t nativeDomain[1024];
    uint8_t*  pSecurityBlob2 = NULL;
    uint32_t  dwSecurityBlobLen2 = 0;
    BOOLEAN   bInLock = FALSE;
    PBYTE     pSessionKey = NULL;
    DWORD     dwSessionKeyLength = 0;

    SMB_PACKET *pResponsePacket = NULL;
    PSESSION_SETUP_RESPONSE_HEADER pResponseHeader = NULL;
    DWORD     dwSequence = 0;
    DWORD     dwResponseSequence = 0;

    dwError = SMBGSSContextBuild(
                    (char *) pSocket->pszHostname,
                    &hSMBGSSContext);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBGSSContextNegotiate(
                    hSMBGSSContext,
                    pSocket->pSecurityBlob,
                    pSocket->securityBlobLen,
                    &pSecurityBlob2,
                    &dwSecurityBlobLen2);
    BAIL_ON_SMB_ERROR(dwError);

    /* @todo: make initial length configurable */
    dwError = SMBSocketBufferAllocate(
                    pSocket,
                    1024*64,
                    &packet.pRawBuffer,
                    &packet.bufferLen);
    BAIL_ON_SMB_ERROR(dwError);

    while (!SMBGSSContextNegotiateComplete(hSMBGSSContext))
    {
        PBYTE pSecurityBlob = NULL;
        PWSTR pwszNativeOS = NULL;
        PWSTR pwszNativeLanman = NULL;
        PWSTR pwszNativeDomain = NULL;

        dwError = SMBPacketMarshallHeader(
                        packet.pRawBuffer,
                        packet.bufferLen,
                        COM_SESSION_SETUP_ANDX,
                        0,
                        0,
                        0xFFFF,
                        0,
                        0,
                        0,
                        FALSE, /* sign messages */
                        &packet);
        BAIL_ON_SMB_ERROR(dwError);

        packet.pData = packet.pParams + sizeof(SESSION_SETUP_REQUEST_HEADER);
        packet.bufferUsed += sizeof(SESSION_SETUP_REQUEST_HEADER);
        /* If most commands have word counts which are easy to compute, this
           should be folded into a parameter to SMBPacketMarshallHeader() */
        packet.pSMBHeader->wordCount = 12;

        SESSION_SETUP_REQUEST_HEADER *pHeader =
            (SESSION_SETUP_REQUEST_HEADER *) packet.pParams;
        pHeader->maxBufferSize = 12288;
        pHeader->maxMpxCount = 50;
        pHeader->vcNumber = 1;
        pHeader->sessionKey = pSocket->sessionKey;
        pHeader->securityBlobLength = 0;
        pHeader->reserved = 0;
        pHeader->capabilities = CAP_UNICODE | CAP_NT_SMBS | CAP_STATUS32 |
            CAP_EXTENDED_SECURITY;

        pHeader->securityBlobLength = dwSecurityBlobLen2;
        packet.pByteCount = &pHeader->byteCount;

        wcstowc16s(nativeOS, L"Unix", sizeof(nativeOS));
        wcstowc16s(nativeLanMan, L"Likewise SMB", sizeof(nativeLanMan));
        /* @todo: change to native domain */
        wcstowc16s(nativeDomain, L"WORKGROUP", sizeof(nativeDomain));

        /* @todo: handle buffer size restart with ERESTART */
        dwError = MarshallSessionSetupRequestData(
                        packet.pData,
                        packet.bufferLen - packet.bufferUsed,
                        (packet.pData - (uint8_t *) packet.pSMBHeader) % 2,
                        &packetByteCount,
                        pSecurityBlob2,
                        dwSecurityBlobLen2,
                        nativeOS,
                        nativeLanMan,
                        nativeDomain);
        BAIL_ON_SMB_ERROR(dwError);

        assert(packetByteCount <= UINT16_MAX);
        *packet.pByteCount = (uint16_t) packetByteCount;
        packet.bufferUsed += *packet.pByteCount;

        dwError = SMBPacketMarshallFooter(&packet);
        BAIL_ON_SMB_ERROR(dwError);

        dwSequence = SMBSocketGetNextSequence(pSocket);
        dwResponseSequence = dwSequence + 1;

        if (bSignMessages && (pSessionKey || pPrimerSessionKey))
        {
            dwError = SMBPacketSign(
                            &packet,
                            dwSequence,
                            (pSessionKey ? pSessionKey : pPrimerSessionKey),
                            (pSessionKey ? dwSessionKeyLength : dwPrimerSessionKeyLen));
            BAIL_ON_SMB_ERROR(dwError);
        }

        /* Because there's no MID, only one SESSION_SETUP_ANDX packet can be
           outstanding. */
        /* @todo: test multiple session setups with multiple MIDs */
        SMB_LOCK_MUTEX(bInLock, &pSocket->sessionMutex);

        /* @todo: on send packet error, the response must be removed from the
           tree. */
        dwError = SMBPacketSend(pSocket, &packet);
        BAIL_ON_SMB_ERROR(dwError);

        if (pResponsePacket)
        {
            SMBSocketPacketFree(pSocket, pResponsePacket);
            pResponsePacket = NULL;
        }

        dwError = SMBSocketReceiveSessionSetupResponse(
                        pSocket,
                        &pResponsePacket);
        BAIL_ON_SMB_ERROR(dwError);

        SMB_UNLOCK_MUTEX(bInLock, &pSocket->sessionMutex);

        if (bSignMessages && (pSessionKey || pPrimerSessionKey))
        {
            dwError = SMBPacketVerifySignature(
                            pResponsePacket,
                            dwResponseSequence,
                            (pSessionKey ? pSessionKey : pPrimerSessionKey),
                            (pSessionKey ? dwSessionKeyLength : dwPrimerSessionKeyLen));
            BAIL_ON_SMB_ERROR(dwError);
        }

        dwError = pResponsePacket->pSMBHeader->error;
        BAIL_ON_SMB_ERROR(dwError);

        dwError = UnmarshallSessionSetupResponse(
                        pResponsePacket->pParams,
                        pResponsePacket->bufferLen - pResponsePacket->bufferUsed,
                        0,
                        &pResponseHeader,
                        &pSecurityBlob,
                        &pwszNativeOS,
                        &pwszNativeLanman,
                        &pwszNativeDomain);
        BAIL_ON_SMB_ERROR(dwError);

        SMB_SAFE_FREE_MEMORY(pSecurityBlob2);

        dwError = SMBGSSContextNegotiate(
                        hSMBGSSContext,
                        pSecurityBlob,
                        pResponseHeader->securityBlobLength,
                        &pSecurityBlob2,
                        &dwSecurityBlobLen2);
        BAIL_ON_SMB_ERROR(dwError);

    }

    dwError = SMBGSSContextGetSessionKey(
                    hSMBGSSContext,
                    &pSessionKey,
                    &dwSessionKeyLength);
    BAIL_ON_SMB_ERROR(dwError);

    *pUID = pResponsePacket->pSMBHeader->uid;
    *phGssContext = hSMBGSSContext;
    *ppSessionKey = pSessionKey;
    *pdwSessionKeyLength = dwSessionKeyLength;

cleanup:

    if (pResponsePacket)
    {
        SMBSocketPacketFree(pSocket, pResponsePacket);
    }

    if (packet.bufferLen)
    {
        SMBSocketBufferFree(
                pSocket,
                packet.pRawBuffer,
                packet.bufferLen);
    }

    SMB_SAFE_FREE_MEMORY(pSecurityBlob2);

    SMB_UNLOCK_MUTEX(bInLock, &pSocket->sessionMutex);

    return dwError;

error:

    *pUID = 0;
    *ppSessionKey = NULL;
    *pdwSessionKeyLength = 0;

    if (hSMBGSSContext)
    {
        SMBGSSContextFree(hSMBGSSContext);
    }

    SMB_SAFE_FREE_MEMORY(pSessionKey);

    goto cleanup;
}

