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
 *        ntlmgsswrapper.c
 *
 * Abstract:
 *        GSS wrapper functions for NTLM implementation.
 *
 * Authors: Marc Guy (mguy@likewisesoftware.com)
 *
 */

#include "client.h"

static GSS_MECH_CONFIG gNtlmMech =
{
    {GSS_MECH_NTLM_LEN, GSS_MECH_NTLM},
    NULL,

    ntlm_gss_acquire_cred,
    ntlm_gss_release_cred,
    NULL, //ntlm_gss_init_sec_cred,
    ntlm_gss_accept_sec_context,
    NULL, //ntlm_gss_process_context_token,
    ntlm_gss_delete_sec_context,
    NULL, //ntlm_gss_context_time,
    NULL, //ntlm_gss_get_mic,
    NULL, //ntlm_gss_verify_mic,
    NULL, //ntlm_gss_wrap,
    NULL, //ntlm_gss_unwrap,
    NULL, //ntlm_gss_display_status,
    NULL, //ntlm_gss_indicate_mechs,
    NULL, //ntlm_gss_compare_name,
    NULL, //ntlm_gss_display_name,
    ntlm_gss_import_name,
    ntlm_gss_release_name,
    ntlm_gss_inquire_cred,
    NULL, //ntlm_gss_add_cred,
    NULL, //ntlm_gss_export_sec_context,
    NULL, //ntlm_gss_import_sec_context,
    NULL, //ntlm_gss_inquire_cred_by_mech,
    NULL, //ntlm_gss_inquire_names_for_mech,
    NULL, //ntlm_gss_inquire_context,
    NULL, //ntlm_gss_internal_release_oid,
    NULL, //ntlm_gss_wrap_size_limit,
    NULL, //ntlm_gss_export_name,
    NULL, //ntlm_gss_store_cred,
    NULL, //ntlm_gss_inquire_sec_context_by_oid,
    NULL, //ntlm_gss_inquire_cred_by_oid,
    NULL, //ntlm_gss_set_sec_context_option,
    NULL, //ntlm_gssspi_set_cred_option,
    NULL, //ntlm_gssspi_mech_invoke,
    NULL, //ntlm_gss_wrap_aead,
    NULL, //ntlm_gss_unwrap_aead,
    NULL, //ntlm_gss_wrap_iov,
    NULL, //ntlm_gss_unwrap_iov,
    NULL, //ntlm_gss_wrap_iov_length,
    NULL, //ntlm_gss_complete_auth_token,
    NULL, //ntlm_gss_inquire_context2
};

OM_uint32
ntlm_gss_acquire_cred(
    OM_uint32 *pMinorStatus,
    const gss_name_t pDesiredName,
    OM_uint32 nTimeReq,
    const gss_OID_set pDesiredMechs,
    gss_cred_usage_t CredUsage,
    gss_cred_id_t* pOutputCredHandle,
    gss_OID_set* pActualMechs,
    OM_uint32 *pTimeRec
    )
{
    OM_uint32 MajorStatus = GSS_S_COMPLETE;
    OM_uint32 MinorStatus = LW_ERROR_SUCCESS;
    NTLM_CRED_HANDLE CredHandle = NULL;
    TimeStamp tsExpiry = 0;
    DWORD fCredentialUse = 0;

    *pOutputCredHandle = (gss_cred_id_t)CredHandle;

    if(pActualMechs)
    {
        *pActualMechs = NULL;
    }

    if(pTimeRec)
    {
        *pTimeRec = 0;
    }

    switch(CredUsage)
    {
    case GSS_C_ACCEPT:
        fCredentialUse = NTLM_CRED_INBOUND;
        break;
    case GSS_C_INITIATE:
        fCredentialUse = NTLM_CRED_OUTBOUND;
        break;
    default:
        MinorStatus = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LW_ERROR(MinorStatus);
    }

    MinorStatus = NtlmClientAcquireCredentialsHandle(
        (SEC_CHAR*)pDesiredName,
        "NTLM",
        fCredentialUse,
        NULL,
        NULL,
        &CredHandle,
        &tsExpiry
        );
    BAIL_ON_LW_ERROR(MinorStatus);

cleanup:
    *pMinorStatus = MinorStatus;
    *pOutputCredHandle = (gss_cred_id_t)CredHandle;

    if(pActualMechs)
    {
        *pActualMechs = NULL;
    }

    if(pTimeRec)
    {
        *pTimeRec = 0;
    }

    return MajorStatus;
error:
    if (MajorStatus == GSS_S_COMPLETE)
    {
        MajorStatus = GSS_S_FAILURE;
    }
    goto cleanup;
}

