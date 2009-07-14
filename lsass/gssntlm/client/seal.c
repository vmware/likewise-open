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
ntlm_gss_wrap(
    OM_uint32 *minorStatus,
    gss_ctx_id_t contextHandle,
    int confReq,
    gss_qop_t qopReq,
    gss_buffer_t data,
    int *confState,
    gss_buffer_t sealedData
)
{

    DWORD dwError;
    PNTLM_CONTEXT gssContext = NULL;

    if (qopReq != GSS_C_QOP_DEFAULT)
        return GSS_S_BAD_QOP;

    gssContext = NTLMLocateContext((PNTLM_CONTEXT)contextHandle, NULL, CONTEXT_BOTH);
    if (!gssContext)
        BAIL_WITH_NTLM_ERROR(LW_ERROR_INVALID_CONTEXT);

    dwError = gssContext->sealRoutine(
                            gssContext,
                            qopReq,
                            data,
                            sealedData
                            );

    BAIL_ON_NTLM_ERROR(dwError);

error:

    NTLMDereferenceContext(gssContext);

    (*minorStatus) = dwError;
    return NTLMTranslateMajorStatus(dwError);
}


OM_uint32
ntlm_gss_seal(
    OM_uint32 *minorStatus,
    gss_ctx_id_t contextHandle,
    int confReq,
    int qopReq,
    gss_buffer_t data,
    int *confState,
    gss_buffer_t sealedData
    )
{


    return ntlm_gss_wrap(
                minorStatus,
                contextHandle,
                confReq,
                (gss_qop_t) qopReq,
                data,
                confState,
                sealedData
                );

}


/* V2 interface */
OM_uint32
ntlm_gss_unwrap(
    OM_uint32 *minorStatus,
    gss_ctx_id_t contextHandle,
    gss_buffer_t sealedData,
    int *confRet,
    gss_qop_t *qopRet,
    gss_buffer_t data
)
{

    DWORD dwError;
    PNTLM_CONTEXT gssContext = NULL;

    *qopRet = GSS_C_QOP_DEFAULT;
    gssContext = NTLMLocateContext((PNTLM_CONTEXT)contextHandle, NULL, CONTEXT_BOTH);
    if (!gssContext)
        BAIL_WITH_NTLM_ERROR(LW_ERROR_INVALID_CONTEXT);

    dwError = gssContext->unsealRoutine(
                            gssContext,
                            0,
                            sealedData,
                            data
                            );

    BAIL_ON_NTLM_ERROR(dwError);

error:

    NTLMDereferenceContext(gssContext);

    (*minorStatus) = dwError;
    return NTLMTranslateMajorStatus(dwError);
}


OM_uint32
ntlm_gss_unseal(
    OM_uint32 *minorStatus,
    gss_ctx_id_t contextHandle,
    gss_buffer_t sealedData,
    gss_buffer_t data,
    int *confRet,
    int *qopRet
    )
{


    return ntlm_gss_unwrap(
                minorStatus,
                contextHandle,
                sealedData,
                confRet,
                (gss_qop_t*) qopRet,
                data
                );

}

OM_uint32
ntlm_gss_wrap_size_limit(
    OM_uint32 *minorStatus,
    gss_ctx_id_t contextHandle,
    int confReq,
    gss_qop_t qopReq,
    OM_uint32 reqOutputSize,
    OM_uint32 *maxInputSize
)
{
    *minorStatus = GSS_S_COMPLETE;

    (*maxInputSize) = reqOutputSize - sizeof(NTLM_SIGNATURE);

    return GSS_S_COMPLETE;
}
