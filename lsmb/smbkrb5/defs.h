#ifndef __DEFS_H__
#define __DEFS_H__

typedef gss_ctx_id_t CtxtHandle, *PCtxtHandle;

#define BAIL_ON_SEC_ERROR(dwMajorStatus) \
    if ((dwMajorStatus != GSS_S_COMPLETE)\
            && (dwMajorStatus != GSS_S_CONTINUE_NEEDED)) {\
        goto sec_error; \
    }

#define BAIL_ON_SMB_KRB_ERROR(ctx, ret)                               \
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
            dwError = SMB_ERROR_PASSWORD_EXPIRED;                     \
        } else if (ret == KRB5_LIBOS_BADPWDMATCH) {                   \
            dwError = SMB_ERROR_PASSWORD_MISMATCH;                    \
        } else if (ret == KRB5KRB_AP_ERR_SKEW) {                      \
            dwError = SMB_ERROR_CLOCK_SKEW;                           \
        } else if (ret == ENOENT) {                                   \
            dwError = SMB_ERROR_KRB5_NO_KEYS_FOUND;                   \
        } else {                                                      \
            dwError = SMB_ERROR_KRB5_CALL_FAILED;                     \
        }                                                             \
        goto error;                                                   \
    }

typedef enum
{
    SMB_GSS_SEC_CONTEXT_STATE_INITIAL = 0,
    SMB_GSS_SEC_CONTEXT_STATE_NEGOTIATE,
    SMB_GSS_SEC_CONTEXT_STATE_COMPLETE
} SMB_GSS_SEC_CONTEXT_STATE;

#endif