OM_uint32
ntlm_gss_release_cred(
    OM_uint32 *pMinorStatus,
    gss_cred_id_t *pCredHandle
    )
{
    OM_uint32 MajorStatus = GSS_S_COMPLETE;
    OM_uint32 MinorStatus = LW_ERROR_SUCCESS;
    NTLM_CRED_HANDLE CredHandle = NULL;

    if (!pCredHandle)
    {
        MajorStatus = GSS_S_NO_CRED;
        MinorStatus = LW_ERROR_NO_CRED;
        BAIL_ON_LW_ERROR(MinorStatus);
    }

    CredHandle = (NTLM_CRED_HANDLE)*pCredHandle;

    MinorStatus = NtlmTransactFreeCredentialsHandle(
        &CredHandle
        );

    BAIL_ON_LW_ERROR(MinorStatus);

cleanup:
    *pMinorStatus = MinorStatus;
    return MajorStatus;
error:
    if (MajorStatus == GSS_S_COMPLETE)
    {
        MajorStatus = GSS_S_FAILURE;
    }
    goto cleanup;
}

OM_uint32
ntlm_gss_init_sec_context(
    OM_uint32 *pMinorStatus,
    const gss_cred_id_t InitiatorCredHandle,
    gss_ctx_id_t *pContextHandle,
    const gss_name_t pTargetName,
    const gss_OID pMechType,
    OM_uint32 nReqFlags,
    OM_uint32 nTimeReq,
    const gss_channel_bindings_t pInputChanBindings,
    const gss_buffer_t pInputToken,
    gss_OID *pActualMechType,
    gss_buffer_t pOutputToken,
    OM_uint32 *pRetFlags,
    OM_uint32 *pTimeRec
    )
{
    OM_uint32 MajorStatus = GSS_S_COMPLETE;
    OM_uint32 MinorStatus = LW_ERROR_SUCCESS;
    PNTLM_CONTEXT_HANDLE phContext = NULL;
    PNTLM_CONTEXT_HANDLE phNewContext = NULL;
    TimeStamp tsExpiry = 0;
    SecBufferDesc InputBuffer = {0};
    SecBufferDesc OutputBuffer = {0};
    SecBuffer InputToken = {0};
    SecBuffer OutputToken = {0};

    *pActualMechType = NULL;
    memset(pOutputToken, 0, sizeof(*pOutputToken));
    *pRetFlags = 0;
    *pTimeRec = 0;

    InputBuffer.cBuffers = 1;
    InputBuffer.pBuffers = &InputToken;

    OutputBuffer.cBuffers = 1;
    OutputBuffer.pBuffers = &OutputToken;

    InputToken.BufferType = SECBUFFER_TOKEN;
    InputToken.cbBuffer = pInputToken->length;
    InputToken.pvBuffer = pInputToken->value;

    MinorStatus = NtlmClientInitializeSecurityContext(
        (PNTLM_CRED_HANDLE)(&InitiatorCredHandle),
        phContext,
        (SEC_CHAR*)pTargetName,
        nReqFlags,
        0, // Reserved
        NTLM_NATIVE_DATA_REP,
        &InputBuffer,
        0, // Reserved
        phNewContext,
        &OutputBuffer,
        pRetFlags,
        &tsExpiry
        );

    BAIL_ON_LW_ERROR(MinorStatus);

    pOutputToken->length = OutputToken.cbBuffer;
    pOutputToken->value = OutputToken.pvBuffer;

cleanup:
    *pMinorStatus = MinorStatus;
    return MajorStatus;
error:
    if (MajorStatus == GSS_S_COMPLETE)
    {
        MajorStatus = GSS_S_FAILURE;
    }
    goto cleanup;
}

