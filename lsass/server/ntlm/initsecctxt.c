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
    CHAR Workstation[HOST_NAME_MAX + 1] = {0};
    CHAR Domain[HOST_NAME_MAX + 1] = {0};
    PCHAR pDot = NULL;

    if(phContext)
    {
        pNtlmContext = *phContext;
    }

    if(!pNtlmContext)
    {
        dwError = gethostname(Domain, HOST_NAME_MAX);
        dwError = LwMapErrnoToLwError(dwError);
        BAIL_ON_NTLM_ERROR(dwError);

        memcpy(Workstation, Domain, strlen(Domain));

        pDot = strchr(Workstation, '.');
        if(pDot)
        {
            pDot[0] = '\0';
        }

        // If we start with a NULL context, create a negotiate message
        dwError = NtlmCreateNegotiateContext(
            fContextReq,
            Domain,
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

    NtlmAddContext(pNtlmContext, &NtlmContextHandle);

    pNtlmContext->CredHandle = *phCredential;

    *phNewContext = NtlmContextHandle;

    //copy message to the output parameter
    dwError = NtlmCopyContextToSecBufferDesc(pNtlmContext, pOutput);

    BAIL_ON_NTLM_ERROR(dwError);

cleanup:
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

    // this is paranoid, but we should make sure we're not writing over an
    // existing context (or at the very least, this is a check that we've
    // initialized our vars in a previous function.
    if(*ppNtlmContext != NULL)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_NTLM_ERROR(dwError);
    }

    dwError = NtlmInitContext(ppNtlmContext);
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NtlmCreateNegotiateMessage(
        dwOptions,
        pDomain,
        pWorkstation,
        pOsVersion,
        &((*ppNtlmContext)->dwMessageSize),
        (PNTLM_NEGOTIATE_MESSAGE*)&((*ppNtlmContext)->pMessage)
        );

    BAIL_ON_NTLM_ERROR(dwError);

    (*ppNtlmContext)->NtlmState = NtlmStateNegotiate;

cleanup:
    return dwError;
error:
    if(*ppNtlmContext)
    {
        if((*ppNtlmContext)->pMessage)
        {
            LwFreeMemory((*ppNtlmContext)->pMessage);
        }
        LwFreeMemory(*ppNtlmContext);
    }
    goto cleanup;
}

DWORD
NtlmCreateResponseContext(
    IN PLSA_CONTEXT pChlngCtxt,
    OUT PLSA_CONTEXT *ppNtlmContext
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PCHAR pUserName = NULL;
    PCHAR pPassword = NULL;

    if(!pChlngCtxt)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_NTLM_ERROR(dwError);
    }

    LsaGetCredentialInfo(pChlngCtxt->CredHandle, &pUserName, &pPassword, NULL);

    BAIL_ON_NTLM_ERROR(dwError);

    dwError = LwAllocateMemory(
        sizeof(LSA_CONTEXT),
        (PVOID*)(PVOID)ppNtlmContext
        );

    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NtlmCreateResponseMessage(
        pChlngCtxt->pMessage,
        NULL,
        pUserName,
        NULL,
        NULL,
        pPassword,
        NTLM_RESPONSE_TYPE_NTLM,
        NTLM_RESPONSE_TYPE_LM,
        &((*ppNtlmContext)->dwMessageSize),
        (PNTLM_RESPONSE_MESSAGE*)&((*ppNtlmContext)->pMessage)
        );

    BAIL_ON_NTLM_ERROR(dwError);

    (*ppNtlmContext)->NtlmState = NtlmStateResponse;

cleanup:
    return dwError;
error:
    if(*ppNtlmContext)
    {
        if((*ppNtlmContext)->pMessage)
        {
            LwFreeMemory((*ppNtlmContext)->pMessage);
        }
        LwFreeMemory(*ppNtlmContext);
    }
    goto cleanup;
}

