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
 *        Active Directory Authentication Provider
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Wei Fu (wfu@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 */
#include "adprovider.h"

DWORD
AD_OnlineFindCellDN(
    IN HANDLE hDirectory,
    IN PCSTR pszComputerDN,
    IN PCSTR pszRootDN,
    OUT PSTR* ppszCellDN
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    PSTR  pszParentDN = NULL;
    PSTR  pszCellDN = NULL;
    PSTR  pszTmpDN = NULL;

    dwError = LsaLdapGetParentDN(pszComputerDN, &pszParentDN);
    BAIL_ON_LSA_ERROR(dwError);

    //
    // Note: We keep looking at all parents of the current DN
    //       until we find a cell or hit the top domain DN.
    for (;;)
    {
        dwError = ADGetCellInformation(hDirectory, pszParentDN, &pszCellDN);
        if (dwError == LSA_ERROR_NO_SUCH_CELL)
        {
            dwError = 0;
        }
        BAIL_ON_LSA_ERROR(dwError);

        if (!IsNullOrEmptyString(pszCellDN))
            break;

        if (!strcasecmp(pszRootDN, pszParentDN))
            break;

        LSA_SAFE_FREE_STRING(pszTmpDN);

        pszTmpDN = pszParentDN;
        pszParentDN = NULL;

        dwError = LsaLdapGetParentDN(pszTmpDN, &pszParentDN);
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppszCellDN = pszCellDN;

cleanup:

    LSA_SAFE_FREE_STRING(pszParentDN);
    LSA_SAFE_FREE_STRING(pszTmpDN);
    return dwError;

error:

    LSA_SAFE_FREE_STRING(pszCellDN);
    *ppszCellDN = NULL;
    goto cleanup;
}

DWORD
AD_OnlineInitializeOperatingMode(
    OUT PAD_PROVIDER_DATA* ppProviderData,
    IN PCSTR pszDomain,
    IN PCSTR pszHostName
    )
{
    DWORD dwError = 0;
    PSTR  pszComputerDN = NULL;
    PSTR  pszCellDN = NULL;
    PSTR  pszRootDN = NULL;
    HANDLE hDirectory = (HANDLE)NULL;
    ADConfigurationMode adConfMode = NonSchemaMode;
    HANDLE hDb = (HANDLE)NULL;
    PLSA_DM_ENUM_DOMAIN_INFO* ppDomainInfo = NULL;
    DWORD dwDomainInfoCount = 0;
    PAD_PROVIDER_DATA pProviderData = NULL;
    PSTR pszNetbiosDomainName = NULL;

    dwError = LsaAllocateMemory(sizeof(*pProviderData), (PVOID*)&pProviderData);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_OpenDb(&hDb);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaDmEngineDiscoverTrusts(pszDomain);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaDmWrapLdapOpenDirectoryDomain(pszDomain, &hDirectory);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLdapConvertDomainToDN(pszDomain, &pszRootDN);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADFindComputerDN(hDirectory, pszHostName, pszDomain,
                               &pszComputerDN);
    BAIL_ON_LSA_ERROR(dwError);

    if (AD_GetCellSupport() == AD_CELL_SUPPORT_UNPROVISIONED)
    {
        LSA_LOG_INFO("Disabling cell support due to cell-support configuration setting");
    }
    else
    {
        dwError = AD_OnlineFindCellDN(
                        hDirectory,
                        pszComputerDN,
                        pszRootDN,
                        &pszCellDN);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (IsNullOrEmptyString(pszCellDN))
    {
        pProviderData->dwDirectoryMode = UNPROVISIONED_MODE;
    }
    else
    {
        PSTR pszValue = pszCellDN + sizeof("CN=$LikewiseIdentityCell,") - 1;

        if (!strcasecmp(pszValue, pszRootDN))
        {
            pProviderData->dwDirectoryMode = DEFAULT_MODE;
            strcpy(pProviderData->cell.szCellDN, pszCellDN);
        }
        else {
            pProviderData->dwDirectoryMode = CELL_MODE;
            strcpy(pProviderData->cell.szCellDN, pszCellDN);
         }
    }

    dwError = ADGetDomainMaxPwdAge(hDirectory, pszDomain,
                                   &pProviderData->adMaxPwdAge);
    BAIL_ON_LSA_ERROR(dwError);

    switch (pProviderData->dwDirectoryMode)
    {
        case DEFAULT_MODE:
        case CELL_MODE:
            dwError = ADGetConfigurationMode(hDirectory, pszCellDN,
                                             &adConfMode);
            BAIL_ON_LSA_ERROR(dwError);
            break;
    }

    dwError = LsaDmWrapGetDomainName(pszDomain, NULL, &pszNetbiosDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    strcpy(pProviderData->szDomain, pszDomain);
    strcpy(pProviderData->szComputerDN, pszComputerDN);
    strcpy(pProviderData->szShortDomain, pszNetbiosDomainName);

    pProviderData->adConfigurationMode = adConfMode;

    if (pProviderData->dwDirectoryMode == CELL_MODE)
    {
        dwError = AD_GetLinkedCellInfo(hDirectory,
                    pszCellDN,
                    pszDomain,
                    &pProviderData->pCellList);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = ADCacheDB_CacheProviderData(
                hDb,
                pProviderData);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaDmEnumDomainInfo(
                NULL,
                NULL,
                &ppDomainInfo,
                &dwDomainInfoCount);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_CacheDomainTrustList(
                hDb,
                ppDomainInfo,
                dwDomainInfoCount);
    BAIL_ON_LSA_ERROR(dwError);

    *ppProviderData = pProviderData;

cleanup:
    LSA_SAFE_FREE_STRING(pszNetbiosDomainName);
    LSA_SAFE_FREE_STRING(pszRootDN);
    LSA_SAFE_FREE_STRING(pszComputerDN);
    LSA_SAFE_FREE_STRING(pszCellDN);
    LsaLdapCloseDirectory(hDirectory);
    ADCacheDB_SafeCloseDb(&hDb);
    LsaDmFreeEnumDomainInfoArray(ppDomainInfo);

    return dwError;

error:
    *ppProviderData = NULL;

    if (pProviderData)
    {
        ADProviderFreeProviderData(pProviderData);
        pProviderData = NULL;
    }

    goto cleanup;
}

DWORD
AD_GetLinkedCellInfo(
    IN HANDLE hDirectory,
    IN PCSTR pszCellDN,
    IN PCSTR pszDomain,
    OUT PDLINKEDLIST* ppCellList
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    LDAP *pLd = NULL;
    LDAPMessage *pCellMessage1 = NULL;
    LDAPMessage *pCellMessage2 = NULL;
    DWORD dwCount = 0;
    PSTR szAttributeList[] =
                    {AD_LDAP_DESCRIP_TAG,
                     NULL
                    };
    PSTR szAttributeListCellName[] =
                    {AD_LDAP_NAME_TAG,
                     NULL
                    };
    PSTR* ppszValues = NULL;
    DWORD dwNumValues = 0;
    DWORD iValue = 0;
    PSTR  pszLinkedCell = NULL;
    PSTR  pszLinkedCellGuid = NULL;
    PSTR  pszDirectoryRoot = NULL;
    CHAR  szQuery[1024];
    BOOLEAN bValidADEntry = FALSE;
    PSTR pszStrTokSav = NULL;
    PCSTR pszDelim = ";";
    HANDLE hGCDirectory = (HANDLE)NULL;
    LDAP* pGCLd = NULL;
    PDLINKEDLIST pCellList = NULL;

    pLd = LsaLdapGetSession(hDirectory);

    dwError = LsaLdapDirectorySearch(
                      hDirectory,
                      pszCellDN,
                      LDAP_SCOPE_BASE,
                      "(objectClass=*)",
                      szAttributeList,
                      &pCellMessage1);
    BAIL_ON_LSA_ERROR(dwError);

    dwCount = ldap_count_entries(
                      pLd,
                      pCellMessage1);
    if (dwCount < 0) {
       dwError = LSA_ERROR_LDAP_ERROR;
    } else if (dwCount == 0) {
       dwError = LSA_ERROR_NO_SUCH_CELL;
    } else if (dwCount > 1) {
       dwError = LSA_ERROR_DUPLICATE_CELLNAME;
    }
    BAIL_ON_LSA_ERROR(dwError);

    //Confirm the entry we obtain from AD is valid by retrieving its DN
    dwError = LsaLdapIsValidADEntry(
                    hDirectory,
                    pCellMessage1,
                    &bValidADEntry);
    BAIL_ON_LSA_ERROR(dwError);

    if (!bValidADEntry){
        dwError = LSA_ERROR_LDAP_FAILED_GETDN;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaLdapGetStrings(
                     hDirectory,
                     pCellMessage1,
                     AD_LDAP_DESCRIP_TAG,
                     &ppszValues,
                     &dwNumValues);
    BAIL_ON_LSA_ERROR(dwError);

    for (iValue = 0; iValue < dwNumValues; iValue++)
    {
        if (!strncasecmp(ppszValues[iValue], "linkedCells=", sizeof("linkedCells=")-1))
        {
            pszLinkedCell = ppszValues[iValue] + sizeof("linkedCells=") - 1;
           break;
        }
    }

    if (!IsNullOrEmptyString(pszLinkedCell)){
        dwError = LsaLdapConvertDomainToDN(
                        pszDomain,
                        &pszDirectoryRoot);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaDmWrapLdapOpenDirectoryGc(pszDomain,
                                               &hGCDirectory);
        BAIL_ON_LSA_ERROR(dwError);

        pGCLd = LsaLdapGetSession(hGCDirectory);

        pszLinkedCellGuid = strtok_r (pszLinkedCell, pszDelim, &pszStrTokSav);
        while (pszLinkedCellGuid != NULL)
        {
            PSTR  pszHexStr = NULL;
            PAD_LINKED_CELL_INFO pLinkedCellInfo = NULL;
            PSTR  pszCellDirectoryRoot = NULL;
            PSTR  pszLinkedCellDN = NULL;
            PSTR  pszCellDN = NULL;

            dwError = LsaAllocateMemory(
                    sizeof(AD_LINKED_CELL_INFO),
                    (PVOID*)&pLinkedCellInfo);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = ADGuidStrToHex(
                            pszLinkedCellGuid,
                            &pszHexStr);
            BAIL_ON_LSA_ERROR(dwError);

            sprintf(szQuery, "(objectGuid=%s)", pszHexStr);
            LSA_SAFE_FREE_STRING(pszHexStr);

            //Search in root node's GC for cell DN given cell's GUID
            dwError = LsaLdapDirectorySearch(
                              hGCDirectory,
                              pszDirectoryRoot,
                              LDAP_SCOPE_SUBTREE,
                              szQuery,
                              szAttributeListCellName,
                              &pCellMessage2);
            BAIL_ON_LSA_ERROR(dwError);

            dwCount = ldap_count_entries(
                              pGCLd,
                              pCellMessage2);
            if (dwCount < 0) {
               dwError = LSA_ERROR_LDAP_ERROR;
            } else if (dwCount == 0) {
               dwError = LSA_ERROR_NO_SUCH_CELL;
            } else if (dwCount > 1) {
               dwError = LSA_ERROR_DUPLICATE_CELLNAME;
            }
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaLdapGetDN(
                            hGCDirectory,
                            pCellMessage2,
                            &pszLinkedCellDN);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaAllocateStringPrintf(
                             &pLinkedCellInfo->pszCellDN,
                             "CN=$LikewiseIdentityCell,%s",
                             pszLinkedCellDN);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaLdapConvertDNToDomain(
                             pszLinkedCellDN,
                             &pLinkedCellInfo->pszDomain);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaLdapConvertDomainToDN(
                            pLinkedCellInfo->pszDomain,
                            &pszCellDirectoryRoot);
            BAIL_ON_LSA_ERROR(dwError);

            pszCellDN = pLinkedCellInfo->pszCellDN + sizeof("CN=$LikewiseIdentityCell,") - 1;
            //if pszLinkedCellDN is equal to pLinkedCellInfo->pszDomain, it is a default cell, hence a forest cell
            if (!strcasecmp(pszCellDN,
                            pszCellDirectoryRoot)){
                pLinkedCellInfo->bIsForestCell = TRUE;
            }
            else{
                pLinkedCellInfo->bIsForestCell = FALSE;
            }

            dwError = LsaDLinkedListAppend(&pCellList, pLinkedCellInfo);
            BAIL_ON_LSA_ERROR(dwError);

            pszLinkedCellGuid = strtok_r (NULL, pszDelim, &pszStrTokSav);

            LSA_SAFE_FREE_STRING (pszCellDirectoryRoot);
            LSA_SAFE_FREE_STRING (pszLinkedCellDN);

            if (pCellMessage2){
                ldap_msgfree(pCellMessage2);
                pCellMessage2 =  NULL;
            }
        }
    }

    *ppCellList = pCellList;

cleanup:

    if (pCellMessage1) {
        ldap_msgfree(pCellMessage1);
    }

    if (pCellMessage2){
        ldap_msgfree(pCellMessage2);
    }

    if (ppszValues) {
        LsaFreeStringArray(ppszValues, dwNumValues);
    }

    if (hGCDirectory != (HANDLE)NULL) {
        LsaLdapCloseDirectory(hGCDirectory);
    }

    LSA_SAFE_FREE_STRING (pszDirectoryRoot);

    return dwError;

error:
    *ppCellList = NULL;

    if (pCellList)
    {
        ADProviderFreeCellList(pCellList);
    }

    goto cleanup;
}

void
AD_FreeHashStringKey(
    const LSA_HASH_ENTRY *pEntry)
{
    PSTR pszKeyCopy = (PSTR)pEntry->pKey;
    LSA_SAFE_FREE_STRING(pszKeyCopy);
}

DWORD
AD_DetermineTrustModeandDomainName(
    IN PCSTR pszDomain,
    OUT OPTIONAL LSA_TRUST_DIRECTION* pdwTrustDirection,
    OUT OPTIONAL LSA_TRUST_MODE* pdwTrustMode,
    OUT OPTIONAL PSTR* ppszDnsDomainName,
    OUT OPTIONAL PSTR* ppszNetbiosDomainName
    )
{
    DWORD dwError = 0;
    PSTR pszDnsDomainName = NULL;
    PSTR pszNetbiosDomainName = NULL;
    DWORD dwTrustFlags = 0;
    DWORD dwTrustType = 0;
    DWORD dwTrustAttributes = 0;
    LSA_TRUST_DIRECTION dwTrustDirection = LSA_TRUST_DIRECTION_UNKNOWN;
    LSA_TRUST_MODE dwTrustMode = LSA_TRUST_MODE_UNKNOWN;

    if (IsNullOrEmptyString(pszDomain) ||
        IsNullOrEmptyString(gpADProviderData->szDomain) ||
        IsNullOrEmptyString(gpADProviderData->szShortDomain))
    {
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

#if 0
    if (!strcasecmp(gpADProviderData->szDomain, pszDomain) ||
        !strcasecmp(gpADProviderData->szShortDomain, pszDomain))
    {
        dwTrustDirection = LSA_TRUST_DIRECTION_SELF;
        if (ppszDnsDomainName)
        {
            dwError = LsaAllocateString(gpADProviderData->szDomain,
                                        &pszDnsDomainName);
            BAIL_ON_LSA_ERROR(dwError);
        }
        if (ppszNetbiosDomainName)
        {
            dwError = LsaAllocateString(gpADProviderData->szShortDomain,
                                        &pszNetbiosDomainName);
            BAIL_ON_LSA_ERROR(dwError);
        }
        dwError = 0;
        goto cleanup;
    }
#endif

    dwError = LsaDmQueryDomainInfo(pszDomain,
                                   ppszDnsDomainName ? &pszDnsDomainName : NULL,
                                   ppszNetbiosDomainName ? &pszNetbiosDomainName : NULL,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &dwTrustFlags,
                                   &dwTrustType,
                                   &dwTrustAttributes,
                                   &dwTrustDirection,
                                   &dwTrustMode,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL);
    if (LSA_ERROR_NO_SUCH_DOMAIN == dwError)
    {
        LSA_LOG_WARNING("Domain '%s' is unknown.", pszDomain);
    }
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    if (pdwTrustDirection)
    {
        *pdwTrustDirection = dwTrustDirection;
    }
    if (pdwTrustMode)
    {
        *pdwTrustMode = dwTrustMode;
    }

    if (ppszDnsDomainName)
    {
        *ppszDnsDomainName = pszDnsDomainName;
    }
    if (ppszNetbiosDomainName)
    {
        *ppszNetbiosDomainName = pszNetbiosDomainName;
    }

    return dwError;

error:
    LSA_SAFE_FREE_STRING(pszDnsDomainName);
    LSA_SAFE_FREE_STRING(pszNetbiosDomainName);
    goto cleanup;
}

static
DWORD
AD_PacRidsToSidStringList(
    IN OPTIONAL DomSid* pDomainSid,
    IN RidWithAttributeArray* pRids,
    OUT PDWORD pdwSidCount,
    OUT PSTR** pppszSidList
    )
{
    DWORD dwError = 0;
    DomSid* pDomainBasedSid = NULL;
    DWORD i = 0;
    DWORD dwSidCount = 0;
    PSTR* ppszSidList = NULL;

    if (!pDomainSid)
    {
        if (pRids->count != 0)
        {
            dwError = LSA_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
        }
        // No SIDs here, so return empty list.
        dwError = 0;
        goto error;
    }

    dwSidCount = pRids->count;

    dwError = LsaAllocateMemory(sizeof(ppszSidList[0]) * dwSidCount,
                                (PVOID*)&ppszSidList);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = SidAllocateResizedCopy(
                    &pDomainBasedSid,
                    pDomainSid->subauth_count + 1,
                    pDomainSid);
    BAIL_ON_LSA_ERROR(dwError);

    for (i = 0; i < pRids->count; i++)
    {
        pDomainBasedSid->subauth[pDomainBasedSid->subauth_count - 1] =
            pRids->rids[i].rid;

        dwError = AD_SidToString(
                        pDomainBasedSid,
                        &ppszSidList[i]);
        BAIL_ON_LSA_ERROR(dwError);
    }

    *pdwSidCount = dwSidCount;
    *pppszSidList = ppszSidList;

cleanup:
    if (pDomainBasedSid)
    {
        SidFree(pDomainBasedSid);
    }
    return dwError;

error:
    *pdwSidCount = 0;
    *pppszSidList = NULL;

    LsaFreeStringArray(ppszSidList, dwSidCount);
    goto cleanup;
}

static
DWORD
AD_PacAttributedSidsToSidStringList(
    IN DWORD dwSidCount,
    IN NetrSidAttr* pAttributedSids,
    OUT PDWORD pdwSidCount,
    OUT PSTR** pppszSidList,
    OUT PDWORD* ppdwSidAttributeList
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    PSTR* ppszSidList = NULL;
    PDWORD pdwSidAttributeList = NULL;

    dwError = LsaAllocateMemory(sizeof(ppszSidList[0]) * dwSidCount,
                                (PVOID*)&ppszSidList);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAllocateMemory(sizeof(pdwSidAttributeList[0]) * dwSidCount,
                                (PVOID*)&pdwSidAttributeList);
    BAIL_ON_LSA_ERROR(dwError);

    for (i = 0; i < dwSidCount;  i++)
    {
        dwError = AD_SidToString(
                        pAttributedSids[i].sid,
                        &ppszSidList[i]);
        BAIL_ON_LSA_ERROR(dwError);

        pdwSidAttributeList[i] = pAttributedSids[i].attribute;
    }

    *pdwSidCount = dwSidCount;
    *pppszSidList = ppszSidList;
    *ppdwSidAttributeList = pdwSidAttributeList;

cleanup:
    return dwError;

error:
    *pdwSidCount = 0;
    *pppszSidList = NULL;
    *ppdwSidAttributeList = NULL;

    LsaFreeStringArray(ppszSidList, dwSidCount);
    LSA_SAFE_FREE_MEMORY(pdwSidAttributeList);
    goto cleanup;
}

static
DWORD
AD_SidStringListsFromPac(
    IN PAC_LOGON_INFO* pPac,
    OUT PDWORD pdwGroupSidCount,
    OUT PSTR** pppszGroupSidList,
    OUT PDWORD pdwResourceSidCount,
    OUT PSTR** pppszResourceSidList,
    OUT PDWORD pdwExtraSidCount,
    OUT PSTR** pppszExtraSidList,
    OUT PDWORD* ppdwExtraSidAttributeList
    )
{
    DWORD dwError = 0;
    DWORD dwGroupSidCount = 0;
    PSTR* ppszGroupSidList = NULL;
    DWORD dwResourceGroupSidCount = 0;
    PSTR* ppszResourceGroupSidList = NULL;
    DWORD dwExtraSidCount = 0;
    PSTR* ppszExtraSidList = NULL;
    PDWORD pdwExtraSidAttributeList = NULL;

    // PAC group membership SIDs

    dwError = AD_PacRidsToSidStringList(
                    pPac->info3.base.domain_sid,
                    &pPac->info3.base.groups,
                    &dwGroupSidCount,
                    &ppszGroupSidList);
    BAIL_ON_LSA_ERROR(dwError);

    // PAC resource domain group membership SIDs

    dwError = AD_PacRidsToSidStringList(
                    pPac->res_group_dom_sid,
                    &pPac->res_groups,
                    &dwResourceGroupSidCount,
                    &ppszResourceGroupSidList);
    BAIL_ON_LSA_ERROR(dwError);

    // PAC extra SIDs

    dwError = AD_PacAttributedSidsToSidStringList(
                    pPac->info3.sidcount,
                    pPac->info3.sids,
                    &dwExtraSidCount,
                    &ppszExtraSidList,
                    &pdwExtraSidAttributeList);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    if (dwError)
    {
        LsaFreeStringArray(ppszGroupSidList, dwGroupSidCount);
        dwGroupSidCount = 0;
        ppszGroupSidList = NULL;
        LsaFreeStringArray(ppszResourceGroupSidList, dwResourceGroupSidCount);
        dwResourceGroupSidCount = 0;
        ppszResourceGroupSidList = NULL;
        LsaFreeStringArray(ppszExtraSidList, dwExtraSidCount);
        dwExtraSidCount = 0;
        ppszExtraSidList = NULL;
        LSA_SAFE_FREE_MEMORY(pdwExtraSidAttributeList);
    }

    *pdwGroupSidCount = dwGroupSidCount;
    *pppszGroupSidList = ppszGroupSidList;
    *pdwResourceSidCount = dwResourceGroupSidCount;
    *pppszResourceSidList = ppszResourceGroupSidList;
    *pdwExtraSidCount = dwExtraSidCount;
    *pppszExtraSidList = ppszExtraSidList;
    *ppdwExtraSidAttributeList = pdwExtraSidAttributeList;

    return dwError;

error:
    // Handle error in cleanup for simplicity.
    goto cleanup;
}

static
DWORD
AD_PacMembershipFilterWithLdap(
    IN HANDLE hProvider,
    IN LSA_TRUST_DIRECTION dwTrustDirection,
    IN PAD_SECURITY_OBJECT pUserInfo,
    IN DWORD dwMembershipCount,
    IN OUT PAD_GROUP_MEMBERSHIP* ppMemberships
    )
{
    DWORD dwError = 0;
    int iPrimaryGroupIndex = -1;
    size_t sLdapGroupCount = 0;
    PAD_SECURITY_OBJECT* ppLdapGroups = NULL;
    LSA_HASH_TABLE* pMembershipHashTable = NULL;
    time_t now = 0;
    size_t i = 0;

    if (LSA_TRUST_DIRECTION_ONE_WAY == dwTrustDirection)
    {
        goto cleanup;
    }

    dwError = ADLdap_GetUserGroupMembership(
                    hProvider,
                    pUserInfo,
                    &iPrimaryGroupIndex,
                    &sLdapGroupCount,
                    &ppLdapGroups);
    BAIL_ON_LSA_ERROR(dwError);

    if (sLdapGroupCount < 1)
    {
        goto cleanup;
    }

    dwError = LsaGetCurrentTimeSeconds(&now);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaHashCreate(
                    dwMembershipCount,
                    LsaHashCaselessStringCompare,
                    LsaHashCaselessString,
                    NULL,
                    &pMembershipHashTable);
    BAIL_ON_LSA_ERROR(dwError);

    for (i = 0; i < dwMembershipCount; i++)
    {
        // Ignore the NULL entry
        if (ppMemberships[i]->pszParentSid)
        {
            dwError = LsaHashSetValue(pMembershipHashTable,
                                      ppMemberships[i]->pszParentSid,
                                      ppMemberships[i]);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    // For anything that we find via LDAP, make it expirable or primary.
    for (i = 0; i < sLdapGroupCount; i++)
    {
        PAD_GROUP_MEMBERSHIP pMembership = NULL;

        dwError = LsaHashGetValue(pMembershipHashTable,
                                  ppLdapGroups[i]->pszObjectSid,
                                  (PVOID*)&pMembership);
        if (LSA_ERROR_SUCCESS == dwError)
        {
            if ((DWORD)iPrimaryGroupIndex == i)
            {
                pMembership->bIsDomainPrimaryGroup = TRUE;
            }
            pMembership->bIsInLdap = TRUE;
        }
        else if (dwError == ENOENT)
        {
            dwError = LSA_ERROR_SUCCESS;
        }
        else
        {
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

cleanup:
    ADCacheDB_SafeFreeObjectList(sLdapGroupCount, &ppLdapGroups);
    LsaHashSafeFree(&pMembershipHashTable);
    return dwError;

error:
    goto cleanup;
}

DWORD
AD_CacheGroupMembershipFromPac(
    IN HANDLE hProvider,
    IN LSA_TRUST_DIRECTION dwTrustDirection,
    IN PAD_SECURITY_OBJECT pUserInfo,
    IN PAC_LOGON_INFO* pPac
    )
{
    DWORD dwError = 0;
    time_t now = 0;
    HANDLE hDb = (HANDLE)NULL;
    DWORD dwGroupSidCount = 0;
    PSTR* ppszGroupSidList = NULL;
    DWORD dwResourceGroupSidCount = 0;
    PSTR* ppszResourceGroupSidList = NULL;
    DWORD dwExtraSidCount = 0;
    PSTR* ppszExtraSidList = NULL;
    PDWORD pdwExtraSidAttributeList = NULL;
    DWORD i = 0;
    DWORD dwIgnoreExtraSidCount = 0;
    DWORD dwMembershipCount = 0;
    PAD_GROUP_MEMBERSHIP* ppMemberships = NULL;
    DWORD dwMembershipIndex = 0;
    struct {
        PDWORD pdwCount;
        PSTR** pppszSidList;
    } SidsToCombine[] = {
        { &dwGroupSidCount, &ppszGroupSidList },
        { &dwResourceGroupSidCount, &ppszResourceGroupSidList },
        { &dwExtraSidCount, &ppszExtraSidList }
    };
    DWORD dwSidsToCombineIndex = 0;
    PAD_GROUP_MEMBERSHIP pMembershipBuffers = NULL;

    LSA_LOG_VERBOSE(
            "Updating user group membership for uid %lu with PAC information",
             (unsigned long)pUserInfo->userInfo.uid);

    dwError = LsaGetCurrentTimeSeconds(&now);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_SidStringListsFromPac(
                    pPac,
                    &dwGroupSidCount,
                    &ppszGroupSidList,
                    &dwResourceGroupSidCount,
                    &ppszResourceGroupSidList,
                    &dwExtraSidCount,
                    &ppszExtraSidList,
                    &pdwExtraSidAttributeList);
    BAIL_ON_LSA_ERROR(dwError);

    for (i = 0; i < dwGroupSidCount; i++)
    {
        LSA_LOG_VERBOSE("uid %lu's #%lu PAC group membership is %s",
                        (unsigned long)pUserInfo->userInfo.uid,
                        (unsigned long)i,
                        ppszGroupSidList[i]);
    }

    for (i = 0; i < dwResourceGroupSidCount; i++)
    {
        LSA_LOG_VERBOSE("uid %lu's #%lu PAC resource group membership is %s",
                        (unsigned long)pUserInfo->userInfo.uid,
                        (unsigned long)i,
                        ppszResourceGroupSidList[i]);
    }

    for (i = 0; i < dwExtraSidCount; i++)
    {
        LSA_LOG_VERBOSE("uid %lu's #%lu PAC extra membership is %s (attributes = 0x%08x)",
                        (unsigned long)pUserInfo->userInfo.uid,
                        (unsigned long)i,
                        ppszExtraSidList[i],
                        pdwExtraSidAttributeList[i]);

        // Filter out unwanted SIDs.

        // ISSUE-2008/11/03-dalmeida -- Revisit this piece.
        // Apparently, we still let user sids through here.
        // Perhaps we should not filterat all.

        // universal groups seem to have this set to 7
        // local groups seem to have this set to 0x20000007
        // we don't want to treat sids from the sid history like groups.
        if (pdwExtraSidAttributeList[i] != 7 &&
            pdwExtraSidAttributeList[i] != 0x20000007)
        {
            LSA_LOG_VERBOSE("Ignoring non-group SID %s (attribute is 0x%x)",
                            ppszExtraSidList[i],
                            pdwExtraSidAttributeList[i]);
            LSA_SAFE_FREE_STRING(ppszExtraSidList[i]);
            dwIgnoreExtraSidCount++;
        }
    }

    // Allocate one extra for NULL entry
    dwMembershipCount = (dwGroupSidCount + dwResourceGroupSidCount +
                         dwExtraSidCount - dwIgnoreExtraSidCount + 1);

    dwError = LsaAllocateMemory(
                    sizeof(ppMemberships[0]) * dwMembershipCount,
                    (PVOID*)&ppMemberships);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAllocateMemory(
                    sizeof(pMembershipBuffers[0]) * dwMembershipCount,
                    (PVOID*)&pMembershipBuffers);
    BAIL_ON_LSA_ERROR(dwError);

    dwMembershipIndex = 0;
    for (dwSidsToCombineIndex = 0;
         dwSidsToCombineIndex < sizeof(SidsToCombine)/sizeof(SidsToCombine[0]);
         dwSidsToCombineIndex++)
    {
        DWORD dwSidCount = *SidsToCombine[dwSidsToCombineIndex].pdwCount;
        for (i = 0; i < dwSidCount; i++)
        {
            PAD_GROUP_MEMBERSHIP* ppMembership = &ppMemberships[dwMembershipIndex];
            PAD_GROUP_MEMBERSHIP pMembership = &pMembershipBuffers[dwMembershipIndex];
            PSTR* ppszSidList = *SidsToCombine[dwSidsToCombineIndex].pppszSidList;
            if (ppszSidList[i])
            {
                *ppMembership = pMembership;
                pMembership->cache.qwCacheId = -1;
                pMembership->pszParentSid = ppszSidList[i];
                pMembership->pszChildSid = pUserInfo->pszObjectSid;
                pMembership->bIsInPac = TRUE;
                dwMembershipIndex++;
            }
        }
    }

    assert((dwMembershipCount - 1) == dwMembershipIndex);

    // Set up NULL entry.
    ppMemberships[dwMembershipIndex] = &pMembershipBuffers[dwMembershipIndex];
    ppMemberships[dwMembershipIndex]->cache.qwCacheId = -1;
    ppMemberships[dwMembershipIndex]->pszChildSid = pUserInfo->pszObjectSid;

    if (AD_GetTrimUserMembershipEnabled())
    {
        dwError = AD_PacMembershipFilterWithLdap(
                        hProvider,
                        dwTrustDirection,
                        pUserInfo,
                        dwMembershipCount,
                        ppMemberships);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = ADCacheDB_OpenDb(&hDb);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_CacheGroupsForUser(
                        hDb,
                        pUserInfo->pszObjectSid,
                        dwMembershipCount,
                        ppMemberships,
                        TRUE);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    LSA_SAFE_FREE_MEMORY(ppMemberships);
    LSA_SAFE_FREE_MEMORY(pMembershipBuffers);
    LsaFreeStringArray(ppszGroupSidList, dwGroupSidCount);
    LsaFreeStringArray(ppszResourceGroupSidList, dwResourceGroupSidCount);
    LsaFreeStringArray(ppszExtraSidList, dwExtraSidCount);
    LSA_SAFE_FREE_MEMORY(pdwExtraSidAttributeList);
    ADCacheDB_SafeCloseDb(&hDb);

    return dwError;

error:
    goto cleanup;
}

DWORD
AD_GetCachedPasswordHash(
    IN PCSTR pszSamAccount,
    IN PCSTR pszPassword,
    OUT PBYTE *ppbHash
    )
{
    PWSTR pwszPassword = NULL;
    PBYTE pbPrehashedVerifier = NULL;
    size_t sPrehashedVerifierLen = 0;
    PBYTE pbHash = NULL;
    DWORD dwError = LSA_ERROR_SUCCESS;
    size_t sConvertedChars = 0;

    // Allocate space to store the NT hash with the username appended
    sPrehashedVerifierLen = 16 + strlen(pszSamAccount) * sizeof(wchar16_t);
    dwError = LsaAllocateMemory(
                    sPrehashedVerifierLen + sizeof(wchar16_t),
                    (PVOID*)&pbPrehashedVerifier);
    BAIL_ON_LSA_ERROR(dwError);

    // Compute the NT hash (which only depends on the password) and store
    // it in the first 16 bytes of pbPrehashedVerifier

    dwError = LsaMbsToWc16s(
            pszPassword,
            &pwszPassword);
    BAIL_ON_LSA_ERROR(dwError);

    MD4(
        (UCHAR *)pwszPassword,
        strlen(pszPassword) * sizeof(wchar16_t),
        pbPrehashedVerifier);

    // Append the username in UCS-2 encoding to the NT hash
    sConvertedChars = mbstowc16s(
            (wchar16_t *)(pbPrehashedVerifier + 16),
            pszSamAccount,
            strlen(pszSamAccount));
    if (sConvertedChars != strlen(pszSamAccount))
    {
        dwError = LSA_ERROR_STRING_CONV_FAILED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    // Calculate the password verifier in binary format
    dwError = LsaAllocateMemory(
                    16,
                    (PVOID*)&pbHash);
    BAIL_ON_LSA_ERROR(dwError);

    MD4(
        pbPrehashedVerifier,
        sPrehashedVerifierLen,
        pbHash);

    *ppbHash = pbHash;

cleanup:

    LSA_SAFE_FREE_MEMORY(pwszPassword);
    LSA_SAFE_FREE_MEMORY(pbPrehashedVerifier);

    return dwError;

error:

    *ppbHash = NULL;
    LSA_SAFE_FREE_MEMORY(pbHash);

    goto cleanup;
}

DWORD
AD_OnlineCachePasswordVerifier(
    IN PAD_SECURITY_OBJECT pUserInfo,
    IN PCSTR  pszPassword
    )
{
    DWORD dwError = 0;
    PAD_PASSWORD_VERIFIER pVerifier = NULL;
    struct timeval current_tv;
    PBYTE pbHash = NULL;
    HANDLE hDb = (HANDLE)NULL;

    if (gettimeofday(&current_tv, NULL) < 0)
    {
        dwError = errno;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaAllocateMemory(
                    sizeof(*pVerifier),
                    (PVOID*)&pVerifier);
    BAIL_ON_LSA_ERROR(dwError);

    pVerifier->cache.tLastUpdated = current_tv.tv_sec;
    pVerifier->cache.qwCacheId = -1;

    dwError = LsaAllocateString(
                    pUserInfo->pszObjectSid,
                    &pVerifier->pszObjectSid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_GetCachedPasswordHash(
                pUserInfo->pszSamAccountName,
                pszPassword,
                &pbHash);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaByteArrayToHexStr(
                pbHash,
                16,
                &pVerifier->pszPasswordVerifier);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_OpenDb(&hDb);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_CachePasswordVerifier(
                hDb,
                pVerifier);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    ADCACHEDB_SAFE_FREE_PASSWORD_VERIFIER(pVerifier);
    LSA_SAFE_FREE_MEMORY(pbHash);
    ADCacheDB_SafeCloseDb(&hDb);

    return dwError;

error:

    goto cleanup;
}

DWORD
AD_OnlineAuthenticateUser(
    HANDLE hProvider,
    PCSTR  pszLoginId,
    PCSTR  pszPassword
    )
{
    DWORD dwError = 0;
    PLSA_LOGIN_NAME_INFO pLoginInfo = NULL;
    PAD_SECURITY_OBJECT pUserInfo = NULL;
    PAC_LOGON_INFO *pPac = NULL;
    PSTR pszHostname = NULL;
    PSTR pszUsername = NULL;
    PSTR pszServicePassword = NULL;
    PSTR pszDomainDnsName = NULL;
    PSTR pszServicePrincipal = NULL;
    DWORD dwGoodUntilTime = 0;
    LSA_TRUST_DIRECTION dwTrustDirection = LSA_TRUST_DIRECTION_UNKNOWN;
    PSTR pszUserDnsDomainName = NULL;
    PSTR pszFreeUpn = NULL;
    PSTR pszUpn = NULL;

    dwError = LsaCrackDomainQualifiedName(
                    pszLoginId,
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
                        &dwTrustDirection,
                        NULL,
                        &pLoginInfo->pszFullDomainName,
                        NULL);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_FindUserObjectByName(
                    hProvider,
                    pszLoginId,
                    &pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_VerifyUserAccountCanLogin(
                pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaDnsGetHostInfo(&pszHostname);
    BAIL_ON_LSA_ERROR(dwError);

    LsaStrToLower(pszHostname);

    dwError = LsaKrb5GetMachineCreds(
                    pszHostname,
                    &pszUsername,
                    &pszServicePassword,
                    &pszDomainDnsName);
    BAIL_ON_LSA_ERROR(dwError);

    //Leave the realm empty so that kerberos referrals are turned on.
    dwError = LsaAllocateStringPrintf(
                        &pszServicePrincipal,
                        "host/%s.%s@",
                        pszHostname,
                        pszDomainDnsName);
    BAIL_ON_LSA_ERROR(dwError);

    if (pUserInfo->userInfo.bIsGeneratedUPN)
    {
        pszUpn = pUserInfo->userInfo.pszUPN;
    }
    else
    {
        BOOLEAN bIsGeneratedUpn = FALSE;

        LSA_LOG_DEBUG("Using generated UPN instead of '%s'", pUserInfo->userInfo.pszUPN);

        dwError = LsaDmWrapGetDomainName(
                        pUserInfo->pszNetbiosDomainName,
                        &pszUserDnsDomainName,
                        NULL);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = ADGetLDAPUPNString(
                        0,
                        NULL,
                        pszUserDnsDomainName,
                        pUserInfo->pszSamAccountName,
                        &pszFreeUpn,
                        &bIsGeneratedUpn);
        BAIL_ON_LSA_ERROR(dwError);

        pszUpn = pszFreeUpn;
    }

    dwError = LsaSetupUserLoginSession(
                    pUserInfo->userInfo.uid,
                    pUserInfo->userInfo.gid,
                    pszUpn,
                    pszPassword,
                    KRB5_File_Cache,
                    pszServicePrincipal,
                    pszServicePassword,
                    &pPac,
                    &dwGoodUntilTime);
    BAIL_ON_LSA_ERROR(dwError);

    if (pPac != NULL)
    {
        dwError = AD_CacheGroupMembershipFromPac(
                        hProvider,
                        dwTrustDirection,
                        pUserInfo,
                        pPac);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = AD_OnlineCachePasswordVerifier(
                    pUserInfo,
                    pszPassword);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaUmAddUser(
                  pUserInfo->userInfo.uid,
                  pszPassword,
                  dwGoodUntilTime);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    if (pPac)
    {
        FreePacLogonInfo(pPac);
    }

    if (pLoginInfo)
    {
        LsaFreeNameInfo(pLoginInfo);
    }

    ADCacheDB_SafeFreeObject(&pUserInfo);
    LSA_SAFE_FREE_STRING(pszHostname);
    LSA_SAFE_FREE_STRING(pszUsername);
    LSA_SAFE_FREE_STRING(pszServicePassword);
    LSA_SAFE_FREE_STRING(pszDomainDnsName);
    LSA_SAFE_FREE_STRING(pszServicePrincipal);
    LSA_SAFE_FREE_STRING(pszUserDnsDomainName);
    LSA_SAFE_FREE_STRING(pszFreeUpn);

    return dwError;

error:

    goto cleanup;
}

DWORD
AD_CheckExpiredObject(
    IN OUT PAD_SECURITY_OBJECT* ppCachedUser
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    time_t now = 0;
    time_t expirationDate;

    dwError = LsaGetCurrentTimeSeconds(&now);
    BAIL_ON_LSA_ERROR(dwError);

    expirationDate = (*ppCachedUser)->cache.tLastUpdated +
        AD_GetCacheEntryExpirySeconds();

    if (expirationDate <= now)
    {
        LSA_LOG_VERBOSE(
                "Cache entry for sid %s expired %ld seconds ago",
                (*ppCachedUser)->pszObjectSid,
                now - expirationDate);

        //Pretend like the object couldn't be found in the cache
        ADCacheDB_SafeFreeObject(ppCachedUser);
        dwError = LSA_ERROR_NOT_HANDLED;
    }
    else
    {
        LSA_LOG_VERBOSE(
                "Using cache entry for sid %s, updated %ld seconds ago",
                (*ppCachedUser)->pszObjectSid,
                now - (*ppCachedUser)->cache.tLastUpdated);
    }

error:
    return dwError;
}

// Note: We only return whether complete if not expired.
static
DWORD
AD_CheckExpiredMemberships(
    IN size_t sCount,
    IN PAD_GROUP_MEMBERSHIP* ppMemberships,
    IN BOOLEAN bCheckNullParentSid,
    OUT PBOOLEAN pbHaveExpired,
    OUT PBOOLEAN pbIsComplete
    )
{
    DWORD dwError = 0;
    size_t sIndex = 0;
    time_t now = 0;
    DWORD dwCacheEntryExpirySeconds = 0;
    BOOLEAN bHaveExpired = FALSE;
    BOOLEAN bIsComplete = FALSE;

    dwError = LsaGetCurrentTimeSeconds(&now);
    BAIL_ON_LSA_ERROR(dwError);

    //
    // Whenever a membership is cached, an extra "null" entry is added.
    // This entry has the opposite (parent or child) field set such
    // that we can tell whether we cached a user's groups (child set)
    // or a group's members (parent set).
    //
    // If the NULL entry is missing, this means that we got the data
    // because we cached something else (e.g., we cached user's groups
    // but are not trying to find a group's members).
    //
    dwCacheEntryExpirySeconds = AD_GetCacheEntryExpirySeconds();
    for (sIndex = 0; sIndex < sCount; sIndex++)
    {
        PAD_GROUP_MEMBERSHIP pMembership = ppMemberships[sIndex];

        // Ignore what cannot expire (assumes that we already
        // filtered out PAC entries that should not be returned).
        if (pMembership->bIsInPac ||
            pMembership->bIsDomainPrimaryGroup)
        {
            continue;
        }

        if ((pMembership->cache.tLastUpdated > 0) &&
            (pMembership->cache.tLastUpdated + dwCacheEntryExpirySeconds <= now))
        {
            bHaveExpired = TRUE;
            // Note that we only return whether complete
            // if not expired.
            break;
        }

        // Check for NULL entry
        if (bCheckNullParentSid)
        {
            if (pMembership->pszParentSid == NULL)
            {
                bIsComplete = TRUE;
            }
        }
        else
        {
            if (pMembership->pszChildSid == NULL)
            {
                bIsComplete = TRUE;
            }
        }
    }

error:
    *pbHaveExpired = bHaveExpired;
    *pbIsComplete = bIsComplete;
    return dwError;
}

static
DWORD
AD_FilterExpiredMemberships(
    IN OUT size_t* psCount,
    IN OUT PAD_GROUP_MEMBERSHIP* ppMemberships
    )
{
    DWORD dwError = 0;
    size_t sIndex = 0;
    time_t now = 0;
    DWORD dwCacheEntryExpirySeconds = 0;
    size_t sCount = *psCount;
    size_t sOutputCount = 0;

    dwError = LsaGetCurrentTimeSeconds(&now);
    BAIL_ON_LSA_ERROR(dwError);

    // Cannot fail after this.
    dwCacheEntryExpirySeconds = AD_GetCacheEntryExpirySeconds();
    for (sIndex = 0; sIndex < sCount; sIndex++)
    {
        PAD_GROUP_MEMBERSHIP pMembership = ppMemberships[sIndex];

        if (pMembership->bIsInPac ||
            pMembership->bIsDomainPrimaryGroup ||
            ((pMembership->cache.tLastUpdated > 0) &&
             (pMembership->cache.tLastUpdated + dwCacheEntryExpirySeconds <= now)))
        {
            // Keep
            if (sOutputCount != sIndex)
            {
                ppMemberships[sOutputCount] = ppMemberships[sIndex];
            }
            sOutputCount++;
        }
        else
        {
            ADCacheDB_SafeFreeGroupMembership(&ppMemberships[sIndex]);
        }
    }

    *psCount = sOutputCount;

error:
    return dwError;
}

static
BOOLEAN
AD_IncludeOnlyUnexpirableGroupMembershipsCallback(
    IN PAD_GROUP_MEMBERSHIP pMembership
    )
{
    BOOLEAN bInclude = FALSE;

    if (pMembership->bIsInPac || pMembership->bIsDomainPrimaryGroup)
    {
        bInclude = TRUE;
    }

    return bInclude;
}

static
DWORD
AD_CacheMembershipFromRelatedObjects(
    IN HANDLE hDb,
    IN PCSTR pszSid,
    IN int iPrimaryGroupIndex,
    IN BOOLEAN bIsParent,
    IN size_t sCount,
    IN PAD_SECURITY_OBJECT* ppRelatedObjects
    )
{
    DWORD dwError = 0;
    PAD_GROUP_MEMBERSHIP* ppMemberships = NULL;
    PAD_GROUP_MEMBERSHIP pMembershipBuffers = NULL;
    size_t sMaxMemberships = 0;
    size_t sIndex = 0;
    size_t sMembershipCount = 0;
    PAD_SECURITY_OBJECT pPrimaryGroup = NULL;

    if (iPrimaryGroupIndex >= 0)
    {
        pPrimaryGroup = ppRelatedObjects[iPrimaryGroupIndex];
    }

    // Generate a list of AD_GROUP_MEMBERSHIP objects.  Include a
    // NULL entry to indicate that the member list is authoritative
    // parent or child SID (depending on bIsParent).

    // Need an extra entry for the NULL entry that
    // signals a complete list.
    sMaxMemberships = sCount + 1;

    dwError = LsaAllocateMemory(
                    sizeof(*ppMemberships) * sMaxMemberships,
                    (PVOID*)&ppMemberships);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAllocateMemory(
                    sizeof(pMembershipBuffers[0]) * sMaxMemberships,
                    (PVOID*)&pMembershipBuffers);
    BAIL_ON_LSA_ERROR(dwError);

    for (sIndex = 0; sIndex < sCount; sIndex++)
    {
        PAD_GROUP_MEMBERSHIP* ppMembership = &ppMemberships[sMembershipCount];
        PAD_GROUP_MEMBERSHIP pMembership = &pMembershipBuffers[sMembershipCount];
        if (ppRelatedObjects[sIndex])
        {
            *ppMembership = pMembership;
            pMembership->cache.qwCacheId = -1;
            if (bIsParent)
            {
                pMembership->pszParentSid = (PSTR)pszSid;
                pMembership->pszChildSid = ppRelatedObjects[sIndex]->pszObjectSid;
            }
            else
            {
                pMembership->pszParentSid = ppRelatedObjects[sIndex]->pszObjectSid;
                pMembership->pszChildSid = (PSTR)pszSid;
                if (pPrimaryGroup == ppRelatedObjects[sIndex])
                {
                    pMembership->bIsDomainPrimaryGroup = TRUE;
                }
            }
            pMembership->bIsInLdap = TRUE;
            sMembershipCount++;
        }
    }

    // Set up NULL entry.
    ppMemberships[sMembershipCount] = &pMembershipBuffers[sMembershipCount];
    ppMemberships[sMembershipCount]->cache.qwCacheId = -1;
    if (bIsParent)
    {
        ppMemberships[sMembershipCount]->pszParentSid = (PSTR)pszSid;
    }
    else
    {
        ppMemberships[sMembershipCount]->pszChildSid = (PSTR)pszSid;
    }
    sMembershipCount++;

    if (bIsParent)
    {
        dwError = ADCacheDB_CacheGroupMembership(
                        hDb,
                        pszSid,
                        sMembershipCount,
                        ppMemberships);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        dwError = ADCacheDB_CacheGroupsForUser(
                        hDb,
                        pszSid,
                        sMembershipCount,
                        ppMemberships,
                        FALSE);
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    LSA_SAFE_FREE_MEMORY(ppMemberships);
    LSA_SAFE_FREE_MEMORY(pMembershipBuffers);
    return dwError;

error:
    goto cleanup;
}

static
VOID
AD_TransferCacheObjets(
    IN OUT size_t* psFromObjectsCount,
    IN OUT PAD_SECURITY_OBJECT* ppFromObjects,
    IN OUT size_t* psToObjectsCount,
    IN OUT PAD_SECURITY_OBJECT* ppToObjects
    )
{
    size_t bytes = sizeof(ppFromObjects[0]) * (*psFromObjectsCount);

    memcpy(ppToObjects + *psToObjectsCount,
           ppFromObjects,
           bytes);
    *psToObjectsCount += *psFromObjectsCount;

    memset(ppFromObjects, 0, bytes);
    *psFromObjectsCount = 0;
}

static
DWORD
AD_CombineCacheObjects(
    IN OUT size_t* psFromObjectsCount1,
    IN OUT PAD_SECURITY_OBJECT* ppFromObjects1,
    IN OUT size_t* psFromObjectsCount2,
    IN OUT PAD_SECURITY_OBJECT* ppFromObjects2,
    OUT size_t* psCombinedObjectsCount,
    OUT PAD_SECURITY_OBJECT** pppCombinedObjects
    )
{
    DWORD dwError = 0;
    PAD_SECURITY_OBJECT* ppCombinedObjects = NULL;
    size_t sCombinedObjectsCount = 0;

    sCombinedObjectsCount = *psFromObjectsCount1 + *psFromObjectsCount2;
    dwError = LsaAllocateMemory(
                    sizeof(*ppCombinedObjects) * sCombinedObjectsCount,
                    (PVOID*)&ppCombinedObjects);
    BAIL_ON_LSA_ERROR(dwError);

    // Cannot fail after this point!
    sCombinedObjectsCount = 0;

    AD_TransferCacheObjets(
            psFromObjectsCount1,
            ppFromObjects1,
            &sCombinedObjectsCount,
            ppCombinedObjects);

    AD_TransferCacheObjets(
            psFromObjectsCount2,
            ppFromObjects2,
            &sCombinedObjectsCount,
            ppCombinedObjects);

    *pppCombinedObjects = ppCombinedObjects;
    *psCombinedObjectsCount = sCombinedObjectsCount;

cleanup:
    return dwError;

error:
    *pppCombinedObjects = NULL;
    *psCombinedObjectsCount = 0;

    LSA_SAFE_FREE_MEMORY(ppCombinedObjects);
    sCombinedObjectsCount = 0;
    goto cleanup;
}

static
DWORD
AD_CombineCacheObjectsRemoveDuplicates(
    IN OUT size_t* psFromObjectsCount1,
    IN OUT PAD_SECURITY_OBJECT* ppFromObjects1,
    IN OUT size_t* psFromObjectsCount2,
    IN OUT PAD_SECURITY_OBJECT* ppFromObjects2,
    OUT size_t* psCombinedObjectsCount,
    OUT PAD_SECURITY_OBJECT** pppCombinedObjects
    )
{
    DWORD dwError = 0;
    PAD_SECURITY_OBJECT* ppCombinedObjects = NULL;
    size_t sCombinedObjectsCount = 0;
    PLSA_HASH_TABLE pHashTable = NULL;
    size_t i = 0;
    LSA_HASH_ITERATOR hashIterator;
    LSA_HASH_ENTRY* pHashEntry = NULL;

    dwError = LsaHashCreate(
                20,
                AD_CompareObjectSids,
                AD_HashObjectSid,
                NULL,
                &pHashTable);
    BAIL_ON_LSA_ERROR(dwError);

    for (i = 0; i < *psFromObjectsCount1; i++)
    {
        if (!LsaHashExists(pHashTable, ppFromObjects1[i]))
        {
            dwError = LsaHashSetValue(
                            pHashTable,
                            ppFromObjects1[i],
                            ppFromObjects1[i]);
            BAIL_ON_LSA_ERROR(dwError);
            ppFromObjects1[i] = NULL;
        }
    }

    for (i = 0; i < *psFromObjectsCount2; i++)
    {
        if (!LsaHashExists(pHashTable, ppFromObjects2[i]))
        {
            dwError = LsaHashSetValue(
                            pHashTable,
                            ppFromObjects2[i],
                            ppFromObjects2[i]);
            BAIL_ON_LSA_ERROR(dwError);
            ppFromObjects2[i] = NULL;
        }
    }

    sCombinedObjectsCount = pHashTable->sCount;
    dwError = LsaAllocateMemory(
                    sizeof(*ppCombinedObjects) * sCombinedObjectsCount,
                    (PVOID*)&ppCombinedObjects);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaHashGetIterator(pHashTable, &hashIterator);
    BAIL_ON_LSA_ERROR(dwError);

    for (i = 0; (pHashEntry = LsaHashNext(&hashIterator)) != NULL; i++)
    {
        PAD_SECURITY_OBJECT pObject = (PAD_SECURITY_OBJECT) pHashEntry->pKey;

        dwError = LsaHashRemoveKey(pHashTable, pObject);
        BAIL_ON_LSA_ERROR(dwError);

        ppCombinedObjects[i] = pObject;
    }

    if (i != sCombinedObjectsCount)
    {
        dwError = LSA_ERROR_INTERNAL;
        BAIL_ON_LSA_ERROR(dwError);
    }

    *pppCombinedObjects = ppCombinedObjects;
    *psCombinedObjectsCount = sCombinedObjectsCount;

cleanup:
    if (pHashTable)
    {
        pHashTable->fnFree = AD_FreeHashObject;
        LsaHashSafeFree(&pHashTable);
    }
    // Free any remaining objects.
    for (i = 0; i < *psFromObjectsCount1; i++)
    {
        ADCacheDB_SafeFreeObject(&ppFromObjects1[i]);
    }
    *psFromObjectsCount1 = 0;
    for (i = 0; i < *psFromObjectsCount2; i++)
    {
        ADCacheDB_SafeFreeObject(&ppFromObjects2[i]);
    }
    *psFromObjectsCount2 = 0;
    return dwError;

error:
    *pppCombinedObjects = NULL;
    *psCombinedObjectsCount = 0;

    ADCacheDB_SafeFreeObjectList(sCombinedObjectsCount, &ppCombinedObjects);
    sCombinedObjectsCount = 0;
    goto cleanup;
}

void
AD_FilterNullEntries(
    IN OUT PAD_SECURITY_OBJECT* ppEntries,
    IN OUT size_t* psCount
    )
{
    size_t sInput = 0;
    size_t sOutput = 0;

    for (; sInput < *psCount; sInput++)
    {
        if (ppEntries[sInput] != NULL)
        {
            ppEntries[sOutput++] = ppEntries[sInput];
        }
    }

    *psCount = sOutput;
}

DWORD
AD_OnlineFindUserObjectById(
    HANDLE  hProvider,
    uid_t   uid,
    PAD_SECURITY_OBJECT *ppResult
    )
{
    DWORD dwError =  0;
    HANDLE hDb = (HANDLE)NULL;
    PAD_SECURITY_OBJECT pCachedUser = NULL;
    PSTR   pszNT4Name = NULL;

    if (uid == 0) {
    	dwError = LSA_ERROR_NO_SUCH_USER;
    	BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = ADCacheDB_OpenDb(&hDb);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_FindUserById(
            hDb,
            uid,
            &pCachedUser);
    if (dwError == LSA_ERROR_SUCCESS)
    {
        // Frees object if it is expired
        dwError = AD_CheckExpiredObject(&pCachedUser);
    }

    if (dwError == LSA_ERROR_NOT_HANDLED)
    {
        dwError = AD_FindObjectByIdTypeNoCache(
                    hProvider,
                    uid,
                    AccountType_User,
                    &pCachedUser);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppResult = pCachedUser;

cleanup:

    LSA_SAFE_FREE_STRING(pszNT4Name);
    ADCacheDB_SafeCloseDb(&hDb);

    return dwError;

error:

    *ppResult = NULL;
    ADCacheDB_SafeFreeObject(&pCachedUser);

    LSA_REMAP_FIND_USER_BY_ID_ERROR(dwError, FALSE, uid);

    goto cleanup;
}

DWORD
AD_OnlineGetUserGroupObjectMembership(
    IN HANDLE hProvider,
    IN uid_t uid,
    IN BOOLEAN bIsCacheOnlyMode,
    OUT size_t* psCount,
    OUT PAD_SECURITY_OBJECT** pppResults
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    HANDLE hDb = (HANDLE)NULL;
    size_t sMembershipCount = 0;
    PAD_GROUP_MEMBERSHIP* ppMemberships = NULL;
    BOOLEAN bExpired = FALSE;
    BOOLEAN bIsComplete = FALSE;
    BOOLEAN bUseCache = FALSE;
    size_t sUnexpirableResultsCount = 0;
    PAD_SECURITY_OBJECT* ppUnexpirableResults = NULL;
    size_t sResultsCount = 0;
    PAD_SECURITY_OBJECT* ppResults = NULL;
    size_t sFilteredResultsCount = 0;
    PAD_SECURITY_OBJECT* ppFilteredResults = NULL;
    // Only free top level array, do not free string pointers.
    PSTR* ppszSids = NULL;
    PAD_SECURITY_OBJECT pUserInfo = NULL;
    PCSTR pszSid = NULL;
    int iPrimaryGroupIndex = -1;

    dwError = AD_FindUserObjectById(
                    hProvider,
                    uid,
                    &pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    pszSid = pUserInfo->pszObjectSid;

    dwError = ADCacheDB_OpenDb(&hDb);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_GetGroupsForUser(
                    hDb,
                    pszSid,
                    AD_GetTrimUserMembershipEnabled(),
                    &sMembershipCount,
                    &ppMemberships);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_CheckExpiredMemberships(
                    sMembershipCount,
                    ppMemberships,
                    TRUE,
                    &bExpired,
                    &bIsComplete);
    BAIL_ON_LSA_ERROR(dwError);

    if (bExpired)
    {
        LSA_LOG_VERBOSE(
            "Cache entry for user's group membership for sid %s is expired",
            pszSid);
    }
    else if (!bIsComplete)
    {
        LSA_LOG_VERBOSE(
            "Cache entry for user's group membership for sid %s is incomplete",
            pszSid);
    }

    if (bExpired || !bIsComplete)
    {
        LSA_TRUST_DIRECTION dwTrustDirection = LSA_TRUST_DIRECTION_UNKNOWN;

        dwError = AD_DetermineTrustModeandDomainName(
                        pUserInfo->pszNetbiosDomainName,
                        &dwTrustDirection,
                        NULL,
                        NULL,
                        NULL);
        BAIL_ON_LSA_ERROR(dwError);

        if (dwTrustDirection == LSA_TRUST_DIRECTION_ONE_WAY)
        {
            bUseCache = TRUE;
        }
    }
    else
    {
        bUseCache = TRUE;
    }

    if (!bUseCache && bIsCacheOnlyMode)
    {
        dwError = AD_FilterExpiredMemberships(&sMembershipCount, ppMemberships);
        BAIL_ON_LSA_ERROR(dwError);

        bUseCache = TRUE;
    }

    if (!bUseCache)
    {
        // Get the complete object for the unexpirable entries (the ones
        // that we can't get through regular LDAP queries).  Later on, this
        // list will be appended to the list we get from LDAP.

        dwError = AD_GatherSidsFromGroupMemberships(
                        TRUE,
                        AD_IncludeOnlyUnexpirableGroupMembershipsCallback,
                        sMembershipCount,
                        ppMemberships,
                        &sUnexpirableResultsCount,
                        &ppszSids);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = AD_FindObjectsBySidList(
                        hProvider,
                        sUnexpirableResultsCount,
                        ppszSids,
                        &sUnexpirableResultsCount,
                        &ppUnexpirableResults);
        BAIL_ON_LSA_ERROR(dwError);

        LSA_SAFE_FREE_MEMORY(ppszSids);
        ADCacheDB_SafeFreeGroupMembershipList(sMembershipCount, &ppMemberships);

        dwError = ADLdap_GetUserGroupMembership(
                         hProvider,
                         pUserInfo,
                         &iPrimaryGroupIndex,
                         &sResultsCount,
                         &ppResults);
        BAIL_ON_LSA_ERROR(dwError);

        AD_FilterNullEntries(ppResults, &sResultsCount);

        dwError = AD_CacheMembershipFromRelatedObjects(
                        hDb,
                        pszSid,
                        iPrimaryGroupIndex,
                        FALSE,
                        sResultsCount,
                        ppResults);
        BAIL_ON_LSA_ERROR(dwError);

        // Combine and filter out duplicates.
        dwError = AD_CombineCacheObjectsRemoveDuplicates(
                        &sResultsCount,
                        ppResults,
                        &sUnexpirableResultsCount,
                        ppUnexpirableResults,
                        &sFilteredResultsCount,
                        &ppFilteredResults);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        /* If the cache is valid, get the rest of the object info with
         * AD_FindObjectsBySidList.
         */
        LSA_LOG_VERBOSE(
              "Using cached user's group membership for sid %s",
              pszSid);

        dwError = AD_GatherSidsFromGroupMemberships(
                        TRUE,
                        NULL,
                        sMembershipCount,
                        ppMemberships,
                        &sResultsCount,
                        &ppszSids);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = AD_FindObjectsBySidList(
                        hProvider,
                        sResultsCount,
                        ppszSids,
                        &sFilteredResultsCount,
                        &ppFilteredResults);
        BAIL_ON_LSA_ERROR(dwError);
    }

    *psCount = sFilteredResultsCount;
    *pppResults = ppFilteredResults;

cleanup:
    ADCacheDB_SafeCloseDb(&hDb);
    LSA_SAFE_FREE_MEMORY(ppszSids);
    ADCacheDB_SafeFreeGroupMembershipList(sMembershipCount, &ppMemberships);
    ADCacheDB_SafeFreeObjectList(sResultsCount, &ppResults);
    ADCacheDB_SafeFreeObjectList(sUnexpirableResultsCount, &ppUnexpirableResults);
    ADCacheDB_SafeFreeObject(&pUserInfo);

    return dwError;

error:
    *psCount = 0;
    *pppResults = NULL;

    LSA_LOG_ERROR("Failed to find memberships for uid %d (error = %d)",
                  uid, dwError);

    ADCacheDB_SafeFreeObjectList(sFilteredResultsCount, &ppFilteredResults);
    sFilteredResultsCount = 0;

    goto cleanup;
}

DWORD
AD_OnlineEnumUsers(
    HANDLE  hProvider,
    HANDLE  hResume,
    DWORD   dwMaxNumUsers,
    PDWORD  pdwUsersFound,
    PVOID** pppUserInfoList
    )
{
    DWORD dwError = 0;
    PAD_ENUM_STATE pEnumState = (PAD_ENUM_STATE)hResume;
    DWORD dwObjectsCount = 0;
    PAD_SECURITY_OBJECT* ppObjects = NULL;
    PVOID* ppInfoList = NULL;
    DWORD dwInfoCount = 0;

    // If BeginEnum was called in offline mode, it can successfully return
    // with hDirectory set to 0. That is the only way this function can
    // be called with hDirectory as 0. So we should continue enumerating
    // in offline mode.
    if (pEnumState->hDirectory == (HANDLE)NULL)
    {
        dwError = AD_OfflineEnumUsers(
                        hProvider,
                        hResume,
                        dwMaxNumUsers,
                        &dwInfoCount,
                        &ppInfoList);
        BAIL_ON_LSA_ERROR(dwError);

        goto cleanup;
    }

    dwError = LsaAdBatchEnumObjects(
                    pEnumState->hDirectory,
                    pEnumState->bMorePages,
                    &pEnumState->pCookie,
                    &pEnumState->bMorePages,
                    AccountType_User,
                    dwMaxNumUsers,
                    &dwObjectsCount,
                    &ppObjects);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAllocateMemory(sizeof(*ppInfoList) * dwObjectsCount,
                                (PVOID*)&ppInfoList);
    BAIL_ON_LSA_ERROR(dwError);

    for (dwInfoCount = 0; dwInfoCount < dwObjectsCount; dwInfoCount++)
    {
        dwError = ADMarshalFromUserCache(
                        ppObjects[dwInfoCount],
                        pEnumState->dwInfoLevel,
                        &ppInfoList[dwInfoCount]);
        BAIL_ON_LSA_ERROR(dwError);

        ADCacheDB_SafeFreeObject(&ppObjects[dwInfoCount]);
    }

cleanup:
    ADCacheDB_SafeFreeObjectList(dwObjectsCount, &ppObjects);

    *pdwUsersFound = dwInfoCount;
    *pppUserInfoList = ppInfoList;

    return dwError;

error:
    // need to set OUT params in cleanup due to goto cleanup.
    if (ppInfoList)
    {
        LsaFreeUserInfoList(pEnumState->dwInfoLevel, ppInfoList, dwInfoCount);
        ppInfoList = NULL;
        dwInfoCount = 0;
    }

    goto cleanup;
}

DWORD
AD_OnlineFindGroupObjectByName(
    HANDLE  hProvider,
    PCSTR   pszGroupName,
    PAD_SECURITY_OBJECT*  ppResult
    )
{
    DWORD dwError = 0;
    PLSA_LOGIN_NAME_INFO pGroupNameInfo = NULL;
    PSTR  pszGroupName_copy = NULL;
    HANDLE hDb = (HANDLE)NULL;
    PAD_SECURITY_OBJECT pCachedGroup = NULL;

    BAIL_ON_INVALID_STRING(pszGroupName);
    dwError = LsaAllocateString(
                    pszGroupName,
                    &pszGroupName_copy);
    BAIL_ON_LSA_ERROR(dwError);

    LsaStrCharReplace(pszGroupName_copy, AD_GetSpaceReplacement(),' ');

    dwError = LsaCrackDomainQualifiedName(
                        pszGroupName_copy,
                        gpADProviderData->szDomain,
                        &pGroupNameInfo);
    BAIL_ON_LSA_ERROR(dwError);

    if ((pGroupNameInfo->nameType == NameType_Alias) &&
    	!strcasecmp(pGroupNameInfo->pszName, "root"))
    {
    	dwError = LSA_ERROR_NO_SUCH_GROUP;
    	BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = ADCacheDB_OpenDb(&hDb);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_FindGroupByName(
            hDb,
            pGroupNameInfo,
            &pCachedGroup);
    if (dwError == LSA_ERROR_SUCCESS)
    {
        dwError = AD_CheckExpiredObject(&pCachedGroup);
    }

    if (dwError == 0)
    {
        goto cleanup;
    }

    if (dwError != LSA_ERROR_NOT_HANDLED){
        BAIL_ON_LSA_ERROR(dwError);
    }

    // Otherwise, look up the group
    dwError = AD_FindObjectByNameTypeNoCache(
                    hProvider,
                    pszGroupName_copy,
                    pGroupNameInfo->nameType,
                    AccountType_Group,
                    &pCachedGroup);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_CacheObjectEntry(hDb, pCachedGroup);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    if (dwError)
    {
        ADCacheDB_SafeFreeObject(&pCachedGroup);
        LSA_REMAP_FIND_GROUP_BY_NAME_ERROR(dwError, FALSE, pszGroupName);
    }
    *ppResult = pCachedGroup;

    ADCacheDB_SafeCloseDb(&hDb);
    if (pGroupNameInfo)
    {
        LsaFreeNameInfo(pGroupNameInfo);
    }
    LSA_SAFE_FREE_STRING(pszGroupName_copy);

    return dwError;

error:
    // Do not handle error here, but in cleanup, since there is a goto cleanup
    goto cleanup;
}

DWORD
AD_OnlineFindGroupById(
    IN HANDLE hProvider,
    IN gid_t gid,
    IN BOOLEAN bIsCacheOnlyMode,
    IN DWORD dwGroupInfoLevel,
    OUT PVOID* ppGroupInfo
    )
{
    DWORD dwError =  0;
    HANDLE hDb = (HANDLE)NULL;
    PAD_SECURITY_OBJECT pCachedGroup = NULL;
    PSTR   pszNT4Name = NULL;

    if (gid == 0)
    {
        dwError = LSA_ERROR_NO_SUCH_GROUP;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = ADCacheDB_OpenDb(&hDb);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_FindGroupById(
            hDb,
            gid,
            &pCachedGroup);
    if (dwError == LSA_ERROR_SUCCESS)
    {
        dwError = AD_CheckExpiredObject(&pCachedGroup);
    }

    if (dwError == LSA_ERROR_NOT_HANDLED)
    {
        dwError = AD_FindGroupByIdWithCacheMode(
                    hProvider,
                    gid,
                    bIsCacheOnlyMode,
                    dwGroupInfoLevel,
                    ppGroupInfo);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else if (dwError == 0)
    {
        //found in cache and not expired
        dwError = AD_GroupObjectToGroupInfo(
                    hProvider,
                    pCachedGroup,
                    bIsCacheOnlyMode,
                    dwGroupInfoLevel,
                    ppGroupInfo);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    LSA_SAFE_FREE_STRING(pszNT4Name);

    ADCacheDB_SafeCloseDb(&hDb);
    ADCacheDB_SafeFreeObject(&pCachedGroup);

    return dwError;

error:

    *ppGroupInfo = NULL;

    LSA_REMAP_FIND_GROUP_BY_ID_ERROR(dwError, FALSE, gid);

    goto cleanup;
}

DWORD
AD_OnlineEnumGroups(
    HANDLE  hProvider,
    HANDLE  hResume,
    DWORD   dwMaxNumGroups,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
    )
{
    DWORD dwError = 0;
    PAD_ENUM_STATE pEnumState = (PAD_ENUM_STATE)hResume;
    DWORD dwObjectsCount = 0;
    PAD_SECURITY_OBJECT* ppObjects = NULL;
    PVOID* ppInfoList = NULL;
    DWORD dwInfoCount = 0;

    // If BeginEnum was called in offline mode, it can successfully return
    // with hDirectory set to 0. That is the only way this function can
    // be called with hDirectory as 0. So we should continue enumerating
    // in offline mode.
    if (pEnumState->hDirectory == (HANDLE)NULL)
    {
        dwError = AD_OfflineEnumGroups(
                        hProvider,
                        hResume,
                        dwMaxNumGroups,
                        &dwInfoCount,
                        &ppInfoList);
        BAIL_ON_LSA_ERROR(dwError);

        goto cleanup;
    }

    dwError = LsaAdBatchEnumObjects(
                    pEnumState->hDirectory,
                    pEnumState->bMorePages,
                    &pEnumState->pCookie,
                    &pEnumState->bMorePages,
                    AccountType_Group,
                    dwMaxNumGroups,
                    &dwObjectsCount,
                    &ppObjects);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAllocateMemory(sizeof(*ppInfoList) * dwObjectsCount,
                                (PVOID*)&ppInfoList);
    BAIL_ON_LSA_ERROR(dwError);

    for (dwInfoCount = 0; dwInfoCount < dwObjectsCount; dwInfoCount++)
    {
        dwError = AD_GroupObjectToGroupInfo(
                        hProvider,
                        ppObjects[dwInfoCount],
                        TRUE,
                        pEnumState->dwInfoLevel,
                        &ppInfoList[dwInfoCount]);
        BAIL_ON_LSA_ERROR(dwError);

        ADCacheDB_SafeFreeObject(&ppObjects[dwInfoCount]);
    }

cleanup:
    ADCacheDB_SafeFreeObjectList(dwObjectsCount, &ppObjects);

    *pdwGroupsFound = dwInfoCount;
    *pppGroupInfoList = ppInfoList;

    return dwError;

error:
    // need to set OUT params in cleanup due to goto cleanup.
    if (ppInfoList)
    {
        LsaFreeGroupInfoList(pEnumState->dwInfoLevel, ppInfoList, dwInfoCount);
        ppInfoList = NULL;
        dwInfoCount = 0;
    }

    goto cleanup;
}

DWORD
AD_OnlineChangePassword(
    HANDLE hProvider,
    PCSTR pszLoginId,
    PCSTR pszPassword,
    PCSTR pszOldPassword
    )
{
    DWORD dwError = 0;
    PLSA_LOGIN_NAME_INFO pLoginInfo = NULL;
    DWORD dwUserInfoLevel = 2;
    PAD_SECURITY_OBJECT pCachedUser = NULL;
    PLSA_USER_INFO_2 pUserInfo = NULL;
    PSTR pszDomainController = NULL;
    PSTR pszFullDomainName = NULL;

    dwError = LsaCrackDomainQualifiedName(
                    pszLoginId,
                    gpADProviderData->szDomain,
                    &pLoginInfo);
    BAIL_ON_LSA_ERROR(dwError);

    if (!AD_ServicesDomain(pLoginInfo->pszDomainNetBiosName))
    {
        dwError = LSA_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = AD_FindUserObjectByName(
                     hProvider,
                     pszLoginId,
                     &pCachedUser);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADMarshalFromUserCache(
            pCachedUser,
            dwUserInfoLevel,
            (PVOID*)&pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    //
    // TODO: Check if the peer uid belongs in the
    //       Domain Admins groups in which case, we
    //       should allow the password change
   /* if (pContext->uid != pUserInfo->uid) {
        dwError = EACCES;
        BAIL_ON_LSA_ERROR(dwError);
    }*/

    if (!pUserInfo->bUserCanChangePassword) {
        dwError = LSA_ERROR_USER_CANNOT_CHANGE_PASSWD;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserInfo->bAccountDisabled) {
        dwError = LSA_ERROR_ACCOUNT_DISABLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserInfo->bAccountExpired) {
        dwError = LSA_ERROR_ACCOUNT_EXPIRED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserInfo->bAccountLocked) {
        dwError = LSA_ERROR_ACCOUNT_LOCKED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    // Make sure that we are affinitized.
    dwError = LsaDmWrapGetDomainName(
                        pCachedUser->pszNetbiosDomainName,
                        &pszFullDomainName,
                        NULL);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LWNetGetDomainController(pszFullDomainName,
                                       &pszDomainController);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_NetUserChangePassword(pszDomainController,
                                       pCachedUser->pszSamAccountName,
                                       pszOldPassword,
                                       pszPassword);
    BAIL_ON_LSA_ERROR(dwError);

    if (AD_EventlogEnabled())
    {
        LsaSrvLogUserPWChangeSuccessEvent(
                pszLoginId,
                gpszADProviderName);
    }

    // Ignore errors because password change succeeded
    LsaUmModifyUser(
        pUserInfo->uid,
        pszPassword);

cleanup:
    if (pszDomainController)
    {
        LWNetFreeString(pszDomainController);
    }

    if (pLoginInfo)
    {
        LsaFreeNameInfo(pLoginInfo);
    }

    if (pUserInfo) {
        LsaFreeUserInfo(dwUserInfoLevel, (PVOID)pUserInfo);
    }

    ADCacheDB_SafeFreeObject(&pCachedUser);

    LSA_SAFE_FREE_STRING(pszFullDomainName);


    return dwError;

error:

    if (AD_EventlogEnabled())
    {
        LsaSrvLogUserPWChangeFailureEvent(
                pszLoginId,
                gpszADProviderName,
                dwError);
    }

    goto cleanup;
}

DWORD
AD_CreateHomeDirectory(
    PLSA_USER_INFO_1 pUserInfo
    )
{
    DWORD dwError = 0;
    BOOLEAN bExists = FALSE;

    if (IsNullOrEmptyString(pUserInfo->pszHomedir)) {
        dwError = LSA_ERROR_FAILED_CREATE_HOMEDIR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaCheckDirectoryExists(
                    pUserInfo->pszHomedir,
                    &bExists);
    BAIL_ON_LSA_ERROR(dwError);

    if (!bExists && AD_ShouldCreateHomeDir()) {
        dwError = AD_CreateHomeDirectory_Generic(pUserInfo);
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    return dwError;

error:

    LSA_LOG_ERROR("Failed to create home directory for user (%s), actual error %d", IsNullOrEmptyString(pUserInfo->pszName) ? "" : pUserInfo->pszName, dwError);
    dwError = LSA_ERROR_FAILED_CREATE_HOMEDIR;

    goto cleanup;
}

DWORD
AD_CreateHomeDirectory_Generic(
    PLSA_USER_INFO_1 pUserInfo
    )
{
    DWORD dwError = 0;
    mode_t  umask = 0;
    mode_t  perms = (S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);
    BOOLEAN bRemoveDir = FALSE;

    umask = AD_GetUmask();

    dwError = LsaCreateDirectory(
                 pUserInfo->pszHomedir,
                 perms);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaChangePermissions(
                 pUserInfo->pszHomedir,
                 perms & (~umask));
    BAIL_ON_LSA_ERROR(dwError);

    bRemoveDir = TRUE;

    dwError = LsaChangeOwner(
                 pUserInfo->pszHomedir,
                 pUserInfo->uid,
                 pUserInfo->gid);
    BAIL_ON_LSA_ERROR(dwError);

    bRemoveDir = FALSE;

    dwError = AD_ProvisionHomeDir(
                    pUserInfo->uid,
                    pUserInfo->gid,
                    pUserInfo->pszHomedir);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    return dwError;

error:

    if (bRemoveDir) {
       LsaRemoveDirectory(pUserInfo->pszHomedir);
    }

    LSA_LOG_ERROR("Failed to create home directory for user (%s), actual error %d", pUserInfo->pszName, dwError);
    dwError = LSA_ERROR_FAILED_CREATE_HOMEDIR;

    goto cleanup;
}

DWORD
AD_ProvisionHomeDir(
    uid_t ownerUid,
    gid_t ownerGid,
    PCSTR pszHomedirPath
    )
{
    DWORD dwError = 0;
    BOOLEAN bExists = FALSE;
    PSTR pszSkelPaths = NULL;
    PSTR pszSkelPath = NULL;
    PSTR pszIter = NULL;
    size_t stLen = 0;

    dwError = AD_GetSkelDirs(&pszSkelPaths);
    BAIL_ON_LSA_ERROR(dwError);

    if (IsNullOrEmptyString(pszSkelPaths))
    {
        goto cleanup;
    }

    pszIter = pszSkelPaths;
    while ((stLen = strcspn(pszIter, ",")) != 0)
    {
        dwError = LsaStrndup(
                      pszIter,
                      stLen,
                      &pszSkelPath);
        BAIL_ON_LSA_ERROR(dwError);

        LsaStripWhitespace(pszSkelPath, TRUE, TRUE);

        if (IsNullOrEmptyString(pszSkelPath))
        {
            LSA_SAFE_FREE_STRING(pszSkelPath);
            continue;
        }

        dwError = LsaCheckDirectoryExists(
                        pszSkelPath,
                        &bExists);
        BAIL_ON_LSA_ERROR(dwError);

        if (bExists)
        {
            dwError = LsaCopyDirectory(
                        pszSkelPath,
                        ownerUid,
                        ownerGid,
                        pszHomedirPath);
            BAIL_ON_LSA_ERROR(dwError);
        }

        LSA_SAFE_FREE_STRING(pszSkelPath);

        pszIter += stLen;
        stLen = strspn(pszIter, ",");
        pszIter += stLen;
    }

cleanup:

    LSA_SAFE_FREE_STRING(pszSkelPath);
    LSA_SAFE_FREE_STRING(pszSkelPaths);

    return dwError;

error:

    goto cleanup;
}

DWORD
AD_CreateK5Login(
    PLSA_USER_INFO_1 pUserInfo
    )
{
    DWORD   dwError = 0;
    PSTR    pszK5LoginPath = NULL;
    PSTR    pszK5LoginPath_tmp = NULL;
    PSTR    pszData = NULL;
    BOOLEAN bExists = FALSE;
    PSTR pszUpnCopy = NULL;
    PSTR pszUpnCopyLower = NULL;
    int     fd = -1;
    BOOLEAN bRemoveFile = FALSE;

    BAIL_ON_INVALID_STRING(pUserInfo->pszHomedir);
    BAIL_ON_INVALID_STRING(pUserInfo->pszUPN);

    dwError = LsaAllocateStringPrintf(
                    &pszK5LoginPath,
                    "%s/.k5login",
                    pUserInfo->pszHomedir);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaCheckFileExists(
                    pszK5LoginPath,
                    &bExists);
    BAIL_ON_LSA_ERROR(dwError);

    if (bExists) {
        goto cleanup;
    }

    // Create a copy of the UPN to make sure that the realm is uppercase,
    // but preserving the case of the non-realm part.
    dwError = LsaAllocateString(
                    pUserInfo->pszUPN,
                    &pszUpnCopy);
    BAIL_ON_LSA_ERROR(dwError);

    LsaPrincipalRealmToUpper(pszUpnCopy);

    // Create another copy of the UPN that has lowercase non-realm part.
    dwError = LsaAllocateString(
                    pszUpnCopy,
                    &pszUpnCopyLower);
    BAIL_ON_LSA_ERROR(dwError);

    LsaPrincipalNonRealmToLower(pszUpnCopyLower);

    if (!strcmp(pszUpnCopy, pszUpnCopyLower))
    {
        // If the UPNs are the same, just need to write one.
        dwError = LsaAllocateStringPrintf(
                        &pszData,
                        "%s\n",
                        pszUpnCopy);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        // Otherwise, they are different and we want both.
        dwError = LsaAllocateStringPrintf(
                        &pszData,
                        "%s\n%s\n",
                        pszUpnCopy,
                        pszUpnCopyLower);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaAllocateStringPrintf(
                    &pszK5LoginPath_tmp,
                    "%s_lsass",
                    pszK5LoginPath);
    BAIL_ON_LSA_ERROR(dwError);

    fd = open(
            pszK5LoginPath_tmp,
            O_CREAT|O_WRONLY|O_EXCL,
            S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    if (fd < 0) {
        if (errno == EEXIST) {
            goto cleanup;
        } else {
            dwError = errno;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    bRemoveFile = TRUE;

    dwError = LsaWriteData(
                    fd,
                    pszData,
                    strlen(pszData));
    BAIL_ON_LSA_ERROR(dwError);

    close(fd);
    fd = -1;

    dwError = LsaMoveFile(
                    pszK5LoginPath_tmp,
                    pszK5LoginPath);
    BAIL_ON_LSA_ERROR(dwError);

    bRemoveFile = FALSE;

    dwError = LsaChangeOwnerAndPermissions(
                    pszK5LoginPath,
                    pUserInfo->uid,
                    pUserInfo->gid,
                    S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH
                    );
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    if (fd >= 0) {
        close(fd);
    }

    if (bRemoveFile) {

        DWORD dwError2 = LsaRemoveFile(pszK5LoginPath_tmp);
        if (dwError2) {
            LSA_LOG_ERROR("Failed to remove file at [%s][Error code: %d]",
                          pszK5LoginPath_tmp,
                          dwError2);
        }

    }

    LSA_SAFE_FREE_STRING(pszData);
    LSA_SAFE_FREE_STRING(pszUpnCopy);
    LSA_SAFE_FREE_STRING(pszUpnCopyLower);
    LSA_SAFE_FREE_STRING(pszK5LoginPath_tmp);
    LSA_SAFE_FREE_STRING(pszK5LoginPath);

    return dwError;

error:

    goto cleanup;
}

int
AD_CompareObjectSids(
        PCVOID pObjectA,
        PCVOID pObjectB)
{
    return strcasecmp(
            ((PAD_SECURITY_OBJECT)pObjectA)->pszObjectSid,
            ((PAD_SECURITY_OBJECT)pObjectB)->pszObjectSid);
}

size_t
AD_HashObjectSid(
        PCVOID pObject)
{
    return LsaHashCaselessString(((PAD_SECURITY_OBJECT)pObject)->pszObjectSid);
}

void
AD_FreeHashObject(
    const LSA_HASH_ENTRY *pEntry)
{
    ADCacheDB_SafeFreeObject((PAD_SECURITY_OBJECT *)&pEntry->pKey);
}

DWORD
AD_OnlineGetGroupMembers(
    IN HANDLE hProvider,
    IN PCSTR pszDomainName,
    IN PCSTR pszSid,
    IN BOOLEAN bIsCacheOnlyMode,
    OUT size_t* psCount,
    OUT PAD_SECURITY_OBJECT** pppResults
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    HANDLE hDb = (HANDLE)NULL;
    size_t sMembershipCount = 0;
    PAD_GROUP_MEMBERSHIP* ppMemberships = NULL;
    BOOLEAN bExpired = FALSE;
    BOOLEAN bIsComplete = FALSE;
    BOOLEAN bUseCache = FALSE;
    size_t sUnexpirableResultsCount = 0;
    PAD_SECURITY_OBJECT* ppUnexpirableResults = NULL;
    size_t sResultsCount = 0;
    PAD_SECURITY_OBJECT* ppResults = NULL;
    size_t sFilteredResultsCount = 0;
    PAD_SECURITY_OBJECT* ppFilteredResults = NULL;
    // Only free top level array, do not free string pointers.
    PSTR* ppszSids = NULL;

    dwError = ADCacheDB_OpenDb(&hDb);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_GetGroupMembers(
                    hDb,
                    pszSid,
                    AD_GetTrimUserMembershipEnabled(),
                    &sMembershipCount,
                    &ppMemberships);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_CheckExpiredMemberships(
                    sMembershipCount,
                    ppMemberships,
                    FALSE,
                    &bExpired,
                    &bIsComplete);
    BAIL_ON_LSA_ERROR(dwError);

    if (bExpired)
    {
        LSA_LOG_VERBOSE(
            "Cache entry for group membership for sid %s is expired",
            pszSid);
    }
    else if (!bIsComplete)
    {
        LSA_LOG_VERBOSE(
            "Cache entry for group membership for sid %s is incomplete",
            pszSid);
    }

    if (bExpired || !bIsComplete)
    {
        LSA_TRUST_DIRECTION dwTrustDirection = LSA_TRUST_DIRECTION_UNKNOWN;

        dwError = AD_DetermineTrustModeandDomainName(
                        pszDomainName,
                        &dwTrustDirection,
                        NULL,
                        NULL,
                        NULL);
        BAIL_ON_LSA_ERROR(dwError);

        if (dwTrustDirection == LSA_TRUST_DIRECTION_ONE_WAY)
        {
            bUseCache = TRUE;
        }
    }
    else
    {
        bUseCache = TRUE;
    }

    if (!bUseCache && bIsCacheOnlyMode)
    {
        dwError = AD_FilterExpiredMemberships(&sMembershipCount, ppMemberships);
        BAIL_ON_LSA_ERROR(dwError);

        bUseCache = TRUE;
    }

    if (!bUseCache)
    {
        // Get the complete object for the unexpirable entries (the ones
        // that we can't get through regular LDAP queries).  Later on, this
        // list will be appended to the list we get from LDAP.

        dwError = AD_GatherSidsFromGroupMemberships(
                        FALSE,
                        AD_IncludeOnlyUnexpirableGroupMembershipsCallback,
                        sMembershipCount,
                        ppMemberships,
                        &sUnexpirableResultsCount,
                        &ppszSids);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = AD_FindObjectsBySidList(
                        hProvider,
                        sUnexpirableResultsCount,
                        ppszSids,
                        &sUnexpirableResultsCount,
                        &ppUnexpirableResults);
        BAIL_ON_LSA_ERROR(dwError);

        LSA_SAFE_FREE_MEMORY(ppszSids);
        ADCacheDB_SafeFreeGroupMembershipList(sMembershipCount, &ppMemberships);

        dwError = ADLdap_GetGroupMembers(
                        hProvider,
                        pszDomainName,
                        pszSid,
                        &sResultsCount,
                        &ppResults);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = AD_CacheMembershipFromRelatedObjects(
                        hDb,
                        pszSid,
                        -1,
                        TRUE,
                        sResultsCount,
                        ppResults);
        BAIL_ON_LSA_ERROR(dwError);

        // Combine and let the caller will filter out duplicates.
        dwError = AD_CombineCacheObjects(
                        &sResultsCount,
                        ppResults,
                        &sUnexpirableResultsCount,
                        ppUnexpirableResults,
                        &sFilteredResultsCount,
                        &ppFilteredResults);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        /* If the cache is valid, get the rest of the object info with
         * AD_FindObjectsBySidList.
         */
        LSA_LOG_VERBOSE(
              "Using cached group membership for sid %s",
              pszSid);

        dwError = AD_GatherSidsFromGroupMemberships(
                        FALSE,
                        NULL,
                        sMembershipCount,
                        ppMemberships,
                        &sResultsCount,
                        &ppszSids);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = AD_FindObjectsBySidList(
                        hProvider,
                        sResultsCount,
                        ppszSids,
                        &sFilteredResultsCount,
                        &ppFilteredResults);
        BAIL_ON_LSA_ERROR(dwError);
    }

    *psCount = sFilteredResultsCount;
    *pppResults = ppFilteredResults;

cleanup:
    ADCacheDB_SafeCloseDb(&hDb);
    LSA_SAFE_FREE_MEMORY(ppszSids);
    ADCacheDB_SafeFreeGroupMembershipList(sMembershipCount, &ppMemberships);
    ADCacheDB_SafeFreeObjectList(sResultsCount, &ppResults);
    ADCacheDB_SafeFreeObjectList(sUnexpirableResultsCount, &ppUnexpirableResults);

    return dwError;

error:
    *psCount = 0;
    *pppResults = NULL;

    ADCacheDB_SafeFreeObjectList(sFilteredResultsCount, &ppFilteredResults);
    sFilteredResultsCount = 0;

    goto cleanup;
}

static
DWORD
AD_FindObjectBySidNoCache(
    IN HANDLE hProvider,
    IN PCSTR pszSid,
    OUT PAD_SECURITY_OBJECT* ppObject
    )
{
    return LsaAdBatchFindSingleObject(
                LSA_AD_BATCH_QUERY_TYPE_BY_SID,
                pszSid,
                NULL,
                ppObject);
}

static
DWORD
AD_FindObjectByNT4NameNoCache(
    IN HANDLE hProvider,
    IN PCSTR pszNT4Name,
    OUT PAD_SECURITY_OBJECT* ppObject
    )
{
    return LsaAdBatchFindSingleObject(
                LSA_AD_BATCH_QUERY_TYPE_BY_NT4,
                pszNT4Name,
                NULL,
                ppObject);
}

static
DWORD
AD_FindObjectByUpnNoCache(
    IN HANDLE hProvider,
    IN PCSTR pszUpn,
    OUT PAD_SECURITY_OBJECT* ppObject
    )
{
    DWORD dwError = 0;
    PSTR pszSid = NULL;
    PAD_SECURITY_OBJECT pObject = NULL;

    dwError = LsaDmWrapNetLookupObjectSidByName(
                    gpADProviderData->szDomain,
                    pszUpn,
                    &pszSid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_FindObjectBySidNoCache(
                    hProvider,
                    pszSid,
                    &pObject);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    LSA_SAFE_FREE_STRING(pszSid);

    *ppObject = pObject;

    return dwError;

error:
    ADCacheDB_SafeFreeObject(&pObject);
    goto cleanup;
}

static
DWORD
AD_FindObjectByAliasNoCache(
    IN HANDLE hProvider,
    IN PCSTR pszAlias,
    BOOLEAN bIsUserAlias,
    OUT PAD_SECURITY_OBJECT* ppResult
    )
{
    return LsaAdBatchFindSingleObject(
                   bIsUserAlias ? LSA_AD_BATCH_QUERY_TYPE_BY_USER_ALIAS : LSA_AD_BATCH_QUERY_TYPE_BY_GROUP_ALIAS,
                   pszAlias,
                   NULL,
                   ppResult);
}

DWORD
AD_FindObjectByNameTypeNoCache(
    IN HANDLE hProvider,
    IN PCSTR pszName,
    IN ADLogInNameType NameType,
    IN ADAccountType AccountType,
    OUT PAD_SECURITY_OBJECT* ppObject
    )
{
    DWORD dwError = 0;
    BOOLEAN bIsUser = FALSE;
    PAD_SECURITY_OBJECT pObject = NULL;

    switch (AccountType)
    {
        case AccountType_User:
            bIsUser = TRUE;
            break;
        case AccountType_Group:
            bIsUser = FALSE;
            break;
        default:
            LSA_ASSERT(FALSE);
            dwError = LSA_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

    switch (NameType)
    {
        case NameType_NT4:
            dwError = AD_FindObjectByNT4NameNoCache(
                            hProvider,
                            pszName,
                            &pObject);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        case NameType_UPN:
            dwError = AD_FindObjectByUpnNoCache(
                            hProvider,
                            pszName,
                            &pObject);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        case NameType_Alias:
            dwError = AD_FindObjectByAliasNoCache(
                            hProvider,
                            pszName,
                            bIsUser,
                            &pObject);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            LSA_ASSERT(FALSE);
            dwError = LSA_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

    // Check whether the object we find is correct type or not
    if (AccountType != pObject->type)
    {
        dwError = bIsUser ? LSA_ERROR_NO_SUCH_USER : LSA_ERROR_NO_SUCH_GROUP;
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    *ppObject = pObject;

    return dwError;

error:
    if (LSA_ERROR_NO_SUCH_USER_OR_GROUP == dwError)
    {
        dwError = bIsUser ? LSA_ERROR_NO_SUCH_USER : LSA_ERROR_NO_SUCH_GROUP;
    }
    ADCacheDB_SafeFreeObject(&pObject);
    goto cleanup;
}

DWORD
AD_FindObjectByIdTypeNoCache(
    IN HANDLE hProvider,
    IN DWORD dwId,
    IN ADAccountType AccountType,
    OUT PAD_SECURITY_OBJECT* ppObject
    )
{
    DWORD dwError = 0;
    BOOLEAN bIsUser = FALSE;
    PAD_SECURITY_OBJECT pObject = NULL;

    switch (AccountType)
    {
        case AccountType_User:
            bIsUser = TRUE;
            dwError = LsaAdBatchFindSingleObject(
                           LSA_AD_BATCH_QUERY_TYPE_BY_UID,
                           NULL,
                           &dwId,
                           &pObject);
            BAIL_ON_LSA_ERROR(dwError);
            break;

        case AccountType_Group:
            bIsUser = FALSE;
            dwError = LsaAdBatchFindSingleObject(
                           LSA_AD_BATCH_QUERY_TYPE_BY_GID,
                           NULL,
                           &dwId,
                           &pObject);
            BAIL_ON_LSA_ERROR(dwError);
            break;

        default:
            LSA_ASSERT(FALSE);
            dwError = LSA_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

    // Check whether the object we find is correct type or not
    if (AccountType != pObject->type)
    {
        dwError = bIsUser ? LSA_ERROR_NO_SUCH_USER : LSA_ERROR_NO_SUCH_GROUP;
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    *ppObject = pObject;

    return dwError;

error:
    if (LSA_ERROR_NO_SUCH_USER_OR_GROUP == dwError)
    {
        dwError = bIsUser ? LSA_ERROR_NO_SUCH_USER : LSA_ERROR_NO_SUCH_GROUP;
    }
    ADCacheDB_SafeFreeObject(&pObject);
    goto cleanup;
}

DWORD
AD_FindObjectsByListNoCache(
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN DWORD dwCount,
    IN PSTR* ppszList,
    OUT PDWORD pdwCount,
    OUT PAD_SECURITY_OBJECT** pppObjects
    )
{
    return LsaAdBatchFindObjects(
                QueryType,
                dwCount,
                ppszList,
                NULL,
                pdwCount,
                pppObjects);
}

DWORD
AD_FindObjectBySid(
    IN HANDLE hProvider,
    IN PCSTR pszSid,
    OUT PAD_SECURITY_OBJECT* ppResult
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    PAD_SECURITY_OBJECT* ppResultArray = NULL;

    dwError = AD_FindObjectsBySidList(
                    hProvider,
                    1,
                    (PSTR*)&pszSid,
                    NULL,
                    &ppResultArray);
    BAIL_ON_LSA_ERROR(dwError);

    if (ppResultArray && !ppResultArray[0])
    {
        dwError = LSA_ERROR_NO_SUCH_USER_OR_GROUP;
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppResult = ppResultArray[0];

cleanup:
    LSA_SAFE_FREE_MEMORY(ppResultArray);
    return dwError;

error:
    *ppResult = NULL;
    ADCacheDB_SafeFreeObjectList(1, &ppResultArray);
    goto cleanup;
}

DWORD
AD_FindObjectsByList(
    IN LSA_AD_CACHEDB_FIND_OBJECTS_BY_LIST_CALLBACK pFindInCacheCallback,
    IN LSA_AD_LDAP_FIND_OBJECTS_BY_LIST_BATCHED_CALLBACK pFindByListBatchedCallback,
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN size_t sCount,
    IN PSTR* ppszList,
    OUT OPTIONAL size_t* psResultsCount,
    OUT PAD_SECURITY_OBJECT** pppResults
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    PAD_SECURITY_OBJECT* ppResults = NULL;
    size_t sResultsCount = 0;
    size_t sFoundInCache = 0;
    size_t sFoundInAD = 0;
    DWORD  dwFoundInAD = 0;
    HANDLE hDb = 0;
    size_t sRemainNumsToFoundInAD = 0;
    size_t sIndex;
    time_t now = 0;
    // Do not free the strings that ppszRemainSidsList point to
    PSTR* ppszRemainingList = NULL;
    PAD_SECURITY_OBJECT *ppRemainingObjectsResults = NULL;

    dwError = LsaGetCurrentTimeSeconds(&now);
    BAIL_ON_LSA_ERROR(dwError);
    /*
     * Lookup as many objects as possible from the cache.
     */
    dwError = ADCacheDB_OpenDb(&hDb);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = pFindInCacheCallback(
                    hDb,
                    sCount,
                    ppszList,
                    &ppResults);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAllocateMemory(
                    sCount*sizeof(*ppszRemainingList),
                    (PVOID*)&ppszRemainingList);
    BAIL_ON_LSA_ERROR(dwError);

    for (sFoundInCache = 0, sRemainNumsToFoundInAD = 0, sIndex = 0;
         sIndex < sCount;
         sIndex++)
    {
        if ((ppResults[sIndex] != NULL) &&
            (ppResults[sIndex]->cache.tLastUpdated >= 0) &&
            (ppResults[sIndex]->cache.tLastUpdated +
            AD_GetCacheEntryExpirySeconds() <= now))
        {
            switch (QueryType)
            {
                case LSA_AD_BATCH_QUERY_TYPE_BY_SID:
                    LSA_LOG_VERBOSE("Cache entry for Sid %s is expired",
                         LSA_SAFE_LOG_STRING(ppResults[sIndex]->pszObjectSid));

                    break;

                case LSA_AD_BATCH_QUERY_TYPE_BY_DN:
                    LSA_LOG_VERBOSE("Cache entry for DN %s is expired",
                         LSA_SAFE_LOG_STRING(ppResults[sIndex]->pszDN));

                    break;

                default:
                    LSA_ASSERT(FALSE);
                    dwError = LSA_ERROR_INVALID_PARAMETER;
                    BAIL_ON_LSA_ERROR(dwError);
            }
            ADCacheDB_SafeFreeObject(&ppResults[sIndex]);
        }

        if (ppResults[sIndex] != NULL)
        {
            sFoundInCache++;
            continue;
        }
        ppszRemainingList[sRemainNumsToFoundInAD++] = ppszList[sIndex];
    }

    sResultsCount = sCount;
    AD_FilterNullEntries(ppResults, &sResultsCount);
    assert(sResultsCount == sFoundInCache);

    if (!sRemainNumsToFoundInAD)
    {
        goto cleanup;
    }

    dwError = pFindByListBatchedCallback(
                     QueryType,
                     sRemainNumsToFoundInAD,
                     ppszRemainingList,
                     &dwFoundInAD,
                     &ppRemainingObjectsResults);
    BAIL_ON_LSA_ERROR(dwError);

    sFoundInAD = dwFoundInAD;

    sResultsCount += sFoundInAD;

    dwError = ADCacheDB_CacheObjectEntries(
                    hDb,
                    sFoundInAD,
                    ppRemainingObjectsResults);
    BAIL_ON_LSA_ERROR(dwError);

    memcpy(ppResults + sFoundInCache,
           ppRemainingObjectsResults,
           sizeof(*ppRemainingObjectsResults) * sFoundInAD);

    memset(ppRemainingObjectsResults,
           0,
           sizeof(*ppRemainingObjectsResults) * sFoundInAD);

cleanup:

    // We handle error here instead of error label
    // because there is a goto cleanup above.
    if (dwError)
    {
        ADCacheDB_SafeFreeObjectList(sCount, &ppResults);
        sResultsCount = 0;
    }
    else
    {
        assert(sResultsCount == (sFoundInCache + sFoundInAD));
    }

    *pppResults = ppResults;
    if (psResultsCount)
    {
        *psResultsCount = sResultsCount;
    }

    ADCacheDB_SafeCloseDb(&hDb);

    ADCacheDB_SafeFreeObjectList(sFoundInAD, &ppRemainingObjectsResults);
    LSA_SAFE_FREE_MEMORY(ppszRemainingList);

    return dwError;

error:

    // Do not actually handle any errors here.
    goto cleanup;
}

DWORD
AD_FindObjectsBySidList(
    IN HANDLE hProvider,
    IN size_t sCount,
    IN PSTR* ppszSidList,
    OUT OPTIONAL size_t* psResultsCount,
    OUT PAD_SECURITY_OBJECT** pppResults
    )
{
    return AD_FindObjectsByList(
               ADCacheDB_FindObjectsBySidList,
               AD_FindObjectsByListNoCache,
               LSA_AD_BATCH_QUERY_TYPE_BY_SID,
               sCount,
               ppszSidList,
               psResultsCount,
               pppResults);
}

DWORD
AD_FindObjectsByDNList(
    IN HANDLE hProvider,
    IN size_t sCount,
    IN PSTR* ppszDNList,
    OUT OPTIONAL size_t* psResultsCount,
    OUT PAD_SECURITY_OBJECT** pppResults
    )
{
    return AD_FindObjectsByList(
               ADCacheDB_FindObjectsByDNList,
               AD_FindObjectsByListNoCache,
               LSA_AD_BATCH_QUERY_TYPE_BY_DN,
               sCount,
               ppszDNList,
               psResultsCount,
               pppResults);
}

DWORD
AD_OnlineGetNamesBySidList(
    HANDLE          hProvider,
    size_t          sCount,
    PSTR*           ppszSidList,
    PSTR**          pppszDomainNames,
    PSTR**          pppszSamAccounts,
    ADAccountType** ppTypes)
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    PSTR* ppszDomainNames = NULL;
    PSTR* ppszSamAccounts = NULL;
    ADAccountType* pTypes = NULL;
    PAD_SECURITY_OBJECT* ppObjects = NULL;
    size_t sIndex = 0;

    dwError = LsaAllocateMemory(
                    sizeof(*ppszDomainNames) * sCount,
                    (PVOID*)&ppszDomainNames);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAllocateMemory(
                    sizeof(*ppszSamAccounts) * sCount,
                    (PVOID*)&ppszSamAccounts);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAllocateMemory(
                    sizeof(*pTypes) * sCount,
                    (PVOID*)&pTypes);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_FindObjectsBySidList(
                    hProvider,
                    sCount,
                    ppszSidList,
                    NULL,
                    &ppObjects);
    BAIL_ON_LSA_ERROR(dwError);

    for (sIndex = 0; sIndex < sCount; sIndex++)
    {
        if (ppObjects[sIndex] == NULL)
        {
            pTypes[sIndex] = AccountType_NotFound;
            continue;
        }

        if (!IsNullOrEmptyString(ppObjects[sIndex]->pszNetbiosDomainName))
        {
            dwError = LsaAllocateString(
                        ppObjects[sIndex]->pszNetbiosDomainName,
                        &ppszDomainNames[sIndex]);
            BAIL_ON_LSA_ERROR(dwError);
        }

        if (!IsNullOrEmptyString(ppObjects[sIndex]->pszSamAccountName))
        {
            dwError = LsaAllocateString(
                        ppObjects[sIndex]->pszSamAccountName,
                        &ppszSamAccounts[sIndex]);
            BAIL_ON_LSA_ERROR(dwError);
        }

        pTypes[sIndex] = ppObjects[sIndex]->type;
    }

    *pppszDomainNames = ppszDomainNames;
    *pppszSamAccounts = ppszSamAccounts;
    *ppTypes = pTypes;

cleanup:

    ADCacheDB_SafeFreeObjectList(sCount, &ppObjects);

    return dwError;

error:

    *pppszDomainNames = NULL;
    *pppszSamAccounts = NULL;
    *ppTypes = NULL;

    LsaFreeStringArray(ppszDomainNames, sCount);
    LsaFreeStringArray(ppszSamAccounts, sCount);
    LSA_SAFE_FREE_MEMORY(pTypes);

    goto cleanup;
}

DWORD
AD_OnlineFindUserObjectByName(
    HANDLE  hProvider,
    PCSTR   pszLoginId,
    PAD_SECURITY_OBJECT* ppCachedUser
    )
{
    DWORD dwError = 0;
    PLSA_LOGIN_NAME_INFO pUserNameInfo = NULL;
    PSTR  pszLoginId_copy = NULL;
    HANDLE hDb = (HANDLE)NULL;
    PAD_SECURITY_OBJECT pCachedUser = NULL;

    BAIL_ON_INVALID_STRING(pszLoginId);
    dwError = LsaAllocateString(
                    pszLoginId,
                    &pszLoginId_copy);
    BAIL_ON_LSA_ERROR(dwError);

    LsaStrCharReplace(
            pszLoginId_copy,
            AD_GetSpaceReplacement(),
            ' ');

    dwError = LsaCrackDomainQualifiedName(
                        pszLoginId_copy,
                        gpADProviderData->szDomain,
                        &pUserNameInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_OpenDb(&hDb);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_FindUserByName(
            hDb,
            pUserNameInfo,
            &pCachedUser);
    if (dwError == LSA_ERROR_SUCCESS)
    {
        dwError = AD_CheckExpiredObject(&pCachedUser);
    }

    if (dwError == 0){
        goto cleanup;
    }

    if (dwError != LSA_ERROR_NOT_HANDLED){
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = AD_FindObjectByNameTypeNoCache(
                    hProvider,
                    pszLoginId_copy,
                    pUserNameInfo->nameType,
                    AccountType_User,
                    &pCachedUser);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_CacheObjectEntry(hDb, pCachedUser);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    if (dwError)
    {
        ADCacheDB_SafeFreeObject(&pCachedUser);
        LSA_REMAP_FIND_USER_BY_NAME_ERROR(dwError, FALSE, pszLoginId);
    }
    *ppCachedUser = pCachedUser;

    ADCacheDB_SafeCloseDb(&hDb);
    if (pUserNameInfo)
    {
        LsaFreeNameInfo(pUserNameInfo);
    }
    LSA_SAFE_FREE_STRING(pszLoginId_copy);

    return dwError;

error:
    // Do not actually handle error here, handle error in cleanup, since there is 'goto cleanup'
    goto cleanup;
}

DWORD
AD_OnlineFindNSSArtefactByKey(
    HANDLE hProvider,
    PCSTR  pszKeyName,
    PCSTR  pszMapName,
    DWORD  dwInfoLevel,
    LSA_NIS_MAP_QUERY_FLAGS dwFlags,
    PVOID* ppNSSArtefactInfo
    )
{
    DWORD dwError = 0;
    HANDLE hDirectory = (HANDLE)NULL;

    dwError = LsaDmWrapLdapOpenDirectoryDomain(gpADProviderData->szDomain,
                                               &hDirectory);
    BAIL_ON_LSA_ERROR(dwError);

    switch (gpADProviderData->dwDirectoryMode)
    {
    case DEFAULT_MODE:

        dwError = DefaultModeFindNSSArtefactByKey(
                        hDirectory,
                        gpADProviderData->cell.szCellDN,
                        gpADProviderData->szShortDomain,
                        pszKeyName,
                        pszMapName,
                        dwInfoLevel,
                        dwFlags,
                        ppNSSArtefactInfo);
        break;

    case CELL_MODE:

        dwError = CellModeFindNSSArtefactByKey(
                        hDirectory,
                        gpADProviderData->cell.szCellDN,
                        gpADProviderData->szShortDomain,
                        pszKeyName,
                        pszMapName,
                        dwInfoLevel,
                        dwFlags,
                        ppNSSArtefactInfo);
        break;

    case UNPROVISIONED_MODE:

        dwError = LSA_ERROR_NOT_SUPPORTED;
        break;
    }

cleanup:

    if (hDirectory) {
        LsaLdapCloseDirectory(hDirectory);
    }

    return dwError;

error:

    *ppNSSArtefactInfo = NULL;

    goto cleanup;
}

DWORD
AD_OnlineEnumNSSArtefacts(
    HANDLE  hProvider,
    HANDLE  hResume,
    DWORD   dwMaxNSSArtefacts,
    PDWORD  pdwNSSArtefactsFound,
    PVOID** pppNSSArtefactInfoList
    )
{
    DWORD dwError = 0;
    PAD_ENUM_STATE pEnumState = (PAD_ENUM_STATE)hResume;
    HANDLE hDirectory = (HANDLE)NULL;

    dwError = LsaDmWrapLdapOpenDirectoryDomain(gpADProviderData->szDomain,
                                               &hDirectory);
    BAIL_ON_LSA_ERROR(dwError);

    switch (gpADProviderData->dwDirectoryMode)
    {
    case DEFAULT_MODE:
        dwError = DefaultModeEnumNSSArtefacts(
                hDirectory,
                gpADProviderData->cell.szCellDN,
                gpADProviderData->szShortDomain,
                pEnumState,
                dwMaxNSSArtefacts,
                pdwNSSArtefactsFound,
                pppNSSArtefactInfoList
                );
        break;

    case CELL_MODE:
        dwError = CellModeEnumNSSArtefacts(
                hDirectory,
                gpADProviderData->cell.szCellDN,
                gpADProviderData->szShortDomain,
                pEnumState,
                dwMaxNSSArtefacts,
                pdwNSSArtefactsFound,
                pppNSSArtefactInfoList
                );
        break;

    case UNPROVISIONED_MODE:

        dwError = LSA_ERROR_NOT_SUPPORTED;
        break;
    }

cleanup:

    if (hDirectory) {
        LsaLdapCloseDirectory(hDirectory);
    }

    return dwError;

error:

    *pdwNSSArtefactsFound = 0;
    *pppNSSArtefactInfoList = NULL;

    goto cleanup;
}

DWORD
AD_CanonicalizeDomainsInCrackedNameInfo(
    IN OUT PLSA_LOGIN_NAME_INFO pNameInfo
    )
{
    DWORD dwError = 0;
    PSTR pszDomainName = NULL;

    BAIL_ON_INVALID_STRING(pNameInfo->pszDomainNetBiosName);

    pszDomainName = pNameInfo->pszDomainNetBiosName;
    pNameInfo->pszDomainNetBiosName = NULL;
    LSA_SAFE_FREE_STRING(pNameInfo->pszFullDomainName);

    dwError = LsaDmWrapGetDomainName(pszDomainName,
                                     &pNameInfo->pszFullDomainName,
                                     &pNameInfo->pszDomainNetBiosName);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    LSA_SAFE_FREE_STRING(pszDomainName);
    return dwError;

error:
    goto cleanup;
}

DWORD
AD_UpdateUserObjectFlags(
    IN OUT PAD_SECURITY_OBJECT pUser
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    struct timeval current_tv;
    UINT64 u64current_NTtime = 0;
    int64_t qwNanosecsToPasswordExpiry;

    if (gettimeofday(&current_tv, NULL) < 0)
    {
        dwError = errno;
        BAIL_ON_LSA_ERROR(dwError);
    }
    ADConvertTimeUnix2Nt(current_tv.tv_sec,
                         &u64current_NTtime);

    if (pUser->userInfo.qwAccountExpires != 0LL &&
            pUser->userInfo.qwAccountExpires != 9223372036854775807LL &&
            u64current_NTtime >= pUser->userInfo.qwAccountExpires)
    {
        pUser->userInfo.bAccountExpired = TRUE;
    }

    qwNanosecsToPasswordExpiry = gpADProviderData->adMaxPwdAge -
        (u64current_NTtime - pUser->userInfo.qwPwdLastSet);

    if (!pUser->userInfo.bPasswordNeverExpires &&
        gpADProviderData->adMaxPwdAge != 0 &&
        qwNanosecsToPasswordExpiry < 0)
    {
        //password is expired already
        pUser->userInfo.bPasswordExpired = TRUE;
    }

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
AD_VerifyUserAccountCanLogin(
    IN PAD_SECURITY_OBJECT pUserInfo
    )
{
    DWORD dwError = 0;

    dwError = AD_UpdateUserObjectFlags(
                pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    if (pUserInfo->userInfo.bAccountDisabled) {
        dwError = LSA_ERROR_ACCOUNT_DISABLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserInfo->userInfo.bAccountLocked) {
        dwError = LSA_ERROR_ACCOUNT_LOCKED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserInfo->userInfo.bAccountExpired) {
        dwError = LSA_ERROR_ACCOUNT_EXPIRED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserInfo->userInfo.bPasswordExpired) {
        dwError = LSA_ERROR_PASSWORD_EXPIRED;
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}