OM_uint32
ntlm_gss_accept_sec_context(
    OM_uint32 *pMinorStatus,
    gss_ctx_id_t *pContextHandle,
    const gss_cred_id_t AcceptorCredHandle,
    const gss_buffer_t pInputTokenBuffer,
    const gss_channel_bindings_t pInputChanBindings,
    gss_name_t *pSrcName,
    gss_OID *pMechType,
    gss_buffer_t pOutputToken,
    OM_uint32 *pRetFlags,
    OM_uint32 *pTimeRec,
    gss_cred_id_t *pDelegatedCredHandle
    )
{
    OM_uint32 MajorStatus = GSS_S_COMPLETE;
    OM_uint32 MinorStatus = LW_ERROR_SUCCESS;
    SecBufferDesc InputBuffer = {0};
    SecBufferDesc OutputBuffer = {0};
    SecBuffer InputToken = {0};
    SecBuffer OutputToken = {0};
    TimeStamp tsExpiry = 0;
    NTLM_CONTEXT_HANDLE NewCtxtHandle = NULL;

    *pMinorStatus = LW_ERROR_SUCCESS;
    *pSrcName = NULL;
    *pMechType = NULL;
    *pTimeRec = 0;
    *pDelegatedCredHandle = NULL;
    memset(pOutputToken, 0, sizeof(*pOutputToken));

    InputBuffer.cBuffers = 1;
    InputBuffer.pBuffers = &InputToken;

    OutputBuffer.cBuffers = 1;
    OutputBuffer.pBuffers = &OutputToken;

    InputToken.BufferType = SECBUFFER_TOKEN;
    InputToken.cbBuffer = pInputTokenBuffer->length;
    InputToken.pvBuffer = pInputTokenBuffer->value;

    MinorStatus = NtlmClientAcceptSecurityContext(
        (PNTLM_CRED_HANDLE)(&AcceptorCredHandle),
        (PNTLM_CONTEXT_HANDLE)pContextHandle,
        &InputBuffer,
        0,
        NTLM_NATIVE_DATA_REP,
        &NewCtxtHandle,
        &OutputBuffer,
        pTimeRec,
        &tsExpiry);
    BAIL_ON_LW_ERROR(MinorStatus);

    pOutputToken->length = OutputToken.cbBuffer;
    pOutputToken->value = OutputToken.pvBuffer;

    *pContextHandle = (gss_ctx_id_t)NewCtxtHandle;
cleanup:
    *pMinorStatus = MinorStatus;
    return MajorStatus;
error:
    if (MajorStatus == GSS_S_COMPLETE)
    {
        MajorStatus = GSS_S_FAILURE;
    }
    goto cleanup;
}

OM_uint32
ntlm_gss_inquire_context2(
    OM_uint32 *pMinorStatus,
    const gss_ctx_id_t context_handle,
    gss_name_t *src_name,
    gss_name_t *targ_name,
    OM_uint32 *lifetime_rec,
    gss_OID *mech_type,
    OM_uint32 *ctx_flags,
    int *locally_initiated,
    int *open,
    gss_buffer_t session_key
    )
{
    OM_uint32 MajorStatus = GSS_S_COMPLETE;
    OM_uint32 MinorStatus = LW_ERROR_SUCCESS;
    SecPkgContext_SessionKey SessionKey = {0};

    if (src_name)
    {
        *src_name = NULL;
    }
    if (targ_name)
    {
        *targ_name = NULL;
    }
    if (lifetime_rec)
    {
        *lifetime_rec = GSS_C_INDEFINITE;
    }
    if (mech_type)
    {
        *mech_type = GSS_C_NO_OID;
    }
    if (ctx_flags)
    {
        *ctx_flags = 0;
    }
    if (locally_initiated)
    {
        *locally_initiated = 0;
    }
    if (open)
    {
        *open = 1;
    }

    if (session_key)
    {
        memset(session_key, 0, sizeof(gss_buffer_desc));

        MinorStatus = NtlmClientQueryContextAttributes(
            (PNTLM_CONTEXT_HANDLE)context_handle,
            SECPKG_ATTR_SESSION_KEY,
            &SessionKey
            );
        BAIL_ON_LW_ERROR(MinorStatus);

        MinorStatus = LwAllocateMemory(
            SessionKey.SessionKeyLength,
            &session_key->value);
        BAIL_ON_LW_ERROR(MinorStatus);

        session_key->length = SessionKey.SessionKeyLength;
        memcpy(session_key->value, SessionKey.SessionKey, session_key->length);
    }

cleanup:
    *pMinorStatus = MinorStatus;
    return MajorStatus;
error:
    if (MajorStatus == GSS_S_COMPLETE)
    {
        MajorStatus = GSS_S_FAILURE;
    }
    goto cleanup;
}

OM_uint32
ntlm_gss_delete_sec_context(
    OM_uint32 *pMinorStatus,
    gss_ctx_id_t *pContextHandle,
    gss_buffer_t OutputToken
    )
{
    OM_uint32 MajorStatus = GSS_S_COMPLETE;
    OM_uint32 MinorStatus = LW_ERROR_SUCCESS;
    NTLM_CONTEXT_HANDLE ContextHandle = NULL;

    if (OutputToken)
    {
        OutputToken = GSS_C_NO_BUFFER;
    }

    if (!pContextHandle || !*pContextHandle)
    {
        MajorStatus = GSS_S_NO_CONTEXT;
        MinorStatus = LW_ERROR_NO_CONTEXT;
        BAIL_ON_LW_ERROR(MinorStatus);
    }

    ContextHandle = (NTLM_CONTEXT_HANDLE)*pContextHandle;

    MinorStatus = NtlmClientDeleteSecurityContext(
        &ContextHandle
        );

    BAIL_ON_LW_ERROR(MinorStatus);

cleanup:
    *pMinorStatus = MinorStatus;
    return MajorStatus;
error:
    if (MajorStatus == GSS_S_COMPLETE)
    {
        MajorStatus = GSS_S_FAILURE;
    }
    goto cleanup;
}

