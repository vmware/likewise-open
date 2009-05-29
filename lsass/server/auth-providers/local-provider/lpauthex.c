/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software
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
 *        lpauthex.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        AuthenticateUserEx routines
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "includes.h"

/* Forward Declarations */

static DWORD
FillAuthUserInfo(
    PLSA_AUTH_USER_INFO pAuthInfo,
    PLSA_USER_INFO_2 pLsaUserInfo2,
    PCSTR pszMachineName
    );

static DWORD
SidSplitString(
    IN OUT PSTR pszSidString,
    OUT PDWORD pdwRid
    );

static DWORD
AuthenticateNTLMv1(
    PLSA_AUTH_USER_PARAMS pUserParams,
    PLSA_USER_INFO_2 pUserInfo2
    );

static DWORD
AuthenticateNTLMv2(
    PLSA_AUTH_USER_PARAMS pUserParams,
    PLSA_USER_INFO_2 pUserInfo2
    );

static DWORD
CalcNTLMv1SessionKey(
    OUT PLSA_DATA_BLOB pBlob,
    IN PBYTE NTLMHash
    );

static DWORD
CalcNTLMv2SessionKey(
    OUT PLSA_DATA_BLOB pBlob,
    IN PBYTE NTLMHash
    );


/* Code */

/********************************************************
 *******************************************************/

