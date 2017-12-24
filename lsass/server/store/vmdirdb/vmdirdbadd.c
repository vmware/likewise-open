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


/*
 * Map DIRECTORY_MOD attribute array to LDAPMod array
 */
#ifndef ATTR_OBJECT_CLASS
#define ATTR_OBJECT_CLASS                   "objectclass"
#endif
#ifndef ATTR_SAM_ACCOUNT_NAME
#define ATTR_SAM_ACCOUNT_NAME               "sAMAccountName"
#endif
#ifndef ATTR_CN
#define ATTR_CN                             "cn"
#endif


#if 0
DWORD
VmdirDbMapDirectoryModToLdapModArray(
    PSTR pszObjectDn,
    DIRECTORY_MOD *modifications,
    LDAPMod **ppLdapAttributes)
{
    NTSTATUS ntStatus = 0;
    DWORD dwError = 0;
    DWORD i = 0;
    DWORD j = 0;
    DWORD iAttr = 0;
    DWORD bFound = 0;
    DWORD dwModCount = 0;
    LDAPMod *pLdapAttributes = NULL;
    WCHAR wszObjectClass[] = VMDIR_DB_DIR_ATTR_OBJECT_CLASS;
    WCHAR wszSamAccountName[] = VMDIR_DB_DIR_ATTR_SAM_ACCOUNT_NAME;
    WCHAR wszCommonName[] = VMDIR_DB_DIR_ATTR_COMMON_NAME;
    WCHAR wszAttrNetbiosName[] = VMDIR_DB_DIR_ATTR_NETBIOS_NAME;
    WCHAR wszAttrAccountFlags[] = VMDIR_DB_DIR_ATTR_ACCOUNT_FLAGS;
    WCHAR wszLoginShell[] = VMDIR_DB_DIR_ATTR_SHELL;
    WCHAR wszHomeDir[] = VMDIR_DB_DIR_ATTR_HOME_DIR;
    PWSTR *pwszAttributes[] = 
        { wszObjectClass,
          wszSamAccountName,
          wszCommonName,
          wszAttrNetbiosName,
          wszAttrAccountFlags,
          wszLoginShell,
          wszHomeDir,
          NULL,
        };

    for (dwModCount=0; modifications[dwModCount]; dwModCount++)
        ;

    dwError = LwAllocateMemory((dwModCount+1) * sizeof(LDAPMod),
                               (VOID *) &pLdapAttributes);
    BAIL_ON_VMDIRDB_ERROR(dwError);

#if 0
typedef struct _DIRECTORY_MOD
{
    DIR_MOD_FLAGS    ulOperationFlags;
    PWSTR            pwszAttrName;
    ULONG            ulNumValues;
    PATTRIBUTE_VALUE pAttrValues;

} DIRECTORY_MOD, *PDIRECTORY_MOD;

/* for modifications */
typedef struct ldapmod {
        int             mod_op;

#define LDAP_MOD_OP                     (0x0007)
#define LDAP_MOD_ADD            (0x0000)
#define LDAP_MOD_DELETE         (0x0001)
#define LDAP_MOD_REPLACE        (0x0002)
#define LDAP_MOD_INCREMENT      (0x0003) /* OpenLDAP extension */
#define LDAP_MOD_BVALUES        (0x0080)
/* IMPORTANT: do not use code 0x1000 (or above),
 * it is used internally by the backends!
 * (see ldap/servers/slapd/slap.h)
 */

        char            *mod_type;
        union mod_vals_u {
                char            **modv_strvals;
                struct berval   **modv_bvals;
        } mod_vals;
#define mod_values      mod_vals.modv_strvals
#define mod_bvalues     mod_vals.modv_bvals
} LDAPMod;
#endif /* if 0 */
    for (i=0; i<dwModCount; i++)
    {
        for (j=0; pwszAttributes[j]; j++)
        {
            if (LwRtlWC16StringIsEqual(modifications[i].pwszAttrName,
                                       pwszAttributes[j],
                                       FALSE))
            {
                bFound = 1;
                break;
            }
        }

        if (bFound)
        {
            pLdapAttributes[iAttr].mod_op = LDAP_MOD_ADD;
        }
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

#endif

DWORD
VmdirDbAddObject(
    HANDLE        hBindHandle,
    PWSTR         pwszSqlDn,
    DIRECTORY_MOD modifications[]
    )
{
    DWORD dwError = 0;
    NTSTATUS ntStatus = 0;
    PVMDIR_AUTH_PROVIDER_CONTEXT pContext = NULL;
    LDAP *pLd = NULL;
    int ldap_err = 0;
    PSTR pszSqlDn = NULL;
    PSTR pszObjectDn = NULL;

    if (!hBindHandle || !pwszSqlDn)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_VMDIRDB_ERROR(dwError);
    }

    pContext = (PVMDIR_AUTH_PROVIDER_CONTEXT) hBindHandle;
    pLd = pContext->dirContext.pLd;

    ntStatus = LwRtlCStringAllocateFromWC16String(&pszSqlDn, pwszSqlDn);
    if (ntStatus)
    {
        dwError =  LwNtStatusToWin32Error(ntStatus);
        BAIL_ON_VMDIRDB_ERROR(dwError);
    }

    dwError = VmDirConstructMachineDN(
                  pszSqlDn,
                  &pszObjectDn);
    BAIL_ON_VMDIRDB_ERROR(dwError);

#if 0
    /*
     * Map DIRECTORY_MOD -> LDAPMod values
     */
    dwError = VmdirDbMapDirectoryModToLdapModArray(
                  pszObjectDn,
                  modifications,
                  pLdapAttributes);
#endif

{
    PSTR valsUser[] = {"user", NULL};
    PSTR valsObjectDn[] = {pszObjectDn, NULL};
    LDAPMod mod[7] = {
                         { LDAP_MOD_ADD, ATTR_OBJECT_CLASS, {valsUser} },
                         { LDAP_MOD_ADD, ATTR_SAM_ACCOUNT_NAME, {valsObjectDn} },
                         { LDAP_MOD_ADD, ATTR_CN, {valsObjectDn} },
#if 0
                         { LDAP_MOD_ADD, ATTR_NETBIOS_NAME, "users" },
                         { LDAP_MOD_ADD, ATTR_ACCT_FLAGS, "users" },
                         { LDAP_MOD_ADD, ATTR_HOME_DIR, "users" },
                         { LDAP_MOD_ADD, ATTR_LOGIN_SHELL, "users" },
#endif
                     };
    LDAPMod *ldapAttrs[] = { &mod[0], &mod[1],  &mod[2], NULL };

    ldap_err = ldap_add_ext_s(pLd,
                              pszObjectDn,
                              ldapAttrs,
                              NULL, 
                              NULL);
}
    if (ldap_err)
    {
        dwError = LwMapLdapErrorToLwError(ldap_err);
        BAIL_ON_VMDIRDB_ERROR(dwError);
    }

cleanup:
   LW_SAFE_FREE_STRING(pszSqlDn);
   LW_SAFE_FREE_STRING(pszObjectDn);

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