OM_uint32
ntlm_gss_process_context_token(
    OM_uint32 *pMinorStatus,
    const gss_ctx_id_t context_handle,
    const gss_buffer_t token_buffer
    )
{
    OM_uint32 MajorStatus = GSS_S_FAILURE;
    OM_uint32 MinorStatus = LW_ERROR_NOT_IMPLEMENTED;

    BAIL_ON_LW_ERROR(MinorStatus);

cleanup:
    *pMinorStatus = MinorStatus;
    return MajorStatus;
error:
    if (MajorStatus == GSS_S_COMPLETE)
    {
        MajorStatus = GSS_S_FAILURE;
    }
    goto cleanup;
}

OM_uint32
ntlm_gss_context_time(
    OM_uint32 *pMinorStatus,
    const gss_ctx_id_t context_handle,
    OM_uint32 *time_rec
    )
{
    OM_uint32 MajorStatus = GSS_S_FAILURE;
    OM_uint32 MinorStatus = LW_ERROR_NOT_IMPLEMENTED;

    BAIL_ON_LW_ERROR(MinorStatus);

cleanup:
    *pMinorStatus = MinorStatus;
    return MajorStatus;
error:
    if (MajorStatus == GSS_S_COMPLETE)
    {
        MajorStatus = GSS_S_FAILURE;
    }
    goto cleanup;
}

OM_uint32
ntlm_gss_get_mic(
    OM_uint32 *pMinorStatus,
    const gss_ctx_id_t context_handle,
    gss_qop_t qop_req,
    const gss_buffer_t message_buffer,
    gss_buffer_t message_token
    )
{
    OM_uint32 MajorStatus = GSS_S_FAILURE;
    OM_uint32 MinorStatus = LW_ERROR_NOT_IMPLEMENTED;

    BAIL_ON_LW_ERROR(MinorStatus);

cleanup:
    *pMinorStatus = MinorStatus;
    return MajorStatus;
error:
    if (MajorStatus == GSS_S_COMPLETE)
    {
        MajorStatus = GSS_S_FAILURE;
    }
    goto cleanup;
}

OM_uint32
ntlm_gss_verify_mic(
    OM_uint32 *pMinorStatus,
    const gss_ctx_id_t context_handle,
    const gss_buffer_t message_buffer,
    const gss_buffer_t token_buffer,
    gss_qop_t *qop_state
    )
{
    OM_uint32 MajorStatus = GSS_S_FAILURE;
    OM_uint32 MinorStatus = LW_ERROR_NOT_IMPLEMENTED;

    BAIL_ON_LW_ERROR(MinorStatus);

cleanup:
    *pMinorStatus = MinorStatus;
    return MajorStatus;
error:
    if (MajorStatus == GSS_S_COMPLETE)
    {
        MajorStatus = GSS_S_FAILURE;
    }
    goto cleanup;
}

OM_uint32
ntlm_gss_wrap(
    OM_uint32 *pMinorStatus,
    const gss_ctx_id_t context_handle,
    int conf_req_flag,
    gss_qop_t qop_req,
    const gss_buffer_t input_message_buffer,
    int *conf_state,
    gss_buffer_t output_message_buffer
    )
{
    OM_uint32 MajorStatus = GSS_S_FAILURE;
    OM_uint32 MinorStatus = LW_ERROR_NOT_IMPLEMENTED;

    BAIL_ON_LW_ERROR(MinorStatus);

cleanup:
    *pMinorStatus = MinorStatus;
    return MajorStatus;
error:
    if (MajorStatus == GSS_S_COMPLETE)
    {
        MajorStatus = GSS_S_FAILURE;
    }
    goto cleanup;
}

OM_uint32
ntlm_gss_unwrap(
    OM_uint32 *pMinorStatus,
    const gss_ctx_id_t context_handle,
    const gss_buffer_t input_message_buffer,
    gss_buffer_t output_message_buffer,
    int *conf_state,
    gss_qop_t *qop_state
    )
{
    OM_uint32 MajorStatus = GSS_S_FAILURE;
    OM_uint32 MinorStatus = LW_ERROR_NOT_IMPLEMENTED;

    BAIL_ON_LW_ERROR(MinorStatus);

cleanup:
    *pMinorStatus = MinorStatus;
    return MajorStatus;
error:
    if (MajorStatus == GSS_S_COMPLETE)
    {
        MajorStatus = GSS_S_FAILURE;
    }
    goto cleanup;
}

OM_uint32
ntlm_gss_display_status(
    OM_uint32 *pMinorStatus,
    OM_uint32 status_value,
    int status_type,
    const gss_OID mech_type,
    OM_uint32 *message_context,
    gss_buffer_t status_string
    )
{
    OM_uint32 MajorStatus = GSS_S_FAILURE;
    OM_uint32 MinorStatus = LW_ERROR_NOT_IMPLEMENTED;

    BAIL_ON_LW_ERROR(MinorStatus);

cleanup:
    *pMinorStatus = MinorStatus;
    return MajorStatus;
error:
    if (MajorStatus == GSS_S_COMPLETE)
    {
        MajorStatus = GSS_S_FAILURE;
    }
    goto cleanup;
}

