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
 *        NTLM Security Context for negotiate/challenge/authenticate
 *
 * Authors: Marc Guy (mguy@likewisesoftware.com)
 *
 */

#include "ntlmsrvapi.h"


static LSA_CONTEXT_STATE gContextState;

/******************************************************************************/
DWORD
NtlmInitializeContextDatabase(
    VOID
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;

    dwError = pthread_rwlock_init(&gContextState.LsaContextListLock, NULL);
    if(LW_ERROR_SUCCESS != dwError)
    {
        dwError = LW_ERROR_INTERNAL; //LwMapErrnoToLwError(errno);
        BAIL_ON_NTLM_ERROR(dwError);
    }

    LsaListInit(&gContextState.LsaContextList);

error:
    return dwError;
}

/******************************************************************************/
VOID
NtlmShutdownContextDatabase(
    VOID
    )
{
    BOOL bInLock = FALSE;
    PLSA_CONTEXT pContext = NULL;
    PLSA_LIST_LINKS pContextListEntry = NULL;

    ENTER_CONTEXT_LIST_WRITER(bInLock);

        // sweep the context list
        while(!LsaListIsEmpty(&gContextState.LsaContextList))
        {
            pContextListEntry = LsaListRemoveHead(&gContextState.LsaContextList);
            pContext = LW_STRUCT_FROM_FIELD(
                pContextListEntry,
                LSA_CONTEXT,
                ListEntry);

            NtlmFreeContext(pContext);
        }

    LEAVE_CONTEXT_LIST_WRITER(bInLock);

    pthread_rwlock_destroy(&gContextState.LsaContextListLock);

    return;
}

/******************************************************************************/
VOID
NtlmAddContext(
    IN PLSA_CONTEXT         pContext,
    OUT PLSA_CONTEXT_HANDLE pContextHandle
    )
{
    BOOL bInLock = FALSE;

    ENTER_CONTEXT_LIST_WRITER(bInLock);

        LsaListInsertBefore(&gContextState.LsaContextList, &pContext->ListEntry);

    LEAVE_CONTEXT_LIST_WRITER(bInLock);

    *pContextHandle = pContext;

    return;
}

/******************************************************************************/
VOID
NtlmReleaseContext(
    IN LSA_CONTEXT_HANDLE hContext
    )
{
    BOOL bInLock = FALSE;
    PLSA_CONTEXT pContext = hContext;

    ENTER_CONTEXT_LIST_WRITER(bInLock);

        pContext->nRefCount--;

        LW_ASSERT(pContext->nRefCount >= 0);

        if (!(pContext->nRefCount))
        {
            LsaListRemove(&pContext->ListEntry);
            NtlmFreeContext(pContext);
        }

    LEAVE_CONTEXT_LIST_WRITER(bInLock);
    return;
}

/******************************************************************************/
VOID
NtlmGetContextInfo(
    IN LSA_CONTEXT_HANDLE           ContextHandle,
    OUT OPTIONAL PNTLM_STATE        pNtlmState,
    OUT OPTIONAL PVOID*             ppMessage,
    OUT OPTIONAL PDWORD             pdwMessageSize,
    OUT OPTIONAL PLSA_CRED_HANDLE   pCredHandle
    )
{
    PLSA_CONTEXT pContext = ContextHandle;
    BOOL bInLock = FALSE;

    ENTER_CONTEXT_LIST_WRITER(bInLock);

        if(pNtlmState)
        {
            *pNtlmState = pContext->NtlmState;
        }

        if(ppMessage && pdwMessageSize)
        {
            *ppMessage = pContext->pMessage;
            *pdwMessageSize = pContext->dwMessageSize;
        }

        if(pCredHandle)
        {
            *pCredHandle = pContext->CredHandle;
        }

    LEAVE_CONTEXT_LIST_WRITER(bInLock);

    return;
}

/******************************************************************************/
DWORD
NtlmInitContext(
    OUT PLSA_CONTEXT* ppNtlmContext
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PLSA_CONTEXT pContext = NULL;

    if(!ppNtlmContext)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_NTLM_ERROR(dwError);
    }

    *ppNtlmContext = NULL;

    dwError = LsaAllocateMemory(
        sizeof(LSA_CONTEXT),
        (PVOID*)(PVOID)pContext
        );

    BAIL_ON_NTLM_ERROR(dwError);

    pContext->NtlmState = NtlmStateBlank;

cleanup:
    *ppNtlmContext = pContext;
    return dwError;
error:
    LW_SAFE_FREE_MEMORY(pContext);
    goto cleanup;
}

/******************************************************************************/
VOID
NtlmFreeContext(
    IN PLSA_CONTEXT pContext
    )
{
    // For now, we're not adding a ref to the credential (though we probably
    // should?)
    // LsaReleaseCredential(pContext->CredHandle);

    memset(pContext->pMessage, 0, pContext->dwMessageSize);
    LW_SAFE_FREE_MEMORY(pContext->pMessage);

    LW_SAFE_FREE_MEMORY(pContext);

    return;
}

