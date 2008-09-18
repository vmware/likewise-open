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
 *      gssntlm.h
 *   
 * Abstract:
 *
 *      NTLM Client GSSAPI 
 *
 * Authors: Todd Stecher
 */
 #ifndef _GSSNTLM_H_
 #define _GSSNTLM_H_

#include <gssapi.h>

/* 1.3.6.1.4.1.311.2.2.10 */
#define GSS_MECH_NTLM       "\053\006\001\004\001\001\067\002\002\012"
#define GSS_MECH_NTLM_LEN   10




/* 
 * The below is not specified in gssapi RFC, but is 
 * provided to allow for parity with Windows AcquireCredentialsHandle()
 * SSPI API.  Primarily, this is due to the lack of a universal provisioning
 * of credentials for NTLM, ala a kerberos keytab.  Not specifying a cred
 * buffer will default to the currently logged on users credentials.
 *
 */ 

#ifndef _PURE_RFC_GSSAPI_
OM_uint32 
ntlm_gss_acquire_supplied_cred(
    OM_uint32 *minorStatus,
    gss_name_t desiredName,
    gss_buffer_t credBuffer,
    OM_uint32 reqTime,
    gss_OID_set desiredMechs,
    gss_cred_usage_t credUsage,
    gss_cred_id_t *credHandle,
    gss_OID_set *actualMechs,
    OM_uint32 *retTime
);

OM_uint32
ntlm_gss_marshal_supplied_cred(
    OM_uint32 *minorStatus,
    char *username,
    char *domain,
    char *password,
    gss_buffer_t marshalledCred
    );

#endif 

/* required to make an init call before any of the below */
OM_uint32
ntlm_gss_init(OM_uint32 *minorStatus);


OM_uint32 
ntlm_gss_acquire_cred(
    OM_uint32 *minorStatus,
    gss_name_t desiredName,
    OM_uint32 reqTime,
    gss_OID_set desiredMechs,
    gss_cred_usage_t credUsage,
    gss_cred_id_t *credHandle,
    gss_OID_set *actualMechs,
    OM_uint32 *retTime
);

OM_uint32 
ntlm_gss_release_cred(
    OM_uint32 *minorStatus,
    gss_cred_id_t *credHandle
);

OM_uint32
ntlm_gss_accept_sec_context(
     OM_uint32 *minorStatus,
     gss_ctx_id_t *contextHandle,
     gss_cred_id_t credHandle,
     gss_buffer_t gssInputToken,
     gss_channel_bindings_t inputChanBindings,
     gss_name_t *gssSrcName,
     gss_OID *gssMechType,
     gss_buffer_t gssOutputToken,
     OM_uint32 *retFlags,
     OM_uint32 *retTime,
     gss_cred_id_t *delegatedCredHandle
     );


OM_uint32
ntlm_gss_init_sec_context(
    OM_uint32 *minorStatus,
    gss_cred_id_t credHandle,
    gss_ctx_id_t *contextHandle,
    gss_name_t gssTargetName,
    gss_OID mechType,
    OM_uint32 reqFlags,
    OM_uint32 reqTime,
    gss_channel_bindings_t channelBindings,
    gss_buffer_t gssInputToken,
    gss_OID *actualMechtype,
    gss_buffer_t gssOutputToken,
    OM_uint32 *retFlags,
    OM_uint32 *retTime
    );

OM_uint32
ntlm_gss_delete_sec_context(
     OM_uint32 *minorStatus,
     gss_ctx_id_t *contextHandle,
     gss_buffer_t outputToken
     );

OM_uint32
ntlm_gss_release_buffer(
    OM_uint32 *minorStatus,
    gss_buffer_t buffer
);


OM_uint32
ntlm_gss_wrap(
    OM_uint32 *minorStatus,
    gss_ctx_id_t contextHandle,
    int confReq,
    gss_qop_t qopReq,
    gss_buffer_t data,
    int *confState,
    gss_buffer_t sealedData
);
 
OM_uint32
ntlm_gss_seal(
    OM_uint32 *minorStatus,
    gss_ctx_id_t contextHandle,
    int confReq,
    int qopReq,
    gss_buffer_t data,
    int *confState,
    gss_buffer_t sealedData
);


OM_uint32
ntlm_gss_unwrap(
    OM_uint32 *minorStatus,
    gss_ctx_id_t contextHandle,
    gss_buffer_t sealedData,
    int *confRet,
    gss_qop_t *qopRet,
    gss_buffer_t data
);

OM_uint32
ntlm_gss_unseal(
    OM_uint32 *minorStatus,
    gss_ctx_id_t contextHandle,
    gss_buffer_t sealedData,
    gss_buffer_t data,
    int *confRet,
    int *qopRet
);

OM_uint32
ntlm_gss_wrap_size_limit(
    OM_uint32 *minorStatus,
    gss_ctx_id_t contextHandle,
    int confReq,
    gss_qop_t qopReq,
    OM_uint32 reqOutputSize,
    OM_uint32 *maxInputSize
);

#endif
 
 
 
