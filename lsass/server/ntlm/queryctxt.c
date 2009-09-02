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
 *        queryctxt.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        QueryContextAttributes client wrapper API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
 */

#include "ntlmsrvapi.h"

DWORD
NtlmServerQueryContextAttributes(
    IN PNTLM_CONTEXT_HANDLE phContext,
    IN DWORD ulAttribute,
    OUT PVOID pBuffer
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PSecPkgContext pContext = (PSecPkgContext)pBuffer;

    switch(ulAttribute)
    {
    case SECPKG_ATTR_NAMES:
        dwError = NtlmServerQueryCtxtNameAttribute(
            phContext,
            &pContext->pNames);
        BAIL_ON_LSA_ERROR(dwError);
        break;
    case SECPKG_ATTR_SESSION_KEY:
        dwError = NtlmServerQueryCtxtSessionKeyAttribute(
            phContext,
            &pContext->pSessionKey);
        BAIL_ON_LSA_ERROR(dwError);
        break;
    case SECPKG_ATTR_SIZES:
        dwError = NtlmServerQueryCtxtSizeAttribute(
            phContext,
            &pContext->pSizes);
        BAIL_ON_LSA_ERROR(dwError);
        break;
    case SECPKG_ATTR_ACCESS_TOKEN:
    case SECPKG_ATTR_AUTHORITY:
    case SECPKG_ATTR_CLIENT_SPECIFIED_TARGET:
    case SECPKG_ATTR_DCE_INFO:
    case SECPKG_ATTR_FLAGS:
    case SECPKG_ATTR_KEY_INFO:
    case SECPKG_ATTR_LAST_CLIENT_TOKEN_STATUS:
    case SECPKG_ATTR_LIFESPAN:
    case SECPKG_ATTR_LOCAL_CRED:
    case SECPKG_ATTR_NATIVE_NAMES:
    case SECPKG_ATTR_NEGOTIATION_INFO:
    case SECPKG_ATTR_PACKAGE_INFO:
    case SECPKG_ATTR_PASSWORD_EXPIRY:
    case SECPKG_ATTR_ROOT_STORE:
    case SECPKG_ATTR_TARGET_INFORMATION:
        dwError = LW_ERROR_NOT_IMPLEMENTED;
        BAIL_ON_LSA_ERROR(dwError);
        break;
    default:
        dwError = LW_ERROR_INVALID_ATTRIBUTE_VALUE;
        BAIL_ON_LSA_ERROR(dwError);
        break;
    }

cleanup:
    return dwError;
error:
    goto cleanup;
}

DWORD
NtlmServerQueryCtxtNameAttribute(
    IN PNTLM_CONTEXT_HANDLE phContext,
    OUT PSecPkgContext_Names *ppNames
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PVOID pMessage = NULL;
    NTLM_STATE State = NtlmStateBlank;
    DWORD dwNegFlags = 0;
    SEC_CHAR* pUserName = NULL;
    SEC_CHAR* pDomainName = NULL;
    SEC_CHAR* pFullName = NULL;
    PSecPkgContext_Names pName = NULL;

    *ppNames = NULL;

    dwError = LwAllocateMemory(sizeof(*pName), OUT_PPVOID(&pName));
    BAIL_ON_LSA_ERROR(dwError);

    NtlmGetContextInfo(
        *phContext,
        &State,
        &dwNegFlags,
        &pMessage,
        NULL,
        NULL,
        NULL);

    if(State != NtlmStateResponse)
    {
        dwError = LW_ERROR_INVALID_CONTEXT;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = NtlmGetUserNameFromResponse(
        pMessage,
        dwNegFlags & NTLM_FLAG_UNICODE,
        &pUserName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = NtlmGetDomainNameFromResponse(
        pMessage,
        dwNegFlags & NTLM_FLAG_UNICODE,
        &pDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateStringPrintf(
        &pFullName,
        "%s\\%s",
        pDomainName,
        pUserName);
    BAIL_ON_LSA_ERROR(dwError);

    pName->pUserName = pFullName;

cleanup:
    LW_SAFE_FREE_MEMORY(pUserName);
    LW_SAFE_FREE_MEMORY(pDomainName);
    *ppNames = pName;
    return dwError;
error:
    LW_SAFE_FREE_MEMORY(pName);
    LW_SAFE_FREE_MEMORY(pFullName);
    goto cleanup;
}

DWORD
NtlmServerQueryCtxtSessionKeyAttribute(
    IN PNTLM_CONTEXT_HANDLE phContext,
    OUT PSecPkgContext_SessionKey *ppSessionKey
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    NTLM_STATE State = NtlmStateBlank;
    PBYTE pKey = NULL;
    PSecPkgContext_SessionKey pSessionKey = NULL;

    *ppSessionKey = NULL;

    dwError = LwAllocateMemory(sizeof(*pSessionKey), OUT_PPVOID(&pSessionKey));
    BAIL_ON_LSA_ERROR(dwError);

    NtlmGetContextInfo(
        *phContext,
        &State,
        NULL,
        NULL,
        NULL,
        &pKey,
        NULL);

    if(State != NtlmStateResponse)
    {
        dwError = LW_ERROR_INVALID_CONTEXT;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwAllocateMemory(
        NTLM_SESSION_KEY_SIZE,
        OUT_PPVOID(&pSessionKey->SessionKey));
    BAIL_ON_LSA_ERROR(dwError);

    memcpy(pSessionKey->SessionKey, pKey, NTLM_SESSION_KEY_SIZE);
    pSessionKey->SessionKeyLength = NTLM_SESSION_KEY_SIZE;

cleanup:
    *ppSessionKey = pSessionKey;
    return dwError;
error:
    if(pSessionKey)
    {
        LW_SAFE_FREE_MEMORY(pSessionKey->SessionKey);
    }
    LW_SAFE_FREE_MEMORY(pSessionKey);
    goto cleanup;
}

DWORD
NtlmServerQueryCtxtSizeAttribute(
    IN PNTLM_CONTEXT_HANDLE phContext,
    OUT PSecPkgContext_Sizes *ppSizes
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PSecPkgContext_Sizes pSizes = NULL;

    *ppSizes = NULL;

    dwError = LwAllocateMemory(sizeof(*pSizes), OUT_PPVOID(&pSizes));
    BAIL_ON_LSA_ERROR(dwError);

    // The Challenge message is easily the largest token we send
    pSizes->cbMaxToken =
        sizeof(NTLM_CHALLENGE_MESSAGE) +
        NTLM_LOCAL_CONTEXT_SIZE +
        sizeof(NTLM_SEC_BUFFER) +
        NTLM_WIN_SPOOF_SIZE +
        (HOST_NAME_MAX * 5) +
        (sizeof(NTLM_TARGET_INFO_BLOCK) * 5);

    pSizes->cbMaxSignature = NTLM_SIGNATURE_SIZE;
    pSizes->cbBlockSize = 1;
    pSizes->cbSecurityTrailer = 4;

cleanup:
    *ppSizes = pSizes;
    return dwError;
error:
    LW_SAFE_FREE_MEMORY(pSizes);
    goto cleanup;
}