/******************************************************************************/
DWORD
NtlmCreateContextFromSecBufferDesc(
    IN PSecBufferDesc   pSecBufferDesc,
    IN NTLM_STATE       nsContextType,
    OUT PLSA_CONTEXT*   ppNtlmContext
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PSecBuffer pSecBuffer = NULL;
    PLSA_CONTEXT pContext = NULL;

    if(!ppNtlmContext || !pSecBufferDesc)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_NTLM_ERROR(dwError);
    }

    *ppNtlmContext = NULL;

    // Transfered context tokens only contain one SecBuffer and are tagged as
    // tokens... verify
    if(pSecBufferDesc->cBuffers != 1 || !(pSecBufferDesc->pBuffers))
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_NTLM_ERROR(dwError);
    }

    pSecBuffer = pSecBufferDesc->pBuffers;

    if(pSecBuffer->BufferType != SECBUFFER_TOKEN ||
       pSecBuffer->cbBuffer == 0)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_NTLM_ERROR(dwError);
    }

    dwError = LsaAllocateMemory(
        sizeof(LSA_CONTEXT),
        (PVOID*)(PVOID)&pContext
        );

    BAIL_ON_NTLM_ERROR(dwError);

    dwError = LsaAllocateMemory(
        pSecBuffer->cbBuffer,
        &(pContext->pMessage)
        );

    BAIL_ON_NTLM_ERROR(dwError);

    memcpy(
        pContext->pMessage,
        pSecBuffer->pvBuffer,
        pSecBuffer->cbBuffer
        );

    pContext->NtlmState = nsContextType;

    // the only thing we don't have is a cred handle.  We'll either have to
    // look that up based on the user name passed through the message (depending
    // on the message type) or find it another way.

cleanup:
    *ppNtlmContext = pContext;

    return dwError;
error:
    LW_SAFE_FREE_MEMORY(pContext->pMessage);
    LW_SAFE_FREE_MEMORY(pContext);
    goto cleanup;
}

/******************************************************************************/
DWORD
NtlmCopyContextToSecBufferDesc(
    IN PLSA_CONTEXT         pNtlmContext,
    IN OUT PSecBufferDesc   pSecBufferDesc
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PSecBuffer pSecBuffer = NULL;

    // We want to make sure that sec buffer desc we write to only contain 1
    // buffer and that buffer is of type SECBUFFER_TOKEN
    if(!pSecBufferDesc || 1 != pSecBufferDesc->cBuffers)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LW_ERROR(dwError);
    }

    pSecBuffer = pSecBufferDesc->pBuffers;

    if(pSecBuffer->BufferType != SECBUFFER_TOKEN)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LW_ERROR(dwError);
    }

    // As long as the caller holds onto the context handle (which they should
    // have), the pMessage buffer should be legit, so this should be safe
    // without a deep copy.
    pSecBuffer->cbBuffer = pNtlmContext->dwMessageSize;
    pSecBuffer->pvBuffer = pNtlmContext->pMessage;

cleanup:
    return dwError;
error:
    goto cleanup;
}

/******************************************************************************/
DWORD
NtlmGetRandomBuffer(
    OUT PBYTE pBuffer,
    IN DWORD dwLen
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    INT nFileDesc;
    DWORD dwBytesRead = 0;

    if(!pBuffer || dwLen <= 0)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_NTLM_ERROR(dwError);
    }

    nFileDesc = open(NTLM_URANDOM_DEV, O_RDONLY);
    if(-1 == nFileDesc)
    {
        nFileDesc = open(NTLM_RANDOM_DEV, O_RDONLY);
        if(-1 == nFileDesc)
        {
            dwError = LW_ERROR_INTERNAL; //LwMapErrnoToLwError(errno);
            BAIL_ON_NTLM_ERROR(dwError);
        }
    }

    dwBytesRead = read(nFileDesc, pBuffer, dwLen);
    close(nFileDesc);

    if(dwBytesRead < dwLen)
    {
        dwError = LW_ERROR_INTERNAL;
    }

error:
    return dwError;
}


/******************************************************************************/
DWORD NtlmCreateNegotiateMessage(
    IN DWORD                        dwOptions,
    IN PCHAR                        pDomain,
    IN PCHAR                        pWorkstation,
    IN PBYTE                        pOsVersion,
    OUT PDWORD                      pdwSize,
    OUT PNTLM_NEGOTIATE_MESSAGE*    ppNegMsg
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PNTLM_SEC_BUFFER pDomainSecBuffer = NULL;
    PNTLM_SEC_BUFFER pWorkstationSecBuffer = NULL;
    PBYTE pBuffer = NULL;
    PNTLM_NEGOTIATE_MESSAGE pMessage = NULL;
    DWORD dwSize = 0;

    // sanity checks
    if(!ppNegMsg)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_NTLM_ERROR(dwError);
    }

    *ppNegMsg = NULL;
    *pdwSize = 0;

    if(dwOptions & NTLM_FLAG_DOMAIN)
    {
        if(!pDomain)
        {
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_NTLM_ERROR(dwError);
        }
    }

    if(dwOptions & NTLM_FLAG_WORKSTATION)
    {
        if(!pWorkstation)
        {
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_NTLM_ERROR(dwError);
        }
    }

    dwSize = sizeof(NTLM_NEGOTIATE_MESSAGE);

    // There is no flag to indicate if there is OS version information added
    // to the packet... if we have OS information, we will need to allocate
    // Domain and Workstation information as well.
    if(pOsVersion)
    {
        dwSize +=
            sizeof(NTLM_SEC_BUFFER) +
            sizeof(NTLM_SEC_BUFFER) +
            NTLM_WIN_SPOOF_SIZE;
    }
    else if(dwOptions & NTLM_FLAG_WORKSTATION)
    {
        dwSize +=
            sizeof(NTLM_SEC_BUFFER) +
            sizeof(NTLM_SEC_BUFFER);
    }
    else if(dwOptions & NTLM_FLAG_DOMAIN)
    {
        dwSize += sizeof(NTLM_SEC_BUFFER);
    }

    dwError = LsaAllocateMemory(dwSize, (PVOID*)(PVOID)&pMessage);
    BAIL_ON_NTLM_ERROR(dwError);

    // Data is checked and memory is allocated; fill in the structure
    //
    memcpy(&(pMessage->NtlmSignature), NTLM_SIGNATURE, NTLM_SIGNATURE_SIZE);
    pMessage->MessageType = NTLM_NEGOTIATE_MSG;
    pMessage->NtlmFlags = dwOptions;

    // Start writing optional information (if there is any) after the structure
    pBuffer = (PBYTE)pMessage + sizeof(NTLM_NEGOTIATE_MESSAGE);

    if(dwOptions & NTLM_FLAG_DOMAIN)
    {
        pDomainSecBuffer = (PNTLM_SEC_BUFFER)pBuffer;

        // The Domain name is ALWAYS given as an OEM (i.e. ASCII) string
        pDomainSecBuffer->usLength = strlen(pDomain);
        pDomainSecBuffer->usMaxLength = pDomainSecBuffer->usLength;

        pBuffer += sizeof(NTLM_SEC_BUFFER);
    }

    if(dwOptions & NTLM_FLAG_WORKSTATION)
    {
        pWorkstationSecBuffer = (PNTLM_SEC_BUFFER)pBuffer;

        // The Workstation name is also ALWAYS given as an OEM string
        pWorkstationSecBuffer->usLength = strlen(pWorkstation);
        pWorkstationSecBuffer->usMaxLength = pWorkstationSecBuffer->usLength;

        pBuffer += sizeof(NTLM_SEC_BUFFER);
    }

    if(pOsVersion)
    {
        memcpy(pBuffer, pOsVersion, NTLM_WIN_SPOOF_SIZE);

        pBuffer += NTLM_WIN_SPOOF_SIZE;
    }

    // So now we're at the very end of the buffer; copy the optional data
    if(pWorkstationSecBuffer && pWorkstationSecBuffer->usLength)
    {
        memcpy(pBuffer, pWorkstation, pWorkstationSecBuffer->usLength);
        pWorkstationSecBuffer->dwOffset = pBuffer - (PBYTE)pMessage;
        pBuffer += pWorkstationSecBuffer->usLength;
    }

    if(pDomainSecBuffer && pDomainSecBuffer->usLength)
    {
        memcpy(pBuffer, pDomain, pDomainSecBuffer->usLength);
        pDomainSecBuffer->dwOffset = pBuffer - (PBYTE)pMessage;
    }