OM_uint32
ntlm_gss_indicate_mechs(
    OM_uint32 *pMinorStatus,
    gss_OID_set *mech_set
    )
{
    OM_uint32 MajorStatus = GSS_S_FAILURE;
    OM_uint32 MinorStatus = LW_ERROR_NOT_IMPLEMENTED;

    BAIL_ON_LW_ERROR(MinorStatus);

cleanup:
    *pMinorStatus = MinorStatus;
    return MajorStatus;
error:
    if (MajorStatus == GSS_S_COMPLETE)
    {
        MajorStatus = GSS_S_FAILURE;
    }
    goto cleanup;
}

OM_uint32
ntlm_gss_compare_name(
    OM_uint32 *pMinorStatus,
    const gss_name_t name1,
    const gss_name_t name2,
    int *name_equal
    )
{
    OM_uint32 MajorStatus = GSS_S_FAILURE;
    OM_uint32 MinorStatus = LW_ERROR_NOT_IMPLEMENTED;

    BAIL_ON_LW_ERROR(MinorStatus);

cleanup:
    *pMinorStatus = MinorStatus;
    return MajorStatus;
error:
    if (MajorStatus == GSS_S_COMPLETE)
    {
        MajorStatus = GSS_S_FAILURE;
    }
    goto cleanup;
}

OM_uint32
ntlm_gss_display_name(
    OM_uint32 *pMinorStatus,
    const gss_name_t input_name,
    gss_buffer_t output_name_buffer,
    gss_OID *output_name_type
    )
{
    OM_uint32 MajorStatus = GSS_S_FAILURE;
    OM_uint32 MinorStatus = LW_ERROR_NOT_IMPLEMENTED;

    BAIL_ON_LW_ERROR(MinorStatus);

cleanup:
    *pMinorStatus = MinorStatus;
    return MajorStatus;
error:
    if (MajorStatus == GSS_S_COMPLETE)
    {
        MajorStatus = GSS_S_FAILURE;
    }
    goto cleanup;
}

OM_uint32
ntlm_gss_import_name(
    OM_uint32 *pMinorStatus,
    const gss_buffer_t input_name_buffer,
    const gss_OID input_name_type,
    gss_name_t *output_name
    )
{
    OM_uint32 MajorStatus = GSS_S_COMPLETE;
    OM_uint32 MinorStatus = LW_ERROR_SUCCESS;

    // formats we will have to potentially handle
    // GSS_C_NT_USER_NAME - principal@realm
    // GSS_C_NT_HOSTBASED_SERVICE - service@host
    // gss_nt_krb5_name - principal@realm
    // gss_nt_krb5_principal
    // GSS_C_NO_OID
    //
    // We need one like gss_nt_ntlm_name - NETBios\user@realm

    BAIL_ON_LW_ERROR(MinorStatus);

cleanup:
    *pMinorStatus = MinorStatus;
    return MajorStatus;
error:
    if (MajorStatus == GSS_S_COMPLETE)
    {
        MajorStatus = GSS_S_FAILURE;
    }
    goto cleanup;
}

OM_uint32
ntlm_gss_export_name(
    OM_uint32 *pMinorStatus,
    const gss_name_t input_name,
    gss_buffer_t exported_name
    )
{
    OM_uint32 MajorStatus = GSS_S_FAILURE;
    OM_uint32 MinorStatus = LW_ERROR_NOT_IMPLEMENTED;

    BAIL_ON_LW_ERROR(MinorStatus);

cleanup:
    *pMinorStatus = MinorStatus;
    return MajorStatus;
error:
    if (MajorStatus == GSS_S_COMPLETE)
    {
        MajorStatus = GSS_S_FAILURE;
    }
    goto cleanup;
}

OM_uint32
ntlm_gss_release_name(
    OM_uint32 *pMinorStatus,
    gss_name_t *name
    )
{
    OM_uint32 MajorStatus = GSS_S_FAILURE;
    OM_uint32 MinorStatus = LW_ERROR_NOT_IMPLEMENTED;

    BAIL_ON_LW_ERROR(MinorStatus);

cleanup:
    *pMinorStatus = MinorStatus;
    return MajorStatus;
error:
    if (MajorStatus == GSS_S_COMPLETE)
    {
        MajorStatus = GSS_S_FAILURE;
    }
    goto cleanup;
}

