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
 *        gssntlm.h
 *
 * Abstract:
 *
 *
 * Authors:
 *
 */

#ifndef __GSSNTLM_H__
#define __GSSNTLM_H__

#include <gssapi/gssapi.h>
#include <gssapi/gssapi_ext.h>

//******************************************************************************
//
// S T R U C T S
//

//******************************************************************************
//
// D E F I N E S
//

/* 1.3.6.1.4.1.311.2.2.10 */
#define GSS_MECH_NTLM       "\x2b\x06\x01\x04\x01\x82\x37\x02\x02\x0a"
#define GSS_MECH_NTLM_LEN   10

//******************************************************************************
//
// E X T E R N S
//

//******************************************************************************
//
// P R O T O T Y P E S
//

OM_uint32
ntlm_gss_acquire_cred(
    OM_uint32 *minor_status,
    const gss_name_t desired_name,
    OM_uint32 time_req,
    const gss_OID_set desired_mechs,
    gss_cred_usage_t cred_usage,
    gss_cred_id_t *output_cred_handle,
    gss_OID_set *actual_mechs,
    OM_uint32 *time_rec
    );

OM_uint32
ntlm_gss_release_cred(
    OM_uint32 *minor_status,
    gss_cred_id_t *cred_handle
    );

OM_uint32
ntlm_gss_init_sec_context(
    OM_uint32 *minor_status,
    const gss_cred_id_t initiator_cred_handle,
    gss_ctx_id_t *context_handle,
    const gss_name_t target_name,
    const gss_OID mech_type,
    OM_uint32 req_flags,
    OM_uint32 time_req,
    const gss_channel_bindings_t input_chan_bindings,
    const gss_buffer_t input_token,
    gss_OID *actual_mech_type,
    gss_buffer_t output_token,
    OM_uint32 *ret_flags,
    OM_uint32 *time_rec
    );

OM_uint32
ntlm_gss_accept_sec_context(
    OM_uint32 *minor_status,
    gss_ctx_id_t *context_handle,
    const gss_cred_id_t acceptor_cred_handle,
    const gss_buffer_t input_token_buffer,
    const gss_channel_bindings_t input_chan_bindings,
    gss_name_t *src_name,
    gss_OID *mech_type,
    gss_buffer_t output_token,
    OM_uint32 *ret_flags,
    OM_uint32 *time_rec,
    gss_cred_id_t *delegated_cred_handle
    );

OM_uint32
ntlm_gss_delete_sec_context(
    OM_uint32 *minor_status,
    gss_ctx_id_t *context_handle,
    gss_buffer_t output_token
    );

OM_uint32
ntlm_gss_import_name(
    OM_uint32 *minor_status,
    const gss_buffer_t input_name_buffer,
    const gss_OID input_name_type,
    gss_name_t *output_name
    );

OM_uint32
ntlm_gss_inquire_cred(
    OM_uint32 *minor_status,
    const gss_cred_id_t cred_handle,
    gss_name_t *name,
    OM_uint32 *lifetime,
    gss_cred_usage_t *cred_usage,
    gss_OID_set *mechanisms
    );

OM_uint32
ntlm_gss_inquire_sec_context_by_oid(
    OM_uint32* pMinorStatus,
    const gss_ctx_id_t GssCtxtHandle,
    const gss_OID Attrib,
    gss_buffer_set_t*
    );

#endif /* __GSSNTLM_H__ */
