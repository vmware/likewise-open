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

/******************************************************************************/
DWORD
NtlmInitContext(
    OUT PNTLM_CONTEXT *ppNtlmContext
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;

    if(!ppNtlmContext)
    {
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_NTLM_ERROR(dwError);
    }

    dwError = LsaAllocateMemory(
        sizeof(NTLM_CONTEXT),
        (PVOID*)ppNtlmContext
        );

    BAIL_ON_NTLM_ERROR(dwError);

    memset((*ppNtlmContext), 0, sizeof(NTLM_CONTEXT));
    (*ppNtlmContext)->NtlmState = NtlmStateBlank;

    BAIL_ON_NTLM_ERROR(dwError);

cleanup:
    return dwError;
error:
    if(*ppNtlmContext)
    {
        LsaFreeMemory(*ppNtlmContext);
        *ppNtlmContext = NULL;
    }
    goto cleanup;
}

/******************************************************************************/
DWORD
NtlmInsertContext(
    PNTLM_CONTEXT pNtlmContext
    )
{
    // WARNING:
    // WARNING: Context lock must already be acquired
    // WARNING:

    DWORD dwError = LSA_ERROR_SUCCESS;
    BOOLEAN bCollision = FALSE;
    PNTLM_CONTEXT pCollisionContext = NULL;

    if(!pNtlmContext)
    {
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_NTLM_ERROR(dwError);
    }

    // This handle should be random and will be our key into a either a
    // double linked list (bad) or a red-black tree (good)
    //
    do
    {
        bCollision = FALSE;

        dwError = NtlmGetRandomBuffer(
            (PBYTE)&(pNtlmContext->ContextHandle),
            sizeof(CtxtHandle)
            );

        if(LSA_ERROR_SUCCESS != dwError)
        {
            break;
        }

        dwError = NtlmFindContext(
            &(pNtlmContext->ContextHandle),
            &pCollisionContext
            );

        if(dwError == LSA_ERROR_SUCCESS)
        {
            bCollision = TRUE;

            // This removes the reference we added for the find function
            NtlmRemoveContext(&(pCollisionContext->ContextHandle));
        }

    } while(bCollision);

    if(LSA_ERROR_INVALID_TOKEN == dwError)
    {
        pNtlmContext->pNext = gpNtlmContextList;
        gpNtlmContextList = pNtlmContext;
        pNtlmContext->dwRefCount++;
    }

cleanup:
    return dwError;
error:
    goto cleanup;
}

/******************************************************************************/
DWORD
NtlmRemoveContext(
    IN PCtxtHandle pCtxtHandle
    )
{
    // WARNING:
    // WARNING: Context lock must already be acquired
    // WARNING:

    DWORD dwError = LSA_ERROR_SUCCESS;
    PNTLM_CONTEXT pTrav = NULL;
    PNTLM_CONTEXT pHold = NULL;

    if(!pCtxtHandle)
    {
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_NTLM_ERROR(dwError);
    }

    pTrav = gpNtlmContextList;

    if(!pTrav)
    {
        dwError = LSA_ERROR_INVALID_TOKEN;
        BAIL_ON_NTLM_ERROR(dwError);
    }
    else if(pTrav->ContextHandle.dwLower == pCtxtHandle->dwLower &&
            pTrav->ContextHandle.dwUpper == pCtxtHandle->dwUpper)
    {
        pTrav->dwRefCount--;

        if(pTrav->dwRefCount <= 0)
        {
            gpNtlmContextList = pTrav->pNext;
            NtlmFreeContext(pTrav);
        }
    }
    else
    {
        while(pTrav->pNext)
        {
            if(pTrav->pNext->ContextHandle.dwLower == pCtxtHandle->dwLower&&
               pTrav->pNext->ContextHandle.dwUpper == pCtxtHandle->dwUpper)
            {
                pHold->dwRefCount--;

                if(pHold->dwRefCount <= 0)
                {
                    pHold = pTrav->pNext;
                    pTrav->pNext = pHold->pNext;

                    NtlmFreeContext(pHold);
                }

                break;
            }
            pTrav = pTrav->pNext;
        }
        dwError = LSA_ERROR_INVALID_TOKEN;
        BAIL_ON_NTLM_ERROR(dwError);
    }

cleanup:
    return dwError;
error:
    goto cleanup;
}

