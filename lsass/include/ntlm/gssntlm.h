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

#if 0
#define gss_acquire_cred ntlm_gss_acquire_cred
#define gss_release_cred ntlm_gss_release_cred
#define gss_init_sec_context ntlm_gss_init_sec_context
#define gss_accept_sec_context ntlm_gss_accept_sec_context
#define gss_process_context_token ntlm_gss_process_context_token
#define gss_delete_sec_context ntlm_gss_delete_sec_context
#define gss_context_time ntlm_gss_context_time
#define gss_get_mic ntlm_gss_get_mic
#define gss_verify_mic ntlm_gss_verify_mic
#define gss_wrap ntlm_gss_wrap
#define gss_unwrap ntlm_gss_unwrap
#define gss_display_status ntlm_gss_display_status
#define gss_indicate_mechs ntlm_gss_indicate_mechs
#define gss_compare_name ntlm_gss_compare_name
#define gss_display_name ntlm_gss_display_name
#define gss_import_name ntlm_gss_import_name
#define gss_export_name ntlm_gss_export_name
#define gss_release_name ntlm_gss_release_name
#define gss_release_buffer ntlm_gss_release_buffer
#define gss_release_oid_set ntlm_gss_release_oid_set
#define gss_inquire_cred ntlm_gss_inquire_cred
#define gss_inquire_context ntlm_gss_inquire_context
#define gss_inquire_context2 ntlm_gss_inquire_context2
#define gss_wrap_size_limit ntlm_gss_wrap_size_limit
#define gss_add_cred ntlm_gss_add_cred
#define gss_inquire_cred_by_mech ntlm_gss_inquire_cred_by_mech
#define gss_export_sec_context ntlm_gss_export_sec_context
#define gss_import_sec_context ntlm_gss_import_sec_context
#define gss_create_empty_oid_set ntlm_gss_create_empty_oid_set
#define gss_add_oid_set_member ntlm_gss_add_oid_set_member
#define gss_test_oid_set_member ntlm_gss_test_oid_set_member
#define gss_inquire_names_for_mech ntlm_gss_inquire_names_for_mech
#define gss_inquire_mechs_for_name ntlm_gss_inquire_mechs_for_name
#define gss_canonicalize_name ntlm_gss_canonicalize_name
#define gss_duplicate_name ntlm_gss_duplicate_name
#define gss_sign ntlm_gss_sign
#define gss_verify ntlm_gss_verify
#define gss_seal ntlm_gss_seal
#define gss_unseal ntlm_gss_unseal
#endif

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
ntlm_gss_process_context_token(
    OM_uint32 *minor_status,
    const gss_ctx_id_t context_handle,
    const gss_buffer_t token_buffer
    );

OM_uint32
ntlm_gss_delete_sec_context(
    OM_uint32 *minor_status,
    gss_ctx_id_t *context_handle,
    gss_buffer_t output_token
    );

OM_uint32
ntlm_gss_context_time(
    OM_uint32 *minor_status,
    const gss_ctx_id_t context_handle,
    OM_uint32 *time_rec
    );

OM_uint32
ntlm_gss_get_mic(
    OM_uint32 *minor_status,
    const gss_ctx_id_t context_handle,
    gss_qop_t qop_req,
    const gss_buffer_t message_buffer,
    gss_buffer_t message_token
    );

OM_uint32
ntlm_gss_verify_mic(
    OM_uint32 *minor_status,
    const gss_ctx_id_t context_handle,
    const gss_buffer_t message_buffer,
    const gss_buffer_t token_buffer,
    gss_qop_t *qop_state
    );

OM_uint32
ntlm_gss_wrap(
    OM_uint32 *minor_status,
    const gss_ctx_id_t context_handle,
    int conf_req_flag,
    gss_qop_t qop_req,
    const gss_buffer_t input_message_buffer,
    int *conf_state,
    gss_buffer_t output_message_buffer
    );

OM_uint32
ntlm_gss_unwrap(
    OM_uint32 *minor_status,
    const gss_ctx_id_t context_handle,
    const gss_buffer_t input_message_buffer,
    gss_buffer_t output_message_buffer,
    int *conf_state,
    gss_qop_t *qop_state
    );

OM_uint32
ntlm_gss_display_status(
    OM_uint32 *minor_status,
    OM_uint32 status_value,
    int status_type,
    const gss_OID mech_type,
    OM_uint32 *message_context,
    gss_buffer_t status_string
    );

OM_uint32
ntlm_gss_indicate_mechs(
    OM_uint32 *minor_status,
    gss_OID_set *mech_set
    );

OM_uint32
ntlm_gss_compare_name(
    OM_uint32 *minor_status,
    const gss_name_t name1,
    const gss_name_t name2,
    int *name_equal
    );

OM_uint32
ntlm_gss_display_name(
    OM_uint32 *minor_status,
    const gss_name_t input_name,
    gss_buffer_t output_name_buffer,
    gss_OID *output_name_type
    );

OM_uint32
ntlm_gss_import_name(
    OM_uint32 *minor_status,
    const gss_buffer_t input_name_buffer,
    const gss_OID input_name_type,
    gss_name_t *output_name
    );

