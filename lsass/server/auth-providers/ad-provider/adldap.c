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
 *        provider-main.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Local Authentication Provider
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Wei Fu (wfu@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 *
 */

#include "adprovider.h"
#include "adnetapi.h"

DWORD
ADGetDomainQualifiedString(
    PCSTR pszNetBIOSDomainName,
    PCSTR pszName,
    PSTR* ppszQualifiedName
    )
{
    DWORD dwError = 0;
    PSTR  pszQualifiedName = NULL;

    dwError = LsaAllocateStringPrintf(
                    &pszQualifiedName,
                    "%s%c%s",
                    pszNetBIOSDomainName,
                    LsaGetDomainSeparator(),
                    pszName);
    BAIL_ON_LSA_ERROR(dwError);

    LsaStrnToUpper(pszQualifiedName, strlen(pszNetBIOSDomainName));

    LsaStrToLower(pszQualifiedName + strlen(pszNetBIOSDomainName) + 1);

    *ppszQualifiedName = pszQualifiedName;

cleanup:

    return dwError;

error:

    *ppszQualifiedName = NULL;

    LSA_SAFE_FREE_STRING(pszQualifiedName);

    goto cleanup;
}

DWORD
ADGetLDAPUPNString(
    IN OPTIONAL HANDLE hDirectory,
    IN OPTIONAL LDAPMessage* pMessage,
    IN PCSTR pszDnsDomainName,
    IN PCSTR pszSamaccountName,
    OUT PSTR* ppszUPN,
    OUT PBOOLEAN pbIsGeneratedUPN
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    LDAP *pLd = NULL;
    PSTR *ppszValues = NULL;
    PSTR pszUPN = NULL;
    BOOLEAN bIsGeneratedUPN = FALSE;

    if (hDirectory && pMessage)
    {
        pLd = LsaLdapGetSession(hDirectory);

        ppszValues = (PSTR*)ldap_get_values(pLd, pMessage, AD_LDAP_UPN_TAG);
        if (ppszValues && ppszValues[0])
        {
            dwError = LsaAllocateString(ppszValues[0], &pszUPN);
            BAIL_ON_LSA_ERROR(dwError);

            if (!index(pszUPN, '@'))
            {
                dwError = LSA_ERROR_DATA_ERROR;
                BAIL_ON_LSA_ERROR(dwError);
            }

            // Do not touch the non-realm part, just the realm part
            // to make sure the realm conforms to spec.
            LsaPrincipalRealmToUpper(pszUPN);
        }
    }

    if (!pszUPN)
    {
        dwError = LsaAllocateStringPrintf(
                        &pszUPN,
                        "%s@%s",
                        pszSamaccountName,
                        pszDnsDomainName);
        BAIL_ON_LSA_ERROR(dwError);

        bIsGeneratedUPN = TRUE;

        // If we genereate, we do lower@UPPER regardless of whatever
        // SAM account name case was provided.  (Note: It may be that
        // we should preseve case from the SAM account name, but we
        // would need to make sure that the SAM account name provided
        // to this function is matches the case in AD and is not derived
        // from something the user typed in locally.
        LsaPrincipalNonRealmToLower(pszUPN);
        LsaPrincipalRealmToUpper(pszUPN);
    }

    *ppszUPN = pszUPN;
    *pbIsGeneratedUPN = bIsGeneratedUPN;

cleanup:
    if (ppszValues)
    {
        ldap_value_free(ppszValues);
    }
    return dwError;

error:
    *ppszUPN = NULL;

    LSA_SAFE_FREE_STRING(pszUPN);

    goto cleanup;
}

DWORD
ADGetUserPrimaryGroupSid(
    IN PLSA_DM_LDAP_CONNECTION pConn,
    IN PCSTR pszDomainDnsName,
    IN PCSTR pszUserDN,
    IN PCSTR pszUserObjectsid,
    OUT PSTR* ppszPrimaryGroupSID
    )
{
    DWORD dwError = 0;
    LDAP* pLd = NULL;
    PLSA_SECURITY_IDENTIFIER pUserSID = NULL;
    PSTR pszPrimaryGroupSID = NULL;
    PSTR pszQuery = NULL;
    DWORD dwCount = 0;
    DWORD dwUserPrimaryGroupID = 0;
    LDAPMessage *pMessage = NULL;
    PSTR szAttributeListUserPrimeGID[] =
    {
        AD_LDAP_PRIMEGID_TAG,
        NULL
    };
    PSTR pszDirectoryRoot = NULL;
    HANDLE hDirectory = NULL;

    dwError = LsaAllocSecurityIdentifierFromString(
                            pszUserObjectsid,
                            &pUserSID);
    BAIL_ON_LSA_ERROR(dwError);

    // Find the user's primary group ID.
    dwError = LsaDmLdapDirectorySearch(
                    pConn,
                    pszUserDN,
                    LDAP_SCOPE_BASE,
                    "objectClass=*",
                    szAttributeListUserPrimeGID,
                    &hDirectory,
                    &pMessage);
    BAIL_ON_LSA_ERROR(dwError);

    pLd = LsaLdapGetSession(hDirectory);

    dwCount = ldap_count_entries(
                      pLd,
                      pMessage);
    if (dwCount != 1)
    {
        dwError = LSA_ERROR_LDAP_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaLdapGetUInt32(
                hDirectory,
                pMessage,
                AD_LDAP_PRIMEGID_TAG,
                &dwUserPrimaryGroupID);
    BAIL_ON_LSA_ERROR(dwError);

     // Find the primary group's SID.
    dwError = LsaSetSecurityIdentifierRid(
                pUserSID,
                dwUserPrimaryGroupID);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaGetSecurityIdentifierString(
                pUserSID,
                &pszPrimaryGroupSID);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszPrimaryGroupSID = pszPrimaryGroupSID;

cleanup:
    LSA_SAFE_FREE_STRING(pszQuery);
    LSA_SAFE_FREE_STRING(pszDirectoryRoot);
    if (pMessage)
    {
        ldap_msgfree(pMessage);
    }
    if (pUserSID)
    {
        LsaFreeSecurityIdentifier(pUserSID);
    }

    return dwError;

error:
    LSA_SAFE_FREE_STRING(pszPrimaryGroupSID);
    *ppszPrimaryGroupSID = NULL;

    goto cleanup;
}

DWORD
ADFindComputerDN(
    IN PLSA_DM_LDAP_CONNECTION pConn,
    PCSTR pszHostName,
    PCSTR pszDomainName,
    PSTR* ppszComputerDN
    )
{
    DWORD dwError = 0;
    LDAP *pLd = NULL;
    PSTR pszDirectoryRoot = NULL;
    PSTR szAttributeList[] = {"*", NULL};
    PSTR pszQuery = NULL;
    LDAPMessage *pMessage = NULL;
    DWORD dwCount = 0;
    PSTR pszComputerDN = NULL;
    PSTR pszEscapedUpperHostName = NULL;
    HANDLE hDirectory = NULL;

    dwError = LsaLdapConvertDomainToDN(pszDomainName, &pszDirectoryRoot);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLdapEscapeString(
                &pszEscapedUpperHostName,
                pszHostName);
    BAIL_ON_LSA_ERROR(dwError);

    LsaStrToUpper(pszEscapedUpperHostName);

    dwError = LsaAllocateStringPrintf(&pszQuery,
                                      "(sAMAccountName=%s$)",
                                      pszEscapedUpperHostName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaDmLdapDirectorySearch(
                    pConn,
                    pszDirectoryRoot,
                    LDAP_SCOPE_SUBTREE,
                    pszQuery,
                    szAttributeList,
                    &hDirectory,
                    &pMessage);
    BAIL_ON_LSA_ERROR(dwError);

    pLd = LsaLdapGetSession(hDirectory);

    dwCount = ldap_count_entries(
                pLd,
                pMessage
                );
    if (dwCount < 0) {
        dwError = LSA_ERROR_LDAP_ERROR;
    } else if (dwCount == 0) {
        dwError = LSA_ERROR_NO_SUCH_DOMAIN;
    } else if (dwCount > 1) {
        dwError = LSA_ERROR_DUPLICATE_DOMAINNAME;
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLdapGetDN(
                    hDirectory,
                    pMessage,
                    &pszComputerDN);
    BAIL_ON_LSA_ERROR(dwError);

    if (IsNullOrEmptyString(pszComputerDN))
    {
        dwError = LSA_ERROR_LDAP_FAILED_GETDN;
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppszComputerDN = pszComputerDN;

cleanup:
    LSA_SAFE_FREE_STRING(pszEscapedUpperHostName);
    LSA_SAFE_FREE_STRING(pszDirectoryRoot);
    LSA_SAFE_FREE_STRING(pszQuery);

    if (pMessage) {
        ldap_msgfree(pMessage);
    }

    return dwError;

error:

    *ppszComputerDN = NULL;
    LSA_SAFE_FREE_STRING(pszComputerDN);

    goto cleanup;
}

DWORD
ADGetCellInformation(
    IN PLSA_DM_LDAP_CONNECTION pConn,
    PCSTR  pszDN,
    PSTR*  ppszCellDN
    )
{
    DWORD dwError = 0;
    LDAP *pLd = NULL;
    PSTR szAttributeList[] = {"*", NULL};
    LDAPMessage *pMessage = NULL;
    DWORD dwCount = 0;
    PSTR pszCellDN = NULL;
    HANDLE hDirectory = NULL;

    dwError = LsaDmLdapDirectorySearch(
                    pConn,
                    pszDN,
                    LDAP_SCOPE_ONELEVEL,
                    "(name=$LikewiseIdentityCell)",
                    szAttributeList,
                    &hDirectory,
                    &pMessage);
    BAIL_ON_LSA_ERROR(dwError);

    pLd = LsaLdapGetSession(hDirectory);

    dwCount = ldap_count_entries(
                pLd,
                pMessage
                );
    if (dwCount < 0) {
        dwError = LSA_ERROR_LDAP_ERROR;
    } else if (dwCount == 0) {
        dwError = LSA_ERROR_NO_SUCH_CELL;
    } else if (dwCount > 1) {
        dwError = LSA_ERROR_INTERNAL;
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLdapGetDN(
                    hDirectory,
                    pMessage,
                    &pszCellDN);
    BAIL_ON_LSA_ERROR(dwError);

    if (IsNullOrEmptyString(pszCellDN))
    {
        dwError = LSA_ERROR_LDAP_FAILED_GETDN;
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppszCellDN = pszCellDN;

cleanup:

    if (pMessage) {
        ldap_msgfree(pMessage);
    }

    return dwError;

error:

    *ppszCellDN = NULL;

    LSA_SAFE_FREE_STRING(pszCellDN);

    goto cleanup;
}

DWORD
ADGetDomainMaxPwdAge(
    IN PLSA_DM_LDAP_CONNECTION pConn,
    PCSTR  pszDomainName,
    PUINT64 pMaxPwdAge)
{
    DWORD dwError = 0;
    LDAP *pLd = NULL;
    PSTR szAttributeList[] = {
            AD_LDAP_MAX_PWDAGE_TAG,
            NULL};
    LDAPMessage *pMessage = NULL;
    DWORD dwCount = 0;
    PSTR pszDirectoryRoot = NULL;
    int64_t int64MaxPwdAge = 0;
    HANDLE hDirectory = NULL;

    dwError = LsaLdapConvertDomainToDN(
                    pszDomainName,
                    &pszDirectoryRoot);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaDmLdapDirectorySearch(
                    pConn,
                    pszDirectoryRoot,
                    LDAP_SCOPE_BASE,
                    "(objectClass=*)",
                    szAttributeList,
                    &hDirectory,
                    &pMessage);
    BAIL_ON_LSA_ERROR(dwError);

    pLd = LsaLdapGetSession(hDirectory);

    dwCount = ldap_count_entries(
                pLd,
                pMessage
                );
    if (dwCount < 0) {
        dwError = LSA_ERROR_LDAP_ERROR;
    } else if (dwCount == 0) {
        dwError = LSA_ERROR_NO_SUCH_DOMAIN;
    } else if (dwCount > 1) {
        dwError = LSA_ERROR_DUPLICATE_DOMAINNAME;
    }
    BAIL_ON_LSA_ERROR(dwError);

    //process "maxPwdAge"
    dwError = LsaLdapGetInt64(
                hDirectory,
                pMessage,
                AD_LDAP_MAX_PWDAGE_TAG,
                &int64MaxPwdAge);
    BAIL_ON_LSA_ERROR(dwError);

    if (int64MaxPwdAge >= 0)
    {
        *pMaxPwdAge = (UINT64) int64MaxPwdAge;
    }
    else
    {
        *pMaxPwdAge = (UINT64) (0 - int64MaxPwdAge); // Store the abs value of this
    }

cleanup:

    LSA_SAFE_FREE_STRING(pszDirectoryRoot);

    if (pMessage) {
        ldap_msgfree(pMessage);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
ADGetConfigurationMode(
    IN PLSA_DM_LDAP_CONNECTION pConn,
    PCSTR  pszDN,
    ADConfigurationMode* pADConfMode
    )
{
    DWORD dwError = 0;
    LDAP *pLd = NULL;
    PSTR szAttributeList[] = {AD_LDAP_DESCRIPTION_TAG, NULL};
    LDAPMessage *pMessage = NULL;
    DWORD dwCount = 0;
    ADConfigurationMode adConfMode = NonSchemaMode;
    HANDLE hDirectory = NULL;

    PSTR* ppszValues = NULL;
    DWORD dwNumValues = 0;
    DWORD i = 0;

    BAIL_ON_INVALID_POINTER(pConn);

    dwError = LsaDmLdapDirectorySearch(
                    pConn,
                    pszDN,
                    LDAP_SCOPE_BASE,
                    "(objectClass=*)",
                    szAttributeList,
                    &hDirectory,
                    &pMessage);
    if (dwError == LDAP_NO_SUCH_OBJECT){
        dwError = LSA_ERROR_INCOMPATIBLE_MODES_BETWEEN_TRUSTEDDOMAINS;
    }
    BAIL_ON_LSA_ERROR(dwError);

    pLd = LsaLdapGetSession(hDirectory);

    dwCount = ldap_count_entries(
                pLd,
                pMessage
                );
    if (dwCount < 0) {
        dwError = LSA_ERROR_LDAP_ERROR;
    } else if (dwCount == 0) {
        dwError = LSA_ERROR_NO_SUCH_CELL;
    } else if (dwCount > 1) {
        dwError = LSA_ERROR_INTERNAL;
    }

    dwError = LsaLdapGetStrings(
                    hDirectory,
                    pMessage,
                    AD_LDAP_DESCRIPTION_TAG,
                    &ppszValues,
                    &dwNumValues);
    BAIL_ON_LSA_ERROR(dwError);

    for (i = 0; i < dwNumValues; i++)
    {
        if (!strncasecmp(ppszValues[i], "use2307Attrs=", sizeof("use2307Attrs=")-1))
        {
           PSTR pszValue = ppszValues[i] + sizeof("use2307Attrs=") - 1;
           if (!IsNullOrEmptyString(pszValue) && !strcasecmp(pszValue, "true")) {
              adConfMode = SchemaMode;
              break;
           }
        }
    }

    *pADConfMode = adConfMode;

cleanup:

    if (pMessage) {
        ldap_msgfree(pMessage);
    }

    if (ppszValues) {
        LsaFreeStringArray(ppszValues, dwNumValues);
    }

    return dwError;

error:

    *pADConfMode = UnknownMode;

    goto cleanup;
}

#define AD_GUID_SIZE 16

DWORD
ADGuidStrToHex(
        PCSTR pszStr,
        PSTR* ppszHexStr)
{
   DWORD dwError = 0;
   PSTR pszHexStr = NULL;
   PSTR pszUUIDStr = NULL;
   int iValue = 0, jValue = 0;
   uuid_t uuid= {0};
   unsigned char temp;

   BAIL_ON_INVALID_STRING(pszStr);

   dwError = LsaAllocateString(
                 pszStr,
                 &pszUUIDStr);
   BAIL_ON_LSA_ERROR(dwError);

   if (uuid_parse(pszUUIDStr, uuid) < 0) {
       dwError = LSA_ERROR_INVALID_OBJECTGUID;
       BAIL_ON_LSA_ERROR(dwError);
   }

   for(iValue = 0; iValue < 2; iValue++){
       temp = uuid[iValue];
       uuid[iValue] = uuid[3-iValue];
       uuid[3-iValue] = temp;
   }
   temp = uuid[4];
   uuid[4] = uuid[5];
   uuid[5] = temp;

   temp = uuid[6];
   uuid[6] = uuid[7];
   uuid[7] = temp;

   dwError = LsaAllocateMemory(
                sizeof(CHAR)*(AD_GUID_SIZE*3+1),
               (PVOID*)&pszHexStr);
   BAIL_ON_LSA_ERROR(dwError);

   for (iValue = 0, jValue = 0; jValue < AD_GUID_SIZE; ){
       if (iValue % 3 == 0){
           *((char*)(pszHexStr+iValue++)) = '\\';
       }
       else{
           sprintf((char*)pszHexStr+iValue, "%.2X", uuid[jValue]);
           iValue += 2;
           jValue++;
       }
   }

   *ppszHexStr = pszHexStr;

cleanup:

    LSA_SAFE_FREE_STRING(pszUUIDStr);

    return dwError;

error:

    *ppszHexStr = NULL;

    LSA_SAFE_FREE_STRING(pszHexStr);

    goto cleanup;
}

DWORD
ADCopyAttributeList(
    PSTR   szAttributeList[],
    PSTR** pppOutputAttributeList)
{
    DWORD dwError = 0;
    size_t sAttrListSize = 0, iValue = 0;
    PSTR* ppOutputAttributeList = NULL;

    if (!szAttributeList){
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    while (szAttributeList[sAttrListSize]){
        sAttrListSize++;
    }
    sAttrListSize++;

    dwError = LsaAllocateMemory(
                sAttrListSize * sizeof(PSTR),
                (PVOID*)&ppOutputAttributeList);
    BAIL_ON_LSA_ERROR(dwError);

    for (iValue = 0; iValue < sAttrListSize - 1; iValue++){
        dwError = LsaAllocateString(
                      szAttributeList[iValue],
                      &ppOutputAttributeList[iValue]);
        BAIL_ON_LSA_ERROR(dwError);
    }
    ppOutputAttributeList[iValue] = NULL;

    *pppOutputAttributeList = ppOutputAttributeList;

cleanup:

    return dwError;

error:
    LsaFreeNullTerminatedStringArray(ppOutputAttributeList);

    *pppOutputAttributeList = NULL;

    goto cleanup;
}

DWORD
ADGetUserOrGroupRealAttributeList(
    DWORD dwDirectoryMode,
    ADConfigurationMode adConfMode,
    PSTR** pppRealAttributeList)
{
    DWORD dwError = 0;
    PSTR* ppRealAttributeList = NULL;

    PSTR szRealAttributeListDefaultSchema[] =
        {
            AD_LDAP_OBJECTCLASS_TAG,
            AD_LDAP_OBJECTSID_TAG,
            AD_LDAP_UID_TAG,
            AD_LDAP_GID_TAG,
            AD_LDAP_SAM_NAME_TAG,
            AD_LDAP_PASSWD_TAG,
            AD_LDAP_HOMEDIR_TAG,
            AD_LDAP_SHELL_TAG,
            AD_LDAP_GECOS_TAG,
            AD_LDAP_SEC_DESC_TAG,
            AD_LDAP_UPN_TAG,
            AD_LDAP_USER_CTRL_TAG,
            AD_LDAP_PWD_LASTSET_TAG,
            AD_LDAP_ACCOUT_EXP_TAG,
            AD_LDAP_ALIAS_TAG,
            AD_LDAP_DISPLAY_NAME_TAG,
            NULL
        };

    PSTR szRealAttributeListOther[] =
        {
            AD_LDAP_OBJECTCLASS_TAG,
            AD_LDAP_OBJECTSID_TAG,
            AD_LDAP_UPN_TAG,
            AD_LDAP_SAM_NAME_TAG,
            AD_LDAP_USER_CTRL_TAG,
            AD_LDAP_PWD_LASTSET_TAG,
            AD_LDAP_ACCOUT_EXP_TAG,
            NULL
        };

    PSTR szRealAttributeListUnprovision[] =
        {
             AD_LDAP_OBJECTCLASS_TAG,
             AD_LDAP_OBJECTSID_TAG,
             AD_LDAP_NAME_TAG,
             AD_LDAP_DISPLAY_NAME_TAG,
             AD_LDAP_SAM_NAME_TAG,
             AD_LDAP_PRIMEGID_TAG,
             AD_LDAP_UPN_TAG,
             AD_LDAP_USER_CTRL_TAG,
             AD_LDAP_PWD_LASTSET_TAG,
             AD_LDAP_ACCOUT_EXP_TAG,
             NULL
        };

    switch (dwDirectoryMode)
    {
        case DEFAULT_MODE:
            if (adConfMode == SchemaMode){
                dwError = ADCopyAttributeList(
                                szRealAttributeListDefaultSchema,
                                &ppRealAttributeList);
            }
            else if (adConfMode == NonSchemaMode){
                dwError = ADCopyAttributeList(
                                szRealAttributeListOther,
                                &ppRealAttributeList);
            }
            else{
                dwError = LSA_ERROR_INVALID_PARAMETER;
            }
            BAIL_ON_LSA_ERROR(dwError);

            break;

        case CELL_MODE:
            dwError = ADCopyAttributeList(
                            szRealAttributeListOther,
                            &ppRealAttributeList);
            BAIL_ON_LSA_ERROR(dwError);

            break;

        case UNPROVISIONED_MODE:
            dwError = ADCopyAttributeList(
                            szRealAttributeListUnprovision,
                            &ppRealAttributeList);
            BAIL_ON_LSA_ERROR(dwError);

            break;

        default:
            dwError = LSA_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
    }

    *pppRealAttributeList = ppRealAttributeList;

cleanup:
    return dwError;

error:
     LsaFreeNullTerminatedStringArray(ppRealAttributeList);
    *pppRealAttributeList = NULL;

    goto cleanup;
}

DWORD
ADGetUserRealAttributeList(
    DWORD dwDirectoryMode,
    ADConfigurationMode adConfMode,
    PSTR** pppRealAttributeList)
{
    DWORD dwError = 0;
    PSTR* ppRealAttributeList = NULL;

    PSTR szRealAttributeListDefaultSchema[] =
        {
            AD_LDAP_OBJECTSID_TAG,
            AD_LDAP_UID_TAG,
            AD_LDAP_GID_TAG,
            AD_LDAP_SAM_NAME_TAG,
            AD_LDAP_PASSWD_TAG,
            AD_LDAP_HOMEDIR_TAG,
            AD_LDAP_SHELL_TAG,
            AD_LDAP_GECOS_TAG,
            AD_LDAP_SEC_DESC_TAG,
            AD_LDAP_UPN_TAG,
            AD_LDAP_USER_CTRL_TAG,
            AD_LDAP_PWD_LASTSET_TAG,
            AD_LDAP_ACCOUT_EXP_TAG,
            AD_LDAP_OBJECTSID_TAG,
            AD_LDAP_ALIAS_TAG,
            NULL
        };

    PSTR szRealAttributeListOther[] =
        {
            AD_LDAP_OBJECTSID_TAG,
            AD_LDAP_UPN_TAG,
            AD_LDAP_SAM_NAME_TAG,
            AD_LDAP_USER_CTRL_TAG,
            AD_LDAP_PWD_LASTSET_TAG,
            AD_LDAP_ACCOUT_EXP_TAG,
            NULL
        };

    PSTR szRealAttributeListUnprovision[] =
        {
             AD_LDAP_OBJECTSID_TAG,
             AD_LDAP_NAME_TAG,
             AD_LDAP_DISPLAY_NAME_TAG,
             AD_LDAP_SAM_NAME_TAG,
             AD_LDAP_PRIMEGID_TAG,
             AD_LDAP_UPN_TAG,
             AD_LDAP_USER_CTRL_TAG,
             AD_LDAP_PWD_LASTSET_TAG,
             AD_LDAP_ACCOUT_EXP_TAG,
             NULL
        };

    switch (dwDirectoryMode)
    {
        case DEFAULT_MODE:
            if (adConfMode == SchemaMode){
                dwError = ADCopyAttributeList(
                                szRealAttributeListDefaultSchema,
                                &ppRealAttributeList);
            }
            else if (adConfMode == NonSchemaMode){
                dwError = ADCopyAttributeList(
                                szRealAttributeListOther,
                                &ppRealAttributeList);
            }
            else{
                dwError = LSA_ERROR_INVALID_PARAMETER;
            }
            BAIL_ON_LSA_ERROR(dwError);

            break;

        case CELL_MODE:
            dwError = ADCopyAttributeList(
                            szRealAttributeListOther,
                            &ppRealAttributeList);
            BAIL_ON_LSA_ERROR(dwError);

            break;

        case UNPROVISIONED_MODE:
            dwError = ADCopyAttributeList(
                            szRealAttributeListUnprovision,
                            &ppRealAttributeList);
            BAIL_ON_LSA_ERROR(dwError);

            break;

        default:
            dwError = LSA_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
    }

    *pppRealAttributeList = ppRealAttributeList;

cleanup:
    return dwError;

error:
     LsaFreeNullTerminatedStringArray(ppRealAttributeList);
    *pppRealAttributeList = NULL;

    goto cleanup;
}

DWORD
ADGetUserPseudoAttributeList(
    ADConfigurationMode adConfMode,
    PSTR** pppPseudoAttributeList)
{
    DWORD dwError = 0;
    PSTR* ppPseudoAttributeList = NULL;

    PSTR szPseudoAttributeListSchema[] =
        {
                AD_LDAP_UID_TAG,
                AD_LDAP_GID_TAG,
                AD_LDAP_NAME_TAG,
                AD_LDAP_PASSWD_TAG,
                AD_LDAP_HOMEDIR_TAG,
                AD_LDAP_SHELL_TAG,
                AD_LDAP_GECOS_TAG,
                AD_LDAP_SEC_DESC_TAG,
                AD_LDAP_KEYWORDS_TAG,
                AD_LDAP_ALIAS_TAG,
                NULL
        };

    PSTR szPseudoAttributeListNonSchema[] =
         {
             AD_LDAP_NAME_TAG,
             AD_LDAP_KEYWORDS_TAG,
             NULL
         };

    switch (adConfMode)
    {
        case SchemaMode:
            dwError = ADCopyAttributeList(
                            szPseudoAttributeListSchema,
                            &ppPseudoAttributeList);
            BAIL_ON_LSA_ERROR(dwError);

            break;

        case NonSchemaMode:
            dwError = ADCopyAttributeList(
                             szPseudoAttributeListNonSchema,
                             &ppPseudoAttributeList);
             BAIL_ON_LSA_ERROR(dwError);

            break;

        default:
            dwError = LSA_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
    }

    *pppPseudoAttributeList = ppPseudoAttributeList;

cleanup:
    return dwError;

error:
     LsaFreeNullTerminatedStringArray(ppPseudoAttributeList);
    *pppPseudoAttributeList = NULL;
    goto cleanup;
}

//
// DWORD dwID - this is assumed to be a hashed UID or GID.
//
DWORD
UnprovisionedModeMakeLocalSID(
    PCSTR pszDomainSID,
    DWORD dwID,
    PSTR* ppszLocalSID
    )
{
    DWORD dwError = 0;
    PSTR pszUnhashedLocalSID = NULL;
    DWORD dwUnhashedLocalRID = 0;
    DWORD dwHashedLocalRID = 0;
    PLSA_SECURITY_IDENTIFIER pUnhashedLocalSID = NULL;

    dwUnhashedLocalRID = dwID & 0x0007FFFF;

    dwError = LsaAllocateStringPrintf(&pszUnhashedLocalSID,
                    "%s-%u",
                    pszDomainSID,
                    dwUnhashedLocalRID);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAllocSecurityIdentifierFromString(
                    pszUnhashedLocalSID,
                    &pUnhashedLocalSID);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaGetSecurityIdentifierHashedRid(
                    pUnhashedLocalSID,
                    &dwHashedLocalRID);
    BAIL_ON_LSA_ERROR(dwError);

    //The user of this function is expected to provide
    //a hashed ID; applying the hash algorithm against
    //it and the root domain SID should not alter its value.
    //If the ID is below 1000, however, it likely represents
    //a builtin object like "Administrator" or Guests, and therefore
    //will use a domain like S-1-5-32, not the root domain
    //The check attempted below would have no meaning, since the domain
    //is not known.
    //TODO: use logic from list of well-known SID's to check SID validity
    //see: http://support.microsoft.com/kb/243330
    if (dwHashedLocalRID != dwID)
    {
        if (dwID >= 1000)
        {
            dwError = LSA_ERROR_NO_SUCH_USER_OR_GROUP;
            BAIL_ON_LSA_ERROR(dwError);
        }
        else  //dwID < 1000.  Try again using domain for builtin SIDs
        {
            PCSTR pszBuiltinDomainSID = "S-1-5-32";
            LSA_SAFE_FREE_STRING(pszUnhashedLocalSID);

            if (pUnhashedLocalSID)
            {
                LsaFreeSecurityIdentifier(pUnhashedLocalSID);
                pUnhashedLocalSID = NULL;
            }

            dwError = LsaAllocateStringPrintf(&pszUnhashedLocalSID,
                            "%s-%u",
                            pszBuiltinDomainSID,
                            dwUnhashedLocalRID);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaAllocSecurityIdentifierFromString(
                            pszUnhashedLocalSID,
                            &pUnhashedLocalSID);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaGetSecurityIdentifierHashedRid(
                            pUnhashedLocalSID,
                            &dwHashedLocalRID);
            BAIL_ON_LSA_ERROR(dwError);

            if (dwHashedLocalRID != dwID)
            {
                dwError = LSA_ERROR_NO_SUCH_USER_OR_GROUP;
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
    }

    *ppszLocalSID = pszUnhashedLocalSID;

cleanup:

    if (pUnhashedLocalSID != NULL)
    {
        LsaFreeSecurityIdentifier(pUnhashedLocalSID);
    }

    return dwError;

error:
    LSA_SAFE_FREE_STRING(pszUnhashedLocalSID);
    *ppszLocalSID = NULL;

    goto cleanup;

}

DWORD
ADGetGroupRealAttributeList(
    DWORD dwDirectoryMode,
    ADConfigurationMode adConfMode,
    PSTR** pppRealAttributeList)
{
    DWORD dwError = 0;
    PSTR* ppRealAttributeList = NULL;

    PSTR szRealAttributeListDefaultSchema[] =
        {
            AD_LDAP_OBJECTSID_TAG,
            AD_LDAP_GID_TAG,
            AD_LDAP_SAM_NAME_TAG,
            AD_LDAP_PASSWD_TAG,
            AD_LDAP_UPN_TAG,
            AD_LDAP_MEMBER_TAG,
            AD_LDAP_DISPLAY_NAME_TAG,
            NULL
        };

    PSTR szRealAttributeListOther[] =
        {
            AD_LDAP_OBJECTSID_TAG,
            AD_LDAP_UPN_TAG,
            AD_LDAP_SAM_NAME_TAG,
            AD_LDAP_MEMBER_TAG,
            NULL
        };

    PSTR szRealAttributeListUnprovision[] =
        {
             AD_LDAP_OBJECTSID_TAG,
             AD_LDAP_NAME_TAG,
             AD_LDAP_DISPLAY_NAME_TAG,
             AD_LDAP_SAM_NAME_TAG,
             AD_LDAP_UPN_TAG,
             AD_LDAP_MEMBER_TAG,
             NULL
        };

    switch (dwDirectoryMode)
    {
        case DEFAULT_MODE:
            if (adConfMode == SchemaMode){
                dwError = ADCopyAttributeList(
                                szRealAttributeListDefaultSchema,
                                &ppRealAttributeList);
            }
            else if (adConfMode == NonSchemaMode){
                dwError = ADCopyAttributeList(
                                szRealAttributeListOther,
                                &ppRealAttributeList);
            }
            else{
                dwError = LSA_ERROR_INVALID_PARAMETER;
            }
            BAIL_ON_LSA_ERROR(dwError);

            break;

        case CELL_MODE:
            dwError = ADCopyAttributeList(
                            szRealAttributeListOther,
                            &ppRealAttributeList);
            BAIL_ON_LSA_ERROR(dwError);

            break;

        case UNPROVISIONED_MODE:
            dwError = ADCopyAttributeList(
                            szRealAttributeListUnprovision,
                            &ppRealAttributeList);
            BAIL_ON_LSA_ERROR(dwError);

            break;

        default:
            dwError = LSA_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
    }

    *pppRealAttributeList = ppRealAttributeList;

cleanup:
    return dwError;

error:
     LsaFreeNullTerminatedStringArray(ppRealAttributeList);
    *pppRealAttributeList = NULL;

    goto cleanup;
}

DWORD
ADGetGroupPseudoAttributeList(
    ADConfigurationMode adConfMode,
    PSTR** pppPseudoAttributeList)
{
    DWORD dwError = 0;
    PSTR* ppPseudoAttributeList = NULL;

    PSTR szPseudoAttributeListSchema[] =
        {
                AD_LDAP_GID_TAG,
                AD_LDAP_NAME_TAG,
                AD_LDAP_PASSWD_TAG,
                AD_LDAP_KEYWORDS_TAG,
                AD_LDAP_MEMBER_TAG,
                AD_LDAP_SAM_NAME_TAG,
                AD_LDAP_DISPLAY_NAME_TAG,
                NULL
        };

    PSTR szPseudoAttributeListNonSchema[] =
         {
             AD_LDAP_NAME_TAG,
             AD_LDAP_KEYWORDS_TAG,
             NULL
         };

    switch (adConfMode)
    {
        case SchemaMode:
            dwError = ADCopyAttributeList(
                            szPseudoAttributeListSchema,
                            &ppPseudoAttributeList);
            BAIL_ON_LSA_ERROR(dwError);

            break;

        case NonSchemaMode:
            dwError = ADCopyAttributeList(
                             szPseudoAttributeListNonSchema,
                             &ppPseudoAttributeList);
             BAIL_ON_LSA_ERROR(dwError);

            break;

        default:
            dwError = LSA_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
    }

    *pppPseudoAttributeList = ppPseudoAttributeList;

cleanup:
    return dwError;

error:
     LsaFreeNullTerminatedStringArray(ppPseudoAttributeList);
    *pppPseudoAttributeList = NULL;
    goto cleanup;
}

static
VOID
DestroyQueryListEntry(
    IN OUT PLSA_AD_QUERY_LISTS_ENTRY* ppEntry
    )
{
    PLSA_AD_QUERY_LISTS_ENTRY pEntry = *ppEntry;
    if (pEntry)
    {
        LsaFreeStringArray(pEntry->ppszQueryValues, pEntry->dwQueryCount);
        LsaFreeMemory(pEntry);
        *ppEntry = NULL;
    }
}

static
DWORD
CreateQueryListEntry(
    OUT PLSA_AD_QUERY_LISTS_ENTRY* ppEntry,
    IN DWORD dwQueryCount,
    IN PSTR* ppszQueryValues
    )
{
    DWORD dwError = 0;
    PLSA_AD_QUERY_LISTS_ENTRY pEntry = NULL;

    dwError = LsaAllocateMemory(sizeof(*pEntry), (PVOID*)&pEntry);
    BAIL_ON_LSA_ERROR(dwError);

    pEntry->dwQueryCount = dwQueryCount;
    pEntry->ppszQueryValues = ppszQueryValues;

    *ppEntry = pEntry;

cleanup:
    return dwError;

error:
    *ppEntry = NULL;
    DestroyQueryListEntry(&pEntry);
    goto cleanup;
}

// Give an attribute name "pszAttributeName"
// Return all the values of the attribute for a given DN "pszDN"
// The number of values can be more than 1500 since "ranging" is handled correctly in the routine
// Now accept whether to do an extended DN search and whether to parse result to get Sids
DWORD
ADLdap_GetAttributeValuesList(
    IN PLSA_DM_LDAP_CONNECTION pConn,
    IN PCSTR pszDN,
    IN PCSTR pszAttributeName,
    IN BOOLEAN bDoExtDnSearch,
    IN BOOLEAN bDoSidParsing,
    OUT PDWORD pdwTotalCount,
    OUT PSTR** pppszValues
    )
{
    DWORD dwError = 0;
    // Do not free "szAttributeList"
    PSTR szAttributeList[2] = {NULL,NULL};
    PSTR* ppszValuesTotal = NULL;
    PSTR* ppszValues = NULL;
    LDAPMessage* pMessage = NULL;
    DWORD dwCount = 0;
    DWORD dwTotalCount = 0;
    PDLINKEDLIST pList = NULL;
    PDLINKEDLIST pNode = NULL;
    PLSA_AD_QUERY_LISTS_ENTRY pEntry = NULL;
    PSTR pszRangeAttr = NULL;
    LDAP* pLd = NULL;
    BerElement* pBer = NULL;
    PSTR pszRetrievedAttr = NULL;
    //Do Not free "pszRetrievedRangeAttr"
    PSTR pszRetrievedRangeAttr = NULL;
    BOOLEAN bIsEnd = FALSE;
    DWORD iValues = 0;
    DWORD iValuesTotal = 0;
    PSTR pszAttributeRangedName = NULL;
    HANDLE hDirectory = NULL;

    BAIL_ON_INVALID_STRING(pszAttributeName);
    szAttributeList[0] = (PSTR)pszAttributeName;

    for (;;)
    {
        if (pMessage)
        {
            ldap_msgfree(pMessage);
            pMessage = NULL;
        }

        if (!bDoExtDnSearch && bDoSidParsing)
        {
            dwError = LSA_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
        }

        if (bDoExtDnSearch)
        {
            dwError = LsaDmLdapDirectoryExtendedDNSearch(
                            pConn,
                            pszDN,
                            "(objectClass=*)",
                            szAttributeList,
                            LDAP_SCOPE_BASE,
                            &hDirectory,
                            &pMessage);
            BAIL_ON_LSA_ERROR(dwError);
        }
        else
        {
            dwError = LsaDmLdapDirectorySearch(
                            pConn,
                            pszDN,
                            LDAP_SCOPE_BASE,
                            "(objectClass=*)",
                            szAttributeList,
                            &hDirectory,
                            &pMessage);
            BAIL_ON_LSA_ERROR(dwError);
        }
        pLd = LsaLdapGetSession(hDirectory);

        dwError = LsaLdapGetStringsWithExtDnResult(
                        hDirectory,
                        pMessage,
                        pszAttributeName,
                        bDoSidParsing,
                        &ppszValues,
                        &dwCount);
        BAIL_ON_LSA_ERROR(dwError);

        if (ppszValues && dwCount)
        {
            if (pList)
            {
                // This is the case where we started out getting
                // ranged info but the info became non-ranged.
                // We might actually want to allow this to handle
                // a case where the membership list is trimmed
                // while we are enumerating.
                dwError = LSA_ERROR_LDAP_ERROR;
                BAIL_ON_LSA_ERROR(dwError);
            }

            dwTotalCount = dwCount;
            ppszValuesTotal = ppszValues;

            dwCount = 0;
            ppszValues = NULL;

            break;
        }

        if (pszRetrievedAttr)
        {
            ldap_memfree(pszRetrievedAttr);
            pszRetrievedAttr = NULL;
        }

        if (pBer)
        {
             ber_free(pBer, 0);
        }

        LSA_SAFE_FREE_STRING(pszAttributeRangedName);

        dwError = LsaAllocateStringPrintf(
                      &pszAttributeRangedName,
                      "%s;Range=",
                      pszAttributeName);
        BAIL_ON_LSA_ERROR(dwError);

        pszRetrievedAttr = ldap_first_attribute(pLd, pMessage, &pBer);
        while (pszRetrievedAttr)
        {
            if (!strncasecmp(pszRetrievedAttr, pszAttributeRangedName, strlen(pszAttributeRangedName)))
            {
                pszRetrievedRangeAttr = pszRetrievedAttr;
                break;
            }
            ldap_memfree(pszRetrievedAttr);
            pszRetrievedAttr = NULL;

            pszRetrievedAttr = ldap_next_attribute(pLd, pMessage, pBer);
        }

        if (!pszRetrievedRangeAttr)
        {
            // This happens when we have an group with no members,
            break;
        }

        if ('*' == pszRetrievedRangeAttr[strlen(pszRetrievedRangeAttr)-1])
        {
            bIsEnd = TRUE;
        }

        dwError = LsaLdapGetStringsWithExtDnResult(
                        hDirectory,
                        pMessage,
                        pszRetrievedRangeAttr,
                        bDoSidParsing,
                        &ppszValues,
                        &dwCount);
        BAIL_ON_LSA_ERROR(dwError);

        dwTotalCount += dwCount;

        dwError = CreateQueryListEntry(
                        &pEntry,
                        dwCount,
                        ppszValues);
        BAIL_ON_LSA_ERROR(dwError);
        ppszValues = NULL;
        dwCount = 0;

        dwError = LsaDLinkedListPrepend(&pList, pEntry);
        BAIL_ON_LSA_ERROR(dwError);
        pEntry = NULL;

        if (bIsEnd)
        {
            break;
        }

        LSA_SAFE_FREE_STRING(pszRangeAttr);

        dwError = LsaAllocateStringPrintf(
                        &pszRangeAttr,
                        "%s%d-*",
                        pszAttributeRangedName,
                        dwTotalCount);
        BAIL_ON_LSA_ERROR(dwError);

        szAttributeList[0] = pszRangeAttr;
    }

    if (pList && !ppszValuesTotal)
    {
        dwError = LsaAllocateMemory(
                        sizeof(*ppszValuesTotal) * dwTotalCount,
                        (PVOID*)&ppszValuesTotal);
        BAIL_ON_LSA_ERROR(dwError);

        for (pNode = pList; pNode; pNode = pNode->pNext)
        {
            PLSA_AD_QUERY_LISTS_ENTRY pEntry = (PLSA_AD_QUERY_LISTS_ENTRY)pNode->pItem;

            for (iValues = 0; iValues < pEntry->dwQueryCount; iValues++)
            {
                ppszValuesTotal[iValuesTotal] = pEntry->ppszQueryValues[iValues];
                pEntry->ppszQueryValues[iValues] = NULL;
                iValuesTotal++;
            }
        }
    }

    *pdwTotalCount = dwTotalCount;
    *pppszValues = ppszValuesTotal;

cleanup:
    if (pMessage)
    {
        ldap_msgfree(pMessage);
    }

    if (pszRetrievedAttr)
    {
        ldap_memfree(pszRetrievedAttr);
    }

    if (pBer)
    {
        ber_free(pBer, 0);
    }

    LsaFreeStringArray(ppszValues, dwCount);
    DestroyQueryListEntry(&pEntry);
    LSA_SAFE_FREE_STRING(pszAttributeRangedName);
    LSA_SAFE_FREE_STRING(pszRangeAttr);

    for (pNode = pList; pNode; pNode = pNode->pNext)
    {
        PLSA_AD_QUERY_LISTS_ENTRY pEntry = (PLSA_AD_QUERY_LISTS_ENTRY)pNode->pItem;
        DestroyQueryListEntry(&pEntry);
    }
    LsaDLinkedListFree(pList);

    return dwError;

error:
    LsaFreeStringArray(ppszValuesTotal, iValuesTotal);

    *pdwTotalCount = 0;
    *pppszValues = NULL;

    goto cleanup;
}

DWORD
ADLdap_GetGroupMembers(
    IN HANDLE hProvider,
    IN PCSTR pszDomainName,
    IN PCSTR pszSid,
    OUT size_t* psCount,
    OUT PLSA_SECURITY_OBJECT** pppResults
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    DWORD dwSidCount = 0;
    PLSA_SECURITY_OBJECT pGroupObj = NULL;
    PLSA_SECURITY_OBJECT* ppResults = NULL;
    PSTR *ppszLDAPValues = NULL;
    size_t sFoundCount = 0;
    PLSA_DM_LDAP_CONNECTION pConn = NULL;

    dwError = AD_FindObjectBySid(
                    hProvider,
                    pszSid,
                    &pGroupObj);
    BAIL_ON_LSA_ERROR(dwError);

    if (pGroupObj->type != AccountType_Group)
    {
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaDmLdapOpenDc(pszDomainName, &pConn);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADLdap_GetAttributeValuesList(
                    pConn,
                    pGroupObj->pszDN,
                    AD_LDAP_MEMBER_TAG,
                    TRUE,
                    TRUE,
                    &dwSidCount,
                    &ppszLDAPValues);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_FindObjectsBySidList(
                 hProvider,
                 dwSidCount,
                 ppszLDAPValues,
                 &sFoundCount,
                 &ppResults);
    BAIL_ON_LSA_ERROR(dwError);

    *psCount = sFoundCount;
    *pppResults = ppResults;

cleanup:
    LsaDbSafeFreeObject(&pGroupObj);
    LsaFreeStringArray(ppszLDAPValues, dwSidCount);
    LsaDmLdapClose(pConn);

    return dwError;

error:
    *psCount = 0;
    *pppResults = NULL;

    LSA_LOG_ERROR("Failed to find group's members of objectSid=%s. [error code:%d]",
                  LSA_SAFE_LOG_STRING(pszSid), dwError);

    LsaDbSafeFreeObjectList((DWORD)sFoundCount, &ppResults);
    goto cleanup;
}

DWORD
ADLdap_GetUserGroupMembership(
    IN HANDLE hProvider,
    IN PLSA_SECURITY_OBJECT pUserInfo,
    OUT int* piPrimaryGroupIndex,
    OUT size_t* psNumGroupsFound,
    OUT PLSA_SECURITY_OBJECT** pppGroupInfoList
    )
{
    DWORD dwError =  0;
    PLSA_DM_LDAP_CONNECTION pConn = NULL;
    PSTR pszPrimaryGroupSID = NULL;
    PSTR pszFullDomainName = NULL;
    INT i = 0;
    PLSA_SECURITY_OBJECT* ppGroupInfoList = NULL;
    size_t sNumGroupsFound = 0;
    int    iPrimaryGroupIndex = -1;
    DWORD dwSidCount = 0;
    PSTR *ppszLDAPValues = NULL;
    PSTR *ppszTempLDAPValues = NULL;

    // If we cannot get dn, then we cannot get DN information for this objects, hence BAIL
    if (IsNullOrEmptyString(pUserInfo->pszDN))
    {
        dwError = LSA_ERROR_NO_SUCH_USER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaLdapConvertDNToDomain(
                 pUserInfo->pszDN,
                 &pszFullDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaDmLdapOpenDc(pszFullDomainName, &pConn);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADLdap_GetAttributeValuesList(
                    pConn,
                    pUserInfo->pszDN,
                    AD_LDAP_MEMBEROF_TAG,
                    TRUE,
                    TRUE,
                    &dwSidCount,
                    &ppszLDAPValues);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADGetUserPrimaryGroupSid(
                pConn,
                pszFullDomainName,
                pUserInfo->pszDN,
                pUserInfo->pszObjectSid,
                &pszPrimaryGroupSID);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaReallocMemory(
                ppszLDAPValues,
                (PVOID*)&ppszTempLDAPValues,
                (dwSidCount+1)*sizeof(*ppszLDAPValues));
    BAIL_ON_LSA_ERROR(dwError);

    // Do not free "ppszTempLDAPValues"
    ppszLDAPValues = ppszTempLDAPValues;

    // Append the pszPrimaryGroupSID entry to the results list
    ppszLDAPValues[dwSidCount] = pszPrimaryGroupSID;
    pszPrimaryGroupSID = NULL;
    dwSidCount++;

    dwError = AD_FindObjectsBySidList(
            hProvider,
            dwSidCount,
            ppszLDAPValues,
            &sNumGroupsFound,
            &ppGroupInfoList);
    BAIL_ON_LSA_ERROR(dwError);

    // Determine primary group index
    if (ppGroupInfoList && sNumGroupsFound)
    {
        for (i = (INT)sNumGroupsFound - 1; i >= 0; i--)
        {
            // ppszLDAPValues[dwSidCount-1] stores user's primiary group Sid
            if (!strcmp(ppGroupInfoList[i]->pszObjectSid,  ppszLDAPValues[dwSidCount-1]))
            {
                iPrimaryGroupIndex = i;
                break;
            }
        }
    }

    *psNumGroupsFound = sNumGroupsFound;
    *pppGroupInfoList = ppGroupInfoList;
    *piPrimaryGroupIndex = iPrimaryGroupIndex;

cleanup:
    LSA_SAFE_FREE_STRING(pszFullDomainName);
    LsaFreeStringArray(ppszLDAPValues, dwSidCount);
    LSA_SAFE_FREE_STRING(pszPrimaryGroupSID);

    LsaDmLdapClose(pConn);

    return dwError;

error:
    *pppGroupInfoList = NULL;
    *psNumGroupsFound = 0;
    *piPrimaryGroupIndex = -1;

    if ( dwError != LSA_ERROR_DOMAIN_IS_OFFLINE )
    {
        LSA_LOG_ERROR("Failed to find user's group memberships of UID=%d. [error code:%d]",
                      pUserInfo->userInfo.uid, dwError);
    }

    LsaDbSafeFreeObjectList((DWORD)sNumGroupsFound, &ppGroupInfoList);

    goto cleanup;
}

DWORD
ADLdap_IsValidDN(
    IN PLSA_DM_LDAP_CONNECTION pConn,
    PCSTR    pszDN,
    PBOOLEAN pbValidDN
    )
{
    DWORD dwError = 0;
    PSTR szAttributeList[] =
    {
        AD_LDAP_DN_TAG,
        NULL
    };
    LDAPMessage* pMessage = NULL;
    HANDLE hDirectory = NULL;

    dwError = LsaDmLdapDirectorySearch(
                    pConn,
                    pszDN,
                    LDAP_SCOPE_ONELEVEL,
                    "(objectClass=*)",
                    szAttributeList,
                    &hDirectory,
                    &pMessage);
    BAIL_ON_LSA_ERROR(dwError);

    *pbValidDN = TRUE;

cleanup:

    if (pMessage)
    {
        ldap_msgfree(pMessage);
    }

    return dwError;

error:

    if (dwError == LDAP_NO_SUCH_OBJECT)
    {
        dwError = 0;
    }

    *pbValidDN = FALSE;

    goto cleanup;
}

DWORD
ADLdap_GetObjectSid(
    HANDLE hDirectory,
    LDAPMessage* pMessage,
    PSTR* ppszSid
    )
{
    DWORD dwError = 0;
    UCHAR* pucSIDBytes = NULL;
    DWORD dwSIDByteLength = 0;
    PSTR pszSid = NULL;

    BAIL_ON_INVALID_POINTER(pMessage);

    if (hDirectory == (HANDLE)NULL)
    {
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaLdapGetBytes(
                hDirectory,
                pMessage,
                AD_LDAP_OBJECTSID_TAG,
                &pucSIDBytes,
                &dwSIDByteLength);
    BAIL_ON_LSA_ERROR(dwError);
    BAIL_ON_INVALID_POINTER(pucSIDBytes);

    dwError = LsaSidBytesToString(
                pucSIDBytes,
                dwSIDByteLength,
                &pszSid);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszSid = pszSid;

cleanup:
    LSA_SAFE_FREE_MEMORY(pucSIDBytes);

    return dwError;

error:
    LSA_SAFE_FREE_STRING(pszSid);
    *ppszSid = NULL;

    goto cleanup;
}

DWORD
ADLdap_GetAccountType(
    IN HANDLE hDirectory,
    IN LDAPMessage* pMessage,
    OUT ADAccountType* pAccountType
    )
{
    DWORD dwError = 0;
    ADAccountType accountType = AccountType_NotFound;
    PSTR* ppszValues = NULL;
    DWORD dwNumValues = 0;
    DWORD iValue = 0;

    // Determine whether this object is user or group.

    dwError = LsaLdapGetStrings(
                    hDirectory,
                    pMessage,
                    AD_LDAP_OBJECTCLASS_TAG,
                    &ppszValues,
                    &dwNumValues);
    BAIL_ON_LSA_ERROR(dwError);

    for (iValue = 0; iValue < dwNumValues; iValue++)
    {
        if (!strncasecmp(ppszValues[iValue], "user", sizeof("user")-1))
        {
            accountType = AccountType_User;
            break;
        }
        else if (!strncasecmp(ppszValues[iValue], "group", sizeof("group")-1))
        {
            accountType = AccountType_Group;
            break;
        }
    }

cleanup:
    LsaFreeStringArray(ppszValues, dwNumValues);

    *pAccountType = accountType;

    return dwError;

error:
    goto cleanup;
}