OM_uint32
ntlm_gss_release_buffer(
    OM_uint32 *pMinorStatus,
    gss_buffer_t pBuffer
    )
{
    // According to RFC 2744, this function should not fail
    if (pBuffer)
    {
        LW_SAFE_FREE_MEMORY(pBuffer->value);
        pBuffer->value = NULL;
        pBuffer->length = 0;
    }

    *pMinorStatus = LW_ERROR_SUCCESS;
    return GSS_S_COMPLETE;
}

OM_uint32
ntlm_gss_release_oid_set(
    OM_uint32 *pMinorStatus,
    gss_OID_set *set
    )
{
    OM_uint32 MajorStatus = GSS_S_FAILURE;
    OM_uint32 MinorStatus = LW_ERROR_NOT_IMPLEMENTED;

    BAIL_ON_LW_ERROR(MinorStatus);

cleanup:
    *pMinorStatus = MinorStatus;
    return MajorStatus;
error:
    if (MajorStatus == GSS_S_COMPLETE)
    {
        MajorStatus = GSS_S_FAILURE;
    }
    goto cleanup;
}

OM_uint32
ntlm_gss_inquire_cred(
    OM_uint32 *pMinorStatus,
    const gss_cred_id_t CredHandle,
    gss_name_t *name,
    OM_uint32 *lifetime,
    gss_cred_usage_t *cred_usage,
    gss_OID_set *mechanisms
    )
{
    OM_uint32 MajorStatus = GSS_S_FAILURE;
    OM_uint32 MinorStatus = LW_ERROR_NOT_IMPLEMENTED;

    BAIL_ON_LW_ERROR(MinorStatus);

cleanup:
    *pMinorStatus = MinorStatus;
    return MajorStatus;
error:
    if (MajorStatus == GSS_S_COMPLETE)
    {
        MajorStatus = GSS_S_FAILURE;
    }
    goto cleanup;
}

OM_uint32
ntlm_gss_inquire_context(
    OM_uint32 *pMinorStatus,
    const gss_ctx_id_t context_handle,
    gss_name_t *src_name,
    gss_name_t *targ_name,
    OM_uint32 *lifetime_rec,
    gss_OID *mech_type,
    OM_uint32 *ctx_flags,
    int *locally_initiated,
    int *open
    )
{
    OM_uint32 MajorStatus = GSS_S_FAILURE;
    OM_uint32 MinorStatus = LW_ERROR_NOT_IMPLEMENTED;

    BAIL_ON_LW_ERROR(MinorStatus);

cleanup:
    *pMinorStatus = MinorStatus;
    return MajorStatus;
error:
    if (MajorStatus == GSS_S_COMPLETE)
    {
        MajorStatus = GSS_S_FAILURE;
    }
    goto cleanup;
}

OM_uint32
ntlm_gss_wrap_size_limit(
    OM_uint32 *pMinorStatus,
    const gss_ctx_id_t context_handle,
    int conf_req_flag,
    gss_qop_t qop_req,
    OM_uint32 req_output_size,
    OM_uint32 *max_input_size
    )
{
    OM_uint32 MajorStatus = GSS_S_FAILURE;
    OM_uint32 MinorStatus = LW_ERROR_NOT_IMPLEMENTED;

    BAIL_ON_LW_ERROR(MinorStatus);

cleanup:
    *pMinorStatus = MinorStatus;
    return MajorStatus;
error:
    if (MajorStatus == GSS_S_COMPLETE)
    {
        MajorStatus = GSS_S_FAILURE;
    }
    goto cleanup;
}

OM_uint32
ntlm_gss_add_cred(
    OM_uint32 *pMinorStatus,
    const gss_cred_id_t InputCredHandle,
    const gss_name_t desired_name,
    const gss_OID desired_mech,
    gss_cred_usage_t cred_usage,
    OM_uint32 initiator_time_req,
    OM_uint32 acceptor_time_req,
    gss_cred_id_t *pOutputCredHandle,
    gss_OID_set *actual_mechs,
    OM_uint32 *initiator_time_rec,
    OM_uint32 *acceptor_time_rec
    )
{
    OM_uint32 MajorStatus = GSS_S_FAILURE;
    OM_uint32 MinorStatus = LW_ERROR_NOT_IMPLEMENTED;

    BAIL_ON_LW_ERROR(MinorStatus);

cleanup:
    *pMinorStatus = MinorStatus;
    return MajorStatus;
error:
    if (MajorStatus == GSS_S_COMPLETE)
    {
        MajorStatus = GSS_S_FAILURE;
    }
    goto cleanup;
}

