#include "includes.h"

#define BAIL_ON_SEC_ERROR(ulMajorStatus) \
    if ((ulMajorStatus != GSS_S_COMPLETE)\
            && (ulMajorStatus != GSS_S_CONTINUE_NEEDED)) {\
        goto sec_error; \
    }

#define BAIL_ON_KRB_ERROR(ctx, ret)                                   \
    if (ret) {                                                        \
        if (ctx)  {                                                   \
            PCSTR pszKrb5Error = krb5_get_error_message(ctx, ret);    \
            if (pszKrb5Error) {                                       \
                SMB_LOG_ERROR("KRB5 Error at %s:%d: %s",              \
                        __FILE__,                                     \
                        __LINE__,                                     \
                        pszKrb5Error);                                \
                krb5_free_error_message(ctx, pszKrb5Error);           \
            }                                                         \
        } else {                                                      \
            SMB_LOG_ERROR("KRB5 Error at %s:%d [Code:%d]",            \
                    __FILE__,                                         \
                    __LINE__,                                         \
                    ret);                                             \
        }                                                             \
        if (ret == KRB5KDC_ERR_KEY_EXP) {                             \
            ntStatus = SMB_ERROR_PASSWORD_EXPIRED;                    \
        } else if (ret == KRB5_LIBOS_BADPWDMATCH) {                   \
            ntStatus = SMB_ERROR_PASSWORD_MISMATCH;                   \
        } else if (ret == KRB5KRB_AP_ERR_SKEW) {                      \
            ntStatus = SMB_ERROR_CLOCK_SKEW;                          \
        } else if (ret == ENOENT) {                                   \
            ntStatus = SMB_ERROR_KRB5_NO_KEYS_FOUND;                  \
        } else {                                                      \
            ntStatus = SMB_ERROR_KRB5_CALL_FAILED;                    \
        }                                                             \
        goto error;                                                   \
    }

#define SRV_PRINCIPAL "not_defined_in_RFC4178@please_ignore"
#define SRV_KRB5_CACHE_PATH "MEMORY:lwio_krb5_cc"

static BOOLEAN gbSrvKrb5Initialized = FALSE;
static PSTR    gpszUsername = NULL;

static
void
srv_display_status(
    PCSTR     pszId,
    OM_uint32 maj_stat,
    OM_uint32 min_stat
    );

static
void
srv_display_status_1(
    PCSTR     pszId,
    OM_uint32 code,
    int       type
    );

static
NTSTATUS
SrvGetTGTFromKeytab(
    PCSTR  pszUserName,
    PCSTR  pszPassword,
    PCSTR  pszCachePath,
    PDWORD pdwGoodUntilTime
    );

static
NTSTATUS
SrvDestroyKrb5Cache(
    PCSTR pszCachePath
    );

static
NTSTATUS
SrvSetDefaultKrb5CachePath(
    PCSTR pszCachePath,
    PSTR* ppszOrigCachePath
    );

