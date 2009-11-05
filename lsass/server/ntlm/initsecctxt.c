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
 *        initsecctxt.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        InitializeSecurityContext client wrapper API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com
 */

#include "ntlmsrvapi.h"

DWORD
NtlmServerInitializeSecurityContext(
    IN OPTIONAL NTLM_CRED_HANDLE hCredential,
    IN OPTIONAL const NTLM_CONTEXT_HANDLE hContext,
    IN OPTIONAL SEC_CHAR* pszTargetName,
    IN DWORD fContextReq,
    IN DWORD Reserved1,
    IN DWORD TargetDataRep,
    IN OPTIONAL const SecBuffer* pInput,
    IN DWORD Reserved2,
    IN OUT OPTIONAL PNTLM_CONTEXT_HANDLE phNewContext,
    OUT PSecBuffer pOutput,
    OUT PDWORD pfContextAttr,
    OUT OPTIONAL PTimeStamp ptsExpiry
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PNTLM_CONTEXT pNtlmContext = NULL;
    PSTR pWorkstation = NULL;
    PSTR pDomain = NULL;
    PNTLM_CHALLENGE_MESSAGE pMessage = NULL;
    DWORD dwMessageSize = 0;

    if (hContext)
    {
        pNtlmContext = hContext;
    }

    if (!pNtlmContext)
    {
        dwError = NtlmGetNameInformation(
                &pWorkstation,
                &pDomain,
                NULL,
                NULL);
        BAIL_ON_LSA_ERROR(dwError);


        // If we start with a NULL context, create a negotiate message
        dwError = NtlmCreateNegotiateContext(
            hCredential,
            fContextReq,
            pDomain,
            pWorkstation,
            (PBYTE)&gXpSpoof,  //for now add OS ver info... config later
            &pNtlmContext
            );
        BAIL_ON_LSA_ERROR(dwError);

        // copy message to the output parameter... this should be a deep copy since
        // the caller will most likely delete it before cleaning up it's context.
        dwError = NtlmCopyContextToSecBuffer(pNtlmContext, pOutput);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LW_WARNING_CONTINUE_NEEDED;
    }
    else
    {
        if (pInput->BufferType != SECBUFFER_TOKEN ||
           pInput->cbBuffer == 0)
        {
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
        }

        pMessage = pInput->pvBuffer;
        dwMessageSize = pInput->cbBuffer;

        dwError = NtlmCreateResponseContext(
            pMessage,
            hCredential,
            &pNtlmContext);
        BAIL_ON_LSA_ERROR(dwError);

        // copy message to the output parameter... this should be a deep copy since
        // the caller will most likely delete it before cleaning up it's context.
        dwError = NtlmCopyContextToSecBuffer(pNtlmContext, pOutput);
        BAIL_ON_LSA_ERROR(dwError);
    }

    *phNewContext = pNtlmContext;


cleanup:
    LW_SAFE_FREE_STRING(pWorkstation);
    LW_SAFE_FREE_STRING(pDomain);
    return dwError;
error:
    // If this function has already succeed once, we MUST make sure phNewContext
    // is set so the caller can cleanup whatever context is remaining.  It
    // could be the original negotiate context or a new response context but
    // either way it is vital that the caller get a context they can actually
    // cleanup ONCE they've received ANY context from this function.
    //
    // If hContext is NULL, that indicates this is the first time through this
    // call and we can safely release our context.
    if ( pNtlmContext && !hContext)
    {
        NtlmReleaseContext(&pNtlmContext);

        phNewContext = NULL;
    }

    goto cleanup;
}

DWORD
NtlmCreateNegotiateContext(
    IN NTLM_CRED_HANDLE hCred,
    IN DWORD dwOptions,
    IN PCSTR pDomain,
    IN PCSTR pWorkstation,
    IN PBYTE pOsVersion,
    OUT PNTLM_CONTEXT *ppNtlmContext
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PNTLM_CONTEXT pNtlmContext = NULL;
    DWORD dwMessageSize = 0;
    PNTLM_NEGOTIATE_MESSAGE pMessage = NULL;

    *ppNtlmContext = NULL;

    dwError = NtlmCreateContext(hCred, &pNtlmContext);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = NtlmCreateNegotiateMessage(
        dwOptions,
        pDomain,
        pWorkstation,
        pOsVersion,
        &dwMessageSize,
        &pMessage);
    BAIL_ON_LSA_ERROR(dwError);

    pNtlmContext->dwMessageSize = dwMessageSize;
    pNtlmContext->pMessage = pMessage;
    pNtlmContext->NtlmState = NtlmStateNegotiate;

cleanup:

    *ppNtlmContext = pNtlmContext;

    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pNtlmContext->pMessage);

    if (pNtlmContext)
    {
        LW_SAFE_FREE_MEMORY(pNtlmContext);
    }
    goto cleanup;
}

