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

/*
 *  GSS SEC CONTEXT APIs
 *  
 *  The following are stubs into IPC calls
 *
 */  

OM_uint32
gss_init_sec_context(
    OM_uint32 *minor_status,
    gss_cred_id_t cred_handle,
    gss_ctx_id_t *context_handle,
    gss_name_t target_name,
    gss_OID mech_type,
    OM_uint32 req_flags,
    OM_uint32 time_req,
    gss_channel_bindings_t input_chan_bindings,
    gss_buffer_t input_token,
    gss_OID *actual_mech_type,
    gss_buffer_t output_token,
    OM_uint32 *ret_flags,
    OM_uint32 *time_rec
    )
{
    DWORD dwError;
    DWORD minorStatus = LSA_ERROR_SUCCESS;
    gss_buffer_t tmpOutput = NULL;
    SEC_BUFFER inputToken;
    SEC_BUFFER outputToken;
    SEC_BUFFER channelBindings;
    LSA_STRING targetName;
    OID *mechType = NULL;
    OID *actualMechType = NULL;
    PLSACLIENTCONNECTIONCONTEXT pCxnCtxt = NULL;

    ZERO_STRUCT(targetName);
    ZERO_STRUCT(channelBindings);
    

    /* @todo - validate parameters */
    if (cred_handle == NULL) {
        dwError = LSA_ERROR_NO_CRED;
        BAIL_ON_LSA_ERROR(dwError);
    }
    /* @todo - vallidate OID / mech type  == NTLM*/

    /* @todo - validate buffers */

    /* translate them, if needed */
    NTLMConvertGSSBufferToSecBuffer(&inputToken, input_token);

    /* translate target name */
    dwError = NTLMTranslateGSSname(
                        target_name,
                        &targetName
                        );

    BAIL_ON_LSA_ERROR(dwError);


    /* @todo - translate OID */
    /* dwError = NTLMMarshallOID(); */
    /* BAIL_ON_LSA_ERROR(dwError); */

    /* @todo - translate channel_bindings */
    /* dwError = NTLMMarshallChannelBindings(); */
    /* BAIL_ON_LSA_ERROR(dwError); */

    dwError = LSAClientOpenContext(*pCxnCtxt);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaGssInitSecContext(
                        pCxnCtxt,
                        &minorStatus,
                        (PVOID) cred_handle,
                        (PVOID) context_handle,
                        &targetName,
                        mechType,
                        (DWORD) req_flags,
                        (DWORD) time_req,
                        &channelBindings,
                        &inputToken,
                        &actualMechType,
                        &outputToken, 
                        (PDWORD) ret_flags,
                        (PDWORD) time_rec
                        );

    BAIL_ON_LSA_ERROR(dwError);
    BAIL_ON_LSA_ERROR(minorStatus);

    /* @todo - debug logging here on success */

    /* translate return vals */
    tmpOutput = (gss_buffer_t) NTLM_ALLOC(sizeof(gss_buffer_t));
    if (!tmpOutput) {
        dwError = LSA_NO_MEMORY;
        BAIL_ON_LSA_ERROR(dwError);
    }
        
    NTLMConvertSecBufferToGSSBufferAllocate(outputToken, &tmpOutput);

    /* @todo - finalized context moved into user space */
    if (dwError == LSA_ERROR_SUCCESS) {

        dwError = LsaGssFinalizeSecContext();
        BAIL_ON_LSA_ERROR(dwError);

    }

    /* @todo translate actual mech type back */    
    output_token = tmpOutput;

error:
    
    SECBUFFER_FREE(outputToken);
    NTLM_FREE(tmpOutput);
    LSAClientCloseContext(pCxnCtxt);

    return NTLMTranslateLsaErrorToGssError(dwError);
}
	 




OM_uint32
ntlm_gss_accept_sec_context(
     OM_uint32 *minor_status,
     gss_ctx_id_t *context_handle,
     gss_cred_id_t verifier_cred_handle,
     gss_buffer_t input_token,
     gss_channel_bindings_t input_chan_bindings,
     gss_name_t *src_name,
     gss_OID *mech_type,
     gss_buffer_t output_token,
     OM_uint32 *ret_flags,
     OM_uint32 *time_rec,
     gss_cred_id_t *delegated_cred_handle
     )
{
    DWORD dwError;
    DWORD minorStatus = LSA_ERROR_SUCCESS;
    gss_buffer_t tmpOutput = NULL;
    SEC_BUFFER inputToken;
    SEC_BUFFER outputToken;
    SEC_BUFFER channelBindings;
    LSA_STRING srcName;
    OID *mechType = NULL;
    OID *actualMechType = NULL;
    PLSACLIENTCONNECTIONCONTEXT pCxnCtxt = NULL;

    ZERO_STRUCT(srcName);
    ZERO_STRUCT(channelBindings);

    /* validate parameters */

    /* @todo - vallidate OID / mech type  == NTLM*/

    /* @todo - validate buffers */


    /* translate them, if needed */
    NTLMConvertGSSBufferToSecBuffer(&inputToken, input_token);

    /* translate src name  @todo - do we need this?*/
    dwError = NTLMTranslateGSSname(
                        src_name,
                        &srcName
                        );

    BAIL_ON_LSA_ERROR(dwError);

    /* @todo - translate OID */
    /* dwError = NTLMMarshallOID(); */
    /* BAIL_ON_LSA_ERROR(dwError); */

    /* @todo - translate channel_bindings */
    /* dwError = NTLMMarshallChannelBindings(); */
    /* BAIL_ON_LSA_ERROR(dwError); */

    dwError = LSAClientOpenContext(*pCxnCtxt);
    BAIL_ON_LSA_ERROR(dwError);

    /* pass over IPC */
    dwError = LsaGssAcceptSecContext(
                        &minorStatus,
                        (PVOID) cred_handle,
                        (PVOID) context_handle,
                        &inputToken,
                        &channelBindings,
                        &srcName,
                        mechType,
                        &outputToken,
                        (PDWORD) ret_flags,
                        (PDWORD) time_rec,
                        (PVOID) delegated_cred_handle
                        );

    BAIL_ON_LSA_ERROR(dwError);
    BAIL_ON_LSA_ERROR(minorStatus);

    /* @todo - debug logging here on success */

    /* translate return vals */
    tmpOutput = (gss_buffer_t) NTLM_ALLOC(sizeof(gss_buffer_t));
    if (!tmpOutput) {
        dwError = LSA_NO_MEMORY;
        BAIL_ON_LSA_ERROR(dwError);
    }
        
    NTLMConvertSecBufferToGSSBufferAllocate(outputToken, &tmpOutput);

    /* @todo - finalized context moved into user space */
    if (dwError == LSA_ERROR_SUCCESS) {

        dwError = LsaGssFinalizeSecContext();
        BAIL_ON_LSA_ERROR(dwError);

    }

    /* @todo translate actual mech type back */    
    output_token = tmpOutput;
    SECBUFFER_ZERO(tmpOutput);

error:
    
    SECBUFFER_FREE(outputToken);
    NTLM_FREE(tmpOutput);
    LSAClientCloseContext(pCxnCtxt);

    return NTLMTranslateLsaErrorToGssError(dwError);


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
     OM_uint32 *minor_status,
     gss_ctx_id_t *context_handle,
     gss_buffer_t output_token
	)
{
    return GSS_S_UNAVAILABLE;
    
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

/*
 *
 * libgssntlm context management functions
 *
 * Contexts are maintained solely in lsassd, until they are eventually
 * satisfied, and then exported into libgssntlm (and its calling application
 * process space).
 *
 */ 













