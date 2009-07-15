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
 *        context.c
 *
 * Abstract:
 *               ntlm_gss_accept_sec_context
 *
 *
 * Author:      Todd Stecher (2007)
 *              Krishna Ganugapati
 *              Marc Guy
 *
 */

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
     )
{
    DWORD dwError;
    SEC_BUFFER inputToken;
    SEC_BUFFER outputToken;
    LSA_STRING srcName;

    ZERO_STRUCT(srcName);

    *minorStatus = GSS_S_COMPLETE;

    dwError = NTLMInitializedCheck();
    BAIL_ON_NTLM_ERROR(dwError);

    /* @todo - vallidate OID / mech type  == NTLM*/

    MAKE_SECBUFFER(&inputToken, gssInputToken);

    /* @todo - translate OID */
    /* dwError = NTLMMarshallOID(); */
    /* BAIL_ON_LSA_ERROR(dwError); */

    /* @todo - channel_bindings */

    dwError = NTLMGssAcceptSecContext(
                        minorStatus,
                        (PVOID) credHandle,
                        (PVOID*)contextHandle,
                        &inputToken,
                        &srcName,
                        &outputToken,
                        (DWORD*) retFlags,
                        (PDWORD) retTime
                        );

    if (LW_ERROR_MASK(dwError))
        goto error;

    MAKE_GSS_BUFFER(gssOutputToken, &outputToken);
    outputToken.buffer = NULL;

    /* @todo translate actual mech type back */
    /* @todo src name */


cleanup:

    NTLM_SAFE_FREE(outputToken.buffer);
    return NTLMTranslateMajorStatus(dwError);

error:

    (*minorStatus) = NTLMTranslateMinorStatus(*minorStatus);
    goto cleanup;

}
