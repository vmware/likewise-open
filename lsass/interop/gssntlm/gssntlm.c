/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (c) Likewise Software.  All rights Reserved.
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
 * license@likewise.com
 */

/*
 * Copyright (C) Likewise Software.  All rights reserved.
 *
 * Module Name:
 *
 *        gssntlm.c
 *
 * Abstract:
 *
 *        GSS wrapper functions for NTLM implementation.
 *
 * Authors: Marc Guy (mguy@likewisesoftware.com)
 *
 */

#include <ntlm/sspintlm.h>
#include <ntlm/gssntlm.h>
#include <lwerror.h>
#include <lwmem.h>
#include <string.h>

#undef BAIL_ON_LW_ERROR
#define BAIL_ON_LW_ERROR(dwError) \
    do { \
        if (dwError) \
        { \
            goto error; \
        } \
    } while (0)

//
// Since there is no GSSAPI mech plugin header, this must be kept in sync with
// gss_mechanism in krb5/src/lib/gssapi/mglueP.h.
//

typedef struct _GSS_MECH_CONFIG
{
    gss_OID_desc MechType;
    PVOID pContext;

    OM_uint32
    (*gss_acquire_cred)(
        OM_uint32*,
        gss_name_t,
        OM_uint32,
        gss_OID_set,
        INT,
        gss_cred_id_t*,
        gss_OID_set*,
        OM_uint32*
        );

    OM_uint32
    (*gss_release_cred)(
        OM_uint32*,
        gss_cred_id_t*
        );

    OM_uint32
    (*gss_init_sec_cred)(
        OM_uint32*,
        gss_cred_id_t,
        gss_ctx_id_t*,
        gss_name_t,
        gss_OID,
        OM_uint32,
        OM_uint32,
        gss_channel_bindings_t,
        gss_buffer_t,
        gss_OID*,
        gss_buffer_t,
        OM_uint32*,
        OM_uint32*
        );

    OM_uint32
    (*gss_accept_sec_context)(
        OM_uint32*,
        gss_ctx_id_t*,
        gss_cred_id_t,
        gss_buffer_t,
        gss_channel_bindings_t,
        gss_name_t*,
        gss_OID*,
        gss_buffer_t,
        OM_uint32*,
        OM_uint32*,
        gss_cred_id_t*
        );

    OM_uint32
    (*gss_process_context_token)(
        OM_uint32*,
        gss_ctx_id_t,
        gss_buffer_t
        );

    OM_uint32
    (*gss_delete_sec_context)(
        OM_uint32*,
        gss_ctx_id_t*,
        gss_buffer_t
        );

    OM_uint32
    (*gss_context_time)(
        OM_uint32*,
        gss_ctx_id_t,
        OM_uint32*
        );

    OM_uint32
    (*gss_get_mic)(
        OM_uint32*,
        gss_ctx_id_t,
        gss_qop_t,
        gss_buffer_t,
        gss_buffer_t
        );

    OM_uint32
    (*gss_verify_mic)(
        OM_uint32*,
        gss_ctx_id_t,
        gss_buffer_t,
        gss_buffer_t,
        gss_qop_t*
        );

    OM_uint32
    (*gss_wrap)(
        OM_uint32*,
        gss_ctx_id_t,
        INT,
        gss_qop_t,
        gss_buffer_t,
        PINT,
        gss_buffer_t
        );

    OM_uint32
    (*gss_unwrap)(
        OM_uint32*,
        gss_ctx_id_t,
        gss_buffer_t,
        gss_buffer_t,
        PINT,
        gss_qop_t*
        );

    OM_uint32
    (*gss_display_status)(
        OM_uint32*,
        OM_uint32,
        INT,
        gss_OID,
        OM_uint32*,
        gss_buffer_t
        );

    OM_uint32
    (*gss_indicate_mechs)(
        OM_uint32*,
        gss_OID_set*
        );

    OM_uint32
    (*gss_compare_name)(
        OM_uint32*,
        gss_name_t,
        gss_name_t,
        PINT);

    OM_uint32
    (*gss_display_name)(
        OM_uint32*,
        gss_name_t,
        gss_buffer_t,
        gss_OID*
        );

    OM_uint32
    (*gss_import_name)(
        OM_uint32*,
        gss_buffer_t,
        gss_OID,
        gss_name_t*
        );

    OM_uint32
    (*gss_release_name)(
        OM_uint32*,
        gss_name_t*);

    OM_uint32
    (*gss_inquire_cred)(
        OM_uint32*,
        gss_cred_id_t,
        gss_name_t*,
        OM_uint32*,
        PINT,
        gss_OID_set*
        );

    OM_uint32
    (*gss_add_cred)(
        OM_uint32*,
        gss_cred_id_t,
        gss_name_t,
        gss_OID,
        gss_cred_usage_t,
        OM_uint32,
        OM_uint32,
        gss_cred_id_t*,
        gss_OID_set*,
        OM_uint32*,
        OM_uint32*
        );

    OM_uint32
    (*gss_export_sec_context)(
        OM_uint32,
        gss_ctx_id_t*,
        gss_buffer_t
        );

    OM_uint32
    (*gss_import_sec_context)(
        OM_uint32,
        gss_buffer_t,
        gss_ctx_id_t*
        );

    OM_uint32
    (*gss_inquire_cred_by_mech)(
        OM_uint32*,
        gss_cred_id_t,
        gss_OID,
        gss_name_t*,
        OM_uint32*,
        OM_uint32*,
        gss_cred_usage_t*
        );

    OM_uint32
    (*gss_inquire_names_for_mech)(
        OM_uint32,
        gss_OID,
        gss_OID_set*
        );

    OM_uint32
    (*gss_inquire_context)(
        OM_uint32*,
        gss_ctx_id_t,
        gss_name_t*,
        gss_name_t*,
        OM_uint32*,
        gss_OID*,
        OM_uint32*,
        PINT,
        PINT
        );

    OM_uint32
    (*gss_internal_release_oid)(
        OM_uint32*,
        gss_OID*
        );

    OM_uint32
    (*gss_wrap_size_limit)(
        OM_uint32*,
        gss_ctx_id_t,
        INT,
        gss_qop_t,
        OM_uint32,
        OM_uint32*
        );

    OM_uint32
    (*gss_export_name)(
        OM_uint32*,
        const gss_name_t,
        gss_buffer_t
        );

    OM_uint32
    (*gss_store_cred)(
        OM_uint32*,
        const gss_cred_id_t,
        gss_cred_usage_t,
        const gss_OID,
        OM_uint32,
        OM_uint32,
        gss_OID_set*,
        gss_cred_usage_t*
        );

    OM_uint32
    (*gss_inquire_sec_context_by_oid)(
        OM_uint32*,
        const gss_ctx_id_t,
        const gss_OID,
        gss_buffer_set_t*
        );

    OM_uint32
    (*gss_inquire_cred_by_oid)(
        OM_uint32*,
        const gss_cred_id_t,
        const gss_OID,
        gss_buffer_set_t*
        );

    OM_uint32
    (*gss_set_sec_context_option)(
        OM_uint32*,
        gss_ctx_id_t*,
        const gss_OID,
        const gss_buffer_t
        );

    OM_uint32
    (*gssspi_set_cred_option)(
        OM_uint32*,
        gss_cred_id_t,
        const gss_OID,
        const gss_buffer_t
        );

    OM_uint32
    (*gssspi_mech_invoke)(
        OM_uint32*,
        const gss_OID,
        const gss_OID,
        gss_buffer_t
        );

    OM_uint32
    (*gss_wrap_aead)(
        OM_uint32*,
        gss_ctx_id_t,
        INT,
        gss_qop_t,
        gss_buffer_t,
        gss_buffer_t,
        PINT,
        gss_buffer_t
        );

    OM_uint32
    (*gss_unwrap_aead)(
        OM_uint32*,
        gss_ctx_id_t,
        gss_buffer_t,
        gss_buffer_t,
        gss_buffer_t,
        PINT,
        gss_qop_t*);

    OM_uint32
    (*gss_wrap_iov)(
        OM_uint32*,
        gss_ctx_id_t,
        INT,
        gss_qop_t,
        PINT,
        gss_iov_buffer_desc,
        INT
        );

    OM_uint32
    (*gss_unwrap_iov)(
        OM_uint32*,
        gss_ctx_id_t,
        PINT,
        gss_qop_t*,
        gss_iov_buffer_desc*,
        INT
        );

    OM_uint32
    (*gss_wrap_iov_length)(
        OM_uint32*,
        gss_ctx_id_t,
        INT,
        gss_qop_t,
        PINT,
        gss_iov_buffer_desc*,
        INT
        );

    OM_uint32
    (*gss_complete_auth_token)(
        OM_uint32*,
        const gss_ctx_id_t,
        gss_buffer_t
        );

    OM_uint32
    (*gss_inquire_context2)(
        OM_uint32*,
        gss_ctx_id_t,
        gss_name_t*,
        gss_name_t*,
        OM_uint32*,
        gss_OID*,
        OM_uint32*,
        PINT,
        PINT,
        gss_buffer_t
        );

} GSS_MECH_CONFIG, *PGSS_MECH_CONFIG;

