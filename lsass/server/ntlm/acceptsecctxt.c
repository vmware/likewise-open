/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
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
 *        acceptsecctxt.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        AcceptSecurityContext client wrapper API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
 */

#include "ntlmsrvapi.h"
#include "lwsecurityidentifier.h"
#include "lsasrvapi.h"

DWORD
NtlmServerAcceptSecurityContext(
    IN LWMsgAssoc* pAssoc,
    IN PNTLM_CRED_HANDLE phCredential,
    IN OUT PNTLM_CONTEXT_HANDLE phContext,
    IN PSecBufferDesc pInput,
    IN DWORD fContextReq,
    IN DWORD TargetDataRep,
    IN OUT PNTLM_CONTEXT_HANDLE phNewContext,
    IN OUT PSecBufferDesc pOutput,
    OUT PDWORD  pfContextAttr,
    OUT PTimeStamp ptsTimeStamp
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PNTLM_CONTEXT pNtlmContext = NULL;
    PNTLM_CONTEXT pNtlmCtxtOut = NULL;
    PNTLM_CONTEXT pNtlmCtxtChlng = NULL;
    NTLM_CONTEXT_HANDLE ContextHandle = NULL;
    PNTLM_NEGOTIATE_MESSAGE pNegMsg = NULL;
    PNTLM_RESPONSE_MESSAGE pRespMsg = NULL;
    DWORD dwMessageSize = 0;
    BYTE SessionKey[NTLM_SESSION_KEY_SIZE] = {0};

    ptsTimeStamp = 0;

    if (phContext)
    {
        pNtlmContext = *phContext;
    }

    if (!pNtlmContext)
    {
        dwError = NtlmGetMessageFromSecBufferDesc(
            pInput,
            &dwMessageSize,
            (PVOID*)&pNegMsg
            );
        BAIL_ON_LW_ERROR(dwError);

        dwError = NtlmCreateChallengeContext(
            pNegMsg,
            phCredential,
            &pNtlmCtxtOut);

        BAIL_ON_LW_ERROR(dwError);

        dwError = NtlmCopyContextToSecBufferDesc(pNtlmCtxtOut, pOutput);
        BAIL_ON_LW_ERROR(dwError);

        dwError = LW_WARNING_CONTINUE_NEEDED;
    }
    else
    {
        // The only time we should get a context handle passed in is when
        // we are validating a challenge and we need to look up the original
        // challenge sent
        pNtlmCtxtChlng = pNtlmContext;

        // In this case we need to grab the response message sent in
        dwError = NtlmGetMessageFromSecBufferDesc(
            pInput,
            &dwMessageSize,
            (PVOID*)&pRespMsg
            );
        BAIL_ON_LW_ERROR(dwError);

        dwError = NtlmValidateResponse(
            pAssoc,
            pRespMsg,
            dwMessageSize,
            pNtlmCtxtChlng,
            SessionKey);
        BAIL_ON_LW_ERROR(dwError);

        // The response has been validated and contains all the information we
        // are looking for; retain it... the original (challenge) context will
        // be freed when we return.
        dwError = NtlmCreateValidatedContext(
            pRespMsg,
            dwMessageSize,
            pNtlmCtxtChlng->NegotiatedFlags,
            SessionKey,
            NTLM_SESSION_KEY_SIZE,
            phCredential,
            &pNtlmCtxtOut
            );
        BAIL_ON_LW_ERROR(dwError);
    }

    //NtlmAddContext(pNtlmCtxtOut, &ContextHandle);
    ContextHandle = pNtlmCtxtOut;

cleanup:
    *phNewContext = ContextHandle;

    return(dwError);
error:
    if (ContextHandle)
    {
        NtlmReleaseContext(&ContextHandle);
    }
    else if (pNtlmCtxtOut)
    {
        NtlmReleaseContext(&pNtlmCtxtOut);
    }
    goto cleanup;
}

