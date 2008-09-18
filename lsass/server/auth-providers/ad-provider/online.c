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
AD_TestNetworkConnection(
    PCSTR pszDomain
    )
{
    DWORD dwError = 0;

    if (!pszDomain)
    {
        LSA_LOG_DEBUG("AD_TestNetworkConnection called with no domain name, system is not joined yet.");
        goto cleanup;
    }

#if defined (__LWI_DARWIN__)
    PSTR  pszQuestion = NULL;
    PVOID pBuffer = NULL;
    int   responseSize = 0;
    int   retryCounter = 0;
    DWORD dwBufferSize = 1024*64;

    /* This routine performs a simple DNS query looking for the SRV records for the current
       domain of the local system. This excercises the res_query library function which will
       ensure at startup time that the network stack has fully initialized. Without this test
       some platforms such as the Mac would fail to load the AD provider reliably due to problems
       issuing DNS queries or connecting via LDAP port to the DC. The subsequent calls into
       res_query when TRY_AGAIN is returned causes the next calls to block and only return when
       able to complete the query. */

    dwError = LsaAllocateStringPrintf(
                     &pszQuestion,
                     "_ldap._tcp.%s",
                     pszDomain);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAllocateMemory(
                  dwBufferSize,
                  &pBuffer);
    BAIL_ON_LSA_ERROR(dwError);

    while(TRUE)
    {
        dwError = res_init();
        BAIL_ON_LSA_ERROR(dwError);

        _res.options &= ~(RES_USEVC);

        responseSize = res_query(pszQuestion, ns_c_in, ns_t_srv,
                                 (PBYTE)pBuffer, dwBufferSize);
        if (responseSize < 0) 
        {           
            if (h_errno == TRY_AGAIN && retryCounter < 3)
            {
                LSA_LOG_DEBUG("AD_TestNetworkConnection query DNS for domain '%s' failed with TRY_AGAIN h_errno", pszDomain);
                retryCounter++;
                continue;
            }
            else
            {
                LSA_LOG_DEBUG("AD_TestNetworkConnection query DNS for domain '%s' failed with h_errno %d", pszDomain, h_errno);
                dwError = LSA_ERROR_INVALID_DNS_RESPONSE;
                BAIL_ON_LSA_ERROR(dwError);
            }
        }

        // If we get here, then our DNS query attempt succeeded (i.e. We sent data to/from the name server)
        break;
    }
#else
    BAIL_ON_LSA_ERROR(dwError);
#endif

    LSA_LOG_DEBUG("AD_TestNetworkConnection query DNS for domain '%s' was successful", pszDomain);

cleanup:

#if defined (__LWI_DARWIN__)
    LSA_SAFE_FREE_MEMORY(pBuffer);
    LSA_SAFE_FREE_STRING(pszQuestion);
#endif

    return dwError;

error:

    LSA_LOG_DEBUG("AD_TestNetworkConnection query DNS for domain '%s' failed, returning error %d", pszDomain, dwError);

    goto cleanup;
}

