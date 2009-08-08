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
        BAIL_ON_LW_ERROR(dwError);
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
            pContextListEntry = LsaListRemoveHead(
                &gContextState.LsaContextList);

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
    IN PLSA_CONTEXT pContext,
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
    IN LSA_CONTEXT_HANDLE ContextHandle,
    OUT OPTIONAL PNTLM_STATE pNtlmState,
    OUT OPTIONAL PVOID* ppMessage,
    OUT OPTIONAL PDWORD pdwMessageSize,
    OUT OPTIONAL PNTLM_CRED_HANDLE pCredHandle
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
NtlmCreateContext(
    IN PNTLM_CRED_HANDLE pCredHandle,
    OUT PLSA_CONTEXT* ppNtlmContext
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PLSA_CONTEXT pContext = NULL;

    if(!ppNtlmContext)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LW_ERROR(dwError);
    }

    *ppNtlmContext = NULL;

    dwError = LsaAllocateMemory(
        sizeof(LSA_CONTEXT),
        (PVOID*)(PVOID)&pContext
        );

    BAIL_ON_LW_ERROR(dwError);

    pContext->NtlmState = NtlmStateBlank;
    pContext->nRefCount = 1;

    pContext->CredHandle = *pCredHandle;
    NtlmReferenceCredential(pContext->CredHandle);

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
    NtlmReleaseCredential(pContext->CredHandle);

    memset(pContext->pMessage, 0, pContext->dwMessageSize);
    LW_SAFE_FREE_MEMORY(pContext->pMessage);

    LW_SAFE_FREE_MEMORY(pContext);

    return;
}

/******************************************************************************/
DWORD
NtlmGetMessageFromSecBufferDesc(
    IN PSecBufferDesc pSecBufferDesc,
    OUT PDWORD pdwMessageSize,
    OUT PVOID *ppMessage
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PSecBuffer pSecBuffer = NULL;
    DWORD dwMessageSize = 0;
    PBYTE pMessage = NULL;

    *pdwMessageSize = 0;
    *ppMessage = NULL;

    if(!pSecBufferDesc)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LW_ERROR(dwError);
    }

    // Transfered context tokens only contain one SecBuffer and are tagged as
    // tokens... verify
    if(pSecBufferDesc->cBuffers != 1 || !(pSecBufferDesc->pBuffers))
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LW_ERROR(dwError);
    }

    pSecBuffer = pSecBufferDesc->pBuffers;

    if(pSecBuffer->BufferType != SECBUFFER_TOKEN ||
       pSecBuffer->cbBuffer == 0)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LW_ERROR(dwError);
    }

    pMessage = pSecBuffer->pvBuffer;
    dwMessageSize = pSecBuffer->cbBuffer;

cleanup:
    *pdwMessageSize = dwMessageSize;
    *ppMessage = pMessage;

    return dwError;

error:
    dwMessageSize = 0;
    pMessage = NULL;
    goto cleanup;
}

