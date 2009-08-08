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
 *        credentials.c
 *
 * Abstract:
 *        NTLM credential wrapper
 *
 * Authors:
 *
 */

#include "ntlmsrvapi.h"

static NTLM_CRED_STATE gCredState;

/******************************************************************************/
VOID
NtlmInitializeCredentialsDatabase(
    VOID
    )
{
    gCredState.NtlmCredListLock = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;

    LsaListInit(&gCredState.NtlmCredList);
}

/******************************************************************************/
VOID
NtlmShutdownCredentialsDatabase(
    VOID
    )
{
    BOOL bInLock = FALSE;
    PNTLM_CREDENTIALS pCreds = NULL;
    PLSA_LIST_LINKS pCredListEntry = NULL;

    ENTER_NTLM_CREDS_LIST(bInLock);

    // sweep the context list
    while(!LsaListIsEmpty(&gCredState.NtlmCredList))
    {
        pCredListEntry = LsaListRemoveHead(&gCredState.NtlmCredList);
        pCreds = LW_STRUCT_FROM_FIELD(
            pCredListEntry,
            NTLM_CREDENTIALS,
            ListEntry);

        NtlmFreeCredentials(pCreds);
    }

    LEAVE_NTLM_CREDS_LIST(bInLock);
}

/******************************************************************************/
VOID
NtlmAddCredential(
    IN PNTLM_CREDENTIALS pCred,
    OUT PNTLM_CRED_HANDLE pCredHandle
    )
{
    BOOL bInLock = FALSE;

    ENTER_NTLM_CREDS_LIST(bInLock);

        LsaListInsertBefore(&gCredState.NtlmCredList, &pCred->ListEntry);

    LEAVE_NTLM_CREDS_LIST(bInLock);

    *pCredHandle = pCred;

    return;
}

/******************************************************************************/
VOID
NtlmReleaseCredential(
    IN NTLM_CRED_HANDLE hCred
    )
{
    BOOL bInLock = FALSE;
    PNTLM_CREDENTIALS pCreds = hCred;

    if(hCred && hCred != INVALID_NTLM_CRED_HANDLE)
    {
        ENTER_NTLM_CREDS_LIST(bInLock);

            pCreds->nRefCount--;

            LW_ASSERT(pCreds->nRefCount >= 0);

            if (!(pCreds->nRefCount))
            {
                LsaListRemove(&pCreds->ListEntry);
                NtlmFreeCredentials(pCreds);
            }

        LEAVE_NTLM_CREDS_LIST(bInLock);
    }
    return;
}

/******************************************************************************/
DWORD
NtlmCreateCredentials(
    IN PLSA_CRED_HANDLE pLsaCredHandle,
    IN DWORD dwDirection,
    IN PSTR pServerName,
    IN PSTR pDomainName,
    IN PSTR pDnsServerName,
    IN PSTR pDnsDomainName,
    OUT PNTLM_CREDENTIALS* ppNtlmCreds
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PNTLM_CREDENTIALS pCreds = NULL;

    if (!ppNtlmCreds ||
        !pServerName ||
        !pDomainName ||
        !pDnsServerName ||
        !pDnsDomainName)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LW_ERROR(dwError);
    }

    *ppNtlmCreds = NULL;

    dwError = LsaAllocateMemory(sizeof(*pCreds), OUT_PPVOID(&pCreds));
    BAIL_ON_LW_ERROR(dwError);

    dwError = LsaAllocateString(
        pServerName,
        &pCreds->pszServerName);
    BAIL_ON_LW_ERROR(dwError);

    dwError = LsaAllocateString(
        pDomainName,
        &pCreds->pszDomainName);
    BAIL_ON_LW_ERROR(dwError);

    dwError = LsaAllocateString(
        pDnsServerName,
        &pCreds->pszDnsServerName);
    BAIL_ON_LW_ERROR(dwError);

    dwError = LsaAllocateString(
        pDnsDomainName,
        &pCreds->pszDnsDomainName);
    BAIL_ON_LW_ERROR(dwError);

    pCreds->CredHandle = *pLsaCredHandle;
    pCreds->nRefCount = 1;

    LsaReferenceCredential(pCreds->CredHandle);

cleanup:
    *ppNtlmCreds = pCreds;
    return dwError;
error:
    if(pCreds)
    {
        LW_SAFE_FREE_STRING(pCreds->pszServerName);
        LW_SAFE_FREE_STRING(pCreds->pszDomainName);
        LW_SAFE_FREE_STRING(pCreds->pszDnsServerName);
        LW_SAFE_FREE_STRING(pCreds->pszDnsDomainName);

        if(pCreds->CredHandle)
        {
            LsaReleaseCredential(pCreds->CredHandle);
        }
    }

    LW_SAFE_FREE_MEMORY(pCreds);
    goto cleanup;
}

/******************************************************************************/
VOID
NtlmGetCredentialInfo(
    IN NTLM_CRED_HANDLE CredHandle,
    OUT OPTIONAL PCSTR* pszUserName,
    OUT OPTIONAL PCSTR* pszPassword,
    OUT OPTIONAL PDWORD pUid,
    OUT OPTIONAL PCSTR* pszServerName,
    OUT OPTIONAL PCSTR* pszDomainName,
    OUT OPTIONAL PCSTR* pszDnsServerName,
    OUT OPTIONAL PCSTR* pszDnsDomainName
    )
{
    if(pszServerName)
    {
        *pszServerName = NULL;
    }
    if(pszDomainName)
    {
        *pszDomainName = NULL;
    }
    if(pszDnsServerName)
    {
        *pszDnsServerName = NULL;
    }
    if(pszDnsDomainName)
    {
        *pszDnsDomainName = NULL;
    }

    if(CredHandle != INVALID_NTLM_CRED_HANDLE)
    {
        PNTLM_CREDENTIALS pCred = CredHandle;
        BOOL bInLock = FALSE;

        ENTER_NTLM_CREDS_LIST(bInLock);

        if(pszServerName)
        {
            *pszServerName = pCred->pszServerName;
        }

        if(pszDomainName)
        {
            *pszDomainName = pCred->pszDomainName;
        }

        if(pszDnsServerName)
        {
            *pszDnsServerName = pCred->pszDnsServerName;
        }

        if(pszDnsDomainName)
        {
            *pszDnsDomainName = pCred->pszDnsDomainName;
        }

        LsaGetCredentialInfo(
            pCred->CredHandle,
            pszUserName,
            pszPassword,
            pUid);

        LEAVE_NTLM_CREDS_LIST(bInLock);
    }

    return;
}

/******************************************************************************/
VOID
NtlmReferenceCredential(
    IN NTLM_CRED_HANDLE hCredential
    )
{
    BOOL bInLock = FALSE;
    PNTLM_CREDENTIALS pCred = NULL;

    if(hCredential != INVALID_NTLM_CRED_HANDLE)
    {
        pCred = hCredential;

        ENTER_NTLM_CREDS_LIST(bInLock);

        pCred->nRefCount++;

        LEAVE_NTLM_CREDS_LIST(bInLock);
    }
}

/******************************************************************************/
VOID
NtlmFreeCredentials(
    IN PNTLM_CREDENTIALS pCreds
    )
{
    LsaReleaseCredential(pCreds->CredHandle);

    LW_SAFE_FREE_STRING(pCreds->pszDomainName);
    LW_SAFE_FREE_STRING(pCreds->pszServerName);
    LW_SAFE_FREE_STRING(pCreds->pszDnsDomainName);
    LW_SAFE_FREE_STRING(pCreds->pszDnsServerName);

    return;
}

