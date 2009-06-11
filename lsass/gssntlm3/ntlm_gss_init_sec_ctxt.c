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
 *
 *        GSS context management and APIs
 *
 * Author: Todd Stecher (2007)
 *
 */

#include "client.h"



/*
 *  GSS SEC CONTEXT APIs
 *
 *  The following are stubs into IPC calls
 *
 */

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
    )
{
    DWORD dwError;
    SEC_BUFFER inputToken;
    SEC_BUFFER outputToken;
    LSA_STRING targetName;
    /* OID *actualMechType = NULL; */

    ZERO_STRUCT(targetName);
    ZERO_STRUCT(channelBindings);
    ZERO_STRUCT(outputToken);

    *minorStatus = GSS_S_COMPLETE;

    dwError = NTLMInitializedCheck();
    BAIL_ON_NTLM_ERROR(dwError);

    /* @todo - validate parameters */
    if (credHandle == NULL)
    {
        dwError = LSA_ERROR_NO_CRED;
        BAIL_ON_NTLM_ERROR(dwError);
    }

    if (contextHandle == NULL)
    {
        dwError = LSA_ERROR_NO_CONTEXT;
        BAIL_ON_NTLM_ERROR(dwError);
    }

    /* @todo - vallidate OID / mech type  == NTLM*/

    /* @todo - validate buffers */
    MAKE_SECBUFFER(&inputToken, gssInputToken);

    /* @todo - NT name is simply char */
    dwError = NTLMTranslateGSSName(
                        gssTargetName,
                        &targetName
                        );

    BAIL_ON_NTLM_ERROR(dwError);

    /* @todo - translate OID */
    /* dwError = NTLMMarshallOID(); */
    /* BAIL_ON_LSA_ERROR(dwError); */

    /* @todo - translate channel_bindings */
    /* dwError = NTLMMarshallChannelBindings(); */
    /* BAIL_ON_LSA_ERROR(dwError); */

    dwError = NTLMGssInitSecContext(
                    minorStatus,
                    (PVOID) credHandle,
                    (PVOID*)contextHandle,
                    &targetName,
                    reqFlags,
                    reqTime,
                    &inputToken,
                    &outputToken,
                    (DWORD*)retFlags,
                    (DWORD*)retTime
                    );

    if (LSA_ERROR_MASK(dwError))
       goto error;

    MAKE_GSS_BUFFER(gssOutputToken, &outputToken);
    outputToken.buffer = NULL;

cleanup:

    NTLM_SAFE_FREE(outputToken.buffer);

    return NTLMTranslateMajorStatus(dwError);

error:

    (*minorStatus) = NTLMTranslateMinorStatus(*minorStatus);

    goto cleanup;

}

