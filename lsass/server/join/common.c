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
 *        common.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Join to Active Directory
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 */

#include "includes.h"

DWORD
LsaSetSMBAccessTokenWithFlags(
    IN PCSTR pszDomain,
    IN PCSTR pszUsername,
    IN PCSTR pszPassword,
    IN DWORD dwFlags,
    OUT PLSA_ACCESS_TOKEN_FREE_INFO* ppFreeInfo
    )
{
    DWORD dwError = 0;
    PLSA_ACCESS_TOKEN_FREE_INFO pFreeInfo = NULL;

    BAIL_ON_INVALID_POINTER(ppFreeInfo);
    BAIL_ON_INVALID_STRING(pszDomain);
    BAIL_ON_INVALID_STRING(pszUsername);

    if ( !(dwFlags & LSA_NET_JOIN_DOMAIN_NOTIMESYNC) && geteuid() == 0)
    {
        dwError = LsaSyncTimeToDC(pszDomain);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaSetSMBAccessToken(
                    pszDomain,
                    pszUsername,
                    pszPassword,
                    TRUE,
                    &pFreeInfo);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    *ppFreeInfo = pFreeInfo;

    return dwError;

error:
    LsaFreeSMBAccessToken(&pFreeInfo);
    goto cleanup;
}

