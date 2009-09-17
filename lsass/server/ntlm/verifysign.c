/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        verifysign.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        VerifySignature client wrapper API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
 */

#include "ntlmsrvapi.h"

DWORD
NtlmServerVerifySignature(
    IN PNTLM_CONTEXT_HANDLE phContext,
    IN PSecBufferDesc pMessage,
    IN DWORD MessageSeqNo,
    OUT PBOOLEAN pbVerified,
    OUT PBOOLEAN pbEncrypted
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PNTLM_CONTEXT pContext = *phContext;
    // The following pointers point into pMessage and will not be freed
    PSecBuffer pToken = NULL;
    PSecBuffer pData = NULL;
    SecBuffer TempToken = {0};
    BYTE TempTokenData[NTLM_SIGNATURE_SIZE] = {0};
    BOOLEAN bVerified = FALSE;
    BOOLEAN bEncrypted = FALSE;
    DWORD dwIndex = 0;

    for (dwIndex = 0; dwIndex < pMessage->cBuffers; dwIndex++)
    {
        if (pMessage->pBuffers[dwIndex].BufferType == SECBUFFER_TOKEN)
        {
            if (!pToken)
            {
                pToken = &pMessage->pBuffers[dwIndex];
            }
        }
        else if (pMessage->pBuffers[dwIndex].BufferType == SECBUFFER_DATA)
        {
            if (!pData)
            {
                pData = &pMessage->pBuffers[dwIndex];
            }
        }
    }

    // Do a full sanity check here
    if (!pToken ||
        pToken->cbBuffer != NTLM_SIGNATURE_SIZE ||
        !pToken->pvBuffer ||
        !pData ||
        !pData->cbBuffer ||
        !pData->pvBuffer)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    TempToken.BufferType = SECBUFFER_TOKEN;
    TempToken.cbBuffer = NTLM_SIGNATURE_SIZE;
    TempToken.pvBuffer = TempTokenData;

    if (pContext->NegotiatedFlags & NTLM_FLAG_SIGN)
    {
        NtlmMakeSignature(
            pContext,
            pData,
            MessageSeqNo,
            &TempToken
            );
    }
    else if (pContext->NegotiatedFlags & NTLM_FLAG_ALWAYS_SIGN)
    {
        // Use the dummy signature 0x01000000000000000000000000000000
        memset(TempTokenData, 0, NTLM_SIGNATURE_SIZE);
        *((PDWORD)(TempTokenData)) = NTLM_VERSION;
    }
    else
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (TempToken.cbBuffer == pToken->cbBuffer &&
        !memcmp(TempToken.pvBuffer, pToken->pvBuffer, pToken->cbBuffer))
    {
        bVerified = TRUE;
    }

cleanup:
    *pbVerified = bVerified;
    *pbEncrypted = bEncrypted;

    return dwError;

error:
    bVerified = 0;
    bEncrypted = 0;

    goto cleanup;
}