/******************************************************************************/
DWORD
NtlmCopyContextToSecBufferDesc(
    IN PLSA_CONTEXT pNtlmContext,
    IN OUT PSecBufferDesc pSecBufferDesc
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PSecBuffer pSecBuffer = NULL;
    PBYTE pBuffer = NULL;

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

    pSecBuffer->cbBuffer = pNtlmContext->dwMessageSize;
    dwError = LsaAllocateMemory(pSecBuffer->cbBuffer, OUT_PPVOID(&pBuffer));
    BAIL_ON_LW_ERROR(dwError);

    memcpy(pBuffer, pNtlmContext->pMessage, pSecBuffer->cbBuffer);

    pSecBuffer->pvBuffer = (PVOID)pBuffer;

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
        BAIL_ON_LW_ERROR(dwError);
    }

    nFileDesc = open(NTLM_URANDOM_DEV, O_RDONLY);
    if(-1 == nFileDesc)
    {
        nFileDesc = open(NTLM_RANDOM_DEV, O_RDONLY);
        if(-1 == nFileDesc)
        {
            dwError = LW_ERROR_INTERNAL; //LwMapErrnoToLwError(errno);
            BAIL_ON_LW_ERROR(dwError);
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
DWORD
NtlmCreateNegotiateMessage(
    IN DWORD dwOptions,
    IN PCSTR pDomain,
    IN PCSTR pWorkstation,
    IN PBYTE pOsVersion,
    OUT PDWORD pdwSize,
    OUT PNTLM_NEGOTIATE_MESSAGE* ppNegMsg
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
        BAIL_ON_LW_ERROR(dwError);
    }

    *ppNegMsg = NULL;
    *pdwSize = 0;

    if(dwOptions & NTLM_FLAG_DOMAIN)
    {
        if(!pDomain)
        {
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_LW_ERROR(dwError);
        }
        else
        {
            dwSize += strlen(pDomain);
        }
    }

    if(dwOptions & NTLM_FLAG_WORKSTATION)
    {
        if(!pWorkstation)
        {
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_LW_ERROR(dwError);
        }
        else
        {
            dwSize += strlen(pWorkstation);
        }
    }

    dwSize += sizeof(NTLM_NEGOTIATE_MESSAGE);

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
    BAIL_ON_LW_ERROR(dwError);

    // Data is checked and memory is allocated; fill in the structure
    //
    memcpy(
        &pMessage->NtlmSignature,
        NTLM_NETWORK_SIGNATURE,
        NTLM_NETWORK_SIGNATURE_SIZE);
    pMessage->MessageType = NTLM_NEGOTIATE_MSG;
    pMessage->NtlmFlags = dwOptions;

    // Start writing optional information (if there is any) after the structure
    pBuffer = (PBYTE)pMessage + sizeof(NTLM_NEGOTIATE_MESSAGE);

    // If you have OS info, you HAVE to at least adjust the pointers past the
    // domain secbuffer (even if you don't fill it in with data).
    if(dwOptions & NTLM_FLAG_DOMAIN ||
        pOsVersion)
    {
        if(pDomain && (dwOptions & NTLM_FLAG_DOMAIN))
        {
            pDomainSecBuffer = (PNTLM_SEC_BUFFER)pBuffer;

            // The Domain name is ALWAYS given as an OEM (i.e. ASCII) string
            pDomainSecBuffer->usLength = strlen(pDomain);
            pDomainSecBuffer->usMaxLength = pDomainSecBuffer->usLength;
        }

        pBuffer += sizeof(NTLM_SEC_BUFFER);
    }

    // If you have a domain or OS info, you HAVE to at least adjust the pointers
    // past the workstation secbuffer (even if you don't fill it in with data)
    if(dwOptions & NTLM_FLAG_WORKSTATION ||
        dwOptions & NTLM_FLAG_DOMAIN ||
        pOsVersion)
    {
        if(pWorkstation && (dwOptions & NTLM_FLAG_WORKSTATION))
        {
            pWorkstationSecBuffer = (PNTLM_SEC_BUFFER)pBuffer;

            // The Workstation name is also ALWAYS given as an OEM string
            pWorkstationSecBuffer->usLength = strlen(pWorkstation);
            pWorkstationSecBuffer->usMaxLength = pWorkstationSecBuffer->usLength;
        }

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
        pBuffer += pDomainSecBuffer->usLength;
    }

    LW_ASSERT(pBuffer == (PBYTE)pMessage + dwSize);

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
    IN PNTLM_NEGOTIATE_MESSAGE pNegMsg,
    IN PCSTR pServerName,
    IN PCSTR pDomainName,
    IN PCSTR pDnsServerName,
    IN PCSTR pDnsDomainName,
    IN PBYTE pOsVersion,
    OUT PDWORD pdwSize,
    OUT PNTLM_CHALLENGE_MESSAGE* ppChlngMsg
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwSize = 0;
    PNTLM_CHALLENGE_MESSAGE pMessage = NULL;
    PNTLM_SEC_BUFFER pTargetInfoSecBuffer= NULL;
    PNTLM_TARGET_INFO_BLOCK pTargetInfoBlock = NULL;
    DWORD dwTargetInfoSize = 0;
    DWORD dwTargetNameSize = 0;
    PBYTE pBuffer = NULL;
    DWORD dwOptions = 0;
    PBYTE pTrav = NULL;

    // sanity checks
    if(!pNegMsg || !ppChlngMsg)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LW_ERROR(dwError);
    }

    *ppChlngMsg = NULL;
    *pdwSize = 0;

    dwSize = sizeof(NTLM_CHALLENGE_MESSAGE);

    // sanity check... we need to have at least NTLM or NTLM2 requested
    if(!(pNegMsg->NtlmFlags & NTLM_FLAG_NTLM) &&
       !(pNegMsg->NtlmFlags & NTLM_FLAG_NTLM2))
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LW_ERROR(dwError);
    }

    // We need to build up the challenge options based on the negotiate options.
    // We *must* have either unicode or ansii string set
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
        BAIL_ON_LW_ERROR(dwError);
    }

    // calculate optional data size
    if(pOsVersion)
    {
        dwSize +=
            NTLM_LOCAL_CONTEXT_SIZE +   // NTLM context (for local auth)
            sizeof(NTLM_SEC_BUFFER) +   // For target information block
            NTLM_WIN_SPOOF_SIZE;        // Win version info

        if(gbUseNtlmV2)
        {
            // This is for the terminating target info block
            dwTargetInfoSize += sizeof(NTLM_TARGET_INFO_BLOCK);

            dwOptions |= NTLM_FLAG_TARGET_INFO;
        }
    }
    else if(gbUseNtlmV2)
    {
        // This is the same as the 'if' statement above, minus accounting for
        // space for the OS version spoof.
        dwSize +=
            NTLM_LOCAL_CONTEXT_SIZE +
            sizeof(NTLM_SEC_BUFFER);

        dwTargetInfoSize += sizeof(NTLM_TARGET_INFO_BLOCK);
        dwOptions |= NTLM_FLAG_TARGET_INFO;
    }

    // Allocate space in the target information block for each piece of target
    // information we have.
    if(gbUseNtlmV2)
    {
        // Target information block info is always in unicode format.
        if(!LW_IS_NULL_OR_EMPTY_STR(pServerName))
        {
            dwTargetInfoSize += sizeof(NTLM_TARGET_INFO_BLOCK);
            dwTargetInfoSize += strlen(pServerName) * sizeof(WCHAR);
        }
        if(!LW_IS_NULL_OR_EMPTY_STR(pDomainName))
        {
            dwTargetInfoSize += sizeof(NTLM_TARGET_INFO_BLOCK);
            dwTargetInfoSize += strlen(pDomainName) * sizeof(WCHAR);
        }
        if(!LW_IS_NULL_OR_EMPTY_STR(pDnsServerName))
        {
            dwTargetInfoSize += sizeof(NTLM_TARGET_INFO_BLOCK);
            dwTargetInfoSize += strlen(pDnsServerName) * sizeof(WCHAR);
        }
        if(!LW_IS_NULL_OR_EMPTY_STR(pDnsDomainName))
        {
            dwTargetInfoSize += sizeof(NTLM_TARGET_INFO_BLOCK);
            dwTargetInfoSize += strlen(pDnsDomainName) * sizeof(WCHAR);
        }

        dwSize += dwTargetInfoSize;
    }

    if(pNegMsg->NtlmFlags & NTLM_FLAG_REQUEST_TARGET)
    {
        // To determine what name will be returned we check in this order:
        // 1). Domain name
        // 2). Server name
        //
        // For now, we never set the type to share (since we don't really know
        // what that means).
        if(!LW_IS_NULL_OR_EMPTY_STR(pDomainName))
        {
            dwTargetNameSize = strlen(pDomainName);
            dwOptions |= NTLM_FLAG_TYPE_DOMAIN;
        }
        else if(!LW_IS_NULL_OR_EMPTY_STR(pServerName))
        {
            dwTargetNameSize = strlen(pServerName);
            dwOptions |= NTLM_FLAG_TYPE_SERVER;
        }

        if(pNegMsg->NtlmFlags & NTLM_FLAG_UNICODE)
        {
            dwTargetNameSize *= sizeof(WCHAR);
        }

        // Documentation indicates that the NTLM_FLAG_REQUEST_TARGET flag is
        // often set but doesn't have much meaning in a type 2 message (it's
        // for type 1 messages).  We'll propogate it for now when we have
        // target information to return (but we may remove it or make it
        // configurable in the future.
        dwOptions |= NTLM_FLAG_REQUEST_TARGET;
    }

    dwSize += dwTargetNameSize;

    dwError = LsaAllocateMemory(dwSize, OUT_PPVOID(&pMessage));
    BAIL_ON_LW_ERROR(dwError);

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
    dwOptions |= (NTLM_FLAG_NTLM  |  // we support NTLM
                  //NTLM_FLAG_NTLM2 |  // we support NTLM2 - TODO
                  //NTLM_FLAG_128   |  // we support 128 bit encryption
                  NTLM_FLAG_56    ); // we support 56 bit encryption

    // These options are being set now simply because they appear to be
    // required for authentication to work
    dwOptions |= (NTLM_FLAG_SIGN |
                  NTLM_FLAG_SEAL |
                  NTLM_FLAG_UNKNOWN_02000000);

    // Data is checked and memory is allocated; fill in the structure
    //
    memcpy(
        &(pMessage->NtlmSignature),
        NTLM_NETWORK_SIGNATURE,
        NTLM_NETWORK_SIGNATURE_SIZE
        );

    pMessage->MessageType = NTLM_CHALLENGE_MSG;

    pMessage->NtlmFlags = dwOptions;

    if(pMessage->NtlmFlags & NTLM_FLAG_REQUEST_TARGET)
    {
        pMessage->Target.usLength = dwTargetNameSize;
        pMessage->Target.usMaxLength = pMessage->Target.usLength;
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

    BAIL_ON_LW_ERROR(dwError);

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

        }

        // Always account for the size of the TargetInfoSecBuffer, even if it's
        // not filled in.  Otherwise, OS version info won't end up in the right
        // place.
        pBuffer += sizeof(NTLM_SEC_BUFFER);

        if(pOsVersion)
        {
            memcpy(pBuffer, pOsVersion, NTLM_WIN_SPOOF_SIZE);

            pBuffer += NTLM_WIN_SPOOF_SIZE;
        }
    }

    if(pMessage->Target.usLength)
    {
        pMessage->Target.dwOffset = pBuffer - (PBYTE)pMessage;

        if(pMessage->NtlmFlags & NTLM_FLAG_TYPE_DOMAIN)
        {
            pTrav = (PBYTE)pDomainName;
        }
        else
        {
            pTrav = (PBYTE)pServerName;
        }

        while(*pTrav)
        {
            *pBuffer = *pTrav;

            if(pMessage->NtlmFlags & NTLM_FLAG_UNICODE)
            {
                pBuffer++;
            }
            pBuffer++;

            pTrav++;
        }
    }

    if(pTargetInfoSecBuffer)
    {
        // Remember, target block information is ALWAYS in UNICODE
        pTargetInfoSecBuffer->dwOffset = pBuffer - (PBYTE)pMessage;

        if(!LW_IS_NULL_OR_EMPTY_STR(pDomainName))
        {
            pTargetInfoBlock = (PNTLM_TARGET_INFO_BLOCK)pBuffer;

            pTargetInfoBlock->sLength = strlen(pDomainName) * sizeof(WCHAR);

            pTargetInfoBlock->sType = NTLM_TIB_DOMAIN_NAME;

            pTrav = (PBYTE)pDomainName;

            pBuffer += sizeof(NTLM_TARGET_INFO_BLOCK);

            while(*pTrav)
            {
                *pBuffer = *pTrav;

                pBuffer++;
                pBuffer++;

                pTrav++;
            }
        }

        if(!LW_IS_NULL_OR_EMPTY_STR(pServerName))
        {
            pTargetInfoBlock = (PNTLM_TARGET_INFO_BLOCK)pBuffer;

            pTargetInfoBlock->sLength = strlen(pServerName) * sizeof(WCHAR);

            pTargetInfoBlock->sType = NTLM_TIB_SERVER_NAME;

            pTrav = (PBYTE)pServerName;

            pBuffer += sizeof(NTLM_TARGET_INFO_BLOCK);

            while(*pTrav)
            {
                *pBuffer = *pTrav;

                pBuffer++;
                pBuffer++;

                pTrav++;
            }
        }

        if(!LW_IS_NULL_OR_EMPTY_STR(pDnsDomainName))
        {
            pTargetInfoBlock = (PNTLM_TARGET_INFO_BLOCK)pBuffer;

            pTargetInfoBlock->sLength = strlen(pDnsDomainName) * sizeof(WCHAR);

            pTargetInfoBlock->sType = NTLM_TIB_DNS_DOMAIN_NAME;

            pTrav = (PBYTE)pDnsDomainName;

            pBuffer += sizeof(NTLM_TARGET_INFO_BLOCK);

            while(*pTrav)
            {
                *pBuffer = *pTrav;

                pBuffer++;
                pBuffer++;

                pTrav++;
            }
        }

        if(!LW_IS_NULL_OR_EMPTY_STR(pDnsServerName))
        {
            pTargetInfoBlock = (PNTLM_TARGET_INFO_BLOCK)pBuffer;

            pTargetInfoBlock->sLength = strlen(pDnsServerName) * sizeof(WCHAR);

            pTargetInfoBlock->sType = NTLM_TIB_DNS_SERVER_NAME;

            pTrav = (PBYTE)pDnsServerName;

            pBuffer += sizeof(NTLM_TARGET_INFO_BLOCK);

            while(*pTrav)
            {
                *pBuffer = *pTrav;

                pBuffer++;
                pBuffer++;

                pTrav++;
            }
        }

        pTargetInfoBlock = (PNTLM_TARGET_INFO_BLOCK)pBuffer;
        pTargetInfoBlock->sLength = 0;
        pTargetInfoBlock->sType = NTLM_TIB_TERMINATOR;

        pBuffer += sizeof(*pTargetInfoBlock);
    }

    LW_ASSERT(pBuffer == (PBYTE)pMessage + dwSize);

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
    IN PNTLM_CHALLENGE_MESSAGE pChlngMsg,
    IN PCSTR pUserName,
    IN PCSTR pPassword,
    IN PBYTE pOsVersion,
    IN DWORD dwNtRespType,
    IN DWORD dwLmRespType,
    OUT PDWORD pdwSize,
    OUT PNTLM_RESPONSE_MESSAGE* ppRespMsg,
    OUT PBYTE pLmUserSessionKey,
    OUT PBYTE pNtlmUserSessionKey
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
    PCHAR pAuthTargetName;
    CHAR pWorkstation[HOST_NAME_MAX];

    // sanity checks
    if(!pChlngMsg || !ppRespMsg)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LW_ERROR(dwError);
    }

    dwError = NtlmGetAuthTargetNameFromChallenge(pChlngMsg, &pAuthTargetName);
    BAIL_ON_LW_ERROR(dwError);

    dwError = gethostname(pWorkstation, HOST_NAME_MAX);
    if(dwError)
    {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_LW_ERROR(dwError);
    }

    *ppRespMsg = NULL;
    *pdwSize = 0;

    dwSize += sizeof(NTLM_RESPONSE_MESSAGE);

    dwAuthTrgtNameSize = strlen(pAuthTargetName);
    dwUserNameSize = strlen(pUserName);
    dwWorkstationSize = strlen(pWorkstation);

    if(pChlngMsg->NtlmFlags & NTLM_FLAG_UNICODE)
    {
        dwAuthTrgtNameSize *= sizeof(WCHAR);
        dwUserNameSize *= sizeof(WCHAR);
        dwWorkstationSize *= sizeof(WCHAR);
    }

    dwSize += dwAuthTrgtNameSize;
    dwSize += dwUserNameSize;
    dwSize += dwWorkstationSize;

    // calculate the response size...
    dwError = NtlmCalculateResponseSize(
        pChlngMsg,
        dwNtRespType,
        &dwNtMsgSize
        );
    BAIL_ON_LW_ERROR(dwError);

    dwSize += dwNtMsgSize;

    dwError = NtlmCalculateResponseSize(
        pChlngMsg,
        dwLmRespType,
        &dwLmMsgSize
        );
    BAIL_ON_LW_ERROR(dwError);

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
    BAIL_ON_LW_ERROR(dwError);

    // Data is checked and memory is allocated; fill in the structure
    //
    memcpy(
        &pMessage->NtlmSignature,
        NTLM_NETWORK_SIGNATURE,
        NTLM_NETWORK_SIGNATURE_SIZE);

    pMessage->MessageType = NTLM_RESPONSE_MSG;

    pMessage->LmResponse.usLength = dwLmMsgSize;
    pMessage->LmResponse.usMaxLength = pMessage->LmResponse.usLength;

    pMessage->NtResponse.usLength = dwNtMsgSize;
    pMessage->NtResponse.usMaxLength = pMessage->NtResponse.usLength;

    pMessage->AuthTargetName.usLength = dwAuthTrgtNameSize;
    pMessage->AuthTargetName.usMaxLength = pMessage->AuthTargetName.usLength;

    pMessage->UserName.usLength = dwUserNameSize;
    pMessage->UserName.usMaxLength = pMessage->UserName.usLength;

    pMessage->Workstation.usLength = dwWorkstationSize;
    pMessage->Workstation.usMaxLength = pMessage->Workstation.usLength;

    // We've filled in the main structure, now add optional data at the end
    pBuffer = (PBYTE)pMessage + sizeof(NTLM_RESPONSE_MESSAGE);

    pSessionKey = (PNTLM_SEC_BUFFER)pBuffer;

    pSessionKey->usLength = 0;
    if(pChlngMsg->NtlmFlags & NTLM_FLAG_KEY_EXCH)
    {
        // we won't fill this in until after we return from this function, but
        // we'll save space to fill in later
        pSessionKey->usLength = NTLM_SESSION_KEY_SIZE;
    }
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
        pUserName,
        pPassword,
        dwLmRespType,
        dwLmMsgSize,
        pLmUserSessionKey,
        pBuffer
        );

    BAIL_ON_LW_ERROR(dwError);

    pBuffer += pMessage->LmResponse.usLength;

    pMessage->NtResponse.dwOffset = pBuffer - (PBYTE)pMessage;

    dwError = NtlmBuildResponse(
        pChlngMsg,
        pUserName,
        pPassword,
        dwNtRespType,
        dwNtMsgSize,
        pNtlmUserSessionKey,
        pBuffer
        );

    BAIL_ON_LW_ERROR(dwError);

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

    // This is only a partial validation since we may be adding a session key
    // to this message.
    LW_ASSERT(pBuffer + pSessionKey->usLength == (PBYTE)pMessage + dwSize);

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
VOID
NtlmStoreSecondaryKey(
    IN PBYTE pMasterKey,
    IN PBYTE pSecondaryKey,
    IN OUT PNTLM_RESPONSE_MESSAGE pMessage
    )
{
    PNTLM_SEC_BUFFER pSecBuffer = NULL;
    BYTE EncryptedKey[NTLM_SESSION_KEY_SIZE] = {0};
    PBYTE pSessionKey = NULL;
    RC4_KEY Rc4Key;

    // Encrypt the secondary key with the master key
    memset(&Rc4Key, 0, sizeof(Rc4Key));

    RC4_set_key(&Rc4Key, NTLM_SESSION_KEY_SIZE, pMasterKey);
    RC4(&Rc4Key, NTLM_SESSION_KEY_SIZE, pSecondaryKey, EncryptedKey);

    // The session key is the first bit of information dangling from the
    // structure.
    pSecBuffer = (PNTLM_SEC_BUFFER)((PBYTE)pMessage + sizeof(*pMessage));

    pSessionKey = pSecBuffer->dwOffset + (PBYTE)pMessage;

    memcpy(pSessionKey, EncryptedKey, pSecBuffer->usLength);
}