cleanup:
    *ppNegMsg = pMessage;
    *pdwSize = dwSize;
    return dwError;
error:
    LW_SAFE_FREE_MEMORY(pMessage);
    dwSize = 0;
    goto cleanup;
}

/******************************************************************************/
DWORD
NtlmCreateChallengeMessage(
    IN PNTLM_NEGOTIATE_MESSAGE      pNegMsg,
    IN PCHAR                        pServerName,
    IN PCHAR                        pDomainName,
    IN PCHAR                        pDnsServerName,
    IN PCHAR                        pDnsDomainName,
    IN PBYTE                        pOsVersion,
    OUT PDWORD                      pdwSize,
    OUT PNTLM_CHALLENGE_MESSAGE*    ppChlngMsg
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwSize = 0;
    PNTLM_CHALLENGE_MESSAGE pMessage = NULL;
    PNTLM_SEC_BUFFER pTargetInfoSecBuffer= NULL;
    PNTLM_TARGET_INFO_BLOCK pTargetInfoBlock = NULL;
    DWORD dwTargetInfoSize = 0;
    PBYTE pBuffer = NULL;
    DWORD dwOptions = 0;
    PBYTE pTrav = NULL;

    // sanity checks
    if(!pNegMsg || !ppChlngMsg)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_NTLM_ERROR(dwError);
    }

    *ppChlngMsg = NULL;
    *pdwSize = 0;

    dwSize = sizeof(NTLM_CHALLENGE_MESSAGE);

    // calculate optional data size
    if(pOsVersion)
    {
        dwSize +=
            NTLM_LOCAL_CONTEXT_SIZE +
            sizeof(NTLM_SEC_BUFFER) +
            NTLM_WIN_SPOOF_SIZE;

        dwTargetInfoSize += sizeof(NTLM_TARGET_INFO_BLOCK);
    }
    else if(pServerName || pDomainName || pDnsServerName || pDnsDomainName)
    {
        dwSize +=
            NTLM_LOCAL_CONTEXT_SIZE +
            sizeof(NTLM_SEC_BUFFER);

        dwTargetInfoSize += sizeof(NTLM_TARGET_INFO_BLOCK);
        dwOptions |= NTLM_FLAG_TARGET_INFO;
    }

    if(pServerName)
    {
        dwTargetInfoSize += sizeof(NTLM_TARGET_INFO_BLOCK);
        dwTargetInfoSize += strlen(pServerName) * sizeof(WCHAR);

        if(pNegMsg->NtlmFlags & NTLM_FLAG_REQUEST_TARGET)
        {
            if(pNegMsg->NtlmFlags & NTLM_FLAG_UNICODE)
            {
                dwSize += strlen(pServerName) * sizeof(WCHAR);
            }
            else if(pNegMsg->NtlmFlags & NTLM_FLAG_OEM)
            {
                dwSize += strlen(pServerName);
            }
            else
            {
                // Something appears to be wrong with the negotiation message
                // passed in... it didn't specify a string type... bail
                dwError = LW_ERROR_INVALID_PARAMETER;
                BAIL_ON_NTLM_ERROR(dwError);
            }

            // Documentation indicates that the NTLM_FLAG_REQUEST_TARGET flag is
            // often set but doesn't have much meaning in a type 2 message (it's
            // for type 1 messages).  We'll propogate it for now when we have
            // target information to return (but we may remove it or make it
            // configurable in the future.
            dwOptions |= NTLM_FLAG_REQUEST_TARGET | NTLM_FLAG_TYPE_SERVER;
        }
    }

    if(pDomainName)
    {
        dwTargetInfoSize += sizeof(NTLM_TARGET_INFO_BLOCK);
        dwTargetInfoSize += strlen(pDomainName) * sizeof(WCHAR);
    }

    if(pDnsServerName)
    {
        dwTargetInfoSize += sizeof(NTLM_TARGET_INFO_BLOCK);
        dwTargetInfoSize += strlen(pDnsServerName) * sizeof(WCHAR);
    }

    if(pDnsDomainName)
    {
        dwTargetInfoSize += sizeof(NTLM_TARGET_INFO_BLOCK);
        dwTargetInfoSize += strlen(pDnsDomainName) * sizeof(WCHAR);
    }

    dwSize += dwTargetInfoSize;

    dwError = LsaAllocateMemory(dwSize, (PVOID*)(PVOID)&pMessage);
    BAIL_ON_NTLM_ERROR(dwError);

    // We need to build up the challenge options based on the negotiate options
    //
    if(pNegMsg->NtlmFlags & NTLM_FLAG_UNICODE)
    {
        dwOptions |= NTLM_FLAG_UNICODE;
    }
    else if(pNegMsg->NtlmFlags & NTLM_FLAG_OEM)
    {
        dwOptions |= NTLM_FLAG_OEM;
    }
    else
    {
        // bit of a sanity check, the negotiation message should have had at
        // least one of those flags set... if it didin't...
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_NTLM_ERROR(dwError);
    }

    // Another sanity check... we need to have at least NTLM or NTLM2 requested
    if(!(pNegMsg->NtlmFlags & NTLM_FLAG_NTLM) &&
       !(pNegMsg->NtlmFlags & NTLM_FLAG_NTLM2))
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_NTLM_ERROR(dwError);
    }

    // If the client wants to support a dummy signature, we will too
    if(pNegMsg->NtlmFlags & NTLM_FLAG_ALWAYS_SIGN)
    {
        dwOptions |= NTLM_FLAG_ALWAYS_SIGN;
    }

    // We will not set the following flags:
    // NTLM_FLAG_LOCAL_CALL - Indicates local authentication which we do not
    //                        provide yet.  We will not use the local context
    // NTLM_FLAG_TYPE_SHARE - The authentication target is a network share (?).
    //                        Odd.
    // NTLM_FLAG_TYPE_DOMAIN - I think we can always choose server.
    dwOptions |= (NTLM_FLAG_NTLM  |  // we support NTLM
                  NTLM_FLAG_NTLM2 |  // we support NTLM2
                  NTLM_FLAG_128   |  // we support 128 bit encryption
                  NTLM_FLAG_56    ); // we support 56 bit encryption

    // Data is checked and memory is allocated; fill in the structure
    //
    memcpy(
        &(pMessage->NtlmSignature),
        NTLM_SIGNATURE,
        NTLM_SIGNATURE_SIZE
        );

    pMessage->MessageType = NTLM_CHALLENGE_MSG;

    pMessage->NtlmFlags = dwOptions;

    if(pMessage->NtlmFlags & NTLM_FLAG_REQUEST_TARGET)
    {
        if(pServerName)
        {
            if(pMessage->NtlmFlags & NTLM_FLAG_UNICODE)
            {
                pMessage->Target.usLength =
                    strlen(pServerName) * sizeof(WCHAR);
            }
            else
            {
                pMessage->Target.usLength = strlen(pServerName);
            }

            pMessage->Target.usMaxLength = pMessage->Target.usLength;
        }
    }
    else
    {
        pMessage->Target.usLength = 0;
        pMessage->Target.usMaxLength = 0;
        pMessage->Target.dwOffset = 0;
    }

    dwError = NtlmGetRandomBuffer(
        pMessage->Challenge,
        NTLM_CHALLENGE_SIZE
        );

    BAIL_ON_NTLM_ERROR(dwError);

    // Main structure has been filled, now fill in optional data
    pBuffer = (PBYTE)pMessage + sizeof(NTLM_CHALLENGE_MESSAGE);

    if(pOsVersion || pMessage->NtlmFlags & NTLM_FLAG_TARGET_INFO)
    {
        // We have to fill in a local context which we will never use
        // ... so make it all zeros
        memset(pBuffer, 0, NTLM_LOCAL_CONTEXT_SIZE);
        pBuffer += NTLM_LOCAL_CONTEXT_SIZE;

        if(pMessage->NtlmFlags & NTLM_FLAG_TARGET_INFO)
        {
            pTargetInfoSecBuffer = (PNTLM_SEC_BUFFER)pBuffer;

            pTargetInfoSecBuffer->usLength = dwTargetInfoSize;
            pTargetInfoSecBuffer->usMaxLength = pTargetInfoSecBuffer->usLength;

            pBuffer += sizeof(NTLM_SEC_BUFFER);
        }

        if(pOsVersion)
        {
            memcpy(pBuffer, pOsVersion, NTLM_WIN_SPOOF_SIZE);

            pBuffer += NTLM_WIN_SPOOF_SIZE;
        }
    }

    if(pMessage->Target.usLength)
    {
        pMessage->Target.dwOffset = pBuffer - (PBYTE)pMessage;

        if(pMessage->NtlmFlags & NTLM_FLAG_OEM)
        {
            memcpy(pBuffer, pServerName, pMessage->Target.usLength);
            pBuffer += pMessage->Target.usLength;
        }
        else
        {
            pTrav = (PBYTE)pServerName;

            while(*pTrav)
            {
                *pBuffer = *pTrav;
                pBuffer++;
                pBuffer++;

                pTrav++;
            }
        }
    }

    if(pTargetInfoSecBuffer->usLength)
    {
        // Remember, target block information is ALWAYS in UNICODE

        if(pServerName)
        {
            pTargetInfoBlock = (PNTLM_TARGET_INFO_BLOCK)pBuffer;

            pTargetInfoBlock->sLength =
                strlen(pServerName) * sizeof(WCHAR);

            pTargetInfoBlock->sType = NTLM_TIB_SERVER_NAME;

            pTrav = (PBYTE)pServerName;

            pBuffer = (PBYTE)pTargetInfoBlock + sizeof(NTLM_TARGET_INFO_BLOCK);

            while(*pTrav)
            {
                *pBuffer = *pTrav;

                pBuffer++;
                pBuffer++;

                pTrav++;
            }
        }

        if(pDomainName)
        {
            pTargetInfoBlock = (PNTLM_TARGET_INFO_BLOCK)pBuffer;

            pTargetInfoBlock->sLength =
                strlen(pDomainName) * sizeof(WCHAR);

            pTargetInfoBlock->sType = NTLM_TIB_DOMAIN_NAME;

            pTrav = (PBYTE)pDomainName;

            pBuffer = (PBYTE)pTargetInfoBlock + sizeof(NTLM_TARGET_INFO_BLOCK);

            while(*pTrav)
            {
                *pBuffer = *pTrav;

                pBuffer++;
                pBuffer++;

                pTrav++;
            }
        }

        if(pDnsServerName)
        {
            pTargetInfoBlock = (PNTLM_TARGET_INFO_BLOCK)pBuffer;

            pTargetInfoBlock->sLength =
                strlen(pDnsServerName) * sizeof(WCHAR);

            pTargetInfoBlock->sType = NTLM_TIB_DNS_SERVER_NAME;

            pTrav = (PBYTE)pDnsServerName;

            pBuffer = (PBYTE)pTargetInfoBlock + sizeof(NTLM_TARGET_INFO_BLOCK);

            while(*pTrav)
            {
                *pBuffer = *pTrav;

                pBuffer++;
                pBuffer++;

                pTrav++;
            }
        }

        if(pDnsDomainName)
        {
            pTargetInfoBlock = (PNTLM_TARGET_INFO_BLOCK)pBuffer;

            pTargetInfoBlock->sLength =
                strlen(pDnsDomainName) * sizeof(WCHAR);

            pTargetInfoBlock->sType = NTLM_TIB_DNS_DOMAIN_NAME;

            pTrav = (PBYTE)pDnsDomainName;

            pBuffer = (PBYTE)pTargetInfoBlock + sizeof(NTLM_TARGET_INFO_BLOCK);

            while(*pTrav)
            {
                *pBuffer = *pTrav;

                pBuffer++;
                pBuffer++;

                pTrav++;
            }
        }

        pTargetInfoBlock->sLength = 0;
        pTargetInfoBlock->sType = NTLM_TIB_TERMINATOR;
    }

