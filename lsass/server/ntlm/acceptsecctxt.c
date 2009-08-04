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

DWORD
NtlmServerAcceptSecurityContext(
    IN PLSA_CRED_HANDLE phCredential,
    IN OUT PLSA_CONTEXT_HANDLE phContext,
    IN PSecBufferDesc pInput,
    IN DWORD fContextReq,
    IN DWORD TargetDataRep,
    IN OUT PLSA_CONTEXT_HANDLE phNewContext,
    IN OUT PSecBufferDesc pOutput,
    OUT PDWORD  pfContextAttr,
    OUT PTimeStamp ptsTimeStamp
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PLSA_CONTEXT pNtlmCtxtOut = NULL;
    PLSA_CONTEXT pNtlmCtxtChlng = NULL;
    LSA_CONTEXT_HANDLE ContextHandle = NULL;
    PNTLM_NEGOTIATE_MESSAGE pNegMsg = NULL;
    PNTLM_RESPONSE_MESSAGE pRespMsg = NULL;
    DWORD dwMessageSize = 0;

    if(ptsTimeStamp)
    {
        memset(ptsTimeStamp, 0, sizeof(TimeStamp));
    }

    if(!phContext)
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
    }
    else
    {
        // The only time we should get a context handle passed in is when
        // we are validating a challenge and we need to look up the original
        // challenge sent
        pNtlmCtxtChlng = *phContext;

        // In this case we need to grab the response message sent in
        dwError = NtlmGetMessageFromSecBufferDesc(
            pInput,
            &dwMessageSize,
            (PVOID*)&pRespMsg
            );
        BAIL_ON_LW_ERROR(dwError);

        dwError = NtlmValidateResponse(pRespMsg, dwMessageSize, pNtlmCtxtChlng);
        BAIL_ON_LW_ERROR(dwError);
    }

    if(pNtlmCtxtOut)
    {
        dwError = NtlmCopyContextToSecBufferDesc(pNtlmCtxtOut, pOutput);
        BAIL_ON_LW_ERROR(dwError);

        NtlmAddContext(pNtlmCtxtOut, &ContextHandle);

        dwError = LW_WARNING_CONTINUE_NEEDED;
    }

cleanup:
    *phNewContext = ContextHandle;

    return(dwError);
error:
    if(ContextHandle)
    {
        NtlmReleaseContext(ContextHandle);
        ContextHandle = NULL;
    }
    else if(pNtlmCtxtOut)
    {
        // pNtlmCtxtOut never made it into the list.. free directly
        NtlmFreeContext(pNtlmCtxtOut);
    }
    goto cleanup;
}

DWORD
NtlmCreateChallengeContext(
    IN PNTLM_NEGOTIATE_MESSAGE pNtlmNegMsg,
    IN PLSA_CRED_HANDLE pCredHandle,
    OUT PLSA_CONTEXT *ppNtlmContext
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PLSA_CONTEXT pNtlmContext = NULL;
    DWORD dwMessageSize = 0;
    PNTLM_CHALLENGE_MESSAGE pMessage = NULL;
    PSTR pDomainName = NULL;
    PSTR pDnsDomainName = NULL;

    *ppNtlmContext = NULL;

    dwError = NtlmCreateContext(pCredHandle, &pNtlmContext);
    BAIL_ON_LW_ERROR(dwError);

    dwError = NtlmGetDomainFromCredential(pCredHandle, &pDnsDomainName);
    BAIL_ON_LW_ERROR(dwError);

    dwError = NtlmGetNetBiosName((PCSTR)pDnsDomainName, &pDomainName);
    BAIL_ON_LW_ERROR(dwError);

    dwError = NtlmCreateChallengeMessage(
        pNtlmNegMsg,
        NULL,
        pDomainName,
        NULL,
        pDnsDomainName,
        (PBYTE)&gW2KSpoof,
        &dwMessageSize,
        &pMessage
        );

    BAIL_ON_LW_ERROR(dwError);

    pNtlmContext->pMessage = pMessage;
    pNtlmContext->NtlmState = NtlmStateChallenge;

cleanup:
    LSA_SAFE_FREE_STRING(pDomainName);
    LSA_SAFE_FREE_STRING(pDnsDomainName);
    *ppNtlmContext = pNtlmContext;
    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pMessage);

    if(pNtlmContext)
    {
        LsaReleaseCredential(*pCredHandle);
        LW_SAFE_FREE_MEMORY(pNtlmContext);
    }
    goto cleanup;
}