/******************************************************************************/
VOID
NtlmWeakenSessionKey(
    IN PNTLM_CHALLENGE_MESSAGE pChlngMsg,
    IN OUT PBYTE pMasterKey,
    OUT PDWORD pcbKeyLength
    )
{
    // Only weaken the key if LanManagerSessionKey was used
    if(pChlngMsg->NtlmFlags & NTLM_FLAG_LM_KEY)
    {
        if(pChlngMsg->NtlmFlags & NTLM_FLAG_56)
        {
            pMasterKey[7] = 0xa0;
            memset(&pMasterKey[8], 0, 8);
        }
        else if(!(pChlngMsg->NtlmFlags & NTLM_FLAG_128))
        {
            // TODO verify the endianness of this operation.  These values
            // might be in reverse order.
            pMasterKey[5] = 0xe5;
            pMasterKey[6] = 0x38;
            pMasterKey[7] = 0xb0;
            memset(&pMasterKey[8], 0, 8);
        }
    }
}

/******************************************************************************/
DWORD
NtlmGetAuthTargetNameFromChallenge(
    IN PNTLM_CHALLENGE_MESSAGE pChlngMsg,
    OUT PCHAR* ppAuthTargetName
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PCHAR pName = NULL;
    DWORD dwNameLength = 0;
    PBYTE pBuffer = NULL;
    PNTLM_SEC_BUFFER pTargetSecBuffer = &pChlngMsg->Target;
    DWORD nIndex = 0;

    *ppAuthTargetName = NULL;

    dwNameLength = pTargetSecBuffer->usLength;
    pBuffer = pTargetSecBuffer->dwOffset + (PBYTE)pChlngMsg;

    if(pChlngMsg->NtlmFlags & NTLM_FLAG_OEM)
    {
        dwError = LsaAllocateMemory(dwNameLength + 1, OUT_PPVOID(&pName));
        BAIL_ON_LW_ERROR(dwError);

        memcpy(pName, pBuffer, dwNameLength);
    }
    else
    {
        dwNameLength = dwNameLength / sizeof(WCHAR);

        dwError = LsaAllocateMemory(dwNameLength + 1, OUT_PPVOID(&pName));
        BAIL_ON_LW_ERROR(dwError);

        for(nIndex = 0; nIndex < dwNameLength; nIndex++)
        {
            pName[nIndex] = pBuffer[nIndex * sizeof(WCHAR)];
        }
    }

cleanup:
    *ppAuthTargetName = pName;
    return dwError;
error:
    LSA_SAFE_FREE_STRING(pName);
    goto cleanup;
}