cleanup:
    *pdwSize = dwSize;
    *ppChlngMsg = pMessage;
    return dwError;
error:
    dwSize = 0;
    LW_SAFE_FREE_MEMORY(pMessage);
    goto cleanup;
}

/******************************************************************************/
DWORD
NtlmCreateResponseMessage(
    IN PNTLM_CHALLENGE_MESSAGE  pChlngMsg,
    IN PCHAR                    pAuthTargetName,
    IN PCSTR                    pUserName,
    IN PCHAR                    pWorkstation,
    IN PBYTE                    pOsVersion,
    IN PCSTR                    pPassword,
    IN DWORD                    dwNtRespType,
    IN DWORD                    dwLmRespType,
    OUT PDWORD                  pdwSize,
    OUT PNTLM_RESPONSE_MESSAGE* ppRespMsg
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwSize = 0;
    PNTLM_RESPONSE_MESSAGE pMessage = NULL;
    PBYTE pBuffer = NULL;
    PNTLM_SEC_BUFFER pSessionKey = NULL;
    DWORD dwNtMsgSize = 0;
    DWORD dwLmMsgSize = 0;
    DWORD dwAuthTrgtNameSize = 0;
    DWORD dwUserNameSize = 0;
    DWORD dwWorkstationSize = 0;
    PBYTE pTrav = NULL;

    // sanity checks
    if(!pChlngMsg || !ppRespMsg)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_NTLM_ERROR(dwError);
    }

    *ppRespMsg = NULL;
    *pdwSize = 0;

    dwAuthTrgtNameSize = strlen(pAuthTargetName);
    dwUserNameSize = strlen(pUserName);
    dwWorkstationSize = strlen(pWorkstation);

    dwSize += dwAuthTrgtNameSize +
              dwUserNameSize +
              dwWorkstationSize;

    if(pChlngMsg->NtlmFlags & NTLM_FLAG_UNICODE)
    {
        dwSize *= sizeof(WCHAR);
    }

    dwSize += sizeof(NTLM_RESPONSE_MESSAGE);

    // calculate the response size...
    dwError = NtlmCalculateResponseSize(
        pChlngMsg,
        dwNtRespType,
        &dwNtMsgSize
        );
    BAIL_ON_NTLM_ERROR(dwError);

    dwSize += dwNtMsgSize;

    dwError = NtlmCalculateResponseSize(
        pChlngMsg,
        dwLmRespType,
        &dwLmMsgSize
        );
    BAIL_ON_NTLM_ERROR(dwError);

    dwSize += dwLmMsgSize;

    // we're just going to send an empty session key for now... we may support
    // this option later
    dwSize += sizeof(NTLM_SEC_BUFFER);

    // we're going to send our flags as well
    dwSize += sizeof(pChlngMsg->NtlmFlags);

    if(pOsVersion)
    {
        dwSize += NTLM_WIN_SPOOF_SIZE;
    }

    dwError = LsaAllocateMemory(dwSize, (PVOID*)(PVOID)&pMessage);
    BAIL_ON_NTLM_ERROR(dwError);

    // Data is checked and memory is allocated; fill in the structure
    //
    memcpy(&(pMessage->NtlmSignature), NTLM_SIGNATURE, NTLM_SIGNATURE_SIZE);

    pMessage->MessageType = NTLM_RESPONSE_MSG;

    pMessage->LmResponse.usLength = dwNtMsgSize;
    pMessage->LmResponse.usMaxLength = pMessage->LmResponse.usLength;

    pMessage->NtResponse.usLength = dwNtMsgSize;
    pMessage->NtResponse.usMaxLength = pMessage->NtResponse.usLength;

    if(pChlngMsg->NtlmFlags & NTLM_FLAG_UNICODE)
    {
        pMessage->AuthTargetName.usLength =
            dwAuthTrgtNameSize * sizeof(WCHAR);
        pMessage->AuthTargetName.usMaxLength =
            pMessage->AuthTargetName.usLength;

        pMessage->UserName.usLength =
            dwUserNameSize * sizeof(WCHAR);
        pMessage->UserName.usMaxLength =
            pMessage->UserName.usLength;

        pMessage->Workstation.usLength =
            dwWorkstationSize * sizeof(WCHAR);
        pMessage->Workstation.usMaxLength =
            pMessage->Workstation.usLength;
    }
    else
    {
        pMessage->AuthTargetName.usLength = dwAuthTrgtNameSize;
        pMessage->AuthTargetName.usMaxLength =
            pMessage->AuthTargetName.usLength;

        pMessage->UserName.usLength = dwUserNameSize;
        pMessage->UserName.usMaxLength =
            pMessage->UserName.usLength;

        pMessage->Workstation.usLength = dwWorkstationSize;
        pMessage->Workstation.usMaxLength =
            pMessage->Workstation.usLength;
    }

    // We've filled in the main structure, now add optional data at the end
    pBuffer = (PBYTE)pMessage + sizeof(NTLM_RESPONSE_MESSAGE);

    pSessionKey = (PNTLM_SEC_BUFFER)pBuffer;

    pSessionKey->usLength = 0;
    pSessionKey->usMaxLength = pSessionKey->usLength;

    pBuffer += sizeof(NTLM_SEC_BUFFER);

    memcpy(pBuffer, &(pChlngMsg->NtlmFlags), sizeof(pChlngMsg->NtlmFlags));

    pBuffer += sizeof(pChlngMsg->NtlmFlags);

    if(pOsVersion)
    {
        memcpy(pBuffer, pOsVersion, NTLM_WIN_SPOOF_SIZE);
        pBuffer += NTLM_WIN_SPOOF_SIZE;
    }

    pMessage->LmResponse.dwOffset = pBuffer - (PBYTE)pMessage;

    dwError = NtlmBuildResponse(
        pChlngMsg,
        pPassword,
        dwLmRespType,
        dwLmMsgSize,
        pBuffer
        );

    BAIL_ON_NTLM_ERROR(dwError);

    pBuffer += pMessage->LmResponse.usLength;

    pMessage->NtResponse.dwOffset = pBuffer - (PBYTE)pMessage;

    dwError = NtlmBuildResponse(
        pChlngMsg,
        pPassword,
        dwNtRespType,
        dwNtMsgSize,
        pBuffer
        );

    BAIL_ON_NTLM_ERROR(dwError);

    pBuffer += pMessage->NtResponse.usLength;

    pMessage->AuthTargetName.dwOffset = pBuffer - (PBYTE)pMessage;
    pTrav = (PBYTE)pAuthTargetName;
    while(*pTrav)
    {
        *pBuffer = *pTrav;
        pBuffer++;
        if(pChlngMsg->NtlmFlags & NTLM_FLAG_UNICODE)
        {
            pBuffer++;
        }
        pTrav++;
    }

    pMessage->UserName.dwOffset = pBuffer - (PBYTE)pMessage;
    pTrav = (PBYTE)pUserName;
    while(*pTrav)
    {
        *pBuffer = *pTrav;
        pBuffer++;
        if(pChlngMsg->NtlmFlags & NTLM_FLAG_UNICODE)
        {
            pBuffer++;
        }
        pTrav++;
    }

    pMessage->Workstation.dwOffset = pBuffer - (PBYTE)pMessage;
    pTrav = (PBYTE)pWorkstation;
    while(*pTrav)
    {
        *pBuffer = *pTrav;
        pBuffer++;
        if(pChlngMsg->NtlmFlags & NTLM_FLAG_UNICODE)
        {
            pBuffer++;
        }
        pTrav++;
    }

    pSessionKey->dwOffset = pBuffer - (PBYTE)pMessage;

