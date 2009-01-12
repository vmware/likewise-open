/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
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

#include "includes.h"

DWORD
SMBCreatePlainAccessTokenA(
    PCSTR pszUsername,
    PCSTR pszPassword,
    PHANDLE phAccessToken
    )
{
    DWORD dwError = 0;
    PWSTR pwszUsername = NULL;
    PWSTR pwszPassword = NULL;
    
    dwError = SMBMbsToWc16s(pszUsername, &pwszUsername);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBMbsToWc16s(pszPassword, &pwszPassword);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBCreatePlainAccessTokenW(pwszUsername, pwszPassword, phAccessToken);
    BAIL_ON_SMB_ERROR(dwError);

error:
    
    SMB_SAFE_FREE_MEMORY(pwszUsername);
    SMB_SAFE_FREE_MEMORY(pwszPassword);

    return dwError;
}

DWORD
SMBCreatePlainAccessTokenW(
    PCWSTR pwszUsername,
    PCWSTR pwszPassword,
    PHANDLE phAccessToken
    )
{
    DWORD dwError = 0;
    PSMB_API_HANDLE pAPIHandle = NULL;
    PSMB_SECURITY_TOKEN_REP pSecurityToken = NULL;

    dwError = SMBAllocateMemory(sizeof(*pAPIHandle), (void**) (void*) &pAPIHandle);
    BAIL_ON_SMB_ERROR(dwError);

    pAPIHandle->type = SMB_API_HANDLE_ACCESS;

    pSecurityToken = &pAPIHandle->variant.securityToken;
    
    pSecurityToken->type = SMB_SECURITY_TOKEN_TYPE_PLAIN;
    
    dwError = SMBWc16sDup(pwszUsername, &pSecurityToken->payload.plain.pwszUsername);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBWc16sDup(pwszPassword, &pSecurityToken->payload.plain.pwszPassword);
    BAIL_ON_SMB_ERROR(dwError);

    *phAccessToken = (HANDLE) pAPIHandle;
    
cleanup:

    return dwError;

error:

    if (pAPIHandle)
    {
        SMBCloseHandle(NULL, pAPIHandle);
    }

    goto cleanup;
}

DWORD
SMBCreateKrb5AccessTokenA(
    PCSTR pszPrincipal,
    PCSTR pszCachePath,
    PHANDLE phAccessToken
    )
{
    DWORD dwError = 0;
    PWSTR pwszPrincipal = NULL;
    PWSTR pwszCachePath = NULL;
    
    dwError = SMBMbsToWc16s(pszPrincipal, &pwszPrincipal);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBMbsToWc16s(pszCachePath, &pwszCachePath);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBCreateKrb5AccessTokenW(pwszPrincipal, pwszCachePath, phAccessToken);
    BAIL_ON_SMB_ERROR(dwError);

error:
    
    SMB_SAFE_FREE_MEMORY(pwszPrincipal);
    SMB_SAFE_FREE_MEMORY(pwszCachePath);

    return dwError;
}

DWORD
SMBCreateKrb5AccessTokenW(
    PCWSTR pwszPrincipal,
    PCWSTR pwszCachePath,
    PHANDLE phAccessToken
    )
{
    DWORD dwError = 0;
    PSMB_API_HANDLE pAPIHandle = NULL;
    PSMB_SECURITY_TOKEN_REP pSecurityToken = NULL;

    dwError = SMBAllocateMemory(sizeof(*pAPIHandle), (void**) (void*) &pAPIHandle);
    BAIL_ON_SMB_ERROR(dwError);

    pAPIHandle->type = SMB_API_HANDLE_ACCESS;

    pSecurityToken = &pAPIHandle->variant.securityToken;
    
    pSecurityToken->type = SMB_SECURITY_TOKEN_TYPE_KRB5;
    
    dwError = SMBWc16sDup(pwszPrincipal, &pSecurityToken->payload.krb5.pwszPrincipal);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBWc16sDup(pwszCachePath, &pSecurityToken->payload.krb5.pwszCachePath);
    BAIL_ON_SMB_ERROR(dwError);

    *phAccessToken = (HANDLE) pAPIHandle;
    
cleanup:

    return dwError;

error:

    if (pAPIHandle)
    {
        SMBCloseHandle(NULL, pAPIHandle);
    }

    goto cleanup;
}
