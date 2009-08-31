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
 *        structs.h
 *
 * Abstract:
 *
 *
 * Authors:
 *
 */

#ifndef __STRUCTS_H__
#define __STRUCTS_H__

typedef struct __NTLM_CLIENT_CONNECTION_CONTEXT
{
    LWMsgProtocol* pProtocol;
    LWMsgAssoc* pAssoc;
} NTLM_CLIENT_CONNECTION_CONTEXT, *PNTLM_CLIENT_CONNECTION_CONTEXT;

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

#endif /* __STRUCTS_H__ */