//
// Prototypes
//

PGSS_MECH_CONFIG
gss_mech_initialize(
    void
    );

//
// Globals
//

static GSS_MECH_CONFIG gNtlmMech =
{
    {GSS_MECH_NTLM_LEN, GSS_MECH_NTLM},
    NULL,

    ntlm_gss_acquire_cred,
    ntlm_gss_release_cred,
    ntlm_gss_init_sec_context,
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
    NULL, //ntlm_gss_release_name,
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
    ntlm_gss_inquire_sec_context_by_oid,
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

//
// Function Definitions
//

OM_uint32
ntlm_gss_acquire_cred(
    OM_uint32* pMinorStatus,
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
    OM_uint32* pMinorStatus,
    gss_cred_id_t* pCredHandle
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

    MinorStatus = NtlmClientFreeCredentialsHandle(
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
    OM_uint32* pMinorStatus,
    const gss_cred_id_t InitiatorCredHandle,
    gss_ctx_id_t* pContextHandle,
    const gss_name_t pTargetName,
    const gss_OID pMechType,
    OM_uint32 nReqFlags,
    OM_uint32 nTimeReq,
    const gss_channel_bindings_t pInputChanBindings,
    const gss_buffer_t pInputToken,
    gss_OID* pActualMechType,
    gss_buffer_t pOutputToken,
    OM_uint32* pRetFlags,
    OM_uint32* pTimeRec
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

    if(MinorStatus == LW_WARNING_CONTINUE_NEEDED)
    {
        MajorStatus = GSS_S_CONTINUE_NEEDED;
    }
    else
    {
        BAIL_ON_LW_ERROR(MinorStatus);
    }

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
    OM_uint32* pMinorStatus,
    gss_ctx_id_t *pContextHandle,
    const gss_cred_id_t AcceptorCredHandle,
    const gss_buffer_t pInputTokenBuffer,
    const gss_channel_bindings_t pInputChanBindings,
    gss_name_t* pSrcName,
    gss_OID* pMechType,
    gss_buffer_t pOutputToken,
    OM_uint32* pRetFlags,
    OM_uint32* pTimeRec,
    gss_cred_id_t* pDelegatedCredHandle
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

    if(pSrcName)
    {
        *pSrcName = NULL;
    }
    if(pMechType)
    {
        *pMechType = NULL;
    }
    if(pTimeRec)
    {
        *pTimeRec = 0;
    }
    if(pDelegatedCredHandle)
    {
        *pDelegatedCredHandle = NULL;
    }

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

    if(MinorStatus == LW_WARNING_CONTINUE_NEEDED)
    {
        MajorStatus = GSS_S_CONTINUE_NEEDED;
    }
    else
    {
        BAIL_ON_LW_ERROR(MinorStatus);
    }

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
    OM_uint32* pMinorStatus,
    const gss_ctx_id_t ContextHandle,
    gss_name_t* pSrcName,
    gss_name_t* pTargetName,
    OM_uint32* pLifeTime,
    gss_OID* pMechType,
    OM_uint32* pCtxtFlags,
    PINT pLocallyInitiated,
    PINT pOpen,
    gss_buffer_t SessionKeyBuffer
    )
{
    OM_uint32 MajorStatus = GSS_S_COMPLETE;
    OM_uint32 MinorStatus = LW_ERROR_SUCCESS;
    SecPkgContext_SessionKey SessionKey = {0};

    if (pSrcName)
    {
        *pSrcName = NULL;
    }
    if (pTargetName)
    {
        *pTargetName = NULL;
    }
    if (pLifeTime)
    {
        *pLifeTime = GSS_C_INDEFINITE;
    }
    if (pMechType)
    {
        *pMechType = GSS_C_NO_OID;
    }
    if (pCtxtFlags)
    {
        *pCtxtFlags = 0;
    }
    if (pLocallyInitiated)
    {
        *pLocallyInitiated = 0;
    }
    if (pOpen)
    {
        *pOpen = 1;
    }

    if (SessionKeyBuffer)
    {
        memset(SessionKeyBuffer, 0, sizeof(gss_buffer_desc));

        MinorStatus = NtlmClientQueryContextAttributes(
            (PNTLM_CONTEXT_HANDLE)&ContextHandle,
            SECPKG_ATTR_SESSION_KEY,
            &SessionKey
            );
        BAIL_ON_LW_ERROR(MinorStatus);

        MinorStatus = LwAllocateMemory(
            SessionKey.SessionKeyLength,
            &SessionKeyBuffer->value);
        BAIL_ON_LW_ERROR(MinorStatus);

        SessionKeyBuffer->length = SessionKey.SessionKeyLength;
        memcpy(
            SessionKeyBuffer->value,
            SessionKey.SessionKey,
            SessionKeyBuffer->length);
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
    OM_uint32* pMinorStatus,
    gss_ctx_id_t* pContextHandle,
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
ntlm_gss_import_name(
    OM_uint32* pMinorStatus,
    const gss_buffer_t InputNameBuffer,
    const gss_OID InputNameType,
    gss_name_t* pOutputName
    )
{
    OM_uint32 MajorStatus = GSS_S_COMPLETE;
    OM_uint32 MinorStatus = LW_ERROR_SUCCESS;

    *pOutputName = GSS_C_NO_NAME;

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
ntlm_gss_release_buffer(
    OM_uint32* pMinorStatus,
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

#if 0
OM_uint32
ntlm_gss_inquire_cred(
    OM_uint32* pMinorStatus,
    const gss_cred_id_t CredHandle,
    gss_name_t* pName,
    OM_uint32* pLifeTime,
    gss_cred_usage_t* pCredUsage,
    gss_OID_set* pMechs
    )
{
    OM_uint32 MajorStatus = GSS_S_COMPLETE;
    OM_uint32 MinorStatus = LW_ERROR_SUCCESS;
    SecPkgCred_Names CredNames;

    memset(&CredNames, 0, sizeof(CredNames));

    MinorStatus = NtlmClientQueryCredentialsAttributes(
        (PNTLM_CRED_HANDLE)&CredHandle,
        SECPKG_CRED_ATTR_NAMES,
        &CredNames);
    BAIL_ON_LW_ERROR(MinorStatus);

    if(pLifeTime)
    {
        *pLifeTime = 0;
    }

    if(pCredUsage)
    {
        *pCredUsage = 0;
    }

    if(pMechs)
    {
        *pMechs = GSS_C_NO_OID_SET;
    }

cleanup:
    *pMinorStatus = MinorStatus;

    if(pName)
    {
        *pName = (gss_name_t)CredNames.pUserName;
    }

    return MajorStatus;
error:
    if (MajorStatus == GSS_S_COMPLETE)
    {
        MajorStatus = GSS_S_FAILURE;
    }

    LW_SAFE_FREE_MEMORY(CredNames.pUserName);

    goto cleanup;
}
#else
OM_uint32
ntlm_gss_inquire_cred(
    OM_uint32* pMinorStatus,
    const gss_cred_id_t CredHandle,
    gss_name_t* pName,
    OM_uint32* pLifeTime,
    gss_cred_usage_t* pCredUsage,
    gss_OID_set* pMechs
    )
{
    if(pName)
    {
        *pName = GSS_C_NO_NAME;
    }
    if(pLifeTime)
    {
        *pLifeTime = 0;
    }
    if(pCredUsage)
    {
        *pCredUsage = 0;
    }
    if(pMechs)
    {
        *pMechs = GSS_C_NO_OID_SET;
    }

    return GSS_S_COMPLETE;
}
#endif

OM_uint32
ntlm_gss_inquire_sec_context_by_oid(
    OM_uint32* pMinorStatus,
    const gss_ctx_id_t GssCtxtHandle,
    const gss_OID Attrib,
    gss_buffer_set_t* ppGssSessionKey
    )
{
    OM_uint32 MajorStatus = GSS_S_COMPLETE;
    OM_uint32 MinorStatus = LW_ERROR_SUCCESS;
    gss_OID SessionKeyOid = GSS_C_INQ_SSPI_SESSION_KEY;
    NTLM_CONTEXT_HANDLE NtlmCtxtHandle = (NTLM_CONTEXT_HANDLE)GssCtxtHandle;
    SecPkgContext_SessionKey NtlmSessionKey;
    gss_buffer_set_t pGssSessionKey = NULL;
    gss_buffer_t pSessionKeyElement = NULL;

    if (Attrib->length != SessionKeyOid->length ||
        memcmp(Attrib->elements, SessionKeyOid->elements, Attrib->length))
    {
        MinorStatus = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LW_ERROR(MinorStatus);
    }

    MinorStatus = NtlmClientQueryContextAttributes(
        &NtlmCtxtHandle,
        SECPKG_ATTR_SESSION_KEY,
        &NtlmSessionKey);
    BAIL_ON_LW_ERROR(MinorStatus);

    MinorStatus = LwAllocateMemory(
        sizeof(gss_buffer_set_desc),
        OUT_PPVOID(&pGssSessionKey));
    BAIL_ON_LW_ERROR(MinorStatus);

    MinorStatus = LwAllocateMemory(
        sizeof(gss_buffer_desc),
        OUT_PPVOID(&pSessionKeyElement));
    BAIL_ON_LW_ERROR(MinorStatus);

    pSessionKeyElement->length = NtlmSessionKey.SessionKeyLength;
    pSessionKeyElement->value = NtlmSessionKey.SessionKey;

    pGssSessionKey->count = 1;
    pGssSessionKey->elements = pSessionKeyElement;

cleanup:
    *pMinorStatus = MinorStatus;
    *ppGssSessionKey = pGssSessionKey;

    return MajorStatus;
error:
    if (MajorStatus == GSS_S_COMPLETE)
    {
        MajorStatus = GSS_S_FAILURE;
    }

    LW_SAFE_FREE_MEMORY(pSessionKeyElement);
    LW_SAFE_FREE_MEMORY(pGssSessionKey);

    goto cleanup;
}

PGSS_MECH_CONFIG
gss_mech_initialize(void)
{
    return &gNtlmMech;
}
