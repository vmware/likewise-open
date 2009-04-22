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

static
DWORD
LsaDmEnginepAddTrust(
    IN OPTIONAL PCSTR pszTrusteeDomainName,
    IN NetrDomainTrust* pTrustInfo,
    IN LSA_TRUST_DIRECTION dwTrustDirection,
    IN LSA_TRUST_MODE dwTrustMode,
    IN BOOLEAN bIsTransitiveOnewayChild,
    IN OPTIONAL PCSTR pszDnsForestName
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
                                    dwTrustDirection,
                                    dwTrustMode,
                                    bIsTransitiveOnewayChild,
                                    pszDnsForestName,
                                    NULL);
    if (dwError == LSA_ERROR_DUPLICATE_DOMAINNAME ||
        dwError == LSA_ERROR_NO_SUCH_DOMAIN)
    {
        // We enumerate at the current domain we are joined to
        // And later we might enumerate at the forest root
        // Since we request all members of the forest at each
        // query, we would get duplicates.
        // Ignore the duplicates, and also ignore the domains without domainSid.
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
    if (LSA_ERROR_DOMAIN_IS_OFFLINE == dwError)
    {
        LSA_LOG_ERROR("Unable to enumerate trusts for '%s' domain because it is offline",
                      pszDomainName, dwError);
    }
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
        dwError = LsaDmEnginepAddTrust(
                        NULL,
                        pPrimaryTrust,
                        LSA_TRUST_DIRECTION_SELF,
                        LSA_TRUST_MODE_MY_FOREST,
                        FALSE,
                        pszForestName);
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
        LSA_TRUST_DIRECTION dwTrustDirection = LSA_TRUST_DIRECTION_UNKNOWN;
        LSA_TRUST_MODE dwTrustMode = LSA_TRUST_MODE_UNKNOWN;

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

            LSA_LOG_WARNING("Ignoring down level trust to domain '%s'",
                            LSA_SAFE_LOG_STRING(pszNetbiosName));

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

            LSA_LOG_WARNING("Skipping trust with an invalid DNS domain name (Netbios name is '%s')",
                            LSA_SAFE_LOG_STRING(pszNetbiosName));

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
            // check whether we are in other forest or not
            if (ppTrustedForestRootList)
            {
                dwTrustMode = LSA_TRUST_MODE_MY_FOREST;

            }
            else
            {
                dwTrustMode = LSA_TRUST_MODE_OTHER_FOREST;
            }
         }
        else if (pCurrentTrust->trust_attrs & NETR_TRUST_ATTR_FOREST_TRANSITIVE)
        {
            // This is a forest trust, so the forest name is the same as the
            // domain name.
            pszCurrentTrustForestRootName = pszDnsDomainName;
            dwTrustMode = LSA_TRUST_MODE_OTHER_FOREST;
        }
        else
        {
            // This must be an external trust.  So we do not ever
            // need the forest name as we are not supposed to
            // ever talk to it.
            pszCurrentTrustForestRootName = NULL;
            dwTrustMode = LSA_TRUST_MODE_EXTERNAL;
        }

        //Determine trust direction
        if (pCurrentTrust->trust_flags & NETR_TRUST_FLAG_IN_FOREST)
        {
            dwTrustDirection = LSA_TRUST_DIRECTION_TWO_WAY;
        }
        else if ((pCurrentTrust->trust_flags & NETR_TRUST_FLAG_OUTBOUND) &&
                (pCurrentTrust->trust_flags & NETR_TRUST_FLAG_INBOUND))
        {
           dwTrustDirection = LSA_TRUST_DIRECTION_TWO_WAY;
        }
        else if ((pCurrentTrust->trust_flags & NETR_TRUST_FLAG_OUTBOUND) &&
                !(pCurrentTrust->trust_flags & NETR_TRUST_FLAG_INBOUND))
        {
           dwTrustDirection = LSA_TRUST_DIRECTION_ONE_WAY;
        }
        else if (!(pCurrentTrust->trust_flags & NETR_TRUST_FLAG_OUTBOUND) &&
                (pCurrentTrust->trust_flags & NETR_TRUST_FLAG_INBOUND))
        {
           dwTrustDirection = LSA_TRUST_DIRECTION_ZERO_WAY;
        }
        else
        {
           dwTrustDirection = LSA_TRUST_DIRECTION_UNKNOWN;
           LSA_LOG_WARNING("Trust direction cannot be determined.");
        }

        // Add the trust.
        dwError = LsaDmEnginepAddTrust(
                         pszDomainName,
                         pCurrentTrust,
                         dwTrustDirection,
                         dwTrustMode,
                         FALSE,
                         pszCurrentTrustForestRootName);
        BAIL_ON_LSA_ERROR(dwError);

        // If the caller wants trusted forest roots, add them to the
        // output list of trusted forest roots.
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

