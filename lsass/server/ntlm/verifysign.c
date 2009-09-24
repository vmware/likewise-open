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
    OUT PDWORD pQop
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PNTLM_CONTEXT pContext = *phContext;
    // The following pointers point into pMessage and will not be freed
    PSecBuffer pToken = NULL;
    PSecBuffer pData = NULL;

    NtlmGetSecBuffers(pMessage, &pToken, &pData, NULL);

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

    dwError = NtlmVerifySignature(
        pContext,
        &pContext->VerifyKey,
        pData,
        pToken
        );
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    return dwError;

error:
    goto cleanup;
}

DWORD
NtlmVerifySignature(
    IN PNTLM_CONTEXT pContext,
    IN RC4_KEY* pSignKey,
    IN PSecBuffer pData,
    IN PSecBuffer pToken
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PNTLM_SIGNATURE pNtlmSig = NULL;
    DWORD dwCrc32 = 0;
    BYTE TokenData[NTLM_SIGNATURE_SIZE] = {0};

    //Decrypt the received token, remember to skip the first 4 bytes
    RC4(
        pSignKey,
        pToken->cbBuffer - 4,
        ((PBYTE)(pToken->pvBuffer)) + 4,
        TokenData);

    pNtlmSig = (PNTLM_SIGNATURE)TokenData;
    pNtlmSig->dwCounterValue = 0;

    if (pContext->NegotiatedFlags & NTLM_FLAG_ALWAYS_SIGN)
    {
        // Use the dummy signature 0x01000000000000000000000000000000
        dwCrc32 = 0;
    }
    else if (pContext->NegotiatedFlags & NTLM_FLAG_SIGN)
    {
        // generate a crc for the message
        dwCrc32 = NtlmCrc32(pData->pvBuffer, pData->cbBuffer);
    }
    else
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    // There is a minor issue we need to contend with... the windows client
    // appears to be sending us dummy signatures even when we have requested
    // that it not.  Check for it for now, and we'll figure out the cause.
    if (dwCrc32 != pNtlmSig->dwCrc32 &&
        0 != pNtlmSig->dwCrc32)
    {
        dwError = LW_ERROR_INVALID_MESSAGE;
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

VOID
NtlmGetSecBuffers(
    PSecBufferDesc pMessage,
    PSecBuffer* ppToken,
    PSecBuffer* ppData,
    PSecBuffer* ppPadding
    )
{
    DWORD dwIndex = 0;
    PSecBuffer pToken = NULL;
    PSecBuffer pData = NULL;
    PSecBuffer pPadding = NULL;

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
        else if (pMessage->pBuffers[dwIndex].BufferType == SECBUFFER_PADDING)
        {
            if (!pPadding)
            {
                pPadding = &pMessage->pBuffers[dwIndex];
            }
        }
    }

    if (ppToken)
    {
        *ppToken = pToken;
    }

    if (ppData)
    {
        *ppData = pData;
    }

    if (ppPadding)
    {
        *ppPadding = pPadding;
    }
}