/******************************************************************************/
DWORD
NtlmBuildResponse(
    IN PNTLM_CHALLENGE_MESSAGE pChlngMsg,
    IN PCSTR pUserName,
    IN PCSTR pPassword,
    IN DWORD dwResponseType,
    IN DWORD dwBufferSize,
    OUT PBYTE pUserSessionKey,
    OUT PBYTE pBuffer
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;

    if(!pChlngMsg)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LW_ERROR(dwError);
    }

    switch(dwResponseType)
    {
    case NTLM_RESPONSE_TYPE_LM:
        {
            NtlmBuildLmResponse(
                pChlngMsg,
                pPassword,
                dwBufferSize,
                pUserSessionKey,
                pBuffer
                );
        }
        break;
    case NTLM_RESPONSE_TYPE_LMv2:
        {
            dwError = NtlmBuildLmV2Response();
            BAIL_ON_LW_ERROR(dwError);
        }
        break;
    case NTLM_RESPONSE_TYPE_NTLM:
        {
            dwError = NtlmBuildNtlmResponse(
                pChlngMsg,
                pPassword,
                dwBufferSize,
                pUserSessionKey,
                pBuffer
                );
            BAIL_ON_LW_ERROR(dwError);
        }
        break;
    case NTLM_RESPONSE_TYPE_NTLMv2:
        {
            dwError = NtlmBuildNtlmV2Response(
                pChlngMsg,
                pUserName,
                pPassword,
                dwBufferSize,
                pUserSessionKey,
                pBuffer
                );
            BAIL_ON_LW_ERROR(dwError);
        }
        break;
    case NTLM_RESPONSE_TYPE_NTLM2:
        {
            dwError = NtlmBuildNtlm2Response();
            BAIL_ON_LW_ERROR(dwError);
        }
        break;
    case NTLM_RESPONSE_TYPE_ANONYMOUS:
        {
            dwError = NtlmBuildAnonymousResponse();
            BAIL_ON_LW_ERROR(dwError);
        }
        break;
    default:
        {
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_LW_ERROR(dwError);
        }
    }

