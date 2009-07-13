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
 *        auth_test.c
 *
 * Abstract:
 *
 *       test auth provider - same user & key
 *
 * Author: Todd Stecher (2007)
 *
 */
#include "server.h"

#define TEST_USER "Freddy"
#define TEST_DOMAIN "test9xyz9test"
#define TEST_UID 1000
#define TEST_HOST "ubuntu1"
#define TEST_PASSWORD "password123"
#define TEST_PROVIDERNAME "testprovider"


AUTH_USER testUser;
//NTLM_OWF owf;

DWORD
testProviderUserFromUid(
    uid_t uid,
    PAUTH_USER authUser
    )
{
    memcpy(authUser, &testUser, sizeof(AUTH_USER));
    return LW_ERROR_SUCCESS;
}

void
testProviderFreeAuthUser(
    PAUTH_USER authUser
    )
{
    /* do nothing */
    return;
}

DWORD
testProviderGetUserInfo(
    PAUTH_USER user,
    PAUTH_INFO *userInfo
    )
{
    DWORD dwError = 0;
    PAUTH_INFO localUserInfo = NULL;

    localUserInfo = (PAUTH_INFO) NTLMAllocateMemory(sizeof(AUTH_INFO));
    if (!localUserInfo) 
        BAIL_WITH_NTLM_ERROR(LW_ERROR_OUT_OF_MEMORY);

    memcpy(&localUserInfo->authUser, &testUser, sizeof(AUTH_USER));

    *userInfo = localUserInfo;
    localUserInfo = NULL;

error:

    NTLMFreeMemory(localUserInfo);
    return dwError;
}


static void
testProviderFreeUserInfo(
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
testProviderDestroy(
    void
    )
{
    return;
}

DWORD
testUserInit(
    void
    )
{
    DWORD dwError = 0;

    LSA_STRING user = {0};
    LSA_STRING domain = {0};
    LSA_STRING password = {0};

    dwError = LsaInitializeLsaStringA(
                TEST_USER,
                &user
                );

    BAIL_ON_NTLM_ERROR(dwError);


    dwError = LsaInitializeLsaStringA(
                TEST_DOMAIN,
                &domain
                );

    BAIL_ON_NTLM_ERROR(dwError);

    dwError = LsaInitializeLsaStringA(
                TEST_PASSWORD,
                &password
                );

    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NTLMInitializeAuthUser(
                &user,
                &domain,
                &password,
                TEST_UID,
                &testUser);


    BAIL_ON_NTLM_ERROR(dwError);

error:

    LsaFreeLsaString(&user);
    LsaFreeLsaString(&domain);
    LsaFreeLsaString(&password);

    return dwError;

}

DWORD
testProviderGetNTOWF(
    PAUTH_USER user,
    NTLM_OWF *ntOWF
)
{
    if (user->dwFlags & AUTH_USER_PASSWORD_SUPPLIED)
        memcpy(ntOWF, user->ntOWF, 16);
    else
        memcpy(ntOWF, testUser.ntOWF, 16);

    return (LW_ERROR_SUCCESS);
}

        

DWORD
testProviderInit(
    PAUTH_PROVIDER_INITDATA initData,
    PAUTH_PROVIDER thisProvider
)
{
    DWORD dwError = 0;

    dwError = LsaInitializeLsaStringA(
                TEST_PROVIDERNAME,
                &thisProvider->providerName
                );

    BAIL_ON_NTLM_ERROR(dwError);

    dwError = LsaInitializeLsaStringA(
                TEST_DOMAIN,
                &thisProvider->realm
                );

    BAIL_ON_NTLM_ERROR(dwError);
    thisProvider->getUserInfo = testProviderGetUserInfo;
    thisProvider->freeUserInfo = testProviderFreeUserInfo;
    thisProvider->destroyAuthProvider = testProviderDestroy;
    thisProvider->userFromUid = testProviderUserFromUid;
    thisProvider->freeAuthUser = testProviderFreeAuthUser;
    thisProvider->getNTOwf = testProviderGetNTOWF;
    thisProvider->responseMessageHandler = NTLMLocalResponseMessageHandler;

    dwError = testUserInit();

    BAIL_ON_NTLM_ERROR(dwError);

error:

    return dwError;
}

