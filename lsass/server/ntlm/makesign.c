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
 *        makesign.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        MakeSignature client wrapper API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
 */

#include "ntlmsrvapi.h"

DWORD
NtlmServerMakeSignature(
    IN PNTLM_CONTEXT_HANDLE phContext,
    IN BOOLEAN bEncrypt,
    IN OUT PSecBufferDesc pMessage,
    IN DWORD MessageSeqNo
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PNTLM_CONTEXT pContext = *phContext;
    // The following pointers point into pMessage and will not be freed
    PSecBuffer pToken = NULL;
    PSecBuffer pData = NULL;
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

    if (pContext->NegotiatedFlags & NTLM_FLAG_SIGN)
    {
        NtlmMakeSignature(
            pContext,
            pData,
            MessageSeqNo,
            pToken
            );
    }
    else if (pContext->NegotiatedFlags & NTLM_FLAG_ALWAYS_SIGN)
    {
        // Use the dummy signature 0x01000000000000000000000000000000
        memset(pToken->pvBuffer, 0, NTLM_SIGNATURE_SIZE);
        *((PDWORD)(pToken->pvBuffer)) = NTLM_VERSION;
    }
    else
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

VOID
NtlmMakeSignature(
    IN PNTLM_CONTEXT pContext,
    IN PSecBuffer pData,
    IN DWORD dwMsgSeqNum,
    IN OUT PSecBuffer pToken
    )
{
    DWORD dwCrc32 = 0;
    PBYTE pBuffer = pToken->pvBuffer;
    DWORD nBufferSize = pToken->cbBuffer;
    BYTE Signature[NTLM_SIGNATURE_SIZE] = {0};
    PNTLM_SIGNATURE pNtlmSig = (PNTLM_SIGNATURE)Signature;

    LW_ASSERT(pToken->cbBuffer == NTLM_SIGNATURE_SIZE);

    memset(pToken, 0, NTLM_SIGNATURE_SIZE);

    dwCrc32 = NtlmCrc32(pData->pvBuffer, pData->cbBuffer);

    pNtlmSig->dwMsgSeqNum = pContext->dwMsgSeqNum;
    pNtlmSig->dwCrc32 = dwCrc32;

    RC4(&pContext->SignAndSealKey, 12, &Signature[4], &Signature[4]);

    pNtlmSig->dwCounterValue = NTLM_COUNTER_VALUE;
    pNtlmSig->dwVersion = NTLM_VERSION;

    memcpy(pBuffer, Signature, nBufferSize);
}