OM_uint32
ntlm_gss_inquire_cred_by_mech(
    OM_uint32 *pMinorStatus,
    const gss_cred_id_t CredHandle,
    const gss_OID mech_type,
    gss_name_t *name,
    OM_uint32 *initiator_lifetime,
    OM_uint32 *acceptor_lifetime,
    gss_cred_usage_t *cred_usage
    )
{
    OM_uint32 MajorStatus = GSS_S_FAILURE;
    OM_uint32 MinorStatus = LW_ERROR_NOT_IMPLEMENTED;

    BAIL_ON_LW_ERROR(MinorStatus);

cleanup:
    *pMinorStatus = MinorStatus;
    return MajorStatus;
error:
    if (MajorStatus == GSS_S_COMPLETE)
    {
        MajorStatus = GSS_S_FAILURE;
    }
    goto cleanup;
}

OM_uint32
ntlm_gss_export_sec_context(
    OM_uint32 *pMinorStatus,
    gss_ctx_id_t *pContextHandle,
    gss_buffer_t interprocess_token
    )
{
    OM_uint32 MajorStatus = GSS_S_FAILURE;
    OM_uint32 MinorStatus = LW_ERROR_NOT_IMPLEMENTED;

    BAIL_ON_LW_ERROR(MinorStatus);

cleanup:
    *pMinorStatus = MinorStatus;
    return MajorStatus;
error:
    if (MajorStatus == GSS_S_COMPLETE)
    {
        MajorStatus = GSS_S_FAILURE;
    }
    goto cleanup;
}

OM_uint32
ntlm_gss_import_sec_context(
    OM_uint32 *pMinorStatus,
    const gss_buffer_t interprocess_token,
    gss_ctx_id_t *pContextHandle
    )
{
    OM_uint32 MajorStatus = GSS_S_FAILURE;
    OM_uint32 MinorStatus = LW_ERROR_NOT_IMPLEMENTED;

    BAIL_ON_LW_ERROR(MinorStatus);

cleanup:
    *pMinorStatus = MinorStatus;
    return MajorStatus;
error:
    if (MajorStatus == GSS_S_COMPLETE)
    {
        MajorStatus = GSS_S_FAILURE;
    }
    goto cleanup;
}

OM_uint32
ntlm_gss_create_empty_oid_set(
    OM_uint32 *pMinorStatus,
    gss_OID_set *oid_set
    )
{
    OM_uint32 MajorStatus = GSS_S_FAILURE;
    OM_uint32 MinorStatus = LW_ERROR_NOT_IMPLEMENTED;

    BAIL_ON_LW_ERROR(MinorStatus);

cleanup:
    *pMinorStatus = MinorStatus;
    return MajorStatus;
error:
    if (MajorStatus == GSS_S_COMPLETE)
    {
        MajorStatus = GSS_S_FAILURE;
    }
    goto cleanup;
}

OM_uint32
ntlm_gss_add_oid_set_member(
    OM_uint32 *pMinorStatus,
    const gss_OID member_oid,
    gss_OID_set *oid_set
    )
{
    OM_uint32 MajorStatus = GSS_S_FAILURE;
    OM_uint32 MinorStatus = LW_ERROR_NOT_IMPLEMENTED;

    BAIL_ON_LW_ERROR(MinorStatus);

cleanup:
    *pMinorStatus = MinorStatus;
    return MajorStatus;
error:
    if (MajorStatus == GSS_S_COMPLETE)
    {
        MajorStatus = GSS_S_FAILURE;
    }
    goto cleanup;
}

OM_uint32
ntlm_gss_test_oid_set_member(
    OM_uint32 *pMinorStatus,
    const gss_OID member,
    const gss_OID_set set,
    int *present
    )
{
    OM_uint32 MajorStatus = GSS_S_FAILURE;
    OM_uint32 MinorStatus = LW_ERROR_NOT_IMPLEMENTED;

    BAIL_ON_LW_ERROR(MinorStatus);

cleanup:
    *pMinorStatus = MinorStatus;
    return MajorStatus;
error:
    if (MajorStatus == GSS_S_COMPLETE)
    {
        MajorStatus = GSS_S_FAILURE;
    }
    goto cleanup;
}

OM_uint32
ntlm_gss_inquire_names_for_mech(
    OM_uint32 *pMinorStatus,
    const gss_OID mechanism,
    gss_OID_set *name_types
    )
{
    OM_uint32 MajorStatus = GSS_S_FAILURE;
    OM_uint32 MinorStatus = LW_ERROR_NOT_IMPLEMENTED;

    BAIL_ON_LW_ERROR(MinorStatus);

cleanup:
    *pMinorStatus = MinorStatus;
    return MajorStatus;
error:
    if (MajorStatus == GSS_S_COMPLETE)
    {
        MajorStatus = GSS_S_FAILURE;
    }
    goto cleanup;
}

