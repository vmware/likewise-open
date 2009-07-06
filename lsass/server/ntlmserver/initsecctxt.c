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

#include <ntlmsrvapi.h>

DWORD
NtlmServerInitializeSecurityContext(
    PCredHandle phCredential,
    PCtxtHandle phContext,
    SEC_CHAR * pszTargetName,
    DWORD fContextReq,
    DWORD Reserved1,
    DWORD TargetDataRep,
    PSecBufferDesc pInput,
    DWORD Reserved2,
    PCtxtHandle phNewContext,
    PSecBufferDesc pOutput,
    PDWORD pfContextAttr,
    PTimeStamp ptsExpiry
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    PNTLM_CONTEXT pNtlmContext = NULL;
    PNTLM_CONTEXT pNtlmContextIn;
    BOOLEAN bInLock = FALSE;

    if(!phCredential)
    {
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_NTLM_ERROR(dwError);
    }

    if(!phContext)
    {
        dwError = NtlmInitContext(&pNtlmContext);
        BAIL_ON_NTLM_ERROR(dwError);
    }
    else
    {
        ENTER_NTLM_CONTEXT_LIST_WRITER_LOCK(bInLock);
            dwError = NtlmFindContext(phContext, &pNtlmContext);
        LEAVE_NTLM_CONTEXT_LIST_WRITER_LOCK(bInLock);

        BAIL_ON_NTLM_ERROR(dwError);
    }

    if(!pNtlmContext)
    {
        // If we start with a blank context, upgrade it to a negotiate
        // message
        dwError = NtlmCreateNegotiateContext(&pNtlmContext);
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

    ENTER_NTLM_CONTEXT_LIST_WRITER_LOCK(bInLock);
    //
        dwError = NtlmInsertContext(
            pNtlmContext
            );
    //
    LEAVE_NTLM_CONTEXT_LIST_WRITER_LOCK(bInLock);

    pNtlmContext->CredHandle = *phCredential;

    *phNewContext = pNtlmContext->ContextHandle;

    //copy message to the output parameter


cleanup:
    return(dwError);
error:
    if( pNtlmContext->ContextHandle.dwLower ||
        pNtlmContext->ContextHandle.dwUpper )
    {
        ENTER_NTLM_CONTEXT_LIST_WRITER_LOCK(bInLock);
            NtlmRemoveContext(&(pNtlmContext->ContextHandle));
        LEAVE_NTLM_CONTEXT_LIST_WRITER_LOCK(bInLock);
    }
    memset(phNewContext, 0, sizeof(CtxtHandle));
    goto cleanup;
}

DWORD
NtlmCreateNegotiateContext(
    IN OUT PNTLM_CONTEXT *ppNtlmContext
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    CHAR HostName[HOST_NAME_MAX + 1];
    PNTLM_NEGOTIATE_MESSAGE pNtlmNegMsg = NULL;

    if(!ppNtlmContext)
    {
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_NTLM_ERROR(dwError);
    }

    // if we have a NULL context, try to create a new one.
    if(!(*ppNtlmContext))
    {
        // We must not have any message in the context, we should only be
        // creating a new context or replacing a blank... blanks have no message
        if((*ppNtlmContext)->pMessage)
        {
            dwError = LSA_ERROR_INVALID_PARAMETER;
            BAIL_ON_NTLM_ERROR(dwError);
        }

        dwError = LsaAllocateMemory(
            sizeof(NTLM_CONTEXT),
            (PVOID*)ppNtlmContext
            );

        BAIL_ON_NTLM_ERROR(dwError);
    }

    gethostname(HostName, HOST_NAME_MAX);

    dwError = NtlmCreateNegotiateMessage(
        0,
        NULL,
        HostName,
        NULL,
        &pNtlmNegMsg
        );

    BAIL_ON_NTLM_ERROR(dwError);

    (*ppNtlmContext)->pMessage = (PVOID)pNtlmNegMsg;
    (*ppNtlmContext)->NtlmState = NtlmStateNegotiate;

cleanup:
    return dwError;
error:
    // Don't worry about freeing the context... the caller will have to handle
    // that
    if(pNtlmNegMsg)
    {
        LsaFreeMemory(pNtlmNegMsg);

        if((*ppNtlmContext) && (*ppNtlmContext)->pMessage)
        {
            (*ppNtlmContext)->pMessage = NULL;
        }
    }
    goto cleanup;
}

DWORD
NtlmCreateResponseContext(
    PNTLM_CONTEXT pChlngCtxt,
    OUT PNTLM_CONTEXT *ppNtlmContext
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    PNTLM_RESPONSE_MESSAGE pRespMsg = NULL;
    PNTLM_CREDENTIALS pNtlmCreds = NULL;
    BOOLEAN bInLock = FALSE;

    if(!pChlngCtxt)
    {
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_NTLM_ERROR(dwError);
    }

    ENTER_NTLM_CREDS_LIST_READER_LOCK(bInLock)
        dwError = NtlmFindCredentials(&(pChlngCtxt->CredHandle), &pNtlmCreds);
    LEAVE_NTLM_CREDS_LIST_READER_LOCK(bInLock)

    BAIL_ON_NTLM_ERROR(dwError);

    dwError = LsaAllocateMemory(
        sizeof(NTLM_CONTEXT),
        (PVOID*)ppNtlmContext
        );

    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NtlmCreateResponseMessage(
        pChlngCtxt->pMessage,
        NULL,
        pNtlmCreds->pUserName,
        NULL,
        NULL,
        pNtlmCreds->pPassword,
        NTLM_RESPONSE_TYPE_NTLM,
        NTLM_RESPONSE_TYPE_LM,
        &pRespMsg
        );

    BAIL_ON_NTLM_ERROR(dwError);

    (*ppNtlmContext)->pMessage = (PVOID)pRespMsg;
    (*ppNtlmContext)->NtlmState = NtlmStateResponse;

cleanup:
    if(pNtlmCreds)
    {
        NtlmFreeCredentials(pNtlmCreds);
    }
    return dwError;
error:
    goto cleanup;
}