DWORD
NtlmValidateResponse(
    PNTLM_RESPONSE_MESSAGE pRespMsg,
    DWORD dwRespMsgSize,
    PLSA_CONTEXT pChlngCtxt
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PNTLM_CHALLENGE_MESSAGE pChlngMsg = NULL;
    LSA_CRED_HANDLE pCredHandle = NULL;
    PCSTR pPassword = NULL;
    DWORD dwResponseType = NTLM_RESPONSE_TYPE_NTLM;
    DWORD dwRespSize = 0;
    PBYTE pOurResp = NULL;
    PBYTE pTheirResp = NULL;
    BYTE UserSessionKey[NTLM_SESSION_KEY_SIZE] = {0};

    // sanity check
    if(!pRespMsg || ! pChlngCtxt)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LW_ERROR(dwError);
    }

    pChlngMsg = pChlngCtxt->pMessage;
    pCredHandle = pChlngCtxt->CredHandle;

    LsaGetCredentialInfo(pCredHandle, NULL, &pPassword, NULL);

    // We only validate the highest level response we receive.
    if(pRespMsg->NtResponse.usLength > NTLM_RESPONSE_SIZE_NTLM)
    {
        dwResponseType = NTLM_RESPONSE_TYPE_NTLMv2;
        dwError = LW_ERROR_NOT_IMPLEMENTED;
        BAIL_ON_LW_ERROR(dwError);
    }
    else if(pRespMsg->NtResponse.usLength == NTLM_RESPONSE_SIZE_NTLM)
    {
        dwResponseType = NTLM_RESPONSE_TYPE_NTLM;
        pTheirResp = pRespMsg->NtResponse.dwOffset + (PBYTE)pRespMsg;
    }
    else if(pRespMsg->LmResponse.usLength)
    {
        dwResponseType = NTLM_RESPONSE_TYPE_LM;
        pTheirResp = pRespMsg->LmResponse.dwOffset + (PBYTE)pRespMsg;
    }
    else
    {
        // we couldn't identify the response type... bail
        dwError = LW_ERROR_UNEXPECTED_MESSAGE;
        BAIL_ON_LW_ERROR(dwError);
    }

    // calculate the response size...
    dwError = NtlmCalculateResponseSize(
        pChlngMsg,
        dwResponseType,
        &dwRespSize
        );
    BAIL_ON_LW_ERROR(dwError);

    dwError = LwAllocateMemory(dwRespSize, OUT_PPVOID(&pOurResp));
    BAIL_ON_LW_ERROR(dwError);

    // Recreate the response
    dwError = NtlmBuildResponse(
        pChlngMsg,
        pPassword,
        dwResponseType,
        dwRespSize,
        UserSessionKey,
        pOurResp
        );
    BAIL_ON_LW_ERROR(dwError);

    // Verify the response
    dwError = LW_ERROR_INVALID_ACCOUNT;
    if(dwRespSize == dwRespMsgSize)
    {
        if(!memcmp(pOurResp, pTheirResp, dwRespSize))
        {
            dwError = LW_ERROR_SUCCESS;
        }
    }

cleanup:
    LW_SAFE_FREE_MEMORY(pOurResp);
    return dwError;
error:
    goto cleanup;
}

DWORD
NtlmGetNetBiosName(
    PCSTR pDnsDomainName,
    PSTR *ppDomainName
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PSTR pDomainName = NULL;
    DWORD dwDomainNameLength = 0;
    PCHAR pSymbol = NULL;
    DWORD dwIndex = 0;

    *ppDomainName = NULL;

    dwError = LsaAllocateString(pDnsDomainName, &pDomainName);
    BAIL_ON_LW_ERROR(dwError);

    pSymbol = strchr(pDomainName, '.');

    *pSymbol = '\0';

    dwDomainNameLength = strlen(pDomainName);
    for(dwIndex = 0; dwDomainNameLength; dwIndex++)
    {
        pDomainName[dwIndex] = toupper(pDomainName[dwIndex]);
    }

cleanup:
    *ppDomainName = pDomainName;
    return dwError;
error:
    LSA_SAFE_FREE_STRING(pDomainName);
    goto cleanup;
}
