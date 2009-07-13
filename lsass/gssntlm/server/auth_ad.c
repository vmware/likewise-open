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
 *        auth_ad.c
 *
 * Abstract:
 *
 *       active directory auth provider
 *
 * Author: Todd Stecher (2007)
 *
 */
#include "server.h"

#define AD_PROVIDERNAME "active directory provider"

 DWORD
adProviderGetUserInfo(
    PAUTH_USER user,
    PAUTH_INFO *userInfo
    )
{
    DWORD dwError = LW_ERROR_NOT_IMPLEMENTED;
    BAIL_ON_NTLM_ERROR(dwError);

error:

    return dwError;
}


static void
adProviderFreeUserInfo(
    PAUTH_INFO userInfo
)
{
    if (!userInfo)
        return;

    /* LsaFreeLsaString(userInfo->authUser.userName); */
    /* LsaFreeLsaString(userInfo->authUser.domain); */
    /* todo - clear out OWFs - fill w/ 0s */
}


static void
adProviderDestroy( void )
{
    return;
}


DWORD
adProviderInit(
    PAUTH_PROVIDER_INITDATA initData,
    PAUTH_PROVIDER thisProvider
)
{
    DWORD dwError;

    dwError = LsaInitializeLsaStringA(
                AD_PROVIDERNAME,
                &thisProvider->providerName
                );

    BAIL_ON_NTLM_ERROR(dwError);

    thisProvider->getUserInfo = adProviderGetUserInfo;
    thisProvider->freeUserInfo = adProviderFreeUserInfo;
    thisProvider->destroyAuthProvider = adProviderDestroy;

error:

    return dwError;
}

