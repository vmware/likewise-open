
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