DWORD
NtlmCreateChallengeContext(
    IN PNTLM_NEGOTIATE_MESSAGE pNtlmNegMsg,
    IN PNTLM_CRED_HANDLE pCredHandle,
    OUT PNTLM_CONTEXT *ppNtlmContext
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PNTLM_CONTEXT pNtlmContext = NULL;
    DWORD dwMessageSize = 0;
    PNTLM_CHALLENGE_MESSAGE pMessage = NULL;
    PCSTR pServerName = NULL;
    PCSTR pDomainName = NULL;
    PCSTR pDnsServerName = NULL;
    PCSTR pDnsDomainName = NULL;

    *ppNtlmContext = NULL;

    dwError = NtlmCreateContext(pCredHandle, &pNtlmContext);
    BAIL_ON_LW_ERROR(dwError);

    NtlmGetCredentialInfo(
        *pCredHandle,
        NULL,
        NULL,
        NULL,
        &pServerName,
        &pDomainName,
        &pDnsServerName,
        &pDnsDomainName
        );

    dwError = NtlmCreateChallengeMessage(
        pNtlmNegMsg,
        pServerName,
        pDomainName,
        pDnsServerName,
        pDnsDomainName,
        (PBYTE)&gW2KSpoof,
        &dwMessageSize,
        &pMessage
        );

    BAIL_ON_LW_ERROR(dwError);

    pNtlmContext->NegotiatedFlags = pMessage->NtlmFlags;
    pNtlmContext->dwMessageSize = dwMessageSize;
    pNtlmContext->pMessage = pMessage;
    pNtlmContext->NtlmState = NtlmStateChallenge;

cleanup:
    *ppNtlmContext = pNtlmContext;
    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pMessage);

    if (pNtlmContext)
    {
        NtlmReleaseCredential(pCredHandle);
        LW_SAFE_FREE_MEMORY(pNtlmContext);
    }
    goto cleanup;
}

DWORD
NtlmCreateValidatedContext(
    IN PNTLM_RESPONSE_MESSAGE pNtlmRespMsg,
    IN DWORD dwMsgSize,
    IN DWORD NegotiatedFlags,
    IN PBYTE pSessionKey,
    IN DWORD dwSessionKeyLen,
    IN PNTLM_CRED_HANDLE pCredHandle,
    OUT PNTLM_CONTEXT *ppNtlmContext
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PNTLM_CONTEXT pNtlmContext = NULL;

    *ppNtlmContext = NULL;

    dwError = NtlmCreateContext(pCredHandle, &pNtlmContext);
    BAIL_ON_LW_ERROR(dwError);

    dwError = LwAllocateMemory(dwMsgSize, OUT_PPVOID(&pNtlmContext->pMessage));

    pNtlmContext->dwMessageSize = dwMsgSize;
    memcpy(pNtlmContext->pMessage, pNtlmRespMsg, dwMsgSize);
    pNtlmContext->NtlmState = NtlmStateResponse;

    pNtlmContext->NegotiatedFlags = NegotiatedFlags;

    memcpy(pNtlmContext->SessionKey, pSessionKey, NTLM_SESSION_KEY_SIZE);
    pNtlmContext->cbSessionKeyLen = dwSessionKeyLen;

cleanup:
    *ppNtlmContext = pNtlmContext;
    return dwError;

error:
    if (pNtlmContext)
    {
        NtlmReleaseCredential(pCredHandle);
        LW_SAFE_FREE_MEMORY(pNtlmContext);
    }
    goto cleanup;
}