cleanup:
    return dwError;
error:
    goto cleanup;
}

/******************************************************************************/
VOID
NtlmBuildLmResponse(
    IN PNTLM_CHALLENGE_MESSAGE pChlngMsg,
    IN PCSTR pPassword,
    IN DWORD dwResponseSize,
    OUT PBYTE pUserSessionKey,
    OUT PBYTE pResponse
    )
{
    DWORD dwIndex = 0;
    DWORD dwPasswordLen = 0;
    BYTE LmHash[NTLM_HASH_SIZE] = {0};
    ULONG64 ulKey1 = 0;
    ULONG64 ulKey2 = 0;
    ULONG64 ulKey3 = 0;
    DES_key_schedule DesKeySchedule;

    memset(&DesKeySchedule, 0, sizeof(DES_key_schedule));

    memset(pResponse, 0, NTLM_RESPONSE_SIZE_NTLM);
    memset(pUserSessionKey, 0, NTLM_SESSION_KEY_SIZE);

    dwPasswordLen = strlen(pPassword);

    // if password is less than 15 characters, create LM hash, otherwise, the
    // hash is set to zero.
    if(dwPasswordLen <= NTLM_LM_MAX_PASSWORD_SIZE)
    {
        // convert the password to upper case
        for(dwIndex = 0; dwIndex < NTLM_LM_MAX_PASSWORD_SIZE; dwIndex++)
        {
            LmHash[dwIndex] = toupper(pPassword[dwIndex]);

            if(!pPassword[dwIndex])
            {
                break;
            }
        }

        ulKey1 = NtlmCreateKeyFromHash(&LmHash[0], 7);
        ulKey2 = NtlmCreateKeyFromHash(&LmHash[7], 7);

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

    // The LM user session is... surprisingly... just the first half of the hash
    // we just generated padded out to 16 bytes.
    memcpy(pUserSessionKey, LmHash, 8);

    ulKey1 = NtlmCreateKeyFromHash(&LmHash[0],  7);
    ulKey2 = NtlmCreateKeyFromHash(&LmHash[7],  7);
    ulKey3 = NtlmCreateKeyFromHash(&LmHash[14], 2);

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
}

/******************************************************************************/
DWORD
NtlmBuildNtlmResponse(
    IN PNTLM_CHALLENGE_MESSAGE pChlngMsg,
    IN PCSTR pPassword,
    IN DWORD dwResponseSize,
    OUT PBYTE pUserSessionKey,
    OUT PBYTE pResponse
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    BYTE NtlmHash[MD4_DIGEST_LENGTH] = {0};
    ULONG64 ulKey1 = 0;
    ULONG64 ulKey2 = 0;
    ULONG64 ulKey3 = 0;
    DES_key_schedule DesKeySchedule;

    memset(&DesKeySchedule, 0, sizeof(DES_key_schedule));

    memset(pResponse, 0, NTLM_RESPONSE_SIZE_NTLM);
    memset(pUserSessionKey, 0, NTLM_SESSION_KEY_SIZE);

    NtlmCreateNtlmV1Hash(pPassword, NtlmHash);

    // Generate the session key
    dwError = NtlmCreateMD4Digest(
        NtlmHash,
        MD4_DIGEST_LENGTH,
        pUserSessionKey
        );
    BAIL_ON_LW_ERROR(dwError);

    ulKey1 = NtlmCreateKeyFromHash(&NtlmHash[0], 7);
    ulKey2 = NtlmCreateKeyFromHash(&NtlmHash[7], 7);
    ulKey3 = NtlmCreateKeyFromHash(&NtlmHash[14], 2);

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
    memset(pResponse, 0, NTLM_RESPONSE_SIZE_NTLM);
    memset(pUserSessionKey, 0, NTLM_SESSION_KEY_SIZE);
    goto cleanup;
}

/******************************************************************************/
DWORD
NtlmBuildNtlmV2Response(
    IN PNTLM_CHALLENGE_MESSAGE pChlngMsg,
    IN PCSTR pUserName,
    IN PCSTR pPassword,
    IN DWORD dwResponseSize,
    OUT PBYTE pUserSessionKey,
    OUT PBYTE pResponse
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    BYTE NtlmHashV1[MD4_DIGEST_LENGTH] = {0};
    BYTE NtlmHashV2[MD4_DIGEST_LENGTH] = {0};
    PBYTE pBlob = NULL;
    DWORD dwBlobSize = 0;
    PSTR pTarget = NULL;
    DWORD dwKeyLen = NTLM_HASH_SIZE;

    memset(pResponse, 0, dwResponseSize);
    memset(pUserSessionKey, 0, NTLM_SESSION_KEY_SIZE);

    dwError = NtlmCreateNtlmV1Hash(pPassword, NtlmHashV1);
    BAIL_ON_LW_ERROR(dwError);

    dwError = NtlmGetAuthTargetNameFromChallenge(pChlngMsg, &pTarget);
    BAIL_ON_LW_ERROR(dwError);

    dwError = NtlmCreateNtlmV2Hash(pUserName, pTarget, NtlmHashV1, NtlmHashV2);
    BAIL_ON_LW_ERROR(dwError);

    dwError = NtlmCreateNtlmV2Blob(pChlngMsg, NtlmHashV2, &dwBlobSize, &pBlob);
    BAIL_ON_LW_ERROR(dwError);

    LW_ASSERT(dwResponseSize == dwBlobSize);

    memcpy(pResponse, pBlob, dwResponseSize);

    // Generate the session key which is just the first 16 bytes of the response
    // HMAC md5 encrypted using the NTLMv2 hash as the key
    HMAC(
        EVP_md5(),
        NtlmHashV2,
        NTLM_HASH_SIZE,
        pResponse,
        NTLM_HASH_SIZE,
        pUserSessionKey,
        &dwKeyLen);

cleanup:
    LSA_SAFE_FREE_MEMORY(pTarget);
    LSA_SAFE_FREE_MEMORY(pBlob);

    return dwError;
error:
    memset(pResponse, 0, NTLM_RESPONSE_SIZE_NTLM);
    memset(pUserSessionKey, 0, NTLM_SESSION_KEY_SIZE);
    goto cleanup;
}

/******************************************************************************/
DWORD
NtlmCreateNtlmV2Blob(
    IN PNTLM_CHALLENGE_MESSAGE pChlngMsg,
    IN BYTE NtlmHashV2[MD4_DIGEST_LENGTH],
    OUT PDWORD pdwSize,
    OUT PBYTE* ppBlob
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PNTLM_BLOB pNtlmBlob = NULL;
    BYTE BlobSignature[] = NTLM_BLOB_SIGNATURE;
    PBYTE pBuffer = NULL;
    PBYTE pOriginal = NULL;
    DWORD dwBlobSize = 0;
    PNTLM_SEC_BUFFER pTargetInfo = NULL;
    PBYTE pTargetBuffer = NULL;
    PBYTE pChallenge = NULL;
    BYTE TempBlobHash[NTLM_HASH_SIZE] = {0};
    DWORD dwKeyLen = NTLM_HASH_SIZE;

    *ppBlob = NULL;
    *pdwSize = 0;

    pChallenge = pChlngMsg->Challenge;

    pTargetInfo =
        (PNTLM_SEC_BUFFER)
            ((PBYTE)pChlngMsg +
            sizeof(NTLM_CHALLENGE_MESSAGE) +
            NTLM_LOCAL_CONTEXT_SIZE);

    pTargetBuffer = (PBYTE)pChlngMsg + pTargetInfo->dwOffset;

    // We're going to allocate the entire blob at this point and just use
    // portions of it as needed.
    dwBlobSize =
        NTLM_HASH_SIZE +
        sizeof(NTLM_BLOB) +
        pTargetInfo->usLength +
        NTLM_BLOB_TRAILER_SIZE;

    dwError = LsaAllocateMemory(dwBlobSize, OUT_PPVOID(&pOriginal));
    BAIL_ON_LW_ERROR(dwError);

    // the beginning of the blob will contain the final hash value, so push
    // the blob pointer up by that amount
    pNtlmBlob = (PNTLM_BLOB)pOriginal + NTLM_HASH_SIZE;

    memcpy(
        pNtlmBlob->NtlmBlobSignature,
        BlobSignature,
        NTLM_BLOB_SIGNATURE_SIZE);

    pNtlmBlob->Reserved1 = 0;

    pNtlmBlob->TimeStamp = (ULONG64)time(NULL);
    pNtlmBlob->TimeStamp += 11644473600;
    pNtlmBlob->TimeStamp *= 10000000;

    dwError = NtlmGetRandomBuffer(
        (PBYTE)&pNtlmBlob->ClientNonce,
        sizeof(pNtlmBlob->ClientNonce));

    pNtlmBlob->Reserved2 = 0;

    // Copy in the target info buffer
    pBuffer = (PBYTE)pNtlmBlob + sizeof(NTLM_BLOB);
    memcpy(pBuffer, pTargetBuffer, pTargetInfo->usLength);

    // The last 4 bytes should never have changed so they should still be 0

    // Now prepend the challenge and encrypt
    pBuffer = (PBYTE)pNtlmBlob - NTLM_CHALLENGE_SIZE;
    memcpy(pBuffer, pChallenge, NTLM_CHALLENGE_SIZE);

    HMAC(
        EVP_md5(),
        NtlmHashV2,
        NTLM_HASH_SIZE,
        pBuffer,
        dwBlobSize - (NTLM_HASH_SIZE / 2),
        TempBlobHash,
        &dwKeyLen);

    memcpy(pOriginal, TempBlobHash, NTLM_HASH_SIZE);

cleanup:
    *ppBlob = pOriginal;
    return dwError;
error:
    LSA_SAFE_FREE_MEMORY(pOriginal);
    goto cleanup;
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
        BAIL_ON_LW_ERROR(dwError);
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
            NtlmCalculateNtlmV2ResponseSize(pChlngMsg, &dwSize);
        }
        break;
    default:
        {
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_LW_ERROR(dwError);
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
VOID
NtlmCalculateNtlmV2ResponseSize(
    IN PNTLM_CHALLENGE_MESSAGE pChlngMsg,
    OUT PDWORD pdwSize
    )
{
    PNTLM_SEC_BUFFER pTargetInfo = NULL;

    pTargetInfo =
        (PNTLM_SEC_BUFFER)
            ((PBYTE)pChlngMsg +
            sizeof(NTLM_CHALLENGE_MESSAGE) +
            NTLM_LOCAL_CONTEXT_SIZE);

    *pdwSize =
        NTLM_HASH_SIZE +
        sizeof(NTLM_BLOB) +
        pTargetInfo->usLength +
        NTLM_BLOB_TRAILER_SIZE;
}

/******************************************************************************/
DWORD
NtlmCreateNtlmV1Hash(
    PCSTR pPassword,
    BYTE Hash[MD4_DIGEST_LENGTH]
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwTempPassSize = strlen(pPassword) * sizeof(WCHAR);
    PWCHAR pwcTempPass = NULL;

    memset(Hash, 0, MD4_DIGEST_LENGTH);

    dwError = LsaAllocateMemory(dwTempPassSize, OUT_PPVOID(&pwcTempPass));
    BAIL_ON_LW_ERROR(dwError);

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
        Hash
        );
    BAIL_ON_LW_ERROR(dwError);

cleanup:
    LSA_SAFE_FREE_MEMORY(pwcTempPass);
    return dwError;
error:
    memset(Hash, 0, MD4_DIGEST_LENGTH);
    goto cleanup;
}

/******************************************************************************/
DWORD
NtlmCreateNtlmV2Hash(
    PCSTR pUserName,
    PCSTR pDomain,
    BYTE NtlmV1Hash[MD4_DIGEST_LENGTH],
    BYTE NtlmV2Hash[MD4_DIGEST_LENGTH]
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwTempBufferSize = 0;
    PWSTR pTempBuffer = NULL;
    PWCHAR pTrav = NULL;
    DWORD dwKeySize = NTLM_HASH_SIZE;

    memset(NtlmV2Hash, 0, MD4_DIGEST_LENGTH);

    dwTempBufferSize = (strlen(pDomain) + strlen(pUserName)) * sizeof(WCHAR);

    dwError = LsaAllocateMemory(dwTempBufferSize, OUT_PPVOID(&pTempBuffer));
    BAIL_ON_LW_ERROR(dwError);

    pTrav = pTempBuffer;

    // remember that the user name is converted to upper case
    while(*pUserName)
    {
        *pTrav = toupper(*pUserName);
        pTrav++;
        pUserName++;
    }

    while(*pDomain)
    {
        *pTrav = *pDomain;
        pTrav++;
        pDomain++;
    }

    HMAC(
        EVP_md5(),
        NtlmV1Hash,
        MD4_DIGEST_LENGTH,
        (PBYTE)pTempBuffer,
        dwTempBufferSize,
        NtlmV2Hash,
        &dwKeySize);

cleanup:
    LSA_SAFE_FREE_MEMORY(pTempBuffer);
    return dwError;
error:
    memset(NtlmV2Hash, 0, MD4_DIGEST_LENGTH);
    goto cleanup;
}

/******************************************************************************/
VOID
NtlmGenerateLanManagerSessionKey(
    IN PNTLM_RESPONSE_MESSAGE pMessage,
    IN PBYTE pLmUserSessionKey,
    OUT PBYTE pLanManagerSessionKey
    )
{
    ULONG64 ulKey1 = 0;
    ULONG64 ulKey2 = 0;
    BYTE KeyBuffer[NTLM_SESSION_KEY_SIZE] = {0};
    DES_key_schedule DesKeySchedule;
    PNTLM_SEC_BUFFER pLmSecBuffer = NULL;
    PBYTE pLmResponse = NULL;

    memset(&DesKeySchedule, 0, sizeof(DES_key_schedule));

    pLmSecBuffer = &pMessage->LmResponse;
    pLmResponse = (PBYTE)pMessage + pLmSecBuffer->dwOffset;

    memcpy(KeyBuffer, pLmUserSessionKey, 8);
    memset(&KeyBuffer[8], 0xbd, 6);

    ulKey1 = NtlmCreateKeyFromHash(&KeyBuffer[0], 7);
    ulKey2 = NtlmCreateKeyFromHash(&KeyBuffer[7], 7);

    DES_set_key_unchecked((const_DES_cblock*)&ulKey1, &DesKeySchedule);
    DES_ecb_encrypt(
        (const_DES_cblock *)pLmResponse,
        (DES_cblock*)&pLanManagerSessionKey[0],
        &DesKeySchedule,
        DES_ENCRYPT
        );

    DES_set_key_unchecked((const_DES_cblock*)&ulKey2, &DesKeySchedule);
    DES_ecb_encrypt(
        (const_DES_cblock *)pLmResponse,
        (DES_cblock*)&pLanManagerSessionKey[8],
        &DesKeySchedule,
        DES_ENCRYPT
        );
}

/******************************************************************************/
DWORD
NtlmCreateMD4Digest(
    IN PBYTE pBuffer,
    IN DWORD dwBufferLen,
    OUT BYTE MD4Digest[MD4_DIGEST_LENGTH]
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    MD4_CTX Md4Ctx;

    dwError = MD4_Init(&Md4Ctx);

    if(dwError != 1)
    {
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_LW_ERROR(dwError);
    }

    dwError = MD4_Update(&Md4Ctx, pBuffer, dwBufferLen);

    if(dwError != 1)
    {
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_LW_ERROR(dwError);
    }

    dwError = MD4_Final(MD4Digest, &Md4Ctx);

    if(dwError != 1)
    {
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_LW_ERROR(dwError);
    }

    dwError = LW_ERROR_SUCCESS;

cleanup:
    return dwError;
error:
    goto cleanup;
}

/******************************************************************************/
ULONG64
NtlmCreateKeyFromHash(
    IN PBYTE pBuffer,
    IN DWORD dwLength
    )
{
    ULONG64 Key = 0;
    DWORD nIndex = 0;

    LW_ASSERT(dwLength <= 7);

    for(nIndex = 0; nIndex < dwLength; nIndex++)
    {
        ((PBYTE)(&Key))[6 - nIndex] = pBuffer[nIndex];
    }

    NtlmSetParityBit(&Key);

    Key = LW_ENDIAN_SWAP64(Key);

    return Key;
}

/******************************************************************************/
VOID
NtlmSetParityBit(
    IN OUT PULONG64 pKey
    )
{
    ULONG64 NewKey = *pKey;

    NewKey = NewKey << 1;
    NewKey = (NewKey & 0x00000000000000FF)|((NewKey & 0xFFFFFFFFFFFFFF00) << 1);
    NewKey = (NewKey & 0x000000000000FFFF)|((NewKey & 0xFFFFFFFFFFFF0000) << 1);
    NewKey = (NewKey & 0x0000000000FFFFFF)|((NewKey & 0xFFFFFFFFFF000000) << 1);
    NewKey = (NewKey & 0x00000000FFFFFFFF)|((NewKey & 0xFFFFFFFF00000000) << 1);
    NewKey = (NewKey & 0x000000FFFFFFFFFF)|((NewKey & 0xFFFFFF0000000000) << 1);
    NewKey = (NewKey & 0x0000FFFFFFFFFFFF)|((NewKey & 0xFFFF000000000000) << 1);
    NewKey = (NewKey & 0x00FFFFFFFFFFFFFF)|((NewKey & 0xFF00000000000000) << 1);

    DES_set_odd_parity((DES_cblock*)&NewKey);

    *pKey = NewKey;
}