/******************************************************************************/
DWORD
NtlmRemoveAllContext(
    VOID
    )
{
    // WARNING:
    // WARNING: Context lock must already be acquired
    // WARNING:
    // WARNING: Also, this wipes the contexts... all of them... regardless
    // WARNING: of the reference count.  Only use this at shutdown.
    // WARNING:

    DWORD dwError = LSA_ERROR_SUCCESS;
    PNTLM_CONTEXT pTrav = NULL;

    while(gpNtlmContextList)
    {
        pTrav = gpNtlmContextList;
        gpNtlmContextList = pTrav->pNext;

        dwError = NtlmFreeContext(pTrav);
        BAIL_ON_NTLM_ERROR(dwError);
    }

cleanup:
    return dwError;
error:
    goto cleanup;
}

/******************************************************************************/
DWORD
NtlmFindContext(
    IN PCtxtHandle pCtxtHandle,
    OUT PNTLM_CONTEXT *ppNtlmContext
    )
{
    // WARNING:
    // WARNING: Context lock must already be acquired
    // WARNING:

    DWORD dwError = LSA_ERROR_SUCCESS;
    PNTLM_CONTEXT pTrav = NULL;

    *ppNtlmContext = NULL;

    if(!pCtxtHandle)
    {
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_NTLM_ERROR(dwError);
    }

    pTrav = gpNtlmContextList;

    while(pTrav)
    {
        if(pTrav->ContextHandle.dwLower == (*ppNtlmContext)->ContextHandle.dwLower &&
           pTrav->ContextHandle.dwUpper == (*ppNtlmContext)->ContextHandle.dwUpper)
        {
            *ppNtlmContext = pTrav;
            (*ppNtlmContext)->dwRefCount++;
            break;
        }

        pTrav = pTrav->pNext;
    }

    if(!(*ppNtlmContext))
    {
        dwError = LSA_ERROR_INVALID_TOKEN;
    }

cleanup:
    return dwError;
error:
    goto cleanup;

}

/******************************************************************************/
DWORD
NtlmFreeContext(
    PNTLM_CONTEXT pNtlmContext
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;

    if(!pNtlmContext)
    {
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_NTLM_ERROR(dwError);
    }

    if(pNtlmContext->pMessage)
    {
        LsaFreeMemory(pNtlmContext->pMessage);
    }

    LsaFreeMemory(pNtlmContext);

cleanup:
    return dwError;
error:
    goto cleanup;
}

/******************************************************************************/
DWORD
NtlmCreateContextFromSecBufferDesc(
    PSecBufferDesc pSecBufferDesc,
    NTLM_STATE nsContextType,
    PNTLM_CONTEXT *ppNtlmContext
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    return dwError;
}

/******************************************************************************/
DWORD
NtlmGetRandomBuffer(
    PBYTE pBuffer,
    DWORD dwLen
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    INT nFileDesc;
    DWORD dwBytesRead = 0;

    if(!pBuffer || dwLen <= 0)
    {
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_NTLM_ERROR(dwError);
    }

    nFileDesc = open(NTLM_URANDOM_DEV, O_RDONLY);
    if(-1 == nFileDesc)
    {
        nFileDesc = open(NTLM_RANDOM_DEV, O_RDONLY);
        if(-1 == nFileDesc)
        {
            // We should translate the errno value to our codes
            dwError = LSA_ERROR_INTERNAL;  //errno;
            BAIL_ON_NTLM_ERROR(dwError);
        }
    }

    dwBytesRead = read(nFileDesc, pBuffer, dwLen);
    close(nFileDesc);

    if(dwBytesRead < dwLen)
    {
        dwError = LSA_ERROR_INTERNAL;
    }

error:
    return dwError;
}


