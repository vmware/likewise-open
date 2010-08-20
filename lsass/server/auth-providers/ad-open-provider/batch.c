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
 *        batch.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Active Directory Authentication Provider
 *
 * Authors: Wei Fu (wfu@likewisesoftware.com)
 *          Danilo Almeida (dalmeida@likewisesoftware.com)
 */

#include "adprovider.h"
#include "batch_build.h"
#include "batch_marshal.h"
#include "batch_gather.h"
#include "batch_p.h"

static
PCSTR
LsaAdBatchParseDcPart(
    IN PCSTR pszDn
    )
{
    PCSTR pszFound = NULL;
    const char DC_PART[] = "dc=";

    if (!strncasecmp(pszDn, DC_PART, sizeof(DC_PART)))
    {
        return pszDn;
    }

    pszFound = strstr(pszDn, ",dc=");
    if (pszFound)
    {
        return pszFound + 1;
    }

    pszFound = strstr(pszDn, ",DC=");
    if (pszFound)
    {
        return pszFound + 1;
    }

    pszFound = strstr(pszDn, ",Dc=");
    if (pszFound)
    {
        return pszFound + 1;
    }

    pszFound = strstr(pszDn, ",dC=");
    if (pszFound)
    {
        return pszFound + 1;
    }

    return NULL;
}

