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
SrvUnmarshallSessionSetupRequest(
    PLWIO_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    PBYTE*              ppSecurityBlob,
    PULONG              pulSecurityBlobLength
    );

static
NTSTATUS
SrvMarshallSessionSetupResponse(
    PLWIO_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    USHORT              usMid,
    PBYTE               pSecurityBlob,
    ULONG               ulSecurityBlobLength,
    PSMB_PACKET*        ppSmbResponse
    );

NTSTATUS
SrvProcessSessionSetup(
    IN  PLWIO_SRV_CONNECTION pConnection,
    IN  PSMB_PACKET          pSmbRequest,
    OUT PSMB_PACKET*         ppSmbResponse
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_PACKET pSmbResponse = NULL;
    PBYTE       pSecurityBlob = NULL; // Do Not Free
    ULONG       ulSecurityBlobLength = 0;
    PLWIO_SRV_SESSION pSession = NULL;
    UNICODE_STRING uniUsername = {0};

    ntStatus = SrvUnmarshallSessionSetupRequest(
                    pConnection,
                    pSmbRequest,
                    &pSecurityBlob,
                    &ulSecurityBlobLength);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvMarshallSessionSetupResponse(
                    pConnection,
                    pSmbRequest,
                    pSmbRequest->pSMBHeader->mid,
                    pSecurityBlob,
                    ulSecurityBlobLength,
                    &pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    if (SrvGssNegotiateIsComplete(pConnection->hGssContext, pConnection->hGssNegotiate))
    {
        ntStatus = SrvConnectionCreateSession(
                        pConnection,
                        &pSession);
        BAIL_ON_NT_STATUS(ntStatus);

        if (!pConnection->pSessionKey)
        {
             ntStatus = SrvGssGetSessionDetails(
                        pConnection->hGssContext,
                        pConnection->hGssNegotiate,
                        &pConnection->pSessionKey,
                        &pConnection->ulSessionKeyLength,
                        &pSession->pszClientPrincipalName);
             BAIL_ON_NT_STATUS(ntStatus);
        }
        else
        {
             ntStatus = SrvGssGetSessionDetails(
                        pConnection->hGssContext,
                        pConnection->hGssNegotiate,
                        NULL,
                        NULL,
                        &pSession->pszClientPrincipalName);
             BAIL_ON_NT_STATUS(ntStatus);
        }

        /* Generate and store the IoSecurityContext */

        ntStatus = RtlUnicodeStringAllocateFromCString(
                       &uniUsername,
                       pSession->pszClientPrincipalName);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = IoSecurityCreateSecurityContextFromUsername(
                       &pSession->pIoSecurityContext,
                       &uniUsername);
        BAIL_ON_NT_STATUS(ntStatus);


        pSmbResponse->pSMBHeader->uid = pSession->uid;

        SrvConnectionSetState(pConnection, LWIO_SRV_CONN_STATE_READY);
    }

    *ppSmbResponse = pSmbResponse;

cleanup:
    RtlUnicodeStringFree(&uniUsername);

    if (pSession)
    {
        SrvSessionRelease(pSession);
    }

    return (ntStatus);

error:

    *ppSmbResponse = NULL;

    if (pSmbResponse)
    {
        SMBPacketRelease(
            pConnection->hPacketAllocator,
            pSmbResponse);
    }

    goto cleanup;
}

static
NTSTATUS
SrvUnmarshallSessionSetupRequest(
    PLWIO_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    PBYTE*              ppSecurityBlob,
    PULONG              pulSecurityBlobLength
    )
{
    NTSTATUS ntStatus = 0;
    // Don't free the following
    // They are pointing into the request buffer
    SESSION_SETUP_REQUEST_HEADER* pHeader = NULL;
    PBYTE pSecurityBlob = NULL;
    PWSTR pwszNativeOS = NULL;
    PWSTR pwszNativeLanMan = NULL;
    PWSTR pwszNativeDomain = NULL;
    ULONG ulOffset = 0;

    ulOffset = (PBYTE)pSmbRequest->pParams - (PBYTE)pSmbRequest->pSMBHeader;

    ntStatus = UnmarshallSessionSetupRequest(
                    pSmbRequest->pParams,
                    pSmbRequest->pNetBIOSHeader->len - ulOffset,
                    0,
                    &pHeader,
                    &pSecurityBlob,
                    &pwszNativeOS,
                    &pwszNativeLanMan,
                    &pwszNativeDomain);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pConnection->clientProperties.pwszNativeOS)
    {
        SrvFreeMemory(pConnection->clientProperties.pwszNativeOS);
        pConnection->clientProperties.pwszNativeOS = NULL;
    }
    if (pwszNativeOS)
    {
        ntStatus = SrvAllocateStringW(
                        pwszNativeOS,
                        &pConnection->clientProperties.pwszNativeOS);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (pConnection->clientProperties.pwszNativeLanMan)
    {
        SrvFreeMemory(pConnection->clientProperties.pwszNativeLanMan);
        pConnection->clientProperties.pwszNativeLanMan = NULL;
    }
    if (pwszNativeLanMan)
    {
        ntStatus = SrvAllocateStringW(
                        pwszNativeLanMan,
                        &pConnection->clientProperties.pwszNativeLanMan);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (pConnection->clientProperties.pwszNativeDomain)
    {
        SrvFreeMemory(pConnection->clientProperties.pwszNativeDomain);
        pConnection->clientProperties.pwszNativeDomain = NULL;
    }
    if (pwszNativeDomain)
    {
        ntStatus = SrvAllocateStringW(
                        pwszNativeDomain,
                        &pConnection->clientProperties.pwszNativeDomain);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pConnection->clientProperties.Capabilities = pHeader->capabilities;
    pConnection->clientProperties.MaxBufferSize = pHeader->maxBufferSize;
    pConnection->clientProperties.MaxMpxCount = pHeader->maxMpxCount;
    pConnection->clientProperties.SessionKey = pHeader->sessionKey;
    pConnection->clientProperties.VcNumber = pHeader->vcNumber;

    *ppSecurityBlob = pSecurityBlob;
    *pulSecurityBlobLength = pHeader->securityBlobLength;

cleanup:

    return ntStatus;

error:

    *ppSecurityBlob = NULL;
    *pulSecurityBlobLength = 0;

    goto cleanup;
}

static
NTSTATUS
SrvMarshallSessionSetupResponse(
    PLWIO_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    USHORT              usMid,
    PBYTE               pSecurityBlob,
    ULONG               ulSecurityBlobLength,
    PSMB_PACKET*        ppSmbResponse
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_PACKET pSmbResponse = NULL;
    SESSION_SETUP_RESPONSE_HEADER* pResponseHeader = NULL;
    PBYTE pReplySecurityBlob = NULL;
    ULONG ulReplySecurityBlobLength = 0;
    wchar16_t wszNativeOS[256];
    wchar16_t wszNativeLanMan[256];
    wchar16_t wszNativeDomain[256];
    ULONG     ulPackageByteCount = 0;

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

    ntStatus = SrvGssNegotiate(
                    pConnection->hGssContext,
                    pConnection->hGssNegotiate,
                    pSecurityBlob,
                    ulSecurityBlobLength,
                    &pReplySecurityBlob,
                    &ulReplySecurityBlobLength);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketMarshallHeader(
                pSmbResponse->pRawBuffer,
                pSmbResponse->bufferLen,
                COM_SESSION_SETUP_ANDX,
                0,
                TRUE,
                0,
                pSmbRequest->pSMBHeader->tid,
                pSmbRequest->pSMBHeader->pid,
                usMid,
                pConnection->serverProperties.bRequireSecuritySignatures,
                pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    pSmbResponse->pSMBHeader->wordCount = 4;

    pResponseHeader = (SESSION_SETUP_RESPONSE_HEADER*)pSmbResponse->pParams;
    pSmbResponse->pData = pSmbResponse->pParams + sizeof(SESSION_SETUP_RESPONSE_HEADER);
    pSmbResponse->bufferUsed += sizeof(SESSION_SETUP_RESPONSE_HEADER);

    wcstowc16s(wszNativeOS, L"Unix", sizeof(wszNativeOS));
    wcstowc16s(wszNativeLanMan, L"Likewise IO", sizeof(wszNativeLanMan));
    /* @todo: change to native domain */
    wcstowc16s(wszNativeDomain, L"WORKGROUP", sizeof(wszNativeDomain));

    pResponseHeader->action = 0; // No guest access for now
    pResponseHeader->securityBlobLength = (USHORT)ulReplySecurityBlobLength;

    ntStatus = MarshallSessionSetupResponseData(
                    pSmbResponse->pData,
                    pSmbResponse->bufferLen - pSmbResponse->bufferUsed,
                    (pSmbResponse->pData - (uint8_t *) pSmbResponse->pSMBHeader) % 2,
                    &ulPackageByteCount,
                    pReplySecurityBlob,
                    pResponseHeader->securityBlobLength,
                    wszNativeOS,
                    wszNativeLanMan,
                    wszNativeDomain);
    BAIL_ON_NT_STATUS(ntStatus);

    assert(ulPackageByteCount <= UINT16_MAX);
    pResponseHeader->byteCount = (uint16_t) ulPackageByteCount;
    pSmbResponse->bufferUsed += ulPackageByteCount;

    ntStatus = SMBPacketUpdateAndXOffset(pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketMarshallFooter(pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    if (!SrvGssNegotiateIsComplete(pConnection->hGssContext, pConnection->hGssNegotiate))
    {
        pSmbResponse->pSMBHeader->error = STATUS_MORE_PROCESSING_REQUIRED;
    }

    *ppSmbResponse = pSmbResponse;

cleanup:

    if (pReplySecurityBlob)
    {
        SrvFreeMemory(pReplySecurityBlob);
    }

    return ntStatus;

error:

    *ppSmbResponse = NULL;

    if (pSmbResponse)
    {
        SMBPacketRelease(
            pConnection->hPacketAllocator,
            pSmbResponse);
    }

    goto cleanup;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
