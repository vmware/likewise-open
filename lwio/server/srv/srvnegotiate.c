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

typedef NTSTATUS (*PFN_SRV_BUILD_NEGOTIATE_RESPONSE)(
                        PSMB_SRV_CONNECTION pConnection,
			PSMB_PACKET         pSmbRequest,
                        uint16_t            idxDialect,
                        PSMB_PACKET         pPacket
                        );

typedef struct _SRV_NEGOTIATE_RESPONSE_HANDLER
{
    PCSTR pszDialectName;
    PFN_SRV_BUILD_NEGOTIATE_RESPONSE pfnNegotiateResponseBuilder;
} SRV_NEGOTIATE_RESPONSE_HANDLER, *PSRV_NEGOTIATE_RESPONSE_HANDLER;

static
NTSTATUS
SrvBuildNegotiateResponseByDialect_NTLM_0_12(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    uint16_t            idxDialect,
    PSMB_PACKET         pPacket
    );

static
NTSTATUS
SrvBuildNegotiateResponseByDialect_Invalid(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    uint16_t            idxDialect,
    PSMB_PACKET         pPacket
    );

#define SRV_NEGOTIATE_DIALECT_NTLM_0_12 "NT LM 0.12"

static SRV_NEGOTIATE_RESPONSE_HANDLER gNegotiateResponseHandlers[] =
{
    { SRV_NEGOTIATE_DIALECT_NTLM_0_12, &SrvBuildNegotiateResponseByDialect_NTLM_0_12 }
};

static
NTSTATUS
SrvMarshallNegotiateResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    PSTR*               ppDialectArray,
    ULONG               ulNumDialects,
    PSMB_PACKET*        ppSmbResponse
    );

static
int
SrvChooseDialect(
    PSTR* ppszDialectArray,
    ULONG ulNumDialects
    );

static
NTSTATUS
SrvBuildNegotiateResponseForDialect(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    uint16_t            idxDialect,
    PCSTR               pszDialect,
    PSMB_PACKET         pPacket
    );

NTSTATUS
SrvProcessNegotiate(
    PLWIO_SRV_CONTEXT pContext,
    PSMB_PACKET*      ppSmbResponse
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_PACKET pSmbResponse = NULL;
    PSTR  pszDialectArray[128];
    ULONG ulNumDialects = 128;
    PSMB_SRV_CONNECTION pConnection = pContext->pConnection;
    PSMB_PACKET         pSmbRequest = pContext->pRequest;

    ntStatus = UnmarshallNegotiateRequest(
                    pSmbRequest->pParams,
                    pSmbRequest->bufferLen - pSmbRequest->bufferUsed,
                    (uint8**)&pszDialectArray,
                    &ulNumDialects);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvMarshallNegotiateResponse(
                    pConnection,
                    pSmbRequest,
                    pszDialectArray,
                    ulNumDialects,
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
            pConnection->hPacketAllocator,
            pSmbResponse);
    }

    goto cleanup;
}

