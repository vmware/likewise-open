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
    OUT PLSA_CRED_HANDLE phCredential,
    OUT PTimeStamp ptsExpiry
    )
{
    DWORD dwError = 0;
    PSEC_WINNT_AUTH_IDENTITY pSecWinAuthData = pAuthData;
    LSA_CRED_HANDLE CredHandle = NULL;
    PCHAR pPassword = NULL;

    // While it is true that 0 is the id for root, for now we don't store root
    // credentials in our list so we can use it as an invalid value
    uid_t Uid = (uid_t)0;
    gid_t Gid = (gid_t)0;

    *phCredential = NULL;
    memset(ptsExpiry, 0, sizeof(TimeStamp));

    // For the moment, we're not going to worry about fCredentialUse... it
    // will not effect anything at this point (though we do want to track the
    // information it provides).

    if(strcmp("NTLM", pszPackage))
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_NTLM_ERROR(dwError);
    }

    dwError = NtlmGetProcessSecurity(pAssoc, &Uid, &Gid);
    BAIL_ON_LW_ERROR(dwError);

    if(!pSecWinAuthData)
    {
        CredHandle = LsaGetCredential(Uid);

        if(!CredHandle)
        {
            dwError = LW_ERROR_NO_CRED;
            BAIL_ON_LW_ERROR(dwError);
        }
    }
    else
    {
        dwError = LsaAllocateMemory(
            pSecWinAuthData->PasswordLength + 1,
            (PVOID*)(PVOID)&pPassword);
        BAIL_ON_LW_ERROR(dwError);

        memcpy(
            pPassword,
            pSecWinAuthData->Password,
            pSecWinAuthData->PasswordLength);

        dwError = LsaAddCredential(pszPrincipal, pPassword, &Uid, &CredHandle);
        BAIL_ON_LW_ERROR(dwError);
    }

cleanup:
    if(pPassword)
    {
        LsaFreeMemory(pPassword);
    }

    *phCredential = CredHandle;

    return(dwError);
error:
    if(CredHandle)
    {
        LsaReleaseCredential(CredHandle);
        CredHandle = NULL;
    }
    memset(ptsExpiry, 0, sizeof(TimeStamp));
    goto cleanup;
}

DWORD
NtlmGetProcessSecurity(
    IN LWMsgAssoc* pAssoc,
    OUT uid_t* pUid,
    OUT gid_t* pGid
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    LWMsgSecurityToken* token = NULL;
    uid_t uid = (uid_t) 0;
    gid_t gid = (gid_t) 0;

    dwError = MAP_LWMSG_ERROR(
        lwmsg_assoc_get_peer_security_token(pAssoc, &token));
    BAIL_ON_LW_ERROR(dwError);

    if (token == NULL || strcmp(lwmsg_security_token_get_type(token), "local"))
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LW_ERROR(dwError);
    }

    dwError = MAP_LWMSG_ERROR(lwmsg_local_token_get_eid(token, &uid, &gid));
    BAIL_ON_LW_ERROR(dwError);

cleanup:
    *pUid = uid;
    *pGid = gid;

    return dwError;
error:
    uid = (uid_t) 0;
    gid = (gid_t) 0;
    goto cleanup;

}
