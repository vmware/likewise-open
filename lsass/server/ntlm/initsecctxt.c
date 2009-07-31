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
    IN OPTIONAL PLSA_CRED_HANDLE phCredential,
    IN OPTIONAL PLSA_CONTEXT_HANDLE phContext,
    IN OPTIONAL SEC_CHAR * pszTargetName,
    IN DWORD fContextReq,
    IN DWORD Reserved1,
    IN DWORD TargetDataRep,
    IN OPTIONAL PSecBufferDesc pInput,
    IN DWORD Reserved2,
    IN OUT OPTIONAL PLSA_CONTEXT_HANDLE phNewContext,
    IN OUT OPTIONAL PSecBufferDesc pOutput,
    OUT PDWORD pfContextAttr,
    OUT OPTIONAL PTimeStamp ptsExpiry
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PLSA_CONTEXT pNtlmContext = NULL;
    PLSA_CONTEXT pNtlmContextIn = NULL;
    LSA_CONTEXT_HANDLE NtlmContextHandle = NULL;
    PSTR Workstation = NULL;
    PSTR pDomain = NULL;

    if(phContext)
    {
        pNtlmContext = *phContext;
    }

    if(!pNtlmContext)
    {
        dwError = NtlmGetDomainFromCredential(
            phCredential,
            &pDomain);
        BAIL_ON_NTLM_ERROR(dwError);

        // If we start with a NULL context, create a negotiate message
        dwError = NtlmCreateNegotiateContext(
            fContextReq,
            pDomain,
            Workstation,
            (PBYTE)&gXpSpoof,  //for now add OS ver info... config later
            &pNtlmContext
            );
        BAIL_ON_NTLM_ERROR(dwError);
    }
    else
    {
        //... create a new response message
        dwError = NtlmCreateContextFromSecBufferDesc(
            pInput,
            NtlmStateChallenge,
            &pNtlmContextIn
            );

        dwError = NtlmCreateResponseContext(pNtlmContextIn, &pNtlmContext);
        BAIL_ON_NTLM_ERROR(dwError);
    }

    pNtlmContext->CredHandle = *phCredential;

    NtlmAddContext(pNtlmContext, &NtlmContextHandle);

    *phNewContext = NtlmContextHandle;

    //copy message to the output parameter
    dwError = NtlmCopyContextToSecBufferDesc(pNtlmContext, pOutput);

    BAIL_ON_NTLM_ERROR(dwError);

cleanup:
    LSA_SAFE_FREE_STRING(pDomain);
    return dwError;
error:
    if( pNtlmContext )
    {
        NtlmReleaseContext(pNtlmContext);
    }
    phNewContext = NULL;
    goto cleanup;
}

DWORD
NtlmCreateNegotiateContext(
    IN DWORD dwOptions,
    IN PCHAR pDomain,
    IN PCHAR pWorkstation,
    IN PBYTE pOsVersion,
    OUT PLSA_CONTEXT *ppNtlmContext
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PLSA_CONTEXT pNtlmContext = NULL;

    *ppNtlmContext = NULL;

    dwError = NtlmCreateContext(&pNtlmContext);
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NtlmCreateNegotiateMessage(
        dwOptions,
        pDomain,
        pWorkstation,
        pOsVersion,
        &pNtlmContext->dwMessageSize,
        (PNTLM_NEGOTIATE_MESSAGE*)&pNtlmContext->pMessage);

    BAIL_ON_NTLM_ERROR(dwError);

    pNtlmContext->NtlmState = NtlmStateNegotiate;

cleanup:

    *ppNtlmContext = pNtlmContext;

    return dwError;

error:
    if(pNtlmContext)
    {
        LSA_SAFE_FREE_MEMORY(pNtlmContext->pMessage);
        LSA_SAFE_FREE_MEMORY(pNtlmContext);

        pNtlmContext = NULL;
    }
    goto cleanup;
}

