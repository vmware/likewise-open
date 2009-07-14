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

#include "client.h"

/* V2 interface */
OM_uint32
ntlm_gss_get_mic(
    OM_uint32 *minorStatus,
    gss_ctx_id_t contextHandle,
    gss_qop_t qop,
    gss_buffer_t messageBuffer,
    gss_buffer_t messageToken
    )
{

    DWORD dwError;
    PNTLM_CONTEXT gssContext = NULL;

    if (qop != GSS_C_QOP_DEFAULT)
        return GSS_S_BAD_QOP;

    gssContext = NTLMLocateContext((PNTLM_CONTEXT)contextHandle, NULL, 0);
    if (!gssContext)
        BAIL_WITH_LSA_ERROR(LW_ERROR_INVALID_CONTEXT);

    dwError = gssContext->signRoutine(
                            gssContext,
                            0,
                            messageBuffer,
                            messageToken
                            );

    BAIL_ON_NTLM_ERROR(dwError);

error:

    NTLMDereferenceContext(gssContext);

    (*minorStatus) = dwError;
    return NTLMTranslateMajorStatus(dwError);
}

OM_uint32
ntlm_gss_sign(
    OM_uint32 *minorStatus,
    gss_ctx_id_t contextHandle,
    int qop,
    gss_buffer_t messageBuffer,
    gss_buffer_t messageToken
    )
{

    return ntlm_gss_get_mic(
                minorStatus,
                contextHandle,
                (gss_qop_t) qop,
                messageBuffer,
                messageToken
                );
}

/* V2 interface */
OM_uint32
ntlm_gss_verify_mic(
    OM_uint32 *minorStatus,
    gss_ctx_id_t contextHandle,
    gss_buffer_t messageBuffer,
    gss_buffer_t tokenBuffer,
    gss_qop_t *qop
)
{
    DWORD dwError;
    PNTLM_CONTEXT gssContext = NULL;

    *qop = GSS_C_QOP_DEFAULT;
    gssContext = NTLMLocateContext((PNTLM_CONTEXT)contextHandle, NULL, 0);
    if (!gssContext)
        BAIL_WITH_LSA_ERROR(LW_ERROR_INVALID_CONTEXT);

    dwError = gssContext->verifyRoutine(
                            gssContext,
                            0,
                            messageBuffer,
                            tokenBuffer
                            );

    BAIL_ON_NTLM_ERROR(dwError);

error:

    NTLMDereferenceContext(gssContext);

    (*minorStatus) = dwError;
    return NTLMTranslateMajorStatus(dwError);
}

OM_uint32
ntlm_gss_verify(
    OM_uint32 *minorStatus,
    gss_ctx_id_t contextHandle,
    gss_buffer_t messageBuffer,
    gss_buffer_t tokenBuffer,
    int *qop
)
{
    return ntlm_gss_verify_mic(
                minorStatus,
                contextHandle,
                messageBuffer,
                tokenBuffer,
                (gss_qop_t *) qop
                );
}

