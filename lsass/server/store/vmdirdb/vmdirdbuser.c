/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software
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
 *        vmdirdbuser.c
 *
 * Abstract:
 *
 *
 *      VMware vmdir LDAP Database Provider
 *
 *      VMDIR User Specific Management Methods
 *
 * Authors: Adam Bernstein (abernstein@vmware.com)
 *
 */

#include "includes.h"

DWORD
VmdirDbSetPassword(
    HANDLE hBindHandle,
    PWSTR  pwszSqlUserDN,
    PWSTR  pwszPassword
    )
{
    DWORD dwError = 0;
    NTSTATUS ntStatus = 0;
    int ldap_err = 0;
    PSTR pszSqlUserDN = NULL;
    PSTR pszUserDN = NULL;
    PSTR pszPassword = NULL;
    PVMDIR_AUTH_PROVIDER_CONTEXT pContext = NULL;
    LDAP *pLd = NULL;

    if (!hBindHandle || !pwszSqlUserDN || !pwszPassword)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_VMDIRDB_ERROR(dwError);
    }

    pContext = (PVMDIR_AUTH_PROVIDER_CONTEXT) hBindHandle;
    pLd = pContext->dirContext.pLd;

    ntStatus = LwRtlCStringAllocateFromWC16String(&pszSqlUserDN, pwszSqlUserDN);
    BAIL_ON_VMDIRDB_ERROR(LwNtStatusToWin32Error(ntStatus));

    ntStatus = LwRtlCStringAllocateFromWC16String(&pszPassword, pwszPassword);
    BAIL_ON_VMDIRDB_ERROR(LwNtStatusToWin32Error(ntStatus));

    dwError = VmDirConstructMachineDN(
                  pszSqlUserDN,
                  &pszUserDN);
    BAIL_ON_VMDIRDB_ERROR(dwError);

    {
        struct berval bvalUserPassword = {(ber_len_t) strlen(pszPassword), pszPassword};

        struct berval *bvalsUserPassword[] =  {
            &bvalUserPassword, 0,
        };
        LDAPMod mod_del[] = {
            { LDAP_MOD_REPLACE | LDAP_MOD_BVALUES, ATTR_USER_PASSWORD, {.modv_bvals = bvalsUserPassword}},
        };
        LDAPMod *ldapAttrs_del[] = { &mod_del[0], NULL };
    
        ldap_err = ldap_modify_ext_s(pLd,
                                  pszUserDN,
                                  ldapAttrs_del,
                                  NULL,
                                  NULL);
        if (ldap_err)
        {
            dwError = LwMapLdapErrorToLwError(ldap_err);
            BAIL_ON_VMDIRDB_ERROR(dwError);
        }
    }

cleanup:
    LW_SAFE_FREE_STRING(pszSqlUserDN);
    LW_SAFE_FREE_STRING(pszPassword);
    LW_SAFE_FREE_STRING(pszUserDN);

    return dwError;

error:
    goto cleanup;

}


DWORD
VmdirDbChangePassword(
    HANDLE hBindHandle,
    PWSTR  pwszUserDN,
    PWSTR  pwszOldPassword,
    PWSTR  pwszNewPassword
    )
{
    DWORD dwError = 0;

    BAIL_ON_VMDIRDB_ERROR(dwError);

cleanup:


    return dwError;

error:

    goto cleanup;
}

DWORD
VmdirDbVerifyPassword(
    HANDLE hBindHandle,
    PWSTR  pwszUserDN,
    PWSTR  pwszPassword
    )
{
    DWORD dwError = 0;

    return dwError;
}


DWORD
VmdirDbGetUserCount(
    HANDLE hBindHandle,
    PDWORD pdwNumUsers
    )
{
    return 0;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
