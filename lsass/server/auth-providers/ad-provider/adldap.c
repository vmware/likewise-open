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
    PSTR  pszSeparator = "\\";
    DWORD dwLen = 0;

    dwLen = strlen(pszNetBIOSDomainName) + strlen(pszSeparator) + strlen(pszName) + 1;
    dwError = LsaAllocateMemory(dwLen * sizeof(CHAR),
                                (PVOID*)&pszQualifiedName);
    BAIL_ON_LSA_ERROR(dwError);

    sprintf(pszQualifiedName, "%s%s%s", pszNetBIOSDomainName, pszSeparator, pszName);

    LsaStrnToUpper(pszQualifiedName, strlen(pszNetBIOSDomainName));

    LsaStrToLower(pszQualifiedName + strlen(pszNetBIOSDomainName) + strlen(pszSeparator));

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
    IN PSTR pszDnsDomainName,
    IN PSTR pszSamaccountName,
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
ADGetGroupMembers(
    HANDLE hDirectory,
    LDAPMessage* pMessage,
    PCSTR  pszNetBIOSDomainName,
    PCSTR  pszFullDomainName,
    PSTR** pppszValues,
    PDWORD pdwNumValues
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    LDAP *pLd = NULL;
    PSTR pszLDAPUserDN = NULL;
    PSTR *ppszLDAPValues = NULL;
    PSTR *ppszValues = NULL;
    DWORD dwNumValues = 0;
    int   iIndex = 0;
    PSTR pszDirectoryRoot = NULL;
    PSTR szAttributeListName[] =
         {AD_LDAP_SAM_NAME_TAG,
          NULL
         };
    LDAPMessage *pMessageReal = NULL;
    LDAPMessage *pMessageRealUser = NULL;
    PSTR pszSamAccountName = NULL;
    DWORD dwCount = 0;

    pLd = LsaLdapGetSession(hDirectory);

    dwError = LsaLdapConvertDomainToDN(
                    pszFullDomainName,
                    &pszDirectoryRoot);
    BAIL_ON_LSA_ERROR(dwError);

    ppszLDAPValues = ldap_get_values(pLd, pMessage, AD_LDAP_MEMBER_TAG);
    if (ppszLDAPValues) {
        dwNumValues = ldap_count_values(ppszLDAPValues);
        if (dwNumValues < 0)
        {

            dwError = LSA_ERROR_LDAP_ERROR;
            BAIL_ON_LSA_ERROR(dwError);

        }
        else if (dwNumValues > 0)
        {
            DWORD dwNumUsersProcessed = 0;
            DWORD dwNumUsersProcessedPrev = -1;
            PSTR pszQuery = NULL;

            dwError = LsaAllocateMemory((dwNumValues+1) * sizeof(PSTR),
                                        (PVOID*)&ppszValues);
            BAIL_ON_LSA_ERROR(dwError);

            while(dwNumUsersProcessed < dwNumValues)
            {
                dwNumUsersProcessedPrev = dwNumUsersProcessed;
                dwError = ADBuildGetUsersByDNQuery(
                            ppszLDAPValues,
                            dwNumValues,
                            &dwNumUsersProcessed,
                            &pszQuery
                            );
                BAIL_ON_LSA_ERROR(dwError);

                dwError = LsaLdapDirectorySearch(
                                      hDirectory,
                                      pszDirectoryRoot,
                                      LDAP_SCOPE_SUBTREE,
                                      pszQuery,
                                      szAttributeListName,
                                      &pMessageReal);
                BAIL_ON_LSA_ERROR(dwError);

                dwCount = ldap_count_entries(
                                      pLd,
                                      pMessageReal);
                if (dwCount < 0)
                {
                    dwError = LSA_ERROR_LDAP_ERROR;
                    BAIL_ON_LSA_ERROR(dwError);
                }
                else if (dwCount == 0)
                {
                    dwError = LSA_ERROR_NO_SUCH_USER_OR_GROUP;
                    BAIL_ON_LSA_ERROR(dwError);
                }
                else if (dwCount > dwNumUsersProcessed - dwNumUsersProcessedPrev)
                {
                    LSA_LOG_ERROR("Query found %d users when it was expecting %d. Full query: %s",
                        dwCount,
                        dwNumUsersProcessed - dwNumUsersProcessedPrev,
                        pszQuery
                        );
                    dwError = LSA_ERROR_LDAP_ERROR;
                    BAIL_ON_LSA_ERROR(dwError);

                }
                else if (dwCount < dwNumUsersProcessed - dwNumUsersProcessedPrev)
                {
                    LSA_LOG_ERROR("Query found %d users when it was expecting %d. Full query: %s",
                        dwCount,
                        dwNumUsersProcessed - dwNumUsersProcessedPrev,
                        pszQuery
                        );
                }

                pMessageRealUser = pMessageReal;

                while(pMessageRealUser)
                {
                    dwError = LsaLdapGetString(
                                hDirectory,
                                pMessageRealUser,
                                AD_LDAP_SAM_NAME_TAG,
                                &pszSamAccountName);
                    BAIL_ON_LSA_ERROR(dwError);

                    if (!IsNullOrEmptyString(pszSamAccountName)){

                        dwError = ADGetDomainQualifiedString(
                                    pszNetBIOSDomainName,
                                    pszSamAccountName,
                                    &ppszValues[iIndex]);
                        BAIL_ON_LSA_ERROR(dwError);

                        LsaStrCharReplace(ppszValues[iIndex], ' ', AD_GetSeparator());

                        iIndex++;
                    }
                    else
                    {
                        dwError = LsaLdapGetDN(
                                        hDirectory,
                                        pMessageRealUser,
                                        &pszLDAPUserDN);
                        BAIL_ON_LSA_ERROR(dwError);

                        if (IsNullOrEmptyString(pszLDAPUserDN))
                        {
                            dwError = LSA_ERROR_LDAP_FAILED_GETDN;
                            BAIL_ON_LSA_ERROR(dwError);
                        }

                        LSA_LOG_VERBOSE("User object %s lacks a samAccountName.",
                                             pszLDAPUserDN);

                        LSA_SAFE_FREE_STRING(pszLDAPUserDN);
                    }

                    pMessageRealUser = ldap_next_entry(pLd, pMessageRealUser);
                    LSA_SAFE_FREE_STRING(pszSamAccountName);
                }

                if (pMessageReal)
                {
                    ldap_msgfree(pMessageReal);
                    pMessageReal = NULL;
                }

                LSA_SAFE_FREE_STRING(pszQuery);
            }
        }
    }

    dwNumValues = iIndex;

    *pppszValues = ppszValues;
    *pdwNumValues = dwNumValues;

cleanup:

    if (ppszLDAPValues) {
        ldap_value_free(ppszLDAPValues);
        ppszLDAPValues = NULL;
    }

    if (pszLDAPUserDN) {
        ber_memfree(pszLDAPUserDN);
        pszLDAPUserDN = NULL;
    }

    LSA_SAFE_FREE_STRING(pszSamAccountName);
    LSA_SAFE_FREE_STRING(pszDirectoryRoot);

    if (pMessageReal)
    {
        ldap_msgfree(pMessageReal);
    }

    return dwError;

error:

    if (ppszValues) {
        LsaFreeStringArray(ppszValues, dwNumValues);
    }

    *pppszValues = NULL;
    *pdwNumValues = 0;

    goto cleanup;
}


DWORD
ADBuildGetUsersByDNQuery(
    PSTR*  ppszUserDNList,
    DWORD  dwNumUsersFound,
    PDWORD pdwNumUsersProcessed,
    PSTR*  ppszQuery
    )
{
    DWORD dwError = 0;
    PSTR  pszQuery = NULL;
    DWORD dwQueryLength = 0;
    INT   iQueryIndex = 0;
    INT   iUserDNIndex = 0;
    DWORD dwNumUsersProcessed = *pdwNumUsersProcessed;
    PCSTR pszPrefix =
        "(|";
    PCSTR pszSuffix = ")";
    PCSTR pszUserDNSearchConditionFormat = "(distinguishedName=%s)";
    DWORD dwUserDNSearchConditionBaseLength =
               strlen(pszUserDNSearchConditionFormat) - 2;
    PSTR pszEscapedUserDN = NULL;

    dwQueryLength = strlen(pszPrefix) + strlen(pszSuffix);

    iUserDNIndex = dwNumUsersProcessed;

    while (dwNumUsersProcessed < dwNumUsersFound)
    {
        DWORD dwUserDNStatementLength = 0;

        BAIL_ON_INVALID_STRING(ppszUserDNList[dwNumUsersProcessed]);

        LSA_SAFE_FREE_STRING(pszEscapedUserDN);
        dwError = LsaLdapEscapeString(
                    &pszEscapedUserDN,
                    ppszUserDNList[dwNumUsersProcessed]);
        BAIL_ON_LSA_ERROR(dwError);

        dwUserDNStatementLength = dwUserDNSearchConditionBaseLength +
                                  strlen(pszEscapedUserDN);

        if ((dwQueryLength + dwUserDNStatementLength) < MAX_LDAP_QUERY_LENGTH)
        {
            dwQueryLength += dwUserDNStatementLength;
            dwNumUsersProcessed++;
        }
        else
        {
            break;
        }
    }

    dwError = LsaAllocateMemory(
                (dwQueryLength+1)*sizeof(CHAR),
                (PVOID*)&pszQuery);
    BAIL_ON_LSA_ERROR(dwError);

    strcpy(pszQuery, pszPrefix);
    iQueryIndex += strlen(pszPrefix);

    for(; iUserDNIndex < dwNumUsersProcessed; iUserDNIndex++)
    {
        LSA_SAFE_FREE_STRING(pszEscapedUserDN);
        dwError = LsaLdapEscapeString(
                    &pszEscapedUserDN,
                    ppszUserDNList[iUserDNIndex]);
        BAIL_ON_LSA_ERROR(dwError);

        sprintf(pszQuery+iQueryIndex,
                pszUserDNSearchConditionFormat,
                pszEscapedUserDN);
        iQueryIndex += dwUserDNSearchConditionBaseLength;
        iQueryIndex += strlen(pszEscapedUserDN);
    }

    strcpy(pszQuery+iQueryIndex, pszSuffix);
    iQueryIndex += strlen(pszSuffix);

    assert(strlen(pszQuery) == dwQueryLength);

    *ppszQuery = pszQuery;
    *pdwNumUsersProcessed = dwNumUsersProcessed;

cleanup:

    LSA_SAFE_FREE_STRING(pszEscapedUserDN);
    return dwError;

error:

    LSA_SAFE_FREE_STRING(pszQuery);
    *ppszQuery = NULL;

    goto cleanup;
}

/*
 * search for those pseudo groups whose associated real groups
 * have the given user referenced by pszUserDN as a member
 */
