/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */
/*
 * Copyright Likewise Software
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
 *        srvgss_p.h
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Elements
 *
 *        GSS Support (Private Header)
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 */

#ifndef __SRVGSS_P_H__
#define __SRVGSS_P_H__

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
                LWIO_LOG_ERROR("KRB5 Error at %s:%d: %s",              \
                        __FILE__,                                     \
                        __LINE__,                                     \
                        pszKrb5Error);                                \
                krb5_free_error_message(ctx, pszKrb5Error);           \
            }                                                         \
        } else {                                                      \
            LWIO_LOG_ERROR("KRB5 Error at %s:%d [Code:%d]",            \
                    __FILE__,                                         \
                    __LINE__,                                         \
                    ret);                                             \
        }                                                             \
        if (ret == KRB5KDC_ERR_KEY_EXP) {                             \
            ntStatus = LWIO_ERROR_PASSWORD_EXPIRED;                    \
        } else if (ret == KRB5_LIBOS_BADPWDMATCH) {                   \
            ntStatus = LWIO_ERROR_PASSWORD_MISMATCH;                   \
        } else if (ret == KRB5KRB_AP_ERR_SKEW) {                      \
            ntStatus = LWIO_ERROR_CLOCK_SKEW;                          \
        } else if (ret == ENOENT) {                                   \
            ntStatus = LWIO_ERROR_KRB5_NO_KEYS_FOUND;                  \
        } else {                                                      \
            ntStatus = LWIO_ERROR_KRB5_CALL_FAILED;                    \
        }                                                             \
        goto error;                                                   \
    }

#define SRV_PRINCIPAL       "not_defined_in_RFC4178@please_ignore"

typedef enum
{
    SRV_GSS_CONTEXT_STATE_INITIAL = 0,
    SRV_GSS_CONTEXT_STATE_HINTS,
    SRV_GSS_CONTEXT_STATE_NEGOTIATE,
    SRV_GSS_CONTEXT_STATE_COMPLETE
} SRV_GSS_CONTEXT_STATE;

typedef struct _SRV_GSS_NEGOTIATE_CONTEXT
{
    SRV_GSS_CONTEXT_STATE state;

    gss_ctx_id_t* pGssContext;

} SRV_GSS_NEGOTIATE_CONTEXT, *PSRV_GSS_NEGOTIATE_CONTEXT;

static
NTSTATUS
SrvGssContinueNegotiate(
    PSRV_GSS_NEGOTIATE_CONTEXT pGssNegotiate,
    PBYTE                      pSecurityInputBlob,
    ULONG                      ulSecurityInputBlobLen,
    PBYTE*                     ppSecurityOutputBlob,
    ULONG*                     pulSecurityOutputBloblen
    );

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

#endif /* __SRVGSS_P_H__ */
