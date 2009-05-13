/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        context.c
 *
 * Abstract:
 *
 *        GSS context management and APIs
 *
 * Author: Todd Stecher (2007)
 *
 */

#include "client.h"

DWORD
NTLMGssInitSecContext(
    DWORD          *pdwMinorStatus,
    PVOID           pCredential,
    PVOID          *pContext,
    PLSA_STRING     pTargetName,
    DWORD           dwReqFlags,
    DWORD           dwReqTime,
    PSEC_BUFFER     inputToken,
    PSEC_BUFFER     pOutputToken,
    DWORD          *pFlags,
    DWORD          *pTimeValid
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    DWORD msgError = LSA_ERROR_SUCCESS;
    NTLM_CREDENTIAL *pCred = NULL;
    NTLM_CONTEXT *pCtxt = NULL;
    SEC_BUFFER outputToken;

    ZERO_STRUCT(outputToken);

    pCtxt = (PNTLM_CONTEXT) (*pContext);

    /* locate, and reference credential handle */
    /* @todo - GSSAPI RFC doesn't require a valid cred handle */
    /* take it from the context */
    pCred = NTLMValidateCredential(
                (NTLM_CREDENTIAL*) pCredential,
                true
                );

    if (NULL == pCred) {
        dwError = LSA_ERROR_INVALID_CREDENTIAL;
        BAIL_ON_NTLM_ERROR(dwError);
    }

    /* check credential usage */
    if ((pCred->flags & GSS_C_INITIATE) == 0)  {
        dwError = LSA_ERROR_INVALID_CREDENTIAL;
        BAIL_ON_NTLM_ERROR(dwError);
    }

    /* @todo validate uid, pid */

    /* if null context, create a new one */
    if (pCtxt == (PNTLM_CONTEXT) GSS_C_NO_CONTEXT) {

        dwError = NTLMCreateContext(
                        pCred,
                        CONTEXT_CLIENT,
                        &pCtxt
                        );

        BAIL_ON_NTLM_ERROR(dwError);

    } else {

        pCtxt = NTLMLocateContext(
                    pCtxt,
                    pCred,
                    CONTEXT_CLIENT
                    );

        if (pCtxt == NULL) {
            dwError = LSA_ERROR_NO_CONTEXT;
            BAIL_ON_NTLM_ERROR(dwError);
        }
    }

    switch (pCtxt->dwState) {

        case INIT_CTX_INIT_STATE:

        case INIT_CTX_RECV_CHALLENGE_MSG:


    /* process message */
    msgError = pCtxt->processNextMessage(
                        pCtxt,
                        inputToken,
                        &outputToken
                        );

    if (msgError != LSA_WARNING_CONTINUE_NEEDED)
        BAIL_ON_NTLM_ERROR(msgError);



    if (msgError == GSS_S_COMPLETE)
    {
        NTLM_LOCK_CONTEXTS();
        dwError = NTLMCreateKeys(pCtxt);
        NTLM_UNLOCK_CONTEXTS();

        BAIL_ON_NTLM_ERROR(dwError);
    }

    memcpy(pOutputToken, &outputToken, sizeof(SEC_BUFFER));
    ZERO_STRUCT(outputToken);

    *pContext = pCtxt;

    NTLMDumpContext(D_ERROR, pCtxt); /*@todo - dumb this down to D_CTXT */

error:

    if (dwError == LSA_ERROR_SUCCESS && msgError )
        dwError = msgError;

    (*pdwMinorStatus) = dwError;

    /* context is invalid after error */
    if (LSA_ERROR_MASK(dwError))
        NTLMRemoveContext(pCtxt);

    NTLMFreeSecBuffer(&outputToken);
    NTLMDereferenceCredential(pCred);
    NTLMDereferenceContext(pCtxt);

    return dwError;
}

DWORD
NTLMGssAcceptSecContext(
    DWORD          *pdwMinorStatus,
    PVOID           pCredential,
    PVOID          *pContext,
    PSEC_BUFFER     inputToken,
    PLSA_STRING     pSrcName,
    PSEC_BUFFER     pOutputToken,
    DWORD          *pFlags,
    DWORD          *pTimeValid
    )
{
    DWORD dwError = 0;
    DWORD msgError = LSA_ERROR_SUCCESS;
    NTLM_CREDENTIAL *pCred = NULL;
    NTLM_CONTEXT *pCtxt = NULL;
    SEC_BUFFER outputToken;

    ZERO_STRUCT(outputToken);

    if (!pContext)
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_INVALID_CONTEXT);

    /* locate, and reference credential handle */
    pCred = NTLMValidateCredential(
                (NTLM_CREDENTIAL*) pCredential,
                true
                );

    if (NULL == pCred) {
        dwError = LSA_ERROR_INVALID_CREDENTIAL;
        BAIL_ON_NTLM_ERROR(dwError);
    }

    /* check credential usage */
    if ((pCred->flags & GSS_C_ACCEPT) == 0) {
        dwError = LSA_ERROR_INVALID_CREDENTIAL;
        BAIL_ON_NTLM_ERROR(dwError);
    }

    /* validate oid, possibly alter mech */

    /* validate secbuffer data */

    /* @todo validate uid, pid */

    /* if null context, create a new one */
    if (*pContext == GSS_C_NO_CONTEXT) {

        dwError = NTLMCreateContext(
                        pCred,
                        CONTEXT_SERVER,
                        &pCtxt
                        );

        BAIL_ON_NTLM_ERROR(dwError);

    } else {

        pCtxt = NTLMLocateContext(
                    (PNTLM_CONTEXT) *pContext,
                    pCred,
                    CONTEXT_SERVER
                    );

        if (pCtxt == NULL) {
            dwError = LSA_ERROR_NO_CONTEXT;
            BAIL_ON_NTLM_ERROR(dwError);
        }
    }

    /* process message */
    msgError = pCtxt->processNextMessage(
                        pCtxt,
                        inputToken,
                        &outputToken
                        );

    if (msgError != LSA_WARNING_CONTINUE_NEEDED)
        BAIL_ON_NTLM_ERROR(msgError);

    if (msgError == GSS_S_COMPLETE)
    {
        NTLM_LOCK_CONTEXTS();
        dwError = NTLMCreateKeys(pCtxt);
        NTLM_UNLOCK_CONTEXTS();

        BAIL_ON_NTLM_ERROR(dwError);
    }

    *pContext = pCtxt;
    memcpy(pOutputToken, &outputToken, sizeof(SEC_BUFFER));
    ZERO_STRUCT(outputToken);

    NTLMDumpContext(D_ERROR, pCtxt); /*@todo - dumb this down to D_CTXT */

error:

    if (dwError == LSA_ERROR_SUCCESS && msgError)
        dwError = msgError;

    (*pdwMinorStatus) = dwError;

    /* context is invalid after hard error */
    if (LSA_ERROR_MASK(dwError) && pCtxt)
        NTLMRemoveContext(pCtxt);

    NTLMFreeSecBuffer(&outputToken);
    NTLMDereferenceCredential(pCred);
    NTLMDereferenceContext(pCtxt);

    return dwError;

}


