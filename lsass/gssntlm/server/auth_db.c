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
 *        auth_db.c
 *
 * Abstract:
 *
 *       Likewise DB local user auth provider 
 *
 * Author: Todd Stecher (2007)
 *
 */
#include "server.h"

#define DB_PROVIDERNAME "dbProvider"

PAUTH_PROVIDER provider;

DWORD
dbProviderUserFromUid(
    uid_t uid,
    PAUTH_USER authUser
    )
{
    DWORD dwError;
    LSA_STRING user = {0};
    LSA_STRING domain = {0};
    HANDLE hLsaConnection = (HANDLE)NULL;
    PLSA_USER_INFO_0 ui = NULL;
    
    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaFindUserById(
                    hLsaConnection,
                    uid,
                    0,
                    (PVOID*) &ui
                    );

    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaInitializeLsaStringA(
                    ui->pszName,
                    &user
                    );
                    
    BAIL_ON_LSA_ERROR(dwError);

    dwError = NTLMGetWorkstationName(&domain);
                    
    BAIL_ON_LSA_ERROR(dwError);

    dwError = NTLMInitializeAuthUser(
                    &user,
                    &domain,
                    NULL,
                    uid,
                    authUser
                    );

    BAIL_ON_LSA_ERROR(dwError);

    authUser->provider = provider;

error:

    NTLM_SAFE_FREE(domain.buffer);
    NTLM_SAFE_FREE(user.buffer);

    if (ui)
       LsaFreeUserInfo(0, ui);

    if (hLsaConnection != (HANDLE)NULL) 
        LsaCloseServer(hLsaConnection);

    return dwError;
}

void
dbProviderFreeAuthUser(
    PAUTH_USER authUser
    )
{
    NTLM_SAFE_FREE(authUser->user.buffer);
    NTLM_SAFE_FREE(authUser->domain.buffer);
}

DWORD
dbProviderGetUserInfo(
    PAUTH_USER user,
    PAUTH_INFO *userInfo
    )
{
    DWORD dwError = 0;
    PSTR ansiUser = NULL;
    PAUTH_INFO localUserInfo = NULL;
    PLSA_USER_INFO_1  ui1 = NULL;
    HANDLE hLsaConnection = (HANDLE)NULL;

    dwError = LsaStringToAnsi(
                    &user->user,
                    &ansiUser
                    );

    BAIL_ON_LSA_ERROR(dwError);
                
    localUserInfo = (PAUTH_INFO) NTLMAllocateMemory(sizeof(AUTH_INFO));
    if (!localUserInfo) 
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_OUT_OF_MEMORY);

    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaFindUserByName(
                    hLsaConnection,
                    ansiUser,
                    1,
                    (PVOID*) &ui1
                    );

    BAIL_ON_NTLM_ERROR(dwError);

    if (ui1->pNTHash)
        memcpy(localUserInfo->ntOWF, ui1->pNTHash, 16);
    else
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_INVALID_PASSWORD);

    if (ui1->pLMHash)
        memcpy(localUserInfo->lmOWF, ui1->pLMHash, 16);
    else
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_INVALID_PASSWORD);

    localUserInfo->authUser = user;

    (*userInfo) = localUserInfo;
    localUserInfo = NULL;

error:

    NTLM_SAFE_FREE(ansiUser);
    NTLM_SAFE_FREE(localUserInfo);

    if (ui1)
       LsaFreeUserInfo(1, ui1);

    if (hLsaConnection != (HANDLE)NULL) 
        LsaCloseServer(hLsaConnection);

    NTLMFreeMemory(localUserInfo);
    return dwError;
}


static void
dbProviderFreeUserInfo(
    PAUTH_INFO userInfo
)
{
    if (!userInfo)
        return;

    NTLM_SAFE_FREE(userInfo);
}


void
dbProviderDestroy( void )
{ 
    /* nothing */
    return;
}

DWORD
dbProviderGetNTOWF(
    PAUTH_USER user,
    NTLM_OWF *ntOWF
)
{
    DWORD dwError;
    PAUTH_INFO localUserInfo = NULL;

    if (user->dwFlags & AUTH_USER_PASSWORD_SUPPLIED)
    {
        memcpy(ntOWF, user->ntOWF, 16);
        return LSA_ERROR_SUCCESS;
    }

    dwError = dbProviderGetUserInfo(
                    user,
                    &localUserInfo
                    );

    BAIL_ON_NTLM_ERROR(dwError);
    
    memcpy(ntOWF, localUserInfo->ntOWF, 16);

error:

    NTLM_SAFE_FREE(localUserInfo);

    return dwError;
}

        

DWORD
dbProviderInit(
    PAUTH_PROVIDER_INITDATA initData,
    PAUTH_PROVIDER thisProvider
)
{
    DWORD dwError = 0;

    dwError = LsaInitializeLsaStringA(
                DB_PROVIDERNAME,
                &thisProvider->providerName
                );

    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NTLMGetWorkstationName(&thisProvider->realm);

    BAIL_ON_NTLM_ERROR(dwError);

    thisProvider->getUserInfo = dbProviderGetUserInfo;
    thisProvider->freeUserInfo = dbProviderFreeUserInfo;
    thisProvider->destroyAuthProvider = dbProviderDestroy;
    thisProvider->userFromUid = dbProviderUserFromUid;
    thisProvider->freeAuthUser = dbProviderFreeAuthUser;
    thisProvider->getNTOwf = dbProviderGetNTOWF;
    thisProvider->responseMessageHandler = NTLMLocalResponseMessageHandler;

    provider = thisProvider;

error:

    return dwError;
}