DWORD
ADGetUserPseudoGroupMembership(
    HANDLE                   hDirectory,
    DWORD                    dwDirectoryMode,
    ADConfigurationMode      adConfMode,
    PCSTR                    pszCellDN,
    PCSTR                    pszNetBIOSDomainName,
    PSTR                     pszUserDN,
    PLSA_SECURITY_IDENTIFIER pUserSID,
    int                      *piPrimaryGroupIndex,
    PDWORD                   pdwGroupsFound,
    PAD_SECURITY_OBJECT**    pppGroupInfoList
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    LDAP *pLd = NULL;
    PSTR pszDirectoryRoot = NULL;
    PAD_SECURITY_OBJECT* ppPseudoGroupInfoList = NULL;
    PSTR szAttributeListGroups[] =
    {
        AD_LDAP_SAM_NAME_TAG,
        AD_LDAP_GID_TAG,
        AD_LDAP_OBJECTSID_TAG,
        AD_LDAP_PASSWD_TAG,
        AD_LDAP_MEMBER_TAG,
        AD_LDAP_KEYWORDS_TAG,
        NULL
    };
    PSTR pszQuery1 = NULL;
    LDAPMessage *pMessageReal = NULL;
    LDAPMessage *pMessagePseudo = NULL;
    LDAPMessage *pGroupMessageReal = NULL;
    LDAPMessage *pGroupMessagePseudo = NULL;
    DWORD dwCount = 0;
    DWORD dwPseudoGroupsFound = 0;
    DWORD dwRealGroupsFound = 0;
    UCHAR* pucSIDBytes = NULL;
    DWORD dwSIDByteLength = 0;
    PLSA_SECURITY_IDENTIFIER pSID = NULL;
    PLSA_SECURITY_IDENTIFIER pDomainSID = NULL;
    PSTR pszDomainSID = NULL;
    PSTR pszPrimaryRealGroupSID = NULL;
    PSTR* ppszRealSIDList = NULL;
    DWORD dwRealSIDsProcessed = 0;
    PSTR* ppszKeywordList = NULL;
    DWORD dwNumKeywords = 0;
    PSTR pszDomain = NULL;
    PSTR pszObjectDN = NULL;
    PSTR pszQuery = NULL;
    INT iPseudoGroupIndex = 0;
    INT iRealGroupIndex = 0;
    INT iKeywordIndex = 0;
    INT iPrimaryGroupIndex = -1;
    PSTR pszRealDirectoryRoot = NULL;
    HANDLE hRealDirectory = (HANDLE)NULL;
    LDAP *pRealLd = NULL;
    PSTR pszRealDomainName = NULL;
    PSTR pszEscapedUserDN = NULL;

    dwError = LsaLdapConvertDNToDomain(
                  pszUserDN,
                  &pszRealDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLdapEscapeString(
                &pszEscapedUserDN,
                pszUserDN);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaDmWrapLdapOpenDirectoryGc(pszRealDomainName,
                                           &hRealDirectory);
    BAIL_ON_LSA_ERROR(dwError);

    pRealLd = LsaLdapGetSession(hRealDirectory);

    if (adConfMode == SchemaMode &&
        gpADProviderData->dwDirectoryMode == DEFAULT_MODE)
    {
       LSA_LOG_ERROR("Illegal call to method in Schema/Default Cell Mode");
       dwError = LSA_ERROR_INTERNAL;
       BAIL_ON_LSA_ERROR(dwError);
    }

    pLd = LsaLdapGetSession(hDirectory);

    pszDomain = gpADProviderData->szDomain;

    dwError = LsaLdapConvertDomainToDN(pszDomain, &pszDirectoryRoot);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLdapConvertDomainToDN(pszRealDomainName, &pszRealDirectoryRoot);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADGetUserPrimaryGroupSID(
                hRealDirectory,
                pRealLd,
                pszRealDirectoryRoot,
                pszUserDN,
                pUserSID,
                &pszPrimaryRealGroupSID);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAllocateStringPrintf(
                &pszObjectDN,
                "CN=Groups,%s",
                pszCellDN);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAllocateStringPrintf(
                &pszQuery1,
                "(&(objectClass=group)(|(member=%s)(objectSid=%s)))",
                pszEscapedUserDN,
                pszPrimaryRealGroupSID);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLdapDirectorySearch(
                    hRealDirectory,
                    pszRealDirectoryRoot,
                    LDAP_SCOPE_SUBTREE,
                    pszQuery1,
                    szAttributeListGroups,
                    &pMessageReal);
    BAIL_ON_LSA_ERROR(dwError);

    LSA_SAFE_FREE_STRING(pszQuery1);

    dwCount = ldap_count_entries(
                      pRealLd,
                      pMessageReal);
    if (dwCount < 0)
    {
        dwError = LSA_ERROR_LDAP_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwRealGroupsFound = dwCount;

    if (dwRealGroupsFound == 0)
    {
        goto cleanup;
    }

    dwError = LsaAllocateMemory(
                dwRealGroupsFound * sizeof(PSTR),
                (PVOID*)&ppszRealSIDList);
    BAIL_ON_LSA_ERROR(dwError);

    iRealGroupIndex = 0;
    pGroupMessageReal = pMessageReal;
    while (pGroupMessageReal)
    {
        dwError = LsaLdapGetBytes(
                    hRealDirectory,
                    pGroupMessageReal,
                    AD_LDAP_OBJECTSID_TAG,
                    &pucSIDBytes,
                    &dwSIDByteLength);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaAllocSecurityIdentifierFromBinary(
                    (UCHAR*)pucSIDBytes,
                    dwSIDByteLength,
                    &pSID);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaGetSecurityIdentifierString(
                    pSID,
                    &ppszRealSIDList[iRealGroupIndex++]);
        BAIL_ON_LSA_ERROR(dwError);

        if (pucSIDBytes) {
           LsaFreeMemory(pucSIDBytes);
           pucSIDBytes = NULL;
           dwSIDByteLength = 0;
        }

        if (pSID)
        {
            LsaFreeSecurityIdentifier(pSID);
            pSID = NULL;
        }

        pGroupMessageReal = ldap_next_entry(pRealLd, pGroupMessageReal);
    }

    /* the number of pseudo groups is unknown, but it will be
     * less than or equal to the number of real groups */
    dwError = LsaAllocateMemory(
                    sizeof(PAD_SECURITY_OBJECT) * dwRealGroupsFound,
                    (PVOID*)&ppPseudoGroupInfoList);
    BAIL_ON_LSA_ERROR(dwError);

    iPseudoGroupIndex = 0;
    while (dwRealSIDsProcessed < dwRealGroupsFound)
    {
        dwError = ADBuildGetPseudoGroupsBySIDQuery(
                    (PCSTR*)ppszRealSIDList,
                    dwRealGroupsFound,
                    &dwRealSIDsProcessed,
                    &pszQuery);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaLdapDirectorySearch(
                    hDirectory,
                    pszObjectDN,
                    LDAP_SCOPE_ONELEVEL,
                    pszQuery,
                    szAttributeListGroups,
                    &pMessagePseudo);
        BAIL_ON_LSA_ERROR(dwError);

        LSA_SAFE_FREE_STRING(pszQuery);

        dwCount = ldap_count_entries(
                        pLd,
                        pMessagePseudo);
        if (dwCount < 0) {
            dwError = LSA_ERROR_LDAP_ERROR;
            BAIL_ON_LSA_ERROR(dwError);
        }

        dwPseudoGroupsFound = dwCount;

        pGroupMessagePseudo = ldap_first_entry( pLd, pMessagePseudo );
        while (pGroupMessagePseudo)
        {
            CHAR szKeywordTemp[1024];

            dwError = LsaLdapGetStrings(
                        hDirectory,
                        pGroupMessagePseudo,
                        AD_LDAP_KEYWORDS_TAG,
                        &ppszKeywordList,
                        &dwNumKeywords);
            BAIL_ON_LSA_ERROR(dwError);

            iRealGroupIndex = 0;
            pGroupMessageReal = ldap_first_entry( pLd, pMessageReal );
            while (pGroupMessageReal)
            {
                for (iKeywordIndex = 0;
                     iKeywordIndex < dwNumKeywords;
                     iKeywordIndex++)
                {
                    sprintf(szKeywordTemp,
                            "backLink=%s",
                            ppszRealSIDList[iRealGroupIndex]);

                    if (!strcasecmp(ppszKeywordList[iKeywordIndex],
                                    szKeywordTemp))
                    {
                        dwError = ADMarshalToGroupCache(
                                           hDirectory,
                                           dwDirectoryMode,
                                           adConfMode,
                                           pszNetBIOSDomainName,
                                           pGroupMessageReal,
                                           pGroupMessagePseudo,
                                           &ppPseudoGroupInfoList[iPseudoGroupIndex]);
                        BAIL_ON_LSA_ERROR(dwError);

                        if ( strncasecmp(
                                 ppPseudoGroupInfoList[iPseudoGroupIndex]->pszObjectSid,
                                 AD_BUILTIN_GROUP_SID_PREFIX,
                                 sizeof(AD_BUILTIN_GROUP_SID_PREFIX)-1) )
                        {
                            if (!strcmp(
                                        ppPseudoGroupInfoList[iPseudoGroupIndex]->
                                        pszObjectSid,
                                        pszPrimaryRealGroupSID))
                            {
                                iPrimaryGroupIndex = iPseudoGroupIndex;
                            }

                            iPseudoGroupIndex++;
                        }
                        else
                        {
                            ADCacheDB_SafeFreeObject(
                                &ppPseudoGroupInfoList[iPseudoGroupIndex]);
                        }

                        break;
                    }
                }

                pGroupMessageReal = ldap_next_entry(pLd, pGroupMessageReal);

                iRealGroupIndex++;
            }

            pGroupMessagePseudo = ldap_next_entry(pLd, pGroupMessagePseudo);

            if (ppszKeywordList)
            {
                LsaFreeStringArray(ppszKeywordList, dwNumKeywords);
                ppszKeywordList = NULL;
            }
        }

        if (pMessagePseudo)
        {
            ldap_msgfree(pMessagePseudo);
            pMessagePseudo = NULL;
        }
    }

    dwPseudoGroupsFound = iPseudoGroupIndex;

    *pppGroupInfoList = ppPseudoGroupInfoList;
    *pdwGroupsFound = dwPseudoGroupsFound;
    *piPrimaryGroupIndex = iPrimaryGroupIndex;

cleanup:

    if (pMessageReal)
    {
        ldap_msgfree(pMessageReal);
    }

    if (pMessagePseudo)
    {
        ldap_msgfree(pMessagePseudo);
    }

    if (hRealDirectory != (HANDLE)NULL) {
        LsaLdapCloseDirectory(hRealDirectory);
    }

    LSA_SAFE_FREE_STRING(pszDirectoryRoot);
    LSA_SAFE_FREE_STRING(pszRealDirectoryRoot);
    LSA_SAFE_FREE_STRING(pszObjectDN);
    LSA_SAFE_FREE_STRING(pszRealDomainName);
    LSA_SAFE_FREE_STRING(pszPrimaryRealGroupSID);
    LSA_SAFE_FREE_STRING(pszEscapedUserDN);

    if (ppszKeywordList)
    {
        LsaFreeStringArray(ppszKeywordList, dwNumKeywords);
    }

    if (ppszRealSIDList)
    {
        LsaFreeStringArray(ppszRealSIDList, dwRealGroupsFound);
    }

    if (pDomainSID)
    {
        LsaFreeSecurityIdentifier(pDomainSID);
    }

    LSA_SAFE_FREE_STRING(pszDomainSID);

    return dwError;

error:

    ADCacheDB_SafeFreeObjectList(dwPseudoGroupsFound, &ppPseudoGroupInfoList);

    *pppGroupInfoList = NULL;
    *pdwGroupsFound = 0;
    *piPrimaryGroupIndex = -1;

    goto cleanup;
}

DWORD
ADBuildGetPseudoGroupsBySIDQuery(
    PCSTR* ppszRealSIDList,
    DWORD  dwRealGroupsFound,
    PDWORD pdwRealSIDsProcessed,
    PSTR*  ppszQuery
    )
{
    DWORD dwError = 0;
    PSTR  pszQuery = NULL;
    DWORD dwQueryLength = 0;
    INT   iQueryIndex = 0;
    INT   iRealSIDIndex = 0;
    BOOLEAN bDone = FALSE;
    DWORD dwRealSIDsProcessed = *pdwRealSIDsProcessed;
    PCSTR pszPrefix =
        "(&(objectClass=serviceConnectionPoint)(keywords=objectClass=centerisLikewiseGroup)(|";
    PCSTR pszSuffix = "))";
    PCSTR pszSIDSearchConditionFormat = "(keywords=backLink=%s)";
    DWORD dwSIDSearchConditionBaseLength =
               strlen(pszSIDSearchConditionFormat) - 2;

    dwQueryLength = strlen(pszPrefix) + strlen(pszSuffix);

    iRealSIDIndex = dwRealSIDsProcessed;

    while (!bDone &&
           dwRealSIDsProcessed < dwRealGroupsFound &&
           !IsNullOrEmptyString(ppszRealSIDList[dwRealSIDsProcessed]))
    {
        DWORD dwSIDStatementLength =
                              dwSIDSearchConditionBaseLength +
                              strlen(ppszRealSIDList[dwRealSIDsProcessed]);

        if (((dwQueryLength + dwSIDStatementLength) < MAX_LDAP_QUERY_LENGTH) &&
            (dwRealSIDsProcessed < dwRealGroupsFound))
        {
            dwQueryLength += dwSIDStatementLength;
            dwRealSIDsProcessed++;
        }
        else
        {
            bDone = TRUE;
        }
    }

    dwError = LsaAllocateMemory(
                (dwQueryLength+1)*sizeof(CHAR),
                (PVOID*)&pszQuery);
    BAIL_ON_LSA_ERROR(dwError);

    strcpy(pszQuery, pszPrefix);
    iQueryIndex += strlen(pszPrefix);

    for(; iRealSIDIndex < dwRealSIDsProcessed; iRealSIDIndex++)
    {
        sprintf(pszQuery+iQueryIndex,
                pszSIDSearchConditionFormat,
                ppszRealSIDList[iRealSIDIndex]);
        iQueryIndex += dwSIDSearchConditionBaseLength;
        iQueryIndex += strlen(ppszRealSIDList[iRealSIDIndex]);
    }

    strcpy(pszQuery+iQueryIndex, pszSuffix);
    iQueryIndex += strlen(pszSuffix);

    if (strlen(pszQuery) > MAX_LDAP_QUERY_LENGTH)
    {
        dwError = LSA_ERROR_INTERNAL;
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppszQuery = pszQuery;
    *pdwRealSIDsProcessed = dwRealSIDsProcessed;

cleanup:

    return dwError;

error:

    LSA_SAFE_FREE_STRING(pszQuery);
    *ppszQuery = NULL;

    goto cleanup;
}

DWORD
ADGetUserPrimaryGroupSID(
    HANDLE hDirectory,
    LDAP*  pLd,
    PCSTR  pszDirectoryRoot,
    PCSTR  pszUserDN,
    PLSA_SECURITY_IDENTIFIER pUserSID,
    PSTR*  ppszPrimaryGroupSID
    )
{
    DWORD dwError = 0;
    PSTR pszPrimaryGroupSID = NULL;
    DWORD dwCount = 0;
    DWORD dwUserPrimaryGroupID = 0;
    LDAPMessage *pMessage = NULL;
    PSTR szAttributeListUserPrimeGID[] =
    {
        AD_LDAP_PRIMEGID_TAG,
        NULL
    };

    //find the user's primary group ID.
    dwError = LsaLdapDirectorySearch(
                    hDirectory,
                    pszUserDN,
                    LDAP_SCOPE_BASE,
                    "objectClass=*",
                    szAttributeListUserPrimeGID,
                    &pMessage);
    BAIL_ON_LSA_ERROR(dwError);

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

     //find the primary group's SID.
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

    if (pMessage)
    {
        ldap_msgfree(pMessage);
    }

    return dwError;

error:

    LSA_SAFE_FREE_STRING(pszPrimaryGroupSID);

    *ppszPrimaryGroupSID = NULL;

    goto cleanup;

}

DWORD
ADFindComputerDN(
    HANDLE hDirectory,
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

    pLd = LsaLdapGetSession(hDirectory);

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

    dwError = LsaLdapDirectorySearch(
                    hDirectory,
                    pszDirectoryRoot,
                    LDAP_SCOPE_SUBTREE,
                    pszQuery,
                    szAttributeList,
                    &pMessage);
    BAIL_ON_LSA_ERROR(dwError);

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
    HANDLE hDirectory,
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

    pLd = LsaLdapGetSession(hDirectory);

    dwError = LsaLdapDirectorySearch(
                    hDirectory,
                    pszDN,
                    LDAP_SCOPE_ONELEVEL,
                    "(name=$LikewiseIdentityCell)",
                    szAttributeList,
                    &pMessage);
    BAIL_ON_LSA_ERROR(dwError);

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
    HANDLE hDirectory,
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
    PSTR pszMaxPwdAge = NULL;
    PSTR pszDirectoryRoot = NULL;
    UINT64 MaxPwdAge = 0;

    pLd = LsaLdapGetSession(hDirectory);

    dwError = LsaLdapConvertDomainToDN(
                    pszDomainName,
                    &pszDirectoryRoot);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLdapDirectorySearch(
                    hDirectory,
                    pszDirectoryRoot,
                    LDAP_SCOPE_BASE,
                    "(objectClass=*)",
                    szAttributeList,
                    &pMessage);
    BAIL_ON_LSA_ERROR(dwError);

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
    dwError = LsaLdapGetString(
                hDirectory,
                pMessage,
                AD_LDAP_MAX_PWDAGE_TAG,
                &pszMaxPwdAge);
    BAIL_ON_LSA_ERROR(dwError);

    if (pszMaxPwdAge){
        if (pszMaxPwdAge[0] != '-'){
            dwError = ADStr2UINT64(
                        pszMaxPwdAge,
                        &MaxPwdAge);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    *pMaxPwdAge = MaxPwdAge;

cleanup:

    LSA_SAFE_FREE_STRING(pszMaxPwdAge);
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
    HANDLE hDirectory,
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

    PSTR* ppszValues = NULL;
    DWORD dwNumValues = 0;
    DWORD i = 0;

    pLd = LsaLdapGetSession(hDirectory);

    dwError = LsaLdapDirectorySearch(
                    hDirectory,
                    pszDN,
                    LDAP_SCOPE_BASE,
                    "(objectClass=*)",
                    szAttributeList,
                    &pMessage);
    if (dwError == LDAP_NO_SUCH_OBJECT){
        dwError = LSA_ERROR_INCOMPATIBLE_MODES_BETWEEN_TRUSTEDDOMAINS;
    }
    BAIL_ON_LSA_ERROR(dwError);

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
            AD_LDAP_OBJECTSID_TAG,
            AD_LDAP_ALIAS_TAG,
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
ADUnprovisionalModeGetSid(
    HANDLE  hDirectory,
    DWORD   dwUID, //this is assumed to be a hashed UID.
    PSTR*   ppszObjectSID
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    LDAP *pLd = NULL;
    PSTR pszDirectoryRoot = NULL;
    CHAR szQuery[1024];
    PCSTR pszDomain = NULL;
    LDAPMessage *pMessage = NULL;
    DWORD dwCount = 0;
    PSTR pszDomainSID = NULL;
    PSTR pszObjectSID = NULL;
    PBYTE pDomainSIDBytes = NULL;
    DWORD dwDomainSIDByteLen = 0;
    PLSA_SECURITY_IDENTIFIER pDomainSID = NULL;
    PSTR szRealAttributeList[] =
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


    pLd = LsaLdapGetSession(hDirectory);

    pszDomain = gpADProviderData->szDomain;
    dwError = LsaLdapConvertDomainToDN(pszDomain, &pszDirectoryRoot);
    BAIL_ON_LSA_ERROR(dwError);

    //find domain SID
    sprintf(szQuery, "(objectClass=domain)");

    dwError = LsaLdapDirectorySearch(
                    hDirectory,
                    pszDirectoryRoot,
                    LDAP_SCOPE_BASE,
                    szQuery,
                    szRealAttributeList,
                    &pMessage);
    BAIL_ON_LSA_ERROR(dwError);

    dwCount = ldap_count_entries(
                    pLd,
                    pMessage);
    if (dwCount != 1) {
        dwError = LSA_ERROR_LDAP_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaLdapGetBytes(
                hDirectory,
                pMessage,
                AD_LDAP_OBJECTSID_TAG,
                &pDomainSIDBytes,
                &dwDomainSIDByteLen);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAllocSecurityIdentifierFromBinary(
                    pDomainSIDBytes,
                    dwDomainSIDByteLen,
                    &pDomainSID);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaGetSecurityIdentifierString(
                    pDomainSID,
                    &pszDomainSID);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = UnprovisionedModeMakeLocalSID(
                    pszDomainSID,
                    dwUID,
                    &pszObjectSID);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszObjectSID = pszObjectSID;

cleanup:
    if (pDomainSID)
    {
        LsaFreeSecurityIdentifier(pDomainSID);
    }

    LSA_SAFE_FREE_MEMORY(pDomainSIDBytes);

    LSA_SAFE_FREE_STRING(pszDirectoryRoot);
    LSA_SAFE_FREE_STRING(pszDomainSID);

    if (pMessage) {
        ldap_msgfree(pMessage);
        pMessage = NULL;
    }

    return dwError;

error:

    *ppszObjectSID = NULL;

    LSA_SAFE_FREE_STRING(pszObjectSID);
    goto cleanup;
}

DWORD
ADFindUserByNameNonAlias(
    HANDLE  hPseudoDirectory,
    HANDLE  hRealDirectory,
    PCSTR   pszCellDN,
    DWORD   dwDirectoryMode,
    ADConfigurationMode adConfMode,
    PLSA_LOGIN_NAME_INFO pUserNameInfo,
    PAD_SECURITY_OBJECT *ppUserInfo)
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    PAD_SECURITY_OBJECT pUserInfo = NULL;
    PDLINKEDLIST curr = NULL;

    dwError = ADFindUserByNameNonAliasHelper(
                  hPseudoDirectory,
                  hRealDirectory,
                  pszCellDN,
                  dwDirectoryMode,
                  adConfMode,
                  pUserNameInfo,
                  &pUserInfo);
    if ((dwError == LSA_ERROR_NO_SUCH_USER || dwError == LSA_ERROR_NO_SUCH_USER_OR_GROUP)
         && gpADProviderData->pCellList)
    {
        dwError = LSA_ERROR_SUCCESS;
    }
    BAIL_ON_LSA_ERROR(dwError);
    
    if (pUserInfo)
        goto done;

    curr = gpADProviderData->pCellList;

    while (curr && curr->pItem){
        dwError = ADFindUserByNameNonAliasHelper(
                           hPseudoDirectory,
                           hRealDirectory,
                           ((PAD_LINKED_CELL_INFO)curr->pItem)->pszCellDN,
                           dwDirectoryMode,
                           adConfMode,
                           pUserNameInfo,
                           &pUserInfo);
        if ((dwError == LSA_ERROR_NO_SUCH_USER || dwError == LSA_ERROR_NO_SUCH_USER_OR_GROUP)
             && curr->pNext)
        {
           dwError = LSA_ERROR_SUCCESS;
        }
        BAIL_ON_LSA_ERROR(dwError);
        
        if (pUserInfo)
            break;
        
        curr = curr->pNext;
    }

done:

     *ppUserInfo = pUserInfo;

cleanup:

    return dwError;

error:

    *ppUserInfo = NULL;

    ADCacheDB_SafeFreeObject(&pUserInfo);

    goto cleanup;
}

DWORD
ADFindUserByNameNonAliasHelper(
    HANDLE  hPseudoDirectory,
    HANDLE  hRealDirectory,
    PCSTR   pszCellDN,
    DWORD   dwDirectoryMode,
    ADConfigurationMode adConfMode,
    PLSA_LOGIN_NAME_INFO pUserNameInfo,
    PAD_SECURITY_OBJECT *ppUserInfo)
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    LDAP *pPseudoLd = NULL;
    LDAP *pRealLd = NULL;
    PSTR pszRealDirectoryRoot = NULL;
    PSTR* ppszRealAttributeList = NULL;
    PSTR* ppszPseudoAttributeList = NULL;
    PSTR pszQuery1 = NULL;
    PSTR pszQuery2 = NULL;
    PSTR pszUsersContainerDN = NULL;
    PSTR pszUPN = NULL;
    LDAPMessage* pMessageReal = NULL;
    LDAPMessage* pMessagePseudo = NULL;
    DWORD dwCount = 0;
    PBYTE pObjectSIDBytes = NULL;
    PLSA_SECURITY_IDENTIFIER pObjectSID = NULL;
    PSTR pszObjectSID = NULL;
    PAD_SECURITY_OBJECT pUserInfo = NULL;
    BOOLEAN bValidADEntry = false;

    if (!pUserNameInfo){
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = ADGetUserRealAttributeList(
                   dwDirectoryMode,
                   adConfMode,
                   &ppszRealAttributeList);
    BAIL_ON_LSA_ERROR(dwError);

    if (hPseudoDirectory != (HANDLE)NULL)
        pPseudoLd = LsaLdapGetSession(hPseudoDirectory);

    dwError = LsaLdapConvertDomainToDN(pUserNameInfo->pszFullDomainName, &pszRealDirectoryRoot);
    BAIL_ON_LSA_ERROR(dwError);

    BAIL_ON_INVALID_STRING(pUserNameInfo->pszObjectSid);

    switch (pUserNameInfo->nameType)
    {     
        case NameType_NT4:
        case NameType_UPN:
            if (dwDirectoryMode == DEFAULT_MODE && adConfMode == SchemaMode)
            {
                    dwError = LsaAllocateStringPrintf(
                                   &pszQuery1,
                                   "(&(objectSid=%s)(objectClass=User)(!(objectClass=computer))(uidNumber=*))",
                                   pUserNameInfo->pszObjectSid);
            }
            else
            {
                    dwError = LsaAllocateStringPrintf(
                                   &pszQuery1,
                                   "(&(objectSid=%s)(objectClass=User)(!(objectClass=computer)))",
                                   pUserNameInfo->pszObjectSid);
            }

            break;

         default:

            dwError = LSA_ERROR_INVALID_PARAMETER;
    }

    BAIL_ON_LSA_ERROR(dwError);


    //(1) Get pMessageReal
    //    (In one-way trust in CELL_MODE scenario, hRealDirectory is NULL so does pMessageReal)
    if (hRealDirectory != (HANDLE)NULL){
        pRealLd = LsaLdapGetSession(hRealDirectory);

        dwError = LsaLdapDirectorySearch(
                   hRealDirectory,
                   pszRealDirectoryRoot,
                   LDAP_SCOPE_SUBTREE,
                   pszQuery1,
                   ppszRealAttributeList,
                   &pMessageReal);

        BAIL_ON_LSA_ERROR(dwError);

        dwCount = ldap_count_entries(
                       pRealLd,
                       pMessageReal
                       );
        if (dwCount < 0) {
               dwError = LSA_ERROR_LDAP_ERROR;
        } else if (dwCount == 0) {
               dwError = LSA_ERROR_NO_SUCH_USER;
        } else if (dwCount > 1) {
               dwError = LSA_ERROR_DUPLICATE_USERNAME;
        }
        BAIL_ON_LSA_ERROR(dwError);

        //Confirm the entry we obtain from AD is valid by retrieving its DN
        dwError = LsaLdapIsValidADEntry(
                        hRealDirectory,
                        pMessageReal,
                        &bValidADEntry);
        BAIL_ON_LSA_ERROR(dwError);

        if (!bValidADEntry){
            dwError = LSA_ERROR_LDAP_FAILED_GETDN;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    //(2) Get pMessagePseudo (except for UNPROVISIONED_MODE)
    if (dwDirectoryMode == DEFAULT_MODE && adConfMode == SchemaMode){
        pMessagePseudo = pMessageReal;
    }
    else if (dwDirectoryMode != UNPROVISIONED_MODE)
    {
        dwError = ADGetUserPseudoAttributeList(
                         adConfMode,
                         &ppszPseudoAttributeList);
        BAIL_ON_LSA_ERROR(dwError);

        //If we proceed here, we are in DEFAULT or CELL mode, so pszCellDN should not be empty
        if (IsNullOrEmptyString(pszCellDN) || IsNullOrEmptyString(pUserNameInfo->pszObjectSid)){
            dwError = LSA_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
        }

        dwError = LsaAllocateStringPrintf(
                      &pszUsersContainerDN,
                      "CN=Users,%s",
                      pszCellDN);
       BAIL_ON_LSA_ERROR(dwError);

        switch (adConfMode)
        {
            case SchemaMode:
                dwError = LsaAllocateStringPrintf(
                              &pszQuery2,
                              "(&(objectClass=posixAccount)(keywords=objectClass=centerisLikewiseUser)(keywords=backLink=%s))",
                              pUserNameInfo->pszObjectSid);

                break;

            case NonSchemaMode:
                dwError = LsaAllocateStringPrintf(
                               &pszQuery2,
                                "(&(objectClass=serviceConnectionPoint)(keywords=objectClass=centerisLikewiseUser)(keywords=backLink=%s))",
                               pUserNameInfo->pszObjectSid);

                break;

            default:
                dwError = LSA_ERROR_INVALID_PARAMETER;
        }
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaLdapDirectorySearch(
                          hPseudoDirectory,
                          pszUsersContainerDN,
                          LDAP_SCOPE_ONELEVEL,
                          pszQuery2,
                          ppszPseudoAttributeList,
                          &pMessagePseudo);

         BAIL_ON_LSA_ERROR(dwError);

         dwCount = ldap_count_entries(
                          pPseudoLd,
                          pMessagePseudo
                          );
         if (dwCount < 0) {
            dwError = LSA_ERROR_LDAP_ERROR;
         } else if (dwCount == 0) {
            dwError = LSA_ERROR_NO_SUCH_USER;
         } else if (dwCount > 1) {
            dwError = LSA_ERROR_DUPLICATE_USERNAME;
         }
         BAIL_ON_LSA_ERROR(dwError);

         //Confirm the entry we obtain from AD is valid by retrieving its DN
         dwError = LsaLdapIsValidADEntry(
                         hPseudoDirectory,
                         pMessagePseudo,
                         &bValidADEntry);
         BAIL_ON_LSA_ERROR(dwError);

         if (!bValidADEntry){
             dwError = LSA_ERROR_LDAP_FAILED_GETDN;
             BAIL_ON_LSA_ERROR(dwError);
         }
    }

    //(3) Marshal result to obtain UserInfo
    dwError = ADMarshalToUserCacheEx(
                    hPseudoDirectory,
                    hRealDirectory,
                    dwDirectoryMode,
                    adConfMode,
                    pUserNameInfo,
                    pMessageReal,
                    pMessagePseudo,
                    &pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    *ppUserInfo = pUserInfo;

cleanup:
    LsaFreeNullTerminatedStringArray(ppszRealAttributeList);
    LsaFreeNullTerminatedStringArray(ppszPseudoAttributeList);

    LSA_SAFE_FREE_STRING(pszRealDirectoryRoot);

    if (pMessageReal) {
        if (pMessageReal == pMessagePseudo)
        {
            pMessagePseudo = NULL;
        }
        ldap_msgfree(pMessageReal);
    }

    if (pMessagePseudo) {
        ldap_msgfree(pMessagePseudo);
    }

    if (pObjectSID) {
        LsaFreeSecurityIdentifier(pObjectSID);
    }

    LSA_SAFE_FREE_MEMORY(pObjectSIDBytes);

    if (pszObjectSID){
        LsaFreeString(pszObjectSID);
    }

    LSA_SAFE_FREE_STRING(pszUPN);
    LSA_SAFE_FREE_STRING(pszQuery1);
    LSA_SAFE_FREE_STRING(pszQuery2);
    LSA_SAFE_FREE_STRING(pszUsersContainerDN);

    return dwError;

error:

    *ppUserInfo = NULL;

    ADCacheDB_SafeFreeObject(&pUserInfo);

    goto cleanup;
}

DWORD
ADFindObjectNameByDN(
    PCSTR pszObjectDN,
    PSTR* ppszNT4Name,
    ADAccountType* pObjectType)
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    HANDLE hDirectory = (HANDLE)NULL;
    LDAP *pLd = NULL;
    PSTR pszNT4Name = NULL;
    PSTR pszNetbiosDomainName = NULL;
    PSTR pszSamaccountName = NULL;
    PSTR pszRealDomainName = NULL;
    PSTR pszEscapedObjectDN = NULL;
    PSTR szRealAttributeList[] =
        {
            AD_LDAP_OBJECTCLASS_TAG,
            AD_LDAP_SAM_NAME_TAG,
            NULL
        };
    PSTR pszQuery = NULL;
    PSTR pszDirectoryRoot = NULL;
    LDAPMessage* pMessageReal = NULL;
    BOOLEAN bValidADEntry = FALSE;
    PSTR* ppszValues = NULL;
    DWORD dwNumValues = 0;
    DWORD dwCount = 0;
    DWORD iValue = 0;
    ADAccountType objectType = 0;
    LSA_TRUST_DIRECTION dwTrustDirection = LSA_TRUST_DIRECTION_UNKNOWN;


    if (!pszObjectDN){
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaLdapEscapeString(
                &pszEscapedObjectDN,
                pszObjectDN);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLdapConvertDNToDomain(
                 pszEscapedObjectDN,
                 &pszRealDomainName);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaDmQueryDomainInfo(
                   pszRealDomainName,
                   NULL,
                   &pszNetbiosDomainName,
                   NULL,
                   NULL,
                   NULL,
                   NULL,
                   NULL,
                   NULL,
                   &dwTrustDirection,
                   NULL,
                   NULL,
                   NULL,
                   NULL,
                   NULL,
                   NULL);
    BAIL_ON_LSA_ERROR(dwError);

    BAIL_ON_INVALID_STRING(pszNetbiosDomainName);

    if (dwTrustDirection != LSA_TRUST_DIRECTION_SELF && 
        dwTrustDirection != LSA_TRUST_DIRECTION_TWO_WAY)
    {
        dwError = LSA_ERROR_NO_SUCH_USER_OR_GROUP;
    }
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaDmWrapLdapOpenDirectoryDomain(pszRealDomainName, &hDirectory);
    BAIL_ON_LSA_ERROR(dwError);

    pLd = LsaLdapGetSession(hDirectory);

    dwError = LsaAllocateStringPrintf(
                &pszQuery,
                "(&(distinguishedName=%s)(|(objectClass=User)(objectClass=Group))(!(objectClass=computer)))",
                pszEscapedObjectDN);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLdapConvertDomainToDN(pszRealDomainName, &pszDirectoryRoot);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLdapDirectorySearch(
               hDirectory,
               pszDirectoryRoot,
               LDAP_SCOPE_SUBTREE,
               pszQuery,
               szRealAttributeList,
               &pMessageReal);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwCount = ldap_count_entries(
                   pLd,
                   pMessageReal
                   );
    if (dwCount < 0) {
           dwError = LSA_ERROR_LDAP_ERROR;
    } else if (dwCount == 0) {
           dwError = LSA_ERROR_NO_SUCH_USER_OR_GROUP;
    } else if (dwCount > 1) {
           dwError = LSA_ERROR_DUPLICATE_USER_OR_GROUP;
    }
    BAIL_ON_LSA_ERROR(dwError);

    //Confirm the entry we obtain from AD is valid by retrieving its DN
    dwError = LsaLdapIsValidADEntry(
                    hDirectory,
                    pMessageReal,
                    &bValidADEntry);
    BAIL_ON_LSA_ERROR(dwError);

    if (!bValidADEntry){
        dwError = LSA_ERROR_LDAP_FAILED_GETDN;
        BAIL_ON_LSA_ERROR(dwError);
    }

    //determine whether this object is user or group
    dwError = LsaLdapGetStrings(
                           hDirectory,
                           pMessageReal,
                           AD_LDAP_OBJECTCLASS_TAG,
                           &ppszValues,
                           &dwNumValues);
    BAIL_ON_LSA_ERROR(dwError);

    for (iValue = 0; iValue < dwNumValues; iValue++)
    {
        if (!strncasecmp(ppszValues[iValue], "user", sizeof("user")-1))
        {
           objectType = AccountType_User;
           break;
        }
        else if (!strncasecmp(ppszValues[iValue], "group", sizeof("group")-1))
        {
            objectType = AccountType_Group;
            break;
        }
    }

    if(objectType != AccountType_User && objectType != AccountType_Group)
    {
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaLdapGetString(hDirectory,
                               pMessageReal,
                               AD_LDAP_SAM_NAME_TAG,
                               &pszSamaccountName);
    BAIL_ON_LSA_ERROR(dwError);
    BAIL_ON_INVALID_STRING(pszSamaccountName);
    
    dwError = LsaAllocateStringPrintf(
                    &pszNT4Name,
                    "%s\\%s",
                    pszNetbiosDomainName,
                    pszSamaccountName);
    BAIL_ON_LSA_ERROR(dwError);
    
    *ppszNT4Name = pszNT4Name;
    *pObjectType = objectType;

cleanup:

    LSA_SAFE_FREE_STRING(pszRealDomainName);
    LSA_SAFE_FREE_STRING(pszEscapedObjectDN);

    if (pMessageReal) {        
        ldap_msgfree(pMessageReal);
    }

    if (ppszValues) {
        LsaFreeStringArray(ppszValues, dwNumValues);
    }
    
    if (hDirectory != (HANDLE)NULL) {
        LsaLdapCloseDirectory(hDirectory);
    }

    LSA_SAFE_FREE_STRING(pszQuery);
    LSA_SAFE_FREE_STRING(pszNetbiosDomainName);
    LSA_SAFE_FREE_STRING(pszSamaccountName);
    LSA_SAFE_FREE_STRING(pszDirectoryRoot);

    return dwError;

error:

    *ppszNT4Name = NULL;

    LSA_SAFE_FREE_STRING(pszNT4Name);

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


DWORD
ADFindGroupByNameNT4(
    HANDLE  hPseudoDirectory,
    HANDLE  hRealDirectory,
    PCSTR   pszCellDN,
    DWORD   dwDirectoryMode,
    ADConfigurationMode adConfMode,
    PLSA_LOGIN_NAME_INFO pGroupNameInfo,
    PAD_SECURITY_OBJECT* ppGroupInfo
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    PAD_SECURITY_OBJECT pGroupInfo = NULL;
    PDLINKEDLIST curr = NULL;

    dwError = ADFindGroupByNameNT4Helper(
                    hPseudoDirectory,
                    hRealDirectory,
                    pszCellDN,
                    dwDirectoryMode,
                    adConfMode,
                    pGroupNameInfo,
                    &pGroupInfo);
    if ((dwError == LSA_ERROR_NO_SUCH_GROUP || dwError == LSA_ERROR_NO_SUCH_USER_OR_GROUP)
         && gpADProviderData->pCellList)
    {
       dwError = LSA_ERROR_SUCCESS;
    }
    BAIL_ON_LSA_ERROR(dwError);
    
    if (pGroupInfo)
        goto done;

    curr = gpADProviderData->pCellList;

    while (curr && curr->pItem)
    {
        dwError = ADFindGroupByNameNT4Helper(
                           hPseudoDirectory,
                           hRealDirectory,
                           ((PAD_LINKED_CELL_INFO)curr->pItem)->pszCellDN,
                           dwDirectoryMode,
                           adConfMode,
                           pGroupNameInfo,
                           &pGroupInfo);
        if ((dwError == LSA_ERROR_NO_SUCH_GROUP || dwError == LSA_ERROR_NO_SUCH_USER_OR_GROUP)
             && curr->pNext)
        {
           dwError = LSA_ERROR_SUCCESS;
        }
        BAIL_ON_LSA_ERROR(dwError);
        
        if (pGroupInfo)
             break;
        
        curr = curr->pNext;        
    }
done:

     *ppGroupInfo = pGroupInfo;

cleanup:

    return dwError;

error:

    *ppGroupInfo = NULL;

    ADCacheDB_SafeFreeObject(&pGroupInfo);

    goto cleanup;
}

DWORD
ADFindGroupByNameNT4Helper(
    HANDLE  hPseudoDirectory,
    HANDLE  hRealDirectory,
    PCSTR   pszCellDN,
    DWORD   dwDirectoryMode,
    ADConfigurationMode adConfMode,
    PLSA_LOGIN_NAME_INFO pGroupNameInfo,
    PAD_SECURITY_OBJECT* ppGroupInfo
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    LDAP* pPseudoLd = NULL;
    LDAP* pRealLd = NULL;
    PSTR pszPseudoDirectoryRoot = NULL;
    PSTR pszRealDirectoryRoot = NULL;
    PSTR* ppszRealAttributeList = NULL;
    PSTR* ppszPseudoAttributeList = NULL;
    PSTR pszQuery = NULL;
    PSTR pszGroupsContainerDN = NULL;
    LDAPMessage* pMessageReal = NULL;
    LDAPMessage* pMessagePseudo = NULL;
    DWORD dwCount = 0;
    PCSTR pszFullDomainName = NULL;
    PAD_SECURITY_OBJECT pGroupInfo = NULL;
    BOOLEAN bValidADEntry = false;
    PSTR pszEscapedGroupName = NULL;

    if (!pGroupNameInfo){
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = ADGetGroupRealAttributeList(
                   dwDirectoryMode,
                   adConfMode,
                   &ppszRealAttributeList);
    BAIL_ON_LSA_ERROR(dwError);

    if (hPseudoDirectory != (HANDLE)NULL)
        pPseudoLd = LsaLdapGetSession(hPseudoDirectory);

    pszFullDomainName = gpADProviderData->szDomain;
    dwError = LsaLdapConvertDomainToDN(pszFullDomainName, &pszPseudoDirectoryRoot);
    BAIL_ON_LSA_ERROR(dwError);
    dwError = LsaLdapConvertDomainToDN(pGroupNameInfo->pszFullDomainName, &pszRealDirectoryRoot);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLdapEscapeString(
                &pszEscapedGroupName,
                pGroupNameInfo->pszName);
    BAIL_ON_LSA_ERROR(dwError);

    switch (pGroupNameInfo->nameType)
    {
        case NameType_NT4:

            dwError = LsaAllocateStringPrintf(
                         &pszQuery,
                         "(&(sAMAccountName=%s)(objectClass=Group))",
                         pszEscapedGroupName);

            break;

        default:
            dwError = LSA_ERROR_INVALID_PARAMETER;
    }
    BAIL_ON_LSA_ERROR(dwError);

    //(1) Get pMessageReal
    if (hRealDirectory != (HANDLE)NULL){
        pRealLd = LsaLdapGetSession(hRealDirectory);

        dwError = LsaLdapDirectorySearch(
                   hRealDirectory,
                   pszRealDirectoryRoot,
                   LDAP_SCOPE_SUBTREE,
                   pszQuery,
                   ppszRealAttributeList,
                   &pMessageReal);
        BAIL_ON_LSA_ERROR(dwError);

        LSA_SAFE_FREE_STRING(pszQuery);

        dwCount = ldap_count_entries(
                       pRealLd,
                       pMessageReal
                       );
        if (dwCount < 0) {
               dwError = LSA_ERROR_LDAP_ERROR;
        } else if (dwCount == 0) {
               dwError = LSA_ERROR_NO_SUCH_GROUP;
        } else if (dwCount > 1) {
               dwError = LSA_ERROR_DUPLICATE_GROUPNAME;
        }
        BAIL_ON_LSA_ERROR(dwError);

        //Confirm the entry we obtain from AD is valid by retrieving its DN
        dwError = LsaLdapIsValidADEntry(
                        hRealDirectory,
                        pMessageReal,
                        &bValidADEntry);
        BAIL_ON_LSA_ERROR(dwError);

        if (!bValidADEntry){
            dwError = LSA_ERROR_LDAP_FAILED_GETDN;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    //(2) Get pMessagePseudo (except for UNPROVISIONED_MODE)
    if (dwDirectoryMode == DEFAULT_MODE && adConfMode == SchemaMode){
        pMessagePseudo = pMessageReal;
    }
    else if (dwDirectoryMode != UNPROVISIONED_MODE){
        dwError = ADGetGroupPseudoAttributeList(
                         adConfMode,
                         &ppszPseudoAttributeList);
        BAIL_ON_LSA_ERROR(dwError);

        //If we proceed here, we are in DEFAULT or CELL mode, so pszCellDN should not be empty
        if (IsNullOrEmptyString(pszCellDN) || IsNullOrEmptyString(pGroupNameInfo->pszObjectSid)){
            dwError = LSA_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
        }

        dwError = LsaAllocateStringPrintf(
                     &pszGroupsContainerDN,
                     "CN=Groups,%s",
                     pszCellDN);
        BAIL_ON_LSA_ERROR(dwError);

        switch (adConfMode)
        {
            case SchemaMode:
                dwError = LsaAllocateStringPrintf(
                             &pszQuery,
                             "(&(objectClass=posixGroup)(keywords=objectClass=centerisLikewiseGroup)(keywords=backLink=%s))",
                             pGroupNameInfo->pszObjectSid);

                break;

            case NonSchemaMode:
                dwError = LsaAllocateStringPrintf(
                             &pszQuery,
                             "(&(objectClass=serviceConnectionPoint)(keywords=objectClass=centerisLikewiseGroup)(keywords=backLink=%s))",
                             pGroupNameInfo->pszObjectSid);

                break;

            default:
                dwError = LSA_ERROR_INVALID_PARAMETER;
        }
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaLdapDirectorySearch(
                          hPseudoDirectory,
                          pszGroupsContainerDN,
                          LDAP_SCOPE_ONELEVEL,
                          pszQuery,
                          ppszPseudoAttributeList,
                          &pMessagePseudo);

         BAIL_ON_LSA_ERROR(dwError);

         dwCount = ldap_count_entries(
                          pPseudoLd,
                          pMessagePseudo
                          );
         if (dwCount < 0) {
            dwError = LSA_ERROR_LDAP_ERROR;
         } else if (dwCount == 0) {
            // This is an unenabled group. Set pMessagePseudo back to NULL
            // so that we return it back that way without an error.
            ldap_msgfree(pMessagePseudo);
            pMessagePseudo = NULL;
         } else if (dwCount > 1) {
            dwError = LSA_ERROR_DUPLICATE_GROUPNAME;
         } else if (dwCount == 1) {
            //Confirm the entry we obtain from AD is valid by retrieving its DN
            dwError = LsaLdapIsValidADEntry(
                            hPseudoDirectory,
                            pMessagePseudo,
                            &bValidADEntry);
            BAIL_ON_LSA_ERROR(dwError);

            if (!bValidADEntry){
                dwError = LSA_ERROR_LDAP_FAILED_GETDN;
                BAIL_ON_LSA_ERROR(dwError);
            }
         }
         BAIL_ON_LSA_ERROR(dwError);
    }

    //(3) Marshal result to obtain GroupInfo
    dwError = ADMarshalToGroupCacheEx(
                       hPseudoDirectory,
                       hRealDirectory,
                       dwDirectoryMode,
                       adConfMode,
                       pGroupNameInfo,
                       pMessageReal,
                       pMessagePseudo,
                       &pGroupInfo);
    BAIL_ON_LSA_ERROR(dwError);

    *ppGroupInfo = pGroupInfo;

cleanup:
    LsaFreeNullTerminatedStringArray(ppszRealAttributeList);
    LsaFreeNullTerminatedStringArray(ppszPseudoAttributeList);

    LSA_SAFE_FREE_STRING(pszEscapedGroupName);

    LSA_SAFE_FREE_STRING(pszPseudoDirectoryRoot);
    LSA_SAFE_FREE_STRING(pszRealDirectoryRoot);

    LSA_SAFE_FREE_STRING(pszQuery);
    LSA_SAFE_FREE_STRING(pszGroupsContainerDN);

    if (pMessageReal) {
        if (pMessageReal == pMessagePseudo)
        {
            pMessagePseudo = NULL;
        }
        ldap_msgfree(pMessageReal);
    }

    if (pMessagePseudo) {
        ldap_msgfree(pMessagePseudo);
    }

    return dwError;

error:

    *ppGroupInfo = NULL;

    ADCacheDB_SafeFreeObject(&pGroupInfo);

    goto cleanup;
}

static
VOID
DestroyDNListEntry(
    IN OUT PLSA_AD_DN_LISTS_ENTRY* ppEntry
    )
{
    PLSA_AD_DN_LISTS_ENTRY pEntry = *ppEntry;
    if (pEntry)
    {
        LsaFreeStringArray(pEntry->ppszDNValues, pEntry->dwDNCount);
        LsaFreeMemory(pEntry);
        *ppEntry = NULL;
    }
}

static
DWORD
CreateDNListEntry(
    OUT PLSA_AD_DN_LISTS_ENTRY* ppEntry,
    IN DWORD dwDNCount,
    IN PSTR* ppszDNValues
    )
{
    DWORD dwError = 0;
    PLSA_AD_DN_LISTS_ENTRY pEntry = NULL;

    dwError = LsaAllocateMemory(sizeof(*pEntry), (PVOID*)&pEntry);
    BAIL_ON_LSA_ERROR(dwError);

    pEntry->dwDNCount = dwDNCount;
    pEntry->ppszDNValues = ppszDNValues;    

    *ppEntry = pEntry;

cleanup:
    return dwError;

error:
    *ppEntry = NULL;
    DestroyDNListEntry(&pEntry);
    goto cleanup;
}

DWORD
ADLdap_GetGroupMembersDNList(
    IN HANDLE hDirectory,
    IN PCSTR pszGroupDN,
    OUT PDWORD pdwTotalDNCount,
    OUT PSTR** pppszDNValues
    )
{
    DWORD dwError = 0;
    PSTR szAttributeListMembers[] =
    {
        AD_LDAP_MEMBER_TAG,
        NULL
    };
    PSTR* ppszDNValuesTotal = NULL;
    PSTR* ppszDNValues = NULL;
    LDAPMessage* pMessage = NULL;
    DWORD dwDNCount = 0;
    DWORD dwTotalDNCount = 0;
    PDLINKEDLIST pDNList = NULL;
    PDLINKEDLIST pNode = NULL;
    PLSA_AD_DN_LISTS_ENTRY pDNEntry = NULL;
    PSTR pszRangeAttr = NULL;
    LDAP* pLd = LsaLdapGetSession(hDirectory);
    BerElement* pBer = NULL;
    PSTR pszRetrievedAttr = NULL;
    PSTR pszRetrievedRangeAttr = NULL;
    BOOLEAN bIsEnd = FALSE;
    DWORD iDNValues = 0;
    DWORD iDNValuesTotal = 0;

    for (;;)
    {
        if (pMessage)
        {
            ldap_msgfree(pMessage);
            pMessage = NULL;
        }

        dwError = LsaLdapDirectorySearch(
                        hDirectory,
                        pszGroupDN,
                        LDAP_SCOPE_BASE,
                        "(objectClass=*)",
                        szAttributeListMembers,
                        &pMessage);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaLdapGetStrings(
                        hDirectory,
                        pMessage,
                        AD_LDAP_MEMBER_TAG,
                        &ppszDNValues,
                        &dwDNCount);
        BAIL_ON_LSA_ERROR(dwError);

        if (ppszDNValues && dwDNCount)
        {
            if (pDNList)
            {
                // This is the case where we started out getting
                // ranged info but the info became non-ranged.
                // We might actually want to allow this to handle
                // a case where the membership list is trimmed
                // while we are enumerating.
                dwError = LSA_ERROR_LDAP_ERROR;
                BAIL_ON_LSA_ERROR(dwError);
            }

            dwTotalDNCount = dwDNCount;
            ppszDNValuesTotal = ppszDNValues;

            dwDNCount = 0;
            ppszDNValues = NULL;

            break;
        }

        if (pszRetrievedAttr)
        {
            ldap_memfree(pszRetrievedAttr);
        }

        if (pBer)
        {
             ber_free(pBer, 0);
        }

        pszRetrievedAttr = ldap_first_attribute(pLd, pMessage, &pBer);
        while (pszRetrievedAttr)
        {
            if (!strncasecmp(pszRetrievedAttr, "member;Range=", sizeof("member;Range=")-1))
            {
                pszRetrievedRangeAttr = pszRetrievedAttr;
                break;
            }
            ldap_memfree(pszRetrievedAttr);
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

        dwError = LsaLdapGetStrings(
                        hDirectory,
                        pMessage,
                        pszRetrievedRangeAttr,
                        &ppszDNValues,
                        &dwDNCount);
        BAIL_ON_LSA_ERROR(dwError);

        dwTotalDNCount += dwDNCount;

        dwError = CreateDNListEntry(
                        &pDNEntry,
                        dwDNCount,
                        ppszDNValues);
        BAIL_ON_LSA_ERROR(dwError);
        ppszDNValues = NULL;
        dwDNCount = 0;

        dwError = LsaDLinkedListPrepend(&pDNList, pDNEntry);
        BAIL_ON_LSA_ERROR(dwError);
        pDNEntry = NULL;

        if (bIsEnd)
        {
            break;
        }

        LSA_SAFE_FREE_STRING(pszRangeAttr);

        dwError = LsaAllocateStringPrintf(
                        &pszRangeAttr,
                        "member;Range=%d-*",
                        dwTotalDNCount);
        BAIL_ON_LSA_ERROR(dwError);

        szAttributeListMembers[0] = pszRangeAttr;
    }

    if (pDNList && !ppszDNValuesTotal)
    {
        dwError = LsaAllocateMemory(
                        sizeof(PSTR) * dwTotalDNCount,
                        (PVOID*)&ppszDNValuesTotal);
        BAIL_ON_LSA_ERROR(dwError);

        for (pNode = pDNList; pNode; pNode = pNode->pNext)
        {
            PLSA_AD_DN_LISTS_ENTRY pEntry = (PLSA_AD_DN_LISTS_ENTRY)pNode->pItem;

            for (iDNValues = 0; iDNValues < pEntry->dwDNCount; iDNValues++)
            {
                ppszDNValuesTotal[iDNValuesTotal] = pEntry->ppszDNValues[iDNValues];
                pEntry->ppszDNValues[iDNValues] = NULL;
                iDNValuesTotal++;
            }
        }
    }

    *pdwTotalDNCount = dwTotalDNCount;
    *pppszDNValues = ppszDNValuesTotal;

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

    LsaFreeStringArray(ppszDNValues, dwDNCount);
    DestroyDNListEntry(&pDNEntry);
    LSA_SAFE_FREE_STRING(pszRangeAttr);

    for (pNode = pDNList; pNode; pNode = pNode->pNext)
    {
        PLSA_AD_DN_LISTS_ENTRY pEntry = (PLSA_AD_DN_LISTS_ENTRY)pNode->pItem;
        DestroyDNListEntry(&pEntry);
    }
    LsaDLinkedListFree(pDNList);

    return dwError;

error:
    LsaFreeStringArray(ppszDNValuesTotal, iDNValuesTotal);

    *pdwTotalDNCount = 0;
    *pppszDNValues = NULL;

    goto cleanup;
}

DWORD
ADLdap_GetGroupMembers(
    HANDLE hProvider,
    HANDLE hDirectory,
    PCSTR  pszDomainName,
    PCSTR  pszDomainNetBiosName,
    PCSTR pszSid,
    size_t* psCount,
    PAD_SECURITY_OBJECT** pppResults)
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    DWORD dwDnCount = 0;
    PAD_SECURITY_OBJECT pGroupObj = NULL;
    PAD_SECURITY_OBJECT* ppResults = NULL;
    PSTR *ppszLDAPValues = NULL;
    DWORD dwFoundCount = 0;

    dwError = AD_FindObjectBySid(
                    hProvider,
                    pszDomainName,
                    pszDomainNetBiosName,
                    pszSid,
                    &pGroupObj);
    BAIL_ON_LSA_ERROR(dwError);

    if (pGroupObj->type != AccountType_Group)
    {
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = ADLdap_GetGroupMembersDNList(
                    hDirectory,
                    pGroupObj->pszDN,
                    &dwDnCount,
                    &ppszLDAPValues);
    BAIL_ON_LSA_ERROR(dwError);

#if 0
    dwError = AD_FindObjectsByDNList(
            hProvider,
            sDnCount,
            ppszLDAPValues,
            &ppResults);
            
    BAIL_ON_LSA_ERROR(dwError);
#endif
    
    dwError = ADLdap_FindObjectsByDNListBatched(
            hProvider,
            dwDnCount,
            ppszLDAPValues,
            &dwFoundCount,
            &ppResults);
    BAIL_ON_LSA_ERROR(dwError);

    *psCount = dwFoundCount;
    *pppResults = ppResults;

cleanup:
    ADCacheDB_SafeFreeObject(&pGroupObj);

    LsaFreeStringArray(ppszLDAPValues, dwDnCount);  

    return dwError;

error:
    *psCount = 0;
    *pppResults = NULL;
    ADCacheDB_SafeFreeObjectList(dwFoundCount, &ppResults);
    goto cleanup;
}

DWORD
ADLdap_GetUserGroupMembership(
    HANDLE  hProvider,
    uid_t   uid,
    int     *piPrimaryGroupIndex,
    PDWORD  pdwNumGroupsFound,
    PAD_SECURITY_OBJECT** pppGroupInfoList
    )
{
    DWORD dwError =  0;
    HANDLE hDirectory = (HANDLE)NULL;
    PAD_SECURITY_OBJECT* ppGroupInfoList = NULL;
    DWORD  dwNumGroupsFound = 0;
    int    iPrimaryGroupIndex = -1;
    PSTR pszFullDomainName = NULL;
    PSTR pszDomainDN = NULL;
    PSTR pszCellDN = NULL;
    PAD_SECURITY_OBJECT pUserInfo = NULL;    

    switch (gpADProviderData->dwDirectoryMode)
    {
        case DEFAULT_MODE:
            
            dwError = AD_FindUserObjectById(
                            hProvider,
                            uid,
                            &pUserInfo);
            BAIL_ON_LSA_ERROR(dwError);
            
            dwError = AD_DetermineTrustModeandDomainName(
                                pUserInfo->pszNetbiosDomainName,                                
                                NULL,
                                NULL,
                                &pszFullDomainName,
                                NULL);
            BAIL_ON_LSA_ERROR(dwError);
            
            dwError = LsaDmWrapLdapOpenDirectoryDomain(
                               pszFullDomainName,
                               &hDirectory);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaLdapConvertDomainToDN(
                               pszFullDomainName,
                               &pszDomainDN);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaAllocateStringPrintf(
                         &pszCellDN,
                         "CN=$LikewiseIdentityCell,%s",
                         pszDomainDN);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = DefaultModeGetUserGroupMembership(
                          hDirectory,
                          pszCellDN,
                          pUserInfo->pszNetbiosDomainName,
                          uid,
                          &iPrimaryGroupIndex,
                          &dwNumGroupsFound,
                          &ppGroupInfoList);
            BAIL_ON_LSA_ERROR(dwError);

            break;

        case CELL_MODE:

            dwError = LsaDmWrapLdapOpenDirectoryDomain(
                               gpADProviderData->szDomain,
                               &hDirectory);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = CellModeGetUserGroupMembership(
                          hDirectory,
                          gpADProviderData->cell.szCellDN,
                          gpADProviderData->szShortDomain,
                          uid,
                          &iPrimaryGroupIndex,
                          &dwNumGroupsFound,
                          &ppGroupInfoList);
            BAIL_ON_LSA_ERROR(dwError);

            break;

        case UNPROVISIONED_MODE:
            
            dwError = AD_FindUserObjectById(
                            hProvider,
                            uid,
                            &pUserInfo);
            BAIL_ON_LSA_ERROR(dwError);
            
            dwError = AD_DetermineTrustModeandDomainName(
                                pUserInfo->pszNetbiosDomainName,                                
                                NULL,
                                NULL,
                                &pszFullDomainName,
                                NULL);
            BAIL_ON_LSA_ERROR(dwError);
            
            dwError = LsaDmWrapLdapOpenDirectoryDomain(
                               pszFullDomainName,
                               &hDirectory);
            BAIL_ON_LSA_ERROR(dwError);


            dwError = UnprovisionedModeGetUserGroupMembership(
                          hDirectory,
                          pUserInfo->pszNetbiosDomainName,
                          uid,
                          &iPrimaryGroupIndex,
                          &dwNumGroupsFound,
                          &ppGroupInfoList);
            BAIL_ON_LSA_ERROR(dwError);

            break;
            
        default:
            dwError = LSA_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

    *piPrimaryGroupIndex = iPrimaryGroupIndex;
    *pppGroupInfoList = ppGroupInfoList;
    *pdwNumGroupsFound = dwNumGroupsFound;
        
cleanup:
    
    LSA_SAFE_FREE_STRING(pszFullDomainName);
    LSA_SAFE_FREE_STRING(pszDomainDN);
    LSA_SAFE_FREE_STRING(pszCellDN);
    
    ADCacheDB_SafeFreeObject(&pUserInfo);


    if (hDirectory != (HANDLE)NULL) {
        LsaLdapCloseDirectory(hDirectory);
    }

    return dwError;

error:

    *pppGroupInfoList = NULL;
    *pdwNumGroupsFound = 0;
    *piPrimaryGroupIndex = -1;

    LSA_LOG_ERROR("Failed to find user's group memberships of UID=%d. [error code:%d]",
                  uid, dwError);

    ADCacheDB_SafeFreeObjectList(dwNumGroupsFound, &ppGroupInfoList);

    goto cleanup;
}

DWORD
ADLdap_GetGCObjectInfoBySid(
    PCSTR pszGCHostName,
    PCSTR pszObjectSid,
    PSTR* ppszObjectDN,
    PSTR* ppszObjectDomainName,
    PSTR* ppszObjectSamaccountName)
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    LDAP* pGCLd = NULL;
    HANDLE hGCDirectory = (HANDLE)NULL;
    LDAPMessage *pGCMessage = NULL;
    PSTR  pszQuery = NULL;
    PSTR  pszObjectDN = NULL;
    PSTR  pszObjectDomainName = NULL;
    PSTR  pszObjectSamaccountName = NULL;
    DWORD dwCount = 0;
    PSTR szGCObjectAttributeList[] =
                    {AD_LDAP_SAM_NAME_TAG,
                     NULL
                    };

    BAIL_ON_INVALID_STRING(pszGCHostName);
    BAIL_ON_INVALID_STRING(pszObjectSid);

    dwError = LsaDmWrapLdapOpenDirectoryGc(pszGCHostName, &hGCDirectory);
    BAIL_ON_LSA_ERROR(dwError);

    pGCLd = LsaLdapGetSession(hGCDirectory);

    //Search in root node's GC for object's DN, sAMAccountName given object's objectSid
    dwError = LsaAllocateStringPrintf(
                 &pszQuery,
                 "(objectSid=%s)",
                 pszObjectSid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLdapDirectorySearch(
                      hGCDirectory,
                      "",
                      LDAP_SCOPE_SUBTREE,
                      pszQuery,
                      szGCObjectAttributeList,
                      &pGCMessage);
    BAIL_ON_LSA_ERROR(dwError);

    dwCount = ldap_count_entries(
                      pGCLd,
                      pGCMessage);
    if (dwCount < 0) {
       dwError = LSA_ERROR_LDAP_ERROR;
    } else if (dwCount == 0) {
       dwError = LSA_ERROR_NO_SUCH_USER_OR_GROUP;
    } else if (dwCount > 1) {
       dwError = LSA_ERROR_DUPLICATE_USER_OR_GROUP;
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLdapGetDN(
                    hGCDirectory,
                    pGCMessage,
                    &pszObjectDN);
    BAIL_ON_LSA_ERROR(dwError);
    BAIL_ON_INVALID_STRING(pszObjectDN);

    dwError = LsaLdapConvertDNToDomain(
                     pszObjectDN,
                     &pszObjectDomainName);
    BAIL_ON_LSA_ERROR(dwError);
    BAIL_ON_INVALID_STRING(pszObjectDomainName);

    dwError = LsaLdapGetString(
                hGCDirectory,
                pGCMessage,
                AD_LDAP_SAM_NAME_TAG,
                &pszObjectSamaccountName);
    BAIL_ON_LSA_ERROR(dwError);
    BAIL_ON_INVALID_STRING(pszObjectSamaccountName);

    *ppszObjectDN = pszObjectDN;
    *ppszObjectDomainName = pszObjectDomainName;
    *ppszObjectSamaccountName = pszObjectSamaccountName;

cleanup:

    if (pGCMessage) {
        ldap_msgfree(pGCMessage);
    }

    if (hGCDirectory != (HANDLE)NULL) {
        LsaLdapCloseDirectory(hGCDirectory);
    }

    LSA_SAFE_FREE_STRING(pszQuery);

    return dwError;

error:

   *ppszObjectDN = NULL;
   *ppszObjectDomainName = NULL;
   *ppszObjectSamaccountName = NULL;

   LSA_SAFE_FREE_STRING(pszObjectDN);
   LSA_SAFE_FREE_STRING(pszObjectDomainName);
   LSA_SAFE_FREE_STRING(pszObjectSamaccountName);

   goto cleanup;
}


//Following functions are utility functions to support MFS
DWORD
ADLdap_FindUserNameByAlias(
    PCSTR pszAlias,
    PSTR* ppszNT4Name
    )
{
    DWORD dwError = 0;
    PSTR pszNT4Name = NULL;
    PCSTR pszJoinedCellDN = gpADProviderData->cell.szCellDN;
    PCSTR pszJoinedDomainName = gpADProviderData->szDomain;

    if (gpADProviderData->dwDirectoryMode == UNPROVISIONED_MODE)
    {
        /* In unprovisioned mode, there are no aliases. The mode needs to be
         * checked before the pszJoinedCellDN variable is checked because it
         * will be NULL in this mode. If BAIL_ON_INVALID_STRING was run first,
         * it would return LSA_ERROR_INVALID_PARAMETER.
         */
        dwError = LSA_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    BAIL_ON_INVALID_STRING(pszAlias);
    BAIL_ON_INVALID_STRING(pszJoinedCellDN);
    BAIL_ON_INVALID_STRING(pszJoinedDomainName);

    switch (gpADProviderData->dwDirectoryMode){
        case DEFAULT_MODE:
            dwError = ADLdap_DefaultFindUserNameByAlias(
                             pszAlias,
                             &pszNT4Name);

            break;
        case CELL_MODE:
            dwError = ADLdap_CellFindUserNameByAlias(
                             pszAlias,
                             pszJoinedDomainName,
                             pszJoinedCellDN,
                             &pszNT4Name);

            break;

        default:
                dwError = LSA_ERROR_NOT_HANDLED;
    }
    BAIL_ON_LSA_ERROR(dwError);

    *ppszNT4Name = pszNT4Name;

cleanup:

    return dwError;

error:

    if (dwError == LSA_ERROR_DUPLICATE_USERNAME ||
        dwError == LSA_ERROR_DUPLICATE_USER_OR_GROUP)
    {
        if (AD_EventlogEnabled())
        {
            LsaSrvLogUserAliasConflictEvent(
                          pszAlias,
                          gpszADProviderName,
                          dwError);
        }
    }

    *ppszNT4Name = NULL;

    LSA_SAFE_FREE_STRING(pszNT4Name);

    goto cleanup;
}

DWORD
ADLdap_DefaultFindUserNameByAlias(
    PCSTR pszAlias,
    PSTR* ppszNT4Name
    )
{
    DWORD dwError = 0;
    PSTR pszNT4Name = NULL;
    PCSTR pszJoinedDomainName = gpADProviderData->szDomain;
    PSTR* ppszDomainNames = NULL;
    DWORD dwCount = 0;
    DWORD i = 0;

    dwError = ADLdap_DefaultFindUserNameByAliasInDomain(
                            pszAlias,
                            pszJoinedDomainName,
                            &pszNT4Name);
    if (!dwError)
    {
        goto cleanup;
    }
    else if (dwError != LSA_ERROR_NO_SUCH_USER)
    {
        BAIL_ON_LSA_ERROR(dwError);
    }

    //if not found, search in two-way trusted domain
    dwError = LsaDmWrapEnumExtraTwoWayForestTrustDomains(&ppszDomainNames, &dwCount);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LSA_ERROR_NO_SUCH_USER;

    for (i = 0; i < dwCount; i++)
    {
        dwError = ADLdap_DefaultFindUserNameByAliasInDomain(
                                pszAlias,
                                ppszDomainNames[i],
                                &pszNT4Name);
        if (!dwError)
        {
            goto cleanup;
        }
        else if (dwError != LSA_ERROR_NO_SUCH_USER)
        {
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    LSA_SAFE_FREE_STRING_ARRAY(ppszDomainNames);

    *ppszNT4Name = pszNT4Name;

    return dwError;

error:
    LSA_SAFE_FREE_STRING(pszNT4Name);
    goto cleanup;
}

DWORD
ADLdap_DefaultFindUserNameByAliasInDomain(
    PCSTR pszAlias,
    PCSTR pszDomainName,
    PSTR* ppszNT4Name
    )
{
    DWORD dwError = 0;
    HANDLE hDirectory = (HANDLE)NULL;
    PSTR pszCellDN = NULL;
    PSTR pszNT4Name = NULL;
    ADConfigurationMode adConfMode = NonSchemaMode;
    PSTR pszDirectoryRoot = NULL;

    dwError = LsaDmWrapLdapOpenDirectoryDomain(pszDomainName, &hDirectory);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLdapConvertDomainToDN(
                    pszDomainName,
                    &pszDirectoryRoot);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAllocateStringPrintf(&pszCellDN,"CN=$LikewiseIdentityCell,%s", pszDirectoryRoot);
    BAIL_ON_LSA_ERROR(dwError);

  //search the primary domain (the one current joined to)
    dwError = ADGetConfigurationMode(
                         hDirectory,
                         pszCellDN,
                         &adConfMode);
    if (dwError == LSA_ERROR_INCOMPATIBLE_MODES_BETWEEN_TRUSTEDDOMAINS){
        dwError = LSA_ERROR_NO_SUCH_USER;
    }
    BAIL_ON_LSA_ERROR(dwError);

    switch (adConfMode){
    case SchemaMode:
        dwError = ADLdap_DefaultSchemaFindUserNameByAlias(
                         pszAlias,
                         pszDomainName,
                         &pszNT4Name);

        break;
    case NonSchemaMode:
        dwError = ADLdap_DefaultNonSchemaFindUserNameByAlias(
                                 pszAlias,
                                 pszDomainName,
                                 &pszNT4Name);
        break;

    default:
        dwError = LSA_ERROR_NOT_HANDLED;
    }
    BAIL_ON_LSA_ERROR(dwError);

    *ppszNT4Name = pszNT4Name;

cleanup:

    if (hDirectory != (HANDLE)NULL) {
        LsaLdapCloseDirectory(hDirectory);
    }

    LSA_SAFE_FREE_STRING(pszDirectoryRoot);
    LSA_SAFE_FREE_STRING(pszCellDN);


    return dwError;

error:

    *ppszNT4Name = NULL;

    LSA_SAFE_FREE_STRING(pszNT4Name);

    goto cleanup;
}

DWORD
ADLdap_CellFindUserNameByAlias(
    PCSTR pszAlias,
    PCSTR pszDomainName,
    PCSTR pszCellDN,
    PSTR* ppszNT4Name
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    PSTR pszNT4Name = NULL;
    PDLINKEDLIST curr = NULL;

    dwError = ADLdap_CellFindUserNameByAliasInOneCell(
                  pszAlias,
                  pszDomainName,
                  pszCellDN,
                  &pszNT4Name);
    if ((dwError == LSA_ERROR_NO_SUCH_USER || dwError == LSA_ERROR_NO_SUCH_USER_OR_GROUP)
         && gpADProviderData->pCellList)
    {
       dwError = LSA_ERROR_SUCCESS;
    }
    BAIL_ON_LSA_ERROR(dwError);
    
    if (!IsNullOrEmptyString(pszNT4Name))
        goto done;

    curr = gpADProviderData->pCellList;

    while (curr && curr->pItem)
    {
        PSTR pszCurrDomainName = NULL;

        dwError = LsaLdapConvertDNToDomain(
                         ((PAD_LINKED_CELL_INFO)curr->pItem)->pszCellDN,
                         &pszCurrDomainName);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = ADLdap_CellFindUserNameByAliasInOneCell(
                           pszAlias,
                           pszCurrDomainName,
                           ((PAD_LINKED_CELL_INFO)curr->pItem)->pszCellDN,
                           &pszNT4Name);
        BAIL_ON_LSA_ERROR(dwError);

        LSA_SAFE_FREE_STRING(pszCurrDomainName);
        
        if ((dwError == LSA_ERROR_NO_SUCH_USER || dwError == LSA_ERROR_NO_SUCH_USER_OR_GROUP)
             && curr->pNext)
        {
           dwError = LSA_ERROR_SUCCESS;
        }
        BAIL_ON_LSA_ERROR(dwError);
        
        if (!IsNullOrEmptyString(pszNT4Name))
            break;
        
        curr = curr->pNext;
    }

done:

     *ppszNT4Name = pszNT4Name;

cleanup:

    return dwError;

error:

    *ppszNT4Name = NULL;

    LSA_SAFE_FREE_STRING(pszNT4Name);

    goto cleanup;
}

DWORD
ADLdap_CellFindUserNameByAliasInOneCell(
    PCSTR pszAlias,
    PCSTR pszDomainName,
    PCSTR pszCellDN,
    PSTR* ppszNT4Name
    )
{
    DWORD dwError = 0;
    PSTR pszNT4Name = NULL;
    HANDLE hDirectory = (HANDLE)NULL;
    PSTR  pszQuery = NULL;
    ADConfigurationMode adConfMode = NonSchemaMode;
    PSTR pszEscapedAlias = NULL;

    dwError = LsaDmWrapLdapOpenDirectoryDomain(pszDomainName, &hDirectory);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADGetConfigurationMode(
                         hDirectory,
                         pszCellDN,
                         &adConfMode);
    if (dwError == LSA_ERROR_INCOMPATIBLE_MODES_BETWEEN_TRUSTEDDOMAINS){
        dwError = LSA_ERROR_NO_SUCH_USER;
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLdapEscapeString(
                &pszEscapedAlias,
                pszAlias);
    BAIL_ON_LSA_ERROR(dwError);

    switch (adConfMode){
        case SchemaMode:
                dwError = LsaAllocateStringPrintf(
                                 &pszQuery,
                                 "(&(objectClass=posixAccount)(keywords=objectClass=centerisLikewiseUser)(uid=%s))",
                                 pszEscapedAlias);

            break;

        case NonSchemaMode:
                dwError = LsaAllocateStringPrintf(
                                 &pszQuery,
                                 "(&(objectClass=serviceConnectionPoint)(keywords=objectClass=centerisLikewiseUser)(keywords=uid=%s))",
                                 pszEscapedAlias);

            break;

        default:
            dwError = LSA_ERROR_INVALID_PARAMETER;
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADLdap_CellFindUserNameHelper(
                     hDirectory,
                     pszDomainName,
                     pszCellDN,
                     pszQuery,
                     &pszNT4Name);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszNT4Name = pszNT4Name;

cleanup:

    if (hDirectory != (HANDLE)NULL) {
        LsaLdapCloseDirectory(hDirectory);
    }

    LSA_SAFE_FREE_STRING(pszQuery);

    LSA_SAFE_FREE_STRING(pszEscapedAlias);

    return dwError;

error:

    *ppszNT4Name = NULL;

    LSA_SAFE_FREE_STRING(pszNT4Name);

    goto cleanup;
}

DWORD
ADLdap_CellFindUserNameHelper(
    HANDLE hDirectory,
    PCSTR pszDomainName,
    PCSTR pszCellDN,
    PCSTR pszQuery,
    PSTR* ppszNT4Name)
{
    DWORD dwError = 0;
    PSTR pszNT4Name = NULL;
    PSTR pszUsersContainerDN = NULL;
    LDAP* pLd = NULL;
    LDAPMessage *pCellUserMessage = NULL;
    DWORD dwCount = 0;
    PSTR* ppszValues = NULL;
    DWORD dwNumValues = 0;
    DWORD iValue = 0;
    PCSTR pszObjectSID = NULL;
    BOOLEAN bValidADEntry = FALSE;
    PSTR szCellUserAttributeList[] =
            {
                AD_LDAP_KEYWORDS_TAG,
                NULL
            };
    PSTR* ppszDomainNames = NULL;
    DWORD i = 0;

    if (hDirectory == (HANDLE)NULL){
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pLd = LsaLdapGetSession(hDirectory);

    dwError = LsaAllocateStringPrintf(
                     &pszUsersContainerDN,
                     "CN=Users,%s",
                     pszCellDN);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLdapDirectorySearch(
               hDirectory,
               pszUsersContainerDN,
               LDAP_SCOPE_ONELEVEL,
               pszQuery,
               szCellUserAttributeList,
               &pCellUserMessage);
    BAIL_ON_LSA_ERROR(dwError);

    dwCount = ldap_count_entries(
                   pLd,
                   pCellUserMessage);
    if (dwCount < 0) {
           dwError = LSA_ERROR_LDAP_ERROR;
    } else if (dwCount == 0) {
           dwError = LSA_ERROR_NO_SUCH_USER;
    } else if (dwCount > 1) {
           dwError = LSA_ERROR_DUPLICATE_USERNAME;
    }
    BAIL_ON_LSA_ERROR(dwError);

    //Confirm the entry we obtain from AD is valid by retrieving its DN
    dwError = LsaLdapIsValidADEntry(
                    hDirectory,
                    pCellUserMessage,
                    &bValidADEntry);
    BAIL_ON_LSA_ERROR(dwError);

    if (!bValidADEntry){
        dwError = LSA_ERROR_LDAP_FAILED_GETDN;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaLdapGetStrings(hDirectory,
                                pCellUserMessage,
                                AD_LDAP_KEYWORDS_TAG,
                                &ppszValues,
                                &dwNumValues);
    BAIL_ON_LSA_ERROR(dwError);

    for (iValue = 0; iValue < dwNumValues; iValue++)
    {
        if (!strncasecmp(ppszValues[iValue], "backLink=", sizeof("backLink=")-1))
        {
           pszObjectSID = ppszValues[iValue] + sizeof("backLink=") - 1;
           break;
        }
    }

    if (IsNullOrEmptyString(pszObjectSID)) {
        dwError = LSA_ERROR_INVALID_SID;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaDmWrapNetLookupNameByObjectSid(
                    pszDomainName,
                    pszObjectSID,
                    &pszNT4Name);
    BAIL_ON_LSA_ERROR(dwError);

    if (dwError == LSA_ERROR_NO_SUCH_USER_OR_GROUP){

        dwError = LsaDmWrapEnumExtraForestTrustDomains(&ppszDomainNames, &dwCount);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LSA_ERROR_NO_SUCH_USER_OR_GROUP;

        for (i = 0; i < dwCount; i++)
        {
            //find a two-way across forest trust
            dwError = LsaDmWrapNetLookupNameByObjectSid(
                            pszDomainName,
                            pszObjectSID,
                            &pszNT4Name);
            BAIL_ON_LSA_ERROR(dwError);

            if (!dwError)
            {
                break;
            }
            else if (dwError != LSA_ERROR_NO_SUCH_USER_OR_GROUP)
            {
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
    }
    BAIL_ON_LSA_ERROR(dwError);

    BAIL_ON_INVALID_STRING(pszNT4Name);

    *ppszNT4Name = pszNT4Name;

cleanup:

    if (pCellUserMessage) {
        ldap_msgfree(pCellUserMessage);
    }

    LSA_SAFE_FREE_STRING(pszUsersContainerDN);
    LSA_SAFE_FREE_STRING_ARRAY(ppszDomainNames);

    if (ppszValues) {
        LsaFreeStringArray(ppszValues, dwNumValues);
    }

    return dwError;

error:

    *ppszNT4Name = NULL;

    LSA_SAFE_FREE_STRING(pszNT4Name);

    goto cleanup;
}

DWORD
ADLdap_FindUserNameById(
    uid_t uid,
    PSTR* ppszNT4Name)
{
    DWORD dwError = 0;
    PSTR pszNT4Name = NULL;
    PCSTR pszJoinedCellDN = gpADProviderData->cell.szCellDN;
    PCSTR pszJoinedDomainName = gpADProviderData->szDomain;

    BAIL_ON_INVALID_STRING(pszJoinedDomainName);

    switch (gpADProviderData->dwDirectoryMode){
        case DEFAULT_MODE:
            BAIL_ON_INVALID_STRING(pszJoinedCellDN);

            dwError = ADLdap_DefaultFindUserNameById(
                             uid,
                             &pszNT4Name);

            break;

        case CELL_MODE:
            BAIL_ON_INVALID_STRING(pszJoinedCellDN);

            dwError = ADLdap_CellFindUserNameById(
                             uid,
                             pszJoinedDomainName,
                             pszJoinedCellDN,
                             &pszNT4Name);

            break;

        case UNPROVISIONED_MODE:
            dwError = ADLdap_UnprovisionedFindUserNameById(
                             uid,
                             &pszNT4Name);

            break;


        default:
                dwError = LSA_ERROR_NOT_HANDLED;
    }
    BAIL_ON_LSA_ERROR(dwError);

    *ppszNT4Name = pszNT4Name;

cleanup:

    return dwError;

error:
    *ppszNT4Name = NULL;

    LSA_SAFE_FREE_STRING(pszNT4Name);

    goto cleanup;
}

DWORD
ADLdap_DefaultFindUserNameById(
    uid_t uid,
    PSTR* ppszNT4Name
    )
{
    DWORD dwError = 0;
    PSTR pszNT4Name = NULL;
    PCSTR pszJoinedDomainName = gpADProviderData->szDomain;
    PSTR* ppszDomainNames = NULL;
    DWORD dwCount = 0;
    DWORD i = 0;

    dwError = ADLdap_DefaultFindUserNameByIdInDomain(
                            uid,
                            pszJoinedDomainName,
                            &pszNT4Name);
    if (!dwError)
    {
        goto cleanup;
    }
    else if (dwError != LSA_ERROR_NO_SUCH_USER)
    {
        BAIL_ON_LSA_ERROR(dwError);
    }

    //if not found, search in two-way trusted domain
    dwError = LsaDmWrapEnumExtraTwoWayForestTrustDomains(
                     &ppszDomainNames,
                     &dwCount);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LSA_ERROR_NO_SUCH_USER;

    for (i = 0; i < dwCount; i++)
    {
        //find a two-way across forest trust
        dwError = ADLdap_DefaultFindUserNameByIdInDomain(
                                uid,
                                ppszDomainNames[i],
                                &pszNT4Name);
        if (!dwError)
        {
            goto cleanup;
        }
        else if (dwError != LSA_ERROR_NO_SUCH_USER)
        {
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    LSA_SAFE_FREE_STRING_ARRAY(ppszDomainNames);

    *ppszNT4Name = pszNT4Name;

    return dwError;

error:
    LSA_SAFE_FREE_STRING(pszNT4Name);
    goto cleanup;
}

DWORD
ADLdap_DefaultFindUserNameByIdInDomain(
    uid_t uid,
    PCSTR pszDomainName,
    PSTR* ppszNT4Name
    )
{
    DWORD dwError = 0;
    HANDLE hDirectory = (HANDLE)NULL;
    PSTR pszCellDN = NULL;
    PSTR pszNT4Name = NULL;
    ADConfigurationMode adConfMode = NonSchemaMode;
    PSTR pszDirectoryRoot = NULL;

    dwError = LsaDmWrapLdapOpenDirectoryDomain(pszDomainName, &hDirectory);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLdapConvertDomainToDN(
                    pszDomainName,
                    &pszDirectoryRoot);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAllocateStringPrintf(&pszCellDN,"CN=$LikewiseIdentityCell,%s", pszDirectoryRoot);
    BAIL_ON_LSA_ERROR(dwError);
    LSA_SAFE_FREE_STRING(pszDirectoryRoot);

  //search the primary domain (the one current joined to)
    dwError = ADGetConfigurationMode(
                         hDirectory,
                         pszCellDN,
                         &adConfMode);
    if (dwError == LSA_ERROR_INCOMPATIBLE_MODES_BETWEEN_TRUSTEDDOMAINS){
        dwError = LSA_ERROR_NO_SUCH_USER;
    }
    BAIL_ON_LSA_ERROR(dwError);

    switch (adConfMode){
    case SchemaMode:
        dwError = ADLdap_DefaultSchemaFindUserNameById(
                                 uid,
                                 pszDomainName,
                                 &pszNT4Name);

        break;
    case NonSchemaMode:
        dwError = ADLdap_DefaultNonSchemaFindUserNameById(
                                 uid,
                                 pszDomainName,
                                 &pszNT4Name);
        break;

    default:
        dwError = LSA_ERROR_NOT_HANDLED;
    }
    BAIL_ON_LSA_ERROR(dwError);

    *ppszNT4Name = pszNT4Name;

cleanup:

    if (hDirectory != (HANDLE)NULL) {
        LsaLdapCloseDirectory(hDirectory);
    }

    LSA_SAFE_FREE_STRING(pszDirectoryRoot);
    LSA_SAFE_FREE_STRING(pszCellDN);


    return dwError;

error:

    *ppszNT4Name = NULL;

    LSA_SAFE_FREE_STRING(pszNT4Name);

    goto cleanup;
}

DWORD
ADLdap_DefaultSchemaFindUserNameByAlias(
    PCSTR pszAlias,
    PCSTR pszRootDomainName,
    PSTR* ppszNT4Name
    )
{
    DWORD dwError = 0;
    PSTR pszNT4Name = NULL;
    HANDLE hGCDirectory = (HANDLE)NULL;
    PSTR pszQuery = NULL;
    PSTR pszEscapedAlias = NULL;

    BAIL_ON_INVALID_STRING(pszRootDomainName);

    dwError = LsaDmWrapLdapOpenDirectoryGc(pszRootDomainName, &hGCDirectory);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLdapEscapeString(
                &pszEscapedAlias,
                pszAlias);
    BAIL_ON_LSA_ERROR(dwError);

    //Search in root node's GC for object's DN, sAMAccountName
    // (1) given user's alias stored in uid Or
    // (2) given user's id stored in uidNumber
    dwError = LsaAllocateStringPrintf(
                     &pszQuery,
                     "(&(objectClass=User)(uid=%s))",
                      pszEscapedAlias);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADLdap_DefaultSchemaFindUserNameHelper(
                       hGCDirectory,
                       pszQuery,
                       &pszNT4Name);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszNT4Name = pszNT4Name;


cleanup:
    if (hGCDirectory != (HANDLE)NULL) {
        LsaLdapCloseDirectory(hGCDirectory);
    }

    LSA_SAFE_FREE_STRING(pszQuery);

    LSA_SAFE_FREE_STRING(pszEscapedAlias);

    return dwError;

error:

    *ppszNT4Name = NULL;

    LSA_SAFE_FREE_STRING(pszNT4Name);

    goto cleanup;
}

DWORD
ADLdap_DefaultSchemaFindUserNameById(
    uid_t uid,
    PCSTR pszRootDomainName,
    PSTR* ppszNT4Name
    )
{
    DWORD dwError = 0;
    PSTR pszNT4Name = NULL;
    HANDLE hGCDirectory = (HANDLE)NULL;
    PSTR pszQuery = NULL;

    BAIL_ON_INVALID_STRING(pszRootDomainName);

    dwError = LsaDmWrapLdapOpenDirectoryGc(pszRootDomainName, &hGCDirectory);
    BAIL_ON_LSA_ERROR(dwError);

    //Search in root node's GC for object's DN, sAMAccountName
    // (1) given user's alias stored in uid Or
    // (2) given user's id stored in uidNumber
    dwError = LsaAllocateStringPrintf(
                     &pszQuery,
                     "(&(objectClass=User)(uidNumber=%d))",
                      uid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADLdap_DefaultSchemaFindUserNameHelper(
                       hGCDirectory,
                       pszQuery,
                       &pszNT4Name);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszNT4Name = pszNT4Name;


cleanup:
    if (hGCDirectory != (HANDLE)NULL) {
        LsaLdapCloseDirectory(hGCDirectory);
    }

    LSA_SAFE_FREE_STRING(pszQuery);

    return dwError;

error:

    *ppszNT4Name = NULL;

    LSA_SAFE_FREE_STRING(pszNT4Name);

    goto cleanup;
}

DWORD
ADLdap_DefaultSchemaFindUserNameHelper(
    HANDLE hGCDirectory,
    PCSTR pszQuery,
    PSTR* ppszNT4Name
    )
{
    DWORD dwError = 0;
    PSTR pszNT4Name = NULL;
    LDAPMessage *pMessage = NULL;
    LDAP* pGCLd = NULL;
    PSTR pszUserDN = NULL;
    PSTR pszUserDomainName = NULL;
    PSTR pszUserNetBiosName = NULL;
    PSTR pszUserSamaccountName = NULL;
    DWORD dwCount = 0;
    PSTR szGCObjectAttributeList[] =
                    {AD_LDAP_SAM_NAME_TAG,
                     NULL
                    };

    if (hGCDirectory == (HANDLE)NULL){
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pGCLd = LsaLdapGetSession(hGCDirectory);

    dwError = LsaLdapDirectorySearch(
                      hGCDirectory,
                      "",
                      LDAP_SCOPE_SUBTREE,
                      pszQuery,
                      szGCObjectAttributeList,
                      &pMessage);
    BAIL_ON_LSA_ERROR(dwError);

    dwCount = ldap_count_entries(
                      pGCLd,
                      pMessage);
    if (dwCount < 0) {
       dwError = LSA_ERROR_LDAP_ERROR;
    } else if (dwCount == 0) {
       dwError = LSA_ERROR_NO_SUCH_USER;
    } else if (dwCount > 1) {
       dwError = LSA_ERROR_DUPLICATE_USERNAME;
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLdapGetDN(
                    hGCDirectory,
                    pMessage,
                    &pszUserDN);
    BAIL_ON_LSA_ERROR(dwError);
    BAIL_ON_INVALID_STRING(pszUserDN);

    dwError = LsaLdapConvertDNToDomain(
                     pszUserDN,
                     &pszUserDomainName);
    BAIL_ON_LSA_ERROR(dwError);
    BAIL_ON_INVALID_STRING(pszUserDomainName);

    dwError = LsaLdapGetString(
                hGCDirectory,
                pMessage,
                AD_LDAP_SAM_NAME_TAG,
                &pszUserSamaccountName);
    BAIL_ON_LSA_ERROR(dwError);
    BAIL_ON_INVALID_STRING(pszUserSamaccountName);

    dwError = LsaDmWrapGetDomainName(
                pszUserDomainName,
                NULL,
                &pszUserNetBiosName);
    BAIL_ON_LSA_ERROR(dwError);

    LsaStrToUpper(pszUserNetBiosName);
    LsaStrToLower(pszUserSamaccountName);

    dwError = LsaAllocateStringPrintf(
                     &pszNT4Name,
                     "%s\\%s",
                     pszUserNetBiosName,
                     pszUserSamaccountName);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszNT4Name = pszNT4Name;

cleanup:

    if (pMessage) {
        ldap_msgfree(pMessage);
    }

    LSA_SAFE_FREE_STRING(pszUserDN);
    LSA_SAFE_FREE_STRING(pszUserDomainName);
    LSA_SAFE_FREE_STRING(pszUserNetBiosName);
    LSA_SAFE_FREE_STRING(pszUserSamaccountName);

    return dwError;

error:

    *ppszNT4Name = NULL;
    LSA_SAFE_FREE_STRING(pszNT4Name);

    goto cleanup;
}

DWORD
ADLdap_DefaultNonSchemaFindUserNameByAlias(
    PCSTR pszAlias,
    PCSTR pszRootDomainName,
    PSTR* ppszNT4Name
    )
{
    DWORD dwError = 0;
    PSTR pszNT4Name = NULL;
    HANDLE hGCDirectory = (HANDLE)NULL;
    PSTR pszQuery = NULL;
    PSTR pszEscapedAlias = NULL;

    BAIL_ON_INVALID_STRING(pszRootDomainName);

    dwError = LsaDmWrapLdapOpenDirectoryGc(pszRootDomainName, &hGCDirectory);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLdapEscapeString(
                &pszEscapedAlias,
                pszAlias);
    BAIL_ON_LSA_ERROR(dwError);

    //Search in root node's GC for object's DN, sAMAccountName
    // (1) given user's alias stored in uid Or
    // (2) given user's id stored in uidNumber
    dwError = LsaAllocateStringPrintf(
                     &pszQuery,
                     "(&(objectClass=serviceConnectionPoint)(keywords=objectClass=centerisLikewiseUser)(keywords=uid=%s))",
                      pszEscapedAlias);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADLdap_DefaultNonSchemaFindUserNameHelper(
                       hGCDirectory,
                       pszRootDomainName,
                       pszQuery,
                       &pszNT4Name);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszNT4Name = pszNT4Name;


cleanup:
    if (hGCDirectory != (HANDLE)NULL) {
        LsaLdapCloseDirectory(hGCDirectory);
    }

    LSA_SAFE_FREE_STRING(pszQuery);

    LSA_SAFE_FREE_STRING(pszEscapedAlias);

    return dwError;

error:

    *ppszNT4Name = NULL;

    LSA_SAFE_FREE_STRING(pszNT4Name);

    goto cleanup;

}

DWORD
ADLdap_DefaultNonSchemaFindUserNameById(
    uid_t uid,
    PCSTR pszRootDomainName,
    PSTR* ppszNT4Name
    )
{
    DWORD dwError = 0;
    PSTR pszNT4Name = NULL;
    HANDLE hGCDirectory = (HANDLE)NULL;
    PSTR pszQuery = NULL;

    BAIL_ON_INVALID_STRING(pszRootDomainName);

    dwError = LsaDmWrapLdapOpenDirectoryGc(pszRootDomainName, &hGCDirectory);
    BAIL_ON_LSA_ERROR(dwError);

    //Search in root node's GC for object's DN, sAMAccountName
    // (1) given user's alias stored in uid Or
    // (2) given user's id stored in uidNumber
    dwError = LsaAllocateStringPrintf(
                     &pszQuery,
                     "(&(objectClass=serviceConnectionPoint)(keywords=objectClass=centerisLikewiseUser)(keywords=uidNumber=%d))",
                      uid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADLdap_DefaultNonSchemaFindUserNameHelper(
                       hGCDirectory,
                       pszRootDomainName,
                       pszQuery,
                       &pszNT4Name);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszNT4Name = pszNT4Name;


cleanup:
    if (hGCDirectory != (HANDLE)NULL) {
        LsaLdapCloseDirectory(hGCDirectory);
    }

    LSA_SAFE_FREE_STRING(pszQuery);

    return dwError;

error:

    *ppszNT4Name = NULL;

    LSA_SAFE_FREE_STRING(pszNT4Name);

    goto cleanup;
}

DWORD
ADLdap_DefaultNonSchemaFindUserNameHelper(
    HANDLE hGCDirectory,
    PCSTR  pszRootDomainName,
    PCSTR  pszQuery,
    PSTR* ppszNT4Name
    )
{
    DWORD dwError = 0;
    PSTR pszNT4Name = NULL;
    PSTR szCellUserAttributeList[] =
            {
                AD_LDAP_KEYWORDS_TAG,
                NULL
            };
    LDAP* pGCLd = NULL;
    LDAPMessage *pCellUserMessage = NULL;
    PSTR pszUserDN = NULL;
    PSTR pszUserDomainName = NULL;
    PSTR pszUserNetBiosName = NULL;
    PSTR pszUserSamaccountName = NULL;
    DWORD dwCount = 0;
    PSTR* ppszValues = NULL;
    DWORD dwNumValues = 0;
    DWORD iValue = 0;
    PCSTR pszObjectSID = NULL;
    PSTR pszRootDomainDN = NULL;
    PSTR pszCellDN = NULL;
    LDAPMessage *pUserMessage = NULL;
    LDAPMessage *pFoundUserMessage = NULL;
    PSTR pUserPseudoDN = NULL;
    BOOLEAN bFound = FALSE;
    PSTR pszUserName = NULL;

    if (hGCDirectory == (HANDLE)NULL){
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pGCLd = LsaLdapGetSession(hGCDirectory);

    dwError = LsaLdapConvertDomainToDN(
                    pszRootDomainName,
                    &pszRootDomainDN);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAllocateStringPrintf(
                 &pszCellDN,
                 "CN=$LikewiseIdentityCell,%s",
                 pszRootDomainDN);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLdapDirectorySearch(
                      hGCDirectory,
                      "",
                      LDAP_SCOPE_SUBTREE,
                      pszQuery,
                      szCellUserAttributeList,
                      &pCellUserMessage);
    BAIL_ON_LSA_ERROR(dwError);

    dwCount = ldap_count_entries(
                      pGCLd,
                      pCellUserMessage);
    if (dwCount < 0) {
       dwError = LSA_ERROR_LDAP_ERROR;
    } else if (dwCount == 0) {
       dwError = LSA_ERROR_NO_SUCH_USER;
    }
    BAIL_ON_LSA_ERROR(dwError);

    pUserMessage = ldap_first_entry(
                           pGCLd,
                           pCellUserMessage);
    BAIL_ON_LSA_ERROR(dwError);

    while (pUserMessage)
    {
        LSA_SAFE_FREE_STRING(pUserPseudoDN);

        dwError = LsaLdapGetDN(
                      hGCDirectory,
                      pUserMessage,
                      &pUserPseudoDN);
        BAIL_ON_LSA_ERROR(dwError);

        LsaStrToUpper(pUserPseudoDN);

        if (strstr(pUserPseudoDN, ",CN=$LIKEWISEIDENTITYCELL,DC="))
        {            
            if (!bFound)
            {
                bFound = TRUE;
                pFoundUserMessage = pUserMessage;
                dwError = LsaAllocateString(
                             pUserPseudoDN,
                             &pszUserName);
                BAIL_ON_LSA_ERROR(dwError);
            }
            else
            {
                dwError = LSA_ERROR_DUPLICATE_USERNAME;
                if (AD_EventlogEnabled())
                {
                    LsaSrvLogDuplicateObjectFoundEvent(
                                    pszUserName,
                                    pUserPseudoDN,
                                    gpszADProviderName,
                                    dwError);
                }
                BAIL_ON_LSA_ERROR(dwError);
            }      
        }

        pUserMessage = ldap_next_entry(
                                  pGCLd,
                                  pUserMessage);
    }

    if (!bFound || !pFoundUserMessage)
    {
        dwError = LSA_ERROR_NO_SUCH_USER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaLdapGetStrings(hGCDirectory,
                                pFoundUserMessage,
                                AD_LDAP_KEYWORDS_TAG,
                                &ppszValues,
                                &dwNumValues);
    BAIL_ON_LSA_ERROR(dwError);

    for (iValue = 0; iValue < dwNumValues; iValue++)
    {
        if (!strncasecmp(ppszValues[iValue], "backLink=", sizeof("backLink=")-1))
        {
           pszObjectSID = ppszValues[iValue] + sizeof("backLink=") - 1;
           break;
        }
    }

    if (IsNullOrEmptyString(pszObjectSID)) {
        dwError = LSA_ERROR_INVALID_SID;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError =  ADLdap_GetGCObjectInfoBySid(
                      pszRootDomainName,
                      pszObjectSID,
                      &pszUserDN,
                      &pszUserDomainName,
                      &pszUserSamaccountName);
    BAIL_ON_LSA_ERROR(dwError);

    BAIL_ON_INVALID_STRING(pszUserDN);
    BAIL_ON_INVALID_STRING(pszUserDomainName);
    BAIL_ON_INVALID_STRING(pszUserSamaccountName);

    dwError = LsaDmWrapGetDomainName(
                pszUserDomainName,
                NULL,
                &pszUserNetBiosName);
    BAIL_ON_LSA_ERROR(dwError);

    LsaStrToUpper(pszUserNetBiosName);
    LsaStrToLower(pszUserSamaccountName);

    dwError = LsaAllocateStringPrintf(
                     &pszNT4Name,
                     "%s\\%s",
                     pszUserNetBiosName,
                     pszUserSamaccountName);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszNT4Name = pszNT4Name;

cleanup:

    if (pCellUserMessage) {
        ldap_msgfree(pCellUserMessage);
    }

    LSA_SAFE_FREE_STRING(pszUserDN);
    LSA_SAFE_FREE_STRING(pszUserDomainName);
    LSA_SAFE_FREE_STRING(pszUserNetBiosName);
    LSA_SAFE_FREE_STRING(pszUserSamaccountName);

    LSA_SAFE_FREE_STRING(pszRootDomainDN);
    LSA_SAFE_FREE_STRING(pszCellDN);
    LSA_SAFE_FREE_STRING(pUserPseudoDN);
    LSA_SAFE_FREE_STRING(pszUserName);

    if (ppszValues) {
        LsaFreeStringArray(ppszValues, dwNumValues);
    }

    return dwError;

error:

    *ppszNT4Name = NULL;

    LSA_SAFE_FREE_STRING(pszNT4Name);

    goto cleanup;
}

DWORD
ADLdap_CellFindUserNameById(
    uid_t uid,
    PCSTR pszDomainName,
    PCSTR pszCellDN,
    PSTR* ppszNT4Name
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    PSTR pszNT4Name = NULL;
    PDLINKEDLIST curr = NULL;


    dwError = ADLdap_CellFindUserNameByIdInOneCell(
                  uid,
                  pszDomainName,
                  pszCellDN,
                  &pszNT4Name);
     
    if ((dwError == LSA_ERROR_NO_SUCH_USER || dwError == LSA_ERROR_NO_SUCH_USER_OR_GROUP)
         && gpADProviderData->pCellList)
    {
       dwError = LSA_ERROR_SUCCESS;
    }
    BAIL_ON_LSA_ERROR(dwError);
    
    if (!IsNullOrEmptyString(pszNT4Name))
       goto done;

    curr = gpADProviderData->pCellList;

    while (curr && curr->pItem)
    {
        PSTR pszCurrDomainName = NULL;

        dwError = LsaLdapConvertDNToDomain(
                         ((PAD_LINKED_CELL_INFO)curr->pItem)->pszCellDN,
                         &pszCurrDomainName);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = ADLdap_CellFindUserNameByIdInOneCell(
                           uid,
                           pszCurrDomainName,
                           ((PAD_LINKED_CELL_INFO)curr->pItem)->pszCellDN,
                           &pszNT4Name);        

        LSA_SAFE_FREE_STRING(pszCurrDomainName);

        if ((dwError == LSA_ERROR_NO_SUCH_USER || dwError == LSA_ERROR_NO_SUCH_USER_OR_GROUP)
             && curr->pNext)
        {
           dwError = LSA_ERROR_SUCCESS;
        }
        BAIL_ON_LSA_ERROR(dwError);
        
        if (!IsNullOrEmptyString(pszNT4Name))
                break;

        curr = curr->pNext;
    }

done:

     *ppszNT4Name = pszNT4Name;

cleanup:

    return dwError;

error:

    *ppszNT4Name = NULL;

    LSA_SAFE_FREE_STRING(pszNT4Name);

    goto cleanup;
}

DWORD
ADLdap_CellFindUserNameByIdInOneCell(
    uid_t uid,
    PCSTR pszDomainName,
    PCSTR pszCellDN,
    PSTR* ppszNT4Name
    )
{
    DWORD dwError = 0;
    PSTR pszNT4Name = NULL;
    HANDLE hDirectory = (HANDLE)NULL;
    PSTR pszQuery = NULL;
    ADConfigurationMode adConfMode = NonSchemaMode;

    dwError = LsaDmWrapLdapOpenDirectoryDomain(pszDomainName, &hDirectory);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADGetConfigurationMode(
                         hDirectory,
                         pszCellDN,
                         &adConfMode);
    if (dwError == LSA_ERROR_INCOMPATIBLE_MODES_BETWEEN_TRUSTEDDOMAINS){
        dwError = LSA_ERROR_NO_SUCH_USER;
    }
    BAIL_ON_LSA_ERROR(dwError);

    switch (adConfMode){
        case SchemaMode:
                dwError = LsaAllocateStringPrintf(
                                 &pszQuery,
                                 "(&(objectClass=posixAccount)(keywords=objectClass=centerisLikewiseUser)(uidNumber=%d))",
                                 uid);

            break;

        case NonSchemaMode:
                dwError = LsaAllocateStringPrintf(
                                 &pszQuery,
                                 "(&(objectClass=serviceConnectionPoint)(keywords=objectClass=centerisLikewiseUser)(keywords=uidNumber=%d))",
                                 uid);

            break;

        default:
            dwError = LSA_ERROR_INVALID_PARAMETER;
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADLdap_CellFindUserNameHelper(
                     hDirectory,
                     pszDomainName,
                     pszCellDN,
                     pszQuery,
                     &pszNT4Name);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszNT4Name = pszNT4Name;

cleanup:

    if (hDirectory != (HANDLE)NULL) {
        LsaLdapCloseDirectory(hDirectory);
    }

    LSA_SAFE_FREE_STRING(pszQuery);

    return dwError;

error:

    *ppszNT4Name = NULL;

    LSA_SAFE_FREE_STRING(pszNT4Name);

    goto cleanup;
}

DWORD
ADLdap_UnprovisionedFindUserNameById(
    uid_t uid,
    PSTR* ppszNT4Name)
{
    DWORD dwError = 0;
    PSTR pszNT4Name = NULL;
    PCSTR pszJoinedDomainName = gpADProviderData->szDomain;
    PSTR* ppszDomainNames = NULL;
    DWORD dwCount = 0;
    DWORD i = 0;

    dwError = ADLdap_UnprovisionedFindObjectNameByIdInDomain(
                          (DWORD)uid,
                          pszJoinedDomainName,
                          &pszNT4Name);
    if (!dwError)
    {
        goto cleanup;
    }
    else if (dwError != LSA_ERROR_NO_SUCH_USER &&
             dwError != LSA_ERROR_NO_SUCH_USER_OR_GROUP)
    {
        BAIL_ON_LSA_ERROR(dwError);
    }

    //if not found, search in one-way or two-way trusts
    dwError = LsaDmWrapEnumExtraForestTrustDomains(&ppszDomainNames, &dwCount);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LSA_ERROR_NO_SUCH_USER;

    for (i = 0; i < dwCount; i++)
    {
        dwError = ADLdap_UnprovisionedFindObjectNameByIdInDomain(
                                (DWORD)uid,
                                ppszDomainNames[i],
                                &pszNT4Name);
        if (!dwError)
        {
            goto cleanup;
        }
        else if (dwError != LSA_ERROR_NO_SUCH_USER &&
                 dwError != LSA_ERROR_NO_SUCH_USER_OR_GROUP)
        {
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    LSA_SAFE_FREE_STRING_ARRAY(ppszDomainNames);

    *ppszNT4Name = pszNT4Name;

    return dwError;

error:
    LSA_SAFE_FREE_STRING(pszNT4Name);
    goto cleanup;
}

DWORD
ADLdap_UnprovisionedFindObjectNameByIdInDomainHelper(    
    DWORD dwId,
    PCSTR pszPrimaryDomainName,
    PCSTR pszDomainName,
    PSTR* ppszNT4Name)
{
    DWORD dwError = 0;
    PSID pDomainSid = NULL;
    PSTR pszDomainSid = NULL;
    PSTR pszObjectSid = NULL;
    PSTR pszNT4Name = NULL;
    LSA_TRUST_DIRECTION dwTrustDirection = LSA_TRUST_DIRECTION_UNKNOWN;
    LSA_TRUST_MODE dwTrustMode = LSA_TRUST_MODE_UNKNOWN;    
    
    dwError = LsaDmQueryDomainInfo(pszDomainName,
                                   NULL,
                                   NULL,
                                   &pDomainSid,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &dwTrustDirection,
                                   &dwTrustMode,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL);
    if (LSA_ERROR_NO_SUCH_DOMAIN == dwError)
    {
        LSA_LOG_WARNING("Domain '%s' is unknown.", pszDomainName);
    }
    BAIL_ON_LSA_ERROR(dwError);
    
    if (dwTrustDirection == LSA_TRUST_DIRECTION_ZERO_WAY)
    {
        LSA_LOG_WARNING("Domain '%s' is not trusted.", pszDomainName);
        dwError =  LSA_ERROR_NO_SUCH_USER_OR_GROUP;
    }
    BAIL_ON_LSA_ERROR(dwError);
    
    if (dwTrustDirection == LSA_TRUST_DIRECTION_UNKNOWN ||
        dwTrustMode == LSA_TRUST_MODE_UNKNOWN)
    {
        LSA_LOG_WARNING("Domain '%s' has unknown trust.", pszDomainName);
        dwError = LSA_ERROR_NO_SUCH_USER_OR_GROUP;
    }
    BAIL_ON_LSA_ERROR(dwError);
    
    BAIL_ON_INVALID_POINTER(pDomainSid);
    
    dwError = AD_SidToString(
                 pDomainSid,
                 &pszDomainSid);
    BAIL_ON_LSA_ERROR(dwError);
    
    BAIL_ON_INVALID_STRING(pszDomainSid);
    
    dwError = UnprovisionedModeMakeLocalSID(
                 pszDomainSid,
                 dwId,
                 &pszObjectSid);
    BAIL_ON_LSA_ERROR(dwError);
    
    BAIL_ON_INVALID_STRING(pszObjectSid);
        
    dwError = LsaDmWrapNetLookupNameByObjectSid(
                 pszPrimaryDomainName,
                 pszObjectSid,
                 &pszNT4Name);
    BAIL_ON_LSA_ERROR(dwError);
    
    *ppszNT4Name = pszNT4Name;
    
cleanup:

    LSA_SAFE_FREE_MEMORY(pDomainSid);
    LSA_SAFE_FREE_STRING(pszDomainSid);
    LSA_SAFE_FREE_STRING(pszObjectSid);    

    return dwError;
    
error:

    *ppszNT4Name = NULL;
    
    LSA_SAFE_FREE_STRING(pszNT4Name);
    
    goto cleanup;
    
}

DWORD
ADLdap_UnprovisionedFindObjectNameByIdInDomain(
    DWORD dwId,
    PCSTR pszDomainName,
    PSTR* ppszNT4Name)
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    LSA_TRUST_DIRECTION dwTrustDirection = LSA_TRUST_DIRECTION_UNKNOWN;
    LSA_TRUST_MODE dwTrustMode = LSA_TRUST_MODE_UNKNOWN;
    PSTR* ppszDomainNames = NULL;
    DWORD dwCount = 0;
    PSTR pszNT4Name = NULL;
    INT i = 0;
    
    dwError = AD_DetermineTrustModeandDomainName(
                         pszDomainName,
                         &dwTrustDirection,
                         &dwTrustMode,
                         NULL,
                         NULL);
    BAIL_ON_LSA_ERROR(dwError);
    
    // If pszDomainName is primary domain
    // search all trusted domains whose TrustMode is LSA_TRUST_MODE_MY_FOREST
    if (dwTrustDirection == LSA_TRUST_DIRECTION_SELF)
    {
        // If this is an external trust, then our search space should be limited to the current domain
        // rather than the whole forest
        if (dwTrustMode == LSA_TRUST_MODE_EXTERNAL)
        {
            dwError = ADLdap_UnprovisionedFindObjectNameByIdInDomainHelper(
                             dwId,
                             gpADProviderData->szDomain,
                             pszDomainName,
                             &pszNT4Name);
        }
        else
        {
            dwError = LsaDmWrapEnumInMyForestTrustDomains(
                             &ppszDomainNames,
                             &dwCount);
            BAIL_ON_LSA_ERROR(dwError);
            
            for (i = 0; i < dwCount; i++)
            {
                
                dwError = ADLdap_UnprovisionedFindObjectNameByIdInDomainHelper(
                                 dwId,
                                 gpADProviderData->szDomain,
                                 ppszDomainNames[i],
                                 &pszNT4Name);
                if (!dwError)
                {
                    break;
                }
                else if (dwError == LSA_ERROR_NO_SUCH_USER_OR_GROUP)
                     {
                         continue;
                     }
                     else
                     {
                         BAIL_ON_LSA_ERROR(dwError);
                     }
            }
        }
        BAIL_ON_LSA_ERROR(dwError);
    }
    // Otherwise, the trusted domain is outside of my forest just search that particular domain
    else
    {
        dwError = ADLdap_UnprovisionedFindObjectNameByIdInDomainHelper(
                         dwId,
                         gpADProviderData->szDomain,
                         pszDomainName,
                         &pszNT4Name);
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppszNT4Name = pszNT4Name;

cleanup:

    LSA_SAFE_FREE_STRING_ARRAY(ppszDomainNames);

    return dwError;

error:

    *ppszNT4Name = NULL;
    
    LSA_SAFE_FREE_STRING(pszNT4Name);

    goto cleanup;
}

DWORD
ADLdap_FindGroupNameByAlias(
    PCSTR pszAlias,
    PSTR* ppszNT4Name
    )
{
    DWORD dwError = 0;
    PSTR pszNT4Name = NULL;
    PCSTR pszJoinedCellDN = gpADProviderData->cell.szCellDN;
    PCSTR pszJoinedDomainName = gpADProviderData->szDomain;

    BAIL_ON_INVALID_STRING(pszAlias);
    BAIL_ON_INVALID_STRING(pszJoinedCellDN);
    BAIL_ON_INVALID_STRING(pszJoinedDomainName);

    switch (gpADProviderData->dwDirectoryMode){
        case DEFAULT_MODE:
            dwError = ADLdap_DefaultFindGroupNameByAlias(
                             pszAlias,
                             &pszNT4Name);

            break;

        case CELL_MODE:
            dwError = ADLdap_CellFindGroupNameByAlias(
                             pszAlias,
                             pszJoinedDomainName,
                             pszJoinedCellDN,
                             &pszNT4Name);

            break;

        default:
                dwError = LSA_ERROR_NOT_HANDLED;
    }
    BAIL_ON_LSA_ERROR(dwError);

    *ppszNT4Name = pszNT4Name;

cleanup:

    return dwError;

error:
    *ppszNT4Name = NULL;

    LSA_SAFE_FREE_STRING(pszNT4Name);

    goto cleanup;
}

DWORD
ADLdap_DefaultFindGroupNameByAlias(
    PCSTR pszAlias,
    PSTR* ppszNT4Name
    )
{
    DWORD dwError = 0;
    PSTR pszNT4Name = NULL;
    PCSTR pszJoinedDomainName = gpADProviderData->szDomain;
    PSTR* ppszDomainNames = NULL;
    DWORD dwCount = 0;
    DWORD i = 0;

    dwError = ADLdap_DefaultFindGroupNameByAliasInDomain(
                            pszAlias,
                            pszJoinedDomainName,
                            &pszNT4Name);
    if (!dwError)
    {
        goto cleanup;
    }
    else if (dwError != LSA_ERROR_NO_SUCH_GROUP)
    {
        BAIL_ON_LSA_ERROR(dwError);
    }

    //if not found, search in two-way trusted domain
    dwError = LsaDmWrapEnumExtraTwoWayForestTrustDomains(&ppszDomainNames, &dwCount);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LSA_ERROR_NO_SUCH_GROUP;

    for (i = 0; i < dwCount; i++)
    {
        dwError = ADLdap_DefaultFindGroupNameByAliasInDomain(
                                pszAlias,
                                ppszDomainNames[i],
                                &pszNT4Name);
        if (!dwError)
        {
            goto cleanup;
        }
        else if (dwError != LSA_ERROR_NO_SUCH_GROUP)
        {
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    LSA_SAFE_FREE_STRING_ARRAY(ppszDomainNames);

    *ppszNT4Name = pszNT4Name;

    return dwError;

error:
    LSA_SAFE_FREE_STRING(pszNT4Name);
    goto cleanup;
}

DWORD
ADLdap_DefaultFindGroupNameByAliasInDomain(
    PCSTR pszAlias,
    PCSTR pszDomainName,
    PSTR* ppszNT4Name
    )
{
    DWORD dwError = 0;
    HANDLE hDirectory = (HANDLE)NULL;
    PSTR pszCellDN = NULL;
    PSTR pszNT4Name = NULL;
    ADConfigurationMode adConfMode = NonSchemaMode;
    PSTR pszDirectoryRoot = NULL;

    dwError = LsaDmWrapLdapOpenDirectoryDomain(pszDomainName, &hDirectory);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLdapConvertDomainToDN(
                    pszDomainName,
                    &pszDirectoryRoot);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAllocateStringPrintf(&pszCellDN,"CN=$LikewiseIdentityCell,%s", pszDirectoryRoot);
    BAIL_ON_LSA_ERROR(dwError);
    LSA_SAFE_FREE_STRING(pszDirectoryRoot);

  //search the primary domain (the one current joined to)
    dwError = ADGetConfigurationMode(
                         hDirectory,
                         pszCellDN,
                         &adConfMode);
    if (dwError == LSA_ERROR_INCOMPATIBLE_MODES_BETWEEN_TRUSTEDDOMAINS){
        dwError = LSA_ERROR_NO_SUCH_GROUP;
    }
    BAIL_ON_LSA_ERROR(dwError);

    switch (adConfMode){
    case SchemaMode:
        dwError = ADLdap_DefaultSchemaFindGroupNameByAlias(
                         pszAlias,
                         pszDomainName,
                         &pszNT4Name);

        break;
    case NonSchemaMode:
        dwError = ADLdap_DefaultNonSchemaFindGroupNameByAlias(
                                 pszAlias,
                                 pszDomainName,
                                 &pszNT4Name);
        break;

    default:
        dwError = LSA_ERROR_NOT_HANDLED;
    }
    BAIL_ON_LSA_ERROR(dwError);

    *ppszNT4Name = pszNT4Name;

cleanup:

    if (hDirectory != (HANDLE)NULL) {
        LsaLdapCloseDirectory(hDirectory);
    }

    LSA_SAFE_FREE_STRING(pszDirectoryRoot);
    LSA_SAFE_FREE_STRING(pszCellDN);


    return dwError;

error:

    *ppszNT4Name = NULL;

    LSA_SAFE_FREE_STRING(pszNT4Name);

    goto cleanup;
}

DWORD
ADLdap_CellFindGroupNameByAlias(
    PCSTR pszAlias,
    PCSTR pszDomainName,
    PCSTR pszCellDN,
    PSTR* ppszNT4Name
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    PSTR pszNT4Name = NULL;
    PDLINKEDLIST curr = NULL;


    dwError = ADLdap_CellFindGroupNameByAliasInOneCell(
                  pszAlias,
                  pszDomainName,
                  pszCellDN,
                  &pszNT4Name);
   
    if ((dwError == LSA_ERROR_NO_SUCH_GROUP || dwError == LSA_ERROR_NO_SUCH_USER_OR_GROUP)
         && gpADProviderData->pCellList)
    {
       dwError = LSA_ERROR_SUCCESS;
    }
    BAIL_ON_LSA_ERROR(dwError);
    
    if (!IsNullOrEmptyString(pszNT4Name))
       goto done;
    
    curr = gpADProviderData->pCellList;

    while (curr && curr->pItem)
    {
        PSTR pszCurrDomainName = NULL;

        dwError = LsaLdapConvertDNToDomain(
                         ((PAD_LINKED_CELL_INFO)curr->pItem)->pszCellDN,
                         &pszCurrDomainName);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = ADLdap_CellFindGroupNameByAliasInOneCell(
                           pszAlias,
                           pszCurrDomainName,
                           ((PAD_LINKED_CELL_INFO)curr->pItem)->pszCellDN,
                           &pszNT4Name);        

        LSA_SAFE_FREE_STRING(pszCurrDomainName);
        
        if ((dwError == LSA_ERROR_NO_SUCH_GROUP || dwError == LSA_ERROR_NO_SUCH_USER_OR_GROUP)
             && curr->pNext)
        {
           dwError = LSA_ERROR_SUCCESS;
        }
        BAIL_ON_LSA_ERROR(dwError);
        
        if (!IsNullOrEmptyString(pszNT4Name))
              break;
        
        curr = curr->pNext;
    }

done:

     *ppszNT4Name = pszNT4Name;

cleanup:

    return dwError;

error:

    *ppszNT4Name = NULL;

    LSA_SAFE_FREE_STRING(pszNT4Name);

    goto cleanup;
}

DWORD
ADLdap_CellFindGroupNameByAliasInOneCell(
    PCSTR pszAlias,
    PCSTR pszDomainName,
    PCSTR pszCellDN,
    PSTR* ppszNT4Name
    )
{
    DWORD dwError = 0;
    PSTR pszNT4Name = NULL;
    HANDLE hDirectory = (HANDLE)NULL;
    PSTR  pszQuery = NULL;
    ADConfigurationMode adConfMode = NonSchemaMode;
    PSTR pszEscapedAlias = NULL;

    dwError = LsaDmWrapLdapOpenDirectoryDomain(pszDomainName, &hDirectory);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADGetConfigurationMode(
                         hDirectory,
                         pszCellDN,
                         &adConfMode);
    if (dwError == LSA_ERROR_INCOMPATIBLE_MODES_BETWEEN_TRUSTEDDOMAINS){
        dwError = LSA_ERROR_NO_SUCH_GROUP;
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLdapEscapeString(
                &pszEscapedAlias,
                pszAlias);
    BAIL_ON_LSA_ERROR(dwError);

    switch (adConfMode){
        case SchemaMode:
                dwError = LsaAllocateStringPrintf(
                                 &pszQuery,
                                 "(&(objectClass=posixGroup)(keywords=objectClass=centerisLikewiseGroup)(displayName=%s))",
                                 pszEscapedAlias);

            break;

        case NonSchemaMode:
                dwError = LsaAllocateStringPrintf(
                                 &pszQuery,
                                 "(&(objectClass=serviceConnectionPoint)(keywords=objectClass=centerisLikewiseGroup)(keywords=displayName=%s))",
                                 pszEscapedAlias);

            break;

        default:
            dwError = LSA_ERROR_INVALID_PARAMETER;
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADLdap_CellFindGroupNameHelper(
                     hDirectory,
                     pszDomainName,
                     pszCellDN,
                     pszQuery,
                     &pszNT4Name);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszNT4Name = pszNT4Name;

cleanup:

    if (hDirectory != (HANDLE)NULL) {
        LsaLdapCloseDirectory(hDirectory);
    }

    LSA_SAFE_FREE_STRING(pszQuery);

    LSA_SAFE_FREE_STRING(pszEscapedAlias);

    return dwError;

error:

    *ppszNT4Name = NULL;

    LSA_SAFE_FREE_STRING(pszNT4Name);

    goto cleanup;
}

DWORD
ADLdap_CellFindGroupNameById(
    gid_t gid,
    PCSTR pszDomainName,
    PCSTR pszCellDN,
    PSTR* ppszNT4Name
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    PSTR pszNT4Name = NULL;
    PDLINKEDLIST curr = NULL;


    dwError = ADLdap_CellFindGroupNameByIdInOneCell(
                  gid,
                  pszDomainName,
                  pszCellDN,
                  &pszNT4Name);
     
    if ((dwError == LSA_ERROR_NO_SUCH_GROUP || dwError == LSA_ERROR_NO_SUCH_USER_OR_GROUP)
         && gpADProviderData->pCellList)
    {
       dwError = LSA_ERROR_SUCCESS;
    }
    BAIL_ON_LSA_ERROR(dwError);
    
    if (!IsNullOrEmptyString(pszNT4Name))
       goto done;

    curr = gpADProviderData->pCellList;

    while (curr && curr->pItem){
        PSTR pszCurrDomainName = NULL;

        dwError = LsaLdapConvertDNToDomain(
                         ((PAD_LINKED_CELL_INFO)curr->pItem)->pszCellDN,
                         &pszCurrDomainName);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = ADLdap_CellFindGroupNameByIdInOneCell(
                           gid,
                           pszCurrDomainName,
                           ((PAD_LINKED_CELL_INFO)curr->pItem)->pszCellDN,
                           &pszNT4Name);
        BAIL_ON_LSA_ERROR(dwError);

        LSA_SAFE_FREE_STRING(pszCurrDomainName);
        
        if ((dwError == LSA_ERROR_NO_SUCH_GROUP || dwError == LSA_ERROR_NO_SUCH_USER_OR_GROUP)
             && curr->pNext)
        {
           dwError = LSA_ERROR_SUCCESS;
        }
        BAIL_ON_LSA_ERROR(dwError);
        
        if (!IsNullOrEmptyString(pszNT4Name))
            break;
        
        curr = curr->pNext;
    }

done:

     *ppszNT4Name = pszNT4Name;

cleanup:

    return dwError;

error:

    *ppszNT4Name = NULL;

    LSA_SAFE_FREE_STRING(pszNT4Name);

    goto cleanup;
}

DWORD
ADLdap_CellFindGroupNameByIdInOneCell(
    gid_t gid,
    PCSTR pszDomainName,
    PCSTR pszCellDN,
    PSTR* ppszNT4Name
    )
{
    DWORD dwError = 0;
    PSTR pszNT4Name = NULL;
    HANDLE hDirectory = (HANDLE)NULL;
    PSTR pszQuery = NULL;
    ADConfigurationMode adConfMode = NonSchemaMode;

    dwError = LsaDmWrapLdapOpenDirectoryDomain(pszDomainName, &hDirectory);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADGetConfigurationMode(
                         hDirectory,
                         pszCellDN,
                         &adConfMode);
    if (dwError == LSA_ERROR_INCOMPATIBLE_MODES_BETWEEN_TRUSTEDDOMAINS){
        dwError = LSA_ERROR_NO_SUCH_GROUP;
    }
    BAIL_ON_LSA_ERROR(dwError);

    switch (adConfMode){
        case SchemaMode:
                dwError = LsaAllocateStringPrintf(
                                 &pszQuery,
                                 "(&(objectClass=posixGroup)(keywords=objectClass=centerisLikewiseGroup)(gidNumber=%d))",
                                 gid);

            break;

        case NonSchemaMode:
                dwError = LsaAllocateStringPrintf(
                                 &pszQuery,
                                 "(&(objectClass=serviceConnectionPoint)(keywords=objectClass=centerisLikewiseGroup)(keywords=gidNumber=%d))",
                                 gid);

            break;

        default:
            dwError = LSA_ERROR_INVALID_PARAMETER;
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADLdap_CellFindGroupNameHelper(
                     hDirectory,
                     pszDomainName,
                     pszCellDN,
                     pszQuery,
                     &pszNT4Name);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszNT4Name = pszNT4Name;

cleanup:

    if (hDirectory != (HANDLE)NULL) {
        LsaLdapCloseDirectory(hDirectory);
    }

    LSA_SAFE_FREE_STRING(pszQuery);

    return dwError;

error:

    *ppszNT4Name = NULL;

    LSA_SAFE_FREE_STRING(pszNT4Name);

    goto cleanup;
}

DWORD
ADLdap_CellFindGroupNameHelper(
    HANDLE hDirectory,
    PCSTR pszDomainName,
    PCSTR pszCellDN,
    PCSTR pszQuery,
    PSTR* ppszNT4Name)
{
    DWORD dwError = 0;
    PSTR pszNT4Name = NULL;
    PSTR pszGroupsContainerDN = NULL;
    LDAP* pLd = NULL;
    LDAPMessage *pCellGroupMessage = NULL;
    DWORD dwCount = 0;
    PSTR* ppszValues = NULL;
    DWORD dwNumValues = 0;
    DWORD iValue = 0;
    PCSTR pszObjectSID = NULL;
    BOOLEAN bValidADEntry = FALSE;
    PSTR szCellGroupAttributeList[] =
            {
                AD_LDAP_KEYWORDS_TAG,
                NULL
            };
    PSTR* ppszDomainNames = NULL;
    DWORD i = 0;

    if (hDirectory == (HANDLE)NULL){
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pLd = LsaLdapGetSession(hDirectory);

    dwError = LsaAllocateStringPrintf(
                     &pszGroupsContainerDN,
                     "CN=Groups,%s",
                     pszCellDN);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLdapDirectorySearch(
               hDirectory,
               pszGroupsContainerDN,
               LDAP_SCOPE_ONELEVEL,
               pszQuery,
               szCellGroupAttributeList,
               &pCellGroupMessage);
    BAIL_ON_LSA_ERROR(dwError);

    dwCount = ldap_count_entries(
                   pLd,
                   pCellGroupMessage);
    if (dwCount < 0) {
           dwError = LSA_ERROR_LDAP_ERROR;
    } else if (dwCount == 0) {
           dwError = LSA_ERROR_NO_SUCH_GROUP;
    } else if (dwCount > 1) {
           dwError = LSA_ERROR_DUPLICATE_GROUPNAME;
    }
    BAIL_ON_LSA_ERROR(dwError);

    //Confirm the entry we obtain from AD is valid by retrieving its DN
    dwError = LsaLdapIsValidADEntry(
                    hDirectory,
                    pCellGroupMessage,
                    &bValidADEntry);
    BAIL_ON_LSA_ERROR(dwError);

    if (!bValidADEntry){
        dwError = LSA_ERROR_LDAP_FAILED_GETDN;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaLdapGetStrings(hDirectory,
                                pCellGroupMessage,
                                AD_LDAP_KEYWORDS_TAG,
                                &ppszValues,
                                &dwNumValues);
    BAIL_ON_LSA_ERROR(dwError);

    for (iValue = 0; iValue < dwNumValues; iValue++)
    {
        if (!strncasecmp(ppszValues[iValue], "backLink=", sizeof("backLink=")-1))
        {
           pszObjectSID = ppszValues[iValue] + sizeof("backLink=") - 1;
           break;
        }
    }
    if (IsNullOrEmptyString(pszObjectSID)) {
        dwError = LSA_ERROR_INVALID_SID;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaDmWrapNetLookupNameByObjectSid(
                    pszDomainName,
                    pszObjectSID,
                    &pszNT4Name);
    BAIL_ON_LSA_ERROR(dwError);

    if (dwError == LSA_ERROR_NO_SUCH_USER_OR_GROUP){

        dwError = LsaDmWrapEnumExtraForestTrustDomains(&ppszDomainNames, &dwCount);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LSA_ERROR_NO_SUCH_USER_OR_GROUP;

        for (i = 0; i < dwCount; i++)
        {
            //find a two-way across forest trust
            dwError = LsaDmWrapNetLookupNameByObjectSid(
                            pszDomainName,
                            pszObjectSID,
                            &pszNT4Name);
            BAIL_ON_LSA_ERROR(dwError);

            if (!dwError)
            {
                break;
            }
            else if (dwError != LSA_ERROR_NO_SUCH_USER_OR_GROUP)
            {
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
    }
    BAIL_ON_LSA_ERROR(dwError);

    BAIL_ON_INVALID_STRING(pszNT4Name);

    *ppszNT4Name = pszNT4Name;

cleanup:

    if (pCellGroupMessage) {
        ldap_msgfree(pCellGroupMessage);
    }

    LSA_SAFE_FREE_STRING(pszGroupsContainerDN);
    LSA_SAFE_FREE_STRING_ARRAY(ppszDomainNames);

    if (ppszValues) {
        LsaFreeStringArray(ppszValues, dwNumValues);
    }

    return dwError;

error:

    *ppszNT4Name = NULL;

    LSA_SAFE_FREE_STRING(pszNT4Name);

    goto cleanup;
}

DWORD
ADLdap_DefaultSchemaFindGroupNameByAlias(
    PCSTR pszAlias,
    PCSTR pszRootDomainName,
    PSTR* ppszNT4Name
    )
{
    DWORD dwError = 0;
    PSTR pszNT4Name = NULL;
    HANDLE hGCDirectory = (HANDLE)NULL;
    PSTR pszQuery = NULL;
    PSTR pszEscapedAlias = NULL;

    BAIL_ON_INVALID_STRING(pszRootDomainName);

    dwError = LsaDmWrapLdapOpenDirectoryGc(pszRootDomainName, &hGCDirectory);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLdapEscapeString(
                &pszEscapedAlias,
                pszAlias);
    BAIL_ON_LSA_ERROR(dwError);

    //Search in root node's GC for object's DN, sAMAccountName
    //Given group's alias stored in displayName
    dwError = LsaAllocateStringPrintf(
                     &pszQuery,
                     "(&(objectClass=Group)(displayName=%s))",
                      pszEscapedAlias);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADLdap_DefaultSchemaFindGroupNameHelper(
                       hGCDirectory,
                       pszQuery,
                       &pszNT4Name);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszNT4Name = pszNT4Name;


cleanup:
    if (hGCDirectory != (HANDLE)NULL) {
        LsaLdapCloseDirectory(hGCDirectory);
    }

    LSA_SAFE_FREE_STRING(pszQuery);

    LSA_SAFE_FREE_STRING(pszEscapedAlias);

    return dwError;

error:

    *ppszNT4Name = NULL;

    LSA_SAFE_FREE_STRING(pszNT4Name);

    goto cleanup;
}

DWORD
ADLdap_DefaultSchemaFindGroupNameById(
    gid_t gid,
    PCSTR pszRootDomainName,
    PSTR* ppszNT4Name
    )
{
    DWORD dwError = 0;
    PSTR pszNT4Name = NULL;
    HANDLE hGCDirectory = (HANDLE)NULL;
    PSTR pszQuery = NULL;

    BAIL_ON_INVALID_STRING(pszRootDomainName);

    dwError = LsaDmWrapLdapOpenDirectoryGc(pszRootDomainName, &hGCDirectory);
    BAIL_ON_LSA_ERROR(dwError);

    //Search in root node's GC for object's DN, sAMAccountName
    //Given group's gid stored in gidNumber
    dwError = LsaAllocateStringPrintf(
                     &pszQuery,
                     "(&(objectClass=Group)(gidNumber=%d))",
                      gid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADLdap_DefaultSchemaFindGroupNameHelper(
                       hGCDirectory,
                       pszQuery,
                       &pszNT4Name);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszNT4Name = pszNT4Name;


cleanup:
    if (hGCDirectory != (HANDLE)NULL) {
        LsaLdapCloseDirectory(hGCDirectory);
    }

    LSA_SAFE_FREE_STRING(pszQuery);

    return dwError;

error:

    *ppszNT4Name = NULL;

    LSA_SAFE_FREE_STRING(pszNT4Name);

    goto cleanup;
}

DWORD
ADLdap_DefaultSchemaFindGroupNameHelper(
    HANDLE hGCDirectory,
    PCSTR pszQuery,
    PSTR* ppszNT4Name
    )
{
    DWORD dwError = 0;
    PSTR pszNT4Name = NULL;
    LDAPMessage *pMessage = NULL;
    LDAP* pGCLd = NULL;
    PSTR pszGroupDN = NULL;
    PSTR pszGroupDomainName = NULL;
    PSTR pszGroupNetBiosName = NULL;
    PSTR pszGroupSamaccountName = NULL;
    DWORD dwCount = 0;
    PSTR szGCObjectAttributeList[] =
                    {AD_LDAP_SAM_NAME_TAG,
                     NULL
                    };

    if (hGCDirectory == (HANDLE)NULL){
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pGCLd = LsaLdapGetSession(hGCDirectory);

    dwError = LsaLdapDirectorySearch(
                         hGCDirectory,
                         "",
                         LDAP_SCOPE_SUBTREE,
                         pszQuery,
                         szGCObjectAttributeList,
                         &pMessage);
    BAIL_ON_LSA_ERROR(dwError);

    dwCount = ldap_count_entries(
                     pGCLd,
                     pMessage);
    if (dwCount < 0) {
       dwError = LSA_ERROR_LDAP_ERROR;
    } else if (dwCount == 0) {
       dwError = LSA_ERROR_NO_SUCH_GROUP;
    } else if (dwCount > 1) {
       dwError = LSA_ERROR_DUPLICATE_GROUPNAME;
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLdapGetDN(
                   hGCDirectory,
                   pMessage,
                   &pszGroupDN);
    BAIL_ON_LSA_ERROR(dwError);
    BAIL_ON_INVALID_STRING(pszGroupDN);

    dwError = LsaLdapConvertDNToDomain(
                    pszGroupDN,
                    &pszGroupDomainName);
    BAIL_ON_LSA_ERROR(dwError);
    BAIL_ON_INVALID_STRING(pszGroupDomainName);

    dwError = LsaLdapGetString(
               hGCDirectory,
               pMessage,
               AD_LDAP_SAM_NAME_TAG,
               &pszGroupSamaccountName);
    BAIL_ON_LSA_ERROR(dwError);
    BAIL_ON_INVALID_STRING(pszGroupSamaccountName);

    dwError = LsaDmWrapGetDomainName(
                pszGroupDomainName,
                NULL,
                &pszGroupNetBiosName);
    BAIL_ON_LSA_ERROR(dwError);

    LsaStrToUpper(pszGroupNetBiosName);
    LsaStrToLower(pszGroupSamaccountName);

    dwError = LsaAllocateStringPrintf(
                    &pszNT4Name,
                    "%s\\%s",
                    pszGroupNetBiosName,
                    pszGroupSamaccountName);
    BAIL_ON_LSA_ERROR(dwError);

   *ppszNT4Name = pszNT4Name;

cleanup:

       if (pMessage) {
           ldap_msgfree(pMessage);
       }

       LSA_SAFE_FREE_STRING(pszGroupDN);
       LSA_SAFE_FREE_STRING(pszGroupDomainName);
       LSA_SAFE_FREE_STRING(pszGroupNetBiosName);
       LSA_SAFE_FREE_STRING(pszGroupSamaccountName);

       return dwError;

   error:

       *ppszNT4Name = NULL;

       LSA_SAFE_FREE_STRING(pszNT4Name);

       goto cleanup;

}


DWORD
ADLdap_DefaultNonSchemaFindGroupNameByAlias(
    PCSTR pszAlias,
    PCSTR pszRootDomainName,
    PSTR* ppszNT4Name
    )
{
    DWORD dwError = 0;
    PSTR pszNT4Name = NULL;
    HANDLE hGCDirectory = (HANDLE)NULL;
    PSTR pszQuery = NULL;
    PSTR pszEscapedAlias = NULL;

    BAIL_ON_INVALID_STRING(pszRootDomainName);

    dwError = LsaDmWrapLdapOpenDirectoryGc(pszRootDomainName, &hGCDirectory);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLdapEscapeString(
                &pszEscapedAlias,
                pszAlias);
    BAIL_ON_LSA_ERROR(dwError);

    //Search in root node's GC for object's DN, sAMAccountName
    // Given group's alias stored in displayName
    dwError = LsaAllocateStringPrintf(
                     &pszQuery,
                     "(&(objectClass=serviceConnectionPoint)(keywords=objectClass=centerisLikewiseGroup)(keywords=displayName=%s))",
                      pszEscapedAlias);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADLdap_DefaultNonSchemaFindGroupNameHelper(
                       hGCDirectory,
                       pszRootDomainName,
                       pszQuery,
                       &pszNT4Name);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszNT4Name = pszNT4Name;


cleanup:
    if (hGCDirectory != (HANDLE)NULL) {
        LsaLdapCloseDirectory(hGCDirectory);
    }

    LSA_SAFE_FREE_STRING(pszQuery);

    LSA_SAFE_FREE_STRING(pszEscapedAlias);

    return dwError;

error:

    *ppszNT4Name = NULL;

    LSA_SAFE_FREE_STRING(pszNT4Name);

    goto cleanup;

}

DWORD
ADLdap_DefaultNonSchemaFindGroupNameById(
    gid_t gid,
    PCSTR pszRootDomainName,
    PSTR* ppszNT4Name
    )
{
    DWORD dwError = 0;
    PSTR pszNT4Name = NULL;
    HANDLE hGCDirectory = (HANDLE)NULL;
    PSTR pszQuery = NULL;

    BAIL_ON_INVALID_STRING(pszRootDomainName);

    dwError = LsaDmWrapLdapOpenDirectoryGc(pszRootDomainName, &hGCDirectory);
    BAIL_ON_LSA_ERROR(dwError);

    //Search in root node's GC for object's DN, sAMAccountName
    //Given group's gid stored in gidNumber
    dwError = LsaAllocateStringPrintf(
                     &pszQuery,
                     "(&(objectClass=serviceConnectionPoint)(keywords=objectClass=centerisLikewiseGroup)(keywords=gidNumber=%d))",
                      gid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADLdap_DefaultNonSchemaFindGroupNameHelper(
                       hGCDirectory,
                       pszRootDomainName,
                       pszQuery,
                       &pszNT4Name);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszNT4Name = pszNT4Name;


cleanup:
    if (hGCDirectory != (HANDLE)NULL) {
        LsaLdapCloseDirectory(hGCDirectory);
    }

    LSA_SAFE_FREE_STRING(pszQuery);

    return dwError;

error:

    *ppszNT4Name = NULL;

    LSA_SAFE_FREE_STRING(pszNT4Name);

    goto cleanup;
}

DWORD
ADLdap_DefaultNonSchemaFindGroupNameHelper(
    HANDLE hGCDirectory,
    PCSTR  pszRootDomainName,
    PCSTR  pszQuery,
    PSTR* ppszNT4Name
    )
{
    DWORD dwError = 0;
    PSTR pszNT4Name = NULL;
    PSTR szCellGroupAttributeList[] =
            {
                AD_LDAP_KEYWORDS_TAG,
                NULL
            };
    LDAP* pGCLd = NULL;
    LDAPMessage *pCellGroupMessage = NULL;
    PSTR pszGroupDN = NULL;
    PSTR pszGroupDomainName = NULL;
    PSTR pszGroupNetBiosName = NULL;
    PSTR pszGroupSamaccountName = NULL;
    DWORD dwCount = 0;
    PSTR* ppszValues = NULL;
    DWORD dwNumValues = 0;
    DWORD iValue = 0;
    PCSTR pszObjectSID = NULL;
    PSTR pszRootDomainDN = NULL;
    PSTR pszCellDN = NULL;
    LDAPMessage *pGroupMessage = NULL;
    LDAPMessage *pFoundGroupMessage = NULL;
    PSTR pGroupPseudoDN = NULL;
    BOOLEAN bFound = FALSE;
    PSTR pszGroupName = NULL;    


    if (hGCDirectory == (HANDLE)NULL){
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pGCLd = LsaLdapGetSession(hGCDirectory);

    dwError = LsaLdapConvertDomainToDN(
                    pszRootDomainName,
                    &pszRootDomainDN);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAllocateStringPrintf(
                 &pszCellDN,
                 "CN=$LikewiseIdentityCell,%s",
                 pszRootDomainDN);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLdapDirectorySearch(
                      hGCDirectory,
                      "",
                      LDAP_SCOPE_SUBTREE,
                      pszQuery,
                      szCellGroupAttributeList,
                      &pCellGroupMessage);
    BAIL_ON_LSA_ERROR(dwError);

    dwCount = ldap_count_entries(
                      pGCLd,
                      pCellGroupMessage);
    if (dwCount < 0) {
       dwError = LSA_ERROR_LDAP_ERROR;
    } else if (dwCount == 0) {
       dwError = LSA_ERROR_NO_SUCH_GROUP;
    }
    BAIL_ON_LSA_ERROR(dwError);

    pGroupMessage = ldap_first_entry(
                           pGCLd,
                           pCellGroupMessage);
    BAIL_ON_LSA_ERROR(dwError);

    while (pGroupMessage)
    {
        LSA_SAFE_FREE_STRING(pGroupPseudoDN);

        dwError = LsaLdapGetDN(
                      hGCDirectory,
                      pGroupMessage,
                      &pGroupPseudoDN);
        BAIL_ON_LSA_ERROR(dwError);

        LsaStrToUpper(pGroupPseudoDN);

        if (strstr(pGroupPseudoDN, ",CN=$LIKEWISEIDENTITYCELL,DC="))
        {
            if (!bFound)
            {
                bFound = TRUE;
                pFoundGroupMessage = pGroupMessage;
                dwError = LsaAllocateString(
                                 pGroupPseudoDN,
                                 &pszGroupName);
                BAIL_ON_LSA_ERROR(dwError);
            }
            else
            {
                dwError = LSA_ERROR_DUPLICATE_GROUPNAME;
                if (AD_EventlogEnabled())
                {
                    LsaSrvLogDuplicateObjectFoundEvent(
                                    pszGroupName,
                                    pGroupPseudoDN,
                                    gpszADProviderName,
                                    dwError);
                }
                BAIL_ON_LSA_ERROR(dwError);
            }            
        }

        pGroupMessage = ldap_next_entry(
                                  pGCLd,
                                  pGroupMessage);
    }

    if (!bFound || !pFoundGroupMessage){
        dwError = LSA_ERROR_NO_SUCH_GROUP;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaLdapGetStrings(hGCDirectory,
                                pFoundGroupMessage,
                                AD_LDAP_KEYWORDS_TAG,
                                &ppszValues,
                                &dwNumValues);
    BAIL_ON_LSA_ERROR(dwError);

    for (iValue = 0; iValue < dwNumValues; iValue++)
    {
        if (!strncasecmp(ppszValues[iValue], "backLink=", sizeof("backLink=")-1))
        {
           pszObjectSID = ppszValues[iValue] + sizeof("backLink=") - 1;
           break;
        }
    }
    if (IsNullOrEmptyString(pszObjectSID)) {
        dwError = LSA_ERROR_INVALID_SID;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError =  ADLdap_GetGCObjectInfoBySid(
                      pszRootDomainName,
                      pszObjectSID,
                      &pszGroupDN,
                      &pszGroupDomainName,
                      &pszGroupSamaccountName);
    BAIL_ON_LSA_ERROR(dwError);

    BAIL_ON_INVALID_STRING(pszGroupDN);
    BAIL_ON_INVALID_STRING(pszGroupDomainName);
    BAIL_ON_INVALID_STRING(pszGroupSamaccountName);

    dwError = LsaDmWrapGetDomainName(
                pszGroupDomainName,
                NULL,
                &pszGroupNetBiosName);
    BAIL_ON_LSA_ERROR(dwError);

    LsaStrToUpper(pszGroupNetBiosName);
    LsaStrToLower(pszGroupSamaccountName);

    dwError = LsaAllocateStringPrintf(
                     &pszNT4Name,
                     "%s\\%s",
                     pszGroupNetBiosName,
                     pszGroupSamaccountName);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszNT4Name = pszNT4Name;

cleanup:

    if (pCellGroupMessage) {
        ldap_msgfree(pCellGroupMessage);
    }

    LSA_SAFE_FREE_STRING(pszGroupDN);
    LSA_SAFE_FREE_STRING(pGroupPseudoDN);
    LSA_SAFE_FREE_STRING(pszGroupName);
    LSA_SAFE_FREE_STRING(pszGroupDomainName);
    LSA_SAFE_FREE_STRING(pszGroupNetBiosName);
    LSA_SAFE_FREE_STRING(pszGroupSamaccountName);

    LSA_SAFE_FREE_STRING(pszRootDomainDN);
    LSA_SAFE_FREE_STRING(pszCellDN);

    if (ppszValues) {
        LsaFreeStringArray(ppszValues, dwNumValues);
    }

    return dwError;

error:

    *ppszNT4Name = NULL;

    LSA_SAFE_FREE_STRING(pszNT4Name);

    goto cleanup;
}

DWORD
ADLdap_FindGroupNameById(
    gid_t gid,
    PSTR* ppszNT4Name)
{
    DWORD dwError = 0;
    PSTR pszNT4Name = NULL;
    PCSTR pszJoinedCellDN = gpADProviderData->cell.szCellDN;
    PCSTR pszJoinedDomainName = gpADProviderData->szDomain;

    BAIL_ON_INVALID_STRING(pszJoinedDomainName);

    switch (gpADProviderData->dwDirectoryMode){
        case DEFAULT_MODE:
            BAIL_ON_INVALID_STRING(pszJoinedCellDN);

            dwError = ADLdap_DefaultFindGroupNameById(
                             gid,
                             &pszNT4Name);

            break;

        case CELL_MODE:
            BAIL_ON_INVALID_STRING(pszJoinedCellDN);

            dwError = ADLdap_CellFindGroupNameById(
                             gid,
                             pszJoinedDomainName,
                             pszJoinedCellDN,
                             &pszNT4Name);

            break;

        case UNPROVISIONED_MODE:
            dwError = ADLdap_UnprovisionedFindGroupNameById(
                             gid,
                             &pszNT4Name);

            break;


        default:
                dwError = LSA_ERROR_NOT_HANDLED;
    }
    BAIL_ON_LSA_ERROR(dwError);

    *ppszNT4Name = pszNT4Name;

cleanup:

    return dwError;

error:
    *ppszNT4Name = NULL;

    LSA_SAFE_FREE_STRING(pszNT4Name);

    goto cleanup;
}

DWORD
ADLdap_DefaultFindGroupNameById(
    gid_t gid,
    PSTR* ppszNT4Name
    )
{
    DWORD dwError = 0;
    PSTR pszNT4Name = NULL;
    PCSTR pszJoinedDomainName = gpADProviderData->szDomain;
    PSTR* ppszDomainNames = NULL;
    DWORD dwCount = 0;
    DWORD i = 0;

    dwError = ADLdap_DefaultFindGroupNameByIdInDomain(
                            gid,
                            pszJoinedDomainName,
                            &pszNT4Name);
    if (!dwError)
    {
        goto cleanup;
    }
    else if (dwError != LSA_ERROR_NO_SUCH_GROUP)
    {
        BAIL_ON_LSA_ERROR(dwError);
    }

    //if not found, search in two-way trusted domain
    dwError = LsaDmWrapEnumExtraTwoWayForestTrustDomains(&ppszDomainNames, &dwCount);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LSA_ERROR_NO_SUCH_GROUP;

    for (i = 0; i < dwCount; i++)
    {
        dwError = ADLdap_DefaultFindGroupNameByIdInDomain(
                                gid,
                                ppszDomainNames[i],
                                &pszNT4Name);
        if (!dwError)
        {
            goto cleanup;
        }
        else if (dwError != LSA_ERROR_NO_SUCH_GROUP)
        {
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    LSA_SAFE_FREE_STRING_ARRAY(ppszDomainNames);

    *ppszNT4Name = pszNT4Name;

    return dwError;

error:
    LSA_SAFE_FREE_STRING(pszNT4Name);
    goto cleanup;
}

DWORD
ADLdap_DefaultFindGroupNameByIdInDomain(
    gid_t gid,
    PCSTR pszDomainName,
    PSTR* ppszNT4Name
    )
{
    DWORD dwError = 0;
    HANDLE hDirectory = (HANDLE)NULL;
    PSTR pszCellDN = NULL;
    PSTR pszNT4Name = NULL;
    ADConfigurationMode adConfMode = NonSchemaMode;
    PSTR pszDirectoryRoot = NULL;

    dwError = LsaDmWrapLdapOpenDirectoryDomain(pszDomainName, &hDirectory);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLdapConvertDomainToDN(
                    pszDomainName,
                    &pszDirectoryRoot);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAllocateStringPrintf(&pszCellDN,"CN=$LikewiseIdentityCell,%s", pszDirectoryRoot);
    BAIL_ON_LSA_ERROR(dwError);
    LSA_SAFE_FREE_STRING(pszDirectoryRoot);

  //search the primary domain (the one current joined to)
    dwError = ADGetConfigurationMode(
                         hDirectory,
                         pszCellDN,
                         &adConfMode);
    if (dwError == LSA_ERROR_INCOMPATIBLE_MODES_BETWEEN_TRUSTEDDOMAINS){
        dwError = LSA_ERROR_NO_SUCH_GROUP;
    }
    BAIL_ON_LSA_ERROR(dwError);

    switch (adConfMode){
    case SchemaMode:
        dwError = ADLdap_DefaultSchemaFindGroupNameById(
                                 gid,
                                 pszDomainName,
                                 &pszNT4Name);

        break;
    case NonSchemaMode:
        dwError = ADLdap_DefaultNonSchemaFindGroupNameById(
                                 gid,
                                 pszDomainName,
                                 &pszNT4Name);
        break;

    default:
        dwError = LSA_ERROR_NOT_HANDLED;
    }
    BAIL_ON_LSA_ERROR(dwError);

    *ppszNT4Name = pszNT4Name;

cleanup:

    if (hDirectory != (HANDLE)NULL) {
        LsaLdapCloseDirectory(hDirectory);
    }

    LSA_SAFE_FREE_STRING(pszDirectoryRoot);
    LSA_SAFE_FREE_STRING(pszCellDN);


    return dwError;

error:

    *ppszNT4Name = NULL;

    LSA_SAFE_FREE_STRING(pszNT4Name);

    goto cleanup;
}

DWORD
ADLdap_UnprovisionedFindGroupNameById(
    gid_t gid,
    PSTR* ppszNT4Name)
{
    DWORD dwError = 0;
    PSTR pszNT4Name = NULL;
    PCSTR pszJoinedDomainName = gpADProviderData->szDomain;
    PSTR* ppszDomainNames = NULL;
    DWORD dwCount = 0;
    DWORD i = 0;

    dwError = ADLdap_UnprovisionedFindObjectNameByIdInDomain(
                          (DWORD)gid,
                          pszJoinedDomainName,
                          &pszNT4Name);
    if (!dwError)
    {
        goto cleanup;
    }
    else if (dwError != LSA_ERROR_NO_SUCH_GROUP &&
             dwError != LSA_ERROR_NO_SUCH_USER_OR_GROUP)
    {
       BAIL_ON_LSA_ERROR(dwError);
    }

    //if not found, search in two-way and one-way trusted domain
    dwError = LsaDmWrapEnumExtraForestTrustDomains(&ppszDomainNames, &dwCount);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LSA_ERROR_NO_SUCH_GROUP;

    for (i = 0; i < dwCount; i++)
    {
        dwError = ADLdap_UnprovisionedFindObjectNameByIdInDomain(
                                (DWORD)gid,
                                ppszDomainNames[i],
                                &pszNT4Name);
        if (!dwError)
        {
            goto cleanup;
        }
        else if (dwError != LSA_ERROR_NO_SUCH_GROUP &&
                 dwError != LSA_ERROR_NO_SUCH_USER_OR_GROUP)
        {
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    LSA_SAFE_FREE_STRING_ARRAY(ppszDomainNames);

    *ppszNT4Name = pszNT4Name;

    return dwError;

error:
    LSA_SAFE_FREE_STRING(pszNT4Name);
    goto cleanup;
}

DWORD
ADLdap_FindUserSidDNById(
    uid_t uid,
    PSTR* ppszUserSid,
    PSTR* ppszUserDN)
{
    DWORD dwError = 0;
    PSTR pszUserSid = NULL;
    PSTR pszUserDN = NULL;
    PSTR pszObjectDomainName = NULL;
    PSTR pszObjectSamaccountName = NULL;
    PSTR pszNT4Name = NULL;
    PLSA_LOGIN_NAME_INFO pUserNameInfo = NULL;

    dwError = ADLdap_FindUserNameById(
                     uid,
                     &pszNT4Name);
    BAIL_ON_LSA_ERROR(dwError);
    BAIL_ON_INVALID_STRING(pszNT4Name);

    dwError = LsaDmWrapNetLookupObjectSidByName(
                gpADProviderData->szDomain,
                pszNT4Name,
                &pszUserSid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaCrackDomainQualifiedName(
                        pszNT4Name,
                        NULL,
                        &pUserNameInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_CanonicalizeDomainsInCrackedNameInfo(pUserNameInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADLdap_GetGCObjectInfoBySid(
                     pUserNameInfo->pszFullDomainName,
                     pszUserSid,
                     &pszUserDN,
                     &pszObjectDomainName,
                     &pszObjectSamaccountName);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszUserSid = pszUserSid;
    *ppszUserDN = pszUserDN;

cleanup:
    LSA_SAFE_FREE_STRING(pszNT4Name);
    LSA_SAFE_FREE_STRING(pszObjectDomainName);
    LSA_SAFE_FREE_STRING(pszObjectSamaccountName);
    
    if (pUserNameInfo)
    {
        LsaFreeNameInfo(pUserNameInfo);
    }

    return dwError;

error:
    *ppszUserSid = NULL;
    *ppszUserDN = NULL;

    LSA_SAFE_FREE_STRING(pszUserSid);
    LSA_SAFE_FREE_STRING(pszUserDN);

    goto cleanup;
}

DWORD
ADLdap_GetMapTypeString(
    LsaNSSMapType dwMapType,
    PSTR*         ppszMapType
    )
{
    DWORD dwError = 0;
    PCSTR pszGroups = "netgroups";
    PCSTR pszServices = "services";
    PCSTR pszMounts = "automounts";
    PCSTR pszId = NULL;
    PSTR pszMapType = NULL;

    switch(dwMapType)
    {
        case LSA_NSS_ARTEFACT_TYPE_NETGROUP:

            pszId = pszGroups;
            break;

        case LSA_NSS_ARTEFACT_TYPE_SERVICE:

             pszId = pszServices;
             break;

        case LSA_NSS_ARTEFACT_TYPE_MOUNT:

            pszId = pszMounts;
            break;

        default:

            dwError = LSA_ERROR_INVALID_NSS_ARTEFACT_TYPE;
            BAIL_ON_LSA_ERROR(dwError);

            break;
    }

    dwError = LsaAllocateString(
                  pszId,
                  &pszMapType);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszMapType = pszMapType;

cleanup:

    return(dwError);

error:

    *ppszMapType = NULL;

    goto cleanup;
}

DWORD
ADLdap_GetUserLoginInfo(
    uid_t   uid,
    PLSA_LOGIN_NAME_INFO* ppLoginInfo
    )    
{
    DWORD dwError = 0;
    PLSA_LOGIN_NAME_INFO pLoginInfo = NULL;
    PSTR pszNT4Name = NULL;
    
    //Todo: hit the cache first
    dwError = ADLdap_FindUserNameById(
                    uid,
                    &pszNT4Name);
    BAIL_ON_LSA_ERROR(dwError);

    BAIL_ON_INVALID_STRING(pszNT4Name);

    dwError = LsaCrackDomainQualifiedName(
                    pszNT4Name,
                    gpADProviderData->szDomain,
                    &pLoginInfo);
    BAIL_ON_LSA_ERROR(dwError);

    if (!AD_ServicesDomain(pLoginInfo->pszDomainNetBiosName)) {
        dwError = LSA_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    LSA_SAFE_FREE_STRING(pLoginInfo->pszFullDomainName);

    dwError = AD_DetermineTrustModeandDomainName(
                        pLoginInfo->pszDomainNetBiosName,
                        NULL,
                        NULL,
                        &pLoginInfo->pszFullDomainName,
                        NULL);
    BAIL_ON_LSA_ERROR(dwError);
    
    *ppLoginInfo = pLoginInfo;
    
cleanup:
    
    LSA_SAFE_FREE_STRING(pszNT4Name);    

    
    return dwError;

error:
    
    if (pLoginInfo)
    {
        LsaFreeNameInfo(pLoginInfo);
    }
    goto cleanup;

    
}