DWORD
NtlmValidateResponse(
    IN LWMsgAssoc* pAssoc,
    IN PNTLM_RESPONSE_MESSAGE pRespMsg,
    IN DWORD dwRespMsgSize,
    IN PNTLM_CONTEXT pChlngCtxt,
    OUT BYTE pSessionKey[NTLM_SESSION_KEY_SIZE]
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PNTLM_CHALLENGE_MESSAGE pChlngMsg = NULL;
    LSA_AUTH_USER_PARAMS Params;
    PLSA_AUTH_USER_INFO pUserInfo = NULL;
    PBYTE pChallengeBuffer = NULL;
    PBYTE pLMRespBuffer = NULL;
    PBYTE pNTRespBuffer = NULL;
    LW_LSA_DATA_BLOB Challenge;
    LW_LSA_DATA_BLOB LMResp;
    LW_LSA_DATA_BLOB NTResp;
    PSTR pUserName = NULL;
    PSTR pDomainName = NULL;
    PSTR pWorkstation = NULL;
    HANDLE Handle = NULL;

    memset(&Params, 0, sizeof(Params));
    memset(&Challenge, 0, sizeof(Challenge));
    memset(&LMResp, 0, sizeof(LMResp));
    memset(&NTResp, 0, sizeof(NTResp));

    // sanity check
    if (!pRespMsg || ! pChlngCtxt)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LW_ERROR(dwError);
    }

    dwError = MAP_LWMSG_ERROR(
        lwmsg_assoc_get_session_data(pAssoc, OUT_PPVOID(&Handle)));
    BAIL_ON_LW_ERROR(dwError);

    pChlngMsg = (PNTLM_CHALLENGE_MESSAGE)pChlngCtxt->pMessage;

    dwError = LwAllocateMemory(
        NTLM_CHALLENGE_SIZE,
        OUT_PPVOID(&pChallengeBuffer));
    BAIL_ON_LW_ERROR(dwError);

    dwError = LwAllocateMemory(
        pRespMsg->LmResponse.usLength,
        OUT_PPVOID(&pLMRespBuffer));
    BAIL_ON_LW_ERROR(dwError);

    dwError = LwAllocateMemory(
        pRespMsg->NtResponse.usLength,
        OUT_PPVOID(&pNTRespBuffer));
    BAIL_ON_LW_ERROR(dwError);

    // The username, domain, and workstation values might come back as Unicode.
    // We could technically prevent this by not allowing NTLM_FLAG_UNICODE to be
    // set during the negotiation phase, but that seems like an odd restriction
    // for now.

    dwError = NtlmGetUserNameFromResponse(
        pRespMsg,
        pChlngMsg->NtlmFlags & NTLM_FLAG_UNICODE,
        &pUserName);
    BAIL_ON_LW_ERROR(dwError);

    dwError = NtlmGetDomainNameFromResponse(
        pRespMsg,
        pChlngMsg->NtlmFlags & NTLM_FLAG_UNICODE,
        &pDomainName);
    BAIL_ON_LW_ERROR(dwError);

    dwError = NtlmGetWorkstationFromResponse(
        pRespMsg,
        pChlngMsg->NtlmFlags & NTLM_FLAG_UNICODE,
        &pWorkstation);
    BAIL_ON_LW_ERROR(dwError);

    memcpy(
        pChallengeBuffer,
        pChlngMsg->Challenge,
        NTLM_CHALLENGE_SIZE);

    memcpy(
        pLMRespBuffer,
        (PBYTE)pRespMsg + pRespMsg->LmResponse.dwOffset,
        pRespMsg->LmResponse.usLength);

    memcpy(
        pNTRespBuffer,
        (PBYTE)pRespMsg + pRespMsg->NtResponse.dwOffset,
        pRespMsg->NtResponse.usLength);

    Challenge.dwLen = NTLM_CHALLENGE_SIZE;
    Challenge.pData = pChallengeBuffer;

    LMResp.dwLen = pRespMsg->LmResponse.usLength;
    LMResp.pData = pLMRespBuffer;

    NTResp.dwLen = pRespMsg->NtResponse.usLength;
    NTResp.pData = pNTRespBuffer;

    Params.AuthType = LSA_AUTH_CHAP;

    Params.pass.chap.pChallenge = &Challenge;
    Params.pass.chap.pLM_resp = &LMResp;
    Params.pass.chap.pNT_resp = &NTResp;

    Params.pszAccountName = pUserName;

    Params.pszDomain = pDomainName;

    Params.pszWorkstation = pWorkstation;

    dwError = LsaSrvAuthenticateUserEx(
        Handle,
        &Params,
        &pUserInfo
        );
    BAIL_ON_LW_ERROR(dwError);

    LW_ASSERT(pUserInfo->pSessionKey->dwLen == NTLM_SESSION_KEY_SIZE);
    memcpy(pSessionKey, pUserInfo->pSessionKey->pData, NTLM_SESSION_KEY_SIZE);

    // Free the pUserInfo for now... we may want to save this off later
    //
    dwError = LsaFreeAuthUserInfo(&pUserInfo);
    BAIL_ON_LW_ERROR(dwError);

