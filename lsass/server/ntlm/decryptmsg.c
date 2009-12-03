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
 *        decryptmsg.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        DecryptMessage client wrapper API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
 */

#include "ntlmsrvapi.h"

DWORD
NtlmServerDecryptMessage(
    IN NTLM_CONTEXT_HANDLE hContext,
    IN OUT PSecBufferDesc pMessage,
    IN DWORD MessageSeqNo,
    OUT PBOOLEAN pbEncrypted
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PSecBuffer pData = NULL;
    PSecBuffer pToken = NULL;
    PNTLM_CONTEXT pContext = hContext;
    PBYTE pBuffer = NULL;
    BOOLEAN bEncrypted = TRUE;

    NtlmGetSecBuffers(pMessage, &pToken, &pData, NULL);

    // Do a full sanity check here
    if (!pToken ||
        pToken->cbBuffer != NTLM_SIGNATURE_SIZE ||
        !pToken->pvBuffer ||
        !pData ||
        !pData->pvBuffer)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    // we need to play it safe and add padding to this decryption
    dwError = LwAllocateMemory(
        pData->cbBuffer + NTLM_PADDING_SIZE,
        OUT_PPVOID(&pBuffer));
    BAIL_ON_LSA_ERROR(dwError);

    RC4(
        pContext->pUnsealKey,
        pData->cbBuffer,
        pData->pvBuffer,
        pBuffer);

    memcpy(pData->pvBuffer, pBuffer, pData->cbBuffer);

    //verify the key
    dwError = NtlmVerifySignature(
        pContext,
        pContext->pUnsealKey,
        pData,
        pToken);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    LW_SAFE_FREE_MEMORY(pBuffer);

    if (pbEncrypted)
    {
        *pbEncrypted = bEncrypted;
    }
    return dwError;

error:

    goto cleanup;
}
