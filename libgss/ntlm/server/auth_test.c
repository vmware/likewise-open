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
#define TEST_DOMAIN "xyz"
#define TEST_UID 1000
#define TEST_HOST "ubuntu1"
#define TEST_PASSWORD "password123"
#define TEST_PROVIDERNAME "testprovider"


AUTH_USER testUser;
NTLM_OWF owf;

DWORD
testProviderUserFromUid(
    uid_t uid,
    PAUTH_USER authUser
    )
{
    memcpy(authUser, &testUser, sizeof(AUTH_USER));
    return LSA_ERROR_SUCCESS;
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
    DWORD dwError;
    PAUTH_INFO localUserInfo = NULL;

    localUserInfo = (PAUTH_INFO) NTLMAllocateMemory(sizeof(AUTH_INFO));
    if (!localUserInfo) 
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_OUT_OF_MEMORY);

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
testProviderDestroy( void )
{
    return;
}

DWORD
testUserInit()
{
    DWORD dwError;

    LSA_STRING user;
    LSA_STRING domain;
    LSA_STRING password;

    dwError = LsaInitializeLsaStringA(
                TEST_USER,
                &user
                );

    BAIL_ON_NTLM_ERROR(dwError);


    dwError = LsaInitializeLsaStringA(
                TEST_HOST,
                &domain
                );

    BAIL_ON_NTLM_ERROR(dwError);

    dwError = LsaInitializeLsaStringA(
                TEST_PASSWORD,
                &password
                );

    BAIL_ON_NTLM_ERROR(dwError);


    if (!NTLMInitializeAuthUser(
            &user,
            &domain,
            &password,
            TEST_UID,
            &testUser))

        BAIL_WITH_NTLM_ERROR(LSA_ERROR_OUT_OF_MEMORY);


error:


    return dwError;

}

DWORD
testProviderGetNTOWF(
    PAUTH_USER user,
    NTLM_OWF *ntOWF
)
{
    memcpy(ntOWF, testUser.ntOWF, 16);
    return (LSA_ERROR_SUCCESS); 
}

DWORD
testProviderResponseMessageHandler(
    PNTLM_CONTEXT pCtxt,
    PSEC_BUFFER pNTResponse,
    PSEC_BUFFER pLMResponse,
    PSEC_BUFFER pSessionKey,
    PAUTH_USER user
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    SEC_BUFFER expected;
    NTLM_OWF ntOWF;
    SEC_BUFFER_S clientChallenge;
    SEC_BUFFER_S sessionKey;
    ULONG negFlags = NTLMContextGetNegotiateFlags(pCtxt);

    ZERO_STRUCT(expected);
    INIT_SEC_BUFFER_S(&clientChallenge, 24);
    INIT_SEC_BUFFER_S(&sessionKey, 16);

    if (pNTResponse->length > NTLM_V1_RESPONSE_LENGTH)
    {
        /* @todo  ntlm v2 support */
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_NOT_IMPLEMENTED);
    }
    else
    {

        /* compute, and validate NTLMv1 challenge */
        dwError = NTLMComputeNTLMv1Response(
                        pCtxt,
                        pCtxt->challenge,
                        &expected
                        );

        BAIL_ON_NTLM_ERROR(dwError);

        if (memcmp(
                expected.buffer, 
                pNTResponse->buffer,
                expected.length
                )) {

            /* @todo debug */
            BAIL_WITH_NTLM_ERROR(LSA_ERROR_LOGON_FAILURE);
        }
    }

    /* success!! Now get the session key */
    dwError = testProviderGetNTOWF(user, &ntOWF);
    BAIL_ON_NTLM_ERROR(dwError);

    if (negFlags & NEGOTIATE_KEY_EXCH)
        memcpy(sessionKey.buffer, pSessionKey->buffer, 16);

    if (negFlags & NEGOTIATE_NTLM2)
        memcpy(clientChallenge.buffer, pLMResponse->buffer, 8);

    /* encrypted session key is decrypted in place in context */
    dwError = NTLMComputeSessionKey(
                pCtxt,
                pCtxt->challenge,
                &clientChallenge,
                &sessionKey,
                false
                );

    BAIL_ON_NTLM_ERROR(dwError);

error:

    NTLM_SAFE_FREE(expected.buffer);

    return dwError;

}
        

DWORD
testProviderInit(
    PAUTH_PROVIDER_INITDATA initData,
    PAUTH_PROVIDER thisProvider
)
{
    DWORD dwError;

    dwError = LsaInitializeLsaStringA(
                TEST_PROVIDERNAME,
                &thisProvider->providerName
                );

    BAIL_ON_NTLM_ERROR(dwError);

    thisProvider->getUserInfo = testProviderGetUserInfo;
    thisProvider->freeUserInfo = testProviderFreeUserInfo;
    thisProvider->destroyAuthProvider = testProviderDestroy;
    thisProvider->userFromUid = testProviderUserFromUid;
    thisProvider->freeAuthUser = testProviderFreeAuthUser;
    thisProvider->getNTOwf = testProviderGetNTOWF;
    thisProvider->responseMessageHandler = testProviderResponseMessageHandler;

    dwError = testUserInit();

    BAIL_ON_NTLM_ERROR(dwError);

error:

    return dwError;
}