ULONG
NTLMContextGetNegotiateFlags(
    PNTLM_CONTEXT pCtxt
)
{
    ULONG negFlags;
    NTLM_LOCK_CONTEXTS(pCtxt);
    negFlags = pCtxt->negotiateFlags;
    NTLM_UNLOCK_CONTEXTS(pCtxt);

    return negFlags;

}


DWORD
NTLMContextGetMarshaledCreds(
    PNTLM_CONTEXT pCtxt,
    PSEC_BUFFER credCopy
)
{
    DWORD dwError;
    NTLM_LOCK_CONTEXTS(pCtxt);
    dwError = NTLMAllocCopySecBuffer(
                    credCopy,
                    &pCtxt->cred->marshaledCredential
                    );


    NTLM_UNLOCK_CONTEXTS(pCtxt);
    return dwError;

}


DWORD
NTLMGssExportSecContext(
    DWORD          *pdwMinorStatus,
    PVOID           pContext,
    DWORD           dwContextFlags,
    PSEC_BUFFER     packedContext
    )
{

    DWORD dwError;
    PNTLM_CONTEXT pCtxt = NULL;
    BYTE tmp[S_BUFLEN];

    NTLM_PACKED_CONTEXT prepackedContext;

    ZERO_STRUCT(prepackedContext);

    pCtxt = NTLMLocateContext(
                    (PNTLM_CONTEXT) pContext,
                    NULL,
                    CONTEXT_BOTH
                    );

    if (pCtxt == NULL)
    {
        dwError = LSA_ERROR_NO_CONTEXT;
        BAIL_ON_NTLM_ERROR(dwError);
    }

    /* lsa doesn't need this context anymore */
    NTLMRemoveContext(pCtxt);

    prepackedContext.contextFlags = pCtxt->flags;
    prepackedContext.negotiateFlags = pCtxt->negotiateFlags;
    prepackedContext.baseSessionKey.length =
        prepackedContext.baseSessionKey.maxLength = pCtxt->baseSessionKey.length;

    memcpy(tmp, pCtxt->baseSessionKey.buffer, pCtxt->baseSessionKey.length);
    prepackedContext.baseSessionKey.buffer = tmp;

    if (pCtxt->flags & CONTEXT_SERVER)
    {
        dwError = LsaCopyLsaString(
                    &prepackedContext.peerName,
                    &pCtxt->peerName
                    );

        BAIL_ON_NTLM_ERROR(dwError);

        dwError = LsaCopyLsaString(
                    &prepackedContext.peerDomain,
                    &pCtxt->peerDomain
                    );

        BAIL_ON_NTLM_ERROR(dwError);
    }


    dwError = NTLMPackContext(
                    &prepackedContext,
                    packedContext
                    );

    BAIL_ON_NTLM_ERROR(dwError);

error:

    (*pdwMinorStatus) = dwError;

    LsaFreeLsaString(&prepackedContext.peerDomain);
    LsaFreeLsaString(&prepackedContext.peerName);

    /* this should delete the context */
    NTLMDereferenceContext(pCtxt);

    return dwError;
}

