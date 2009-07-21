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
    PLSA_CONTEXT pNtlmCtxtIn = NULL;
    PLSA_CONTEXT pNtlmCtxtChlng = NULL;
    LSA_CONTEXT_HANDLE ContextHandle = NULL;

    if(ptsTimeStamp)
    {
        memset(ptsTimeStamp, 0, sizeof(TimeStamp));
    }

    if(!phContext)
    {
        dwError = NtlmCreateContextFromSecBufferDesc(
            pInput,
            NtlmStateNegotiate,
            &pNtlmCtxtIn
            );
        BAIL_ON_NTLM_ERROR(dwError);
    }
    else
    {
        // The only time we should get a context handle passed in is when
        // we are validating a challenge and we need to look up the original
        // challenge sent
        pNtlmCtxtChlng = *phContext;

        // In this case we need to build up a temp context for the response
        // message sent in
        dwError = NtlmInitContext(&pNtlmCtxtIn);
        BAIL_ON_NTLM_ERROR(dwError);

        pNtlmCtxtIn->NtlmState = NtlmStateResponse;
        pNtlmCtxtIn->pMessage = pInput;
    }

    switch(pNtlmCtxtIn->NtlmState)
    {
    case NtlmStateNegotiate:
        {
            // If we start with a blank context, upgrade it to a challenge
            // message
            dwError = NtlmCreateChallengeContext(pNtlmCtxtIn, &pNtlmCtxtOut);

            BAIL_ON_NTLM_ERROR(dwError);
        }
        break;
    case NtlmStateResponse:
        {
            //... authenticate the response
            dwError = NtlmValidateResponse(pNtlmCtxtIn, pNtlmCtxtChlng);
            BAIL_ON_NTLM_ERROR(dwError);

            NtlmFreeContext(pNtlmCtxtChlng);

            pNtlmCtxtChlng = NULL;
        }
        break;
    default:
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_NTLM_ERROR(dwError);
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
    if(pNtlmCtxtChlng)
    {
        NtlmReleaseContext(pNtlmCtxtChlng);
    }
    goto cleanup;
}

DWORD
NtlmCreateChallengeContext(
    IN PLSA_CONTEXT pNtlmNegCtxt,
    OUT PLSA_CONTEXT *ppNtlmContext
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;

    if(!ppNtlmContext)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_NTLM_ERROR(dwError);
    }

    dwError = NtlmInitContext(ppNtlmContext);

    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NtlmCreateChallengeMessage(
        (PNTLM_NEGOTIATE_MESSAGE)pNtlmNegCtxt->pMessage,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        &((*ppNtlmContext)->dwMessageSize),
        (PNTLM_CHALLENGE_MESSAGE*)&((*ppNtlmContext)->pMessage)
        );

    BAIL_ON_NTLM_ERROR(dwError);

    (*ppNtlmContext)->NtlmState = NtlmStateChallenge;

cleanup:
    return dwError;
error:
    goto cleanup;
}

DWORD
NtlmValidateResponse(
    PLSA_CONTEXT pRespCtxt,
    PLSA_CONTEXT pChlngCtxt
    )
{
    return LW_ERROR_SUCCESS;
}
