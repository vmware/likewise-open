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
 *        authapi.c
 *
 * Abstract:
 *
 *        Credential provider package interfaces
 *        
 * Author: Todd Stecher (2007)
 *
 */
#include "server.h"
#include "auth_test.h"
#include "auth_db.h"
#include "auth_ad.h"

AUTH_PROVIDER g_authProviders[] = { 
    { NULL_LSASTRING, NULL, testProviderInit, NULL, NULL, NULL, NULL },
    { NULL_LSASTRING, NULL, dbProviderInit, NULL, NULL, NULL, NULL },
    { NULL_LSASTRING, NULL, adProviderInit, NULL, NULL, NULL, NULL },
};

PAUTH_PROVIDER testProvider = &g_authProviders[0];

DWORD g_authProviderCount = sizeof(g_authProviders) / sizeof(g_authProviders[0]);

void
NTLMDestroyAuthProviders( void ) 
{
    DWORD dwCount;
    for (dwCount = 0; dwCount < g_authProviderCount; dwCount++) 
        g_authProviders[dwCount].destroyAuthProvider();
}

DWORD
NTLMInitializeAuthProviders( 
    LSA_STRING *hostName, 
    LSA_STRING *hostDomain,
    LSA_STRING *configPath
    )
{
    DWORD dwError;
    DWORD dwCount;
    AUTH_PROVIDER_INITDATA initData;

    ZERO_STRUCT(initData);
    initData.hostName = *hostName;
    initData.hostDomain = *hostDomain;
    initData.configPath = *configPath;

    for (dwCount = 0; dwCount < g_authProviderCount; dwCount++) {
        dwError = g_authProviders[dwCount].initAuthProvider(
                        &initData,  
                        &g_authProviders[dwCount]
                        );

        BAIL_ON_NTLM_ERROR(dwError);

        /* debug - show package info */
    }

error:

    return dwError;
}

PAUTH_PROVIDER
NTLMSelectLocalAuthProvider( void )
{
    PAUTH_PROVIDER authProvider = testProvider;
    /* @todo - uids are local - use local DB, possibly nis / sfu */

    return authProvider;
}

PAUTH_PROVIDER
NTLMSelectAuthProvider(
    AUTH_USER *authUser
    )
{
    PAUTH_PROVIDER authProvider = testProvider;
    /* @todo - splitter routine */

    authUser->provider = authProvider;
    return authProvider;
}

PAUTH_PROVIDER
NTLMSelectNamedAuthProvider(
    PLSA_STRING providerName,
    AUTH_USER *authUser
    )
{

    PAUTH_PROVIDER provider = testProvider;

    /* @todo while - compare names */
    authUser->provider = provider;
    return provider;
}

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


void
NTLMDumpAuthUser(
    DWORD lvl,
    PAUTH_USER authUser
    )
{
    if ((db_level & lvl) == 0)
        return;

    DBG(lvl,("user - %ls \n", (wchar_t*)authUser->user.buffer));
    DBG(lvl,("domain - %ls \n", (wchar_t*)authUser->domain.buffer));
    DBG(lvl,("uid - %d \n", authUser->uid));

    /* @todo - dump bytes of password */

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
bool
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
    PAUTH_PROVIDER pAuthProvider;

    ZERO_STRUCT(authUser);

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

    pAuthProvider = NTLMSelectAuthProvider(&authUser);
    if (!pAuthProvider)
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_NO_SUCH_USER);

    memcpy(newAuthUser, &authUser, sizeof(AUTH_USER));
    ZERO_STRUCT(authUser);

    DBG(D_ERROR, ("Created auth user \n"));
    NTLMDumpAuthUser(D_ERROR, newAuthUser);

error:

    NTLMFreeAuthUser(&authUser);
    return ( dwError == 0 );
}

bool
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
NTLMGetAuthUserFromUid(
    uid_t uid,
    PAUTH_USER newAuthUser
    )
{
    PAUTH_PROVIDER authProvider;


    /* @todowalk the providers, asking them if they know about uid */
    authProvider = NTLMSelectLocalAuthProvider();
    return authProvider->userFromUid(uid, newAuthUser);
}
