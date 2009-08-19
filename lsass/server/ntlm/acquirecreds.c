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
 *        acquirecreds.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        AcquireCredentialsHandle client wrapper API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
 */

#include "ntlmsrvapi.h"

DWORD
NtlmServerAcquireCredentialsHandle(
    IN LWMsgAssoc* pAssoc,
    IN SEC_CHAR *pszPrincipal,
    IN SEC_CHAR *pszPackage,
    IN DWORD fCredentialUse,
    IN PLUID pvLogonID,
    IN PVOID pAuthData,
    OUT PNTLM_CRED_HANDLE phCredential,
    OUT PTimeStamp ptsExpiry
    )
{
    DWORD dwError = 0;
    PSEC_WINNT_AUTH_IDENTITY pSecWinAuthData = pAuthData;
    LSA_CRED_HANDLE LsaCredHandle = INVALID_LSA_CRED_HANDLE;
    NTLM_CRED_HANDLE NtlmCredHandle = INVALID_NTLM_CRED_HANDLE;
    PNTLM_CREDENTIALS pNtlmCreds = NULL;
    PSTR pUserName = NULL;
    PSTR pPassword = NULL;
    DWORD dwInvalidUid = -1;
    PSTR pServerName = NULL;
    PSTR pDomainName = NULL;
    PSTR pDnsServerName = NULL;
    PSTR pDnsDomainName = NULL;


    // While it is true that 0 is the id for root, for now we don't store root
    // credentials in our list so we can use it as an invalid value
    uid_t Uid = (uid_t)0;
    gid_t Gid = (gid_t)0;

    *phCredential = NULL;
    *ptsExpiry = 0;

    // For the moment, we're not going to worry about fCredentialUse... it
    // will not effect anything at this point (though we do want to track the
    // information it provides).

    if(strcmp("NTLM", pszPackage))
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LW_ERROR(dwError);
    }

    if(fCredentialUse == NTLM_CRED_OUTBOUND)
    {
        dwError = NtlmGetProcessSecurity(pAssoc, &Uid, &Gid);
        BAIL_ON_LW_ERROR(dwError);

        if(!pSecWinAuthData)
        {
            LsaCredHandle = LsaGetCredential(Uid);

            if(LsaCredHandle == INVALID_LSA_CRED_HANDLE)
            {
                dwError = LW_ERROR_NO_CRED;
                BAIL_ON_LW_ERROR(dwError);
            }
        }
        else
        {
            dwError = LsaAllocateMemory(
                pSecWinAuthData->PasswordLength + 1,
                OUT_PPVOID(&pPassword));
            BAIL_ON_LW_ERROR(dwError);

            memcpy(
                pPassword,
                pSecWinAuthData->Password,
                pSecWinAuthData->PasswordLength);

            if(pSecWinAuthData->UserLength)
            {
                dwError = LsaAllocateMemory(
                    pSecWinAuthData->UserLength + 1,
                    OUT_PPVOID(&pUserName));
                BAIL_ON_LW_ERROR(dwError);

                memcpy(
                    pUserName,
                    pSecWinAuthData->User,
                    pSecWinAuthData->UserLength);
            }
            else if(pszPrincipal)
            {
                dwError = LsaAllocateString(pszPrincipal, &pUserName);
                BAIL_ON_LW_ERROR(dwError);
            }
            else
            {
                dwError = LW_ERROR_INVALID_PARAMETER;
                BAIL_ON_LW_ERROR(dwError);
            }

            dwError = LsaAddCredential(
                pUserName,
                pPassword,
                &dwInvalidUid,
                &LsaCredHandle);
            BAIL_ON_LW_ERROR(dwError);
        }
    }

    dwError = NtlmGetNameInformation(
        &pServerName,
        &pDomainName,
        &pDnsServerName,
        &pDnsDomainName);
    BAIL_ON_LW_ERROR(dwError);

    dwError = NtlmCreateCredentials(
        &LsaCredHandle,
        fCredentialUse,
        pServerName,
        pDomainName,
        pDnsServerName,
        pDnsDomainName,
        &pNtlmCreds);
    BAIL_ON_LW_ERROR(dwError);

    NtlmAddCredential(pNtlmCreds, &NtlmCredHandle);