NTSTATUS
SrvGssInit(
    VOID
    )
{
    NTSTATUS ntStatus = 0;
    PSTR     pszUsername = NULL;
    CHAR     szHostname[256];
    PSTR     pszDomain = NULL;

    if (!gbSrvKrb5Initialized)
    {
        DWORD i = 0;
        DWORD j = 0;
        DWORD dwLength = 0;

        if (gethostname(szHostname, sizeof(szHostname)) != 0)
        {
            ntStatus = LwUnixErrnoToNtStatus(errno);
            BAIL_ON_NT_STATUS(ntStatus);
        }

        ntStatus = LWNetGetCurrentDomain(&pszDomain);
        BAIL_ON_NT_STATUS(ntStatus);

        dwLength = strlen(szHostname) + strlen(pszDomain) + 3;

        ntStatus = SMBAllocateMemory(
                        dwLength,
                        (PVOID*)&pszUsername);
        BAIL_ON_NT_STATUS(ntStatus);

        for (i = 0; i < strlen(szHostname); i++)
        {
            pszUsername[i] = toupper(szHostname[i]);
        }
        pszUsername[i++] = '$';
        pszUsername[i++] = '@';
        for (; j  < strlen(pszDomain); j++)
        {
             pszUsername[i++] = toupper(pszDomain[j]);
        }

        ntStatus = SrvGetTGTFromKeytab(
                        pszUsername,
                        NULL,
                        SRV_KRB5_CACHE_PATH,
                        NULL);
        BAIL_ON_NT_STATUS(ntStatus);

        gbSrvKrb5Initialized = TRUE;
        gpszUsername = pszUsername;
        pszUsername = NULL;
    }

cleanup:

    SMB_SAFE_FREE_STRING(pszUsername);

    if (pszDomain)
    {
        LWNetFreeString(pszDomain);
    }

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
SrvGssCreate(
    PSRV_GSS_CONTEXT* ppGssContext,
    PBYTE*            ppSessionKey,
    PULONG            pulSessionKeyLength
    )
{
    NTSTATUS ntStatus = 0;
    ULONG ulMajorStatus = 0;
    ULONG ulMinorStatus = 0;
    PSRV_GSS_CONTEXT pGssContext = NULL;
    gss_buffer_desc input_name = GSS_C_EMPTY_BUFFER;
    gss_buffer_desc input_desc = GSS_C_EMPTY_BUFFER;
    gss_buffer_desc output_desc = GSS_C_EMPTY_BUFFER;
    gss_name_t      target_name = GSS_C_NO_NAME;
    PBYTE pSessionKey = NULL;
    ULONG ulSessionKeyLength = 0;
    ULONG ret_flags = 0;
    PSTR  pszCurrentCachePath = NULL;

    static gss_OID_desc gss_spnego_mech_oid_desc =
      {6, (void *)"\x2b\x06\x01\x05\x05\x02"};

    ntStatus = SrvSetDefaultKrb5CachePath(
                    SRV_KRB5_CACHE_PATH,
                    &pszCurrentCachePath);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBAllocateMemory(
                    sizeof(SRV_GSS_CONTEXT),
                    (PVOID*)&pGssContext);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBAllocateMemory(
                        sizeof(gss_ctx_id_t),
                        (PVOID*)&pGssContext->pGssContext);
    BAIL_ON_NT_STATUS(ntStatus);

    *pGssContext->pGssContext = GSS_C_NO_CONTEXT;

    input_name.value = gpszUsername;
    input_name.length = strlen(gpszUsername);

    ulMajorStatus = gss_import_name(
                        (OM_uint32 *)&ulMinorStatus,
                        &input_name,
                        (gss_OID) gss_nt_krb5_name,
                        &target_name);

    srv_display_status("gss_import_name", ulMajorStatus, ulMinorStatus);

    BAIL_ON_SEC_ERROR(ulMajorStatus);

    ulMajorStatus = gss_init_sec_context(
                            (OM_uint32 *)&ulMinorStatus,
                            GSS_C_NO_CREDENTIAL,
                            pGssContext->pGssContext,
                            target_name,
                            &gss_spnego_mech_oid_desc,
                            0,
                            0,
                            GSS_C_NO_CHANNEL_BINDINGS,
                            &input_desc,
                            NULL,
                            &output_desc,
                            &ret_flags,
                            NULL);

    srv_display_status("gss_init_sec_context", ulMajorStatus, ulMinorStatus);

    BAIL_ON_SEC_ERROR(ulMajorStatus);

    switch (ulMajorStatus)
    {
        case GSS_S_CONTINUE_NEEDED:

            pGssContext->state = SRV_GSS_CONTEXT_STATE_NEGOTIATE;

            break;

        case GSS_S_COMPLETE:

            pGssContext->state = SRV_GSS_CONTEXT_STATE_COMPLETE;

            break;

        default:

            ntStatus = SMB_ERROR_GSS;
            BAIL_ON_NT_STATUS(ntStatus);

            break;
    }

    if (output_desc.length)
    {
        ntStatus = SMBAllocateMemory(
                        output_desc.length,
                        (PVOID*)&pSessionKey);
        BAIL_ON_NT_STATUS(ntStatus);

        memcpy(pSessionKey, output_desc.value, output_desc.length);

        ulSessionKeyLength = output_desc.length;
    }

    *ppGssContext = pGssContext;
    *ppSessionKey = pSessionKey;
    *pulSessionKeyLength = ulSessionKeyLength;

cleanup:

    gss_release_buffer(&ulMinorStatus, &output_desc);

    gss_release_name(&ulMinorStatus, &target_name);

    if (pszCurrentCachePath)
    {
        SrvSetDefaultKrb5CachePath(
            pszCurrentCachePath,
            NULL);

        SMBFreeMemory(pszCurrentCachePath);
    }

    return ntStatus;

sec_error:

    ntStatus = SMB_ERROR_GSS;

error:

    *ppGssContext = NULL;

    if (pGssContext)
    {
        SrvGssFree(pGssContext);
    }

    goto cleanup;
}

NTSTATUS
SrvGssNegotiate(
    PSRV_GSS_CONTEXT pGssContext,
    PBYTE            pSecurityInputBlob,
    ULONG            ulSecurityInputBlobLen,
    PBYTE*           ppSecurityOutputBlob,
    ULONG*           pulSecurityOutputBloblen
    )
{
    NTSTATUS ntStatus = 0;
    ULONG ulMajorStatus = 0;
    ULONG ulMinorStatus = 0;
    gss_buffer_desc input_desc = {0};
    gss_buffer_desc output_desc = {0};
    ULONG ret_flags = 0;
    PBYTE pSecurityBlob = NULL;
    ULONG ulSecurityBlobLength = 0;

    static gss_OID_desc gss_spnego_mech_oid_desc =
                              {6, (void *)"\x2b\x06\x01\x05\x05\x02"};
    static gss_OID gss_spnego_mech_oid = &gss_spnego_mech_oid_desc;

    if (pGssContext->state == SRV_GSS_CONTEXT_STATE_COMPLETE)
    {
        goto cleanup;
    }

    input_desc.length = ulSecurityInputBlobLen;
    input_desc.value = pSecurityInputBlob;

    ulMajorStatus = gss_accept_sec_context(
                        (OM_uint32 *)&ulMinorStatus,
                        pGssContext->pGssContext,
                        NULL,
                        &input_desc,
                        NULL,
                        NULL,
                        &gss_spnego_mech_oid,
                        &output_desc,
                        &ret_flags,
                        NULL,
                        NULL);

    srv_display_status("gss_accept_sec_context", ulMajorStatus, ulMinorStatus);

    BAIL_ON_SEC_ERROR(ulMajorStatus);

    switch (ulMajorStatus)
    {
        case GSS_S_CONTINUE_NEEDED:

            pGssContext->state = SRV_GSS_CONTEXT_STATE_NEGOTIATE;

            break;

        case GSS_S_COMPLETE:

            pGssContext->state = SRV_GSS_CONTEXT_STATE_COMPLETE;

            break;

        default:

            ntStatus = SMB_ERROR_GSS;
            BAIL_ON_NT_STATUS(ntStatus);

            break;
    }

    if (output_desc.length)
    {
        ntStatus = SMBAllocateMemory(
                        output_desc.length,
                        (PVOID*)&pSecurityBlob);
        BAIL_ON_NT_STATUS(ntStatus);

        memcpy(pSecurityBlob, output_desc.value, output_desc.length);

        ulSecurityBlobLength = output_desc.length;
    }

    *ppSecurityOutputBlob = pSecurityBlob;
    *pulSecurityOutputBloblen = ulSecurityBlobLength;

cleanup:

    gss_release_buffer(&ulMinorStatus, &output_desc);

    return ntStatus;

sec_error:

    ntStatus = SMB_ERROR_GSS;

error:

    *ppSecurityOutputBlob = NULL;
    *pulSecurityOutputBloblen = 0;

    SMB_SAFE_FREE_MEMORY(pSecurityBlob);

    goto cleanup;
}

VOID
SrvGssFree(
    PSRV_GSS_CONTEXT pGssContext
    )
{
    ULONG ulMinorStatus = 0;

    if (pGssContext->pGssContext &&
        (*pGssContext->pGssContext != GSS_C_NO_CONTEXT))
    {
        gss_delete_sec_context(
                        &ulMinorStatus,
                        pGssContext->pGssContext,
                        GSS_C_NO_BUFFER);

        SMBFreeMemory(pGssContext->pGssContext);
    }

    SMBFreeMemory(pGssContext);
}

NTSTATUS
SrvGssShutdown(
    VOID
    )
{
    NTSTATUS ntStatus = 0;

    if (gbSrvKrb5Initialized)
    {
        SMB_SAFE_FREE_STRING(gpszUsername);

        ntStatus = SrvDestroyKrb5Cache(SRV_KRB5_CACHE_PATH);
        BAIL_ON_NT_STATUS(ntStatus);

        gbSrvKrb5Initialized = FALSE;
    }

error:

    return ntStatus;
}

static
void
srv_display_status(
    PCSTR     pszId,
    OM_uint32 maj_stat,
    OM_uint32 min_stat
    )
{
     srv_display_status_1(pszId, maj_stat, GSS_C_GSS_CODE);
     srv_display_status_1(pszId, min_stat, GSS_C_MECH_CODE);
}

static
void
srv_display_status_1(
    PCSTR     pszId,
    OM_uint32 code,
    int       type
    )
{
    OM_uint32 maj_stat, min_stat;
    gss_buffer_desc msg;
    OM_uint32 msg_ctx;

    if ( code == 0 )
    {
        return;
    }

    msg_ctx = 0;
    while (1)
    {
        maj_stat = gss_display_status(&min_stat, code,
                                      type, GSS_C_NULL_OID,
                                      &msg_ctx, &msg);

        switch(code)
        {
#ifdef WIN32
            case SEC_E_OK:
            case SEC_I_CONTINUE_NEEDED:
#else
            case GSS_S_COMPLETE:
            case GSS_S_CONTINUE_NEEDED:
#endif
                SMB_LOG_VERBOSE("GSS-API error calling %s: %d (%s)\n",
                        pszId, code,
                        (char *)msg.value);
                break;

            default:

                SMB_LOG_ERROR("GSS-API error calling %s: %d (%s)\n",
                        pszId, code,
                        (char *)msg.value);
        }

        (void) gss_release_buffer(&min_stat, &msg);

        if (!msg_ctx)
            break;
    }
}

static
NTSTATUS
SrvGetTGTFromKeytab(
    PCSTR  pszUserName,
    PCSTR  pszPassword,
    PCSTR  pszCachePath,
    PDWORD pdwGoodUntilTime
    )
{
    NTSTATUS ntStatus = 0;
    krb5_error_code ret = 0;
    krb5_context ctx = NULL;
    krb5_creds creds = { 0 };
    krb5_ccache cc = NULL;
    krb5_keytab keytab = 0;
    krb5_principal client_principal = NULL;
    krb5_get_init_creds_opt opts;

    ret = krb5_init_context(&ctx);
    BAIL_ON_KRB_ERROR(ctx, ret);

    ret = krb5_parse_name(ctx, pszUserName, &client_principal);
    BAIL_ON_KRB_ERROR(ctx, ret);

    /* use krb5_cc_resolve to get an alternate cache */
    ret = krb5_cc_resolve(ctx, pszCachePath, &cc);
    BAIL_ON_KRB_ERROR(ctx, ret);

    ret = krb5_kt_default(ctx, &keytab);
    BAIL_ON_KRB_ERROR(ctx, ret);

    krb5_get_init_creds_opt_init(&opts);
    krb5_get_init_creds_opt_set_forwardable(&opts, TRUE);

    ret = krb5_get_init_creds_keytab(
                    ctx,
                    &creds,
                    client_principal,
                    keytab,
                    0,    /* start time     */
                    NULL, /* in_tkt_service */
                    &opts  /* options        */
                    );
    BAIL_ON_KRB_ERROR(ctx, ret);

    ret = krb5_cc_initialize(ctx, cc, client_principal);
    BAIL_ON_KRB_ERROR(ctx, ret);

    ret = krb5_cc_store_cred(ctx, cc, &creds);
    BAIL_ON_KRB_ERROR(ctx, ret);

    if (pdwGoodUntilTime)
    {
        *pdwGoodUntilTime = creds.times.endtime;
    }

error:

    if (creds.client == client_principal)
    {
        creds.client = NULL;
    }

    if (ctx)
    {
        if (client_principal)
        {
            krb5_free_principal(ctx, client_principal);
        }

        if (keytab) {
            krb5_kt_close(ctx, keytab);
        }

        if (cc) {
            krb5_cc_close(ctx, cc);
        }

        krb5_free_cred_contents(ctx, &creds);

        krb5_free_context(ctx);
    }

    return ntStatus;
}

static
NTSTATUS
SrvDestroyKrb5Cache(
    PCSTR pszCachePath
    )
{
    NTSTATUS ntStatus = 0;
    krb5_error_code ret = 0;
    krb5_context ctx = NULL;
    krb5_ccache cc = NULL;

    ret = krb5_init_context(&ctx);
    BAIL_ON_KRB_ERROR(ctx, ret);

    /* use krb5_cc_resolve to get an alternate cache */
    ret = krb5_cc_resolve(ctx, pszCachePath, &cc);
    BAIL_ON_KRB_ERROR(ctx, ret);

    ret = krb5_cc_destroy(ctx, cc);
    if (ret != 0) {
        if (ret != KRB5_FCC_NOFILE) {
            BAIL_ON_KRB_ERROR(ctx, ret);
        } else {
            ret = 0;
        }
    }

error:

    if (ctx)
    {
       krb5_free_context(ctx);
    }

    return ntStatus;
}

static
NTSTATUS
SrvSetDefaultKrb5CachePath(
    PCSTR pszCachePath,
    PSTR* ppszOrigCachePath
    )
{
    NTSTATUS ntStatus = 0;
    ULONG ulMajorStatus = 0;
    ULONG ulMinorStatus = 0;
    PSTR  pszOrigCachePath = NULL;

    // Set the default for gss
    ulMajorStatus = gss_krb5_ccache_name(
                            (OM_uint32 *)&ulMinorStatus,
                            pszCachePath,
                            (ppszOrigCachePath) ? (const char**)&pszOrigCachePath : NULL);
    BAIL_ON_SEC_ERROR(ulMajorStatus);

    if (ppszOrigCachePath)
    {
        if (!IsNullOrEmptyString(pszOrigCachePath))
        {
            ntStatus = SMBAllocateString(
                            pszOrigCachePath,
                            ppszOrigCachePath);
            BAIL_ON_NT_STATUS(ntStatus);
        }
        else
        {
            *ppszOrigCachePath = NULL;
        }
    }

    SMB_LOG_DEBUG("Cache path set to [%s]", SMB_SAFE_LOG_STRING(pszCachePath));

cleanup:

    return ntStatus;

sec_error:
error:

    if (ppszOrigCachePath)
    {
        *ppszOrigCachePath = NULL;
    }

    goto cleanup;
}
