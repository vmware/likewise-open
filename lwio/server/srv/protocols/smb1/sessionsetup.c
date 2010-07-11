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
    PSRV_MESSAGE_SMB_V1  pSmbRequest,
    PBYTE*               ppSecurityBlob,
    PULONG               pulSecurityBlobLength
    );

static
NTSTATUS
SrvMarshallSessionSetupResponse(
    PSRV_EXEC_CONTEXT    pExecContext,
    PBYTE                pSecurityBlob,
    ULONG                ulSecurityBlobLength
    );


NTSTATUS
SrvProcessSessionSetup(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus      = 0;
    PLWIO_SRV_CONNECTION       pConnection   = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1      = pCtxProtocol->pSmb1Context;
    ULONG                      iMsg          = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1        pSmbRequest   = &pCtxSmb1->pRequests[iMsg];
    PBYTE                      pSecurityBlob        = NULL; // Do Not Free
    ULONG                      ulSecurityBlobLength = 0;
    PBYTE                      pInitSecurityBlob        = NULL;
    ULONG                      ulInitSecurityBlobLength = 0;
    LW_MAP_SECURITY_GSS_CONTEXT hContextHandle = NULL;
    BOOLEAN                    bGssNegotiateLocked = FALSE;
    PSTR                       pszClientPrincipalName = NULL;
    PLWIO_SRV_SESSION          pSessionExisting       = NULL;
    PSRV_MESSAGE_SMB_V1        pSmbResponse = &pCtxSmb1->pResponses[iMsg];
    BOOLEAN                    bLoggedInAsGuest = FALSE;
    BOOLEAN                    bInitializedSessionKey = FALSE;

    ntStatus = SrvUnmarshallSessionSetupRequest(
                    pConnection,
                    pSmbRequest,
                    &pSecurityBlob,
                    &ulSecurityBlobLength);
    BAIL_ON_NT_STATUS(ntStatus);

    LWIO_LOCK_MUTEX(bGssNegotiateLocked, &pConnection->mutexGssNegotiate);

    if (pConnection->hGssNegotiate == NULL)
    {
        ntStatus = SrvGssBeginNegotiate(
                       pConnection->hGssContext,
                       &pConnection->hGssNegotiate);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvGssNegotiate(
                       pConnection->hGssContext,
                       pConnection->hGssNegotiate,
                       NULL,
                       0,
                       &pInitSecurityBlob,
                       &ulInitSecurityBlobLength);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SrvMarshallSessionSetupResponse(
                    pExecContext,
                    pSecurityBlob,
                    ulSecurityBlobLength);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pSmbRequest->pHeader->uid)
    {
        ntStatus = SrvConnectionFindSession_SMB_V1(
                        pCtxSmb1,
                        pConnection,
                        pSmbRequest->pHeader->uid,
                        &pCtxSmb1->pSession);
        BAIL_ON_NT_STATUS(ntStatus);

        pSessionExisting = pCtxSmb1->pSession;

        SRV_LOG_VERBOSE(
                pExecContext->pLogContext,
                SMB_PROTOCOL_VERSION_1,
                pSmbRequest->pHeader->command,
                "Using existing session (Id:%u)",
                pSessionExisting->uid);
    }
    else
    {
        ntStatus = SrvConnectionCreateSession(pConnection, &pCtxSmb1->pSession);
        BAIL_ON_NT_STATUS(ntStatus);

        SRV_LOG_VERBOSE(
                pExecContext->pLogContext,
                SMB_PROTOCOL_VERSION_1,
                pSmbRequest->pHeader->command,
                "Created session (Id:%u)",
                pCtxSmb1->pSession->uid);
    }

    pSmbResponse->pHeader->uid = pCtxSmb1->pSession->uid;

    if (SrvGssNegotiateIsComplete(
            pConnection->hGssContext,
            pConnection->hGssNegotiate))
    {
        ntStatus = SrvSetStatSessionInfo(pExecContext, pCtxSmb1->pSession);
        BAIL_ON_NT_STATUS(ntStatus);

        if (!pExecContext->pConnection->pSessionKey)
        {
            ntStatus = SrvGssGetSessionDetails(
                           pConnection->hGssContext,
                           pConnection->hGssNegotiate,
                           &pConnection->pSessionKey,
                           &pConnection->ulSessionKeyLength,
                           &pszClientPrincipalName,
                           &hContextHandle);

            bInitializedSessionKey = TRUE;
        }
        else
        {
            ntStatus = SrvGssGetSessionDetails(
                           pConnection->hGssContext,
                           pConnection->hGssNegotiate,
                           NULL,
                           NULL,
                           &pszClientPrincipalName,
                           &hContextHandle);
        }
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvIoSecCreateSecurityContext(
                       &pCtxSmb1->pSession->pIoSecurityContext,
                       &bLoggedInAsGuest,
                       hContextHandle,
                       pszClientPrincipalName);
        BAIL_ON_NT_STATUS(ntStatus);

        if (bLoggedInAsGuest)
        {
            PBYTE pSessionBuffer = pSmbResponse->pBuffer + pSmbResponse->usHeaderSize;

            ((PSESSION_SETUP_RESPONSE_HEADER)pSessionBuffer)->action = 0x1;

            SrvSessionSetUserFlags(pCtxSmb1->pSession, SMB_SESSION_FLAG_GUEST);

            // The session key for this connection has to come from an
            // authenticated session

            if (bInitializedSessionKey)
            {
                SrvFreeMemory(pConnection->pSessionKey);
                pConnection->pSessionKey = NULL;
                pConnection->ulSessionKeyLength = 0;
            }

            SRV_LOG_VERBOSE(
                    pExecContext->pLogContext,
                    SMB_PROTOCOL_VERSION_1,
                    pSmbRequest->pHeader->command,
                    "Session (Id:%u) is logged in as guest",
                    pCtxSmb1->pSession->uid);
        }

        // Go ahead and close out this GSS negotiate state so we can
        // handle another Session setup.

        SrvGssEndNegotiate(
            pConnection->hGssContext,
            pConnection->hGssNegotiate);
        pConnection->hGssNegotiate = NULL;

        if (pszClientPrincipalName)
        {
            ntStatus = SrvSessionSetPrincipalName(
                            pCtxSmb1->pSession,
                            pszClientPrincipalName);
            BAIL_ON_NT_STATUS(ntStatus);
        }

        if (pConnection->pOEMConnection)
        {
            SRV_OEM_SESSION_ID sessionId =
            {
                    .ver   = SMB_PROTOCOL_VERSION_1,
                    .id    =
                        {
                            .usUid = pCtxSmb1->pSession->uid
                        }
            };

            ntStatus = SrvOEMCreateSession(
                            pConnection->pOEMConnection,
                            pConnection->ulOEMConnectionLength,
                            &sessionId,
                            pCtxSmb1->pSession->pIoSecurityContext,
                            &pCtxSmb1->pSession->pOEMSession,
                            &pCtxSmb1->pSession->ulOEMSessionLength);
            BAIL_ON_NT_STATUS(ntStatus);
        }

        SrvConnectionSetState(pConnection, LWIO_SRV_CONN_STATE_READY);
    }

cleanup:

    LWIO_UNLOCK_MUTEX(bGssNegotiateLocked, &pConnection->mutexGssNegotiate);

    SRV_SAFE_FREE_MEMORY(pInitSecurityBlob);
    SRV_SAFE_FREE_MEMORY(pszClientPrincipalName);

    if (pSessionExisting)
    {
        SrvSessionRelease(pSessionExisting);
    }

    return ntStatus;

error:

    if (pConnection->hGssNegotiate)
    {
        SrvGssEndNegotiate(
                pConnection->hGssContext,
                pConnection->hGssNegotiate);

        pConnection->hGssNegotiate = NULL;
    }

    goto cleanup;
}

