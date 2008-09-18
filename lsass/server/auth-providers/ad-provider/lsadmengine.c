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

/**
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * @file
 *
 *     lsadmengine.c
 *
 * @brief
 *
 *     LSASS Domain Manager (LsaDm) Engine Implementation
 *
 * @details
 *
 *     This module discovers trusts and when domains come back online.
 *     It builds on top of the LsaDm state module.
 *
 * @author Danilo Almeida (dalmeida@likewisesoftware.com)
 *
 */

#include "adprovider.h"

DWORD
LsaDmEnginepAddTrust(
    IN OPTIONAL PCSTR pszTrusteeDomainName,
    IN NetrDomainTrust* pTrustInfo,
    IN PCSTR pszDnsForestName
    )
{
    DWORD dwError = 0;
    PSTR pszDnsDomainName = NULL;
    PSTR pszNetbiosDomainName = NULL;

    dwError = LsaWc16sToMbs(pTrustInfo->dns_name,
                            &pszDnsDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaWc16sToMbs(pTrustInfo->netbios_name,
                            &pszNetbiosDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaDmAddTrustedDomain(pszDnsDomainName,
                                    pszNetbiosDomainName,
                                    pTrustInfo->sid,
                                    &pTrustInfo->guid,
                                    pszTrusteeDomainName,
                                    pTrustInfo->trust_flags,
                                    pTrustInfo->trust_type,
                                    pTrustInfo->trust_attrs,
                                    pszDnsForestName,
                                    NULL);
    if (dwError == LSA_ERROR_DUPLICATE_DOMAINNAME)
    {
        // We enumerate at the current domain we are joined to
        // And later we might enumerate at the forest root
        // Since we request all members of the forest at each
        // query, we would get duplicates.
        // Ignore the duplicates.
        dwError = 0;
    }
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    LSA_SAFE_FREE_MEMORY(pszDnsDomainName);
    LSA_SAFE_FREE_MEMORY(pszNetbiosDomainName);
    return dwError;

error:
    goto cleanup;
}

static
DWORD
LsaDmEnginepDiscoverTrustsForDomain(
    IN PCSTR pszParentTrusteeDomainName,
    IN PCSTR pszDomainName,
    IN PCSTR pszForestName,
    OUT PSTR** ppTrustedForestRootList
    )
{
    DWORD dwError = 0;
    NetrDomainTrust* pTrusts = NULL;
    DWORD dwTrustCount = 0;
    DWORD dwTrustIndex = 0;
    PSTR pszDnsDomainName = NULL;
    PSTR pszNetbiosName = NULL;
    NetrDomainTrust* pPrimaryTrust = NULL;
    PSTR* pTrustedForestRootList = NULL;
    DWORD dwTrustedForestRootListCount = 0;
    PLWNET_DC_INFO pDcInfo = NULL;

    // Enumerate all direct and in-forest trusts.
    // ISSUE-2008/08/14-dalmeida -- Why not just direct outbound and in-forest?
    dwError = LsaDmWrapDsEnumerateDomainTrusts(pszDomainName,
                                               (NETR_TRUST_FLAG_OUTBOUND |
                                                NETR_TRUST_FLAG_INBOUND |
                                                NETR_TRUST_FLAG_IN_FOREST),
                                               &pTrusts,
                                               &dwTrustCount);
    BAIL_ON_LSA_ERROR(dwError);

    // There must be at least one trust (primary)
    if (dwTrustCount < 1)
    {
        LSA_LOG_DEBUG("No entries returned from trust enumeration for '%s'",
                      pszDomainName);
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    // First, if we are the primary domain find primary trust info
    // so that we can add it first.
    if (!pszParentTrusteeDomainName)
    {
        // Since there is no trustee, this is the machine's domain.
        // So let's find the entry for the primary domain.
        for (dwTrustIndex = 0; dwTrustIndex < dwTrustCount; dwTrustIndex++)
        {
            NetrDomainTrust* pCurrentTrust = &pTrusts[dwTrustIndex];
            if (pCurrentTrust->trust_flags & NETR_TRUST_FLAG_PRIMARY)
            {
                if (pPrimaryTrust)
                {
                    // multiple primary trusts are not allowed
                    dwError = LSA_ERROR_INTERNAL;
                    BAIL_ON_LSA_ERROR(dwError);
                }
                pPrimaryTrust = pCurrentTrust;
            }
        }
        // If we did not find the primary trust info, become sad.
        if (!pPrimaryTrust)
        {
            dwError = LSA_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
        }

        // double-check that this is the expected domain name
        dwError = LsaWc16sToMbs(pPrimaryTrust->dns_name, &pszDnsDomainName);
        BAIL_ON_LSA_ERROR(dwError);

        if (strcasecmp(pszDnsDomainName, pszDomainName))
        {
            LSA_LOG_DEBUG("Primary domain mismatch: got '%s', wanted '%s'", pszDnsDomainName, pszDomainName);
            dwError = LSA_ERROR_DATA_ERROR;
            BAIL_ON_LSA_ERROR(dwError);
        }

        // Now add the primary trust info.
        dwError = LsaDmEnginepAddTrust(NULL, pPrimaryTrust, pszForestName);
        BAIL_ON_LSA_ERROR(dwError);
    }

    // allocate trusted forest root results, if the caller wants it.
    if (ppTrustedForestRootList)
    {
        dwError = LsaAllocateMemory(sizeof(pTrustedForestRootList[0]) * dwTrustCount,
                                    (PVOID*)&pTrustedForestRootList);
        BAIL_ON_LSA_ERROR(dwError);
    }

    for (dwTrustIndex = 0; dwTrustIndex < dwTrustCount; dwTrustIndex++)
    {
        NetrDomainTrust* pCurrentTrust = &pTrusts[dwTrustIndex];
        PCSTR pszCurrentTrustForestRootName = NULL;

        // Ignore DOWNLEVEL trusts.
        // These are trusts with domains that are earlier than WIN2K
        if (pCurrentTrust->trust_type == NETR_TRUST_TYPE_DOWNLEVEL)
        {
            LSA_SAFE_FREE_STRING(pszNetbiosName);
            
            if (pCurrentTrust->netbios_name)
            {
                dwError = LsaWc16sToMbs(
                                pCurrentTrust->netbios_name,
                                &pszNetbiosName);
                BAIL_ON_LSA_ERROR(dwError);
            }
            
            LSA_LOG_WARNING("Ignoring down level trust to domain [%s]",
                            IsNullOrEmptyString(pszNetbiosName) ? "" : pszNetbiosName);
            
            continue;
        }

        // if we do not want forest root links, we are enumerating a
        // forest transitive trust, in which case we just want in-forest
        // trusts.
        if (!ppTrustedForestRootList && 
            !(pCurrentTrust->trust_flags & NETR_TRUST_FLAG_IN_FOREST))
        {
            continue;
        }

        // skip the primary trust.
        if (pCurrentTrust->trust_flags & NETR_TRUST_FLAG_PRIMARY)
        {
            if (pszParentTrusteeDomainName)
            {
                // We try to catch multiple primary trusts here
                // in case we did not check in the non-trustee case
                // earlier.
                if (pPrimaryTrust)
                {
                    // multiple primary trusts are not allowed
                    dwError = LSA_ERROR_INTERNAL;
                    BAIL_ON_LSA_ERROR(dwError);
                }
                pPrimaryTrust = pCurrentTrust;
            }
            continue;
        }

        LSA_SAFE_FREE_MEMORY(pszDnsDomainName);
        
        if (pCurrentTrust->dns_name)
        {
            dwError = LsaWc16sToMbs(pCurrentTrust->dns_name, &pszDnsDomainName);
            BAIL_ON_LSA_ERROR(dwError);
        }
        
        if (IsNullOrEmptyString(pszDnsDomainName))
        {
            LSA_SAFE_FREE_STRING(pszNetbiosName);
            
            if (pCurrentTrust->netbios_name)
            {
                dwError = LsaWc16sToMbs(
                                pCurrentTrust->netbios_name,
                                &pszNetbiosName);
                BAIL_ON_LSA_ERROR(dwError);
            }
            
            LSA_LOG_WARNING("Skipping trust with an invalid dns domain name. Netbios name [%s]",
                            IsNullOrEmptyString(pszNetbiosName) ? "" : pszNetbiosName);
            
            continue;
        }
        
        // skip back-link.
        if (pszParentTrusteeDomainName &&
            !strcasecmp(pszDnsDomainName, pszParentTrusteeDomainName))
        {
            continue;
        }

        if (pCurrentTrust->trust_flags & NETR_TRUST_FLAG_IN_FOREST)
        {
            pszCurrentTrustForestRootName = pszForestName;
        }
        else
        {
            LWNET_SAFE_FREE_DC_INFO(pDcInfo);
            
            dwError = LWNetGetDCName(NULL,
                                     pszDnsDomainName,
                                     NULL,
                                     0,
                                     &pDcInfo);
            BAIL_ON_LSA_ERROR(dwError);
            
            pszCurrentTrustForestRootName = pDcInfo->pszDnsForestName;
        }

        // add the trust.
        dwError = LsaDmEnginepAddTrust(pszDomainName, pCurrentTrust,
                                       pszCurrentTrustForestRootName);
        BAIL_ON_LSA_ERROR(dwError);

        // if the caller wants trusted forest roots, add them
        if (ppTrustedForestRootList &&
            (pCurrentTrust->trust_attrs & NETR_TRUST_ATTR_FOREST_TRANSITIVE) &&
            (pCurrentTrust->trust_flags & NETR_TRUST_FLAG_OUTBOUND) &&
            (pCurrentTrust->trust_flags & NETR_TRUST_FLAG_INBOUND))
        {
            // note that we only want 2-way trusts because otherwise
            // we cannot enumerate the trusted forest.
            if (pCurrentTrust->trust_flags & NETR_TRUST_FLAG_IN_FOREST)
            {
                LSA_LOG_WARNING("Skipping unexpected in-forest 2-way transitive forest trust '%s' -> '%s'",
                                pszDomainName, pszDnsDomainName);
                continue;
            }

            dwError = LsaAllocateString(pszDnsDomainName,
                                        &pTrustedForestRootList[dwTrustedForestRootListCount]);
            BAIL_ON_LSA_ERROR(dwError);

            dwTrustedForestRootListCount++;
        }
    }
    
    if (!dwTrustedForestRootListCount)
    {
        // Return NULL rather than an empty list
        LSA_SAFE_FREE_STRING_ARRAY(pTrustedForestRootList);
    }

cleanup:

    LWNET_SAFE_FREE_DC_INFO(pDcInfo);
    LSA_SAFE_FREE_STRING(pszNetbiosName);
    LSA_SAFE_FREE_MEMORY(pszDnsDomainName);

    if (pTrusts)
    {
        NetrFreeMemory(pTrusts);
    }

    if (ppTrustedForestRootList)
    {
        *ppTrustedForestRootList = pTrustedForestRootList;
    }

    return dwError;
    
error:
    LSA_SAFE_FREE_STRING_ARRAY(pTrustedForestRootList);
    goto cleanup;
}

DWORD
LsaDmEngineDiscoverTrusts(
    IN PCSTR pszDnsPrimaryDomainName,
    IN PCSTR pszDnsPrimaryForestName
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    PSTR* pTrustedForestRootList = NULL;
    BOOLEAN bIsForestRoot = FALSE;

    // determine whether the primary domain is a forest root
    if (!strcasecmp(pszDnsPrimaryForestName, pszDnsPrimaryDomainName))
    {
        bIsForestRoot = TRUE;
    }

    // enumerate own domain's trusts
    dwError = LsaDmEnginepDiscoverTrustsForDomain(NULL,
                                                  pszDnsPrimaryDomainName,
                                                  pszDnsPrimaryForestName,
                                                  &pTrustedForestRootList);
    BAIL_ON_LSA_ERROR(dwError);

    // If own domain is not the forest root, enumerate the forest root
    if (!bIsForestRoot)
    {
        if (pTrustedForestRootList)
        {
            LSA_LOG_ERROR("Unexpected trusted forest root list when "
                          "enumerating trusts for '%s'", pszDnsPrimaryDomainName);
            dwError = LSA_ERROR_DATA_ERROR;
            BAIL_ON_LSA_ERROR(dwError);
        }

        dwError = LsaDmEnginepDiscoverTrustsForDomain(pszDnsPrimaryDomainName,
                                                      pszDnsPrimaryForestName,
                                                      pszDnsPrimaryForestName,
                                                      &pTrustedForestRootList);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pTrustedForestRootList)
    {
        size_t i;
        for (i = 0; pTrustedForestRootList[i]; i++)
        {
            PSTR pszDnsForestName = pTrustedForestRootList[i];

            dwError = LsaDmEnginepDiscoverTrustsForDomain(pszDnsPrimaryForestName,
                                                          pszDnsForestName,
                                                          pszDnsForestName,
                                                          NULL);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

cleanup:
    LSA_SAFE_FREE_STRING_ARRAY(pTrustedForestRootList);

    return dwError;
    
error:
    goto cleanup;
}

