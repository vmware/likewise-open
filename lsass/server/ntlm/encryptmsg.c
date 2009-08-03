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
 *        encryptmsg.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        EncryptMessage client wrapper API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
 */

#include "ntlmsrvapi.h"

DWORD
NtlmServerEncryptMessage(
    IN PLSA_CONTEXT_HANDLE phContext,
    IN BOOL bEncrypt,
    IN OUT PSecBufferDesc pMessage,
    IN DWORD dwMsgSeqNum
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PLSA_CONTEXT pContext = *phContext;
    DWORD dwIndex = 0;
    PBYTE pToken = NULL;
    PBYTE pData = NULL;
    PBYTE pPadding = NULL;
    DWORD dwTokenSize = 0;
    DWORD dwDataSize = 0;
    DWORD dwPaddingSize = 0;
    RC4_KEY Rc4Key;

    memset(&Rc4Key, 0, sizeof(Rc4Key));

    // The message should be in the format of:
    // SECBUFFER_TOKEN      - Where the signature is placed
    // SECBUFFER_DATA       - The data we are signing
    // SECBUFFER_PADDING    - Padding (for RC4 or CRC32?)
    //
    // Find these buffers... the first one found of each type will be the one
    // that is used.
    for(dwIndex = 0; dwIndex < pMessage->cBuffers; dwIndex++)
    {
        if(pMessage->pBuffers[dwIndex].BufferType == SECBUFFER_TOKEN)
        {
            if(!pToken)
            {
                pToken = pMessage->pBuffers[dwIndex].pvBuffer;
                dwTokenSize = pMessage->pBuffers[dwIndex].cbBuffer;
            }
        }
        else if(pMessage->pBuffers[dwIndex].BufferType == SECBUFFER_DATA)
        {
            if(!pData)
            {
                pData = pMessage->pBuffers[dwIndex].pvBuffer;
                dwDataSize = pMessage->pBuffers[dwIndex].cbBuffer;
            }
        }
        else if(pMessage->pBuffers[dwIndex].BufferType == SECBUFFER_PADDING)
        {
            if(!pPadding)
            {
                pPadding = pMessage->pBuffers[dwIndex].pvBuffer;
                dwPaddingSize = pMessage->pBuffers[dwIndex].cbBuffer;
            }
        }
    }

    if (dwTokenSize != NTLM_SIGNATURE_SIZE ||
        !pToken || !pData || !pPadding || !dwDataSize || !dwPaddingSize)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LW_ERROR(dwError);
    }

    if(bEncrypt)
    {
        RC4_set_key(&Rc4Key, pContext->cbSessionKeyLen, pContext->SessionKey);
        RC4(&Rc4Key, dwDataSize, pData, pData);
    }

    NtlmMakeSignature(pContext, pData, dwDataSize, dwMsgSeqNum, pToken);

cleanup:
    return dwError;
error:
    goto cleanup;
}