DWORD
AD_OnlineInitializeOperatingMode(
    PCSTR pszDomain,
    PCSTR pszHostName
    )
{
    DWORD dwError = 0;
    PSTR  pszComputerDN = NULL;
    PSTR  pszCellDN = NULL;
    PSTR  pszParentDN = NULL;
    PSTR  pszRootDN = NULL;
    PSTR  pszTmpDN = NULL;
    HANDLE hDirectory = (HANDLE)NULL;
    ADConfigurationMode adConfMode = NonSchemaMode;
    PLWNET_DC_INFO pDCInfo = NULL;
    HANDLE hDb = (HANDLE)NULL;
    PLSA_DM_ENUM_DOMAIN_INFO* ppDomainInfo = NULL;
    DWORD dwDomainInfoCount = 0;

    dwError = ADCacheDB_OpenDb(&hDb);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LWNetGetDCName(NULL, pszDomain, NULL, 0, &pDCInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaDmEngineDiscoverTrusts(pszDomain, pDCInfo->pszDnsForestName);
    BAIL_ON_LSA_ERROR(dwError); 

    dwError = LsaDmWrapLdapOpenDirectoryDomain(pszDomain, &hDirectory);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaLdapConvertDomainToDN(pszDomain, &pszRootDN);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = ADFindComputerDN(hDirectory, pszHostName, pszDomain,
                               &pszComputerDN);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaLdapGetParentDN(pszComputerDN, &pszParentDN);
    BAIL_ON_LSA_ERROR(dwError);
    
    //
    // Note: We keep looking at all parents of the current DN
    //       until we find a cell or hit the top domain DN.
    for(;;)
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
    
    if (IsNullOrEmptyString(pszCellDN)) {
        gpADProviderData->dwDirectoryMode = UNPROVISIONED_MODE;
    } 
    else
    {
        PSTR pszValue = pszCellDN + sizeof("CN=$LikewiseIdentityCell,") - 1;
        
        if (!strcasecmp(pszValue, pszRootDN)){
            gpADProviderData->dwDirectoryMode = DEFAULT_MODE;
            strcpy(gpADProviderData->cell.szCellDN, pszCellDN);
        } 
        else {
            gpADProviderData->dwDirectoryMode = CELL_MODE;
            strcpy(gpADProviderData->cell.szCellDN, pszCellDN);
         }
    }    
    
    dwError = ADGetDomainMaxPwdAge(hDirectory, pszDomain,
                                   &gpADProviderData->adMaxPwdAge);
    BAIL_ON_LSA_ERROR(dwError);   
    
    switch(gpADProviderData->dwDirectoryMode)
    {
        case DEFAULT_MODE:
        case CELL_MODE:
            dwError = ADGetConfigurationMode(hDirectory, pszCellDN,
                                             &adConfMode);
            BAIL_ON_LSA_ERROR(dwError);
            break;
    }    

    strcpy(gpADProviderData->szDomain, pszDomain);
    strcpy(gpADProviderData->szServerName, pDCInfo->pszDomainControllerName);
    strcpy(gpADProviderData->szComputerDN, pszComputerDN);
    strcpy(gpADProviderData->szShortDomain, pDCInfo->pszNetBIOSDomainName);
    
    gpADProviderData->adConfigurationMode = adConfMode;    
    
    if (gpADProviderData->dwDirectoryMode == CELL_MODE)
    {
        dwError = AD_GetLinkedCellInfo(hDirectory, pszCellDN);
        BAIL_ON_LSA_ERROR(dwError); 
    }

    dwError = ADCacheDB_CacheProviderData(
                hDb,
                gpADProviderData);
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

cleanup:

    LSA_SAFE_FREE_STRING(pszRootDN);
    LSA_SAFE_FREE_STRING(pszComputerDN);
    LSA_SAFE_FREE_STRING(pszCellDN);
    LSA_SAFE_FREE_STRING(pszParentDN);
    LSA_SAFE_FREE_STRING(pszTmpDN);
    LsaLdapCloseDirectory(hDirectory);
    LWNET_SAFE_FREE_DC_INFO(pDCInfo);
    ADCacheDB_SafeCloseDb(&hDb);
    LsaDmFreeEnumDomainInfoArray(ppDomainInfo);
    
    return dwError;
    
error:

    goto cleanup;
}

DWORD
AD_GetLinkedCellInfo(
    HANDLE hDirectory,
    PCSTR  pszCellDN)
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
                        gpADProviderData->szDomain,
                        &pszDirectoryRoot);
        BAIL_ON_LSA_ERROR(dwError);
        
        dwError = LsaDmWrapLdapOpenDirectoryGc(gpADProviderData->szDomain,
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
            
            dwError = LsaDLinkedListAppend(&gpADProviderData->pCellList, pLinkedCellInfo);
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

    if (gpADProviderData->pCellList) {
        LsaDLinkedListForEach(gpADProviderData->pCellList, &AD_FreeLinkedCellInfoInList, NULL);
        LsaDLinkedListFree(gpADProviderData->pCellList);
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
    OUT TrustMode* pTrustMode,
    OUT OPTIONAL PSTR* ppszDnsDomainName,
    OUT OPTIONAL PSTR* ppszNetbiosDomainName
    )
{
    DWORD dwError = 0;
    TrustMode trustmode = UnHandledTrust;    
    PSTR pszDnsDomainName = NULL;
    PSTR pszNetbiosDomainName = NULL;
    DWORD dwTrustFlags = 0;
    DWORD dwTrustType = 0;
    DWORD dwTrustAttributes = 0;
    
    //
    // Trusted domains support added
    //
    if (IsNullOrEmptyString(pszDomain) ||
        IsNullOrEmptyString(gpADProviderData->szDomain) ||
        IsNullOrEmptyString(gpADProviderData->szShortDomain))
    {
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    if (!strcasecmp(gpADProviderData->szDomain, pszDomain) ||
        !strcasecmp(gpADProviderData->szShortDomain, pszDomain))
    {
        trustmode = OneSelfTrust; 
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

    dwError = LsaDmQueryDomainInfo(pszDomain,
                                   ppszDnsDomainName ? &pszDnsDomainName : NULL,
                                   ppszNetbiosDomainName ? &pszNetbiosDomainName : NULL,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &dwTrustFlags,
                                   &dwTrustType,
                                   &dwTrustAttributes,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL);
    if (LSA_ERROR_NO_SUCH_DOMAIN == dwError)
    {
        LSA_LOG_WARNING("Warning!  AD_ServicesDomain was passed domain=%s, which it is neither the current domain nor in trusted domain list.", pszDomain);
    }
    BAIL_ON_LSA_ERROR(dwError);

    if (!(dwTrustFlags & NETR_TRUST_FLAG_IN_FOREST) &&
         (dwTrustFlags & NETR_TRUST_FLAG_OUTBOUND) &&
         (dwTrustFlags & NETR_TRUST_FLAG_INBOUND))
    {
        trustmode = TwoWayTrust_acrossforest; 
    }
    else if (dwTrustFlags & NETR_TRUST_FLAG_IN_FOREST)
    {
        trustmode = TwoWayTrust_inforest;
    }
    else if (dwTrustFlags & NETR_TRUST_FLAG_OUTBOUND)
    {
        trustmode = OneWayTrust; 
    }
    
cleanup:
    *pTrustMode = trustmode;
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
    trustmode = UnHandledTrust;
    LSA_SAFE_FREE_STRING(pszDnsDomainName);
    LSA_SAFE_FREE_STRING(pszNetbiosDomainName);
    goto cleanup;
}

DWORD
AD_CacheGroupMembershipFromPac(
    IN HANDLE           hProvider,
    IN TrustMode        trustMode,
    IN PAD_SECURITY_OBJECT pUserInfo,
    IN PAC_LOGON_INFO * pPac)
{
    int iPrimaryGroupIndex = -1;
    DWORD dwNumGroupsFound = 0;            
    PAD_SECURITY_OBJECT* ppGroupInfoResults = NULL;
    LSA_HASH_TABLE *pGroupSids = NULL;
    size_t sIndex = 0;
    size_t sCount = 0;
    PSTR pszSidCopy = NULL;
    DomSid *pBuiltSid = NULL;
    PWSTR pwszSid = NULL;
    PAD_GROUP_MEMBERSHIP *ppUserGroupMemberships = NULL;
    DWORD dwError = LSA_ERROR_SUCCESS;
    // do not free
    PVOID pCacheType = NULL;
    struct timeval current_tv;
    HANDLE hDb = (HANDLE)NULL;

    if (gettimeofday(&current_tv, NULL) < 0)
    {
        dwError = errno;
        BAIL_ON_LSA_ERROR(dwError);
    }

    switch(trustMode)
    {
        case OneWayTrust:
            
                break;
                
        default:
            
            dwError = ADLdap_GetUserGroupMembership(
                             hProvider,
                             pUserInfo->userInfo.uid,
                             &iPrimaryGroupIndex,
                             &dwNumGroupsFound,
                             &ppGroupInfoResults);
            BAIL_ON_LSA_ERROR(dwError);
    
            break;
    }

    dwError = LsaHashCreate(
                    LSA_MAX(pPac->info3.base.groups.count, dwNumGroupsFound) * 2,
                    LsaHashCaselessStringCompare,
                    LsaHashCaselessString,
                    AD_FreeHashStringKey,
                    &pGroupSids);
    BAIL_ON_LSA_ERROR(dwError);

    for (sIndex = 0; sIndex < dwNumGroupsFound; sIndex++)
    {
        if (ppGroupInfoResults[sIndex] != NULL)
        {
            dwError = LsaAllocateString(
                            ppGroupInfoResults[sIndex]->pszObjectSid,
                            &pszSidCopy);
            BAIL_ON_LSA_ERROR(dwError);

            if (sIndex == iPrimaryGroupIndex)
            {
                dwError = LsaHashSetValue(
                            pGroupSids,
                            pszSidCopy,
                            PRIMARY_GROUP_EXPIRATION);
            }
            else
            {
                dwError = LsaHashSetValue(
                            pGroupSids,
                            pszSidCopy,
                            STANDARD_GROUP_EXPIRATION);
            }
            BAIL_ON_LSA_ERROR(dwError);
            pszSidCopy = NULL;
        }
    }

    dwError = SidAllocateResizedCopy(
                    &pBuiltSid,
                    pPac->info3.base.domain_sid->subauth_count + 1,
                    pPac->info3.base.domain_sid);
    BAIL_ON_LSA_ERROR(dwError);

    for (sIndex = 0; sIndex < pPac->info3.base.groups.count; sIndex++)
    {
        pBuiltSid->subauth[pBuiltSid->subauth_count - 1] =
            pPac->info3.base.groups.rids[sIndex].rid;

        LSA_SAFE_FREE_MEMORY(pwszSid);
        dwError = SidToString(
                        pBuiltSid,
                        &pwszSid);
        BAIL_ON_LSA_ERROR(dwError);

        LSA_SAFE_FREE_STRING(pszSidCopy);
        dwError = LsaWc16sToMbs(
                        pwszSid,
                        &pszSidCopy);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaHashGetValue(
                        pGroupSids,
                        pszSidCopy,
                        &pCacheType);
        if (dwError == LSA_ERROR_SUCCESS)
        {
            LSA_SAFE_FREE_STRING(pszSidCopy);
            
            // This group can be obtained from a source other than the
            // pac, so it should remain expirable
            continue;
        }
        if (dwError == ENOENT)
        {
            dwError = LSA_ERROR_SUCCESS;
        }
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaHashSetValue(
                        pGroupSids,
                        pszSidCopy,
                        PAC_GROUP_EXPIRATION);
        BAIL_ON_LSA_ERROR(dwError);
        pszSidCopy = NULL;
    }

    if (pPac->res_group_dom_sid != NULL)
    {
        SidFree(pBuiltSid);
        pBuiltSid = NULL;

        dwError = SidAllocateResizedCopy(
                    &pBuiltSid,
                    pPac->res_group_dom_sid->subauth_count + 1,
                    pPac->res_group_dom_sid);
        BAIL_ON_LSA_ERROR(dwError);

        for (sIndex = 0; sIndex < pPac->res_groups.count; sIndex++)
        {
            pBuiltSid->subauth[pBuiltSid->subauth_count - 1] =
                pPac->res_groups.rids[sIndex].rid;

            LSA_SAFE_FREE_MEMORY(pwszSid);
            dwError = SidToString(
                        pBuiltSid,
                        &pwszSid);
            BAIL_ON_LSA_ERROR(dwError);

            LSA_SAFE_FREE_STRING(pszSidCopy);
            dwError = LsaWc16sToMbs(pwszSid,
                                    &pszSidCopy);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaHashGetValue(
                            pGroupSids,
                            pszSidCopy,
                            &pCacheType);
            if (dwError == LSA_ERROR_SUCCESS)
            {
                LSA_SAFE_FREE_STRING(pszSidCopy);
                
                // This group can be obtained from a source other than the
                // pac, so it should remain expirable
                continue;
            }
            if (dwError == ENOENT)
            {
                dwError = LSA_ERROR_SUCCESS;
            }
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaHashSetValue(
                        pGroupSids,
                        pszSidCopy,
                        PAC_GROUP_EXPIRATION);
            BAIL_ON_LSA_ERROR(dwError);
            pszSidCopy = NULL;
        }
    }

    for (sIndex = 0; sIndex < pPac->info3.sidcount; sIndex++)
    {
        // universal groups seem to have this set to 7
        // we don't want to treat sids from the sid history like groups.
        if (pPac->info3.sids[sIndex].attribute != 7)
        {
            continue;
        }
        LSA_SAFE_FREE_MEMORY(pwszSid);
        dwError = SidToString(
                pPac->info3.sids[sIndex].sid,
                &pwszSid);
        BAIL_ON_LSA_ERROR(dwError);

        LSA_SAFE_FREE_STRING(pszSidCopy);
        dwError = LsaWc16sToMbs(pwszSid,
                                &pszSidCopy);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaHashGetValue(
                pGroupSids,
                pszSidCopy,
                &pCacheType);
        if (dwError == LSA_ERROR_SUCCESS)
        {
            LSA_SAFE_FREE_STRING(pszSidCopy);
            
            // This group can be obtained from a source other than the
            // pac, so it should remain expirable
            continue;
        }
        if (dwError == ENOENT)
        {
            dwError = LSA_ERROR_SUCCESS;
        }
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaHashSetValue(
                pGroupSids,
                pszSidCopy,
                PAC_GROUP_EXPIRATION);
        BAIL_ON_LSA_ERROR(dwError);
        pszSidCopy = NULL;
    }

    sCount = pGroupSids->sCount + 1;
    dwError = LsaAllocateMemory(
                    sizeof(*ppUserGroupMemberships) * sCount,
                    (PVOID*)&ppUserGroupMemberships);
    BAIL_ON_LSA_ERROR(dwError);

    LSA_HASH_ITERATOR iterator;
    LSA_HASH_ENTRY *pEntry = NULL;

    dwError = LsaHashGetIterator(
            pGroupSids,
            &iterator);
    BAIL_ON_LSA_ERROR(dwError);

    sIndex = 0;
    while ((pEntry = LsaHashNext(&iterator)) != NULL)
    {
        dwError = LsaAllocateMemory(
                        sizeof(*ppUserGroupMemberships[sIndex]),
                        (PVOID*)&ppUserGroupMemberships[sIndex]);
        BAIL_ON_LSA_ERROR(dwError);

        ppUserGroupMemberships[sIndex]->cache.qwCacheId = -1;
        switch((size_t)pEntry->pValue)
        {
            case (size_t)PRIMARY_GROUP_EXPIRATION:
                ppUserGroupMemberships[sIndex]->cache.tLastUpdated =
                    (time_t)-2;
                break;
            case (size_t)STANDARD_GROUP_EXPIRATION:
                ppUserGroupMemberships[sIndex]->cache.tLastUpdated =
                    current_tv.tv_sec;
                break;
            case (size_t)PAC_GROUP_EXPIRATION:
                ppUserGroupMemberships[sIndex]->cache.tLastUpdated =
                    (time_t)-1;
                break;
        }

        dwError = LsaAllocateString(
                        pUserInfo->pszObjectSid,
                        &ppUserGroupMemberships[sIndex]->pszChildSid);
        BAIL_ON_LSA_ERROR(dwError);
                    
        dwError = LsaAllocateString(
                        (PSTR)pEntry->pKey,
                        &ppUserGroupMemberships[sIndex]->pszParentSid);
        BAIL_ON_LSA_ERROR(dwError);

        sIndex++;
    }

    dwError = LsaAllocateMemory(
            sizeof(*ppUserGroupMemberships[sIndex]),
            (PVOID*)&ppUserGroupMemberships[sIndex]);
    BAIL_ON_LSA_ERROR(dwError);

    ppUserGroupMemberships[sIndex]->cache.qwCacheId = -1;
    ppUserGroupMemberships[sIndex]->cache.tLastUpdated = current_tv.tv_sec;

    dwError = LsaAllocateString(
                    pUserInfo->pszObjectSid,
                    &ppUserGroupMemberships[sIndex]->pszChildSid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_OpenDb(&hDb);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_CacheGroupsForUser(
                        hDb,
                        pUserInfo->pszObjectSid,
                        sCount,
                        ppUserGroupMemberships,
                        TRUE);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    ADCacheDB_SafeFreeObjectList(dwNumGroupsFound, &ppGroupInfoResults);
    LsaHashSafeFree(&pGroupSids);
    LSA_SAFE_FREE_STRING(pszSidCopy);
    if (pBuiltSid != NULL)
    {
        SidFree(pBuiltSid);
    }
    LSA_SAFE_FREE_MEMORY(pwszSid);
    ADCacheDB_SafeFreeGroupMembershipList(sCount, &ppUserGroupMemberships);
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
    TrustMode trustMode = UnHandledTrust;
    
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
                        &trustMode,
                        &pLoginInfo->pszFullDomainName,
                        NULL);
    BAIL_ON_LSA_ERROR(dwError);
 
    dwError = AD_FindUserObjectByName(
                    hProvider,
                    pszLoginId,
                    &pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

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
        /* User does not have a UPN specified in AD, authentication will be tried with generated UPN */
        LSA_LOG_DEBUG("AD_AuthenticateUser called to logon user with no UPN set in AD, using generated UPN");
    }
 
    dwError = LsaSetupUserLoginSession(
                    pUserInfo->userInfo.uid,
                    pUserInfo->userInfo.gid,
                    pUserInfo->userInfo.pszUPN,
                    pszPassword,
                    KRB5_File_Cache,
                    pszServicePrincipal,
                    pszServicePassword,
                    &pPac);
    BAIL_ON_LSA_ERROR(dwError);

    if (pPac != NULL)
    {
        dwError = AD_CacheGroupMembershipFromPac(
                        hProvider,
                        trustMode,
                        pUserInfo,
                        pPac);
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    dwError = AD_CheckUserIsAllowedLogin(
                        hProvider,
                        pUserInfo->userInfo.uid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_OnlineCachePasswordVerifier(
                    pUserInfo,
                    pszPassword);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    if (pPac)
    {
        FreePacLogonInfo(pPac);
    }

    if (pLoginInfo) {
        LsaFreeNameInfo(pLoginInfo);
    }
    
    ADCacheDB_SafeFreeObject(&pUserInfo);
    LSA_SAFE_FREE_STRING(pszHostname);
    LSA_SAFE_FREE_STRING(pszUsername);
    LSA_SAFE_FREE_STRING(pszServicePassword);
    LSA_SAFE_FREE_STRING(pszDomainDnsName);
    LSA_SAFE_FREE_STRING(pszServicePrincipal);

    return dwError;

error:

    goto cleanup;
}

DWORD
AD_CheckUserIsAllowedLogin(
    HANDLE hProvider,
    uid_t  uid
    )
{
    DWORD  dwError = 0;
    DWORD  dwGroupInfoLevel = 0;
    DWORD  dwNumGroupsFound = 0;
    PVOID* ppGroupInfoList = NULL;
    DWORD  iGroup = 0;
    
    if (!AD_ShouldFilterUserLoginsByGroup())
    {
        goto cleanup;
    }
    
    dwError = AD_GetUserGroupMembership(
                    hProvider,
                    uid,
                    dwGroupInfoLevel,
                    &dwNumGroupsFound,
                    &ppGroupInfoList);
    BAIL_ON_LSA_ERROR(dwError);
    
    for (; iGroup < dwNumGroupsFound; iGroup++)
    {
        PLSA_GROUP_INFO_0 pGroupInfo = ppGroupInfoList[iGroup];
        
        if (AD_IsGroupAllowed(pGroupInfo->pszName))
        {
            goto cleanup;
        }
    }
    
    dwError = EACCES;
    BAIL_ON_LSA_ERROR(dwError);
    
cleanup:

    if (ppGroupInfoList)
    {
        LsaFreeGroupInfoList(dwGroupInfoLevel, ppGroupInfoList, dwNumGroupsFound);
    }

    return dwError;
    
error:

    if (dwError == EACCES)
    {
        LSA_LOG_ERROR("Error: User [%ld] not in restricted login groups", (long)uid);
    }
    else
    {
        LSA_LOG_ERROR("Error: Failed to validate group membership. [Error code: %u]", dwError);
    }

    goto cleanup;
}

DWORD
AD_CheckExpiredObject(
    PAD_SECURITY_OBJECT *ppCachedUser)
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    struct timeval current_tv;
    time_t expirationDate;

    if (gettimeofday(&current_tv, NULL) < 0)
    {
        dwError = errno;
        BAIL_ON_LSA_ERROR(dwError);
    }
    expirationDate = (*ppCachedUser)->cache.tLastUpdated +
        AD_GetCacheEntryExpirySeconds();

    if (expirationDate <= current_tv.tv_sec)
    {
        LSA_LOG_VERBOSE(
                "Cache entry for sid %s expired %ld seconds ago",
                (*ppCachedUser)->pszObjectSid,
                current_tv.tv_sec - expirationDate);

        //Pretend like the object couldn't be found in the cache
        ADCacheDB_SafeFreeObject(ppCachedUser);
        dwError = LSA_ERROR_NOT_HANDLED;
    }
    else
    {
        LSA_LOG_VERBOSE(
                "Using cache entry for sid %s, updated %ld seconds ago",
                (*ppCachedUser)->pszObjectSid,
                current_tv.tv_sec - (*ppCachedUser)->cache.tLastUpdated);
    }

error:
    return dwError;
}

void
AD_FilterNullEntries(
    PAD_SECURITY_OBJECT* ppEntries,
    size_t  *psCount)
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
    
    if (dwError == LSA_ERROR_NOT_HANDLED) {
        //convert uid -> NT4 name
        dwError = ADLdap_FindUserNameById(
                         uid,
                         &pszNT4Name);
        BAIL_ON_LSA_ERROR(dwError);
        
        dwError = AD_FindUserObjectByName(
                    hProvider,
                    pszNT4Name,
                    &pCachedUser);
        BAIL_ON_LSA_ERROR(dwError);    
    }
    else {
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
        
    if (dwError != LSA_ERROR_NO_SUCH_USER) {
       LSA_LOG_DEBUG(
               "Failed to find user by id %lu [error code:%d]",
               (unsigned long)uid,
               dwError);
       dwError = LSA_ERROR_NO_SUCH_USER;
    }

    goto cleanup;
}

DWORD
AD_OnlineGetUserGroupMembership(
    HANDLE  hProvider,
    uid_t   uid,
    DWORD   dwGroupInfoLevel,
    PDWORD  pdwNumGroupsFound,
    PVOID** pppGroupInfoList
    )
{    
    DWORD dwError = LSA_ERROR_SUCCESS;
    HANDLE hDb = (HANDLE)NULL;    
    PAD_SECURITY_OBJECT pUserInfo = NULL;
    DWORD dwNumGroupsFound = 0;            
    size_t sCount = 0;
    size_t sDnCount = 0;
    size_t sIndex;
    PAD_GROUP_MEMBERSHIP *ppUserGroupMemberships = NULL;
    BOOLEAN bFoundNull = FALSE;
    BOOLEAN bExpired = FALSE;
    struct timeval current_tv;
    PAD_SECURITY_OBJECT* ppGroupInfoResults = NULL;
    PVOID* ppGroupInfoList = NULL;
    //Only free top level array, do not free string pointers.
    PSTR *ppszParentSids = NULL;
    DWORD iGroup = 0;
    size_t sMembers = 0;
    PAD_SECURITY_OBJECT *ppMembers = NULL;
    PCSTR pszUserSid = NULL;
    int iPrimaryGroupIndex = -1;
    PCAD_SECURITY_OBJECT pPrimaryGroup = NULL;
    PLSA_LOGIN_NAME_INFO pGroupNameInfo = NULL;
    PSTR pszGroupNT4Name = NULL;

    dwError = AD_FindUserObjectById(
                    hProvider,
                    uid,
                    &pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);
    
    pszUserSid = pUserInfo->pszObjectSid;
    
    if (gettimeofday(&current_tv, NULL) < 0)
    {
        dwError = errno;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    dwError = ADCacheDB_OpenDb(&hDb);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_GetGroupsForUser(
                  hDb,
                  pszUserSid,
                  &sCount,
                  &ppUserGroupMemberships);
    BAIL_ON_LSA_ERROR(dwError);

    /* Whenever user's group membership is cached, an extra "null" entry is added.
     * This entry has pszChildSid set to the user group, and pszParentSid
     * set to NULL. When this entry is present, it indicates that the list
     * is complete for the child sid.
     *
     * If the null entry is missing, ignore the cache data (an incomplete
     * user's group membership list was obtained in the process of caching something
     * else).
     */
    for (sIndex = 0; !bExpired && sIndex < sCount; sIndex++)
    {
        if (ppUserGroupMemberships[sIndex]->cache.tLastUpdated >= 0 &&
            ppUserGroupMemberships[sIndex]->cache.tLastUpdated +
                AD_GetCacheEntryExpirySeconds() <= current_tv.tv_sec)
        {
            bExpired = TRUE;
        }
        if (ppUserGroupMemberships[sIndex]->pszParentSid == NULL)
        {
            bFoundNull = TRUE;
        }
    }

    if(bExpired)
    {
        LSA_LOG_VERBOSE(
            "Cache entry for user's group membership for sid %s is expired",
            pszUserSid);
    }
    else if(!bFoundNull)
    {
        LSA_LOG_VERBOSE(
            "Cache entry for user's group membership for sid %s is incomplete",
            pszUserSid);
    }
    
    if (bExpired || !bFoundNull)
    {
        TrustMode trustMode = UnHandledTrust;
        
        dwError = AD_DetermineTrustModeandDomainName(
                        pUserInfo->pszNetbiosDomainName,
                        &trustMode,
                        NULL,
                        NULL);
        BAIL_ON_LSA_ERROR(dwError);
        
        ADCacheDB_SafeFreeGroupMembershipList(sCount, &ppUserGroupMemberships);

        switch (trustMode)
        {
            case OneWayTrust:
                
                break;

            default:
                        
                dwError = ADLdap_GetUserGroupMembership(
                                 hProvider,
                                 uid,
                                 &iPrimaryGroupIndex,
                                 &dwNumGroupsFound,
                                 &ppGroupInfoResults);
                BAIL_ON_LSA_ERROR(dwError);
        
                break;
        }
        
        if (iPrimaryGroupIndex != -1)
        {
            pPrimaryGroup = ppGroupInfoResults[iPrimaryGroupIndex];
        }

        /* Generate a list of AD_GROUP_MEMBERSHIP objects. Include a null
         * object to indicate that the member list is authoritative for the
         * parent sid.
         */

        ADCacheDB_SafeFreeGroupMembershipList(sCount, &ppUserGroupMemberships);
        
        //first filter all the NULL entries in ppResults
        sDnCount = (size_t)dwNumGroupsFound;
        
        AD_FilterNullEntries(
            ppGroupInfoResults,
            &sDnCount);
        
        sCount = sDnCount + 1;
        dwError = LsaAllocateMemory(
                        sizeof(*ppUserGroupMemberships) * sCount,
                        (PVOID*)&ppUserGroupMemberships);
        BAIL_ON_LSA_ERROR(dwError);

        for(sIndex = 0; sIndex < sDnCount; sIndex++)
        {
            dwError = LsaAllocateMemory(
                            sizeof(*ppUserGroupMemberships[sIndex]),
                            (PVOID*)&ppUserGroupMemberships[sIndex]);
            BAIL_ON_LSA_ERROR(dwError);

            ppUserGroupMemberships[sIndex]->cache.qwCacheId = -1;
            if (ppGroupInfoResults[sIndex] == pPrimaryGroup)
            {
                /* Let's say there is user A and group B in AD. There are
                 * two ways to say that A is a member of B. Either A's DN
                 * is added to the members field in B, *or* A's primary group
                 * field is set to B's DN. Typically the latter case occurs
                 * for the Domain Users group. Let's say that A is defined as
                 * a member of B via the primary group field.
                 *
                 * When someone asks who is a member of B, LSASS will not
                 * show A because that kind of membership list is too long.
                 *
                 * However, if someone asks which groups A is a member of,
                 * LSASS should report B.
                 *
                 * This means that LSASS sees 'A is a member of B' and
                 * 'B does not contain A', which is contradictory.
                 *
                 * With LSASS's caching, when B's membership list needs to be
                 * cached, all previous cache entries for B will be removed.
                 * This removes the entry saying that A is a member of B that
                 * was previously cached by looking up the groups A is a
                 * member of.
                 * 
                 * Setting last updated to -2 stops this entry from being
                 * deleted by the group -> groupmembership operation.
                 */
                ppUserGroupMemberships[sIndex]->cache.tLastUpdated =
                    (time_t)-2;
            }
            else
            {
                ppUserGroupMemberships[sIndex]->cache.tLastUpdated =
                    current_tv.tv_sec;
            }

            dwError = LsaAllocateString(
                            pszUserSid,
                            &ppUserGroupMemberships[sIndex]->pszChildSid);
            BAIL_ON_LSA_ERROR(dwError);
                        
            dwError = LsaAllocateString(
                            ppGroupInfoResults[sIndex]->pszObjectSid,
                            &ppUserGroupMemberships[sIndex]->pszParentSid);
            BAIL_ON_LSA_ERROR(dwError);
        }

        dwError = LsaAllocateMemory(
                        sizeof(*ppUserGroupMemberships[sIndex]),
                        (PVOID*)&ppUserGroupMemberships[sIndex]);
        BAIL_ON_LSA_ERROR(dwError);

        ppUserGroupMemberships[sIndex]->cache.qwCacheId = -1;
        ppUserGroupMemberships[sIndex]->cache.tLastUpdated = current_tv.tv_sec;

        dwError = LsaAllocateString(
                        pszUserSid,
                        &ppUserGroupMemberships[sIndex]->pszChildSid);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = ADCacheDB_CacheGroupsForUser(
                            hDb,
                            pszUserSid,
                            sCount,
                            ppUserGroupMemberships,
                            FALSE);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        /* If the cache is valid, get the rest of the object info with
         * AD_FindObjectsBySidList.
         */
        LSA_LOG_VERBOSE(
              "Using cached user's group membership for sid %s",
              pszUserSid);

        dwError = LsaAllocateMemory(
                        sizeof(*ppszParentSids) * (sCount - 1),
                        (PVOID*)&ppszParentSids);
        BAIL_ON_LSA_ERROR(dwError);

        sDnCount = 0;
        for (sIndex = 0; sIndex < sCount; sIndex++)
        {
            if (ppUserGroupMemberships[sIndex]->pszParentSid != NULL)
            {
                ppszParentSids[sDnCount++] = ppUserGroupMemberships[sIndex]->pszParentSid;
            }
        }

        dwError = AD_FindObjectsBySidList(
                        hProvider,
                        NULL,
                        NULL,
                        sDnCount,
                        ppszParentSids,
                        &ppGroupInfoResults);
        BAIL_ON_LSA_ERROR(dwError);
        
        AD_FilterNullEntries(
            ppGroupInfoResults,
            &sDnCount);
    }
    dwError = LsaAllocateMemory(
                sDnCount * sizeof(PVOID),
                (PVOID*)&ppGroupInfoList);
    BAIL_ON_LSA_ERROR(dwError);
    
    for (iGroup = 0; iGroup < sDnCount; iGroup++){
       TrustMode trustMode = UnHandledTrust;
       
       dwError = LsaAllocateStringPrintf(
                        &pszGroupNT4Name,
                        "%s\\%s",
                        ppGroupInfoResults[iGroup]->pszNetbiosDomainName,ppGroupInfoResults[iGroup]->pszSamAccountName);    
       BAIL_ON_LSA_ERROR(dwError);
       
       dwError = LsaCrackDomainQualifiedName(
                           pszGroupNT4Name,
                           NULL,
                           &pGroupNameInfo);
       BAIL_ON_LSA_ERROR(dwError);    
       
       LSA_SAFE_FREE_STRING(pGroupNameInfo->pszFullDomainName);

       dwError = AD_DetermineTrustModeandDomainName(
                           pGroupNameInfo->pszDomainNetBiosName,                        
                           &trustMode,
                           &pGroupNameInfo->pszFullDomainName,
                           NULL);
       BAIL_ON_LSA_ERROR(dwError);

       if (trustMode != OneWayTrust)
       {
           dwError = AD_GetExpandedGroupUsers(
                           hProvider,
                           pGroupNameInfo->pszFullDomainName,
                           ppGroupInfoResults[iGroup]->pszNetbiosDomainName,           
                           ppGroupInfoResults[iGroup]->pszObjectSid,
                           5,
                           NULL,
                           &sMembers,
                           &ppMembers);
           BAIL_ON_LSA_ERROR(dwError);
       }
       
       dwError = ADMarshalFromGroupCache(
                       ppGroupInfoResults[iGroup],
                       sMembers,
                       ppMembers,
                       dwGroupInfoLevel,
                       &ppGroupInfoList[iGroup]);
       BAIL_ON_LSA_ERROR(dwError);       
       
       ADCacheDB_SafeFreeObjectList(sMembers, &ppMembers);

       LSA_SAFE_FREE_STRING(pszGroupNT4Name);
       if (pGroupNameInfo) {
           LsaFreeNameInfo(pGroupNameInfo);
           pGroupNameInfo = NULL;
       }
       
    }   

    *pppGroupInfoList = ppGroupInfoList;
    *pdwNumGroupsFound = sDnCount;
    
cleanup:

    ADCacheDB_SafeCloseDb(&hDb);    
    ADCacheDB_SafeFreeGroupMembershipList(sCount, &ppUserGroupMemberships);    
    ADCacheDB_SafeFreeObjectList(sDnCount, &ppGroupInfoResults);
    ADCacheDB_SafeFreeObjectList(sMembers, &ppMembers);
    
    ADCacheDB_SafeFreeObject(&pUserInfo);

    if (pGroupNameInfo)
    {
        LsaFreeNameInfo(pGroupNameInfo);
    }
    
    LSA_SAFE_FREE_MEMORY(ppszParentSids);    
    LSA_SAFE_FREE_STRING(pszGroupNT4Name);
    
    return dwError;
    
error:

    *pppGroupInfoList = NULL;
    *pdwNumGroupsFound = 0;
    
    LSA_LOG_ERROR("Failed to find user's group memberships of UID=%d. [error code:%d]",
                  uid, dwError);

    if (ppGroupInfoList) {
        LsaFreeGroupInfoList(
              dwGroupInfoLevel,
              ppGroupInfoList,
              sDnCount);
    }

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
    HANDLE hDirectory = (HANDLE)NULL;

    dwError = LsaDmWrapLdapOpenDirectoryDomain(gpADProviderData->szDomain,
                                               &hDirectory);
    BAIL_ON_LSA_ERROR(dwError);
    
    switch (gpADProviderData->dwDirectoryMode) 
    {
    case DEFAULT_MODE:
        dwError = DefaultModeEnumUsers(
                hDirectory,
                gpADProviderData->cell.szCellDN,
                gpADProviderData->szShortDomain,
                pEnumState,
                dwMaxNumUsers,
                pdwUsersFound,
                pppUserInfoList
                );
        BAIL_ON_LSA_ERROR(dwError);
        break;

    case CELL_MODE:
        dwError = CellModeEnumUsers(
                hDirectory,
                gpADProviderData->cell.szCellDN,
                gpADProviderData->szShortDomain,
                pEnumState,
                dwMaxNumUsers,
                pdwUsersFound,
                pppUserInfoList
                );
        BAIL_ON_LSA_ERROR(dwError);
        break;

    case UNPROVISIONED_MODE:
        dwError = UnprovisionedModeEnumUsers(
                hDirectory,
                gpADProviderData->szShortDomain,
                pEnumState,
                dwMaxNumUsers,
                pdwUsersFound,
                pppUserInfoList
                );
                BAIL_ON_LSA_ERROR(dwError);
        break;
    }
    
cleanup:

    if (hDirectory) {
        LsaLdapCloseDirectory(hDirectory);
    }

    return dwError;
    
error:

    *pdwUsersFound = 0;
    *pppUserInfoList = NULL;

    goto cleanup;
}

DWORD
AD_OnlineFindGroupByName(
    HANDLE  hProvider,
    PCSTR   pszGroupName,
    DWORD   dwGroupInfoLevel,
    PVOID*  ppGroupInfo
    )
{
    DWORD dwError = 0;
    //handle the current domain directory, which is where all the psedu objects shall be found except for default mode
    HANDLE hPseudoDirectory = (HANDLE)NULL; 
    //handle the domain directory where the real object resides (for all modes)
    HANDLE hRealDirectory = (HANDLE)NULL; 
    
    PLSA_LOGIN_NAME_INFO pGroupNameInfo = NULL;
    PSTR  pszGroupName_copy = NULL;
    HANDLE hDb = (HANDLE)NULL;
    PAD_SECURITY_OBJECT pCachedGroup = NULL;
    PSTR pszObjectDN = NULL;
    PSTR pszObjectDomainName = NULL;
    PSTR pszObjectSamaccountName = NULL;
    TrustMode trustMode = UnHandledTrust;
    size_t sMembers = 0;
    PAD_SECURITY_OBJECT *ppMembers = NULL;
    PSTR pszLookupName = NULL; //this name should be limited to NT4 or UPN format only 

    BAIL_ON_INVALID_STRING(pszGroupName);
    
    dwError = LsaAllocateString(
                    pszGroupName,
                    &pszGroupName_copy);
    BAIL_ON_LSA_ERROR(dwError);
    
    LsaStrCharReplace(pszGroupName_copy, AD_GetSeparator(),' ');
    
    dwError = LsaCrackDomainQualifiedName(
                        pszGroupName_copy,
                        gpADProviderData->szDomain,
                        &pGroupNameInfo);    
    BAIL_ON_LSA_ERROR(dwError);
    
    if ((pGroupNameInfo->nameType == NameType_Alias) &&
    	!strcasecmp(pGroupNameInfo->pszName, "root")) {
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
        LSA_SAFE_FREE_STRING(pGroupNameInfo->pszFullDomainName);
        
        dwError = AD_DetermineTrustModeandDomainName(
                            pGroupNameInfo->pszDomainNetBiosName,                        
                            &trustMode,
                            &pGroupNameInfo->pszFullDomainName,
                            NULL);
        BAIL_ON_LSA_ERROR(dwError);
        
        goto FoundInCacheValid;
    }
    
    if (dwError != LSA_ERROR_NOT_HANDLED){
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    // Otherwise, the group couldn't be found in the cache (MFS support added)    
    // If name is alias, convert to NT4 name
    if (pGroupNameInfo->nameType == NameType_Alias){        
        
        dwError = ADLdap_FindGroupNameByAlias(
                         pszGroupName_copy,
                         &pszLookupName);
        BAIL_ON_LSA_ERROR(dwError);
        
        if (pGroupNameInfo) {
            LsaFreeNameInfo(pGroupNameInfo);
            pGroupNameInfo = NULL;
        }
        
        //pszUpnName is formatted as NT4, crack again to fill in pUserNameInfo
        dwError = LsaCrackDomainQualifiedName(
                            pszLookupName,
                            NULL,
                            &pGroupNameInfo);
        BAIL_ON_LSA_ERROR(dwError);        
    }
    else
    {    
        dwError = LsaAllocateString(
                     pszGroupName_copy,
                     &pszLookupName);
        BAIL_ON_LSA_ERROR(dwError);
    }    
    
    LSA_SAFE_FREE_STRING(pGroupNameInfo->pszFullDomainName);
    
    //Get the trustMode and complete filling in correct information for pUserNameInfo
    dwError = AD_DetermineTrustModeandDomainName(
                        pGroupNameInfo->pszDomainNetBiosName,                        
                        &trustMode,
                        &pGroupNameInfo->pszFullDomainName,
                        NULL);
    BAIL_ON_LSA_ERROR(dwError);
    
    LSA_SAFE_FREE_STRING(pGroupNameInfo->pszDomainNetBiosName);
    
    dwError = ADGetDomainNetBios(
                pGroupNameInfo->pszFullDomainName,
                &pGroupNameInfo->pszDomainNetBiosName);
    BAIL_ON_LSA_ERROR(dwError);
    
    LSA_SAFE_FREE_STRING(pGroupNameInfo->pszObjectSid);
    
    dwError = LsaDmWrapNetLookupObjectSidByName(
                gpADProviderData->szDomain,
                pszLookupName,
                &pGroupNameInfo->pszObjectSid);
    BAIL_ON_LSA_ERROR(dwError);
    
    switch (trustMode){
        case OneWayTrust:
            
               switch (gpADProviderData->dwDirectoryMode) 
               {                        
                   case DEFAULT_MODE:      
                   case UNPROVISIONED_MODE:
                          dwError = LSA_ERROR_NO_SUCH_GROUP;
                           
                           break;                        
                           
                   case CELL_MODE:
                           dwError = CellModeFindGroupByName(
                                           gpADProviderData->szDomain,                                           
                                           gpADProviderData->cell.szCellDN,
                                           NULL,
                                           pGroupNameInfo,
                                           &pCachedGroup);
                           
                           break;
                           
                   default:
                           dwError = LSA_ERROR_NOT_HANDLED;
                                               
               }
               BAIL_ON_LSA_ERROR(dwError);
            
            break;
            
        case TwoWayTrust_inforest:                
            dwError =ADLdap_GetGCObjectInfoBySid(
                    gpADProviderData->szDomain,            
                    pGroupNameInfo->pszObjectSid,
                    &pszObjectDN,
                    &pszObjectDomainName,
                    &pszObjectSamaccountName);
            BAIL_ON_LSA_ERROR(dwError);
            
            break;
            
        case TwoWayTrust_acrossforest:
            dwError =ADLdap_GetGCObjectInfoBySid(
                    pGroupNameInfo->pszFullDomainName,            
                    pGroupNameInfo->pszObjectSid,
                    &pszObjectDN,
                    &pszObjectDomainName,
                    &pszObjectSamaccountName);
            BAIL_ON_LSA_ERROR(dwError);
                            
            break;
             
        case OneSelfTrust:  //Single dc scenario
            dwError = LsaAllocateString(gpADProviderData->szDomain,
                                        &pszObjectDomainName);
            BAIL_ON_LSA_ERROR(dwError);           
            
             break;
                 
        default:
            dwError = LSA_ERROR_NOT_HANDLED;
            BAIL_ON_LSA_ERROR(dwError);
    }        
    
    if (trustMode == TwoWayTrust_inforest || trustMode == TwoWayTrust_acrossforest || trustMode == OneSelfTrust){
        
         switch (gpADProviderData->dwDirectoryMode) 
         {
             case DEFAULT_MODE:

                     dwError = DefaultModeFindGroupByName(
                                     pszObjectDomainName,
                                     pGroupNameInfo,
                                     &pCachedGroup);       
                
                     break;
    
             case CELL_MODE:
                     dwError = CellModeFindGroupByName(
                                     gpADProviderData->szDomain,
                                     gpADProviderData->cell.szCellDN,
                                     pszObjectDomainName,
                                     pGroupNameInfo,
                                     &pCachedGroup);                     
                     
                     break;
    
             case UNPROVISIONED_MODE:
                     dwError = UnprovisionedModeFindGroupByName(
                                     pszObjectDomainName,             
                                     pGroupNameInfo,
                                     &pCachedGroup);     

                     
                     break;
    
             default:
                     dwError = LSA_ERROR_NOT_HANDLED;                     
         }
         BAIL_ON_LSA_ERROR(dwError); 
    }    

    dwError = ADCacheDB_CacheObjectEntry(hDb, pCachedGroup);
    BAIL_ON_LSA_ERROR(dwError);

FoundInCacheValid:

    if (trustMode != OneWayTrust)
    {
        dwError = AD_GetExpandedGroupUsers(
            hProvider,
            pGroupNameInfo->pszFullDomainName,
            pGroupNameInfo->pszDomainNetBiosName,        
            pCachedGroup->pszObjectSid,
            5,
            NULL,
            &sMembers,
            &ppMembers);
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    dwError = ADMarshalFromGroupCache(
            pCachedGroup,
            sMembers,
            ppMembers,
            dwGroupInfoLevel,
            ppGroupInfo
            );
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    if (hPseudoDirectory != (HANDLE)NULL) {
        LsaLdapCloseDirectory(hPseudoDirectory);
    }
    
    if (hRealDirectory != (HANDLE)NULL) {
        LsaLdapCloseDirectory(hRealDirectory);
    }
    
    ADCacheDB_SafeCloseDb(&hDb);
    ADCacheDB_SafeFreeObject(&pCachedGroup);
    ADCacheDB_SafeFreeObjectList(sMembers, &ppMembers);

    if (pGroupNameInfo) {
        LsaFreeNameInfo(pGroupNameInfo);
    }
    
    LSA_SAFE_FREE_STRING(pszGroupName_copy);
    LSA_SAFE_FREE_STRING(pszLookupName);
    
    LSA_SAFE_FREE_STRING(pszObjectDN);    
    LSA_SAFE_FREE_STRING(pszObjectDomainName);
    LSA_SAFE_FREE_STRING(pszObjectSamaccountName);

    return dwError;

error:

    *ppGroupInfo = NULL;
    
    if (dwError != LSA_ERROR_NO_SUCH_GROUP) {
       LSA_LOG_DEBUG("Failed to find group [error code:%d]", dwError);
       dwError = LSA_ERROR_NO_SUCH_GROUP;
    }

    goto cleanup;
}

DWORD
AD_OnlineFindGroupById(
    HANDLE  hProvider,
    gid_t   gid,
    DWORD   dwGroupInfoLevel,
    PVOID*  ppGroupInfo
    )
{
    DWORD dwError =  0;
    HANDLE hDb = (HANDLE)NULL;
    PAD_SECURITY_OBJECT pCachedGroup = NULL;
    PSTR   pszNT4Name = NULL;  
    size_t sMembers = 0;
    PAD_SECURITY_OBJECT *ppMembers = NULL;
    PLSA_LOGIN_NAME_INFO pGroupNameInfo = NULL;
    PSTR pszFullDomainName = NULL;
    
    if (gid == 0) {
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
    
    if (dwError == LSA_ERROR_NOT_HANDLED){
        //convert gid -> NT4 name
        dwError = ADLdap_FindGroupNameById(
                         gid,
                         &pszNT4Name);
        BAIL_ON_LSA_ERROR(dwError);
        
        dwError = AD_FindGroupByName(
                    hProvider,
                    pszNT4Name,   
                    dwGroupInfoLevel,
                    ppGroupInfo);
        BAIL_ON_LSA_ERROR(dwError);    

    }
    else if (dwError == 0){//found in cache and not expired
            TrustMode trustMode = UnHandledTrust;
            
            if (IsNullOrEmptyString(pCachedGroup->pszNetbiosDomainName)){
                dwError = ADLdap_FindGroupNameById(
                                            gid,
                                            &pszNT4Name);
                BAIL_ON_LSA_ERROR(dwError);
                           
                dwError = LsaCrackDomainQualifiedName(
                                       pszNT4Name,
                                       NULL,
                                       &pGroupNameInfo);
                BAIL_ON_LSA_ERROR(dwError);   
                
                dwError = LsaAllocateString(pGroupNameInfo->pszDomainNetBiosName,
                                            &pCachedGroup->pszNetbiosDomainName);
                BAIL_ON_LSA_ERROR(dwError);  
            }
            
            dwError = AD_DetermineTrustModeandDomainName(
                                pCachedGroup->pszNetbiosDomainName,
                                &trustMode,
                                &pszFullDomainName,
                                NULL);
            BAIL_ON_LSA_ERROR(dwError);
                     
            if (trustMode != OneWayTrust)
            {
                dwError = AD_GetExpandedGroupUsers(
                    hProvider,              
                    pszFullDomainName,
                    pCachedGroup->pszNetbiosDomainName,       
                    pCachedGroup->pszObjectSid,
                    5,
                    NULL,
                    &sMembers,
                    &ppMembers);
                BAIL_ON_LSA_ERROR(dwError);
            }
            
            dwError = ADMarshalFromGroupCache(
                    pCachedGroup,
                    sMembers,
                    ppMembers,
                    dwGroupInfoLevel,
                    ppGroupInfo);
            BAIL_ON_LSA_ERROR(dwError);
        }
        else{
            BAIL_ON_LSA_ERROR(dwError);
            }
    
cleanup:
    LSA_SAFE_FREE_STRING(pszNT4Name); 
    LSA_SAFE_FREE_STRING(pszFullDomainName);
    
    ADCacheDB_SafeCloseDb(&hDb);
    ADCacheDB_SafeFreeObject(&pCachedGroup); 
    ADCacheDB_SafeFreeObjectList(sMembers, &ppMembers);

    if (pGroupNameInfo)
    {
       LsaFreeNameInfo(pGroupNameInfo);
    }

    return dwError;
    
error:

    *ppGroupInfo = NULL;
        
    if (dwError != LSA_ERROR_NO_SUCH_GROUP) {
       LSA_LOG_DEBUG("Failed to find group [error code:%d]", dwError);
       dwError = LSA_ERROR_NO_SUCH_GROUP;
    }

    goto cleanup;
}

DWORD
AD_OnlineEnumGroups(
    HANDLE  hProvider,
    HANDLE  hResume,
    DWORD   dwMaxGroups,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
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
        dwError = DefaultModeEnumGroups(
                hDirectory,
                gpADProviderData->cell.szCellDN,
                gpADProviderData->szShortDomain,
                pEnumState,
                dwMaxGroups,
                pdwGroupsFound,
                pppGroupInfoList
                );
        break;

    case CELL_MODE:
        dwError = CellModeEnumGroups(
                hDirectory,
                gpADProviderData->cell.szCellDN,
                gpADProviderData->szShortDomain,
                pEnumState,
                dwMaxGroups,
                pdwGroupsFound,
                pppGroupInfoList
                );      
        break;

    case UNPROVISIONED_MODE:
        dwError = UnprovisionedModeEnumGroups(
                hDirectory,
                gpADProviderData->szShortDomain,
                pEnumState,
                dwMaxGroups,
                pdwGroupsFound,
                pppGroupInfoList
                );
        break;
    }
    
cleanup:

    if (hDirectory) {
        LsaLdapCloseDirectory(hDirectory);
    }

    return dwError;
    
error:

    *pdwGroupsFound = 0;
    *pppGroupInfoList = NULL;

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
    
cleanup:
    if (pszDomainController)
    {
        LWNetFreeString(pszDomainController);
    }

    if (pLoginInfo) {
        LsaFreeNameInfo(pLoginInfo);
    }
    
    if (pUserInfo) {
        LsaFreeUserInfo(dwUserInfoLevel, (PVOID)pUserInfo);
    }
    
    ADCacheDB_SafeFreeObject(&pCachedUser);

    LSA_SAFE_FREE_STRING(pszFullDomainName);

    return dwError;
    
error:

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
       LSA_LOG_ERROR("The user'yys [Uid:%ld] home directory is not defined", (long)pUserInfo->uid);
       dwError = LSA_ERROR_FAILED_CREATE_HOMEDIR;
       BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaCheckDirectoryExists(
                    pUserInfo->pszHomedir,
                    &bExists);
    BAIL_ON_LSA_ERROR(dwError);
    
    if (!bExists) {
#if defined (__LWI_DARWIN__)
        dwError = AD_CreateHomeDirectory_Mac(pUserInfo);
#else
        dwError = AD_CreateHomeDirectory_Generic(pUserInfo);
#endif
        BAIL_ON_LSA_ERROR(dwError);
    }
    
cleanup:

    return dwError;
    
error:

    goto cleanup;
}

#if defined (__LWI_DARWIN__)
DWORD
AD_CreateHomeDirectory_Mac(
    PLSA_USER_INFO_1 pUserInfo
    )
{
    DWORD dwError = 0;
    PSTR pszCommand = NULL;
    PLSA_LOGIN_NAME_INFO pLoginInfo = NULL;
    
    dwError = LsaCrackDomainQualifiedName(
                    pUserInfo->pszName,
                    NULL,
                    &pLoginInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAllocateStringPrintf(&pszCommand,
                                      "/usr/sbin/createhomedir -c -u %s\\\\%s",
                                      pLoginInfo->pszDomainNetBiosName,
                                      pLoginInfo->pszName);
    BAIL_ON_LSA_ERROR(dwError);

    LSA_LOG_DEBUG("Creating Mac user home directory with command: %s", pszCommand);

    dwError = system(pszCommand);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LSA_SAFE_FREE_STRING(pszCommand);

    if (pLoginInfo) {
        LsaFreeNameInfo(pLoginInfo);
    }

    return dwError;

error:

    LSA_LOG_ERROR("Failed to create home directory for user (%s), actual error %d", pUserInfo->pszName, dwError);
    dwError = LSA_ERROR_FAILED_CREATE_HOMEDIR;

    goto cleanup;
}
#endif

DWORD
AD_CreateHomeDirectory_Generic(
    PLSA_USER_INFO_1 pUserInfo
    )
{
    DWORD dwError = 0;
    mode_t  umask = 022;
    mode_t  perms = (S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);
    BOOLEAN bRemoveDir = FALSE;

    dwError = LsaCreateDirectory(
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
    
    dwError = LsaCheckDirectoryExists(
                    "/etc/skel",
                    &bExists);
    BAIL_ON_LSA_ERROR(dwError);
    
    if (bExists) {
        dwError = LsaCopyDirectory(
                    "/etc/skel",
                    ownerUid,
                    ownerGid,
                    pszHomedirPath);
        BAIL_ON_LSA_ERROR(dwError);
    }
    
cleanup:

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
    PSTR    pszUPN_upper = NULL;
    PSTR    pszIndex = NULL;
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
    
    dwError = LsaAllocateString(
                    pUserInfo->pszUPN,
                    &pszUPN_upper);
    BAIL_ON_LSA_ERROR(dwError);
    
    pszIndex = strchr(pszUPN_upper, '@');
    if (pszIndex)
    {
       pszIndex++;
       while (pszIndex && *pszIndex)
       {
             *pszIndex = toupper(*pszIndex); 
             pszIndex++;
       }
    }
    
    dwError = LsaAllocateStringPrintf(
                    &pszData,
                    "%s\n%s\n",
                    pUserInfo->pszUPN,
                    pszUPN_upper);
    BAIL_ON_LSA_ERROR(dwError);
    
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
    LSA_SAFE_FREE_STRING(pszUPN_upper);
    LSA_SAFE_FREE_STRING(pszK5LoginPath_tmp);
    LSA_SAFE_FREE_STRING(pszK5LoginPath);
    
    return dwError;
    
error:

    goto cleanup;
}

DWORD
LsaValidateSeparatorCharacter(
    CHAR cValue,
    CHAR* pcValidatedSeparator
    )
{
    DWORD dwError = 0;
    CHAR cValidatedSeparator = gcSeparatorDefault;
    
    if (!ispunct((int)cValue))
    {
        LSA_LOG_ERROR("Error: separator-character must be punctuation; value provided is \"%c\"", 
                        cValue);
        dwError = LSA_ERROR_INVALID_CONFIG;
    }   
    else if (cValue == '@'  || 
            cValue == '#'  ||
            cValue == '/'  || 
            cValue == '\\')
    {
        LSA_LOG_ERROR("Error: separator-character may not be @, #, /, or \\; value provided is \"%c\"", 
                        cValue);
        dwError = LSA_ERROR_INVALID_CONFIG;
    }
    else 
    {
        cValidatedSeparator = cValue;
    }
    BAIL_ON_LSA_ERROR(dwError);
    
    *pcValidatedSeparator = cValidatedSeparator;

cleanup:

    return dwError;

error:

    *pcValidatedSeparator = gcSeparatorDefault;
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
AD_GetExpandedGroupUsers(
    HANDLE  hProvider,
    PCSTR pszDomainName,
    PCSTR pszDomainNetBiosName,
    PCSTR pszGroupSid,
    int iMaxDepth,
    BOOLEAN *pbAllExpanded,
    size_t* psCount,
    PAD_SECURITY_OBJECT** pppResults)
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    /* Keep two hash tables (security object->depth)
     *  one hash table tracks the groups which still need to be expanded
     *  the other hash table tracks the groups which have already been expanded,
     *  or are user objects.
     */
    LSA_HASH_TABLE *pExpanded = NULL, *pToExpand = NULL;
    PAD_SECURITY_OBJECT* ppToSort = NULL;
    size_t sCount = 0;
    size_t sIndex;
    size_t sCurrentDepth = 1;
    BOOLEAN bAllExpanded = TRUE;
    LSA_HASH_ITERATOR iterator;
    const LSA_HASH_ENTRY *pExpandingEntry = NULL;
    PCSTR pszExpandingSid = NULL;
    PAD_SECURITY_OBJECT* ppResults = NULL;
    
    if (IsNullOrEmptyString(pszDomainName)){
        pszDomainName = gpADProviderData->szDomain;
    }
    
    if (IsNullOrEmptyString(pszDomainNetBiosName)){
        pszDomainNetBiosName = gpADProviderData->szShortDomain;
    }    

    dwError = AD_GetGroupMembers(
        hProvider,
        pszDomainName,
        pszDomainNetBiosName,
        pszGroupSid,
        &sCount,
        &ppToSort);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaHashCreate(
        sCount * 2,
        AD_CompareObjectSids,
        AD_HashObjectSid,
        NULL,
        &pExpanded); 
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaHashCreate(
        sCount * 2,
        AD_CompareObjectSids,
        AD_HashObjectSid,
        NULL,
        &pToExpand); 
    BAIL_ON_LSA_ERROR(dwError);

    /* Get an initial list of objects by calling AD_GetGroupMembers. Put
     * objects in either the to_expand or expanded hash tables based on
     * whether they are users, or their depth is too great.
     */
    for (sIndex = 0; sIndex < sCount; sIndex++)
    {
        if (ppToSort[sIndex]->type == AccountType_Group && sCurrentDepth < iMaxDepth)
        {
            dwError = LsaHashSetValue(
                    pToExpand,
                    ppToSort[sIndex],
                    (PVOID)(sCurrentDepth + 1));
            BAIL_ON_LSA_ERROR(dwError);
        }
        else
        {
            dwError = LsaHashSetValue(
                    pExpanded,
                    ppToSort[sIndex],
                    (PVOID)(sCurrentDepth + 1));
            BAIL_ON_LSA_ERROR(dwError);
        }
        // The pointer is now in a hash table
        ppToSort[sIndex] = NULL;
    }

    dwError = LsaHashGetIterator(pToExpand, &iterator);
    BAIL_ON_LSA_ERROR(dwError);

    /* Iterate through the hash table, pToExpand, using AD_GetGroupMembers. */
    while(pToExpand->sCount > 0)
    {
        ADCacheDB_SafeFreeObjectList(sCount, &ppToSort);

        pExpandingEntry = LsaHashNext(&iterator);
        if (pExpandingEntry == NULL)
        {
            //Reached the end of the hash table; gotta start over
            dwError = LsaHashGetIterator(pToExpand, &iterator);
            BAIL_ON_LSA_ERROR(dwError);
            pExpandingEntry = LsaHashNext(&iterator);
            if (pExpandingEntry == NULL)
            {
                dwError = LSA_ERROR_INTERNAL;
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
        pszExpandingSid = ((PAD_SECURITY_OBJECT)pExpandingEntry->pKey)->
            pszObjectSid;
        sCurrentDepth = (size_t)pExpandingEntry->pValue;

        //Mark this object as expanded
        dwError = LsaHashSetValue(
                pExpanded,
                pExpandingEntry->pKey,
                pExpandingEntry->pValue);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaHashRemoveKey(
                pToExpand,
                pExpandingEntry->pKey);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = AD_GetGroupMembers(
            hProvider,
            pszDomainName,
            pszDomainNetBiosName,
            pszExpandingSid,
            &sCount,
            &ppToSort);
        BAIL_ON_LSA_ERROR(dwError);

        if ((sCount + pToExpand->sCount) * 2 > pToExpand->sTableSize)
        {
            //Resize the hash table to avoid collisions
            dwError = LsaHashResize(
                    pToExpand,
                    (sCount + pToExpand->sCount) *2);
            BAIL_ON_LSA_ERROR(dwError);
        }
        if ((sCount + pExpanded->sCount) * 2 > pExpanded->sTableSize)
        {
            //Resize the hash table to avoid collisions
            dwError = LsaHashResize(
                    pExpanded,
                    (sCount + pToExpand->sCount) *2);
            BAIL_ON_LSA_ERROR(dwError);
        }

        for (sIndex = 0; sIndex < sCount; sIndex++)
        {
            if (LsaHashGetValue(
                    pToExpand,
                    ppToSort[sIndex],
                    NULL) != ENOENT ||
                LsaHashGetValue(
                    pExpanded,
                    ppToSort[sIndex],
                    NULL) != ENOENT)
            {
                // One of the hashes already contains this object
                ADCacheDB_SafeFreeObject(&ppToSort[sIndex]);
            }
            else if (ppToSort[sIndex]->type == AccountType_Group &&
                    sCurrentDepth < iMaxDepth)
            {
                dwError = LsaHashSetValue(
                        pToExpand,
                        ppToSort[sIndex],
                        (PVOID)(sCurrentDepth + 1));
                BAIL_ON_LSA_ERROR(dwError);
            }
            else
            {
                if (ppToSort[sIndex]->type == AccountType_Group)
                {
                    bAllExpanded = FALSE;
                }
                dwError = LsaHashSetValue(
                        pExpanded,
                        ppToSort[sIndex],
                        (PVOID)(sCurrentDepth + 1));
                BAIL_ON_LSA_ERROR(dwError);
            }
            // The pointer is now in a hash table
            ppToSort[sIndex] = NULL;
        }
    }
    ADCacheDB_SafeFreeObjectList(sCount, &ppToSort);

    dwError = LsaHashGetIterator(pExpanded, &iterator);
    BAIL_ON_LSA_ERROR(dwError);

    //Remove the groups from the expanded list so that only users remain
    while (TRUE)
    {
        pExpandingEntry = LsaHashNext(&iterator);
        if (pExpandingEntry == NULL)
        {
            //Reached the end of the hash table
            break;
        }
        if (((PAD_SECURITY_OBJECT)pExpandingEntry->pKey)->type == AccountType_Group)
        {
            PAD_SECURITY_OBJECT pRemoveObject = pExpandingEntry->pKey;
            dwError = LsaHashRemoveKey(
                    pExpanded,
                    pExpandingEntry->pKey);
            BAIL_ON_LSA_ERROR(dwError);

            ADCacheDB_SafeFreeObject(&pRemoveObject);
        }
    }

    /* Fill in the final list of users and return it.
    */
    sCount = pExpanded->sCount;
    dwError = LsaAllocateMemory(
            sizeof(*ppResults) * sCount,
            (PVOID*)&ppResults);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaHashGetIterator(pExpanded, &iterator);
    BAIL_ON_LSA_ERROR(dwError);

    for (sIndex = 0; (pExpandingEntry = LsaHashNext(&iterator)) != NULL;
            sIndex++)
    {
        PAD_SECURITY_OBJECT pObject = pExpandingEntry->pKey;

        //Double check that only users remain
        if (pObject->type != AccountType_User)
        {
            dwError = LSA_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
        }

        dwError = LsaHashRemoveKey(
                pExpanded,
                pExpandingEntry->pKey);
        BAIL_ON_LSA_ERROR(dwError);

        ppResults[sIndex] = pObject;
    }

    if (sIndex != sCount)
    {
        dwError = LSA_ERROR_INTERNAL;
        BAIL_ON_LSA_ERROR(dwError);
    }

    *pppResults = ppResults;
    *psCount = sCount;
    if(pbAllExpanded != NULL)
    {
        *pbAllExpanded = bAllExpanded;
    }

cleanup:
    LsaHashSafeFree(&pExpanded);
    LsaHashSafeFree(&pToExpand);
    ADCacheDB_SafeFreeObjectList(sCount, &ppToSort);

    return dwError;

error:
    //Let the hash table handle freeing the objects that are left inside.
    if(pToExpand != NULL)
    {
        pToExpand->fnFree = AD_FreeHashObject;
    }
    if(pExpanded != NULL)
    {
        pExpanded->fnFree = AD_FreeHashObject;
    }
    ADCacheDB_SafeFreeObjectList(sCount, &ppResults);

    *pppResults = NULL;
    *psCount = 0;
    if(pbAllExpanded != NULL)
    {
        *pbAllExpanded = FALSE;
    }

    goto cleanup;
}

DWORD
AD_GetGroupMembers(
    HANDLE hProvider,
    PCSTR pszDomainName,
    PCSTR pszDomainNetBiosName,
    PCSTR pszSid,
    size_t* psCount,
    PAD_SECURITY_OBJECT** pppResults)
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    HANDLE hDb = (HANDLE)NULL;
    size_t sCount = 0;
    size_t sDnCount = 0;
    size_t sDnCountNullFiltered = 0;
    size_t sIndex;
    PAD_GROUP_MEMBERSHIP *ppMemberships = NULL;
    BOOLEAN bFoundNull = FALSE;
    BOOLEAN bExpired = FALSE;
    struct timeval current_tv;
    HANDLE hDirectory = (HANDLE)NULL;
    PAD_SECURITY_OBJECT* ppResults = NULL;
    PAD_SECURITY_OBJECT* ppResultsNullFiltered = NULL;
    PAD_SECURITY_OBJECT* ppUnexpirableResults = NULL;
    //Only free top level array, do not free string pointers.
    PSTR *ppszChildSids = NULL;
    DWORD iValue = 0;
    DWORD jValue = 0;
    size_t sUnexpirableEntries = 0;

    if (gettimeofday(&current_tv, NULL) < 0)
    {
        dwError = errno;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    if (IsNullOrEmptyString(pszDomainName)){
        pszDomainName = gpADProviderData->szDomain;
    }
    
    if (IsNullOrEmptyString(pszDomainNetBiosName)){
        pszDomainNetBiosName = gpADProviderData->szShortDomain;
    }

    dwError = ADCacheDB_OpenDb(&hDb);
    BAIL_ON_LSA_ERROR(dwError);

    /* Call ADCacheDB_GetGroupMembers to receive the cache's known membership.
     */
    dwError = ADCacheDB_GetGroupMembers(
                    hDb,
                    pszSid,
                    &sCount,
                    &ppMemberships);
    BAIL_ON_LSA_ERROR(dwError);

    /* Whenever group->members is cached, an extra "null" entry is added.
     * This entry has pszParentSid set to the parent group, and pszChildSid
     * set to NULL. When this entry is present, it indicates that the list
     * is complete for the parent sid.
     *
     * If the null entry is missing, ignore the cache data (an incomplete
     * group membership list was obtained in the process of caching something
     * else).
     */
    for (sIndex = 0; !bExpired && sIndex < sCount; sIndex++)
    {
        if (ppMemberships[sIndex]->cache.tLastUpdated >= 0 &&
            ppMemberships[sIndex]->cache.tLastUpdated +
                AD_GetCacheEntryExpirySeconds() <= current_tv.tv_sec)
        {
            bExpired = TRUE;
        }
        if (ppMemberships[sIndex]->pszChildSid == NULL)
        {
            bFoundNull = TRUE;
        }
    }

    if(bExpired)
    {
        LSA_LOG_VERBOSE(
            "Cache entry for group membership for sid %s is expired",
            pszSid);
    }
    else if(!bFoundNull)
    {
        LSA_LOG_VERBOSE(
            "Cache entry for group membership for sid %s is incomplete",
            pszSid);
    }

    if (bExpired || !bFoundNull)
    {
        // Get the complete object for the unexpirable entries (the ones
        // that we can't get through regular ldap queries). Later on, this
        // list will be appended to the list we get from ldap.
        for (sIndex = 0; sIndex < sCount; sIndex++)
        {
            if (ppMemberships[sIndex]->cache.tLastUpdated < 0)
            {
                sUnexpirableEntries++;
            }
        }

        dwError = LsaAllocateMemory(
                sizeof(*ppszChildSids) * (sUnexpirableEntries),
                (PVOID*)&ppszChildSids);
        BAIL_ON_LSA_ERROR(dwError);

        sUnexpirableEntries = 0;
        for (sIndex = 0; sIndex < sCount; sIndex++)
        {
            if (ppMemberships[sIndex]->cache.tLastUpdated < 0)
            {
                ppszChildSids[sUnexpirableEntries++] =
                    ppMemberships[sIndex]->pszChildSid;
            }
        }

        dwError = AD_FindObjectsBySidList(
            hProvider,
            pszDomainName,
            pszDomainNetBiosName,
            sUnexpirableEntries,
            ppszChildSids,
            &ppUnexpirableResults);
        BAIL_ON_LSA_ERROR(dwError);
        
        AD_FilterNullEntries(
            ppUnexpirableResults,
            &sUnexpirableEntries);
        
        LSA_SAFE_FREE_MEMORY(ppszChildSids);
        ADCacheDB_SafeFreeGroupMembershipList(sCount, &ppMemberships);

        dwError = LsaDmWrapLdapOpenDirectoryDomain(pszDomainName,
                                                   &hDirectory);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = ADLdap_GetGroupMembers(
            hProvider,
            hDirectory,
            pszDomainName,
            pszDomainNetBiosName,
            pszSid,
            &sDnCount,
            &ppResults);
        BAIL_ON_LSA_ERROR(dwError);

        /* Generate a list of AD_GROUP_MEMBERSHIP objects. Include a null
         * object to indicate that the member list is authoritative for the
         * parent sid.
         */

        //first filter all the NULL entries in ppResults
        dwError = LsaAllocateMemory(
                        sizeof(PAD_SECURITY_OBJECT) *
                            (sDnCount + sUnexpirableEntries),
                        (PVOID*)&ppResultsNullFiltered);
        BAIL_ON_LSA_ERROR(dwError);
        
        for (iValue = 0, sDnCountNullFiltered = 0; iValue < sDnCount; iValue++)
        {
            if (ppResults[iValue]){
                ppResultsNullFiltered[sDnCountNullFiltered] = ppResults[iValue];
                sDnCountNullFiltered++;
                ppResults[iValue] = NULL;
            }
        }

        sCount = sDnCountNullFiltered + 1;
        dwError = LsaAllocateMemory(
                sizeof(*ppMemberships) * sCount,
                (PVOID*)&ppMemberships);
        BAIL_ON_LSA_ERROR(dwError);

        for(sIndex = 0; sIndex < sDnCountNullFiltered; sIndex++)
        {
            dwError = LsaAllocateMemory(
                    sizeof(*ppMemberships[sIndex]),
                    (PVOID*)&ppMemberships[sIndex]);
            BAIL_ON_LSA_ERROR(dwError);

            ppMemberships[sIndex]->cache.qwCacheId = -1;
            ppMemberships[sIndex]->cache.tLastUpdated = current_tv.tv_sec;

            dwError = LsaAllocateString(
                            pszSid,
                            &ppMemberships[sIndex]->pszParentSid);
            BAIL_ON_LSA_ERROR(dwError);
                        
            dwError = LsaAllocateString(
                            ppResultsNullFiltered[sIndex]->pszObjectSid,
                            &ppMemberships[sIndex]->pszChildSid);
            BAIL_ON_LSA_ERROR(dwError);
        }

        dwError = LsaAllocateMemory(
                sizeof(*ppMemberships[sIndex]),
                (PVOID*)&ppMemberships[sIndex]);
        BAIL_ON_LSA_ERROR(dwError);

        ppMemberships[sIndex]->cache.qwCacheId = -1;
        ppMemberships[sIndex]->cache.tLastUpdated = current_tv.tv_sec;

        dwError = LsaAllocateString(
                        pszSid,
                        &ppMemberships[sIndex]->pszParentSid);
        BAIL_ON_LSA_ERROR(dwError);

        // Now that the membership cache list has been created,
        // add the unexpirable entries to the results list
        memcpy(
                &ppResultsNullFiltered[sDnCountNullFiltered],
                ppUnexpirableResults,
                sUnexpirableEntries * sizeof(ppResultsNullFiltered[0])
                );
        memset(
                ppUnexpirableResults,
                0,
                sUnexpirableEntries * sizeof(ppResultsNullFiltered[0])
                );
        sDnCountNullFiltered += sUnexpirableEntries;

        dwError = ADCacheDB_CacheGroupMembership(
                            hDb,
                            pszSid,
                            sCount,
                            ppMemberships);
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

        dwError = LsaAllocateMemory(
                sizeof(*ppszChildSids) * (sCount - 1),
                (PVOID*)&ppszChildSids);
        BAIL_ON_LSA_ERROR(dwError);

        sDnCount = 0;
        for(sIndex = 0; sIndex < sCount; sIndex++)
        {
            if (ppMemberships[sIndex]->pszChildSid != NULL)
            {
                ppszChildSids[sDnCount++] = ppMemberships[sIndex]->pszChildSid;
            }
        }

        dwError = AD_FindObjectsBySidList(
            hProvider,
            pszDomainName,
            pszDomainNetBiosName,
            sDnCount,
            ppszChildSids,
            &ppResults);
        BAIL_ON_LSA_ERROR(dwError);
        
        dwError = LsaAllocateMemory(
                        sizeof(PAD_SECURITY_OBJECT) * sDnCount,
                        (PVOID*)&ppResultsNullFiltered);
        BAIL_ON_LSA_ERROR(dwError);
        
        for (iValue = 0, jValue = 0; iValue < sDnCount; iValue++)
        {
            if (ppResults[iValue]){
                ppResultsNullFiltered[jValue] = ppResults[iValue];
                jValue++;
                ppResults[iValue] = NULL;
            }
        }
        
        sDnCountNullFiltered = jValue;
    }

    *psCount = sDnCountNullFiltered;
    *pppResults = ppResultsNullFiltered;

cleanup:
    ADCacheDB_SafeCloseDb(&hDb);
    
    LSA_SAFE_FREE_MEMORY(ppszChildSids);
    
    ADCacheDB_SafeFreeGroupMembershipList(sCount, &ppMemberships);
    
    ADCacheDB_SafeFreeObjectList(sDnCount, &ppResults);
    ADCacheDB_SafeFreeObjectList(sUnexpirableEntries, &ppUnexpirableResults);
    
    if (hDirectory != (HANDLE)NULL) {
            LsaLdapCloseDirectory(hDirectory);
            hDirectory = (HANDLE)NULL;
        }    
    
    return dwError;

error:
    *psCount = 0;
    *pppResults = NULL;
    
    ADCacheDB_SafeFreeObjectList(sDnCountNullFiltered, &ppResultsNullFiltered);
    
    goto cleanup;
}

DWORD
AD_FindObjectsByDNList(
    HANDLE hProvider,
    size_t sCount,
    PSTR* ppszDnList,
    PAD_SECURITY_OBJECT **pppResults)
{
    /* TODO: Performance Optimization 
     *
     * For the remaining users:
     * Generate large ldap queries to get multiple real user/group objects at the same time.
     * Generate large ldap queries to get multiple pseudo user/group objects from the sids of the real objects.
     *
     * Update cache for all objects.
     */
    DWORD dwError = LSA_ERROR_SUCCESS;    
    PAD_SECURITY_OBJECT* ppResults = NULL;    
    size_t sIndex = 0;
    PCSTR pszCellDN = NULL;
    ADConfigurationMode adConfMode = NonSchemaMode;
    HANDLE hDb = (HANDLE)NULL;
    HANDLE hDirectory = (HANDLE)NULL;
    struct timeval current_tv;
    
    if (sCount > 0 && !(ppszDnList && *ppszDnList)){
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);        
    }
    
    if (sCount == 0 && !(ppszDnList && *ppszDnList)){
        ppResults = NULL;
        goto done;        
    }
    
    if (gettimeofday(&current_tv, NULL) < 0)
    {
        dwError = errno;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    dwError = ADCacheDB_OpenDb(&hDb);
    BAIL_ON_LSA_ERROR(dwError);
    
    //Lookup as many users as possible from the cache.
    dwError = ADCacheDB_FindObjectsByDNList(
                  hDb,
                  sCount,
                  ppszDnList,
                  &ppResults);
    BAIL_ON_LSA_ERROR(dwError);

    for(sIndex = 0; sIndex < sCount; sIndex++)
    {
        //Lookup AD for the user or groups that are not in the cache or expired in the cache
        if ((!ppResults[sIndex])
            || (ppResults[sIndex] && (ppResults[sIndex]->cache.tLastUpdated + AD_GetCacheEntryExpirySeconds()
                                       <= current_tv.tv_sec)))
        {
            if (ppResults[sIndex] && (
                    ppResults[sIndex]->cache.tLastUpdated >= 0 &&
                    ppResults[sIndex]->cache.tLastUpdated +
                    AD_GetCacheEntryExpirySeconds() <= current_tv.tv_sec))
            {
                 LSA_LOG_VERBOSE("Cache entry for sid %s is expired",
                                 ppResults[sIndex]->pszObjectSid);
                 ADCacheDB_SafeFreeObject(&ppResults[sIndex]);
            }            
            if (hDirectory == (HANDLE)NULL)
            {
                dwError = LsaDmWrapLdapOpenDirectoryDomain(gpADProviderData->szDomain,
                                                           &hDirectory);
                BAIL_ON_LSA_ERROR(dwError);
                
                if (gpADProviderData->dwDirectoryMode != UNPROVISIONED_MODE)
                {
                    pszCellDN = gpADProviderData->cell.szCellDN;
                    dwError = ADGetConfigurationMode(
                                             hDirectory,
                                             pszCellDN,
                                             &adConfMode);
                    BAIL_ON_LSA_ERROR(dwError);
                }
                else
                {
                    pszCellDN = NULL;
                    adConfMode = UnknownMode;
                } 
            }
            
            dwError = ADFindUserOrGroupByDN(
                    hDirectory,
                    pszCellDN,
                    gpADProviderData->dwDirectoryMode,
                    adConfMode,
                    gpADProviderData->szShortDomain,
                    ppszDnList[sIndex],
                    &ppResults[sIndex]);     
            //if we cannot find user with the current DN, don't bail on error, instead, leave ppResults[sIndex] to be NULL
            if (dwError == LSA_ERROR_NO_SUCH_USER_OR_GROUP)
                dwError = 0;            
            BAIL_ON_LSA_ERROR(dwError);

            dwError = ADCacheDB_CacheObjectEntry(hDb, ppResults[sIndex]);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

done:
    *pppResults = ppResults;    
    
cleanup:

    ADCacheDB_SafeCloseDb(&hDb);
    
    if (hDirectory != (HANDLE)NULL)
    {
        LsaLdapCloseDirectory(hDirectory);
    }

    return dwError;
    
error: 

    if (ppResults){
        ADCacheDB_SafeFreeObjectList(
                sCount,
                &ppResults);
    }
    
    goto cleanup;
}

DWORD
AD_FindObjectBySid(
    HANDLE hProvider,
    PCSTR pszDomainName,
    PCSTR pszDomainNetBiosName,
    PCSTR pszSid,
    PAD_SECURITY_OBJECT *ppResult)
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    PAD_SECURITY_OBJECT *ppResultArray = NULL;
    
    if (IsNullOrEmptyString(pszDomainName)) {
        pszDomainName = gpADProviderData->szDomain;
    }
    
    if (IsNullOrEmptyString(pszDomainNetBiosName)) {
        pszDomainNetBiosName = gpADProviderData->szShortDomain;
    }

    dwError = AD_FindObjectsBySidList(
                    hProvider,
                    pszDomainName,
                    pszDomainNetBiosName,
                    1,
                    (PSTR *)&pszSid,
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
AD_FindObjectsBySidList(
    HANDLE hProvider,
    PCSTR  pszDomainName,
    PCSTR  pszDomainNetBiosName,    
    size_t sCount,
    PSTR* ppszSidList,
    PAD_SECURITY_OBJECT **pppResults)
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    PAD_SECURITY_OBJECT *ppResults = NULL;
    HANDLE hDb = 0;
    size_t sIndex;
    PSTR pszDirectoryRoot = NULL;
    PSTR szAttributeListName[] = 
            {
                AD_LDAP_SAM_NAME_TAG,
                AD_LDAP_GROUP_TYPE,
                NULL
            };    
    LDAPMessage *pMessageReal = NULL;
    struct timeval current_tv;
    HANDLE hDirectory = (HANDLE)NULL;
    PSTR pszQuery = NULL;
    DWORD dwCount;
    LDAP *pLd = NULL;
    PSTR pszSamAccountName = NULL;
    PSTR pszNt4Name = NULL;
    PSTR pszGroupType = NULL;
    PLSA_LOGIN_NAME_INFO pUserNameInfo = NULL;

    if (gettimeofday(&current_tv, NULL) < 0)
    {
        dwError = errno;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    if (IsNullOrEmptyString(pszDomainName))
    {
        pszDomainName = gpADProviderData->szDomain;
    }
    
    if (IsNullOrEmptyString(pszDomainNetBiosName))
    {
        pszDomainNetBiosName = gpADProviderData->szShortDomain;
    }       

    /* 
     * Lookup as many users as possible from the cache.
     */

    dwError = ADCacheDB_OpenDb(&hDb);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_FindObjectsBySidList(
                    hDb,
                    sCount,
                    ppszSidList,
                    &ppResults);
    BAIL_ON_LSA_ERROR(dwError);

    for (sIndex = 0; sIndex < sCount; sIndex++)
    {          
        if ((ppResults[sIndex] != NULL) &&
            (ppResults[sIndex]->cache.tLastUpdated >= 0) &&
            (ppResults[sIndex]->cache.tLastUpdated +
            AD_GetCacheEntryExpirySeconds() <= current_tv.tv_sec))
        {
            LSA_LOG_VERBOSE("Cache entry for sid %s is expired",
                            ppResults[sIndex]->pszObjectSid);
            ADCacheDB_SafeFreeObject(&ppResults[sIndex]);
        }
        
        if (ppResults[sIndex] != NULL)
        {
            continue;
        }

        if (hDirectory == (HANDLE)NULL)
        {
            dwError = LsaLdapConvertDomainToDN(
                            pszDomainName,
                            &pszDirectoryRoot);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaDmWrapLdapOpenDirectoryDomain(pszDomainName,
                                                       &hDirectory);
            BAIL_ON_LSA_ERROR(dwError);
            
            pLd = LsaLdapGetSession(hDirectory);
        }

        LSA_SAFE_FREE_STRING(pszQuery);
        
        dwError = LsaAllocateStringPrintf(
                        &pszQuery,
                        "(objectSid=%s)",
                        ppszSidList[sIndex]);
        BAIL_ON_LSA_ERROR(dwError);

        if (pMessageReal) {
            ldap_msgfree(pMessageReal);
            pMessageReal = NULL;
        }
        
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
        if (dwCount < 0 || dwCount > 1)
        {
            dwError = LSA_ERROR_LDAP_ERROR;
            BAIL_ON_LSA_ERROR(dwError);
        }
        
        //If the object doesn't exist, then ignore it and leave the result
        //field as NULL.
        if (dwCount == 0)
        {
            continue;
        }
        
        LSA_SAFE_FREE_STRING(pszSamAccountName);

        dwError = LsaLdapGetString(
                    hDirectory,
                    pMessageReal,
                    AD_LDAP_SAM_NAME_TAG,
                    &pszSamAccountName);
        BAIL_ON_LSA_ERROR(dwError);
        
        BAIL_ON_INVALID_STRING(pszSamAccountName);

        LSA_SAFE_FREE_STRING(pszNt4Name);
        
        dwError = LsaAllocateStringPrintf(
                        &pszNt4Name,
                        "%s\\%s",
                        pszDomainNetBiosName,
                        pszSamAccountName);
        BAIL_ON_LSA_ERROR(dwError);

        LSA_SAFE_FREE_STRING(pszGroupType);
        
        dwError = LsaLdapGetString(
                    hDirectory,
                    pMessageReal,
                    "groupType",
                    &pszGroupType);
        BAIL_ON_LSA_ERROR(dwError);
        
        if (IsNullOrEmptyString(pszGroupType))
        {  
            dwError = AD_FindUserObjectByName(
                          hProvider,
                          pszNt4Name,
                          &ppResults[sIndex]);

            if (dwError == LSA_ERROR_NO_SUCH_USER)
            {
                //The user isn't enabled. Leave ppResults[sIndex] as NULL.
                dwError = LSA_ERROR_SUCCESS;
            }
        }
        else
        {
            dwError = AD_FindCachedGroupByNT4Name(
                         hProvider,
                         pszNt4Name,
                         &ppResults[sIndex]);
        }
        BAIL_ON_LSA_ERROR(dwError);

        dwError = ADCacheDB_CacheObjectEntry(hDb, ppResults[sIndex]);
        BAIL_ON_LSA_ERROR(dwError);
    }

    *pppResults = ppResults;

cleanup:

    if (hDirectory != (HANDLE)NULL) {
        LsaLdapCloseDirectory(hDirectory);
    }
    
    ADCacheDB_SafeCloseDb(&hDb);
    LSA_SAFE_FREE_STRING(pszDirectoryRoot);
    
    if (pMessageReal) {
        ldap_msgfree(pMessageReal);
    }
    
    LSA_SAFE_FREE_STRING(pszQuery);
    LSA_SAFE_FREE_STRING(pszSamAccountName);
    LSA_SAFE_FREE_STRING(pszNt4Name);
    LSA_SAFE_FREE_STRING(pszGroupType);
    
    if (pUserNameInfo) {
        LsaFreeNameInfo(pUserNameInfo);
    }
    
    return dwError;

error:

    ADCacheDB_SafeFreeObjectList(sCount, &ppResults);
    *pppResults = NULL;
    
    goto cleanup;
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
                    NULL,
                    NULL,
                    sCount,
                    ppszSidList,
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

VOID
AD_FreeLinkedCellInfoInList(
    PVOID pLinkedCellInfo,
    PVOID pUserData
    )
{
    if (pLinkedCellInfo) {
        AD_FreeLinkedCellInfo((PAD_LINKED_CELL_INFO)pLinkedCellInfo);
    }
}

VOID
AD_FreeLinkedCellInfo(
    PAD_LINKED_CELL_INFO pLinkedCellInfo
    )
{
    LSA_SAFE_FREE_STRING(pLinkedCellInfo->pszCellDN);
    LSA_SAFE_FREE_STRING(pLinkedCellInfo->pszDomain);
    LsaFreeMemory(pLinkedCellInfo);
}

DWORD
AD_OnlineFindUserObjectByName(
    HANDLE  hProvider,
    PCSTR   pszLoginId,    
    PAD_SECURITY_OBJECT* ppCachedUser
    )
{
    DWORD dwError = 0;
    
    DWORD dwFlags = 0;
    PLSA_LOGIN_NAME_INFO pUserNameInfo = NULL;
    PSTR  pszLoginId_copy = NULL;
    PSTR  pszLookupName = NULL;
    HANDLE hDb = (HANDLE)NULL;
    PAD_SECURITY_OBJECT pCachedUser = NULL;
    PSTR pszObjectDN = NULL;
    PSTR pszObjectDomainName = NULL;
    PSTR pszObjectSamaccountName = NULL;
    TrustMode trustMode = UnHandledTrust;   
    
    BAIL_ON_INVALID_STRING(pszLoginId);
    
    if (AD_GetLDAPSignAndSeal()) {
       dwFlags |= LSA_LDAP_OPT_SIGN_AND_SEAL;
    }
    
    dwError = LsaAllocateString(
                    pszLoginId,
                    &pszLoginId_copy);
    BAIL_ON_LSA_ERROR(dwError);
    
    LsaStrCharReplace(pszLoginId_copy, AD_GetSeparator(),' ');
    
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
        goto FoundInCacheValid;
    }
    
    if (dwError != LSA_ERROR_NOT_HANDLED){
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserNameInfo->nameType == NameType_Alias)
    {
        dwError = ADLdap_FindUserNameByAlias(
                         pszLoginId_copy,
                         &pszLookupName);
        BAIL_ON_LSA_ERROR(dwError);
        
        if (pUserNameInfo)
        {
            LsaFreeNameInfo(pUserNameInfo);
            pUserNameInfo = NULL;
        }
        
        //pszUpnName is formatted as NT4, crack again to fill in pUserNameInfo
        dwError = LsaCrackDomainQualifiedName(
                            pszLookupName,
                            NULL,
                            &pUserNameInfo);
        BAIL_ON_LSA_ERROR(dwError);
       
    }
    else
    {    
        dwError = LsaAllocateString(
                     pszLoginId_copy,
                     &pszLookupName);
        BAIL_ON_LSA_ERROR(dwError);
    }

    LSA_SAFE_FREE_STRING(pUserNameInfo->pszFullDomainName);
    
    //Get the trustMode and complete filling in correct information for pUserNameInfo
    dwError = AD_DetermineTrustModeandDomainName(
                        pUserNameInfo->pszDomainNetBiosName,                        
                        &trustMode,
                        &pUserNameInfo->pszFullDomainName,
                        NULL);
    BAIL_ON_LSA_ERROR(dwError);
    
    LSA_SAFE_FREE_STRING(pUserNameInfo->pszDomainNetBiosName);
    
    dwError = ADGetDomainNetBios(
                pUserNameInfo->pszFullDomainName,
                &pUserNameInfo->pszDomainNetBiosName);
    BAIL_ON_LSA_ERROR(dwError);
    
    LSA_SAFE_FREE_STRING(pUserNameInfo->pszObjectSid);
    
    dwError = LsaDmWrapNetLookupObjectSidByName(
                gpADProviderData->szDomain,
                pszLookupName,
                &pUserNameInfo->pszObjectSid);
    BAIL_ON_LSA_ERROR(dwError);
    
    switch (trustMode){
        case OneWayTrust:
            
               switch (gpADProviderData->dwDirectoryMode) 
               {                        
                   case DEFAULT_MODE:      
                   case UNPROVISIONED_MODE:
                          dwError = LSA_ERROR_NO_SUCH_USER;
                           
                           break;                        
                           
                   case CELL_MODE:
                           dwError = CellModeFindUserByName(
                                           gpADProviderData->szDomain,                                           
                                           gpADProviderData->cell.szCellDN,
                                           NULL,
                                           pUserNameInfo,
                                           &pCachedUser);                          
                           
                           break;
                           
                   default:
                           dwError = LSA_ERROR_NOT_HANDLED;
                                               
               }
               BAIL_ON_LSA_ERROR(dwError);
            
            break;
            
        case TwoWayTrust_inforest:                
            dwError =ADLdap_GetGCObjectInfoBySid(
                    gpADProviderData->szDomain,            
                    pUserNameInfo->pszObjectSid,
                    &pszObjectDN,
                    &pszObjectDomainName,
                    &pszObjectSamaccountName);
            BAIL_ON_LSA_ERROR(dwError);
            
            break;
            
        case TwoWayTrust_acrossforest:
            dwError =ADLdap_GetGCObjectInfoBySid(
                    pUserNameInfo->pszFullDomainName,            
                    pUserNameInfo->pszObjectSid,
                    &pszObjectDN,
                    &pszObjectDomainName,
                    &pszObjectSamaccountName);
            BAIL_ON_LSA_ERROR(dwError);
                            
            break;
             
        case OneSelfTrust:  //Single dc scenario
            dwError = LsaAllocateString(gpADProviderData->szDomain,
                                        &pszObjectDomainName);
            BAIL_ON_LSA_ERROR(dwError);           
            
             break;
                 
        default:
            dwError = LSA_ERROR_NOT_HANDLED;
            BAIL_ON_LSA_ERROR(dwError);
    }        
    
    if (trustMode == TwoWayTrust_inforest || trustMode == TwoWayTrust_acrossforest || trustMode == OneSelfTrust){
        
         switch (gpADProviderData->dwDirectoryMode) 
         {
             case DEFAULT_MODE:
                     dwError = DefaultModeFindUserByName(
                                     pszObjectDomainName,
                                     pUserNameInfo,
                                     &pCachedUser);       
                     
                     break;
    
             case CELL_MODE:
                     dwError = CellModeFindUserByName(
                                     gpADProviderData->szDomain,
                                     gpADProviderData->cell.szCellDN,
                                     pszObjectDomainName,
                                     pUserNameInfo,
                                     &pCachedUser);                     
                     
                     break;
    
             case UNPROVISIONED_MODE:
                     dwError = UnprovisionedModeFindUserByName(
                                     pszObjectDomainName,             
                                     pUserNameInfo,
                                     &pCachedUser);                     
                     
                     break;
    
             default:
                     dwError = LSA_ERROR_NOT_HANDLED;                     
         }
         BAIL_ON_LSA_ERROR(dwError); 
    }    

    dwError = ADCacheDB_CacheObjectEntry(hDb, pCachedUser);
    BAIL_ON_LSA_ERROR(dwError);

FoundInCacheValid:
    
    *ppCachedUser = pCachedUser;    
       
cleanup:

    ADCacheDB_SafeCloseDb(&hDb);

    if (pUserNameInfo) {
        LsaFreeNameInfo(pUserNameInfo);
    }
    
    LSA_SAFE_FREE_STRING(pszLoginId_copy);    
    LSA_SAFE_FREE_STRING(pszLookupName);    
    
    LSA_SAFE_FREE_STRING(pszObjectDN);    
    LSA_SAFE_FREE_STRING(pszObjectDomainName);
    LSA_SAFE_FREE_STRING(pszObjectSamaccountName);

    return dwError;

error:

    *ppCachedUser = NULL;
    
    ADCacheDB_SafeFreeObject(&pCachedUser);
    
    if (dwError != LSA_ERROR_NO_SUCH_USER) {
       LSA_LOG_DEBUG("Failed to find user [error code:%d]", dwError);
       dwError = LSA_ERROR_NO_SUCH_USER;
    }

    goto cleanup;
}

DWORD
AD_FindCachedGroupByNT4Name(
    HANDLE  hProvider,
    PCSTR   pszGroupName,    
    PAD_SECURITY_OBJECT* ppCachedGroup
    )
{
    DWORD dwError = 0;
    //handle the current domain directory, which is where all the psedu objects shall be found except for default mode
    HANDLE hPseudoDirectory = (HANDLE)NULL; 
    //handle the domain directory where the real object resides (for all modes)
    HANDLE hRealDirectory = (HANDLE)NULL; 
    
    DWORD dwFlags = 0;
    PLSA_LOGIN_NAME_INFO pGroupNameInfo = NULL;
    PSTR  pszGroupName_copy = NULL;
    HANDLE hDb = (HANDLE)NULL;
    PAD_SECURITY_OBJECT pCachedGroup = NULL;
    PSTR pszObjectDN = NULL;
    PSTR pszObjectDomainName = NULL;
    PSTR pszObjectSamaccountName = NULL;
    TrustMode trustMode = UnHandledTrust;
    
    
    BAIL_ON_INVALID_STRING(pszGroupName);
    
    if (AD_GetLDAPSignAndSeal()) {
       dwFlags |= LSA_LDAP_OPT_SIGN_AND_SEAL;
    }
    
    dwError = LsaAllocateString(
                    pszGroupName,
                    &pszGroupName_copy);
    BAIL_ON_LSA_ERROR(dwError);
    
    LsaStrCharReplace(pszGroupName_copy, AD_GetSeparator(),' ');
    
    dwError = LsaCrackDomainQualifiedName(
                        pszGroupName_copy,
                        gpADProviderData->szDomain,
                        &pGroupNameInfo);    
    BAIL_ON_LSA_ERROR(dwError);

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
    
    if (dwError == 0){
        
        LSA_SAFE_FREE_STRING(pGroupNameInfo->pszFullDomainName);
        
        dwError = AD_DetermineTrustModeandDomainName(
                            pGroupNameInfo->pszDomainNetBiosName,                        
                            &trustMode,
                            &pGroupNameInfo->pszFullDomainName,
                            NULL);
        BAIL_ON_LSA_ERROR(dwError);
        
        goto FoundInCacheValid;
    }
    
    if (dwError != LSA_ERROR_NOT_HANDLED){
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    LSA_SAFE_FREE_STRING(pGroupNameInfo->pszFullDomainName);
    
    //Get the trustMode and complete filling in correct information for pUserNameInfo
    dwError = AD_DetermineTrustModeandDomainName(
                        pGroupNameInfo->pszDomainNetBiosName,                        
                        &trustMode,
                        &pGroupNameInfo->pszFullDomainName,
                        NULL);
    BAIL_ON_LSA_ERROR(dwError);
    
    LSA_SAFE_FREE_STRING(pGroupNameInfo->pszDomainNetBiosName);
    
    dwError = ADGetDomainNetBios(
                pGroupNameInfo->pszFullDomainName,
                &pGroupNameInfo->pszDomainNetBiosName);
    BAIL_ON_LSA_ERROR(dwError);
    
    LSA_SAFE_FREE_STRING(pGroupNameInfo->pszObjectSid);
    
    dwError = LsaDmWrapNetLookupObjectSidByName(
                gpADProviderData->szDomain,
                pszGroupName_copy,
                &pGroupNameInfo->pszObjectSid);
    BAIL_ON_LSA_ERROR(dwError);
    
    switch (trustMode){
        case OneWayTrust:
            
               switch (gpADProviderData->dwDirectoryMode) 
               {                        
                   case DEFAULT_MODE:      
                   case UNPROVISIONED_MODE:
                          dwError = LSA_ERROR_NO_SUCH_GROUP;
                           
                           break;                        
                           
                   case CELL_MODE:
                           dwError = CellModeFindGroupByName(
                                           gpADProviderData->szDomain,                                           
                                           gpADProviderData->cell.szCellDN,
                                           NULL,
                                           pGroupNameInfo,
                                           &pCachedGroup);
                           
                           break;
                           
                   default:
                           dwError = LSA_ERROR_NOT_HANDLED;
                                               
               }
               BAIL_ON_LSA_ERROR(dwError);
            
            break;
            
        case TwoWayTrust_inforest:                
            dwError =ADLdap_GetGCObjectInfoBySid(
                    gpADProviderData->szDomain,            
                    pGroupNameInfo->pszObjectSid,
                    &pszObjectDN,
                    &pszObjectDomainName,
                    &pszObjectSamaccountName);
            BAIL_ON_LSA_ERROR(dwError);
            
            break;
            
        case TwoWayTrust_acrossforest:
            dwError =ADLdap_GetGCObjectInfoBySid(
                    pGroupNameInfo->pszFullDomainName,            
                    pGroupNameInfo->pszObjectSid,
                    &pszObjectDN,
                    &pszObjectDomainName,
                    &pszObjectSamaccountName);
            BAIL_ON_LSA_ERROR(dwError);
                            
            break;
             
        case OneSelfTrust:  //Single dc scenario
            dwError = LsaAllocateString(gpADProviderData->szDomain,
                                        &pszObjectDomainName);
            BAIL_ON_LSA_ERROR(dwError);           
            
             break;
                 
        default:
            dwError = LSA_ERROR_NOT_HANDLED;
            BAIL_ON_LSA_ERROR(dwError);
    }        
    
    if (trustMode == TwoWayTrust_inforest || trustMode == TwoWayTrust_acrossforest || trustMode == OneSelfTrust){
        
         switch (gpADProviderData->dwDirectoryMode) 
         {
             case DEFAULT_MODE:

                     dwError = DefaultModeFindGroupByName(
                                     pszObjectDomainName,
                                     pGroupNameInfo,
                                     &pCachedGroup);       
                
                     break;
    
             case CELL_MODE:
                     dwError = CellModeFindGroupByName(
                                     gpADProviderData->szDomain,
                                     gpADProviderData->cell.szCellDN,
                                     pszObjectDomainName,
                                     pGroupNameInfo,
                                     &pCachedGroup);                     
                     
                     break;
    
             case UNPROVISIONED_MODE:
                     dwError = UnprovisionedModeFindGroupByName(
                                     pszObjectDomainName,             
                                     pGroupNameInfo,
                                     &pCachedGroup);     

                     
                     break;
    
             default:
                     dwError = LSA_ERROR_NOT_HANDLED;                     
         }
         BAIL_ON_LSA_ERROR(dwError); 
    }    

    dwError = ADCacheDB_CacheObjectEntry(hDb, pCachedGroup);
    BAIL_ON_LSA_ERROR(dwError);
    
FoundInCacheValid:

    *ppCachedGroup = pCachedGroup;
     
    
cleanup:

    if (hPseudoDirectory != (HANDLE)NULL) {
        LsaLdapCloseDirectory(hPseudoDirectory);
    }
    
    if (hRealDirectory != (HANDLE)NULL) {
        LsaLdapCloseDirectory(hRealDirectory);
    }
    
    ADCacheDB_SafeCloseDb(&hDb);
    
    if (pGroupNameInfo) {
        LsaFreeNameInfo(pGroupNameInfo);
    }
    
    LSA_SAFE_FREE_STRING(pszGroupName_copy);    
    
    LSA_SAFE_FREE_STRING(pszObjectDN);
    LSA_SAFE_FREE_STRING(pszObjectDomainName);
    LSA_SAFE_FREE_STRING(pszObjectSamaccountName);        
    
    return dwError;

error:

    *ppCachedGroup = NULL;
    ADCacheDB_SafeFreeObject(&pCachedGroup);
    
    if (dwError != LSA_ERROR_NO_SUCH_GROUP) {
       LSA_LOG_DEBUG("Failed to find group [error code:%d]", dwError);
       dwError = LSA_ERROR_NO_SUCH_GROUP;
    }

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
                pEnumState->dwMapType,
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
