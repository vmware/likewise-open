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
 *        vmdirdbadd.c
 *
 * Abstract:
 *
 *
 *      VMware vmdir LDAP Database Provider
 *
 *      vmdir objects creation routines
 *
 * Authors: Adam Bernstein (abernstein@vmware.com)
 */

#include "includes.h"


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
VmdirDbFindModValue(
    PDIRECTORY_MOD modifications,
    PWSTR pwszAttrName,
    PATTRIBUTE_VALUE *ppAttr)
{
    DWORD dwError = ERROR_NOT_FOUND;
    DWORD i = 0;
    DWORD bFound = 0;

    if (!modifications || !pwszAttrName || !ppAttr)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_VMDIRDB_ERROR(dwError);
    }

    for (i=0; modifications[i].pwszAttrName; i++)
    {
        if (LwRtlWC16StringIsEqual(modifications[i].pwszAttrName,
                                   pwszAttrName,
                                   FALSE))
        {
            bFound = 1;
            dwError = 0;
            break;
        }
    }
    if (bFound)
    {
        *ppAttr = modifications[i].pAttrValues;
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

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
    PSTR pszPtr = NULL;
    PSTR pszSqlDn = NULL;
    PSTR pszObjectDn = NULL;
    PSTR pszSamAccountName = NULL;
    PSTR pszAcctUpn = NULL;
    CHAR szUserAccountControl[13] = {0};
    ULONG userAccountControl = 0;
    WCHAR wszAttrAccountFlags[] = VMDIR_DB_DIR_ATTR_ACCOUNT_FLAGS;
    PATTRIBUTE_VALUE pAttr = NULL;
    PSTR pszDcClientsGroupDn = NULL;
    PSTR pszHostSpn = NULL;
    PSTR pszLdapSpn = NULL;
    PSTR pszCifsSpn = NULL;
    PSTR pszRpcSpnUc = NULL;
    PSTR pszDnsSpnUc = NULL;
    PSTR pszLdapSpnUcShort = NULL;

    if (!hBindHandle || !pwszSqlDn)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_VMDIRDB_ERROR(dwError);
    }

    pContext = (PVMDIR_AUTH_PROVIDER_CONTEXT) hBindHandle;
    pLd = pContext->dirContext.pLd;

    ntStatus = LwRtlCStringAllocateFromWC16String(&pszSqlDn, pwszSqlDn);
    BAIL_ON_VMDIRDB_ERROR(LwNtStatusToWin32Error(ntStatus));

    dwError = VmDirConstructMachineDN(
                  pszSqlDn,
                  &pszObjectDn);
    BAIL_ON_VMDIRDB_ERROR(dwError);


    /* Find "Account Flags" value */
    dwError = VmdirDbFindModValue(
                  modifications,
                  wszAttrAccountFlags,
                  &pAttr);
    if (dwError == 0)
    {
        userAccountControl = pAttr->data.ulValue;
    }
    snprintf(szUserAccountControl, sizeof(szUserAccountControl), "%u", userAccountControl);

    /* Construct the sAMAccountName */
    ntStatus = LwRtlCStringDuplicate(&pszSamAccountName,
                                     pszObjectDn);
    BAIL_ON_VMDIRDB_ERROR(LwNtStatusToWin32Error(ntStatus));

    pszPtr = strstr(pszSamAccountName+3, ",dc");
    if (pszPtr)
    {
        *pszPtr = '\0';
    }

    /* Construct the Kerberos UPN */
    dwError = VmDirConstructMachineUPN(
                  pszSqlDn,
                  &pszAcctUpn);
    BAIL_ON_VMDIRDB_ERROR(dwError);

    dwError = VmDirConstructServicePrincipalName(
                  pszSqlDn,
                  "host",
                  &pszHostSpn);
    BAIL_ON_VMDIRDB_ERROR(dwError);

    dwError = VmDirConstructServicePrincipalName(
                  pszSqlDn,
                  "ldap",
                  &pszLdapSpn);
    BAIL_ON_VMDIRDB_ERROR(dwError);

    dwError = VmDirConstructServicePrincipalName(
                  pszSqlDn,
                  "cifs",
                  &pszCifsSpn);
    BAIL_ON_VMDIRDB_ERROR(dwError);

    dwError = VmDirConstructServicePrincipalName(
                  pszSqlDn,
                  "RPC",
                  &pszRpcSpnUc);
    BAIL_ON_VMDIRDB_ERROR(dwError);

    dwError = VmDirConstructServicePrincipalName(
                  pszSqlDn,
                  "DNS",
                  &pszDnsSpnUc);
    BAIL_ON_VMDIRDB_ERROR(dwError);

    dwError = VmDirConstructServicePrincipalNameEx(
                  pszSqlDn,
                  "LDAP",
                  TRUE,  /* Uc host */
                  TRUE,  /* not FQDN */
                  &pszLdapSpnUcShort);
    BAIL_ON_VMDIRDB_ERROR(dwError);


#if 0
    /*
     * Map DIRECTORY_MOD -> LDAPMod values
     * TBD:Adam-Map "modifications" array to LDAP attributes as below */
     */
    dwError = VmdirDbMapDirectoryModToLdapModArray(
                  pszObjectDn,
                  modifications,
                  pLdapAttributes);
    BAIL_ON_VMDIRDB_ERROR(dwError);
#endif

