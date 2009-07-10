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

#include "client.h"

DWORD
NtlmClientAcquireCredentialsHandle(
    IN SEC_CHAR *pszPrincipal,
    IN SEC_CHAR *pszPackage,
    IN DWORD fCredentialUse,
    IN PLUID pvLogonID,
    IN PVOID pAuthData,
    // NOT USED BY NTLM - IN SEC_GET_KEY_FN pGetKeyFn,
    // NOT USED BY NTLM - IN PVOID pvGetKeyArgument,
    OUT PCredHandle phCredential,
    OUT PTimeStamp ptsExpiry
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    HANDLE hServer = INVALID_HANDLE;

    memset(phCredential, 0, sizeof(CredHandle));
    memset(ptsExpiry, 0, sizeof(TimeStamp));

    dwError = NtlmOpenServer(&hServer);
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NtlmTransactAcquireCredentialsHandle(
        hServer,
        pszPrincipal,
        pszPackage,
        fCredentialUse,
        pvLogonID,
        pAuthData,
        //pGetKeyFn,
        //pvGetKeyArgument,
        phCredential,
        ptsExpiry
        );

cleanup:
    if(INVALID_HANDLE != hServer)
    {
        NtlmCloseServer(hServer);
    }
    return(dwError);
error:
    memset(phCredential, 0, sizeof(CredHandle));
    memset(ptsExpiry, 0, sizeof(TimeStamp));
    goto cleanup;
}