cleanup:
    *ppRespMsg = pMessage;
    *pdwSize = dwSize;
    return dwError;
error:
    LW_SAFE_FREE_MEMORY(pMessage);
    dwSize = 0;
    goto cleanup;
}

/******************************************************************************/
DWORD
NtlmValidatResponseMessage(
    IN PNTLM_RESPONSE_MESSAGE pAuthMsg
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;

    // sanity check
    if(!pAuthMsg)
    {
    }

    return dwError;
}

/******************************************************************************/
DWORD
NtlmBuildResponse(
    IN PNTLM_CHALLENGE_MESSAGE  pChlngMsg,
    IN PCSTR                    pPassword,
    IN DWORD                    dwResponseType,
    IN DWORD                    dwBufferSize,
    OUT PBYTE                   pBuffer
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;

    if(!pChlngMsg)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_NTLM_ERROR(dwError);
    }

    switch(dwResponseType)
    {
    case NTLM_RESPONSE_TYPE_LM:
        {
            dwError = NtlmBuildLmResponse(
                pChlngMsg,
                pPassword,
                dwBufferSize,
                pBuffer
                );
            BAIL_ON_NTLM_ERROR(dwError);
        }
        break;
    case NTLM_RESPONSE_TYPE_LMv2:
        {
            dwError = NtlmBuildLmV2Response();
            BAIL_ON_NTLM_ERROR(dwError);
        }
        break;
    case NTLM_RESPONSE_TYPE_NTLM:
        {
            dwError = NtlmBuildNtlmResponse(
                pChlngMsg,
                pPassword,
                dwBufferSize,
                pBuffer
                );
            BAIL_ON_NTLM_ERROR(dwError);
        }
        break;
    case NTLM_RESPONSE_TYPE_NTLMv2:
        {
            dwError = NtlmBuildNtlmV2Response();
            BAIL_ON_NTLM_ERROR(dwError);
        }
        break;
    case NTLM_RESPONSE_TYPE_NTLM2:
        {
            dwError = NtlmBuildNtlm2Response();
            BAIL_ON_NTLM_ERROR(dwError);
        }
        break;
    case NTLM_RESPONSE_TYPE_ANONYMOUS:
        {
            dwError = NtlmBuildAnonymousResponse();
            BAIL_ON_NTLM_ERROR(dwError);
        }
        break;
    default:
        {
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_NTLM_ERROR(dwError);
        }
    }

