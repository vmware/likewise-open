
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