static
NTSTATUS
SrvMarshallNegotiateResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    PSTR*               ppDialectArray,
    ULONG               ulNumDialects,
    PSMB_PACKET*        ppSmbResponse
    )
{
    NTSTATUS    ntStatus = 0;
    PSMB_PACKET pSmbResponse = NULL;
    int         idxDialect = 0;

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

    idxDialect = SrvChooseDialect(
                            ppDialectArray,
                            ulNumDialects);

    ntStatus = SrvBuildNegotiateResponseForDialect(
                    pConnection,
		    pSmbRequest,
                    idxDialect,
                    ((idxDialect < 0) ? "" : ppDialectArray[idxDialect]),
                    pSmbResponse);
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
int
SrvChooseDialect(
    PSTR* ppszDialectArray,
    ULONG ulNumDialects
    )
{
    int idx = -1;
    ULONG iDialect = 0;

    for (; iDialect < ulNumDialects; iDialect++)
    {
        if (!strcmp(ppszDialectArray[iDialect], SRV_NEGOTIATE_DIALECT_NTLM_0_12))
        {
            idx = iDialect;
            break;
        }
    }

    return idx;
}

static
NTSTATUS
SrvBuildNegotiateResponseForDialect(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    uint16_t            idxDialect,
    PCSTR               pszDialect,
    PSMB_PACKET         pPacket
    )
{
    NTSTATUS ntStatus = 0;
    PFN_SRV_BUILD_NEGOTIATE_RESPONSE pfnNegotiateResponseBuilder = &SrvBuildNegotiateResponseByDialect_Invalid;

    if (!IsNullOrEmptyString(pszDialect))
    {
        uint16_t iDialect = 0;
        uint16_t nDialects = 0;

        nDialects = sizeof(gNegotiateResponseHandlers)/sizeof(SRV_NEGOTIATE_RESPONSE_HANDLER);

        for (; iDialect < nDialects; iDialect++)
        {
            PSRV_NEGOTIATE_RESPONSE_HANDLER pHandler = &gNegotiateResponseHandlers[iDialect];

            if (!strcmp(pHandler->pszDialectName, pszDialect))
            {
                pfnNegotiateResponseBuilder = pHandler->pfnNegotiateResponseBuilder;
                break;
            }
        }
    }

    assert (pfnNegotiateResponseBuilder != NULL);

    ntStatus = pfnNegotiateResponseBuilder(
                        pConnection,
			pSmbRequest,
                        idxDialect,
                        pPacket);
    BAIL_ON_NT_STATUS(ntStatus);

error:

    return ntStatus;
}

static
NTSTATUS
SrvBuildNegotiateResponseByDialect_NTLM_0_12(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET pSmbRequest,
    uint16_t    idxDialect,
    PSMB_PACKET pPacket
    )
{
    NTSTATUS ntStatus = 0;
    NEGOTIATE_RESPONSE_HEADER* pResponseHeader = NULL;
    time_t    curTime;
    ULONG64   llUTCTime;
    uint16_t  byteCount = 0;
    uint8_t*  pDataCursor = NULL;
    PSRV_PROPERTIES pServerProperties = &pConnection->serverProperties;
    PBYTE     pSessionKey = 0;
    ULONG     ulSessionKeyLength = 0;

    ntStatus = SMBPacketMarshallHeader(
                pPacket->pRawBuffer,
                pPacket->bufferLen,
                COM_NEGOTIATE,
                0,
                TRUE,
                0,
                pSmbRequest->pSMBHeader->tid,
                pSmbRequest->pSMBHeader->pid,
                0,
                FALSE,
                pPacket);
    BAIL_ON_NT_STATUS(ntStatus);

    pPacket->pSMBHeader->wordCount = 17;

    pResponseHeader = (NEGOTIATE_RESPONSE_HEADER*)pPacket->pParams;
    pPacket->pData = pPacket->pParams + sizeof(NEGOTIATE_RESPONSE_HEADER);
    pPacket->bufferUsed += sizeof(NEGOTIATE_RESPONSE_HEADER);

    pResponseHeader->dialectIndex = idxDialect;

    pResponseHeader->securityMode = 0;
    if (pServerProperties->preferredSecurityMode == SMB_SECURITY_MODE_USER)
    {
        pResponseHeader->securityMode |= 0x1; // User level security
    }
    if (pServerProperties->bEncryptPasswords)
    {
        pResponseHeader->securityMode |= 0x2;
    }
    if (pServerProperties->bEnableSecuritySignatures)
    {
        pResponseHeader->securityMode |= 0x4;
    }
    if (pServerProperties->bRequireSecuritySignatures)
    {
        pResponseHeader->securityMode |= 0x8;
    }

    pResponseHeader->maxMpxCount = pServerProperties->MaxMpxCount;
    pResponseHeader->maxNumberVcs = pServerProperties->MaxNumberVCs;
    pResponseHeader->maxBufferSize = pServerProperties->MaxBufferSize;
    pResponseHeader->maxRawSize = pServerProperties->MaxRawSize;
    pResponseHeader->sessionKey = 0;
    pResponseHeader->capabilities = pServerProperties->Capabilities;

    curTime = time(NULL);

    pResponseHeader->serverTimeZone = (mktime(gmtime(&curTime)) - curTime)/60;

    llUTCTime = (curTime + 11644473600LL) * 10000000LL;

    pResponseHeader->systemTimeLow = llUTCTime & 0xFFFFFFFFLL;
    pResponseHeader->systemTimeHigh = (llUTCTime & 0xFFFFFFFF00000000LL) >> 32;


    pResponseHeader->encryptionKeyLength = 0;

    pDataCursor = pPacket->pData;
    if (pResponseHeader->capabilities & CAP_EXTENDED_SECURITY)
    {
        memcpy(pDataCursor, pServerProperties->GUID, sizeof(pServerProperties->GUID));
        pDataCursor += sizeof(pServerProperties->GUID);

        byteCount += sizeof(pServerProperties->GUID);

        ntStatus = SrvGssBeginNegotiate(
                        pConnection->hGssContext,
                        &pConnection->hGssNegotiate);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvGssNegotiate(
                        pConnection->hGssContext,
                        pConnection->hGssNegotiate,
                        NULL,
                        0,
                        &pSessionKey,
                        &ulSessionKeyLength);
        BAIL_ON_NT_STATUS(ntStatus);

        if (ulSessionKeyLength)
        {
            memcpy(pDataCursor, pSessionKey, ulSessionKeyLength);
            pDataCursor += ulSessionKeyLength;
            byteCount += ulSessionKeyLength;
        }
    }

    pResponseHeader->byteCount = byteCount;
    pPacket->bufferUsed += byteCount;

    ntStatus = SMBPacketMarshallFooter(pPacket);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    if (pSessionKey)
    {
        LwRtlMemoryFree(pSessionKey);
    }

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
SrvBuildNegotiateResponseByDialect_Invalid(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET pSmbRequest,
    uint16_t    idxDialect,
    PSMB_PACKET pPacket
    )
{
    NTSTATUS ntStatus = 0;
    NEGOTIATE_INVALID_RESPONSE_HEADER* pResponseHeader = NULL;

    ntStatus = SMBPacketMarshallHeader(
                pPacket->pRawBuffer,
                pPacket->bufferLen,
                COM_NEGOTIATE,
                0,
                TRUE,
                0,
                pSmbRequest->pSMBHeader->tid,
                0,
                0,
                FALSE,
                pPacket);
    BAIL_ON_NT_STATUS(ntStatus);

    pPacket->pSMBHeader->wordCount = 1;
    pResponseHeader = (NEGOTIATE_INVALID_RESPONSE_HEADER*)pPacket->pParams;
    pPacket->pData = pPacket->pParams + sizeof(NEGOTIATE_INVALID_RESPONSE_HEADER);
    pPacket->bufferUsed += sizeof(NEGOTIATE_INVALID_RESPONSE_HEADER);

    pResponseHeader->dialectIndex = 0xFF;
    pResponseHeader->byteCount = 0;

    ntStatus = SMBPacketMarshallFooter(pPacket);
    BAIL_ON_NT_STATUS(ntStatus);

error:

    return ntStatus;
}