DWORD
NtlmCreateResponseContext(
    IN PNTLM_CHALLENGE_MESSAGE pChlngMsg,
    IN NTLM_CRED_HANDLE hCred,
    IN OUT PNTLM_CONTEXT* ppNtlmContext
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PNTLM_RESPONSE_MESSAGE pMessage = NULL;
    PCSTR pUserNameTemp = NULL;
    PCSTR pPassword = NULL;
    PNTLM_CONTEXT pNtlmContext = NULL;
    PBYTE pMasterKey = NULL;
    BYTE LmUserSessionKey[NTLM_SESSION_KEY_SIZE] = {0};
    BYTE NtlmUserSessionKey[NTLM_SESSION_KEY_SIZE] = {0};
    BYTE LanManagerSessionKey[NTLM_SESSION_KEY_SIZE] = {0};
    BYTE SecondaryKey[NTLM_SESSION_KEY_SIZE] = {0};
    PLSA_LOGIN_NAME_INFO pUserNameInfo = NULL;

    *ppNtlmContext = NULL;

    NtlmGetCredentialInfo(
        hCred,
        &pUserNameTemp,
        &pPassword,
        NULL);

    dwError = LsaCrackDomainQualifiedName(
                        pUserNameTemp,
                        NULL,
                        &pUserNameInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = NtlmCreateContext(hCred, &pNtlmContext);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateString(
                pUserNameTemp,
                &pNtlmContext->pszClientUsername);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = NtlmCreateResponseMessage(
        pChlngMsg,
        pUserNameInfo->pszName,
        pUserNameInfo->pszDomainNetBiosName,
        pPassword,
        (PBYTE)&gXpSpoof,
        NTLM_RESPONSE_TYPE_NTLM,
        NTLM_RESPONSE_TYPE_LM,
        &pNtlmContext->dwMessageSize,
        &pMessage,
        LmUserSessionKey,
        NtlmUserSessionKey
        );
    BAIL_ON_LSA_ERROR(dwError);

    // As a side effect of creating the response, we must also set/produce the
    // master session key...

    pMasterKey = NtlmUserSessionKey;

    if (pChlngMsg->NtlmFlags & NTLM_FLAG_LM_KEY)
    {
        NtlmGenerateLanManagerSessionKey(
            pMessage,
            LmUserSessionKey,
            LanManagerSessionKey);

        pMasterKey = LanManagerSessionKey;
    }

    if (pChlngMsg->NtlmFlags & NTLM_FLAG_KEY_EXCH)
    {
        // This is the key we will use for session security...
        dwError = NtlmGetRandomBuffer(
            SecondaryKey,
            NTLM_SESSION_KEY_SIZE);
        BAIL_ON_LSA_ERROR(dwError);

        // Encrypt it with the "master key" set above and send it along with the
        // response
        NtlmStoreSecondaryKey(
            pMasterKey,
            SecondaryKey,
            pMessage);

        pMasterKey = SecondaryKey;
    }

    NtlmWeakenSessionKey(
        pChlngMsg,
        pMasterKey,
        &pNtlmContext->cbSessionKeyLen);

    memcpy(pNtlmContext->SessionKey, pMasterKey, NTLM_SESSION_KEY_SIZE);

    pNtlmContext->NegotiatedFlags = pChlngMsg->NtlmFlags;
    pNtlmContext->pMessage = pMessage;
    pNtlmContext->NtlmState = NtlmStateResponse;

    NtlmInitializeKeys(pNtlmContext);

cleanup:
    if (pUserNameInfo)
    {
        LsaFreeNameInfo(pUserNameInfo);
    }

    *ppNtlmContext = pNtlmContext;

    return dwError;
error:
    LW_SAFE_FREE_MEMORY(pMessage);

    if (pNtlmContext)
    {
        NtlmFreeContext(&pNtlmContext);
    }

    goto cleanup;
}
