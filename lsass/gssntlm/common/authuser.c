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
 *        authuser.c
 *
 * Abstract:
 *
 *        Credential provider package interfaces
 *        
 * Author: Todd Stecher (2007)
 *
 */
#include "ntlmcommon.h"

void
NTLMFreeAuthUser( 
    PAUTH_USER authUser
    )
{
    if (!authUser)
        return;

    LsaFreeLsaString(&authUser->user);
    LsaFreeLsaString(&authUser->domain);
}


DWORD
NTLMCopyAuthUser(
    PAUTH_USER dest,
    PAUTH_USER src
    )
{
    DWORD dwError;
    ZERO_STRUCTP(dest);

    dwError = LsaCopyLsaString(
                &dest->user,
                &src->user
                );
    BAIL_ON_NTLM_ERROR(dwError);
    
    dwError = LsaCopyLsaString(
                &dest->domain,
                &src->domain
                );
    BAIL_ON_NTLM_ERROR(dwError);

    memcpy(&dest->ntOWF, &src->ntOWF, sizeof(NTLM_OWF)); 
    dest->dwFlags = src->dwFlags;
    dest->provider = src->provider;

    return LSA_ERROR_SUCCESS;

error:

    NTLMFreeAuthUser(dest);
    ZERO_STRUCTP(dest);
    return dwError;
}

/*
 * @brief NTLMInitializeAuthUser
 *
 * Creates an auth_user structure - the basis of identity
 * in gssntlm server.
 * 
 * @param user - user name
 * @param domain - domain name/S
 * @param optional password - password 
 * @param uid - uid of calling user
 * @param newAuthUser - free with NTLMFreeAuthUser()
 *
 * @returns true / false
 *
 * @todo - this may be moved to auth provider method 
 */
DWORD
NTLMInitializeAuthUser(
    PLSA_STRING user,
    PLSA_STRING domain,
    PLSA_STRING password,
    uid_t uid,
    PAUTH_USER newAuthUser
    ) 
{
    DWORD dwError;
    AUTH_USER authUser;

    ZERO_STRUCT(authUser);


    /* @todo - only root / su can specify UID */

    dwError = LsaCopyLsaStringNullTerm(&authUser.user, user);
    BAIL_ON_NTLM_ERROR(dwError);
    
    dwError = LsaCopyLsaStringNullTerm(&authUser.domain, domain);
    BAIL_ON_NTLM_ERROR(dwError);

    if (password) {

        dwError = NTLMComputeNTOWF(
                        password,
                        authUser.ntOWF
                        );

        BAIL_ON_NTLM_ERROR(dwError);
        authUser.dwFlags |= AUTH_USER_PASSWORD_SUPPLIED;
    }

    memcpy(newAuthUser, &authUser, sizeof(AUTH_USER));
    ZERO_STRUCT(authUser);

error:

    NTLMFreeAuthUser(&authUser);
    return dwError;
}

BOOLEAN
NTLMCompareAuthUsers(
    AUTH_USER *u1,
    AUTH_USER *u2
    )
{

    if (u1->uid != u2->uid)
        return false;

    /* users names are always case insensitive*/
    if (!LsaEqualLsaStringNoCase(
            &u1->user,
            &u2->user
            )) 
        return false;

    /* domain names are always case insensitive*/
    /* @todo NB --> fqdn map? */
    if (!LsaEqualLsaStringNoCase(
            &u1->domain,
            &u2->domain
            )) 
        return false;
    /* LM?  not needed */
    return (memcmp( (const void*) u1->ntOWF,
                    (const void*) u2->ntOWF, 
                    16) == 0);
}


DWORD
NTLMMarshalAuthUser(
    PAUTH_USER authUser,
    PSEC_BUFFER marshaledData
)
{
    DWORD len = sizeof(AUTH_USER);
    ULONG bufofs = sizeof(AUTH_USER);
    ULONG ofs = 0;

    len += authUser->user.max + authUser->domain.max;
    marshaledData->buffer = NTLMAllocateMemory(len);
    if (!marshaledData->buffer)
        return LSA_ERROR_OUT_OF_MEMORY;

    marshaledData->length = marshaledData->maxLength = len;

    memcpy(marshaledData->buffer, authUser, sizeof(AUTH_USER));

    NTLMPutLsaString(&authUser->user, marshaledData->buffer, &bufofs, &ofs);
    NTLMPutLsaString(&authUser->domain, marshaledData->buffer, &bufofs, &ofs);

    return LSA_ERROR_SUCCESS;
}

DWORD
NTLMUnMarshalAuthUser(
    PSEC_BUFFER marshaledData,
    PAUTH_USER authUser
)
{

    DWORD dwError;
    ULONG ofs = 0;

    if (marshaledData->length < sizeof(AUTH_USER))
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_INSUFFICIENT_BUFFER);

    memcpy(authUser, marshaledData->buffer, sizeof(AUTH_USER));

    dwError = NTLMGetLsaString(&authUser->user, marshaledData, &ofs);
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NTLMGetLsaString(&authUser->domain, marshaledData, &ofs);
    BAIL_ON_NTLM_ERROR(dwError);

    authUser->dwFlags |= AUTH_USER_PASSWORD_SUPPLIED;

error:

    return dwError;
}