static
DWORD
LsaAdBatchCheckDomainModeCompatibility(
    IN PCSTR pszDnsDomainName,
    IN BOOLEAN bIsExternalTrust,
    IN OPTIONAL PCSTR pszDomainDN
    )
{
    DWORD dwError = 0;
    PLSA_DM_LDAP_CONNECTION pConn = NULL;
    PSTR pszCellDN = NULL;
    ADConfigurationMode adMode = UnknownMode;
    PSTR pszLocalDomainDn = NULL;
    PCSTR pszDomainDnToUse = pszDomainDN;

    // When the primary domain is default mode, need make sure the
    // trusted domain are in the same mode.  Delete those domains
    // that have inconsistent execution mode.

    if (gpADProviderData->dwDirectoryMode != DEFAULT_MODE)
    {
        goto cleanup;
    }

    if (bIsExternalTrust)
    {
        // Exclude all the external trusts in default mode to inherit the feature from 4.0
        // To be specific, external trust in default mode is not supported.
        dwError = LW_ERROR_INCOMPATIBLE_MODES_BETWEEN_TRUSTEDDOMAINS;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (!pszDomainDnToUse)
    {
        dwError = LwLdapConvertDomainToDN(pszDnsDomainName,
                                           &pszLocalDomainDn);
        BAIL_ON_LSA_ERROR(dwError);
        pszDomainDnToUse = pszLocalDomainDn;
    }

    dwError = LsaDmLdapOpenDc(pszDnsDomainName, &pConn);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateStringPrintf(&pszCellDN,
                    "CN=$LikewiseIdentityCell,%s",
                    pszDomainDnToUse);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADGetConfigurationMode(
                         pConn,
                         pszCellDN,
                         &adMode);
    BAIL_ON_LSA_ERROR(dwError);

    if (adMode != gpADProviderData->adConfigurationMode)
    {
        dwError = LW_ERROR_INCOMPATIBLE_MODES_BETWEEN_TRUSTEDDOMAINS;
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    LsaDmLdapClose(pConn);
    LW_SAFE_FREE_STRING(pszCellDN);
    LW_SAFE_FREE_STRING(pszLocalDomainDn);

    return dwError;

error:
    goto cleanup;
}

static
DWORD
LsaAdBatchGetDomainEntryType(
    IN PCSTR pszDomainName,
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN OPTIONAL PCSTR pszDomainDN,
    OUT PBOOLEAN pbSkip,
    OUT PBOOLEAN pbIsOneWayTrust
    )
{
    DWORD dwError = 0;
    DWORD dwTrustDirection = LSA_TRUST_DIRECTION_UNKNOWN;
    DWORD dwTrustMode = LSA_TRUST_MODE_UNKNOWN;
    BOOLEAN bIsExternalTrust = FALSE;
    BOOLEAN bSkip = FALSE;
    BOOLEAN bIsOneWayTrust = FALSE;

    // check trust information to determine whether we need this domain
    dwError = AD_DetermineTrustModeandDomainName(pszDomainName,
                    &dwTrustDirection,
                    &dwTrustMode,
                    NULL,
                    NULL);
    if (LW_ERROR_NO_SUCH_DOMAIN == dwError)
    {
        dwError = 0;
        bSkip = TRUE;

        goto cleanup;
    }
    BAIL_ON_LSA_ERROR(dwError);

    switch (QueryType)
    {
        case LSA_AD_BATCH_QUERY_TYPE_BY_DN:
            if (dwTrustDirection != LSA_TRUST_DIRECTION_TWO_WAY &&
                dwTrustDirection != LSA_TRUST_DIRECTION_SELF)
            {
                bSkip = TRUE;
            }

            break;

        case LSA_AD_BATCH_QUERY_TYPE_BY_SID:
        case LSA_AD_BATCH_QUERY_TYPE_BY_NT4:
            switch (gpADProviderData->dwDirectoryMode)
            {
                case DEFAULT_MODE:
                    if (dwTrustDirection != LSA_TRUST_DIRECTION_TWO_WAY &&
                        dwTrustDirection != LSA_TRUST_DIRECTION_SELF)
                    {
                        bSkip = TRUE;
                    }

                    break;

                case CELL_MODE:
                case UNPROVISIONED_MODE:
                    if (dwTrustDirection != LSA_TRUST_DIRECTION_ONE_WAY &&
                        dwTrustDirection != LSA_TRUST_DIRECTION_TWO_WAY &&
                        dwTrustDirection != LSA_TRUST_DIRECTION_SELF)
                    {
                        bSkip = TRUE;
                    }
                    else if (dwTrustDirection == LSA_TRUST_DIRECTION_ONE_WAY)
                    {
                        bIsOneWayTrust = TRUE;
                    }

                    break;
                default:
                    dwError = LW_ERROR_INVALID_PARAMETER;
                    BAIL_ON_LSA_ERROR(dwError);
            }

            break;

        default:
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
    }

    if (!bSkip)
    {
        bIsExternalTrust = (dwTrustMode == LSA_TRUST_MODE_EXTERNAL) ? TRUE : FALSE;
        dwError = LsaAdBatchCheckDomainModeCompatibility(pszDomainName, bIsExternalTrust, pszDomainDN);
        if (dwError == LW_ERROR_INCOMPATIBLE_MODES_BETWEEN_TRUSTEDDOMAINS)
        {
            dwError = 0;
            bSkip = TRUE;
            LSA_LOG_DEBUG("Mark trusted domain %s [skip] due to incompatible modes from primary domain %s",
                           pszDomainName, gpADProviderData->szDomain);
        }
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    *pbSkip = bSkip;
    *pbIsOneWayTrust = bIsOneWayTrust;

    return dwError;

error:
    goto cleanup;
}

DWORD
LsaAdBatchGetDomainFromNT4Name(
    OUT PSTR* ppszDomainName,
    IN PCSTR pszNT4Name
    )
{
    DWORD dwError = 0;
    PCSTR pszSeparator = NULL;
    size_t sLength = 0;
    PSTR pszDomainName = NULL;

    pszSeparator = strchr(pszNT4Name, LsaGetDomainSeparator());
    if (!pszSeparator)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    sLength = pszSeparator - pszNT4Name;

    dwError = LwStrndup(pszNT4Name, sLength, &pszDomainName);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    *ppszDomainName = pszDomainName;

    return dwError;

error:
    LW_SAFE_FREE_STRING(pszDomainName);
    goto cleanup;
}

static
void
LsaAdBatchFreeDomainListElements(
    IN OUT PLSA_LIST_LINKS pDomainList)
{
    if (pDomainList && pDomainList->Next && pDomainList->Prev)
    {
        while (!LsaListIsEmpty(pDomainList))
        {
            PLSA_LIST_LINKS pLinks = LsaListRemoveTail(pDomainList);
            PLSA_AD_BATCH_DOMAIN_ENTRY pEntry = LW_STRUCT_FROM_FIELD(pLinks, LSA_AD_BATCH_DOMAIN_ENTRY, DomainEntryListLinks);

            LsaAdBatchDestroyDomainEntry(&pEntry);
        }
    }
}

static
void
LsaAdBatchFreeBatchItemListElements(
    IN OUT PLSA_LIST_LINKS pBatchItemList)
{
    if (pBatchItemList && pBatchItemList->Next && pBatchItemList->Prev)
    {
        while (!LsaListIsEmpty(pBatchItemList))
        {
            PLSA_LIST_LINKS pLinks = LsaListRemoveTail(pBatchItemList);
            PLSA_AD_BATCH_ITEM pItem = LW_STRUCT_FROM_FIELD(pLinks, LSA_AD_BATCH_ITEM, BatchItemListLinks);

            LsaAdBatchDestroyBatchItem(&pItem);
        }
    }
}

static
DWORD
LsaAdBatchCreateDomainEntry(
    OUT PLSA_AD_BATCH_DOMAIN_ENTRY* ppEntry,
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN PCSTR pszMatchTerm
    )
{
    DWORD dwError = 0;
    PLSA_AD_BATCH_DOMAIN_ENTRY pEntry = NULL;
    PSTR pszDnsDomainName = NULL;
    PSTR pszNetbiosDomainName = NULL;
    PCSTR pszDcPart = NULL;
    PSTR pszDomainSid = NULL;
    BOOLEAN bSkip = FALSE;
    BOOLEAN bIsOneWayTrust = FALSE;
    PSTR pszDomainName = NULL;

    switch (QueryType)
    {
        case LSA_AD_BATCH_QUERY_TYPE_BY_DN:
            dwError = LwLdapConvertDNToDomain(pszMatchTerm,
                                               &pszDnsDomainName);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaDmWrapGetDomainName(pszDnsDomainName,
                                             NULL,
                                             &pszNetbiosDomainName);
            BAIL_ON_LSA_ERROR(dwError);

            pszDcPart = pszMatchTerm;

            break;

        case LSA_AD_BATCH_QUERY_TYPE_BY_SID:
            dwError = LsaDmEngineGetDomainNameAndSidByObjectSidWithDiscovery(
                            pszMatchTerm,
                            &pszDnsDomainName,
                            &pszNetbiosDomainName,
                            &pszDomainSid);
            BAIL_ON_LSA_ERROR(dwError);

            break;

        case LSA_AD_BATCH_QUERY_TYPE_BY_NT4:
            dwError = LsaAdBatchGetDomainFromNT4Name(&pszDomainName,
                                                     pszMatchTerm);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaDmEngineGetDomainNameWithDiscovery(
                           pszDomainName,
                           &pszDnsDomainName,
                           &pszNetbiosDomainName);
            BAIL_ON_LSA_ERROR(dwError);

            break;

        default:
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaAdBatchGetDomainEntryType(
                    pszDnsDomainName,
                    QueryType,
                    pszDcPart,
                    &bSkip,
                    &bIsOneWayTrust);
    BAIL_ON_LSA_ERROR(dwError);


    dwError = LwAllocateMemory(sizeof(*pEntry), (PVOID*)&pEntry);
    BAIL_ON_LSA_ERROR(dwError);

    LsaListInit(&pEntry->BatchItemList);

    pEntry->QueryType = QueryType;

    pEntry->pszDnsDomainName = pszDnsDomainName;
    pszDnsDomainName = NULL;

    pEntry->pszNetbiosDomainName = pszNetbiosDomainName;
    pszNetbiosDomainName = NULL;

    if (bSkip)
    {
        SetFlag(pEntry->Flags, LSA_AD_BATCH_DOMAIN_ENTRY_FLAG_SKIP);
        LSA_LOG_DEBUG("Trusted domain %s' is marked skip", pEntry->pszDnsDomainName);
    }
    if (bIsOneWayTrust)
    {
        SetFlag(pEntry->Flags, LSA_AD_BATCH_DOMAIN_ENTRY_FLAG_IS_ONE_WAY_TRUST);
    }

    switch (QueryType)
    {
        case LSA_AD_BATCH_QUERY_TYPE_BY_DN:
            LSA_ASSERT(pszDcPart);
            pEntry->QueryMatch.ByDn.pszDcPart = pszDcPart;
            pszDcPart = NULL;
            break;
        case LSA_AD_BATCH_QUERY_TYPE_BY_SID:
            LSA_ASSERT(pszDomainSid);
            pEntry->QueryMatch.BySid.pszDomainSid = pszDomainSid;
            pszDomainSid = NULL;
            pEntry->QueryMatch.BySid.sDomainSidLength = strlen(pEntry->QueryMatch.BySid.pszDomainSid);
            break;
        case LSA_AD_BATCH_QUERY_TYPE_BY_NT4:
            pEntry->QueryMatch.ByNT4.sNetbiosDomainNameLength = strlen(pEntry->pszNetbiosDomainName);
            pEntry->QueryMatch.ByNT4.sDnsDomainNameLength = strlen(pEntry->pszDnsDomainName);
            LSA_ASSERT(pEntry->QueryMatch.ByNT4.sNetbiosDomainNameLength > 0);
            LSA_ASSERT(pEntry->QueryMatch.ByNT4.sDnsDomainNameLength > 0);
            break;
        default:
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
    }

    *ppEntry = pEntry;

cleanup:
    LW_SAFE_FREE_STRING(pszDnsDomainName);
    LW_SAFE_FREE_STRING(pszNetbiosDomainName);
    LW_SAFE_FREE_STRING(pszDomainSid);
    LW_SAFE_FREE_STRING(pszDomainName);
    return dwError;

error:
    *ppEntry = NULL;
    LsaAdBatchDestroyDomainEntry(&pEntry);
    goto cleanup;
}

static
VOID
LsaAdBatchDestroyDomainEntry(
    IN OUT PLSA_AD_BATCH_DOMAIN_ENTRY* ppEntry
    )
{
    PLSA_AD_BATCH_DOMAIN_ENTRY pEntry = *ppEntry;
    if (pEntry)
    {
        LW_SAFE_FREE_STRING(pEntry->pszDnsDomainName);
        LW_SAFE_FREE_STRING(pEntry->pszNetbiosDomainName);

        switch (pEntry->QueryType)
        {
            case LSA_AD_BATCH_QUERY_TYPE_BY_SID:
                LW_SAFE_FREE_STRING(pEntry->QueryMatch.BySid.pszDomainSid);
                break;
        }

        while (!LsaListIsEmpty(&pEntry->BatchItemList))
        {
            PLSA_LIST_LINKS pLinks = LsaListRemoveTail(&pEntry->BatchItemList);
            PLSA_AD_BATCH_ITEM pItem = LW_STRUCT_FROM_FIELD(pLinks, LSA_AD_BATCH_ITEM, BatchItemListLinks);

            LsaAdBatchDestroyBatchItem(&pItem);
        }
        LwFreeMemory(pEntry);
        *ppEntry = NULL;
    }
}

static
DWORD
LsaAdBatchCreateBatchItem(
    OUT PLSA_AD_BATCH_ITEM* ppItem,
    IN PLSA_AD_BATCH_DOMAIN_ENTRY pDomainEntry,
    IN LSA_AD_BATCH_QUERY_TYPE QueryTermType,
    IN OPTIONAL PCSTR pszString,
    IN OPTIONAL PDWORD pdwId
    )
{
    DWORD dwError = 0;
    PLSA_AD_BATCH_ITEM pItem = NULL;
    PCSTR pszQueryString = pszString;


    if (!LSA_IS_XOR(!LW_IS_NULL_OR_EMPTY_STR(pszString), pdwId))
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwAllocateMemory(sizeof(*pItem), (PVOID*)&pItem);
    BAIL_ON_LSA_ERROR(dwError);

    if (LSA_AD_BATCH_QUERY_TYPE_BY_NT4 == QueryTermType)
    {
        LSA_ASSERT(pszQueryString);

        // We only want the SAM account name portion.

        pszQueryString = index(pszQueryString, LsaGetDomainSeparator());
        if (!pszQueryString)
        {
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
        }
        pszQueryString++;
        if (LW_IS_NULL_OR_EMPTY_STR(pszQueryString))
        {
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    pItem->QueryTerm.Type = QueryTermType;
    if (pszQueryString)
    {
        pItem->QueryTerm.pszString = pszQueryString;
    }
    else if (pdwId)
    {
        pItem->QueryTerm.dwId = *pdwId;
    }

cleanup:
    if (dwError)
    {
        LsaAdBatchDestroyBatchItem(&pItem);
    }
    *ppItem = pItem;

    return dwError;

error:
    // Do not handle error here, instead, do it in cleanup because of 'goto cleanup'
    goto cleanup;
}

static
VOID
LsaAdBatchDestroyBatchItem(
    IN OUT PLSA_AD_BATCH_ITEM* ppItem
    )
{
    PLSA_AD_BATCH_ITEM pItem = *ppItem;
    if (pItem)
    {
        LsaAdBatchDestroyBatchItemContents(pItem);
        LwFreeMemory(pItem);
        *ppItem = NULL;
    }
}

VOID
LsaAdBatchDestroyBatchItemContents(
    IN OUT PLSA_AD_BATCH_ITEM pItem
    )
{
    if (IsSetFlag(pItem->Flags, LSA_AD_BATCH_ITEM_FLAG_ALLOCATED_MATCH_TERM))
    {
        LW_SAFE_FREE_STRING(pItem->pszQueryMatchTerm);
    }
    LW_SAFE_FREE_STRING(pItem->pszSid);
    LW_SAFE_FREE_STRING(pItem->pszSamAccountName);
    LW_SAFE_FREE_STRING(pItem->pszDn);
    switch (pItem->ObjectType)
    {
        case LSA_AD_BATCH_OBJECT_TYPE_USER:
            LW_SAFE_FREE_STRING(pItem->UserInfo.pszAlias);
            LW_SAFE_FREE_STRING(pItem->UserInfo.pszPasswd);
            LW_SAFE_FREE_STRING(pItem->UserInfo.pszGecos);
            LW_SAFE_FREE_STRING(pItem->UserInfo.pszHomeDirectory);
            LW_SAFE_FREE_STRING(pItem->UserInfo.pszShell);
            LW_SAFE_FREE_STRING(pItem->UserInfo.pszUserPrincipalName);
            break;
        case LSA_AD_BATCH_OBJECT_TYPE_GROUP:
            LW_SAFE_FREE_STRING(pItem->GroupInfo.pszAlias);
            LW_SAFE_FREE_STRING(pItem->GroupInfo.pszPasswd);
            break;
    }

    memset(pItem, 0, sizeof(*pItem));
}

static
DWORD
LsaAdBatchGetDomainMatchTerm(
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN PCSTR pszQueryTerm,
    OUT PCSTR* ppszMatchTerm
    )
{
    DWORD dwError = 0;
    PCSTR pszMatchTerm = NULL;

    switch (QueryType)
    {
        case LSA_AD_BATCH_QUERY_TYPE_BY_DN:
            pszMatchTerm = LsaAdBatchParseDcPart(pszQueryTerm);
            break;
        case LSA_AD_BATCH_QUERY_TYPE_BY_SID:
            pszMatchTerm = pszQueryTerm;
            break;
        case LSA_AD_BATCH_QUERY_TYPE_BY_NT4:
            pszMatchTerm = pszQueryTerm;
            break;
        default:
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
    }

    *ppszMatchTerm = pszMatchTerm;

cleanup:
    return dwError;

error:
    *ppszMatchTerm = NULL;
    goto cleanup;
}

static
BOOLEAN
LsaAdBatchIsDomainSidMatch(
    IN PCSTR pszDomainSid,
    IN size_t sDomainSidLength,
    IN PCSTR pszObjectSid
    )
{
    BOOLEAN bIsMatch = FALSE;

    if (!strncasecmp(pszObjectSid,
                     pszDomainSid,
                     sDomainSidLength) &&
        (!pszObjectSid[sDomainSidLength] ||
         ('-' == pszObjectSid[sDomainSidLength])))
    {
        bIsMatch = TRUE;
    }

    return bIsMatch;
}

static
BOOLEAN
LsaAdBatchIsDomainNameMatch(
    IN PCSTR pszDomainName,
    IN size_t sDomainNameLength,
    IN PCSTR pszObjectNT4Name
    )
{
    BOOLEAN bIsMatch = FALSE;

    if (!strncasecmp(pszObjectNT4Name,
                     pszDomainName,
                     sDomainNameLength) &&
        (!pszObjectNT4Name[sDomainNameLength] ||
         (LsaGetDomainSeparator() == pszObjectNT4Name[sDomainNameLength])))
    {
        bIsMatch = TRUE;
    }

    return bIsMatch;
}

static
DWORD
LsaAdBatchMatchDomain(
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN PCSTR pszMatchTerm,
    IN PLSA_AD_BATCH_DOMAIN_ENTRY pEntry,
    OUT PBOOLEAN pbIsMatch
    )
{
    DWORD dwError = 0;
    BOOLEAN bIsMatch = FALSE;

    switch (QueryType)
    {
        case LSA_AD_BATCH_QUERY_TYPE_BY_DN:
            if (!strcasecmp(pEntry->QueryMatch.ByDn.pszDcPart, pszMatchTerm))
            {
                bIsMatch = TRUE;
            }
            break;
        case LSA_AD_BATCH_QUERY_TYPE_BY_SID:
            if (LsaAdBatchIsDomainSidMatch(pEntry->QueryMatch.BySid.pszDomainSid,
                                           pEntry->QueryMatch.BySid.sDomainSidLength,
                                           pszMatchTerm))
            {
                bIsMatch = TRUE;
            }
            break;
        case LSA_AD_BATCH_QUERY_TYPE_BY_NT4:
            if (LsaAdBatchIsDomainNameMatch(pEntry->pszNetbiosDomainName,
                                            pEntry->QueryMatch.ByNT4.sNetbiosDomainNameLength,
                                            pszMatchTerm) ||
                LsaAdBatchIsDomainNameMatch(pEntry->pszDnsDomainName,
                                            pEntry->QueryMatch.ByNT4.sDnsDomainNameLength,
                                            pszMatchTerm))
            {
                bIsMatch = TRUE;
            }
        default:
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
    }

    *pbIsMatch = bIsMatch;

cleanup:
    return dwError;

error:
    *pbIsMatch = FALSE;
    goto cleanup;
}

DWORD
LsaAdBatchQueryCellConfigurationMode(
    IN PCSTR pszDnsDomainName,
    IN PCSTR pszCellDN,
    OUT ADConfigurationMode* pAdMode
    )
{
    DWORD dwError = 0;
    PLSA_DM_LDAP_CONNECTION pConn = NULL;
    ADConfigurationMode adMode = UnknownMode;

    dwError = LsaDmLdapOpenDc(pszDnsDomainName, &pConn);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADGetConfigurationMode(
                         pConn,
                         pszCellDN,
                         &adMode);
    BAIL_ON_LSA_ERROR(dwError);

    *pAdMode = adMode;

cleanup:
    LsaDmLdapClose(pConn);

    return dwError;

error:
    *pAdMode = UnknownMode;

    goto cleanup;
}

BOOLEAN
LsaAdBatchIsDefaultSchemaMode(
    VOID
    )
{
    return ((DEFAULT_MODE == gpADProviderData->dwDirectoryMode) &&
            (SchemaMode == gpADProviderData->adConfigurationMode));
}

BOOLEAN
LsaAdBatchIsUnprovisionedMode(
    VOID
    )
{
    return (UNPROVISIONED_MODE == gpADProviderData->dwDirectoryMode);
}

DWORD
LsaAdBatchConvertQTListToBIList(
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN DWORD dwQueryItemsCount,
    IN OPTIONAL PSTR* ppszQueryList,
    IN OPTIONAL PDWORD pdwId,
    OUT PLSA_LIST_LINKS pBatchItemList,
    OUT PDWORD pdwTotalBatchItemCount
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    DWORD dwTotalBatchItemCount = 0;

    if (!LSA_IS_XOR(ppszQueryList, pdwId))
    {
        LSA_ASSERT(FALSE);
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    LsaListInit(pBatchItemList);

    for (i = 0; i < dwQueryItemsCount; i++)
    {
        PLSA_AD_BATCH_ITEM pBatchItem = NULL;

        // Set pBatchItem->pDomainEntry to NULL,
        // Showing we have no knowledge of which domain this BatchItem belongs to at this point
        if (ppszQueryList)
        {
            dwError = LsaAdBatchCreateBatchItem(
                            &pBatchItem,
                            NULL,
                            QueryType,
                            ppszQueryList[i],
                            NULL);
            BAIL_ON_LSA_ERROR(dwError);
        }
        else if (pdwId)
        {
            dwError = LsaAdBatchCreateBatchItem(
                            &pBatchItem,
                            NULL,
                            QueryType,
                            NULL,
                            &pdwId[i]);
            BAIL_ON_LSA_ERROR(dwError);
        }

        if (pBatchItem)
        {
            LsaListInsertTail(pBatchItemList, &pBatchItem->BatchItemListLinks);
            dwTotalBatchItemCount++;
        }
    }

    *pdwTotalBatchItemCount = dwTotalBatchItemCount;

cleanup:
    return dwError;

error:
    goto cleanup;
}

// Note: Before calling the following function, we should
// (1) have pszSid stored in pBatchItem; Or
// (2) pszSid being NULL (meaning we couldn't resolve such object into Sid from Pseudo)
static
DWORD
LsaAdBatchSplitBIListToBIListPerDomain(
    IN OUT PLSA_LIST_LINKS pBatchItemList,
    OUT PLSA_LIST_LINKS pDomainList
    )
{
    DWORD dwError = 0;
    PLSA_LIST_LINKS pDomainLinks = NULL;
    PLSA_AD_BATCH_ITEM pBatchItem = NULL;

    if (!pBatchItemList)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    LsaListInit(pDomainList);

    while (!LsaListIsEmpty(pBatchItemList))
    {
        PCSTR pszMatchTerm = NULL;
        PLSA_LIST_LINKS pBILinks = NULL;
        PLSA_AD_BATCH_DOMAIN_ENTRY pFoundEntry = NULL;

        pBILinks = LsaListRemoveHead(pBatchItemList);
        pBatchItem = LW_STRUCT_FROM_FIELD(pBILinks, LSA_AD_BATCH_ITEM, BatchItemListLinks);

        if (LW_IS_NULL_OR_EMPTY_STR(pBatchItem->pszSid))
        {
            LsaAdBatchDestroyBatchItem(&pBatchItem);
            continue;
        }

        dwError = LsaAdBatchGetDomainMatchTerm(
                        LSA_AD_BATCH_QUERY_TYPE_BY_SID,
                        pBatchItem->pszSid,
                        &pszMatchTerm);
        BAIL_ON_LSA_ERROR(dwError);

        for (pDomainLinks = pDomainList->Next;
             pDomainLinks != pDomainList;
             pDomainLinks = pDomainLinks->Next)
        {
            PLSA_AD_BATCH_DOMAIN_ENTRY pEntry = LW_STRUCT_FROM_FIELD(pDomainLinks, LSA_AD_BATCH_DOMAIN_ENTRY, DomainEntryListLinks);
            BOOLEAN bIsMatch = FALSE;

            dwError = LsaAdBatchMatchDomain(
                            LSA_AD_BATCH_QUERY_TYPE_BY_SID,
                            pszMatchTerm,
                            pEntry,
                            &bIsMatch);
            BAIL_ON_LSA_ERROR(dwError);

            if (bIsMatch)
            {
                pFoundEntry = pEntry;
                break;
            }
        }

        if (!pFoundEntry)
        {
            dwError = LsaAdBatchCreateDomainEntry(
                            &pFoundEntry,
                            LSA_AD_BATCH_QUERY_TYPE_BY_SID,
                            pszMatchTerm);
            if (LW_ERROR_NO_SUCH_DOMAIN == dwError)
            {
                LSA_LOG_DEBUG("Domain not found for query item - '%s'", pBatchItem->pszSid);
                dwError = 0;
                continue;
            }
            BAIL_ON_LSA_ERROR(dwError);

            LsaListInsertTail(pDomainList, &pFoundEntry->DomainEntryListLinks);
        }

        if (!IsSetFlag(pFoundEntry->Flags, LSA_AD_BATCH_DOMAIN_ENTRY_FLAG_SKIP))
        {
            LsaListInsertTail(&pFoundEntry->BatchItemList, &pBatchItem->BatchItemListLinks);
            pBatchItem = NULL;
            pFoundEntry->dwBatchItemCount++;
        }
        // Destroy pBatchItem
        else
        {
            LsaAdBatchDestroyBatchItem(&pBatchItem);
        }
        pFoundEntry = NULL;
    }

cleanup:
    LsaAdBatchDestroyBatchItem(&pBatchItem);
    return dwError;

error:
    goto cleanup;
}

static
DWORD
LsaAdBatchSplitQTListToBIListPerDomain(
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN DWORD dwQueryItemsCount,
    IN PSTR* ppszQueryList,
    OUT PLSA_LIST_LINKS pDomainList
    )
{
    DWORD dwError = 0;
    PLSA_LIST_LINKS pLinks = NULL;
    DWORD i = 0;

    LsaListInit(pDomainList);

    for (i = 0; i < dwQueryItemsCount; i++)
    {
        PCSTR pszMatchTerm = NULL;
        PLSA_AD_BATCH_DOMAIN_ENTRY pFoundEntry = NULL;

        dwError = LsaAdBatchGetDomainMatchTerm(
                        QueryType,
                        ppszQueryList[i],
                        &pszMatchTerm);
        BAIL_ON_LSA_ERROR(dwError);

        for (pLinks = pDomainList->Next;
             pLinks != pDomainList;
             pLinks = pLinks->Next)
        {
            PLSA_AD_BATCH_DOMAIN_ENTRY pEntry = LW_STRUCT_FROM_FIELD(pLinks, LSA_AD_BATCH_DOMAIN_ENTRY, DomainEntryListLinks);
            BOOLEAN bIsMatch = FALSE;

            dwError = LsaAdBatchMatchDomain(
                            QueryType,
                            pszMatchTerm,
                            pEntry,
                            &bIsMatch);
            BAIL_ON_LSA_ERROR(dwError);

            if (bIsMatch)
            {
                pFoundEntry = pEntry;
                break;
            }
        }

        if (!pFoundEntry)
        {
            dwError = LsaAdBatchCreateDomainEntry(
                            &pFoundEntry,
                            QueryType,
                            pszMatchTerm);
            if (LW_ERROR_NO_SUCH_DOMAIN == dwError)
            {
                LSA_LOG_DEBUG("Domain not found for query item - '%s'", ppszQueryList[i]);
                dwError = 0;
                continue;
            }
            BAIL_ON_LSA_ERROR(dwError);

            LsaListInsertTail(pDomainList, &pFoundEntry->DomainEntryListLinks);
        }

        if (!IsSetFlag(pFoundEntry->Flags, LSA_AD_BATCH_DOMAIN_ENTRY_FLAG_SKIP))
        {
            PLSA_AD_BATCH_ITEM pBatchItem = NULL;

            dwError = LsaAdBatchCreateBatchItem(
                            &pBatchItem,
                            pFoundEntry,
                            QueryType,
                            ppszQueryList[i],
                            NULL);
            BAIL_ON_LSA_ERROR(dwError);

            if (pBatchItem)
            {
                LsaListInsertTail(&pFoundEntry->BatchItemList, &pBatchItem->BatchItemListLinks);
            }

            pFoundEntry->dwBatchItemCount++;
        }

        pFoundEntry = NULL;
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

static
DWORD
LsaAdBatchResolveObjectsForDomainList(
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN PLSA_LIST_LINKS pDomainList,
    IN BOOLEAN bResolvePseudoObjects,
    OUT PDWORD pdwObjectsCount,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    )
{
    DWORD dwError = 0;
    // Do not free pLinks
    PLSA_LIST_LINKS pLinks = NULL;
    DWORD dwObjectsCount = 0;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    DWORD dwCurrentIndex = 0;

    for (pLinks = pDomainList->Next;
         pLinks != pDomainList;
         pLinks = pLinks->Next)
    {
        PLSA_AD_BATCH_DOMAIN_ENTRY pEntry = LW_STRUCT_FROM_FIELD(pLinks, LSA_AD_BATCH_DOMAIN_ENTRY, DomainEntryListLinks);

        if (IsSetFlag(pEntry->Flags, LSA_AD_BATCH_DOMAIN_ENTRY_FLAG_SKIP))
        {
            continue;
        }

        dwError = LsaAdBatchFindObjectsForDomainEntry(
                      QueryType,
                      bResolvePseudoObjects,
                      pEntry);
        BAIL_ON_LSA_ERROR(dwError);

        dwObjectsCount += pEntry->dwBatchItemCount;
    }

    dwError = LwAllocateMemory(
                dwObjectsCount * sizeof(*ppObjects),
                (PVOID*)&ppObjects);
    BAIL_ON_LSA_ERROR(dwError);

    // Combine results
    dwCurrentIndex = 0;
    for (pLinks = pDomainList->Next;
         pLinks != pDomainList;
         pLinks = pLinks->Next)
    {
        PLSA_AD_BATCH_DOMAIN_ENTRY pEntry = LW_STRUCT_FROM_FIELD(pLinks, LSA_AD_BATCH_DOMAIN_ENTRY, DomainEntryListLinks);
        DWORD dwDomainObjectsCount = 0;

        if (IsSetFlag(pEntry->Flags, LSA_AD_BATCH_DOMAIN_ENTRY_FLAG_SKIP))
        {
            continue;
        }

        dwError = LsaAdBatchMarshalList(
                        pEntry->pszDnsDomainName,
                        pEntry->pszNetbiosDomainName,
                        &pEntry->BatchItemList,
                        dwObjectsCount - dwCurrentIndex,
                        &ppObjects[dwCurrentIndex],
                        &dwDomainObjectsCount);
        BAIL_ON_LSA_ERROR(dwError);

        dwCurrentIndex += dwDomainObjectsCount;
    }

    LSA_ASSERT(dwCurrentIndex <= dwObjectsCount);

    // Compress the output
    if (dwCurrentIndex < dwObjectsCount)
    {
        PLSA_SECURITY_OBJECT* ppTempObjects = NULL;

        dwError = LwAllocateMemory(
                    dwCurrentIndex * sizeof(*ppTempObjects),
                    (PVOID*)&ppTempObjects);
        BAIL_ON_LSA_ERROR(dwError);

        memcpy(ppTempObjects, ppObjects, sizeof(*ppObjects) * dwCurrentIndex);
        LwFreeMemory(ppObjects);
        ppObjects = ppTempObjects;
        dwObjectsCount = dwCurrentIndex;
    }

    *pdwObjectsCount = dwObjectsCount;
    *pppObjects = ppObjects;

cleanup:
    return dwError;

error:
    *pdwObjectsCount = 0;
    *pppObjects = NULL;

    ADCacheSafeFreeObjectList(dwObjectsCount, &ppObjects);
    goto cleanup;
}

static
DWORD
LsaAdBatchFindObjectsRealBeforePseudo(
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN DWORD dwQueryItemsCount,
    IN PSTR* ppszQueryList,
    OUT PDWORD pdwObjectsCount,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    )
{
    DWORD dwError = 0;
    DWORD dwObjectsCount = 0;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    LSA_LIST_LINKS DomainList = {0};

    dwError = LsaAdBatchSplitQTListToBIListPerDomain(
                        QueryType,
                        dwQueryItemsCount,
                        ppszQueryList,
                        &DomainList);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAdBatchResolveObjectsForDomainList(
                        QueryType,
                        &DomainList,
                        TRUE,
                        &dwObjectsCount,
                        &ppObjects);
    BAIL_ON_LSA_ERROR(dwError);

    *pdwObjectsCount = dwObjectsCount;
    *pppObjects = ppObjects;

cleanup:
    LsaAdBatchFreeDomainListElements(&DomainList);

    return dwError;

error:
    *pdwObjectsCount = 0;
    *pppObjects = NULL;

    ADCacheSafeFreeObjectList(dwObjectsCount, &ppObjects);
    goto cleanup;
}

static
DWORD
LsaAdBatchFindObjectsPseudoBeforeReal(
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN DWORD dwQueryItemsCount,
    IN OPTIONAL PSTR* ppszQueryList,
    IN OPTIONAL PDWORD pdwId,
    OUT PDWORD pdwObjectsCount,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    )
{
    DWORD dwError = 0;
    DWORD dwObjectsCount = 0;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    LSA_LIST_LINKS BatchItemList = {0};
    LSA_LIST_LINKS DomainList = {0};
    DWORD dwTotalBatchItemCount = 0;
    BOOLEAN bResolvedPseudo = FALSE;

    if (!LSA_IS_XOR(ppszQueryList, pdwId))
    {
        LSA_ASSERT(FALSE);
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaAdBatchConvertQTListToBIList(
                    QueryType,
                    dwQueryItemsCount,
                    ppszQueryList,
                    pdwId,
                    &BatchItemList,
                    &dwTotalBatchItemCount);
    BAIL_ON_LSA_ERROR(dwError);

    // We have no knowledge of domain information at this point
    dwError = LsaAdBatchResolvePseudoObjects(
                    QueryType,
                    NULL,
                    dwTotalBatchItemCount,
                    &BatchItemList,
                    &bResolvedPseudo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAdBatchSplitBIListToBIListPerDomain(
                       &BatchItemList,
                       &DomainList);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAdBatchResolveObjectsForDomainList(
                        LSA_AD_BATCH_QUERY_TYPE_BY_SID,
                        &DomainList,
                        bResolvedPseudo ? FALSE : TRUE,
                        &dwObjectsCount,
                        &ppObjects);
    BAIL_ON_LSA_ERROR(dwError);

    *pdwObjectsCount = dwObjectsCount;
    *pppObjects = ppObjects;

cleanup:
    LsaAdBatchFreeDomainListElements(&DomainList);
    LsaAdBatchFreeBatchItemListElements(&BatchItemList);

    return dwError;

error:
    *pdwObjectsCount = 0;
    *pppObjects = NULL;

    ADCacheSafeFreeObjectList(dwObjectsCount, &ppObjects);
    goto cleanup;
}

DWORD
LsaAdBatchFindSingleObject(
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN OPTIONAL PCSTR pszQueryTerm,
    IN OPTIONAL PDWORD pdwId,
    OUT PLSA_SECURITY_OBJECT* ppObject
    )
{
    DWORD dwError = 0;
    DWORD dwCount = 0;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    PLSA_SECURITY_OBJECT pObject = NULL;

    if (!LSA_IS_XOR(!LW_IS_NULL_OR_EMPTY_STR(pszQueryTerm), pdwId))
    {
        LSA_ASSERT(FALSE);
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (!LW_IS_NULL_OR_EMPTY_STR(pszQueryTerm))
    {
        dwError = LsaAdBatchFindObjects(
                        QueryType,
                        1,
                        (PSTR*)&pszQueryTerm,
                        NULL,
                        &dwCount,
                        &ppObjects);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else if (pdwId)
    {
        dwError = LsaAdBatchFindObjects(
                        QueryType,
                        1,
                        NULL,
                        pdwId,
                        &dwCount,
                        &ppObjects);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (dwCount < 1 || !ppObjects[0])
    {
        dwError = LW_ERROR_NO_SUCH_OBJECT;
        BAIL_ON_LSA_ERROR(dwError);
    }
    else if (dwCount > 1)
    {
        LSA_ASSERT(FALSE);
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pObject = ppObjects[0];
    ppObjects[0] = NULL;

cleanup:
    ADCacheSafeFreeObjectList(dwCount, &ppObjects);

    *ppObject = pObject;

    return dwError;

error:
    ADCacheSafeFreeObject(&pObject);
    goto cleanup;
}

static
DWORD
LsaAdBatchFindObjectsInternal(
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN DWORD dwQueryItemsCount,
    IN OPTIONAL PSTR* ppszQueryList,
    IN OPTIONAL PDWORD pdwId,
    OUT PDWORD pdwObjectsCount,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    )
{
    DWORD dwError = 0;

    if (!LSA_IS_XOR(ppszQueryList, pdwId))
    {
        LSA_ASSERT(FALSE);
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    switch (QueryType)
    {
        case LSA_AD_BATCH_QUERY_TYPE_BY_DN:
        case LSA_AD_BATCH_QUERY_TYPE_BY_SID:
        case LSA_AD_BATCH_QUERY_TYPE_BY_NT4:
        case LSA_AD_BATCH_QUERY_TYPE_BY_USER_ALIAS:
        case LSA_AD_BATCH_QUERY_TYPE_BY_GROUP_ALIAS:
            if (pdwId)
            {
                LSA_ASSERT(FALSE);
                dwError = LW_ERROR_INVALID_PARAMETER;
                BAIL_ON_LSA_ERROR(dwError);
            }
            break;
        case LSA_AD_BATCH_QUERY_TYPE_BY_UID:
        case LSA_AD_BATCH_QUERY_TYPE_BY_GID:
            if (ppszQueryList)
            {
                LSA_ASSERT(FALSE);
                dwError = LW_ERROR_INVALID_PARAMETER;
                BAIL_ON_LSA_ERROR(dwError);
            }
            break;
        default:
            LSA_ASSERT(FALSE);
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
    }

    switch (QueryType)
    {
        case LSA_AD_BATCH_QUERY_TYPE_BY_DN:
        case LSA_AD_BATCH_QUERY_TYPE_BY_SID:
        case LSA_AD_BATCH_QUERY_TYPE_BY_NT4:
            dwError = LsaAdBatchFindObjectsRealBeforePseudo(
                            QueryType,
                            dwQueryItemsCount,
                            ppszQueryList,
                            pdwObjectsCount,
                            pppObjects);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        case LSA_AD_BATCH_QUERY_TYPE_BY_USER_ALIAS:
        case LSA_AD_BATCH_QUERY_TYPE_BY_GROUP_ALIAS:
        case LSA_AD_BATCH_QUERY_TYPE_BY_UID:
        case LSA_AD_BATCH_QUERY_TYPE_BY_GID:
            dwError = LsaAdBatchFindObjectsPseudoBeforeReal(
                            QueryType,
                            dwQueryItemsCount,
                            ppszQueryList,
                            pdwId,
                            pdwObjectsCount,
                            pppObjects);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            LSA_ASSERT(FALSE);
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

// When we come in with uid/gid in unprovision mode, we might get either type (user/group) back
// Filter out the wrong typed objects
static
DWORD
LsaAdBatchFilterMisTypeObjects(
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN DWORD dwObjectsCount,
    IN OUT PLSA_SECURITY_OBJECT** pppObjects,
    OUT PDWORD pdwRemainingObjectsCount
    )
{
    DWORD dwError = 0;
    PLSA_SECURITY_OBJECT* ppObjects = *pppObjects;
    DWORD dwRemainingObjectsCount = 0;
    PLSA_SECURITY_OBJECT* ppRemainingObjects = NULL;
    DWORD i = 0;


    if (LSA_AD_BATCH_QUERY_TYPE_BY_UID != QueryType &&
        LSA_AD_BATCH_QUERY_TYPE_BY_GID != QueryType)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwAllocateMemory(
                dwObjectsCount * sizeof(*ppRemainingObjects),
                (PVOID*)&ppRemainingObjects);
    BAIL_ON_LSA_ERROR(dwError);

    for (i = 0; i < dwObjectsCount; i++)
    {
        switch (QueryType)
        {
            case LSA_AD_BATCH_QUERY_TYPE_BY_UID:
                if (LSA_OBJECT_TYPE_USER == ppObjects[i]->type)
                {
                    ppRemainingObjects[dwRemainingObjectsCount++] = ppObjects[i];
                    ppObjects[i] = NULL;
                }
                break;

            case LSA_AD_BATCH_QUERY_TYPE_BY_GID:
                if (LSA_OBJECT_TYPE_GROUP == ppObjects[i]->type)
                {
                    ppRemainingObjects[dwRemainingObjectsCount++] = ppObjects[i];
                    ppObjects[i] = NULL;
                }
                break;
        }
    }

    *pdwRemainingObjectsCount = dwRemainingObjectsCount;
    *pppObjects = ppRemainingObjects;

cleanup:
    ADCacheSafeFreeObjectList(dwObjectsCount, &ppObjects);

    return dwError;

error:
    ADCacheSafeFreeObjectList(dwRemainingObjectsCount, &ppRemainingObjects);
    *pdwRemainingObjectsCount = 0;
    *ppRemainingObjects = NULL;

    goto cleanup;
}

DWORD
LsaAdBatchFindObjects(
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN DWORD dwQueryItemsCount,
    IN OPTIONAL PSTR* ppszQueryList,
    IN OPTIONAL PDWORD pdwId,
    OUT PDWORD pdwObjectsCount,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    )
{
    DWORD dwError = 0;
    DWORD dwObjectsCount = 0;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;

    dwError = LsaAdBatchFindObjectsInternal(
                   QueryType,
                   dwQueryItemsCount,
                   ppszQueryList,
                   pdwId,
                   &dwObjectsCount,
                   &ppObjects);
    BAIL_ON_LSA_ERROR(dwError);

    if (LsaAdBatchIsUnprovisionedMode() &&
        (LSA_AD_BATCH_QUERY_TYPE_BY_UID == QueryType ||
         LSA_AD_BATCH_QUERY_TYPE_BY_GID == QueryType))
    {
        dwError = LsaAdBatchFilterMisTypeObjects(
                            QueryType,
                            dwObjectsCount,
                            &ppObjects,
                            &dwObjectsCount);
        BAIL_ON_LSA_ERROR(dwError);
    }

    *pdwObjectsCount = dwObjectsCount;
    *pppObjects = ppObjects;

cleanup:
    return dwError;

error:
    ADCacheSafeFreeObjectList(dwObjectsCount, &ppObjects);
    *pdwObjectsCount = 0;
    *pppObjects = NULL;

    goto cleanup;
}

static
DWORD
LsaAdBatchFindObjectsForDomainEntry(
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN BOOLEAN bResolvePseudoObjects,
    IN OUT PLSA_AD_BATCH_DOMAIN_ENTRY pEntry
    )
{
    return LsaAdBatchFindObjectsForDomain(
                QueryType,
                pEntry->pszDnsDomainName,
                pEntry->pszNetbiosDomainName,
                IsSetFlag(pEntry->Flags, LSA_AD_BATCH_DOMAIN_ENTRY_FLAG_IS_ONE_WAY_TRUST),
                bResolvePseudoObjects,
                pEntry->dwBatchItemCount,
                &pEntry->BatchItemList);
}

static
DWORD
LsaAdBatchFindObjectsForDomain(
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN PCSTR pszDnsDomainName,
    IN PCSTR pszNetbiosDomainName,
    IN BOOLEAN bIsOneWayTrust,
    IN BOOLEAN bResolvePseudoObjects,
    IN DWORD dwCount,
    IN OUT PLSA_LIST_LINKS pBatchItemList
    )
{
    DWORD dwError = 0;
    // Do not delete
    PLSA_LIST_LINKS pLinks = NULL;

    if (bIsOneWayTrust && LSA_AD_BATCH_QUERY_TYPE_BY_DN == QueryType)
    {
        // This should never happen, this domain should already be skipped.
        LSA_ASSERT(FALSE);
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (bIsOneWayTrust &&
        ((LSA_AD_BATCH_QUERY_TYPE_BY_SID == QueryType) ||
         (LSA_AD_BATCH_QUERY_TYPE_BY_NT4 == QueryType)))
    {
        dwError = LsaAdBatchResolveRpcObjects(
                        QueryType,
                        pszDnsDomainName,
                        pszNetbiosDomainName,
                        dwCount,
                        pBatchItemList);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        dwError = LsaAdBatchResolveRealObjects(
                        QueryType,
                        pszDnsDomainName,
                        dwCount,
                        pBatchItemList);
        BAIL_ON_LSA_ERROR(dwError);
    }

    // If Default schema, real objects already have the pseudo information
    // If Unprovisioned mode, no need to get PseudoMessages.
    if (!LsaAdBatchIsDefaultSchemaMode() &&
        !LsaAdBatchIsUnprovisionedMode() &&
        bResolvePseudoObjects)
    {
        dwError = LsaAdBatchResolvePseudoObjects(
                        LSA_AD_BATCH_QUERY_TYPE_BY_SID,
                        pszDnsDomainName,
                        dwCount,
                        pBatchItemList,
                        NULL);
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    return dwError;

error:
    for (pLinks = pBatchItemList->Next;
         pLinks != pBatchItemList;
         pLinks = pLinks->Next)
    {
        PLSA_AD_BATCH_ITEM pBatchItem = LW_STRUCT_FROM_FIELD(pLinks, LSA_AD_BATCH_ITEM, BatchItemListLinks);

        if (pBatchItem->Flags & LSA_AD_BATCH_ITEM_FLAG_ERROR)
        {
            LSA_LOG_ERROR(
                    "An error occurred while looking up information for user/group (sid = '%s', name = '%s\\%s')",
                    LSA_SAFE_LOG_STRING(pBatchItem->pszSid),
                    LSA_SAFE_LOG_STRING(pszNetbiosDomainName),
                    LSA_SAFE_LOG_STRING(pBatchItem->pszSamAccountName));
        }
    }
    goto cleanup;
}

static
DWORD
LsaAdBatchGetMaxQuerySize(
    VOID
    )
{
    return LSA_AD_BATCH_MAX_QUERY_SIZE;
}

static
DWORD
LsaAdBatchGetMaxQueryCount(
    VOID
    )
{
    return LSA_AD_BATCH_MAX_QUERY_COUNT;
}

static
DWORD
LsaAdBatchResolveRpcObjects(
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN PCSTR pszDnsDomainName,
    IN PCSTR pszNetbiosDomainName,
    IN DWORD dwTotalItemCount,
    // List of PLSA_AD_BATCH_ITEM
    IN OUT PLSA_LIST_LINKS pBatchItemList
    )
{
    DWORD dwError = 0;
    DWORD dwMaxQueryCount = LsaAdBatchGetMaxQueryCount();
    PSTR* ppszQueryList = NULL;
    PLSA_TRANSLATED_NAME_OR_SID* ppTranslatedNames = NULL;
    DWORD dwQueryCount = 0;
    DWORD dwCount = 0;
    DWORD i = 0;
    PLSA_LIST_LINKS pLinks = NULL;
    PLSA_LIST_LINKS pNextLinks = NULL;

    for (pLinks = pBatchItemList->Next;
         pLinks != pBatchItemList;
         pLinks = pNextLinks)
    {
        pNextLinks = NULL;

        LwFreeStringArray(ppszQueryList, dwQueryCount);
        ppszQueryList = NULL;

        if (ppTranslatedNames)
        {
            // Note that there are dwQueryCount elements, not dwCount.
            // If dwCount != dwQueryCount, some elements are NULL.
            LsaFreeTranslatedNameList(ppTranslatedNames, dwQueryCount);
        }
        ppTranslatedNames = NULL;

        dwError = LsaAdBatchBuildQueryForRpc(
                        pszNetbiosDomainName,
                        QueryType,
                        pLinks,
                        pBatchItemList,
                        &pNextLinks,
                        dwMaxQueryCount,
                        &dwQueryCount,
                        &ppszQueryList);
        BAIL_ON_LSA_ERROR(dwError);

        switch (QueryType)
        {
            case LSA_AD_BATCH_QUERY_TYPE_BY_SID:
                dwError = LsaDmWrapNetLookupNamesByObjectSids(
                               gpADProviderData->szDomain,
                               dwQueryCount,
                               ppszQueryList,
                               &ppTranslatedNames,
                               &dwCount);
                BAIL_ON_LSA_ERROR(dwError);
                break;
            case LSA_AD_BATCH_QUERY_TYPE_BY_NT4:
                dwError = LsaDmWrapNetLookupObjectSidsByNames(
                               gpADProviderData->szDomain,
                               dwQueryCount,
                               ppszQueryList,
                               &ppTranslatedNames,
                               &dwCount);
                BAIL_ON_LSA_ERROR(dwError);
                break;
            default:
                LSA_ASSERT(FALSE);
                dwError = LW_ERROR_INTERNAL;
                BAIL_ON_LSA_ERROR(dwError);
        }

        if (dwCount > dwQueryCount)
        {
            LSA_LOG_ERROR("Too many results returned (got %u, expected %u)",
                          dwCount, dwQueryCount);
            dwError = LW_ERROR_RPC_ERROR;
            BAIL_ON_LSA_ERROR(dwError);
        }
        else if (dwCount == 0)
        {
            continue;
        }

        for (i = 0; i < dwQueryCount; i++)
        {
            if (!ppTranslatedNames[i])
            {
                continue;
            }

            XXX; // speed up by using a parallel list for direct a lookup.
            dwError = LsaAdBatchProcessRpcObject(
                            QueryType,
                            pLinks,
                            pNextLinks,
                            ppszQueryList[i],
                            ppTranslatedNames[i]);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

cleanup:
    LwFreeStringArray(ppszQueryList, dwQueryCount);
    if (ppTranslatedNames)
    {
        // Note that there are dwQueryCount elements, not dwCount.
        // If dwCount != dwQueryCount, some elements are NULL.
        LsaFreeTranslatedNameList(ppTranslatedNames, dwQueryCount);
    }
    return dwError;

error:
    goto cleanup;
}

static
DWORD
LsaAdBatchResolveRealObjects(
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN PCSTR pszDnsDomainName,
    IN DWORD dwTotalItemCount,
    // List of PLSA_AD_BATCH_ITEM
    IN OUT PLSA_LIST_LINKS pBatchItemList
    )
{
    DWORD dwError = 0;
    HANDLE hDirectory = 0;
    PLSA_DM_LDAP_CONNECTION pConn = NULL;
    LDAP* pLd = NULL;
    PSTR pszScopeDn = NULL;
    PSTR pszQuery = 0;
    PSTR szAttributeList[] =
    {
        // AD attributes:
        // - common:
        AD_LDAP_OBJECTCLASS_TAG,
        AD_LDAP_OBJECTSID_TAG,
        AD_LDAP_SAM_NAME_TAG,
        AD_LDAP_DN_TAG,
        // - user-specific:
        AD_LDAP_PRIMEGID_TAG,
        AD_LDAP_UPN_TAG,
        AD_LDAP_USER_CTRL_TAG,
        AD_LDAP_ACCOUT_EXP_TAG,
        AD_LDAP_PWD_LASTSET_TAG,
        // schema mode:
        // - (group alias) or (user gecos in unprovisioned mode):
        AD_LDAP_DISPLAY_NAME_TAG,
        // - unix properties (alias is just user alias):
        AD_LDAP_ALIAS_TAG,
        AD_LDAP_UID_TAG,
        AD_LDAP_GID_TAG,
        AD_LDAP_PASSWD_TAG,
        AD_LDAP_GECOS_TAG,
        AD_LDAP_HOMEDIR_TAG,
        AD_LDAP_SHELL_TAG,
        NULL
    };
    LDAPMessage* pMessage = NULL;
    PLSA_LIST_LINKS pLinks = NULL;
    PLSA_LIST_LINKS pNextLinks = NULL;
    DWORD dwMaxQuerySize = LsaAdBatchGetMaxQuerySize();
    DWORD dwMaxQueryCount = LsaAdBatchGetMaxQueryCount();

    dwError = LwLdapConvertDomainToDN(
                       pszDnsDomainName,
                       &pszScopeDn);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaDmLdapOpenDc(pszDnsDomainName, &pConn);
    BAIL_ON_LSA_ERROR(dwError);

    for (pLinks = pBatchItemList->Next;
         pLinks != pBatchItemList;
         pLinks = pNextLinks)
    {
        DWORD dwQueryCount = 0;
        DWORD dwCount = 0;
        LDAPMessage* pCurrentMessage = NULL;

        pNextLinks = NULL;

        LW_SAFE_FREE_STRING(pszQuery);
        if (pMessage)
        {
            ldap_msgfree(pMessage);
            pMessage = NULL;
        }

        dwError = LsaAdBatchBuildQueryForReal(
                        QueryType,
                        pLinks,
                        pBatchItemList,
                        &pNextLinks,
                        dwMaxQuerySize,
                        dwMaxQueryCount,
                        &dwQueryCount,
                        &pszQuery);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaDmLdapDirectorySearch(
                        pConn,
                        pszScopeDn,
                        LDAP_SCOPE_SUBTREE,
                        pszQuery,
                        szAttributeList,
                        &hDirectory,
                        &pMessage);
        BAIL_ON_LSA_ERROR(dwError);

        pLd = LwLdapGetSession(hDirectory);

        dwCount = ldap_count_entries(pLd, pMessage);
        if (dwCount > dwQueryCount)
        {
            LSA_LOG_ERROR("Too many results returned (got %u, expected %u)",
                          dwCount, dwQueryCount);
            dwError = LW_ERROR_LDAP_ERROR;
            BAIL_ON_LSA_ERROR(dwError);
        }
        else if (dwCount == 0)
        {
            continue;
        }

        pCurrentMessage = ldap_first_entry(pLd, pMessage);
        while (pCurrentMessage)
        {
            dwError = LsaAdBatchProcessRealObject(
                            QueryType,
                            pLinks,
                            pNextLinks,
                            hDirectory,
                            pCurrentMessage);
            BAIL_ON_LSA_ERROR(dwError);

            pCurrentMessage = ldap_next_entry(pLd, pCurrentMessage);
        }
    }

cleanup:
    LsaDmLdapClose(pConn);
    LW_SAFE_FREE_STRING(pszScopeDn);
    LW_SAFE_FREE_STRING(pszQuery);
    if (pMessage)
    {
        ldap_msgfree(pMessage);
    }
    return dwError;

error:
    goto cleanup;
}

static
DWORD
LsaAdBatchBuildQueryScopeForPseudo(
    IN BOOLEAN bIsSchemaMode,
    IN LSA_PROVISIONING_MODE Mode,
    IN OPTIONAL PCSTR pszDnsDomainName,
    IN OPTIONAL PCSTR pszCellDn,
    OUT PSTR* ppszScopeDn
    )
{
    DWORD dwError = 0;
    PSTR pszDcPart = NULL;
    PSTR pszScopeDn = NULL;

    switch (Mode)
    {
        case LSA_PROVISIONING_MODE_DEFAULT_CELL:
            if (bIsSchemaMode)
            {
                // ASSERT
                LSA_LOG_ERROR("Schema mode default cell does not need pseudo-objects");
                dwError = LW_ERROR_INTERNAL;
                BAIL_ON_LSA_ERROR(dwError);
            }

            dwError = LwLdapConvertDomainToDN(pszDnsDomainName, &pszDcPart);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LwAllocateStringPrintf(&pszScopeDn, "CN=$LikewiseIdentityCell,%s", pszDcPart);
            BAIL_ON_LSA_ERROR(dwError);
            break;

        case LSA_PROVISIONING_MODE_NON_DEFAULT_CELL:
            dwError = LwAllocateString(pszCellDn, &pszScopeDn);
            BAIL_ON_LSA_ERROR(dwError);
            break;

        default:
            // ASSERT
            LSA_LOG_ERROR("Unexpected mode %u", Mode);
            dwError = LW_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

    *ppszScopeDn = pszScopeDn;

cleanup:
    LW_SAFE_FREE_STRING(pszDcPart);
    return dwError;

error:
    *ppszScopeDn = NULL;
    LW_SAFE_FREE_STRING(pszScopeDn);
    goto cleanup;
}

static
DWORD
LsaAdBatchResolvePseudoObjectSidsViaGcDefaultMode(
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN DWORD dwTotalItemCount,
    // List of PLSA_AD_BATCH_ITEM
    IN OUT PLSA_LIST_LINKS pBatchItemList
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwTotalFoundCount = 0;
    DWORD dwFoundInDomainCount = 0;
    PSTR* ppszDomainNames = NULL;
    DWORD dwDomainCount = 0;
    DWORD i = 0;

    dwError = LsaAdBatchResolvePseudoObjectsInternalDefaultOrCell(
                    QueryType,
                    gpADProviderData->szDomain,
                    NULL,
                    TRUE,
                    gpADProviderData->adConfigurationMode,
                    pBatchItemList,
                    &dwFoundInDomainCount);
    BAIL_ON_LSA_ERROR(dwError);

    dwTotalFoundCount += dwFoundInDomainCount;

    if (dwTotalFoundCount == dwTotalItemCount)
    {
        goto cleanup;
    }
    else if (dwTotalFoundCount > dwTotalItemCount)
    {
        dwError = LW_ERROR_DUPLICATE_USER_OR_GROUP;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (UNPROVISIONED_MODE == gpADProviderData->dwDirectoryMode)
    {
        dwError = LsaDmEnumDomainNames(NULL, NULL, &ppszDomainNames, &dwDomainCount);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        dwError = LsaDmWrapEnumExtraTwoWayForestTrustDomains(&ppszDomainNames, &dwDomainCount);
        BAIL_ON_LSA_ERROR(dwError);
    }

    for (i = 0;
         (i < dwDomainCount) && (dwTotalFoundCount < dwTotalItemCount);
         i++)
    {
        DWORD dwTrustDirection = LSA_TRUST_DIRECTION_UNKNOWN;
        DWORD dwTrustMode = LSA_TRUST_MODE_UNKNOWN;
        BOOLEAN bIsExternalTrust = FALSE;

        if (UNPROVISIONED_MODE != gpADProviderData->dwDirectoryMode)
        {
            // check trust information to determine whether we need this domain
            dwError = AD_DetermineTrustModeandDomainName(ppszDomainNames[i],
                            &dwTrustDirection,
                            &dwTrustMode,
                            NULL,
                            NULL);
            if (LW_ERROR_NO_SUCH_DOMAIN == dwError ||
                LSA_TRUST_DIRECTION_TWO_WAY != dwTrustDirection)
            {

                LSA_ASSERT(FALSE);
                dwError = LW_ERROR_INTERNAL;
            }
            BAIL_ON_LSA_ERROR(dwError);
        }

        bIsExternalTrust = (dwTrustMode == LSA_TRUST_MODE_EXTERNAL) ? TRUE : FALSE;
        dwError = LsaAdBatchCheckDomainModeCompatibility(ppszDomainNames[i], bIsExternalTrust, NULL);
        if (dwError == LW_ERROR_INCOMPATIBLE_MODES_BETWEEN_TRUSTEDDOMAINS)
        {
            dwError = 0;
            continue;
        }
        BAIL_ON_LSA_ERROR(dwError);

        // We only process the forests that have compatible adMode with the primiary domain,
        // Hence the mode should be the same as "gpADProviderData->adConfigurationMode"
        dwError = LsaAdBatchResolvePseudoObjectsInternalDefaultOrCell(
                    QueryType,
                    ppszDomainNames[i],
                    NULL,
                    TRUE,
                    gpADProviderData->adConfigurationMode,
                    pBatchItemList,
                    &dwFoundInDomainCount);
        BAIL_ON_LSA_ERROR(dwError);

        dwTotalFoundCount += dwFoundInDomainCount;
    }


cleanup:
    LwFreeStringArray(ppszDomainNames, dwDomainCount);

    return dwError;

error:
    goto cleanup;
}

static
DWORD
LsaAdBatchResolvePseudoObjectsUnprovMode(
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN DWORD dwTotalItemCount,
    // List of PLSA_AD_BATCH_ITEM
    IN OUT PLSA_LIST_LINKS pBatchItemList
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    PLSA_LIST_LINKS pLinks = NULL;
    BOOLEAN bIsUser = FALSE;

    if (LSA_AD_BATCH_QUERY_TYPE_BY_USER_ALIAS == QueryType ||
        LSA_AD_BATCH_QUERY_TYPE_BY_GROUP_ALIAS == QueryType)
    {
        if (!ADUnprovPlugin_SupportsAliases())
        {
            dwError = LW_ERROR_NOT_SUPPORTED;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    switch (QueryType)
    {
        case LSA_AD_BATCH_QUERY_TYPE_BY_UID:
        case LSA_AD_BATCH_QUERY_TYPE_BY_USER_ALIAS:
            bIsUser = TRUE;
            break;
        case LSA_AD_BATCH_QUERY_TYPE_BY_GID:
        case LSA_AD_BATCH_QUERY_TYPE_BY_GROUP_ALIAS:
            bIsUser = FALSE;
            break;
        default:
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
    }

    for (pLinks = pBatchItemList->Next;
         pLinks != pBatchItemList;
         pLinks = pLinks->Next)
    {
        PLSA_AD_BATCH_ITEM pBatchItem = LW_STRUCT_FROM_FIELD(pLinks, LSA_AD_BATCH_ITEM, BatchItemListLinks);
        PSTR* ppszAlias = NULL;
        DWORD dwId = 0;

        if (bIsUser)
        {
            ppszAlias = &pBatchItem->UserInfo.pszAlias;
        }
        else
        {
            ppszAlias = &pBatchItem->GroupInfo.pszAlias;
        }

        switch (QueryType)
        {
            case LSA_AD_BATCH_QUERY_TYPE_BY_UID:
            case LSA_AD_BATCH_QUERY_TYPE_BY_GID:
                dwError = ADUnprovPlugin_QueryById(
                                        bIsUser,
                                        pBatchItem->QueryTerm.dwId,
                                        &pBatchItem->pszSid,
                                        ppszAlias);
                BAIL_ON_LSA_ERROR(dwError);
                dwId = pBatchItem->QueryTerm.dwId;
                break;

            case LSA_AD_BATCH_QUERY_TYPE_BY_USER_ALIAS:
            case LSA_AD_BATCH_QUERY_TYPE_BY_GROUP_ALIAS:
                dwError = ADUnprovPlugin_QueryByAlias(
                                        bIsUser,
                                        pBatchItem->QueryTerm.pszString,
                                        &pBatchItem->pszSid,
                                        &dwId);
                BAIL_ON_LSA_ERROR(dwError);
                break;

            default:
                dwError = LW_ERROR_INVALID_PARAMETER;
                BAIL_ON_LSA_ERROR(dwError);
        }

        if (bIsUser)
        {
            pBatchItem->UserInfo.uid = (uid_t)dwId;
        }
        else
        {
            pBatchItem->GroupInfo.gid = (gid_t)dwId;
        }

        i++;
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

static
DWORD
LsaAdBatchResolvePseudoObjectsDefaultMode(
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    // If pszDnsDomainName is NULL, we are resolving pseudo objects walking through all the available domains
    // If pszDnsDomainName is not NULL, we only resolving pseudo objects within that particular domain
    IN OPTIONAL PCSTR pszDnsDomainName,
    IN DWORD dwTotalItemCount,
    // List of PLSA_AD_BATCH_ITEM
    IN OUT PLSA_LIST_LINKS pBatchItemList,
    OUT OPTIONAL PBOOLEAN pbResolvedPseudo
    )
{
    DWORD dwError = 0;
    BOOLEAN bResolvedPseudo = TRUE;

    if (!LW_IS_NULL_OR_EMPTY_STR(pszDnsDomainName))
    {
        // Know the domain to search.
        dwError = LsaAdBatchResolvePseudoObjectsInternalDefaultOrCell(
                     QueryType,
                     pszDnsDomainName,
                     NULL,
                     FALSE,
                     gpADProviderData->adConfigurationMode,
                     pBatchItemList,
                     NULL);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        // Need to search all domains.  This happens when there is a
        // search by Unix attribute in default or unprovisioned mode.

        // Note that will do a GC search and therefore not
        // actually resolve full pseudo information.
        LSA_ASSERT(pbResolvedPseudo);
        bResolvedPseudo = FALSE;

        dwError = LsaAdBatchResolvePseudoObjectSidsViaGcDefaultMode(
                     QueryType,
                     dwTotalItemCount,
                     pBatchItemList);
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    if (pbResolvedPseudo)
    {
        *pbResolvedPseudo = bResolvedPseudo;
    }
    return dwError;

error:
    bResolvedPseudo = FALSE;
    goto cleanup;
}

static
DWORD
LsaAdBatchResolvePseudoObjects(
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    // If pszDnsDomainName is NULL, we are resolving pseudo objects walking through all the available domains
    // If pszDnsDomainName is not NULL, we only resolving pseudo objects within that particular domain
    IN OPTIONAL PCSTR pszDnsDomainName,
    IN DWORD dwTotalItemCount,
    // List of PLSA_AD_BATCH_ITEM
    IN OUT PLSA_LIST_LINKS pBatchItemList,
    OUT OPTIONAL PBOOLEAN pbResolvedPseudo
    )
{
    DWORD dwError = 0;
    BOOLEAN bResolvedPseudo = TRUE;

    switch (gpADProviderData->dwDirectoryMode)
    {
        case CELL_MODE:
            // Need to search the cell(s).
            dwError = LsaAdBatchResolvePseudoObjectsWithLinkedCells(
                         QueryType,
                         dwTotalItemCount,
                         pBatchItemList);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        case UNPROVISIONED_MODE:
            dwError = LsaAdBatchResolvePseudoObjectsUnprovMode(
                         QueryType,
                         dwTotalItemCount,
                         pBatchItemList);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        case  DEFAULT_MODE:
            dwError = LsaAdBatchResolvePseudoObjectsDefaultMode(
                         QueryType,
                         pszDnsDomainName,
                         dwTotalItemCount,
                         pBatchItemList,
                         &bResolvedPseudo);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = LW_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    if (pbResolvedPseudo)
    {
        *pbResolvedPseudo = bResolvedPseudo;
    }
    return dwError;

error:
    bResolvedPseudo = FALSE;
    goto cleanup;
}

DWORD
LsaAdBatchIsDefaultCell(
    IN PCSTR pszCellDN,
    OUT PBOOLEAN pbIsDefaultCell
    )
{
    DWORD dwError = 0;
    PSTR pszRootDN = NULL;
    PSTR pszDefaultCellDN = NULL;
    BOOLEAN bIsDefaultCell = FALSE;

    dwError = LwLdapConvertDomainToDN(gpADProviderData->szDomain, &pszRootDN);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateStringPrintf(
                 &pszDefaultCellDN,
                 "CN=$LikewiseIdentityCell,%s",
                 pszRootDN);
    BAIL_ON_LSA_ERROR(dwError);

    if (!strcasecmp(pszCellDN, pszDefaultCellDN))
    {
        bIsDefaultCell = TRUE;
    }

cleanup:
    LW_SAFE_FREE_STRING(pszRootDN);
    LW_SAFE_FREE_STRING(pszDefaultCellDN);

    *pbIsDefaultCell = bIsDefaultCell;

    return dwError;

error:
    goto cleanup;
}

static
DWORD
LsaAdBatchResolvePseudoObjectsWithLinkedCells(
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN DWORD dwTotalItemCount,
    // List of PLSA_AD_BATCH_ITEM
    IN OUT PLSA_LIST_LINKS pBatchItemList
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PDLINKEDLIST pCellNode = NULL;
    DWORD dwTotalFoundCount = 0;
    DWORD dwFoundInCellCount = 0;
    BOOLEAN bIsDefaultCell = FALSE;

    dwError = LsaAdBatchResolvePseudoObjectsInternalDefaultOrCell(
                    QueryType,
                    NULL,
                    gpADProviderData->cell.szCellDN,
                    FALSE,
                    gpADProviderData->adConfigurationMode,
                    pBatchItemList,
                    &dwFoundInCellCount);
    BAIL_ON_LSA_ERROR(dwError);

    dwTotalFoundCount += dwFoundInCellCount;

    for (pCellNode = gpADProviderData->pCellList;
         pCellNode && (dwTotalFoundCount < dwTotalItemCount);
         pCellNode = pCellNode->pNext)
    {
        ADConfigurationMode adMode = UnknownMode;
        PAD_LINKED_CELL_INFO pCellInfo = (PAD_LINKED_CELL_INFO)pCellNode->pItem;

        if (!pCellInfo)
        {
            // This should never happen.
            continue;
        }

        // determine schema/non-schema mode in the current cell
        dwError = LsaAdBatchQueryCellConfigurationMode(
                       gpADProviderData->szDomain,
                       pCellInfo->pszCellDN,
                       &adMode);
        BAIL_ON_LSA_ERROR(dwError);

        if (adMode == UnknownMode)
        {
            continue;
        }

        dwError = LsaAdBatchIsDefaultCell(pCellInfo->pszCellDN,
                                          &bIsDefaultCell);
        BAIL_ON_LSA_ERROR(dwError);

        if (bIsDefaultCell && SchemaMode == adMode)
        {
            dwError = LsaAdBatchResolvePseudoObjectsInternalDefaultSchema(
                                     QueryType,
                                     gpADProviderData->szDomain,
                                     pBatchItemList,
                                     &dwFoundInCellCount);
            BAIL_ON_LSA_ERROR(dwError);
        }
        else
        {
            dwError = LsaAdBatchResolvePseudoObjectsInternalDefaultOrCell(
                                    QueryType,
                                    NULL,
                                    pCellInfo->pszCellDN,
                                    FALSE,
                                    adMode,
                                    pBatchItemList,
                                    &dwFoundInCellCount);
            BAIL_ON_LSA_ERROR(dwError);
        }

        dwTotalFoundCount += dwFoundInCellCount;
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

static
DWORD
LsaAdBatchResolvePseudoObjectsInternalDefaultSchema(
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN PCSTR pszDnsDomainName,
    // List of PLSA_AD_BATCH_ITEM
    IN OUT PLSA_LIST_LINKS pBatchItemList,
    OUT OPTIONAL PDWORD pdwTotalItemFoundCount
    )
{
    DWORD dwError = 0;
    HANDLE hDirectory = NULL;
    LDAP* pLd = NULL;
    PSTR pszQuery = 0;
    PSTR szAttributeList[] =
    {
        AD_LDAP_OBJECTCLASS_TAG,
        AD_LDAP_OBJECTSID_TAG,
        AD_LDAP_SAM_NAME_TAG,
        AD_LDAP_DN_TAG,
        AD_LDAP_PRIMEGID_TAG,
        AD_LDAP_UPN_TAG,
        AD_LDAP_USER_CTRL_TAG,
        AD_LDAP_ACCOUT_EXP_TAG,
        AD_LDAP_PWD_LASTSET_TAG,
        AD_LDAP_DISPLAY_NAME_TAG,
        AD_LDAP_ALIAS_TAG,
        AD_LDAP_UID_TAG,
        AD_LDAP_GID_TAG,
        AD_LDAP_PASSWD_TAG,
        AD_LDAP_GECOS_TAG,
        AD_LDAP_HOMEDIR_TAG,
        AD_LDAP_SHELL_TAG,
        NULL
    };
    LDAPMessage* pMessage = NULL;
    PLSA_LIST_LINKS pLinks = NULL;
    PLSA_LIST_LINKS pNextLinks = NULL;
    DWORD dwMaxQuerySize = LsaAdBatchGetMaxQuerySize();
    DWORD dwMaxQueryCount = LsaAdBatchGetMaxQueryCount();
    DWORD dwTotalItemFoundCount = 0;
    PLSA_DM_LDAP_CONNECTION pConn = NULL;
    PSTR pszDomainDN = NULL;

    if (LW_IS_NULL_OR_EMPTY_STR(pszDnsDomainName))
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaDmLdapOpenDc(pszDnsDomainName, &pConn);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwLdapConvertDomainToDN(pszDnsDomainName,
                                       &pszDomainDN);
    BAIL_ON_LSA_ERROR(dwError);

    for (pLinks = pBatchItemList->Next;
         pLinks != pBatchItemList;
         pLinks = pNextLinks)
    {
        DWORD dwQueryCount = 0;
        DWORD dwCount = 0;
        LDAPMessage* pCurrentMessage = NULL;

        pNextLinks = NULL;

        LW_SAFE_FREE_STRING(pszQuery);
        if (pMessage)
        {
            ldap_msgfree(pMessage);
            pMessage = NULL;
        }

        dwError = LsaAdBatchBuildQueryForPseudoDefaultSchema(
                        QueryType,
                        pLinks,
                        pBatchItemList,
                        &pNextLinks,
                        dwMaxQuerySize,
                        dwMaxQueryCount,
                        &dwQueryCount,
                        &pszQuery);
        BAIL_ON_LSA_ERROR(dwError);

        if (LW_IS_NULL_OR_EMPTY_STR(pszQuery))
        {
            break;
        }

        dwError = LsaDmLdapDirectorySearch(
                        pConn,
                        pszDomainDN,
                        LDAP_SCOPE_SUBTREE,
                        pszQuery,
                        szAttributeList,
                        &hDirectory,
                        &pMessage);
        BAIL_ON_LSA_ERROR(dwError);

        pLd = LwLdapGetSession(hDirectory);

        dwCount = ldap_count_entries(pLd, pMessage);
        if (dwCount > dwQueryCount)
        {
            LSA_LOG_ERROR("Too many results returned (got %u, expected %u)",
                          dwCount, dwQueryCount);
            dwError = LW_ERROR_LDAP_ERROR;
            BAIL_ON_LSA_ERROR(dwError);
        }
        else if (dwCount == 0)
        {
            continue;
        }

        dwCount = 0;

        pCurrentMessage = ldap_first_entry(pLd, pMessage);
        while (pCurrentMessage)
        {
            dwError = LsaAdBatchProcessPseudoObjectDefaultSchema(
                            QueryType,
                            pLinks,
                            pNextLinks,
                            hDirectory,
                            pCurrentMessage);
            BAIL_ON_LSA_ERROR(dwError);
            dwCount++;

            pCurrentMessage = ldap_next_entry(pLd, pCurrentMessage);
        }

        dwTotalItemFoundCount += dwCount;
    }


    if (pdwTotalItemFoundCount)
    {
       *pdwTotalItemFoundCount = dwTotalItemFoundCount;
    }

cleanup:
    LsaDmLdapClose(pConn);
    LW_SAFE_FREE_STRING(pszQuery);
    LW_SAFE_FREE_STRING(pszDomainDN);
    if (pMessage)
    {
        ldap_msgfree(pMessage);
    }
    return dwError;

error:
    if (pdwTotalItemFoundCount)
    {
       *pdwTotalItemFoundCount = 0;
    }

    goto cleanup;
}

static
DWORD
LsaAdBatchResolvePseudoObjectsInternalDefaultOrCell(
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN OPTIONAL PCSTR pszDnsDomainName,
    IN OPTIONAL PCSTR pszCellDn,
    IN BOOLEAN bDoGCSearch,
    IN ADConfigurationMode adMode,
    // List of PLSA_AD_BATCH_ITEM
    IN OUT PLSA_LIST_LINKS pBatchItemList,
    OUT OPTIONAL PDWORD pdwTotalItemFoundCount
    )
{
    DWORD dwError = 0;
    HANDLE hDirectory = NULL;
    LDAP* pLd = NULL;
    PSTR pszScopeDn = NULL;
    PSTR pszQuery = 0;
    PSTR szAttributeList[] =
    {
        // AD attributes:
        // - common:
        AD_LDAP_OBJECTCLASS_TAG,
        AD_LDAP_OBJECTSID_TAG,
        AD_LDAP_SAM_NAME_TAG,
        AD_LDAP_DN_TAG,
        // - user-specific:
        // In default schema mode, we will do GC search,
        // even we look up real objects directly, GC does not provide those user-specific attribute values.
        // Those values will be looked up when we look up real objects in the second step
#if 0
        // Indexed and in GC:
        AD_LDAP_PRIMEGID_TAG,
        AD_LDAP_UPN_TAG,
        AD_LDAP_USER_CTRL_TAG,
        // Not indexed and not in GC:
        AD_LDAP_ACCOUT_EXP_TAG,
        AD_LDAP_PWD_LASTSET_TAG,
#endif
        // non-schema mode:
        AD_LDAP_KEYWORDS_TAG,
        // schema mode:
        // - group alias:
        AD_LDAP_DISPLAY_NAME_TAG,
        // - unix properties (alias is just user alias):
        AD_LDAP_ALIAS_TAG,
        AD_LDAP_UID_TAG,
        AD_LDAP_GID_TAG,
        // A GC search (default mode) will not find these:
        AD_LDAP_PASSWD_TAG,
        AD_LDAP_GECOS_TAG,
        AD_LDAP_HOMEDIR_TAG,
        AD_LDAP_SHELL_TAG,
        NULL
    };
    LDAPMessage* pMessage = NULL;
    PLSA_LIST_LINKS pLinks = NULL;
    PLSA_LIST_LINKS pNextLinks = NULL;
    DWORD dwMaxQuerySize = LsaAdBatchGetMaxQuerySize();
    DWORD dwMaxQueryCount = LsaAdBatchGetMaxQueryCount();
    PSTR pszDomainName = NULL;
    DWORD dwTotalItemFoundCount = 0;
    PSTR pUserPseudoDN = NULL;
    PLSA_DM_LDAP_CONNECTION pConn = NULL;

    if (bDoGCSearch && LW_IS_NULL_OR_EMPTY_STR(pszDnsDomainName))
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (bDoGCSearch)
    {
        dwError = LwAllocateString("", &pszScopeDn);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaDmLdapOpenGc(pszDnsDomainName, &pConn);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        dwError = LsaAdBatchBuildQueryScopeForPseudo(
                            (adMode == SchemaMode),
                            gpADProviderData->dwDirectoryMode,
                            pszDnsDomainName,
                            pszCellDn,
                            &pszScopeDn);
        BAIL_ON_LSA_ERROR(dwError);

        // Need to do this because the pseudo-object domain
        // could be different from the real object's domain
        // (e.g., non-default cell).
        dwError = LwLdapConvertDNToDomain(
                           pszScopeDn,
                           &pszDomainName);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaDmLdapOpenDc(pszDomainName, &pConn);
        BAIL_ON_LSA_ERROR(dwError);
    }

    for (pLinks = pBatchItemList->Next;
         pLinks != pBatchItemList;
         pLinks = pNextLinks)
    {
        DWORD dwQueryCount = 0;
        DWORD dwCount = 0;
        LDAPMessage* pCurrentMessage = NULL;

        pNextLinks = NULL;

        LW_SAFE_FREE_STRING(pszQuery);
        if (pMessage)
        {
            ldap_msgfree(pMessage);
            pMessage = NULL;
        }

        dwError = LsaAdBatchBuildQueryForPseudo(
                        (adMode == SchemaMode),
                        QueryType,
                        pLinks,
                        pBatchItemList,
                        &pNextLinks,
                        dwMaxQuerySize,
                        dwMaxQueryCount,
                        &dwQueryCount,
                        &pszQuery);
        BAIL_ON_LSA_ERROR(dwError);

        if (LW_IS_NULL_OR_EMPTY_STR(pszQuery))
        {
            break;
        }

        dwError = LsaDmLdapDirectorySearch(
                        pConn,
                        pszScopeDn,
                        LDAP_SCOPE_SUBTREE,
                        pszQuery,
                        szAttributeList,
                        &hDirectory,
                        &pMessage);
        BAIL_ON_LSA_ERROR(dwError);

        pLd = LwLdapGetSession(hDirectory);

        dwCount = ldap_count_entries(pLd, pMessage);
        // In Default Non-schema mode, we might get entries in non-default cells due to the GC search
        // Hence, dwCount can be more than dwQueryCount
        if (!(NonSchemaMode == adMode && bDoGCSearch) &&
             dwCount > dwQueryCount)
        {
            LSA_LOG_ERROR("Too many results returned (got %u, expected %u)",
                          dwCount, dwQueryCount);
            dwError = LW_ERROR_LDAP_ERROR;
            BAIL_ON_LSA_ERROR(dwError);
        }
        else if (dwCount == 0)
        {
            continue;
        }

        dwCount = 0;

        pCurrentMessage = ldap_first_entry(pLd, pMessage);
        while (pCurrentMessage)
        {
            // Default Non-schema mode doing a GC search
            if (NonSchemaMode == adMode && bDoGCSearch)
            {
                LW_SAFE_FREE_STRING(pUserPseudoDN);

                dwError = LwLdapGetDN(
                             hDirectory,
                             pCurrentMessage,
                             &pUserPseudoDN);
                BAIL_ON_LSA_ERROR(dwError);

                LwStrToUpper(pUserPseudoDN);

                // Make sure the found pseudo object is enabled in default cell;
                // Otherwise, skip this pCurrentMessage
                if (!strstr(pUserPseudoDN, ",CN=$LIKEWISEIDENTITYCELL,DC="))
                {
                    pCurrentMessage = ldap_next_entry(pLd, pCurrentMessage);
                    continue;
                }
            }

            dwError = LsaAdBatchProcessPseudoObject(
                            QueryType,
                            pLinks,
                            pNextLinks,
                            bDoGCSearch,
                            (adMode == SchemaMode),
                            hDirectory,
                            pCurrentMessage);
            BAIL_ON_LSA_ERROR(dwError);
            dwCount++;

            pCurrentMessage = ldap_next_entry(pLd, pCurrentMessage);
        }

        dwTotalItemFoundCount += dwCount;
    }

    if (pdwTotalItemFoundCount)
    {
       *pdwTotalItemFoundCount = dwTotalItemFoundCount;
    }

cleanup:
    LsaDmLdapClose(pConn);
    LW_SAFE_FREE_STRING(pszDomainName);
    LW_SAFE_FREE_STRING(pszScopeDn);
    LW_SAFE_FREE_STRING(pszQuery);
    LW_SAFE_FREE_STRING(pUserPseudoDN);
    if (pMessage)
    {
        ldap_msgfree(pMessage);
    }
    return dwError;

error:
    if (pdwTotalItemFoundCount)
    {
       *pdwTotalItemFoundCount = 0;
    }

    goto cleanup;
}

static
BOOLEAN
LsaAdBatchIsUserOrGroupObjectType(
    IN LSA_AD_BATCH_OBJECT_TYPE ObjectType
    )
{
    BOOLEAN bIsOk = FALSE;

    switch (ObjectType)
    {
        case LSA_AD_BATCH_OBJECT_TYPE_USER:
        case LSA_AD_BATCH_OBJECT_TYPE_GROUP:
            bIsOk = TRUE;
            break;
        default:
            break;
    }

    return bIsOk;
}

static
DWORD
LsaAdBatchGetObjectTypeFromRealMessage(
    OUT PLSA_AD_BATCH_OBJECT_TYPE pObjectType,
    IN HANDLE hDirectory,
    IN LDAPMessage* pMessage
    )
{
    DWORD dwError = 0;
    LSA_AD_BATCH_OBJECT_TYPE objectType = 0;
    PSTR* ppszValues = NULL;
    DWORD dwValuesCount = 0;
    DWORD i = 0;

    dwError = LwLdapGetStrings(
                    hDirectory,
                    pMessage,
                    AD_LDAP_OBJECTCLASS_TAG,
                    &ppszValues,
                    &dwValuesCount);
    BAIL_ON_LSA_ERROR(dwError);

    for (i = 0; i < dwValuesCount; i++)
    {
        if (!strcasecmp(ppszValues[i], "user"))
        {
            objectType = LSA_AD_BATCH_OBJECT_TYPE_USER;
            break;
        }
        else if (!strcasecmp(ppszValues[i], "group"))
        {
            objectType = LSA_AD_BATCH_OBJECT_TYPE_GROUP;
            break;
        }
    }

    if (!LsaAdBatchIsUserOrGroupObjectType(objectType))
    {
        dwError = LW_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    LwFreeStringArray(ppszValues, dwValuesCount);

    *pObjectType = objectType;

    return dwError;

error:
    goto cleanup;
}

static
DWORD
LsaAdBatchGetObjectTypeFromPseudoKeywords(
    IN DWORD dwKeywordValuesCount,
    IN PSTR* ppszKeywordValues
    )
{
    LSA_AD_BATCH_OBJECT_TYPE objectType = 0;
    DWORD i = 0;

    for (i = 0; i < dwKeywordValuesCount; i++)
    {
        if (!strcasecmp(ppszKeywordValues[i], "objectClass=" AD_LDAP_CLASS_LW_USER))
        {
            objectType = LSA_AD_BATCH_OBJECT_TYPE_USER;
            break;
        }
        else if (!strcasecmp(ppszKeywordValues[i], "objectClass=" AD_LDAP_CLASS_LW_GROUP))
        {
            objectType = LSA_AD_BATCH_OBJECT_TYPE_GROUP;
            break;
        }
    }

    return objectType;
}

static
DWORD
LsaAdBatchGetObjectTypeFromPseudoMessage(
    OUT PLSA_AD_BATCH_OBJECT_TYPE pObjectType,
    IN BOOLEAN bIsSchemaMode,
    IN OPTIONAL DWORD dwKeywordValuesCount,
    IN OPTIONAL PSTR* ppszKeywordValues,
    IN HANDLE hDirectory,
    IN LDAPMessage* pMessage
    )
{
    DWORD dwError = 0;
    LSA_AD_BATCH_OBJECT_TYPE objectType = 0;
    LSA_AD_BATCH_OBJECT_TYPE keywordsObjectType = 0;
    PSTR* ppszValues = NULL;
    DWORD dwValuesCount = 0;
    DWORD i = 0;

    if (LsaAdBatchIsDefaultSchemaMode())
    {
        dwError = LsaAdBatchGetObjectTypeFromRealMessage(
                        &objectType,
                        hDirectory,
                        pMessage);
        BAIL_ON_LSA_ERROR(dwError);
        goto cleanup;
    }

    keywordsObjectType = LsaAdBatchGetObjectTypeFromPseudoKeywords(
                                dwKeywordValuesCount,
                                ppszKeywordValues);
    if (!keywordsObjectType)
    {
        LSA_ASSERT(FALSE);
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (!LsaAdBatchIsUserOrGroupObjectType(keywordsObjectType))
    {
        dwError = LW_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    // double-check against the object class.

    dwError = LwLdapGetStrings(
                    hDirectory,
                    pMessage,
                    AD_LDAP_OBJECTCLASS_TAG,
                    &ppszValues,
                    &dwValuesCount);
    BAIL_ON_LSA_ERROR(dwError);

    for (i = 0; i < dwValuesCount; i++)
    {
        if (bIsSchemaMode)
        {
            if (!strcasecmp(ppszValues[i], AD_LDAP_CLASS_SCHEMA_USER))
            {
                objectType = LSA_AD_BATCH_OBJECT_TYPE_USER;
                break;
            }
            else if (!strcasecmp(ppszValues[i], AD_LDAP_CLASS_SCHEMA_GROUP))
            {
                objectType = LSA_AD_BATCH_OBJECT_TYPE_GROUP;
                break;
            }
        }
        else
        {
            if (!strcasecmp(ppszValues[i], AD_LDAP_CLASS_NON_SCHEMA))
            {
                objectType = keywordsObjectType;
                break;
            }
        }
    }

    if (!LsaAdBatchIsUserOrGroupObjectType(objectType))
    {
        dwError = LW_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (objectType != keywordsObjectType)
    {
        LSA_LOG_DEBUG("Object type mismatch: %u vs %u",
                      keywordsObjectType, objectType);
        dwError = LW_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    LwFreeStringArray(ppszValues, dwValuesCount);

    *pObjectType = objectType;

    return dwError;

error:
    goto cleanup;
}

static
PCSTR
LsaAdBatchFindKeywordAttribute(
    IN DWORD dwKeywordValuesCount,
    IN PSTR* ppszKeywordValues,
    IN PCSTR pszAttributeName
    )
{
    PCSTR pszAttributeValue = NULL;
    size_t sNameLen = 0;
    size_t i = 0;

    if (dwKeywordValuesCount > 0)
    {
        sNameLen = strlen(pszAttributeName);
    }

    for (i = 0; i < dwKeywordValuesCount; i++)
    {
        PCSTR pszKeywordValue = ppszKeywordValues[i];

        // Look for ldap values which are in the form <attributename>=<value>
        if (!strncasecmp(pszKeywordValue, pszAttributeName, sNameLen) &&
            pszKeywordValue[sNameLen] == '=')
        {
            pszAttributeValue = pszKeywordValue + sNameLen + 1;
            break;
        }
    }

    return pszAttributeValue;
}

PCSTR
LsaAdBatchFindKeywordAttributeWithEqual(
    IN DWORD dwKeywordValuesCount,
    IN PSTR* ppszKeywordValues,
    IN PCSTR pszAttributeNameWithEqual,
    IN size_t sAttributeNameWithEqualLength
    )
{
    PCSTR pszAttributeValue = NULL;
    size_t i = 0;

    LSA_ASSERT('=' == pszAttributeNameWithEqual[sAttributeNameWithEqualLength-1]);

    for (i = 0; i < dwKeywordValuesCount; i++)
    {
        PCSTR pszKeywordValue = ppszKeywordValues[i];

        // Look for ldap values which are in the form <attributename>=<value>
        if (!strncasecmp(pszKeywordValue, pszAttributeNameWithEqual, sAttributeNameWithEqualLength))
        {
            pszAttributeValue = pszKeywordValue + sAttributeNameWithEqualLength;
            break;
        }
    }

    return pszAttributeValue;
}

static
DWORD
LsaAdBatchGetCompareStringFromRealObject(
    OUT PSTR* ppszCompareString,
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN HANDLE hDirectory,
    IN LDAPMessage* pMessage
    )
{
    DWORD dwError = 0;
    PSTR pszCompare = NULL;

    switch (QueryType)
    {
        case LSA_AD_BATCH_QUERY_TYPE_BY_DN:
            dwError = LwLdapGetDN(hDirectory, pMessage, &pszCompare);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        case LSA_AD_BATCH_QUERY_TYPE_BY_SID:
            dwError = ADLdap_GetObjectSid(hDirectory, pMessage, &pszCompare);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        case LSA_AD_BATCH_QUERY_TYPE_BY_NT4:
            dwError = LwLdapGetString(
                            hDirectory,
                            pMessage,
                            AD_LDAP_SAM_NAME_TAG,
                            &pszCompare);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            LSA_ASSERT(FALSE);
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
    }

    if (LW_IS_NULL_OR_EMPTY_STR(pszCompare))
    {
        dwError = LW_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    *ppszCompareString = pszCompare;
    return dwError;

error:
    LW_SAFE_FREE_STRING(pszCompare);
    goto cleanup;
}


static
DWORD
LsaAdBatchGetCompareStringFromPseudoObjectDefaultSchema(
    OUT PSTR* ppszCompareString,
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN HANDLE hDirectory,
    IN LDAPMessage* pMessage
    )
{
    DWORD dwError = 0;
    PSTR pszCompare = NULL;

    switch (QueryType)
    {
        case LSA_AD_BATCH_QUERY_TYPE_BY_DN:
            dwError = LwLdapGetDN(hDirectory, pMessage, &pszCompare);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        case LSA_AD_BATCH_QUERY_TYPE_BY_SID:
            dwError = ADLdap_GetObjectSid(hDirectory, pMessage, &pszCompare);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        case LSA_AD_BATCH_QUERY_TYPE_BY_NT4:
            dwError = LwLdapGetString(
                            hDirectory,
                            pMessage,
                            AD_LDAP_SAM_NAME_TAG,
                            &pszCompare);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        case LSA_AD_BATCH_QUERY_TYPE_BY_USER_ALIAS:
            dwError = LwLdapGetString(
                            hDirectory,
                            pMessage,
                            AD_LDAP_ALIAS_TAG,
                            &pszCompare);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        case LSA_AD_BATCH_QUERY_TYPE_BY_GROUP_ALIAS:
            dwError = LwLdapGetString(
                            hDirectory,
                            pMessage,
                            AD_LDAP_DISPLAY_NAME_TAG,
                            &pszCompare);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        case LSA_AD_BATCH_QUERY_TYPE_BY_UID:
            dwError = LwLdapGetString(
                            hDirectory,
                            pMessage,
                            AD_LDAP_UID_TAG,
                            &pszCompare);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        case LSA_AD_BATCH_QUERY_TYPE_BY_GID:
            dwError = LwLdapGetString(
                            hDirectory,
                            pMessage,
                            AD_LDAP_GID_TAG,
                            &pszCompare);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            LSA_ASSERT(FALSE);
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
    }

    if (LW_IS_NULL_OR_EMPTY_STR(pszCompare))
    {
        dwError = LW_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    *ppszCompareString = pszCompare;
    return dwError;

error:
    LW_SAFE_FREE_STRING(pszCompare);
    goto cleanup;
}

static
DWORD
LsaAdBatchGetCompareStringFromPseudoObject(
    OUT PCSTR* ppszCompareString,
    OUT PSTR* ppszFreeString,
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN BOOLEAN bIsSchemaMode,
    IN OPTIONAL DWORD dwKeywordValuesCount,
    IN OPTIONAL PSTR* ppszKeywordValues,
    IN HANDLE hDirectory,
    IN LDAPMessage* pMessage
    )
{
    DWORD dwError = 0;
    PCSTR pszAttributeName = NULL;
    PSTR pszFreeCompare = NULL;
    PCSTR pszCompare = NULL;

    LSA_ASSERT(LSA_IS_XOR(LsaAdBatchIsDefaultSchemaMode(), ppszKeywordValues));

    switch (QueryType)
    {
        case LSA_AD_BATCH_QUERY_TYPE_BY_SID:
            // The SID backlink is stored in keywords except for in
            // default schema mode.  But in default schema mode,
            // we never do a pseudo lookup by SID.
            LSA_ASSERT(!LsaAdBatchIsDefaultSchemaMode());
            pszCompare = LsaAdBatchFindKeywordAttributeStatic(
                                dwKeywordValuesCount,
                                ppszKeywordValues,
                                AD_LDAP_BACKLINK_PSEUDO_TAG);
            if (LW_IS_NULL_OR_EMPTY_STR(pszCompare))
            {
                dwError = LW_ERROR_INVALID_SID;
                BAIL_ON_LSA_ERROR(dwError);
            }
            break;
        case LSA_AD_BATCH_QUERY_TYPE_BY_USER_ALIAS:
            pszAttributeName = AD_LDAP_ALIAS_TAG;
            break;
        case LSA_AD_BATCH_QUERY_TYPE_BY_GROUP_ALIAS:
            pszAttributeName = AD_LDAP_DISPLAY_NAME_TAG;
            break;
        case LSA_AD_BATCH_QUERY_TYPE_BY_UID:
            pszAttributeName = AD_LDAP_UID_TAG;
            break;
        case LSA_AD_BATCH_QUERY_TYPE_BY_GID:
            pszAttributeName = AD_LDAP_GID_TAG;
            break;
        default:
            LSA_ASSERT(FALSE);
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
    }

    if (pszAttributeName)
    {
        if (bIsSchemaMode)
        {
            dwError = LwLdapGetString(
                            hDirectory,
                            pMessage,
                            pszAttributeName,
                            &pszFreeCompare);
            BAIL_ON_LSA_ERROR(dwError);
            pszCompare = pszFreeCompare;
        }
        else
        {
            // Note that pszCompare is valid only as long
            // as the keyword strings are not freed.
            // The caller must keep any returned keyword
            // string around as long as it uses the compare
            // string result.
            pszCompare = LsaAdBatchFindKeywordAttribute(
                                dwKeywordValuesCount,
                                ppszKeywordValues,
                                pszAttributeName);
        }
    }

cleanup:
    LSA_ASSERT(!pszFreeCompare || (pszCompare == pszFreeCompare));

    *ppszFreeString = pszFreeCompare;
    *ppszCompareString = pszCompare;
    return dwError;

error:
    LW_SAFE_FREE_STRING(pszFreeCompare);
    pszCompare = NULL;
    goto cleanup;
}

static
DWORD
LsaAdBatchProcessRpcObject(
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    // List of PLSA_AD_BATCH_ITEM
    IN OUT PLSA_LIST_LINKS pStartBatchItemListLinks,
    IN PLSA_LIST_LINKS pEndBatchItemListLinks,
    IN PSTR pszObjectNT4NameOrSid,
    IN PLSA_TRANSLATED_NAME_OR_SID pTranslatedName
    )
{
    DWORD dwError = 0;
    PSTR pszSid = NULL;
    PSTR pszSamAccountName = NULL;
    PCSTR pszFoundSamAccountName = NULL;
    PLSA_LOGIN_NAME_INFO pLoginNameInfo = NULL;
    PCSTR pszCompare = NULL;
    LSA_AD_BATCH_OBJECT_TYPE objectType = 0;
    LSA_AD_BATCH_OBJECT_TYPE desiredObjectType = 0;
    PLSA_LIST_LINKS pLinks = NULL;
    PLSA_AD_BATCH_ITEM pFoundItem = NULL;

    switch (QueryType)
    {
        case LSA_AD_BATCH_QUERY_TYPE_BY_SID:
            dwError = LsaCrackDomainQualifiedName(
                                 pTranslatedName->pszNT4NameOrSid,
                                 gpADProviderData->szDomain,
                                 &pLoginNameInfo);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LwAllocateString(pszObjectNT4NameOrSid, &pszSid);
            BAIL_ON_LSA_ERROR(dwError);

            LSA_XFER_STRING(pLoginNameInfo->pszName, pszSamAccountName);

            pszCompare = pszSid;
            break;

        case LSA_AD_BATCH_QUERY_TYPE_BY_NT4:
            // The name is in backslash format, not separator
            pszFoundSamAccountName = index(pszObjectNT4NameOrSid, '\\');
            if (!pszFoundSamAccountName)
            {
                LSA_ASSERT(FALSE);
                dwError = LW_ERROR_INTERNAL;
                BAIL_ON_LSA_ERROR(dwError);
            }
            pszFoundSamAccountName++;
            if (!pszFoundSamAccountName[0])
            {
                LSA_ASSERT(FALSE);
                dwError = LW_ERROR_INTERNAL;
                BAIL_ON_LSA_ERROR(dwError);
            }

            dwError = LwAllocateString(pszFoundSamAccountName, &pszSamAccountName);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LwAllocateString(pTranslatedName->pszNT4NameOrSid, &pszSid);
            BAIL_ON_LSA_ERROR(dwError);

            pszCompare = pszSamAccountName;
            break;

        default:
            LSA_ASSERT(FALSE);
            dwError = LW_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaAdBatchAccountTypeToObjectType(
                    pTranslatedName->ObjectType,
                    &objectType);
    BAIL_ON_LSA_ERROR(dwError);

    if (!LsaAdBatchIsUserOrGroupObjectType(objectType))
    {
        // We found something else.
        LSA_LOG_DEBUG("Found non-user/non-group object type %d",
                      pTranslatedName->ObjectType);
        dwError = LW_ERROR_DATA_ERROR;
    }

    desiredObjectType = LsaAdBatchGetObjectTypeFromQueryType(QueryType);
    if ((desiredObjectType != LSA_AD_BATCH_OBJECT_TYPE_UNDEFINED) &&
        (objectType != desiredObjectType))
    {
        LSA_LOG_DEBUG("Object type mismatch: got %u instead of %u",
                      objectType, desiredObjectType);
        // This cannot happen because we restrict the type we search on.
        LSA_ASSERT(FALSE);
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_LSA_ERROR(dwError);
    }

    for (pLinks = pStartBatchItemListLinks;
         pLinks != pEndBatchItemListLinks;
         pLinks = pLinks->Next)
    {
        PLSA_AD_BATCH_ITEM pItem = LW_STRUCT_FROM_FIELD(pLinks, LSA_AD_BATCH_ITEM, BatchItemListLinks);

        if (pItem->pszQueryMatchTerm &&
            !strcasecmp(pItem->pszQueryMatchTerm, pszCompare))
        {
            pFoundItem = pItem;
            break;
        }
    }

    if (!pFoundItem)
    {
        PCSTR pszType = LsaAdBatchGetQueryTypeAsString(QueryType);
        LSA_LOG_DEBUG("Did not find batch item for message for %s '%s'",
                      pszType, pszCompare);
        LSA_ASSERT(FALSE);
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaAdBatchGatherRpcObject(
                    pFoundItem,
                    objectType,
                    &pszSid,
                    &pszSamAccountName);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    LW_SAFE_FREE_STRING(pszSid);
    LW_SAFE_FREE_STRING(pszSamAccountName);
    LSA_SAFE_FREE_LOGIN_NAME_INFO(pLoginNameInfo);

    return dwError;

error:
    goto cleanup;
}

static
DWORD
LsaAdBatchProcessRealObject(
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    // List of PLSA_AD_BATCH_ITEM
    IN OUT PLSA_LIST_LINKS pStartBatchItemListLinks,
    IN PLSA_LIST_LINKS pEndBatchItemListLinks,
    IN HANDLE hDirectory,
    IN LDAPMessage* pMessage
    )
{
    DWORD dwError = 0;
    PSTR pszCompare = NULL;
    LSA_AD_BATCH_OBJECT_TYPE objectType = 0;
    LSA_AD_BATCH_OBJECT_TYPE desiredObjectType = 0;
    PLSA_LIST_LINKS pLinks = NULL;
    PLSA_AD_BATCH_ITEM pFoundItem = NULL;

    // Get compare string

    dwError = LsaAdBatchGetCompareStringFromRealObject(
                    &pszCompare,
                    QueryType,
                    hDirectory,
                    pMessage);
    BAIL_ON_LSA_ERROR(dwError);

    // Get and check object type

    dwError = LsaAdBatchGetObjectTypeFromRealMessage(
                    &objectType,
                    hDirectory,
                    pMessage);
    BAIL_ON_LSA_ERROR(dwError);

    // Sanity check.
    desiredObjectType = LsaAdBatchGetObjectTypeFromQueryType(QueryType);
    if ((desiredObjectType != LSA_AD_BATCH_OBJECT_TYPE_UNDEFINED) &&
        (objectType != desiredObjectType))
    {
        PCSTR pszType = LsaAdBatchGetQueryTypeAsString(QueryType);
        LSA_LOG_DEBUG("Object type mismatch for %s '%s' - got %u instead of %u",
                      pszType, pszCompare, objectType, desiredObjectType);
        // This cannot happen because we restrict the type we search on.
        LSA_ASSERT(FALSE);
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_LSA_ERROR(dwError);
    }

    // Search of corresponding batch item

    for (pLinks = pStartBatchItemListLinks;
         pLinks != pEndBatchItemListLinks;
         pLinks = pLinks->Next)
    {
        PLSA_AD_BATCH_ITEM pItem = LW_STRUCT_FROM_FIELD(pLinks, LSA_AD_BATCH_ITEM, BatchItemListLinks);

        XXX; // may want to just skip if no query term...hmm...
        LSA_ASSERT(pItem->pszQueryMatchTerm);

        if (!strcasecmp(pItem->pszQueryMatchTerm, pszCompare))
        {
            pFoundItem = pItem;
            break;
        }
    }

    if (!pFoundItem)
    {
        PCSTR pszType = LsaAdBatchGetQueryTypeAsString(QueryType);
        LSA_LOG_DEBUG("Did not find batch item for message for %s '%s'",
                      pszType, pszCompare);
        LSA_ASSERT(FALSE);
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaAdBatchGatherRealObject(
                    pFoundItem,
                    objectType,
                    (LSA_AD_BATCH_QUERY_TYPE_BY_SID == QueryType) ? &pszCompare : NULL,
                    hDirectory,
                    pMessage);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    LW_SAFE_FREE_STRING(pszCompare);

    return dwError;

error:
    goto cleanup;
}

static
DWORD
LsaAdBatchProcessPseudoObject(
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    // List of PLSA_AD_BATCH_ITEM
    IN OUT PLSA_LIST_LINKS pStartBatchItemListLinks,
    IN PLSA_LIST_LINKS pEndBatchItemListLinks,
    IN BOOLEAN bIsGcSearch,
    IN BOOLEAN bIsSchemaMode,
    IN HANDLE hDirectory,
    IN LDAPMessage* pMessage
    )
{
    DWORD dwError = 0;
    BOOLEAN bIsValid = FALSE;
    PSTR* ppszKeywordValues = NULL;
    DWORD dwKeywordValuesCount = 0;
    PSTR pszFreeCompare = NULL;
    PCSTR pszCompare = NULL;
    LSA_AD_BATCH_OBJECT_TYPE objectType = 0;
    LSA_AD_BATCH_OBJECT_TYPE desiredObjectType = 0;
    PLSA_LIST_LINKS pLinks = NULL;
    PLSA_AD_BATCH_ITEM pFoundItem = NULL;

    dwError = LwLdapIsValidADEntry(
                    hDirectory,
                    pMessage,
                    &bIsValid);
    BAIL_ON_LSA_ERROR(dwError);

    if (!bIsValid)
    {
        dwError = LW_ERROR_LDAP_FAILED_GETDN;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (!LsaAdBatchIsDefaultSchemaMode())
    {
        dwError = LwLdapGetStrings(
                        hDirectory,
                        pMessage,
                        AD_LDAP_KEYWORDS_TAG,
                        &ppszKeywordValues,
                        &dwKeywordValuesCount);
        BAIL_ON_LSA_ERROR(dwError);
    }

    // Get compare string

    dwError = LsaAdBatchGetCompareStringFromPseudoObject(
                    &pszCompare,
                    &pszFreeCompare,
                    QueryType,
                    bIsSchemaMode,
                    dwKeywordValuesCount,
                    ppszKeywordValues,
                    hDirectory,
                    pMessage);
    if (LW_IS_NULL_OR_EMPTY_STR(pszCompare))
    {
        LSA_ASSERT(FALSE);
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_LSA_ERROR(dwError);
    }

    // Get and check object type

    dwError = LsaAdBatchGetObjectTypeFromPseudoMessage(
                    &objectType,
                    bIsSchemaMode,
                    dwKeywordValuesCount,
                    ppszKeywordValues,
                    hDirectory,
                    pMessage);
    BAIL_ON_LSA_ERROR(dwError);

    // Sanity check.
    desiredObjectType = LsaAdBatchGetObjectTypeFromQueryType(QueryType);
    if ((desiredObjectType != LSA_AD_BATCH_OBJECT_TYPE_UNDEFINED) &&
        (objectType != desiredObjectType))
    {
        PCSTR pszType = LsaAdBatchGetQueryTypeAsString(QueryType);
        LSA_LOG_DEBUG("Object type mismatch for %s '%s' - got %u instead of %u",
                      pszType, pszCompare, objectType, desiredObjectType);
        // This cannot happen because we restrict the type we search on.
        LSA_ASSERT(FALSE);
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_LSA_ERROR(dwError);
    }

    // Search of corresponding batch item

    for (pLinks = pStartBatchItemListLinks;
         pLinks != pEndBatchItemListLinks;
         pLinks = pLinks->Next)
    {
        PLSA_AD_BATCH_ITEM pItem = LW_STRUCT_FROM_FIELD(pLinks, LSA_AD_BATCH_ITEM, BatchItemListLinks);

        if (!pItem->pszQueryMatchTerm)
        {
            // There are two possible cases here:
            //
            // 1) This is a linked cell case where we are searching
            //    linked cells since we keep using the main batch
            //    item list (though this case should go away in the
            //    future as we just keep an unresolved batch items list).
            //
            // 2) This is an item for which we did not find a real object.
            //    This case might be eliminated in the future by removing
            //    unresolvable objects from the batch items list.
            continue;
        }

        if (!strcasecmp(pItem->pszQueryMatchTerm, pszCompare))
        {
            pFoundItem = pItem;
            break;
        }
    }

    if (!pFoundItem)
    {
        PCSTR pszType = LsaAdBatchGetQueryTypeAsString(QueryType);
        LSA_LOG_DEBUG("Did not find batch item for message for %s '%s'",
                      pszType, pszCompare);
        LSA_ASSERT(FALSE);
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (bIsGcSearch)
    {
        dwError = LsaAdBatchGatherPseudoObjectSidFromGc(
                        pFoundItem,
                        objectType,
                        dwKeywordValuesCount,
                        ppszKeywordValues,
                        hDirectory,
                        pMessage);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        dwError = LsaAdBatchGatherPseudoObject(
                        pFoundItem,
                        objectType,
                        bIsSchemaMode,
                        dwKeywordValuesCount,
                        ppszKeywordValues,
                        hDirectory,
                        pMessage);
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    LwFreeStringArray(ppszKeywordValues, dwKeywordValuesCount);
    LW_SAFE_FREE_STRING(pszFreeCompare);

    return dwError;

error:
    goto cleanup;
}

static
DWORD
LsaAdBatchProcessPseudoObjectDefaultSchema(
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    // List of PLSA_AD_BATCH_ITEM
    IN OUT PLSA_LIST_LINKS pStartBatchItemListLinks,
    IN PLSA_LIST_LINKS pEndBatchItemListLinks,
    IN HANDLE hDirectory,
    IN LDAPMessage* pMessage
    )
{
    DWORD dwError = 0;
    PSTR pszCompare = NULL;
    LSA_AD_BATCH_OBJECT_TYPE objectType = 0;
    LSA_AD_BATCH_OBJECT_TYPE desiredObjectType = 0;
    PLSA_LIST_LINKS pLinks = NULL;
    PLSA_AD_BATCH_ITEM pFoundItem = NULL;

    // Get compare string
    dwError = LsaAdBatchGetCompareStringFromPseudoObjectDefaultSchema(
                    &pszCompare,
                    QueryType,
                    hDirectory,
                    pMessage);
    BAIL_ON_LSA_ERROR(dwError);

    // Get and check object type
    dwError = LsaAdBatchGetObjectTypeFromRealMessage(
                    &objectType,
                    hDirectory,
                    pMessage);
    BAIL_ON_LSA_ERROR(dwError);

    // Sanity check.
    desiredObjectType = LsaAdBatchGetObjectTypeFromQueryType(QueryType);
    if ((desiredObjectType != LSA_AD_BATCH_OBJECT_TYPE_UNDEFINED) &&
        (objectType != desiredObjectType))
    {
        PCSTR pszType = LsaAdBatchGetQueryTypeAsString(QueryType);
        LSA_LOG_DEBUG("Object type mismatch for %s '%s' - got %u instead of %u",
                      pszType, pszCompare, objectType, desiredObjectType);
        // This cannot happen because we restrict the type we search on.
        LSA_ASSERT(FALSE);
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_LSA_ERROR(dwError);
    }

    // Search of corresponding batch item
    for (pLinks = pStartBatchItemListLinks;
         pLinks != pEndBatchItemListLinks;
         pLinks = pLinks->Next)
    {
        PLSA_AD_BATCH_ITEM pItem = LW_STRUCT_FROM_FIELD(pLinks, LSA_AD_BATCH_ITEM, BatchItemListLinks);

        if (!pItem->pszQueryMatchTerm)
        {
            // There are two possible cases here:
            //
            // 1) This is a linked cell case where we are searching
            //    linked cells since we keep using the main batch
            //    item list (though this case should go away in the
            //    future as we just keep an unresolved batch items list).
            //
            // 2) This is an item for which we did not find a real object.
            //    This case might be eliminated in the future by removing
            //    unresolvable objects from the batch items list.
            continue;
        }



        if (!strcasecmp(pItem->pszQueryMatchTerm, pszCompare))
        {
            pFoundItem = pItem;
            break;
        }
    }

    if (!pFoundItem)
    {
        PCSTR pszType = LsaAdBatchGetQueryTypeAsString(QueryType);
        LSA_LOG_DEBUG("Did not find batch item for message for %s '%s'",
                      pszType, pszCompare);
        LSA_ASSERT(FALSE);
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaAdBatchGatherPseudoObjectDefaultSchema(
                    pFoundItem,
                    objectType,
                    (LSA_AD_BATCH_QUERY_TYPE_BY_SID == QueryType) ? &pszCompare : NULL,
                    hDirectory,
                    pMessage);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    LW_SAFE_FREE_STRING(pszCompare);

    return dwError;

error:
    goto cleanup;
}

static
PCSTR
LsaAdBatchGetQueryTypeAsString(
    IN LSA_AD_BATCH_QUERY_TYPE QueryType
    )
{
    PCSTR pszType = NULL;

    switch (QueryType)
    {
        case LSA_AD_BATCH_QUERY_TYPE_BY_DN:
            pszType = "DN";
            break;
        case LSA_AD_BATCH_QUERY_TYPE_BY_SID:
            pszType = "SID";
            break;
        case LSA_AD_BATCH_QUERY_TYPE_BY_NT4:
            pszType = "NT4 name";
            break;
        case LSA_AD_BATCH_QUERY_TYPE_BY_USER_ALIAS:
            pszType = "user alias";
            break;
        case LSA_AD_BATCH_QUERY_TYPE_BY_GROUP_ALIAS:
            pszType = "group alias";
            break;
        case LSA_AD_BATCH_QUERY_TYPE_BY_UID:
            pszType = "uid";
            break;
        case LSA_AD_BATCH_QUERY_TYPE_BY_GID:
            pszType = "gid";
            break;
        default:
            pszType = "<unknown>";
            break;
    }

    return pszType;
}

static
BOOLEAN
LsaAdBatchGetQueryTypeIsString(
    IN LSA_AD_BATCH_QUERY_TYPE QueryType
    )
{
    BOOLEAN bIsString = FALSE;

    switch (QueryType)
    {
        case LSA_AD_BATCH_QUERY_TYPE_BY_DN:
        case LSA_AD_BATCH_QUERY_TYPE_BY_SID:
        case LSA_AD_BATCH_QUERY_TYPE_BY_NT4:
        case LSA_AD_BATCH_QUERY_TYPE_BY_USER_ALIAS:
        case LSA_AD_BATCH_QUERY_TYPE_BY_GROUP_ALIAS:
            bIsString = TRUE;
            break;
        case LSA_AD_BATCH_QUERY_TYPE_BY_UID:
        case LSA_AD_BATCH_QUERY_TYPE_BY_GID:
            break;
        default:
            break;
    }

    return bIsString;
}

VOID
LsaAdBatchQueryTermDebugInfo(
    IN PLSA_AD_BATCH_QUERY_TERM pQueryTerm,
    OUT OPTIONAL PCSTR* ppszType,
    OUT OPTIONAL PBOOLEAN pbIsString,
    OUT OPTIONAL PCSTR* ppszString,
    OUT OPTIONAL PDWORD pdwId
    )
{
    PCSTR pszType = LsaAdBatchGetQueryTypeAsString(pQueryTerm->Type);
    BOOLEAN bIsString = LsaAdBatchGetQueryTypeIsString(pQueryTerm->Type);
    PCSTR pszString = NULL;
    DWORD dwId = 0;

    if (bIsString)
    {
        pszString = pQueryTerm->pszString;
    }
    else
    {
        dwId = pQueryTerm->dwId;
    }

    if (ppszType)
    {
        *ppszType = pszType;
    }
    if (pbIsString)
    {
        *pbIsString = bIsString;
    }
    if (ppszString)
    {
        *ppszString= pszString;
    }
    if (pdwId)
    {
        *pdwId = dwId;
    }
}

DWORD
LsaAdBatchAccountTypeToObjectType(
    IN LSA_OBJECT_TYPE AccountType,
    OUT PLSA_AD_BATCH_OBJECT_TYPE pObjectType
    )
{
    DWORD dwError = 0;
    LSA_AD_BATCH_OBJECT_TYPE objectType = 0;

    switch (AccountType)
    {
        case LSA_OBJECT_TYPE_USER:
            objectType = LSA_AD_BATCH_OBJECT_TYPE_USER;
            break;
        case LSA_OBJECT_TYPE_GROUP:
            objectType = LSA_AD_BATCH_OBJECT_TYPE_GROUP;
            break;
        default:
            LSA_ASSERT(FALSE);
            dwError = LW_ERROR_INTERNAL;
    }

    *pObjectType = objectType;
    return dwError;
}

LSA_AD_BATCH_OBJECT_TYPE
LsaAdBatchGetObjectTypeFromQueryType(
    IN LSA_AD_BATCH_QUERY_TYPE QueryType
    )
{
    LSA_AD_BATCH_OBJECT_TYPE objectType = LSA_AD_BATCH_OBJECT_TYPE_UNDEFINED;

    switch (QueryType)
    {
        case LSA_AD_BATCH_QUERY_TYPE_BY_UID:
        case LSA_AD_BATCH_QUERY_TYPE_BY_USER_ALIAS:
            objectType = LSA_AD_BATCH_OBJECT_TYPE_USER;
            break;
        case LSA_AD_BATCH_QUERY_TYPE_BY_GID:
        case LSA_AD_BATCH_QUERY_TYPE_BY_GROUP_ALIAS:
            objectType = LSA_AD_BATCH_OBJECT_TYPE_GROUP;
            break;
        default:
            // The default is undefined (i.e., any).
            break;
    }

    return objectType;
}

BOOLEAN
LsaAdBatchHasValidCharsForSid(
    IN PCSTR pszSidString
    )
{
    BOOLEAN bHasOnlyValidChars = TRUE;
    PCSTR pszCurrent = pszSidString;

    while (*pszCurrent)
    {
        if (!(*pszCurrent == '-' ||
              (*pszCurrent == 'S' || *pszCurrent == 's') ||
              (*pszCurrent >= '0' && *pszCurrent <= '9')))
        {
            bHasOnlyValidChars = FALSE;
            break;
        }
        pszCurrent++;
    }
    return bHasOnlyValidChars;
}

