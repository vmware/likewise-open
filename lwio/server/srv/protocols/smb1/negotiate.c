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

NTSTATUS
SrvBuildNegotiateResponse_SMB_V1_NTLM_0_12(
    PLWIO_SRV_CONNECTION pConnection,
    PSMB_PACKET          pSmbRequest,
    USHORT               idxDialect,
    PSMB_PACKET*         ppSmbResponse
    )
{
    NTSTATUS ntStatus = 0;
    NEGOTIATE_RESPONSE_HEADER* pResponseHeader = NULL;
    time_t    curTime   = 0L;
    LONG64    llUTCTime = 0LL;
    uint16_t  byteCount = 0;
    uint8_t*  pDataCursor = NULL;
    PSRV_PROPERTIES pServerProperties = &pConnection->serverProperties;
    PSMB_PACKET pSmbResponse = NULL;
    PWSTR     pwszHostname = NULL;

    ntStatus = SMBPacketAllocate(
                    pConnection->hPacketAllocator,
                    &pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketBufferAllocate(
                    pConnection->hPacketAllocator,
                    (64 * 1024) + 4096,
                    &pSmbResponse->pRawBuffer,
                    &pSmbResponse->bufferLen);
    BAIL_ON_NT_STATUS(ntStatus);

    if ((pSmbRequest->pSMBHeader->flags2 & FLAG2_EXT_SEC) == 0)
    {
        pServerProperties->Capabilities &= ~CAP_EXTENDED_SECURITY;
    }

    ntStatus = SMBPacketMarshallHeader(
                pSmbResponse->pRawBuffer,
                pSmbResponse->bufferLen,
                COM_NEGOTIATE,
                0,
                TRUE,
                pSmbRequest->pSMBHeader->tid,
                SMB_V1_GET_PROCESS_ID(pSmbRequest->pSMBHeader),
                0,
                pSmbRequest->pSMBHeader->mid,
                FALSE,
                pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);
    if ((pServerProperties->Capabilities & CAP_EXTENDED_SECURITY) == 0)
    {
        pSmbResponse->pSMBHeader->flags2 &= ~FLAG2_EXT_SEC;
    }

    pSmbResponse->pSMBHeader->wordCount = 17;

    pResponseHeader = (NEGOTIATE_RESPONSE_HEADER*)pSmbResponse->pParams;
    pSmbResponse->pData = pSmbResponse->pParams + sizeof(NEGOTIATE_RESPONSE_HEADER);
    pSmbResponse->bufferUsed += sizeof(NEGOTIATE_RESPONSE_HEADER);

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

    ntStatus = WireSMBUTimeToTimeZone(curTime, &pResponseHeader->serverTimeZone);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = WireSMBUTimetoNTTime(curTime, FALSE, &llUTCTime);
    BAIL_ON_NT_STATUS(ntStatus);

    pResponseHeader->systemTimeLow = llUTCTime & 0xFFFFFFFFLL;
    pResponseHeader->systemTimeHigh = (llUTCTime & 0xFFFFFFFF00000000LL) >> 32;

    pDataCursor = pSmbResponse->pData;
    if (pResponseHeader->capabilities & CAP_EXTENDED_SECURITY)
    {
        PBYTE pNegHintsBlob = NULL; /* Do not free */
        ULONG ulNegHintsLength = 0;

        pResponseHeader->encryptionKeyLength = 0;

        memcpy(pDataCursor, pServerProperties->GUID, sizeof(pServerProperties->GUID));
        pDataCursor += sizeof(pServerProperties->GUID);

        byteCount += sizeof(pServerProperties->GUID);

        ntStatus = SrvGssNegHints(&pNegHintsBlob, &ulNegHintsLength);

        /* Microsoft clients ignore the security blob on the neg prot response
           so don't fail here if we can't get a negHintsBlob */

        if (ntStatus == STATUS_SUCCESS)
        {
            memcpy(pDataCursor, pNegHintsBlob, ulNegHintsLength);
            pDataCursor += ulNegHintsLength;
            byteCount += ulNegHintsLength;
        }
    }
    else
    {
        WCHAR wszWorkgroup[] = SRV_NATIVE_DOMAIN_W;
        CHAR  szHostname[HOST_NAME_MAX];
        PWSTR pwszDomain = pConnection->clientProperties.pwszNativeDomain;

        if (!pwszDomain)
        {
            pwszDomain = &wszWorkgroup[0];
        }

        if (gethostname(szHostname, HOST_NAME_MAX) == -1)
        {
            ntStatus = LwErrnoToNtStatus(errno);
            BAIL_ON_NT_STATUS(ntStatus);
        }

        pResponseHeader->encryptionKeyLength =
            sizeof(pConnection->ServerChallenge);

        // Generate challenge and remember it in connection
        if (!RAND_bytes( pConnection->ServerChallenge,
                         sizeof(pConnection->ServerChallenge)))
        {
            ntStatus = STATUS_INTERNAL_ERROR;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        RtlCopyMemory(pDataCursor, &pConnection->ServerChallenge,
                      sizeof(pConnection->ServerChallenge));
        pDataCursor += sizeof(pConnection->ServerChallenge);
        byteCount += sizeof(pConnection->ServerChallenge);

        // Add domain name
        {
            size_t sDomainLen = (wc16slen(pwszDomain)+1) * sizeof(wchar16_t);

            RtlCopyMemory(pDataCursor, pwszDomain, sDomainLen);
            pDataCursor += sDomainLen;
            byteCount += sDomainLen;
        }

        // Add hostname
        {
            size_t sHostnameLen = 0;

            ntStatus = SrvMbsToWc16s(szHostname, &pwszHostname);
            BAIL_ON_NT_STATUS(ntStatus);

            sHostnameLen = (wc16slen(pwszHostname)+1) * sizeof(wchar16_t);
            RtlCopyMemory(pDataCursor, pwszHostname, sHostnameLen);
            pDataCursor += sHostnameLen;
            byteCount += sHostnameLen;
        }
    }

    pResponseHeader->byteCount = byteCount;
    pSmbResponse->bufferUsed += byteCount;

    ntStatus = SMBPacketMarshallFooter(pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppSmbResponse = pSmbResponse;

cleanup:

    SRV_SAFE_FREE_MEMORY(pwszHostname);

    return ntStatus;

error:

    *ppSmbResponse = NULL;

    if (pSmbResponse)
    {
        SMBPacketRelease(pConnection->hPacketAllocator, pSmbResponse);
    }

    goto cleanup;
}

NTSTATUS
SrvBuildNegotiateResponse_SMB_V1_Invalid(
    PLWIO_SRV_CONNECTION pConnection,
    PSMB_PACKET  pSmbRequest,
    PSMB_PACKET* ppSmbResponse
    )
{
    NTSTATUS ntStatus = 0;
    NEGOTIATE_INVALID_RESPONSE_HEADER* pResponseHeader = NULL;
    PSMB_PACKET pSmbResponse = NULL;

    ntStatus = SMBPacketAllocate(
                    pConnection->hPacketAllocator,
                    &pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketBufferAllocate(
                    pConnection->hPacketAllocator,
                    (64 * 1024) + 4096,
                    &pSmbResponse->pRawBuffer,
                    &pSmbResponse->bufferLen);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketMarshallHeader(
                pSmbResponse->pRawBuffer,
                pSmbResponse->bufferLen,
                COM_NEGOTIATE,
                0,
                TRUE,
                0,
                pSmbRequest->pSMBHeader->tid,
                0,
                0,
                FALSE,
                pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);
    if ((pConnection->serverProperties.Capabilities & CAP_EXTENDED_SECURITY) == 0)
    {
        pSmbResponse->pSMBHeader->flags2 &= ~FLAG2_EXT_SEC;
    }

    pSmbResponse->pSMBHeader->wordCount = 1;
    pResponseHeader = (NEGOTIATE_INVALID_RESPONSE_HEADER*)pSmbResponse->pParams;
    pSmbResponse->pData = pSmbResponse->pParams + sizeof(NEGOTIATE_INVALID_RESPONSE_HEADER);
    pSmbResponse->bufferUsed += sizeof(NEGOTIATE_INVALID_RESPONSE_HEADER);

    pResponseHeader->dialectIndex = 0xFF;
    pResponseHeader->byteCount = 0;

    ntStatus = SMBPacketMarshallFooter(pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppSmbResponse = pSmbResponse;

cleanup:

    return ntStatus;

error:

    *ppSmbResponse = NULL;

    if (pSmbResponse)
    {
        SMBPacketRelease(pConnection->hPacketAllocator, pSmbResponse);
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