OM_uint32
ntlm_gss_export_name(
    OM_uint32 *minor_status,
    const gss_name_t input_name,
    gss_buffer_t exported_name
    );

OM_uint32
ntlm_gss_release_name(
    OM_uint32 *minor_status,
    gss_name_t *name
    );

OM_uint32
ntlm_gss_release_buffer(
    OM_uint32 *minor_status,
    gss_buffer_t buffer
    );

OM_uint32
ntlm_gss_release_oid_set(
    OM_uint32 *minor_status,
    gss_OID_set *set
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
ntlm_gss_inquire_context(
    OM_uint32 *minor_status,
    const gss_ctx_id_t context_handle,
    gss_name_t *src_name,
    gss_name_t *targ_name,
    OM_uint32 *lifetime_rec,
    gss_OID *mech_type,
    OM_uint32 *ctx_flags,
    int *locally_initiated,
    int *open
    );

OM_uint32
ntlm_gss_inquire_context2(
    OM_uint32 *minor_status,
    const gss_ctx_id_t context_handle,
    gss_name_t *src_name,
    gss_name_t *targ_name,
    OM_uint32 *lifetime_rec,
    gss_OID *mech_type,
    OM_uint32 *ctx_flags,
    int *locally_initiated,
    int *open,
    gss_buffer_t session_key
    );

OM_uint32
ntlm_gss_wrap_size_limit(
    OM_uint32 *minor_status,
    const gss_ctx_id_t context_handle,
    int conf_req_flag,
    gss_qop_t qop_req,
    OM_uint32 req_output_size,
    OM_uint32 *max_input_size
    );

OM_uint32
ntlm_gss_add_cred(
    OM_uint32 *minor_status,
    const gss_cred_id_t input_cred_handle,
    const gss_name_t desired_name,
    const gss_OID desired_mech,
    gss_cred_usage_t cred_usage,
    OM_uint32 initiator_time_req,
    OM_uint32 acceptor_time_req,
    gss_cred_id_t *output_cred_handle,
    gss_OID_set *actual_mechs,
    OM_uint32 *initiator_time_rec,
    OM_uint32 *acceptor_time_rec
    );

OM_uint32
ntlm_gss_inquire_cred_by_mech(
    OM_uint32 *minor_status,
    const gss_cred_id_t cred_handle,
    const gss_OID mech_type,
    gss_name_t *name,
    OM_uint32 *initiator_lifetime,
    OM_uint32 *acceptor_lifetime,
    gss_cred_usage_t *cred_usage
    );

OM_uint32
ntlm_gss_export_sec_context(
    OM_uint32 *minor_status,
    gss_ctx_id_t *context_handle,
    gss_buffer_t interprocess_token
    );

OM_uint32
ntlm_gss_import_sec_context(
    OM_uint32 *minor_status,
    const gss_buffer_t interprocess_token,
    gss_ctx_id_t *context_handle
    );

OM_uint32
ntlm_gss_create_empty_oid_set(
    OM_uint32 *minor_status,
    gss_OID_set *oid_set
    );

OM_uint32
ntlm_gss_add_oid_set_member(
    OM_uint32 *minor_status,
    const gss_OID member_oid,
    gss_OID_set *oid_set
    );

OM_uint32
ntlm_gss_test_oid_set_member(
    OM_uint32 *minor_status,
    const gss_OID member,
    const gss_OID_set set,
    int *present
    );

OM_uint32
ntlm_gss_inquire_names_for_mech(
    OM_uint32 *minor_status,
    const gss_OID mechanism,
    gss_OID_set *name_types
    );

OM_uint32
ntlm_gss_inquire_mechs_for_name(
    OM_uint32 *minor_status,
    const gss_name_t input_name,
    gss_OID_set *mech_types
    );

OM_uint32
ntlm_gss_canonicalize_name(
    OM_uint32 *minor_status,
    const gss_name_t input_name,
    const gss_OID mech_type,
    gss_name_t *output_name
    );

OM_uint32
ntlm_gss_duplicate_name(
    OM_uint32 *minor_status,
    const gss_name_t src_name,
    gss_name_t *dest_name
    );

OM_uint32
ntlm_gss_sign(
    OM_uint32 *minor_status,
    gss_ctx_id_t context_handle,
    int qop_req,
    gss_buffer_t message_buffer,
    gss_buffer_t message_token
    );

OM_uint32
ntlm_gss_verify(
    OM_uint32 *minor_status,
    gss_ctx_id_t context_handle,
    gss_buffer_t message_buffer,
    gss_buffer_t token_buffer,
    int *qop_state
    );

OM_uint32
ntlm_gss_seal(
    OM_uint32 *minor_status,
    gss_ctx_id_t context_handle,
    int conf_req_flag,
    int qop_req,
    gss_buffer_t input_message_buffer,
    int *conf_state,
    gss_buffer_t output_message_buffer
    );

OM_uint32
ntlm_gss_unseal(
    OM_uint32 *minor_status,
    gss_ctx_id_t context_handle,
    gss_buffer_t input_message_buffer,
    gss_buffer_t output_message_buffer,
    int *conf_state,
    int *qop_state
    );

#endif /* __GSSNTLM_H__ */
