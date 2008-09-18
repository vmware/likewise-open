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
 *        auth_inbound.c
 *
 * Abstract:
 *
 *       inbound auth provider - no user
 *
 * Author: Todd Stecher (2007)
 *
 */
#include "server.h"

#define INBOUND_USER            "INBOUNDONLY"
#define INBOUND_DOMAIN          "INBOUNDONLY"
#define INBOUND_PASSWORD        "INBOUNDONLY"
#define INBOUND_PROVIDERNAME    "inboundprovider"
#define INBOUND_UID             99999


AUTH_USER inboundUser;
NTLM_OWF owf;

DWORD
inboundProviderUserFromUid(
    uid_t uid,
    PAUTH_USER authUser
    )
{
    DWORD dwError;
    dwError = NTLMCopyAuthUser(authUser, &inboundUser);
    BAIL_ON_NTLM_ERROR(dwError);

    authUser->uid = uid;

error:

    return LSA_ERROR_SUCCESS;
}

void
inboundProviderFreeAuthUser(
    PAUTH_USER authUser
    )
{
    /* do nothing */
    return;
}

DWORD
inboundProviderGetUserInfo(
    PAUTH_USER user,
    PAUTH_INFO *userInfo
    )
{
    return LSA_ERROR_NOT_IMPLEMENTED;
}


static void
inboundProviderFreeUserInfo(
    PAUTH_INFO userInfo
)
{
    if (!userInfo)
        return;

    /* LsaFreeLsaString(userInfo->authUser.userName); */
    /* LsaFreeLsaString(userInfo->authUser.domain); */
    /* todo - clear out OWFs - fill w/ 0s */
}


void
inboundProviderDestroy(
    void
    )
{
    return;
}

DWORD
inboundUserInit(
    void
    )
{
    DWORD dwError = 0;

    LSA_STRING user = {0};
    LSA_STRING domain = {0};
    LSA_STRING password = {0};

    dwError = LsaInitializeLsaStringA(
                INBOUND_USER,
                &user);
    BAIL_ON_NTLM_ERROR(dwError);


    dwError = LsaInitializeLsaStringA(
                INBOUND_DOMAIN,
                &domain);
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = LsaInitializeLsaStringA(
                INBOUND_PASSWORD,
                &password);
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NTLMInitializeAuthUser(
                &user,
                &domain,
                &password,
                INBOUND_UID,
                &inboundUser);
    BAIL_ON_NTLM_ERROR(dwError);

error:

    LsaFreeLsaString(&user);
    LsaFreeLsaString(&domain);
    LsaFreeLsaString(&password);

    return dwError;

}

DWORD
inboundProviderGetNTOWF(
    PAUTH_USER user,
    NTLM_OWF *ntOWF
)
{
    memcpy(ntOWF, inboundUser.ntOWF, 16);

    return (LSA_ERROR_SUCCESS); 
}

DWORD
inboundProviderResponseMessageHandler(
    PAUTH_USER user,
    ULONG negFlags,
    PSEC_BUFFER_S pChallenge,
    PSEC_BUFFER pNTResponse,
    PSEC_BUFFER_S pLMResponse,
    PSEC_BUFFER_S pEncSessionKey,
    PSEC_BUFFER_S pBaseSessionKey
)
{
    return LSA_ERROR_NOT_IMPLEMENTED;
}
        

DWORD
inboundProviderInit(
    PAUTH_PROVIDER_INITDATA initData,
    PAUTH_PROVIDER thisProvider
)
{
    DWORD dwError = 0;

    dwError = LsaInitializeLsaStringA(
                INBOUND_PROVIDERNAME,
                &thisProvider->providerName);
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = LsaInitializeLsaStringA(
                INBOUND_DOMAIN,
                &thisProvider->realm);
    BAIL_ON_NTLM_ERROR(dwError);
    
    thisProvider->getUserInfo = inboundProviderGetUserInfo;
    thisProvider->freeUserInfo = inboundProviderFreeUserInfo;
    thisProvider->destroyAuthProvider = inboundProviderDestroy;
    thisProvider->userFromUid = inboundProviderUserFromUid;
    thisProvider->freeAuthUser = inboundProviderFreeAuthUser;
    thisProvider->getNTOwf = inboundProviderGetNTOWF;
    thisProvider->responseMessageHandler = inboundProviderResponseMessageHandler;

    dwError = inboundUserInit();
    BAIL_ON_NTLM_ERROR(dwError);

error:

    return dwError;
}

