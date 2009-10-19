#include <stdafx.h>


DWORD
InitializeDomainManager(
	)
{
	dwError = LoadTrustInformationFromRegistry();
	BAIL_ON_ERROR(dwError);

	dwError = DomainManagerEnumerateTrusts();
	BAIL_ON_ERROR(dwError);

	dwError = WriteTrustInformationToRegistry();

	ENTER_TRUST_CRITICAL_SECTION();

	while(pTrustInfo) {

		dwError = DomainMgrAllocateTrustEntry(
						pTrustInfo,
						&pTrustEntry
						);
		BAIL_ON_ERROR(dwError);

		pTrustEntry->pNext = gpTrustEntry;
		gpTrustEntry = pTrustEntry;

	}

	LEAVE_TRUST_CRITICAL_SECTION();

	return;
}



DWORD
EnumTrustedDomains(
				   )
{
	DWORD dwError = 0;

	return(dwError);
}

DWORD
GetDomainInfo(
	PWSTR pszDomainName
	)
{
	DWORD dwError = 0;

	return dwError;
}


DWORD
UpdateTrustInformation()
{


}

DWORD
DomainManagerAddTrust(
    IN PCWSTR pszTrusteeDomainName,
    IN NetrDomainTrust* pTrustInfo,
    IN LSA_TRUST_DIRECTION dwTrustDirection,
    IN LSA_TRUST_MODE dwTrustMode,
    IN BOOLEAN bIsTransitiveOnewayChild,
    IN OPTIONAL PCWSTR pszDnsForestName
    )
{
    DWORD dwError = 0;

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
    if (dwError == LW_ERROR_DUPLICATE_DOMAINNAME ||
        dwError == LW_ERROR_NO_SUCH_DOMAIN)
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
    LW_SAFE_FREE_MEMORY(pszDnsDomainName);
    LW_SAFE_FREE_MEMORY(pszNetbiosDomainName);
    return dwError;

error:
    goto cleanup;
}

static
DWORD
DomainMgrpDiscoverTrustsForDomain(
    IN PWSTR pszParentTrusteeDomainName,
    IN PWSTR pszDomainName,
    IN PWSTR pszForestName,
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
    if (LW_ERROR_DOMAIN_IS_OFFLINE == dwError)
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
        dwError = LW_ERROR_DATA_ERROR;
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
                    dwError = LW_ERROR_INTERNAL;
                    BAIL_ON_LSA_ERROR(dwError);
                }
                pPrimaryTrust = pCurrentTrust;
            }
        }
        // If we did not find the primary trust info, become sad.
        if (!pPrimaryTrust)
        {
            dwError = LW_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
        }

        // double-check that this is the expected domain name
        dwError = LsaWc16sToMbs(pPrimaryTrust->dns_name, &pszDnsDomainName);
        BAIL_ON_LSA_ERROR(dwError);

        if (strcasecmp(pszDnsDomainName, pszDomainName))
        {
            LSA_LOG_DEBUG("Primary domain mismatch: got '%s', wanted '%s'", pszDnsDomainName, pszDomainName);
            dwError = LW_ERROR_DATA_ERROR;
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
        dwError = LwAllocateMemory(sizeof(pTrustedForestRootList[0]) * dwTrustCount,
                                    (PVOID*)&pTrustedForestRootList);
        BAIL_ON_LSA_ERROR(dwError);
    }

    for (dwTrustIndex = 0; dwTrustIndex < dwTrustCount; dwTrustIndex++)
    {
        NetrDomainTrust* pCurrentTrust = &pTrusts[dwTrustIndex];
        PWSTR pszCurrentTrustForestRootName = NULL;
        LSA_TRUST_DIRECTION dwTrustDirection = LSA_TRUST_DIRECTION_UNKNOWN;
        LSA_TRUST_MODE dwTrustMode = LSA_TRUST_MODE_UNKNOWN;

        // Ignore DOWNLEVEL trusts.
        // These are trusts with domains that are earlier than WIN2K
        if (pCurrentTrust->trust_type == NETR_TRUST_TYPE_DOWNLEVEL)
        {
            LW_SAFE_FREE_STRING(pszNetbiosName);

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
                    dwError = LW_ERROR_INTERNAL;
                    BAIL_ON_LSA_ERROR(dwError);
                }
                pPrimaryTrust = pCurrentTrust;
            }
            continue;
        }

        LW_SAFE_FREE_MEMORY(pszDnsDomainName);

        if (pCurrentTrust->dns_name)
        {
            dwError = LsaWc16sToMbs(pCurrentTrust->dns_name, &pszDnsDomainName);
            BAIL_ON_LSA_ERROR(dwError);
        }

        if (LW_IS_NULL_OR_EMPTY_STR(pszDnsDomainName))
        {
            LW_SAFE_FREE_STRING(pszNetbiosName);

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

            dwError = LwAllocateString(pszDnsDomainName,
                                        &pTrustedForestRootList[dwTrustedForestRootListCount]);
            BAIL_ON_LSA_ERROR(dwError);

            dwTrustedForestRootListCount++;
        }
    }

    if (!dwTrustedForestRootListCount)
    {
        // Return NULL rather than an empty list
        LW_SAFE_FREE_STRING_ARRAY(pTrustedForestRootList);
    }

cleanup:

    LWNET_SAFE_FREE_DC_INFO(pDcInfo);
    LW_SAFE_FREE_STRING(pszNetbiosName);
    LW_SAFE_FREE_MEMORY(pszDnsDomainName);

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
    LW_SAFE_FREE_STRING_ARRAY(pTrustedForestRootList);
    goto cleanup;
}