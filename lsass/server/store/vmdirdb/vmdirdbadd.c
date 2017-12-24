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
 *        samdbadd.c
 *
 * Abstract:
 *
 *
 *      Likewise SAM Database Provider
 *
 *      SAM objects creation routines
 *
 * Authors: Krishna Ganugapati (krishnag@likewise.com)
 *          Sriram Nambakam (snambakam@likewise.com)
 *          Rafal Szczesniak (rafal@likewise.com)
 *          Adam Bernstein (abernstein@vmware.com)
 *
 */

#include "includes.h"


DWORD
VmdirDbAddObject(
    HANDLE        hBindHandle,
    PWSTR         pwszObjectDN,
    DIRECTORY_MOD modifications[]
    )
{
    DWORD dwError = 0;
    NTSTATUS ntStatus = 0;
    PVMDIR_AUTH_PROVIDER_CONTEXT pContext = NULL;
    LDAP *pLd = NULL;
    int ldap_err = 0;
    PSTR pszObjectDN = NULL;
    LDAPMod *ldapAttrs[5] = {0};


    if (!hBindHandle || !pwszObjectDN)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_VMDIRDB_ERROR(dwError);
    }

    pContext = (PVMDIR_AUTH_PROVIDER_CONTEXT) hBindHandle;
    pLd = pContext->dirContext.pLd;

    ntStatus = LwRtlCStringAllocateFromWC16String(&pszObjectDN, pwszObjectDN);
    if (ntStatus)
    {
        dwError =  LwNtStatusToWin32Error(ntStatus);
        BAIL_ON_VMDIRDB_ERROR(dwError);
    }

    /*
     * Map DIRECTORY_MOD -> LDAPMod values
     */

    ldap_err = ldap_add_ext_s(pLd,
                              pszObjectDN,
                              ldapAttrs,
                              NULL, 
                              NULL);
    if (ldap_err)
    {
        dwError = LwMapLdapErrorToLwError(ldap_err);
        BAIL_ON_VMDIRDB_ERROR(dwError);
    }

cleanup:
   LW_SAFE_FREE_STRING(pszObjectDN);
   return dwError;

error:
    goto cleanup;
}



/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