cleanup:
    return dwError;
error:
    goto cleanup;
}

/******************************************************************************/
DWORD
NtlmBuildLmResponse(
    IN PNTLM_CHALLENGE_MESSAGE  pChlngMsg,
    IN PCSTR                    pPassword,
    IN DWORD                    dwLength,
    OUT PBYTE                   pResponse
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwIndex = 0;
    DWORD dwPasswordLen = 0;
    BYTE LmHash[NTLM_LM_HASH_SIZE] = {0};
    ULONG64 ulKey1 = 0;
    ULONG64 ulKey2 = 0;
    ULONG64 ulKey3 = 0;
    DES_key_schedule DesKeySchedule;

    if(dwLength != NTLM_RESPONSE_SIZE_LM)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_NTLM_ERROR(dwError);
    }

    dwPasswordLen = strlen(pPassword);

    // if password is less than 15 characters, create LM hash, otherwise, the
    // hash is set to zero.
    if(dwPasswordLen <= NTLM_LM_MAX_PASSWORD_SIZE)
    {
        // convert the password to upper case
        for(dwIndex = 0; dwIndex < NTLM_LM_MAX_PASSWORD_SIZE; dwIndex++)
        {
            LmHash[0] = toupper(pPassword[dwIndex]);

            if(!pPassword[dwIndex])
            {
                break;
            }
        }

        memcpy(&ulKey1, &LmHash[0], 7);
        memcpy(&ulKey2, &LmHash[7], 7);

        NtlmSetParityBit(&ulKey1);
        NtlmSetParityBit(&ulKey2);

        DES_set_key_unchecked((const_DES_cblock*)&ulKey1, &DesKeySchedule);
        DES_ecb_encrypt(
            (const_DES_cblock *)NTLM_LM_DES_STRING,
            (DES_cblock*)&LmHash[0],
            &DesKeySchedule,
            DES_ENCRYPT
            );

        DES_set_key_unchecked((const_DES_cblock*)&ulKey2, &DesKeySchedule);
        DES_ecb_encrypt(
            (const_DES_cblock *)NTLM_LM_DES_STRING,
            (DES_cblock*)&LmHash[8],
            &DesKeySchedule,
            DES_ENCRYPT
            );
    }

    memcpy(&ulKey1, &LmHash[0],  7);
    memcpy(&ulKey2, &LmHash[7],  7);
    memcpy(&ulKey3, &LmHash[14], 2);

    NtlmSetParityBit(&ulKey1);
    NtlmSetParityBit(&ulKey2);
    NtlmSetParityBit(&ulKey3);

    DES_set_key_unchecked((const_DES_cblock*)&ulKey1, &DesKeySchedule);
    DES_ecb_encrypt(
        (const_DES_cblock *)pChlngMsg->Challenge,
        (DES_cblock*)&pResponse[0],
        &DesKeySchedule,
        DES_ENCRYPT
        );

    DES_set_key_unchecked((const_DES_cblock*)&ulKey2, &DesKeySchedule);
    DES_ecb_encrypt(
        (const_DES_cblock *)pChlngMsg->Challenge,
        (DES_cblock*)&pResponse[8],
        &DesKeySchedule,
        DES_ENCRYPT
        );

    DES_set_key_unchecked((const_DES_cblock*)&ulKey3, &DesKeySchedule);
    DES_ecb_encrypt(
        (const_DES_cblock *)pChlngMsg->Challenge,
        (DES_cblock*)&pResponse[16],
        &DesKeySchedule,
        DES_ENCRYPT
        );