/******************************************************************************/
DWORD NtlmCreateNegotiateMessage(
    IN DWORD dwOptions,
    IN PCHAR pDomain,
    IN PCHAR pWorkstation,
    IN PBYTE pOsVersion,
    OUT PNTLM_NEGOTIATE_MESSAGE *ppNegMsg
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    PNTLM_SEC_BUFFER pDomainSecBuffer = NULL;
    PNTLM_SEC_BUFFER pWorkstationSecBuffer = NULL;
    DWORD dwSize = 0;
    PBYTE pBuffer = NULL;

    // sanity checks
    if(!ppNegMsg)
    {
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_NTLM_ERROR(dwError);
    }

    *ppNegMsg = NULL;

    if(dwOptions & NTLM_FLAG_DOMAIN)
    {
        if(!pDomain)
        {
            dwError = LSA_ERROR_INVALID_PARAMETER;
            BAIL_ON_NTLM_ERROR(dwError);
        }
    }

    if(dwOptions & NTLM_FLAG_WORKSTATION)
    {
        if(!pWorkstation)
        {
            dwError = LSA_ERROR_INVALID_PARAMETER;
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

    dwError = LsaAllocateMemory(dwSize, (PVOID*)ppNegMsg);
    BAIL_ON_NTLM_ERROR(dwError);

    memset(*ppNegMsg, 0, dwSize);

    // Data is checked and memory is allocated; fill in the structure
    //
    memcpy(&((*ppNegMsg)->NtlmSignature), NTLM_SIGNATURE, NTLM_SIGNATURE_SIZE);
    (*ppNegMsg)->MessageType = NTLM_NEGOTIATE_MSG;
    (*ppNegMsg)->NtlmFlags = dwOptions;

    // Start writing optional information (if there is any) after the structure
    pBuffer = ((PBYTE)(*ppNegMsg)) + sizeof(NTLM_NEGOTIATE_MESSAGE);

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
        pWorkstationSecBuffer->dwOffset = pBuffer - (PBYTE)(*ppNegMsg);
        pBuffer += pWorkstationSecBuffer->usLength;
    }

    if(pDomainSecBuffer && pDomainSecBuffer->usLength)
    {
        memcpy(pBuffer, pDomain, pDomainSecBuffer->usLength);
        pDomainSecBuffer->dwOffset = pBuffer - (PBYTE)(*ppNegMsg);
    }

cleanup:
    return dwError;
error:
    if(*ppNegMsg)
    {
        LsaFreeMemory(*ppNegMsg);
        (*ppNegMsg) = NULL;
    }
    goto cleanup;
}

/******************************************************************************/
DWORD
NtlmCreateChallengeMessage(
    IN PNTLM_NEGOTIATE_MESSAGE pNegMsg,
    IN PCHAR pServerName,
    IN PCHAR pDomainName,
    IN PCHAR pDnsServerName,
    IN PCHAR pDnsDomainName,
    IN PBYTE pOsVersion,
    OUT PNTLM_CHALLENGE_MESSAGE *ppChlngMsg
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    DWORD dwSize = 0;
    PNTLM_SEC_BUFFER pTargetInfoSecBuffer= NULL;
    PNTLM_TARGET_INFO_BLOCK pTargetInfoBlock = NULL;
    DWORD dwTargetInfoSize = 0;
    PBYTE pBuffer = NULL;
    DWORD dwOptions = 0;
    PBYTE pTrav = NULL;

    // sanity checks
    if(!pNegMsg || !ppChlngMsg)
    {
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_NTLM_ERROR(dwError);
    }

    *ppChlngMsg = NULL;

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
                dwError = LSA_ERROR_INVALID_PARAMETER;
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

    dwError = LsaAllocateMemory(dwSize, (PVOID*)ppChlngMsg);
    BAIL_ON_NTLM_ERROR(dwError);

    memset(*ppChlngMsg, 0, dwSize);

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
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_NTLM_ERROR(dwError);
    }

    // Another sanity check... we need to have at least NTLM or NTLM2 requested
    if(!(pNegMsg->NtlmFlags & NTLM_FLAG_NTLM) &&
       !(pNegMsg->NtlmFlags & NTLM_FLAG_NTLM2))
    {
        dwError = LSA_ERROR_INVALID_PARAMETER;
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
        &((*ppChlngMsg)->NtlmSignature),
        NTLM_SIGNATURE,
        NTLM_SIGNATURE_SIZE
        );

    (*ppChlngMsg)->MessageType = NTLM_CHALLENGE_MSG;

    (*ppChlngMsg)->NtlmFlags = dwOptions;

    if((*ppChlngMsg)->NtlmFlags & NTLM_FLAG_REQUEST_TARGET)
    {
        if(pServerName)
        {
            if((*ppChlngMsg)->NtlmFlags & NTLM_FLAG_UNICODE)
            {
                (*ppChlngMsg)->Target.usLength =
                    strlen(pServerName) * sizeof(WCHAR);
            }
            else
            {
                (*ppChlngMsg)->Target.usLength = strlen(pServerName);
            }

            (*ppChlngMsg)->Target.usMaxLength = (*ppChlngMsg)->Target.usLength;
        }
    }
    else
    {
        (*ppChlngMsg)->Target.usLength = 0;
        (*ppChlngMsg)->Target.usMaxLength = 0;
        (*ppChlngMsg)->Target.dwOffset = 0;
    }

    dwError = NtlmGetRandomBuffer(
        (*ppChlngMsg)->Challenge,
        NTLM_CHALLENGE_SIZE
        );

    BAIL_ON_NTLM_ERROR(dwError);

    // Main structure has been filled, now fill in optional data
    pBuffer = ((PBYTE)(*ppChlngMsg)) + sizeof(NTLM_CHALLENGE_MESSAGE);

    if(pOsVersion || (*ppChlngMsg)->NtlmFlags & NTLM_FLAG_TARGET_INFO)
    {
        // We have to fill in a local context which we will never use
        // ... so make it all zeros
        memset(pBuffer, 0, NTLM_LOCAL_CONTEXT_SIZE);
        pBuffer += NTLM_LOCAL_CONTEXT_SIZE;

        if((*ppChlngMsg)->NtlmFlags & NTLM_FLAG_TARGET_INFO)
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

    if((*ppChlngMsg)->Target.usLength)
    {
        (*ppChlngMsg)->Target.dwOffset = pBuffer - (PBYTE)(*ppChlngMsg);

        if((*ppChlngMsg)->NtlmFlags & NTLM_FLAG_OEM)
        {
            memcpy(pBuffer, pServerName, (*ppChlngMsg)->Target.usLength);
            pBuffer += (*ppChlngMsg)->Target.usLength;
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
    return dwError;
error:
    if(*ppChlngMsg)
    {
        LsaFreeMemory(*ppChlngMsg);
        *ppChlngMsg = NULL;
    }
    goto cleanup;
}

/******************************************************************************/
DWORD
NtlmCreateResponseMessage(
    IN PNTLM_CHALLENGE_MESSAGE pChlngMsg,
    IN DWORD dwResponseType,
    IN PWCHAR pPassword,
    OUT PNTLM_RESPONSE_MESSAGE *ppRespMsg
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    DWORD dwSize = 0;

    // sanity checks
    if(!pChlngMsg || !ppRespMsg)
    {
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_NTLM_ERROR(dwError);
    }

    *ppRespMsg = NULL;

    // calculate the repsonse... we need to get the size first
    switch(dwResponseType)
    {
    case NTLM_RESPONSE_TYPE_LM:
        {
            dwError = NtlmBuildLmResponse();
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
                (PUSHORT)&dwSize,
                NULL
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
            // this is the only variable sized response
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
            dwError = LSA_ERROR_INVALID_PARAMETER;
            BAIL_ON_NTLM_ERROR(dwError);
        }
    }

    dwError = LsaAllocateMemory(dwSize, (PVOID*)(ppRespMsg));
    BAIL_ON_NTLM_ERROR(dwError);

    memset(*ppRespMsg, 0, dwSize);

    // Data is checked and memory is allocated; fill in the structure
    //
    memcpy(&((*ppRespMsg)->NtlmSignature), NTLM_SIGNATURE, NTLM_SIGNATURE_SIZE);
    (*ppRespMsg)->MessageType = NTLM_RESPONSE_MSG;

    // I don't believe we will be calculating/supporting LM auth... skip this
    memset(&((*ppRespMsg)->LmResponse), 0, sizeof(NTLM_SEC_BUFFER));

cleanup:
    return dwError;
error:
    goto cleanup;
}

/******************************************************************************/
DWORD
NtlmValidatResponseMessage(
    IN PNTLM_RESPONSE_MESSAGE pAuthMsg
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;

    // sanity check
    if(!pAuthMsg)
    {
    }

    return dwError;
}

/******************************************************************************/
DWORD
NtlmBuildLmResponse()
{
    // I don't believe we plan to support this particular authentication.
    return LSA_ERROR_NOT_SUPPORTED;
}

/******************************************************************************/
DWORD
NtlmBuildNtlmResponse(
    PUSHORT pLength,
    PBYTE pResponse
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;

    if(!pLength)
    {
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_NTLM_ERROR(dwError);
    }

    // if an output buffer is not supplied or if it is too small,
    // error out, but report the size we would need.
    if(!pResponse || *pLength < NTLM_RESPONSE_SIZE_NTLM)
    {
        dwError = LSA_ERROR_INSUFFICIENT_BUFFER;
        *pLength = NTLM_RESPONSE_SIZE_NTLM;
        BAIL_ON_NTLM_ERROR(dwError);
    }



cleanup:
    return dwError;
error:
    goto cleanup;
}

/******************************************************************************/
DWORD
NtlmBuildNtlmV2Response()
{
    DWORD dwError = LSA_ERROR_NOT_SUPPORTED;
    return dwError;
}

/******************************************************************************/
DWORD
NtlmBuildLmV2Response()
{
    DWORD dwError = LSA_ERROR_NOT_SUPPORTED;
    return dwError;
}

/******************************************************************************/
DWORD
NtlmBuildNtlm2Response()
{
    DWORD dwError = LSA_ERROR_NOT_SUPPORTED;
    return dwError;
}

/******************************************************************************/
DWORD
NtlmBuildAnonymousResponse()
{
    DWORD dwError = LSA_ERROR_NOT_SUPPORTED;
    return dwError;
}