{
    PSTR valsComputer[] = {"computer", "samDomain", NULL};
    PSTR valsSamAccountName[] = {pszSamAccountName+3, NULL};
    PSTR valsCn[] = {pszObjectDn, NULL};
    PSTR valsDistinguishedName[] = {pszObjectDn, NULL};
    PSTR valsUserAccountControl[] = {szUserAccountControl, NULL};
    PSTR valsUserPassword[] = {"VMware123@", NULL}; /* TBD: Make random dummy password, will be changed later */
    PSTR valsdNSHostName[] = {" ", NULL};
    PSTR valsservicePrincipalName[] = { pszHostSpn, pszLdapSpn, pszCifsSpn,
                                        pszRpcSpnUc, pszDnsSpnUc,
                                        pszLdapSpnUcShort, NULL };
    PSTR valsDescription[] = {" ", NULL};
    PSTR valsOsName[] = {" ", NULL};
    PSTR valsOsVersion[] = {" ", NULL};
    PSTR valsOsServicePack[] = {" ", NULL};
    PSTR valsAcctUpn[] = {pszAcctUpn, NULL};
    PSTR valsMaxPwdAge[] = {"-36288000000000", NULL};

    /* Used to add Machine Account to the "DCUsers" group */
    LDAPMod modgrp[] = {
                         {LDAP_MOD_ADD, ATTR_NAME_MEMBER, {valsDistinguishedName} },
                         { 0, 0, {0} },
                       };

    LDAPMod *ldapMods[] = { &modgrp[0],
                            NULL,
                          };


    LDAPMod mod[] = {
              /* 0  */   { LDAP_MOD_ADD, ATTR_OBJECT_CLASS, {valsComputer} },
                         { LDAP_MOD_ADD, ATTR_SAM_ACCOUNT_NAME, {valsSamAccountName} },
                         { LDAP_MOD_ADD, ATTR_CN, {valsCn} },
                         { LDAP_MOD_ADD, ATTR_DISTINGUISHED_NAME, {valsDistinguishedName} },
                         { LDAP_MOD_ADD, ATTR_ACCT_FLAGS, {valsUserAccountControl} },
              /* 5  */   { LDAP_MOD_ADD, ATTR_USER_PASSWORD, {valsUserPassword} },
                         { LDAP_MOD_ADD, ATTR_DNS_HOSTNAME, {valsdNSHostName} },
                         { LDAP_MOD_ADD, ATTR_SERVICE_PRINCIPAL_NAME, {valsservicePrincipalName} },
                         { LDAP_MOD_ADD, ATTR_DESCRIPTION, {valsDescription} },
                         { LDAP_MOD_ADD, ATTR_OS_NAME, {valsOsName} },
              /* 10 */   { LDAP_MOD_ADD, ATTR_OS_VERSION, {valsOsVersion} },
                         { LDAP_MOD_ADD, ATTR_OS_SERVICE_PACK, {valsOsServicePack} },
                         { LDAP_MOD_ADD, ATTR_KRB_UPN, {valsAcctUpn} },
                         { LDAP_MOD_ADD, ATTR_NAME_MAX_PASSWORD_AGE, {valsMaxPwdAge} },

#if 0

                         { LDAP_MOD_ADD, ATTR_NETBIOS_NAME, "users" },
                         { LDAP_MOD_ADD, ATTR_HOME_DIR, "users" },
                         { LDAP_MOD_ADD, ATTR_LOGIN_SHELL, "users" },
#endif
                         { 0, 0, {0} },
                     };
    LDAPMod *ldapAttrs[] = { &mod[0],  &mod[1],  &mod[2],  &mod[3],  &mod[4],  &mod[5], 
                             &mod[6],  &mod[7],  &mod[8],  &mod[9],  &mod[10], &mod[11], 
                             &mod[12], &mod[13],
                             NULL
                           };

    /* Create the Machine Account for the joining Domain */
    ldap_err = ldap_add_ext_s(pLd,
                              pszObjectDn,
                              ldapAttrs,
                              NULL, 
                              NULL);
    if (ldap_err)
    {
        dwError = LwMapLdapErrorToLwError(ldap_err);
        BAIL_ON_VMDIRDB_ERROR(dwError);
    }

    /* Add the Machine Account to the DCClients group */
    dwError = LwAllocateStringPrintf(
                    &pszDcClientsGroupDn,
                    "%s,%s",
                    "cn=DCClients,cn=Builtin",
                    gVmdirGlobals.pszDomainDn);
    BAIL_ON_VMDIRDB_ERROR(dwError);

    ldap_err = ldap_modify_ext_s(pLd,
                                 pszDcClientsGroupDn,
                                 ldapMods,
                                 NULL,
                                 NULL);
    if (ldap_err)
    {
        dwError = LwMapLdapErrorToLwError(ldap_err);
        BAIL_ON_VMDIRDB_ERROR(dwError);
    }
}

cleanup:
   LW_SAFE_FREE_STRING(pszSqlDn);
   LW_SAFE_FREE_STRING(pszObjectDn);
   LW_SAFE_FREE_STRING(pszAcctUpn);
   LW_SAFE_FREE_STRING(pszHostSpn);
   LW_SAFE_FREE_STRING(pszLdapSpn);
   LW_SAFE_FREE_STRING(pszCifsSpn);
   LW_SAFE_FREE_STRING(pszRpcSpnUc);
   LW_SAFE_FREE_STRING(pszDnsSpnUc);
   LW_SAFE_FREE_STRING(pszLdapSpnUcShort);

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