DWORD
NtlmCreateResponseContext(
    IN PLSA_CONTEXT pChlngCtxt,
    IN OUT PLSA_CONTEXT* ppNtlmContext
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PSTR pUserName = NULL;
    PCSTR pUserNameTemp = NULL;
    PCSTR pPassword = NULL;
    LSA_CRED_HANDLE CredHandle = (*ppNtlmContext)->CredHandle;
    PLSA_CONTEXT pNtlmContext = NULL;
    PBYTE pMasterKey = NULL;
    BYTE LmUserSessionKey[NTLM_SESSION_KEY_SIZE] = {0};
    BYTE NtlmUserSessionKey[NTLM_SESSION_KEY_SIZE] = {0};
    BYTE LanManagerSessionKey[NTLM_SESSION_KEY_SIZE] = {0};
    BYTE SecondaryKey[NTLM_SESSION_KEY_SIZE] = {0};


#if 0
    // Only for testing purposes... remove when implementation is complete
    pPassword = "SecREt01";
    pChlngMsg->Challenge[0] = 0x01;
    pChlngMsg->Challenge[1] = 0x23;
    pChlngMsg->Challenge[2] = 0x45;
    pChlngMsg->Challenge[3] = 0x67;
    pChlngMsg->Challenge[4] = 0x89;
    pChlngMsg->Challenge[5] = 0xab;
    pChlngMsg->Challenge[6] = 0xcd;
    pChlngMsg->Challenge[7] = 0xef;
#endif

    *ppNtlmContext = NULL;

    if(!pChlngCtxt)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_NTLM_ERROR(dwError);
    }

    pChlngCtxt->CredHandle = CredHandle;

    LsaGetCredentialInfo(CredHandle, &pUserNameTemp, &pPassword, NULL);

    dwError = NtlmFixUserName(pUserNameTemp, &pUserName);
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = LsaAllocateMemory(
        sizeof(LSA_CONTEXT),
        (PVOID*)(PVOID)&pNtlmContext
        );
    BAIL_ON_NTLM_ERROR(dwError);

    pNtlmContext->CredHandle = CredHandle;

    dwError = NtlmCreateResponseMessage(
        pChlngCtxt->pMessage,
        (PCSTR)pUserName,
        pPassword,
        (PBYTE)&gXpSpoof,
        NTLM_RESPONSE_TYPE_NTLM,
        NTLM_RESPONSE_TYPE_LM,
        &pNtlmContext->dwMessageSize,
        (PNTLM_RESPONSE_MESSAGE*)&pNtlmContext->pMessage,
        LmUserSessionKey,
        NtlmUserSessionKey
        );
    BAIL_ON_NTLM_ERROR(dwError);

    // As a side effect of creating the response, we must also set/produce the
    // master session key...

    pMasterKey = NtlmUserSessionKey;

    if(((PNTLM_CHALLENGE_MESSAGE)pChlngCtxt->pMessage)->NtlmFlags &
        NTLM_FLAG_LM_KEY)
    {
        NtlmGenerateLanManagerSessionKey(
            (PNTLM_RESPONSE_MESSAGE)pNtlmContext->pMessage,
            LmUserSessionKey,
            LanManagerSessionKey);

        pMasterKey = LanManagerSessionKey;
    }

    if(((PNTLM_CHALLENGE_MESSAGE)pChlngCtxt->pMessage)->NtlmFlags &
        NTLM_FLAG_KEY_EXCH)
    {
        // This is the key we will use for session security...
        dwError = NtlmGetRandomBuffer(
            SecondaryKey,
            NTLM_SESSION_KEY_SIZE);
        BAIL_ON_NTLM_ERROR(dwError);

        // Encrypt it with the "master key" set above and send it along with the
        // response
        NtlmStoreSecondaryKey(
            pMasterKey,
            SecondaryKey,
            (PNTLM_RESPONSE_MESSAGE)pNtlmContext->pMessage);

        pMasterKey = SecondaryKey;
    }

    NtlmWeakenSessionKey(
        pChlngCtxt->pMessage,
        pMasterKey,
        &pNtlmContext->cbSessionKeyLen);

    memcpy(pNtlmContext->SessionKey, pMasterKey, NTLM_SESSION_KEY_SIZE);

    pNtlmContext->NtlmState = NtlmStateResponse;

cleanup:
    LSA_SAFE_FREE_STRING(pUserName);

    *ppNtlmContext = pNtlmContext;

    return dwError;
error:
    if(pNtlmContext)
    {
        LSA_SAFE_FREE_MEMORY(pNtlmContext->pMessage);
        LSA_SAFE_FREE_MEMORY(pNtlmContext);
    }
    goto cleanup;
}

DWORD
NtlmGetDomainFromCredential(
    PLSA_CRED_HANDLE pCredHandle,
    PSTR* ppDomain
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PCSTR pDomain = NULL;

    LsaGetCredentialInfo(*pCredHandle, &pDomain, NULL, NULL);

    pDomain = strchr(pDomain, '@');

    if(!pDomain)
    {
        dwError = LW_ERROR_INVALID_CREDENTIAL;
        BAIL_ON_NTLM_ERROR(dwError);
    }

    pDomain++;

    dwError = LsaAllocateString(pDomain, ppDomain);

cleanup:
    return dwError;
error:
    *ppDomain = NULL;

    goto cleanup;
}

DWORD
NtlmFixUserName(
    IN PCSTR pOriginalUserName,
    OUT PSTR* ppUserName
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PSTR pUserName = NULL;
    PCHAR pSymbol = NULL;

    // The original name is in the NetBiosName\username@location format...
    // We're just interested in the username portion... isolate it.
    pSymbol = strchr(pOriginalUserName, '\\');

    if(!pSymbol)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_NTLM_ERROR(dwError);
    }

    pSymbol++;

    dwError = LsaAllocateString(pSymbol, &pUserName);
    BAIL_ON_NTLM_ERROR(dwError);

    pSymbol = strchr(pUserName, '@');

    if(!pSymbol)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_NTLM_ERROR(dwError);
    }

    pSymbol = '\0';

cleanup:
    *ppUserName = pUserName;
    return dwError;
error:
    LSA_SAFE_FREE_STRING(pUserName);
    goto cleanup;
}