DWORD
NTLMGssDeleteSecContext(
    DWORD          *pdwMinorStatus,
    PVOID           pContext
    )
{

    DWORD dwError = LSA_ERROR_SUCCESS;
    PNTLM_CONTEXT pCtxt = NULL;

    pCtxt = NTLMLocateContext(
                    (PNTLM_CONTEXT) pContext,
                    NULL,
                    CONTEXT_BOTH
                    );

    if (pCtxt == NULL)
    {
        dwError = LSA_ERROR_NO_CONTEXT;
        BAIL_ON_NTLM_ERROR(dwError);
    }

error:

    (*pdwMinorStatus) = dwError;

    /* this should delete the context */
    NTLMDereferenceContext(pCtxt);

    return dwError;
}



/*
 *  GSS SEC CONTEXT APIs
 *
 *  The following are stubs into IPC calls
 *
 */

OM_uint32
ntlm_gss_init_sec_context(
    OM_uint32 *minorStatus,
    gss_cred_id_t credHandle,
    gss_ctx_id_t *contextHandle,
    gss_name_t gssTargetName,
    gss_OID mechType,
    OM_uint32 reqFlags,
    OM_uint32 reqTime,
    gss_channel_bindings_t channelBindings,
    gss_buffer_t gssInputToken,
    gss_OID *actualMechtype,
    gss_buffer_t gssOutputToken,
    OM_uint32 *retFlags,
    OM_uint32 *retTime
    )
{
    DWORD dwError;
    SEC_BUFFER inputToken;
    SEC_BUFFER outputToken;
    LSA_STRING targetName;
    /* OID *actualMechType = NULL; */

    ZERO_STRUCT(targetName);
    ZERO_STRUCT(channelBindings);
    ZERO_STRUCT(outputToken);

    *minorStatus = GSS_S_COMPLETE;

    dwError = NTLMInitializedCheck();
    BAIL_ON_NTLM_ERROR(dwError);

    /* @todo - validate parameters */
    if (credHandle == NULL)
    {
        dwError = LSA_ERROR_NO_CRED;
        BAIL_ON_NTLM_ERROR(dwError);
    }

    if (contextHandle == NULL)
    {
        dwError = LSA_ERROR_NO_CONTEXT;
        BAIL_ON_NTLM_ERROR(dwError);
    }

    /* @todo - vallidate OID / mech type  == NTLM*/

    /* @todo - validate buffers */
    MAKE_SECBUFFER(&inputToken, gssInputToken);

    /* @todo - NT name is simply char */
    dwError = NTLMTranslateGSSName(
                        gssTargetName,
                        &targetName
                        );

    BAIL_ON_NTLM_ERROR(dwError);

    /* @todo - translate OID */
    /* dwError = NTLMMarshallOID(); */
    /* BAIL_ON_LSA_ERROR(dwError); */

    /* @todo - translate channel_bindings */
    /* dwError = NTLMMarshallChannelBindings(); */
    /* BAIL_ON_LSA_ERROR(dwError); */

    dwError = NTLMGssInitSecContext(
                    minorStatus,
                    (PVOID) credHandle,
                    (PVOID*)contextHandle,
                    &targetName,
                    reqFlags,
                    reqTime,
                    &inputToken,
                    &outputToken,
                    (DWORD*)retFlags,
                    (DWORD*)retTime
                    );

    if (LSA_ERROR_MASK(dwError))
       goto error;

    MAKE_GSS_BUFFER(gssOutputToken, &outputToken);
    outputToken.buffer = NULL;

cleanup:

    NTLM_SAFE_FREE(outputToken.buffer);

    return NTLMTranslateMajorStatus(dwError);

error:

    (*minorStatus) = NTLMTranslateMinorStatus(*minorStatus);

    goto cleanup;

}