static
NTSTATUS
SrvUnmarshallSessionSetupRequest(
    PLWIO_SRV_CONNECTION pConnection,
    PSRV_MESSAGE_SMB_V1  pSmbRequest,
    PBYTE*               ppSecurityBlob,
    PULONG               pulSecurityBlobLength
    )
{
    NTSTATUS ntStatus = 0;
    SESSION_SETUP_REQUEST_HEADER* pHeader = NULL; // Do not free
    PBYTE pSecurityBlob    = NULL; // Do not free
    PWSTR pwszNativeOS     = NULL; // Do not free
    PWSTR pwszNativeLanMan = NULL; // Do not free
    PWSTR pwszNativeDomain = NULL; // Do not free
    PBYTE pBuffer          = pSmbRequest->pBuffer + pSmbRequest->usHeaderSize;
    ULONG ulOffset         = pSmbRequest->usHeaderSize;
    ULONG ulBytesAvailable = pSmbRequest->ulMessageSize - pSmbRequest->usHeaderSize;

    ntStatus = UnmarshallSessionSetupRequest(
                    pBuffer,
                    ulBytesAvailable,
                    ulOffset % 2,
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
    PSRV_EXEC_CONTEXT pExecContext,
    PBYTE             pSecurityBlob,
    ULONG             ulSecurityBlobLength
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    ULONG                      iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1        pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V1        pSmbResponse = &pCtxSmb1->pResponses[iMsg];
    SESSION_SETUP_RESPONSE_HEADER* pResponseHeader = NULL;
    PBYTE     pReplySecurityBlob = NULL;
    ULONG     ulReplySecurityBlobLength = 0;
    wchar16_t wszNativeOS[] = {'U', 'n', 'i', 'x', 0 };
    wchar16_t wszNativeLanMan[] = {'L','i','k','e','w','i','s','e',' ','C','I', 'F', 'S', 0 };
    wchar16_t wszNativeDomain[] = { 0 };
    PBYTE pOutBuffer         = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable   = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset           = 0;
    ULONG  ulBytesUsed       = 0;
    ULONG ulTotalBytesUsed   = 0;

    ntStatus = SrvGssNegotiate(
                    pConnection->hGssContext,
                    pConnection->hGssNegotiate,
                    pSecurityBlob,
                    ulSecurityBlobLength,
                    &pReplySecurityBlob,
                    &ulReplySecurityBlobLength);
    BAIL_ON_NT_STATUS(ntStatus);

    if (!pSmbResponse->ulSerialNum)
    {
        ntStatus = SrvMarshalHeader_SMB_V1(
                        pOutBuffer,
                        ulOffset,
                        ulBytesAvailable,
                        COM_SESSION_SETUP_ANDX,
                        STATUS_SUCCESS,
                        TRUE,  /* is response */
                        pSmbRequest->pHeader->tid,
                        SMB_V1_GET_PROCESS_ID(pSmbRequest->pHeader),
                        pSmbRequest->pHeader->uid,
                        pSmbRequest->pHeader->mid,
                        pConnection->serverProperties.bRequireSecuritySignatures,
                        &pSmbResponse->pHeader,
                        &pSmbResponse->pWordCount,
                        &pSmbResponse->pAndXHeader,
                        &pSmbResponse->usHeaderSize);
    }
    else
    {
        ntStatus = SrvMarshalHeaderAndX_SMB_V1(
                        pOutBuffer,
                        ulOffset,
                        ulBytesAvailable,
                        COM_SESSION_SETUP_ANDX,
                        &pSmbResponse->pWordCount,
                        &pSmbResponse->pAndXHeader,
                        &pSmbResponse->usHeaderSize);
    }
    BAIL_ON_NT_STATUS(ntStatus);

    ulTotalBytesUsed += pSmbResponse->usHeaderSize;
    ulOffset         += pSmbResponse->usHeaderSize;
    ulBytesAvailable -= pSmbResponse->usHeaderSize;
    pOutBuffer       += pSmbResponse->usHeaderSize;

    *pSmbResponse->pWordCount = 4;

    if (ulBytesAvailable < sizeof(SESSION_SETUP_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pResponseHeader = (PSESSION_SETUP_RESPONSE_HEADER)pOutBuffer;

    ulTotalBytesUsed += sizeof(SESSION_SETUP_RESPONSE_HEADER);
    ulOffset         += sizeof(SESSION_SETUP_RESPONSE_HEADER);
    ulBytesAvailable -= sizeof(SESSION_SETUP_RESPONSE_HEADER);
    pOutBuffer       += sizeof(SESSION_SETUP_RESPONSE_HEADER);

    /* TODO : change to native domain */

    pResponseHeader->action = 0x0;
    pResponseHeader->securityBlobLength = (USHORT)ulReplySecurityBlobLength;

    ntStatus = MarshallSessionSetupResponseData(
                    pOutBuffer,
                    ulBytesAvailable,
                    ulOffset % 2,
                    &ulBytesUsed,
                    pReplySecurityBlob,
                    pResponseHeader->securityBlobLength,
                    &wszNativeOS[0],
                    &wszNativeLanMan[0],
                    &wszNativeDomain[0]);
    BAIL_ON_NT_STATUS(ntStatus);

    assert(ulBytesUsed <= UINT16_MAX);
    pResponseHeader->byteCount = (USHORT) ulBytesUsed;

    // pOutBuffer       += ulPackageByteCount;
    // ulOffset         += ulPackageByteCount;
    // ulBytesAvailable -= ulPackageByteCount;
    ulTotalBytesUsed += ulBytesUsed;

    if (!SrvGssNegotiateIsComplete(
            pConnection->hGssContext,
            pConnection->hGssNegotiate))
    {
        pSmbResponse->pHeader->error = STATUS_MORE_PROCESSING_REQUIRED;
    }

    pSmbResponse->ulMessageSize = ulTotalBytesUsed;

cleanup:

    if (pReplySecurityBlob)
    {
        SrvFreeMemory(pReplySecurityBlob);
    }

    return ntStatus;

error:

    if (ulTotalBytesUsed)
    {
        pSmbResponse->pHeader = NULL;
        pSmbResponse->pAndXHeader = NULL;
        memset(pSmbResponse->pBuffer, 0, ulTotalBytesUsed);
    }

    pSmbResponse->ulMessageSize = 0;

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