cleanup:
    return dwError;
error:
    goto cleanup;
}

/******************************************************************************/
DWORD
NtlmBuildNtlmResponse(
    IN PNTLM_CHALLENGE_MESSAGE  pChlngMsg,
    IN PCSTR                    pPassword,
    IN DWORD                    dwLength,
    OUT PBYTE                   pResponse
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    BYTE MD4Digest[MD4_DIGEST_LENGTH];
    ULONG64 ulKey1 = 0;
    ULONG64 ulKey2 = 0;
    ULONG64 ulKey3 = 0;
    DES_key_schedule DesKeySchedule;
    DWORD dwTempPassSize = strlen(pPassword) * sizeof(WCHAR);
    PWCHAR pwcTempPass = NULL;

    memset(&DesKeySchedule, 0, sizeof(DES_key_schedule));

    if(dwLength != NTLM_RESPONSE_SIZE_NTLM)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_NTLM_ERROR(dwError);
    }

    dwError = LsaAllocateMemory(dwTempPassSize, (PVOID*)(PVOID)&pwcTempPass);
    BAIL_ON_NTLM_ERROR(dwError);

    while(*pPassword)
    {
        *pwcTempPass = *pPassword;
        pwcTempPass++;
        pPassword++;
    }

    pwcTempPass = (PWCHAR)((PBYTE)pwcTempPass - dwTempPassSize);

    dwError = NtlmCreateMD4Digest(
        (PBYTE)pwcTempPass,
        dwTempPassSize,
        MD4Digest
        );

    BAIL_ON_NTLM_ERROR(dwError);

    memcpy(&ulKey1, &MD4Digest[0], 7);
    memcpy(&ulKey2, &MD4Digest[7], 7);
    memcpy(&ulKey3, &MD4Digest[14], 2);

    NtlmSetParityBit(&ulKey1);
    NtlmSetParityBit(&ulKey2);
    NtlmSetParityBit(&ulKey3);

    DES_set_key_unchecked((const_DES_cblock*)&ulKey1, &DesKeySchedule);
    DES_ecb_encrypt(
        (const_DES_cblock *)pChlngMsg->Challenge,
        (DES_cblock*)&pResponse[0],
        &DesKeySchedule,
        DES_ENCRYPT
        );

    DES_set_key_unchecked((const_DES_cblock*)&ulKey2, &DesKeySchedule);
    DES_ecb_encrypt(
        (const_DES_cblock *)pChlngMsg->Challenge,
        (DES_cblock*)&pResponse[8],
        &DesKeySchedule,
        DES_ENCRYPT
        );

    DES_set_key_unchecked((const_DES_cblock*)&ulKey3, &DesKeySchedule);
    DES_ecb_encrypt(
        (const_DES_cblock *)pChlngMsg->Challenge,
        (DES_cblock*)&pResponse[16],
        &DesKeySchedule,
        DES_ENCRYPT
        );

