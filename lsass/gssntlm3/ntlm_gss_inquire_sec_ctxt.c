
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