DWORD
LocalAuthenticateUserExInternal(
    HANDLE                hProvider,
    PLSA_AUTH_USER_PARAMS pUserParams,
    PLSA_AUTH_USER_INFO*  ppUserInfo
    )
{
    DWORD    dwError = LSA_ERROR_INTERNAL;
    PCSTR    pszDomain = NULL;
    DWORD    dwUserInfoLevel = 2;
    PSTR     pszAccountName = NULL;
    PLSA_USER_INFO_2 pUserInfo2 = NULL;
    PLSA_AUTH_USER_INFO pUserInfo = NULL;
    BOOLEAN bUsingNTLMv2 = FALSE;

    BAIL_ON_INVALID_POINTER(pUserParams->pszAccountName);

    dwError = LocalCheckForQueryAccess(hProvider);
    BAIL_ON_LSA_ERROR(dwError);

    /* Assume the local domain (localhost) if we don't have one */

    if (pUserParams->pszDomain) {
        pszDomain = pUserParams->pszDomain;
    } else {
        pszDomain = gLPGlobals.pszLocalDomain;
    }

    /* Allow the next provider to continue if we don't handle this domain */

    if (!LocalServicesDomain(pszDomain))
    {
        dwError = LSA_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaAllocateStringPrintf(&pszAccountName,
                                      "%s\\%s",
                                      pszDomain,
                                      pUserParams->pszAccountName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalFindUserByName(hProvider,
                                  pszAccountName,
                                  dwUserInfoLevel,
                                  (PVOID*)&pUserInfo2);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalCheckAccountFlags(pUserInfo2);
    BAIL_ON_LSA_ERROR(dwError);

    /* generate the responses and compare */

    if (LsaDataBlobLength(pUserParams->pass.chap.pNT_resp) == 24)
    {
        bUsingNTLMv2 = FALSE;

        dwError = AuthenticateNTLMv1(pUserParams, pUserInfo2);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        bUsingNTLMv2 = TRUE;

        dwError = AuthenticateNTLMv2(pUserParams, pUserInfo2);
        BAIL_ON_LSA_ERROR(dwError);
    }

    /* Fill in the LSA_AUTH_USER_INF0 data now */

    dwError = LsaAllocateMemory(sizeof(LSA_AUTH_USER_INFO),
                                (PVOID*)&pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = FillAuthUserInfo(pUserInfo, pUserInfo2, pszDomain);
    BAIL_ON_LSA_ERROR(dwError);

    /* Session Key */

    dwError = LsaDataBlobAllocate(&pUserInfo->pSessionKey, 16);
    BAIL_ON_LSA_ERROR(dwError);

    if (bUsingNTLMv2)
    {
        dwError = CalcNTLMv2SessionKey(pUserInfo->pSessionKey,
                                       pUserInfo2->info1.pNTHash);
    } else {
        dwError = CalcNTLMv1SessionKey(pUserInfo->pSessionKey,
                                       pUserInfo2->info1.pNTHash);
    }
    BAIL_ON_LSA_ERROR(dwError);

    *ppUserInfo = pUserInfo;
    pUserInfo = NULL;

cleanup:

    LsaFreeAuthUserInfo(&pUserInfo);

    if (pUserInfo2)
    {
        LsaFreeUserInfo(dwUserInfoLevel, pUserInfo2);
    }

    LSA_SAFE_FREE_MEMORY(pszAccountName);

    return dwError;

error:

    goto cleanup;
}

/********************************************************
 *******************************************************/

static DWORD
AuthenticateNTLMv1(
    PLSA_AUTH_USER_PARAMS pUserParams,
    PLSA_USER_INFO_2 pUserInfo2
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    DWORD    dwError = LSA_ERROR_INTERNAL;
    BYTE     NTResponse[24] = { 0 };
    PBYTE    pChal = NULL;
    PBYTE    pNTresp = NULL;

    pChal = LsaDataBlobBuffer(pUserParams->pass.chap.pChallenge);
    BAIL_ON_INVALID_POINTER(pChal);

    ntError = NTLMv1EncryptChallenge(pChal,
                                     NULL,     /* ignore LM hash */
                                     pUserInfo2->info1.pNTHash,
                                     NULL,
                                     NTResponse);
    if (ntError != STATUS_SUCCESS) {
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pNTresp = LsaDataBlobBuffer(pUserParams->pass.chap.pNT_resp);
    BAIL_ON_INVALID_POINTER(pNTresp);

    if (memcmp(pNTresp, NTResponse, 24) != 0)
    {
        dwError = LSA_ERROR_PASSWORD_MISMATCH;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LSA_ERROR_SUCCESS;

cleanup:

    return dwError;

error:

    goto cleanup;
}



/********************************************************
 *******************************************************/
static DWORD
AuthenticateNTLMv2(
    PLSA_AUTH_USER_PARAMS pUserParams,
    PLSA_USER_INFO_2 pUserInfo2
    )
{
    DWORD    dwError = LSA_ERROR_PASSWORD_MISMATCH;

    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    return dwError;

error:

    goto cleanup;
}


/********************************************************
 *******************************************************/

static DWORD
CalcNTLMv1SessionKey(
    OUT PLSA_DATA_BLOB pBlob,
    IN PBYTE NTLMHash
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    PBYTE pBuffer = LsaDataBlobBuffer(pBlob);

    MD4(NTLMHash, 16, pBuffer);

    return dwError;
}


/********************************************************
 *******************************************************/

static DWORD
CalcNTLMv2SessionKey(
    OUT PLSA_DATA_BLOB pBlob,
    IN PBYTE NTLMHash
    )
{
    return LSA_ERROR_NOT_IMPLEMENTED;
}

/********************************************************
 *******************************************************/

static DWORD
SidSplitString(
    IN OUT PSTR pszSidString,
    OUT PDWORD pdwRid
    )
{
    DWORD dwError = LSA_ERROR_INTERNAL;
    PSTR p = NULL;
    PSTR q = NULL;
    DWORD dwRid = 0;

    BAIL_ON_INVALID_POINTER(pszSidString);

    p = strrchr(pszSidString, '-');
    if (p == NULL) {
        dwError = LSA_ERROR_INVALID_SID;
        BAIL_ON_LSA_ERROR(dwError);
    }

    /* Get the RID */

    p++;
    dwRid = strtol(p, &q, 10);
    if ((dwRid == 0) || (*q != '\0')) {
        dwError = LSA_ERROR_INVALID_SID;
        BAIL_ON_LSA_ERROR(dwError);
    }

    /* Split the string now that we know the RID is valid */

    *pdwRid = dwRid;

    p--;
    *p = '\0';

    dwError = LSA_ERROR_SUCCESS;

cleanup:

    return dwError;

error:

    goto cleanup;
}

/********************************************************
 *******************************************************/

static DWORD
FillAuthUserInfo(
    OUT PLSA_AUTH_USER_INFO pAuthInfo,
    IN PLSA_USER_INFO_2 pLsaUserInfo2,
    IN PCSTR pszMachineName
    )
{
    DWORD dwError = LSA_ERROR_INTERNAL;

    /* leave user flags empty for now.  But fill in account flags */

    pAuthInfo->dwAcctFlags = ACB_NORMAL;
    if (pLsaUserInfo2->bAccountDisabled) {
        pAuthInfo->dwAcctFlags |= ACB_DISABLED;
    }
    if (pLsaUserInfo2->bAccountExpired) {
        pAuthInfo->dwAcctFlags |= ACB_PW_EXPIRED;
    }
    if (pLsaUserInfo2->bPasswordNeverExpires) {
        pAuthInfo->dwAcctFlags |= ACB_PWNOEXP;
    }

    /* Copy strings */

    dwError = LsaStrDupOrNull(pLsaUserInfo2->info1.pszName,
                              &pAuthInfo->pszAccount);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaStrDupOrNull(pLsaUserInfo2->info1.pszGecos,
                              &pAuthInfo->pszFullName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaStrDupOrNull(pszMachineName,
                              &pAuthInfo->pszDomain);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaStrDupOrNull(pLsaUserInfo2->info1.pszSid,
                              &pAuthInfo->pszDomainSid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = SidSplitString(pAuthInfo->pszDomainSid,
                             &pAuthInfo->dwUserRid);
    BAIL_ON_LSA_ERROR(dwError);

    pAuthInfo->dwPrimaryGroupRid = 544;
    pAuthInfo->dwNumRids = 0;
    pAuthInfo->dwNumSids = 0;

    /* NTLM Session key */


cleanup:

    return dwError;

error:

    goto cleanup;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