cleanup:
    if(pwcTempPass)
    {
        LsaFreeMemory(pwcTempPass);
    }
    return dwError;
error:
    goto cleanup;
}

/******************************************************************************/
DWORD
NtlmBuildNtlmV2Response(
    VOID
    )
{
    DWORD dwError = LW_ERROR_NOT_SUPPORTED;
    return dwError;
}

/******************************************************************************/
DWORD
NtlmBuildLmV2Response(
    VOID
    )
{
    DWORD dwError = LW_ERROR_NOT_SUPPORTED;
    return dwError;
}

/******************************************************************************/
DWORD
NtlmBuildNtlm2Response(
    VOID
    )
{
    DWORD dwError = LW_ERROR_NOT_SUPPORTED;
    return dwError;
}

/******************************************************************************/
DWORD
NtlmBuildAnonymousResponse(
    VOID
    )
{
    DWORD dwError = LW_ERROR_NOT_SUPPORTED;
    return dwError;
}

/******************************************************************************/
DWORD
NtlmCalculateResponseSize(
    IN PNTLM_CHALLENGE_MESSAGE  pChlngMsg,
    IN DWORD                    dwResponseType,
    OUT PDWORD                  pdwSize
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwSize = 0;

    if(!pChlngMsg)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_NTLM_ERROR(dwError);
    }

    *pdwSize = 0;

    switch(dwResponseType)
    {
    case NTLM_RESPONSE_TYPE_LM:
    case NTLM_RESPONSE_TYPE_LMv2:
    case NTLM_RESPONSE_TYPE_NTLM2:
    case NTLM_RESPONSE_TYPE_NTLM:
        {
            // All 4 of these messages are the same size as an NTLM message
            dwSize = NTLM_RESPONSE_SIZE_NTLM;
        }
        break;
    case NTLM_RESPONSE_TYPE_ANONYMOUS:
        {
            dwSize = NTLM_RESPONSE_SIZE_ANONYMOUS;
        }
        break;
    case NTLM_RESPONSE_TYPE_NTLMv2:
        {
            // This is the only one that needs to be calculated out
            dwError = NtlmCalculateNtlmV2ResponseSize(pChlngMsg, &dwSize);
        }
        break;
    default:
        {
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_NTLM_ERROR(dwError);
        }
    }

cleanup:
    *pdwSize = dwSize;
    return dwError;
error:
    dwSize = 0;
    goto cleanup;
}

/******************************************************************************/
DWORD
NtlmCalculateNtlmV2ResponseSize(
    IN PNTLM_CHALLENGE_MESSAGE  pChlngMsg,
    OUT PDWORD                  pdwSize
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    return dwError;
}

/******************************************************************************/
DWORD
NtlmCreateMD4Digest(
    PBYTE   pBuffer,
    DWORD   dwBufferLen,
    BYTE    MD4Digest[MD4_DIGEST_LENGTH]
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    MD4_CTX Md4Ctx;

    dwError = MD4_Init(&Md4Ctx);

    if(dwError != 1)
    {
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_NTLM_ERROR(dwError);
    }

    dwError = MD4_Update(&Md4Ctx, pBuffer, dwBufferLen);

    if(dwError != 1)
    {
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_NTLM_ERROR(dwError);
    }

    dwError = MD4_Final(MD4Digest, &Md4Ctx);

    if(dwError != 1)
    {
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_NTLM_ERROR(dwError);
    }

cleanup:
    return dwError;
error:
    goto cleanup;
}

/******************************************************************************/
DWORD
NtlmSetParityBit(
    PULONG64 pKey
    )
{
    *pKey = (*pKey & 0xFFFFFFFFFFFFFF00) | ((*pKey & ((ULONG64)7 <<  8)) >> 7);
    *pKey = (*pKey & 0xFFFFFFFFFFFF00FF) | ((*pKey & ((ULONG64)7 << 15)) >> 6);
    *pKey = (*pKey & 0xFFFFFFFFFF00FFFF) | ((*pKey & ((ULONG64)7 << 22)) >> 5);
    *pKey = (*pKey & 0xFFFFFFFF00FFFFFF) | ((*pKey & ((ULONG64)7 << 29)) >> 4);
    *pKey = (*pKey & 0xFFFFFF00FFFFFFFF) | ((*pKey & ((ULONG64)7 << 36)) >> 3);
    *pKey = (*pKey & 0xFFFF00FFFFFFFFFF) | ((*pKey & ((ULONG64)7 << 43)) >> 2);
    *pKey = (*pKey & 0xFF00FFFFFFFFFFFF) | ((*pKey & ((ULONG64)7 << 50)) >> 1);

    DES_set_odd_parity((DES_cblock*)pKey);

    return LW_ERROR_SUCCESS;
}
