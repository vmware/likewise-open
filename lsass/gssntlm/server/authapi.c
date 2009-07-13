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
#include "auth_inbound.h"
#include "auth_db.h"
#include "auth_ad.h"

AUTH_PROVIDER g_authProviders[] = {
#if defined(ENABLE_TESTGSSPROVIDER)
    { NULL_LSASTRING, NULL_LSASTRING, NULL, testProviderInit, NULL, NULL, NULL, NULL },
#endif /* ENABLE_TESTGSSPROVIDER */
    { NULL_LSASTRING, NULL_LSASTRING, NULL, dbProviderInit, NULL, NULL, NULL, NULL },
    { NULL_LSASTRING, NULL_LSASTRING, NULL, adProviderInit, NULL, NULL, NULL, NULL },
    { NULL_LSASTRING, NULL_LSASTRING, NULL, inboundProviderInit, NULL, NULL, NULL, NULL }
};

#if defined(ENABLE_TESTGSSPROVIDER)
PAUTH_PROVIDER g_inboundProvider = &g_authProviders[3];
#else
PAUTH_PROVIDER g_inboundProvider = &g_authProviders[2];
#endif /* ENABLE_TESTGSSPROVIDER */

PAUTH_PROVIDER g_localAuthProvider = NULL;

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
    DWORD dwError = LW_ERROR_SUCCESS;
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


DWORD
NTLMCreateInboundOnlyAuthUser(
    uid_t uid,
    PAUTH_USER authUser
)
{
    DWORD dwError = 0;
    
    dwError = g_inboundProvider->userFromUid(uid, authUser);

    return dwError;
}


PAUTH_PROVIDER
NTLMSelectLocalAuthProvider( void )
{
    DWORD dwError = 0;
    LSA_STRING workstation = {0};

    if (!g_localAuthProvider)
    {
        DWORD x = 0;
        
        dwError = NTLMGetWorkstationName(&workstation);
        BAIL_ON_NTLM_ERROR(dwError);
    
        for (x = 0; x < g_authProviderCount; x++)
        {
            if (LsaEqualLsaStringNoCase(
                    &g_authProviders[x].realm,
                    &workstation
                    ))
            {
                g_localAuthProvider = &g_authProviders[x];
                break;
            }
        }
    }

error:

    NTLM_SAFE_FREE(workstation.buffer);
    
    return g_localAuthProvider;
}

PAUTH_PROVIDER
NTLMSelectAuthProvider(
    AUTH_USER *authUser
    )
{
    DWORD x;
    PAUTH_PROVIDER authProvider = NULL;

    if (authUser->provider)
        return authUser->provider;

    /* supplied creds == who cares :) */
    if (authUser->dwFlags & AUTH_USER_PASSWORD_SUPPLIED)
    {
        authUser->provider = NTLMSelectLocalAuthProvider();
        authProvider = authUser->provider;
        return authProvider;
    }

    for (x = 0; x < g_authProviderCount; x++)
    {
        if (LsaEqualLsaStringNoCase(
                &g_authProviders[x].realm,
                &authUser->domain
                ))
        {
            authProvider = &g_authProviders[x];
            authUser->provider = authProvider;
            break;
        }
    }


    /* @todo - if none are a match, try the AD provider */
    /* @todo - domain cache to avoid hammering DC */

    return authProvider;

}

PAUTH_PROVIDER
NTLMSelectNamedAuthProvider(
    PLSA_STRING providerName,
    AUTH_USER *authUser
    )
{
    DWORD x;
    PAUTH_PROVIDER authProvider = NULL;

    for (x = 0; x < g_authProviderCount; x++)
    {
        if (LsaEqualLsaStringNoCase(
                &g_authProviders[x].providerName,
                providerName
                ))
        {
            authProvider = &g_authProviders[x];
            authUser->provider = authProvider;
            break;
        }
    }

    authUser->provider = authProvider;
    return authProvider;
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