OM_uint32
ntlm_gss_inquire_mechs_for_name(
    OM_uint32 *pMinorStatus,
    const gss_name_t input_name,
    gss_OID_set *mech_types
    )
{
    OM_uint32 MajorStatus = GSS_S_FAILURE;
    OM_uint32 MinorStatus = LW_ERROR_NOT_IMPLEMENTED;

    BAIL_ON_LW_ERROR(MinorStatus);

cleanup:
    *pMinorStatus = MinorStatus;
    return MajorStatus;
error:
    if (MajorStatus == GSS_S_COMPLETE)
    {
        MajorStatus = GSS_S_FAILURE;
    }
    goto cleanup;
}

OM_uint32
ntlm_gss_canonicalize_name(
    OM_uint32 *pMinorStatus,
    const gss_name_t input_name,
    const gss_OID mech_type,
    gss_name_t *output_name
    )
{
    OM_uint32 MajorStatus = GSS_S_FAILURE;
    OM_uint32 MinorStatus = LW_ERROR_NOT_IMPLEMENTED;

    BAIL_ON_LW_ERROR(MinorStatus);

cleanup:
    *pMinorStatus = MinorStatus;
    return MajorStatus;
error:
    if (MajorStatus == GSS_S_COMPLETE)
    {
        MajorStatus = GSS_S_FAILURE;
    }
    goto cleanup;
}

OM_uint32
ntlm_gss_duplicate_name(
    OM_uint32 *pMinorStatus,
    const gss_name_t src_name,
    gss_name_t *dest_name
    )
{
    OM_uint32 MajorStatus = GSS_S_FAILURE;
    OM_uint32 MinorStatus = LW_ERROR_NOT_IMPLEMENTED;

    BAIL_ON_LW_ERROR(MinorStatus);

cleanup:
    *pMinorStatus = MinorStatus;
    return MajorStatus;
error:
    if (MajorStatus == GSS_S_COMPLETE)
    {
        MajorStatus = GSS_S_FAILURE;
    }
    goto cleanup;
}

OM_uint32
ntlm_gss_sign(
    OM_uint32 *pMinorStatus,
    gss_ctx_id_t context_handle,
    int qop_req,
    gss_buffer_t message_buffer,
    gss_buffer_t message_token
    )
{
    OM_uint32 MajorStatus = GSS_S_FAILURE;
    OM_uint32 MinorStatus = LW_ERROR_NOT_IMPLEMENTED;

    BAIL_ON_LW_ERROR(MinorStatus);

cleanup:
    *pMinorStatus = MinorStatus;
    return MajorStatus;
error:
    if (MajorStatus == GSS_S_COMPLETE)
    {
        MajorStatus = GSS_S_FAILURE;
    }
    goto cleanup;
}

OM_uint32
ntlm_gss_verify(
    OM_uint32 *pMinorStatus,
    gss_ctx_id_t context_handle,
    gss_buffer_t message_buffer,
    gss_buffer_t token_buffer,
    int *qop_state
    )
{
    OM_uint32 MajorStatus = GSS_S_FAILURE;
    OM_uint32 MinorStatus = LW_ERROR_NOT_IMPLEMENTED;

    BAIL_ON_LW_ERROR(MinorStatus);

cleanup:
    *pMinorStatus = MinorStatus;
    return MajorStatus;
error:
    if (MajorStatus == GSS_S_COMPLETE)
    {
        MajorStatus = GSS_S_FAILURE;
    }
    goto cleanup;
}

OM_uint32
ntlm_gss_seal(
    OM_uint32 *pMinorStatus,
    gss_ctx_id_t context_handle,
    int conf_req_flag,
    int qop_req,
    gss_buffer_t input_message_buffer,
    int *conf_state,
    gss_buffer_t output_message_buffer
    )
{
    OM_uint32 MajorStatus = GSS_S_FAILURE;
    OM_uint32 MinorStatus = LW_ERROR_NOT_IMPLEMENTED;

    BAIL_ON_LW_ERROR(MinorStatus);

cleanup:
    *pMinorStatus = MinorStatus;
    return MajorStatus;
error:
    if (MajorStatus == GSS_S_COMPLETE)
    {
        MajorStatus = GSS_S_FAILURE;
    }
    goto cleanup;
}

OM_uint32
ntlm_gss_unseal(
    OM_uint32 *pMinorStatus,
    gss_ctx_id_t context_handle,
    gss_buffer_t input_message_buffer,
    gss_buffer_t output_message_buffer,
    int *conf_state,
    int *qop_state
    )
{
    OM_uint32 MajorStatus = GSS_S_FAILURE;
    OM_uint32 MinorStatus = LW_ERROR_NOT_IMPLEMENTED;

    BAIL_ON_LW_ERROR(MinorStatus);

cleanup:
    *pMinorStatus = MinorStatus;
    return MajorStatus;
error:
    if (MajorStatus == GSS_S_COMPLETE)
    {
        MajorStatus = GSS_S_FAILURE;
    }
    goto cleanup;
}

PGSS_MECH_CONFIG
gss_mech_initialize(void)
{
    return &gNtlmMech;
}