static
DWORD
LsaDmEnginepDiscoverTrustsInternal(
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
        if (LSA_ERROR_DOMAIN_IS_OFFLINE == dwError)
        {
            // If we cannot enumerate our forest's trusts, ignore it.
            dwError = 0;
        }
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
            if (LSA_ERROR_DOMAIN_IS_OFFLINE == dwError)
            {
                // If we cannot enumerate a trusted forest's trusts,
                // ignore it.
                dwError = 0;
            }
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

cleanup:
    LSA_SAFE_FREE_STRING_ARRAY(pTrustedForestRootList);

    return dwError;

error:
    goto cleanup;
}

DWORD
LsaDmEngineDiscoverTrusts(
    IN PCSTR pszDnsPrimaryDomainName
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    PLWNET_DC_INFO pDcInfo = NULL;

    // ISSUE-2008/10/09-dalmeida -- Perhaps put this in lsadmwrap.
    dwError = LWNetGetDCName(NULL, pszDnsPrimaryDomainName, NULL, 0, &pDcInfo);
    switch (dwError)
    {
        case LWNET_ERROR_INVALID_DNS_RESPONSE:
        case LWNET_ERROR_FAILED_FIND_DC:
            // We pinged a DC earlier, so we must have gone offline
            // in the last few seconds.
            dwError = LSA_ERROR_DOMAIN_IS_OFFLINE;
            break;
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaDmEnginepDiscoverTrustsInternal(pszDnsPrimaryDomainName,
                                                 pDcInfo->pszDnsForestName);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    LWNET_SAFE_FREE_DC_INFO(pDcInfo);

    return dwError;

error:
    goto cleanup;
}

DWORD
LsaDmEngineGetDomainInfoWithNT4Name(
    IN PCSTR pszDomainName,
    IN PCSTR pszObjectName,
    OUT OPTIONAL PSTR* ppszDnsDomainName,
    OUT OPTIONAL PSTR* ppszNetBiosDomainName
    )
{
    DWORD dwError = 0;
    PSTR pszDnsDomainName = NULL;
    PSTR pszNetbiosDomainName = NULL;
    PSTR pszObjectSid = NULL;
    PSID pSid = NULL;
    uuid_t guid = {0};
    PSTR pszDomainDnsNameFromDsGetDcName = NULL;
    PSTR pszDomainForestNameFromDsGetDcName = NULL;
    PLSA_DM_ENUM_DOMAIN_INFO pDomainInfo = NULL;

    if (IsNullOrEmptyString(pszDomainName) ||
        !strcasecmp(pszDomainName, "BUILTIN") ||
        !strcasecmp(pszDomainName, "NT AUTHORITY"))
    {
        dwError = LSA_ERROR_NO_SUCH_DOMAIN;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaDmWrapGetDomainName(pszDomainName,
                                     &pszDnsDomainName,
                                     &pszNetbiosDomainName);
    if (LSA_ERROR_NO_SUCH_DOMAIN == dwError)
    {
        dwError = LsaDmWrapNetLookupObjectSidByName(
                     gpADProviderData->szDomain,
                     pszObjectName,
                     &pszObjectSid,
                     NULL);
        if (!dwError && !IsNullOrEmptyString(pszObjectSid))
        {
            //Add this newly discovered domain to trusted domain list
            dwError = LsaDmWrapDsGetDcName(
                                    gpADProviderData->szDomain,
                                    pszDomainName,
                                    TRUE,
                                    &pszDomainDnsNameFromDsGetDcName,
                                    &pszDomainForestNameFromDsGetDcName);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaDmWrapDsGetDcName(
                                    gpADProviderData->szDomain,
                                    pszDomainName,
                                    FALSE,
                                    &pszNetbiosDomainName,
                                    NULL);
            BAIL_ON_LSA_ERROR(dwError);

            // Obtain domain's pSID
            dwError = LsaAllocateSidFromCString(&pSid, pszObjectSid);
            BAIL_ON_LSA_ERROR(dwError);

            if (pSid->SubAuthorityCount < 1)
            {
                dwError = LSA_ERROR_INVALID_SID;
                BAIL_ON_LSA_ERROR(dwError);
            }

            pSid->SubAuthorityCount--;

            dwError = LsaDmAddTrustedDomain(
                              pszDomainDnsNameFromDsGetDcName,
                              pszNetbiosDomainName,
                              pSid,
                              &guid,
                              gpADProviderData->szDomain,
                              0,
                              0,
                              0,
                              LSA_TRUST_DIRECTION_ONE_WAY,
                              LSA_TRUST_MODE_OTHER_FOREST,
                              TRUE,
                              pszDomainForestNameFromDsGetDcName,
                              NULL);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaDmWrapGetDomainEnumInfo(
                            pszDomainDnsNameFromDsGetDcName,
                            &pDomainInfo);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = ADState_AddDomainTrust(
                           gpLsaAdProviderState->hStateConnection,
                           pDomainInfo);
            BAIL_ON_LSA_ERROR(dwError);

            LSA_SAFE_FREE_STRING(pszDnsDomainName);
            LSA_SAFE_FREE_STRING(pszNetbiosDomainName);

            dwError = LsaDmWrapGetDomainName(pszDomainName,
                                             &pszDnsDomainName,
                                             &pszNetbiosDomainName);
            BAIL_ON_LSA_ERROR(dwError);
        }
        else
        {
            dwError = LSA_ERROR_NO_SUCH_DOMAIN;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

cleanup:
    LSA_SAFE_FREE_STRING(pszObjectSid);
    LSA_SAFE_FREE_STRING(pszDomainDnsNameFromDsGetDcName);
    LSA_SAFE_FREE_STRING(pszDomainForestNameFromDsGetDcName);
    LSA_SAFE_FREE_MEMORY(pSid);
    LsaDmFreeEnumDomainInfo(pDomainInfo);

    if (ppszDnsDomainName)
    {
        *ppszDnsDomainName = pszDnsDomainName;
    }
    else
    {
        LSA_SAFE_FREE_STRING(pszDnsDomainName);
    }
    if (ppszNetBiosDomainName)
    {
        *ppszNetBiosDomainName = pszNetbiosDomainName;
    }
    else
    {
        LSA_SAFE_FREE_STRING(pszNetbiosDomainName);
    }

    return dwError;

error:
    LSA_SAFE_FREE_STRING(pszDnsDomainName);
    LSA_SAFE_FREE_STRING(pszNetbiosDomainName);

    if (LSA_ERROR_GET_DC_NAME_FAILED == dwError)
    {
        dwError = LSA_ERROR_NO_SUCH_DOMAIN;
    }

    goto cleanup;
}

#define OBJECTSID_PREFIX "S-1-5-21-"

DWORD
LsaDmEngineGetDomainInfoWithObjectSid(
    IN PCSTR pszObjectSid,
    OUT OPTIONAL PSTR* ppszDnsDomainName,
    OUT OPTIONAL PSTR* ppszNetBiosDomainName,
    OUT OPTIONAL PSTR* ppszDomainSid
    )
{
    DWORD dwError = 0;
    PSTR pszDnsDomainName = NULL;
    PSTR pszNetbiosDomainName = NULL;
    PSTR pszDomainSid = NULL;
    PSTR pszName = NULL;
    PSID pSid = NULL;
    uuid_t guid = {0};
    PSTR pszDomainDnsNameFromDsGetDcName = NULL;
    PSTR pszDomainForestNameFromDsGetDcName = NULL;
    PLSA_DM_ENUM_DOMAIN_INFO pDomainInfo = NULL;

    if (IsNullOrEmptyString(pszObjectSid) ||
        strncasecmp(pszObjectSid, OBJECTSID_PREFIX, sizeof(OBJECTSID_PREFIX)-1))
    {
        dwError = LSA_ERROR_NO_SUCH_DOMAIN;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaDmWrapGetDomainNameAndSidByObjectSid(
                    pszObjectSid,
                    &pszDnsDomainName,
                    &pszNetbiosDomainName,
                    &pszDomainSid);

    if (LSA_ERROR_NO_SUCH_DOMAIN == dwError)
    {
        dwError = LsaDmWrapNetLookupNameByObjectSid(
                     gpADProviderData->szDomain,
                     pszObjectSid,
                     &pszName,
                     NULL);
        if (!dwError && !IsNullOrEmptyString(pszName))
        {
            //Add this newly discovered domain to trusted domain list
            //Call DsGetDcName to have detail domain information
            dwError = LsaAdBatchGetDomainFromNT4Name(&pszNetbiosDomainName,
                                                     pszName);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaDmWrapDsGetDcName(
                                    gpADProviderData->szDomain,
                                    pszNetbiosDomainName,
                                    TRUE,
                                    &pszDomainDnsNameFromDsGetDcName,
                                    &pszDomainForestNameFromDsGetDcName);
            BAIL_ON_LSA_ERROR(dwError);

            // Obtain domain's pSID
            dwError = LsaAllocateSidFromCString(&pSid, pszObjectSid);
            BAIL_ON_LSA_ERROR(dwError);

            if (pSid->SubAuthorityCount < 1)
            {
                dwError = LSA_ERROR_INVALID_SID;
                BAIL_ON_LSA_ERROR(dwError);
            }

            pSid->SubAuthorityCount--;

            dwError = LsaDmAddTrustedDomain(
                              pszDomainDnsNameFromDsGetDcName,
                              pszNetbiosDomainName,
                              pSid,
                              &guid,
                              gpADProviderData->szDomain,
                              0,
                              0,
                              0,
                              LSA_TRUST_DIRECTION_ONE_WAY,
                              LSA_TRUST_MODE_OTHER_FOREST,
                              TRUE,
                              pszDomainForestNameFromDsGetDcName,
                              NULL);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaDmWrapGetDomainEnumInfo(
                            pszDomainDnsNameFromDsGetDcName,
                            &pDomainInfo);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = ADState_AddDomainTrust(
                           gpLsaAdProviderState->hStateConnection,
                           pDomainInfo);
            BAIL_ON_LSA_ERROR(dwError);

            LSA_SAFE_FREE_STRING(pszDnsDomainName);
            LSA_SAFE_FREE_STRING(pszNetbiosDomainName);
            LSA_SAFE_FREE_STRING(pszDomainSid);

            dwError = LsaDmWrapGetDomainNameAndSidByObjectSid(
                            pszObjectSid,
                            &pszDnsDomainName,
                            &pszNetbiosDomainName,
                            &pszDomainSid);
            BAIL_ON_LSA_ERROR(dwError);
        }
        else
        {
            dwError = LSA_ERROR_NO_SUCH_DOMAIN;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

cleanup:
    LSA_SAFE_FREE_STRING(pszName);
    LSA_SAFE_FREE_MEMORY(pSid);
    LSA_SAFE_FREE_STRING(pszDomainDnsNameFromDsGetDcName);
    LSA_SAFE_FREE_STRING(pszDomainForestNameFromDsGetDcName);
    LsaDmFreeEnumDomainInfo(pDomainInfo);

    if (ppszDnsDomainName)
    {
        *ppszDnsDomainName = pszDnsDomainName;
    }
    else
    {
        LSA_SAFE_FREE_STRING(pszDnsDomainName);
    }
    if (ppszNetBiosDomainName)
    {
        *ppszNetBiosDomainName = pszNetbiosDomainName;
    }
    else
    {
        LSA_SAFE_FREE_STRING(pszNetbiosDomainName);
    }
    if (ppszDomainSid)
    {
        *ppszDomainSid = pszDomainSid;
    }
    else
    {
        LSA_SAFE_FREE_STRING(pszDomainSid);
    }

    return dwError;

error:
    LSA_SAFE_FREE_STRING(pszDnsDomainName);
    LSA_SAFE_FREE_STRING(pszNetbiosDomainName);
    LSA_SAFE_FREE_STRING(pszDomainSid);

    if (LSA_ERROR_GET_DC_NAME_FAILED == dwError)
    {
        dwError = LSA_ERROR_NO_SUCH_DOMAIN;
    }

    goto cleanup;
}