cleanup:
    LW_SAFE_FREE_STRING(pPassword);
    LW_SAFE_FREE_STRING(pUserName);
    LW_SAFE_FREE_STRING(pServerName);
    LW_SAFE_FREE_STRING(pDomainName);
    LW_SAFE_FREE_STRING(pDnsServerName);
    LW_SAFE_FREE_STRING(pDnsDomainName);

    *phCredential = NtlmCredHandle;

    return(dwError);
error:
    LsaReleaseCredential(LsaCredHandle);
    LsaCredHandle = NULL;

    // The cred may or may not have made it in to the list... be sure to not
    // double free it
    if (NtlmCredHandle)
    {
        NtlmReleaseCredential(NtlmCredHandle);
    }
    else if(pNtlmCreds)
    {
        NtlmFreeCredentials(pNtlmCreds);
    }

    *ptsExpiry = 0;
    goto cleanup;
}

DWORD
NtlmGetNameInformation(
    PSTR* ppszServerName,
    PSTR* ppszDomainName,
    PSTR* ppszDnsServerName,
    PSTR* ppszDnsDomainName
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    CHAR FullDomainName[HOST_NAME_MAX];
    PCHAR pSymbol = NULL;
    PSTR pServerName = NULL;
    PSTR pDomainName = NULL;
    PSTR pDnsServerName = NULL;
    PSTR pDnsDomainName = NULL;
    PSTR pName = NULL;
    struct hostent* pHost = NULL;
    DWORD dwHostSize = 0;

    *ppszServerName = NULL;
    *ppszDomainName = NULL;
    *ppszDnsServerName = NULL;
    *ppszDnsDomainName = NULL;

    dwError = gethostname(FullDomainName, HOST_NAME_MAX);
    if(dwError)
    {
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_LW_ERROR(dwError);
    }

    // There must be a better way to get this information.
    pHost = gethostbyname(FullDomainName);
    if(pHost)
    {
        dwHostSize = strlen((PSTR)pHost->h_name);

        memcpy(
            FullDomainName,
            (PSTR)pHost->h_name,
            HOST_NAME_MAX < dwHostSize ? HOST_NAME_MAX : dwHostSize);
    }

    // This is a horrible fall back, but it's all we've got
    pName = (PSTR)FullDomainName;

    dwError = LsaAllocateString(pName, &pDnsServerName);
    BAIL_ON_LW_ERROR(dwError);

    pSymbol = strchr(pName, '.');

    if(pSymbol)
    {
        *pSymbol = '\0';
    }

    dwError = LsaAllocateString(pName, &pServerName);
    BAIL_ON_LW_ERROR(dwError);

    LwStrToUpper(pServerName);

    if(pSymbol)
    {
        pSymbol++;
        pName = (PSTR)pSymbol;
    }

    dwError = LsaAllocateString(pName, &pDnsDomainName);
    BAIL_ON_LW_ERROR(dwError);

    pSymbol = strchr(pName, '.');

    if(pSymbol)
    {
        *pSymbol = '\0';
    }

    dwError = LsaAllocateString(pName, &pDomainName);
    BAIL_ON_LW_ERROR(dwError);

    LwStrToUpper(pDomainName);

cleanup:
    *ppszServerName = pServerName;
    *ppszDomainName = pDomainName;
    *ppszDnsServerName = pDnsServerName;
    *ppszDnsDomainName = pDnsDomainName;
    return dwError;
error:
    LW_SAFE_FREE_STRING(pServerName);
    LW_SAFE_FREE_STRING(pDomainName);
    LW_SAFE_FREE_STRING(pDnsServerName);
    LW_SAFE_FREE_STRING(pDnsDomainName);
    goto cleanup;
}