cleanup:

    LW_SAFE_FREE_MEMORY(pChallengeBuffer);
    LW_SAFE_FREE_MEMORY(pLMRespBuffer);
    LW_SAFE_FREE_MEMORY(pNTRespBuffer);
    LW_SAFE_FREE_STRING(pUserName);
    LW_SAFE_FREE_STRING(pDomainName);
    LW_SAFE_FREE_STRING(pWorkstation);

    return dwError;
error:
    goto cleanup;
}

/******************************************************************************/
DWORD
NtlmGetDomainNameFromResponse(
    IN PNTLM_RESPONSE_MESSAGE pRespMsg,
    IN BOOLEAN bUnicode,
    OUT PSTR* ppDomainName
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PCHAR pName = NULL;
    DWORD dwNameLength = 0;
    PBYTE pBuffer = NULL;
    PNTLM_SEC_BUFFER pSecBuffer = &pRespMsg->AuthTargetName;
    DWORD nIndex = 0;

    *ppDomainName = NULL;

    dwNameLength = pSecBuffer->usLength;
    pBuffer = pSecBuffer->dwOffset + (PBYTE)pRespMsg;

    if (!bUnicode)
    {
        dwError = LwAllocateMemory(dwNameLength + 1, OUT_PPVOID(&pName));
        BAIL_ON_LW_ERROR(dwError);

        memcpy(pName, pBuffer, dwNameLength);
    }
    else
    {
        dwNameLength = dwNameLength / sizeof(WCHAR);

        dwError = LwAllocateMemory(dwNameLength + 1, OUT_PPVOID(&pName));
        BAIL_ON_LW_ERROR(dwError);

        for (nIndex = 0; nIndex < dwNameLength; nIndex++)
        {
            pName[nIndex] = pBuffer[nIndex * sizeof(WCHAR)];
        }
    }

cleanup:
    *ppDomainName = pName;
    return dwError;
error:
    LW_SAFE_FREE_STRING(pName);
    goto cleanup;
}

/******************************************************************************/
DWORD
NtlmGetWorkstationFromResponse(
    IN PNTLM_RESPONSE_MESSAGE pRespMsg,
    IN BOOLEAN bUnicode,
    OUT PSTR* ppWorkstation
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PCHAR pName = NULL;
    DWORD dwNameLength = 0;
    PBYTE pBuffer = NULL;
    PNTLM_SEC_BUFFER pSecBuffer = &pRespMsg->Workstation;
    DWORD nIndex = 0;

    *ppWorkstation = NULL;

    dwNameLength = pSecBuffer->usLength;
    pBuffer = pSecBuffer->dwOffset + (PBYTE)pRespMsg;

    if (!bUnicode)
    {
        dwError = LwAllocateMemory(dwNameLength + 1, OUT_PPVOID(&pName));
        BAIL_ON_LW_ERROR(dwError);

        memcpy(pName, pBuffer, dwNameLength);
    }
    else
    {
        dwNameLength = dwNameLength / sizeof(WCHAR);

        dwError = LwAllocateMemory(dwNameLength + 1, OUT_PPVOID(&pName));
        BAIL_ON_LW_ERROR(dwError);

        for (nIndex = 0; nIndex < dwNameLength; nIndex++)
        {
            pName[nIndex] = pBuffer[nIndex * sizeof(WCHAR)];
        }
    }

cleanup:
    *ppWorkstation = pName;
    return dwError;
error:
    LW_SAFE_FREE_STRING(pName);
    goto cleanup;
}