OM_uint32
ntlm_gss_accept_sec_context(
     OM_uint32 *minorStatus,
     gss_ctx_id_t *contextHandle,
     gss_cred_id_t credHandle,
     gss_buffer_t gssInputToken,
     gss_channel_bindings_t inputChanBindings,
     gss_name_t *gssSrcName,
     gss_OID *gssMechType,
     gss_buffer_t gssOutputToken,
     OM_uint32 *retFlags,
     OM_uint32 *retTime,
     gss_cred_id_t *delegatedCredHandle
     )
{
    DWORD dwError;
    SEC_BUFFER inputToken;
    SEC_BUFFER outputToken;
    LSA_STRING srcName;

    ZERO_STRUCT(srcName);

    *minorStatus = GSS_S_COMPLETE;

    dwError = NTLMInitializedCheck();
    BAIL_ON_NTLM_ERROR(dwError);

    /* @todo - vallidate OID / mech type  == NTLM*/

    MAKE_SECBUFFER(&inputToken, gssInputToken);

    /* @todo - translate OID */
    /* dwError = NTLMMarshallOID(); */
    /* BAIL_ON_LSA_ERROR(dwError); */

    /* @todo - channel_bindings */

    dwError = NTLMGssAcceptSecContext(
                        minorStatus,
                        (PVOID) credHandle,
                        (PVOID*)contextHandle,
                        &inputToken,
                        &srcName,
                        &outputToken,
                        (DWORD*) retFlags,
                        (PDWORD) retTime
                        );

    if (LSA_ERROR_MASK(dwError))
        goto error;

    MAKE_GSS_BUFFER(gssOutputToken, &outputToken);
    outputToken.buffer = NULL;

    /* @todo translate actual mech type back */
    /* @todo src name */


cleanup:

    NTLM_SAFE_FREE(outputToken.buffer);
    return NTLMTranslateMajorStatus(dwError);

error:

    (*minorStatus) = NTLMTranslateMinorStatus(*minorStatus);
    goto cleanup;

}

/* @todo - finish the below */
OM_uint32
ntlm_gss_inquire_context(
    OM_uint32 *minor_status,
    gss_ctx_id_t context_handle,
    gss_name_t *initiator_name,
    gss_name_t *acceptor_name,
    OM_uint32 *lifetime_rec,
    gss_OID *mech_type,
    OM_uint32 *ret_flags,
    int *locally_initiated,
    int *open
	)
{
	/* lookup context handle in local table */

	/* fail / not */

    return GSS_S_UNAVAILABLE;

}

OM_uint32
ntlm_gss_delete_sec_context(
     OM_uint32 *minorStatus,
     gss_ctx_id_t *contextHandle,
     gss_buffer_t outputToken
     )
{
    DWORD dwError;
    DWORD majorStatus = LSA_ERROR_SUCCESS;
    PNTLM_CONTEXT gssContext = NULL;

    if (contextHandle == NULL)
        return GSS_S_NO_CONTEXT;

    gssContext = NTLMLocateContext((PNTLM_CONTEXT) contextHandle, NULL, 0);
    if (!gssContext)
        BAIL_WITH_LSA_ERROR(LSA_ERROR_INVALID_CONTEXT);

    NTLMRemoveContext(gssContext);

error:

    /* this should delete the context */
    NTLMDereferenceContext(gssContext);

    (*minorStatus) = NTLMTranslateMinorStatus(*minorStatus);

    return NTLMTranslateMajorStatus(majorStatus);
}


OM_uint32
ntlm_gss_process_context_token(
     OM_uint32 *minor_status,
     gss_ctx_id_t context_handle,
     gss_buffer_t token_buffer
	)
{
    return GSS_S_UNAVAILABLE;
}

OM_uint32
ntlm_gss_export_sec_context(
    OM_uint32		*minor_status,
    gss_ctx_id_t	*context_handle,
    gss_buffer_t	interprocess_token
	)
{
    return GSS_S_UNAVAILABLE;
}


OM_uint32
ntlm_gss_import_sec_context(
    OM_uint32		*minor_status,
    gss_buffer_t	interprocess_token,
    gss_ctx_id_t	*context_handle
	)
{
    return GSS_S_UNAVAILABLE;
}

OM_uint32
ntlm_gss_context_time(
     OM_uint32 *minor_status,
     gss_ctx_id_t context_handle,
     OM_uint32 *time_rec
     )
{

	/* lookup context handle in local table */

	/* fail / not */
    return GSS_S_UNAVAILABLE;

}

