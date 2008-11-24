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
#include "batch_marshal.h"
#include "batch_gather.h"
#include "batch_p.h"

#define DC_PART "dc="

static
PCSTR
ParseDcPart(
    IN PCSTR pszDn
    )
{
    PCSTR pszFound = NULL;

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
GetDomainEntryType(
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
    if (LSA_ERROR_NO_SUCH_DOMAIN == dwError)
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
                    dwError = LSA_ERROR_INVALID_PARAMETER;
                    BAIL_ON_LSA_ERROR(dwError);
            }

            break;

        default:
            dwError = LSA_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
    }

    if (!bSkip)
    {
        bIsExternalTrust = (dwTrustMode == LSA_TRUST_MODE_EXTERNAL) ? TRUE : FALSE;
        dwError = CheckDomainModeCompatibility(pszDomainName, bIsExternalTrust, pszDomainDN);
        if (dwError == LSA_ERROR_INCOMPATIBLE_MODES_BETWEEN_TRUSTEDDOMAINS)
        {
            dwError = 0;
            bSkip = TRUE;
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

static
DWORD
CreateBatchDomainEntry(
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

    switch (QueryType)
    {
        case LSA_AD_BATCH_QUERY_TYPE_BY_DN:
            dwError = LsaLdapConvertDNToDomain(pszMatchTerm,
                                               &pszDnsDomainName);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaDmWrapGetDomainName(pszDnsDomainName,
                                             NULL,
                                             &pszNetbiosDomainName);
            BAIL_ON_LSA_ERROR(dwError);

            pszDcPart = pszMatchTerm;

            break;

        case LSA_AD_BATCH_QUERY_TYPE_BY_SID:
            dwError = LsaDmWrapGetDomainNameAndSidByObjectSid(
                            pszMatchTerm,
                            &pszDnsDomainName,
                            &pszNetbiosDomainName,
                            &pszDomainSid);
            BAIL_ON_LSA_ERROR(dwError);

            break;

        case LSA_AD_BATCH_QUERY_TYPE_BY_NT4:
            dwError = LsaDmWrapGetDomainName(pszMatchTerm,
                                             &pszDnsDomainName,
                                             &pszNetbiosDomainName);
            BAIL_ON_LSA_ERROR(dwError);

            break;

        default:
            dwError = LSA_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = GetDomainEntryType(pszDnsDomainName,
                                 QueryType,
                                 pszDcPart,
                                 &bSkip,
                                 &bIsOneWayTrust);
    BAIL_ON_LSA_ERROR(dwError);


    dwError = LsaAllocateMemory(sizeof(*pEntry), (PVOID*)&pEntry);
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
    }
    if (bIsOneWayTrust)
    {
        SetFlag(pEntry->Flags, LSA_AD_BATCH_DOMAIN_ENTRY_FLAG_IS_ONE_WAY_TRUST);
    }

    if (pszDcPart)
    {
        pEntry->QueryMatch.ByDn.pszDcPart = pszDcPart;
        pszDcPart = NULL;
    }
    else if (pszDomainSid)
    {
        pEntry->QueryMatch.BySid.pszDomainSid = pszDomainSid;
        pszDomainSid = NULL;

        pEntry->QueryMatch.BySid.sDomainSidLength = strlen(pEntry->QueryMatch.BySid.pszDomainSid);
    }

    *ppEntry = pEntry;

cleanup:
    LSA_SAFE_FREE_STRING(pszDnsDomainName);
    LSA_SAFE_FREE_STRING(pszNetbiosDomainName);
    LSA_SAFE_FREE_STRING(pszDomainSid);
    return dwError;

error:
    *ppEntry = NULL;
    DestroyBatchDomainEntry(&pEntry);
    goto cleanup;
}

static
VOID
DestroyBatchDomainEntry(
    IN OUT PLSA_AD_BATCH_DOMAIN_ENTRY* ppEntry
    )
{
    PLSA_AD_BATCH_DOMAIN_ENTRY pEntry = *ppEntry;
    if (pEntry)
    {
        PLSA_LIST_LINKS pLinks = NULL;
        PLSA_LIST_LINKS pNextLinks = NULL;

        LSA_SAFE_FREE_STRING(pEntry->pszDnsDomainName);
        LSA_SAFE_FREE_STRING(pEntry->pszNetbiosDomainName);

        switch (pEntry->QueryType)
        {
            case LSA_AD_BATCH_QUERY_TYPE_BY_SID:
                LSA_SAFE_FREE_STRING(pEntry->QueryMatch.BySid.pszDomainSid);
                break;
        }

        for (pLinks = pEntry->BatchItemList.Next;
             pLinks != &pEntry->BatchItemList;
             pLinks = pNextLinks)
        {
            PLSA_AD_BATCH_ITEM pItem = LW_STRUCT_FROM_FIELD(pLinks, LSA_AD_BATCH_ITEM, BatchItemListLinks);
            pNextLinks = pLinks->Next;
            // No need to remove from list since destroying entire list
            // and moving forward.  However, do it anyhow for ease of
            // debugging.
            LsaListRemove(pLinks);
            DestroyBatchItem(&pItem);
        }
        LsaFreeMemory(pEntry);
        *ppEntry = NULL;
    }
}

static
DWORD
CreateBatchItem(
    OUT PLSA_AD_BATCH_ITEM* ppItem,
    IN PLSA_AD_BATCH_DOMAIN_ENTRY pDomainEntry,
    IN LSA_AD_BATCH_QUERY_TYPE QueryTermType,
    IN OPTIONAL PCSTR pszString,
    IN OPTIONAL PDWORD pdwId
    )
{
    DWORD dwError = 0;
    PLSA_AD_BATCH_ITEM pItem = NULL;

    dwError = LsaAllocateMemory(sizeof(*pItem), (PVOID*)&pItem);
    BAIL_ON_LSA_ERROR(dwError);

    pItem->QueryTerm.Type = QueryTermType;
    if (pszString)
    {
        pItem->QueryTerm.pszString = pszString;
    }
    else if (pdwId)
    {
        pItem->QueryTerm.dwId = *pdwId;
    }
    pItem->pDomainEntry = pDomainEntry;

    *ppItem = pItem;

cleanup:
    return dwError;

error:
    *ppItem = NULL;
    DestroyBatchItem(&pItem);
    goto cleanup;
}

static
VOID
DestroyBatchItem(
    IN OUT PLSA_AD_BATCH_ITEM* ppItem
    )
{
    PLSA_AD_BATCH_ITEM pItem = *ppItem;
    if (pItem)
    {
        LSA_SAFE_FREE_STRING(pItem->pszSid);
        LSA_SAFE_FREE_STRING(pItem->pszSamAccountName);
        LSA_SAFE_FREE_STRING(pItem->pszDn);
        switch (pItem->ObjectType)
        {
            case LSA_AD_BATCH_OBJECT_TYPE_USER:
                LSA_SAFE_FREE_STRING(pItem->UserInfo.pszAlias);
                LSA_SAFE_FREE_STRING(pItem->UserInfo.pszPasswd);
                LSA_SAFE_FREE_STRING(pItem->UserInfo.pszGecos);
                LSA_SAFE_FREE_STRING(pItem->UserInfo.pszHomeDirectory);
                LSA_SAFE_FREE_STRING(pItem->UserInfo.pszShell);
                LSA_SAFE_FREE_STRING(pItem->UserInfo.pszUserPrincipalName);
                break;
            case LSA_AD_BATCH_OBJECT_TYPE_GROUP:
                LSA_SAFE_FREE_STRING(pItem->GroupInfo.pszAlias);
                LSA_SAFE_FREE_STRING(pItem->GroupInfo.pszPasswd);
                break;
        }
        LsaFreeMemory(pItem);
        *ppItem = NULL;
    }
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
            pszMatchTerm = ParseDcPart(pszQueryTerm);
            break;
        case LSA_AD_BATCH_QUERY_TYPE_BY_SID:
            pszMatchTerm = pszQueryTerm;
            break;
        default:
            dwError = LSA_ERROR_INVALID_PARAMETER;
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
            if (!strcasecmp(pEntry->pszNetbiosDomainName, pszMatchTerm) ||
                !strcasecmp(pEntry->pszDnsDomainName, pszMatchTerm))
            {
                bIsMatch = TRUE;
            }
        default:
            dwError = LSA_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
    }

    *pbIsMatch = bIsMatch;

cleanup:
    return dwError;

error:
    *pbIsMatch = FALSE;
    goto cleanup;
}

static
DWORD
GetCellConfigurationMode(
    IN PCSTR pszDnsDomainName,
    IN PCSTR pszCellDN,
    OUT ADConfigurationMode* pAdMode
    )
{
    DWORD dwError = 0;
    HANDLE hDirectory = (HANDLE)NULL;
    ADConfigurationMode adMode = UnknownMode;

    dwError = LsaDmWrapLdapOpenDirectoryDomain(
                  pszDnsDomainName,
                  &hDirectory);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADGetConfigurationMode(
                         hDirectory,
                         pszCellDN,
                         &adMode);
    BAIL_ON_LSA_ERROR(dwError);

    *pAdMode = adMode;

cleanup:
    LsaLdapCloseDirectory(hDirectory);

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

static
DWORD
CheckDomainModeCompatibility(
    IN PCSTR pszDnsDomainName,
    IN BOOLEAN bIsExternalTrust,
    IN OPTIONAL PCSTR pszDomainDN
    )
{
    DWORD dwError = 0;
    HANDLE hDirectory = (HANDLE)NULL;
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
        dwError = LSA_ERROR_INCOMPATIBLE_MODES_BETWEEN_TRUSTEDDOMAINS;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (!pszDomainDnToUse)
    {
        dwError = LsaLdapConvertDomainToDN(pszDnsDomainName,
                                           &pszLocalDomainDn);
        BAIL_ON_LSA_ERROR(dwError);
        pszDomainDnToUse = pszLocalDomainDn;
    }

    dwError = LsaDmWrapLdapOpenDirectoryDomain(
                  pszDnsDomainName,
                  &hDirectory);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAllocateStringPrintf(&pszCellDN,
                    "CN=$LikewiseIdentityCell,%s",
                    pszDomainDnToUse);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADGetConfigurationMode(
                         hDirectory,
                         pszCellDN,
                         &adMode);
    BAIL_ON_LSA_ERROR(dwError);

    if (adMode != gpADProviderData->adConfigurationMode)
    {
        dwError = LSA_ERROR_INCOMPATIBLE_MODES_BETWEEN_TRUSTEDDOMAINS;
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    LsaLdapCloseDirectory(hDirectory);
    LSA_SAFE_FREE_STRING(pszCellDN);
    LSA_SAFE_FREE_STRING(pszLocalDomainDn);

    return dwError;

error:
    goto cleanup;
}

DWORD
ADLdap_FindObjectsByDNListBatched(
    IN HANDLE hProvider,
    IN DWORD dwCount,
    IN PSTR* ppszDnList,
    OUT PDWORD pdwCount,
    OUT PAD_SECURITY_OBJECT** pppObjects
    )
{
    return LsaAdBatchFindObjects(hProvider,
                                 LSA_AD_BATCH_QUERY_TYPE_BY_DN,
                                 dwCount, ppszDnList,
                                 pdwCount, pppObjects);
}

DWORD
ADLdap_FindObjectsBySidListBatched(
    IN HANDLE hProvider,
    IN DWORD dwCount,
    IN PSTR* ppszSidList,
    OUT PDWORD pdwCount,
    OUT PAD_SECURITY_OBJECT** pppObjects
    )
{
    return LsaAdBatchFindObjects(hProvider,
                                 LSA_AD_BATCH_QUERY_TYPE_BY_SID,
                                 dwCount, ppszSidList,
                                 pdwCount, pppObjects);
}

static
DWORD
LsaAdBatchFindObjects(
    IN HANDLE hProvider,
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN DWORD dwQueryItemsCount,
    IN PSTR* ppszQueryList,
    OUT PDWORD pdwObjectsCount,
    OUT PAD_SECURITY_OBJECT** pppObjects
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    DWORD dwObjectsCount = 0;
    PAD_SECURITY_OBJECT* ppObjects = NULL;
    DWORD dwCurrentIndex = 0;
    LSA_LIST_LINKS DomainList = { 0 };
    PLSA_LIST_LINKS pLinks = NULL;
    PLSA_LIST_LINKS pNextLinks = NULL;

    LsaListInit(&DomainList);

    for (i = 0; i < dwQueryItemsCount; i++)
    {
        PCSTR pszMatchTerm = NULL;
        PLSA_AD_BATCH_DOMAIN_ENTRY pFoundEntry = NULL;

        dwError = LsaAdBatchGetDomainMatchTerm(
                        QueryType,
                        ppszQueryList[i],
                        &pszMatchTerm);
        BAIL_ON_LSA_ERROR(dwError);

        for (pLinks = DomainList.Next;
             pLinks != &DomainList;
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
            dwError = CreateBatchDomainEntry(
                            &pFoundEntry,
                            QueryType,
                            pszMatchTerm);
            if (LSA_ERROR_NO_SUCH_DOMAIN == dwError)
            {
                LSA_LOG_DEBUG("Domain not found for query item - '%s'", ppszQueryList[i]);
                continue;
            }
            BAIL_ON_LSA_ERROR(dwError);

            LsaListInsertTail(&DomainList, &pFoundEntry->DomainEntryListLinks);
        }

        if (!IsSetFlag(pFoundEntry->Flags, LSA_AD_BATCH_DOMAIN_ENTRY_FLAG_SKIP))
        {
            PLSA_AD_BATCH_ITEM pBatchItem = NULL;

            dwError = CreateBatchItem(
                            &pBatchItem,
                            pFoundEntry,
                            QueryType,
                            ppszQueryList[i],
                            NULL);
            BAIL_ON_LSA_ERROR(dwError);

            LsaListInsertTail(&pFoundEntry->BatchItemList, &pBatchItem->BatchItemListLinks);

            pFoundEntry->dwBatchItemCount++;
        }

        pFoundEntry = NULL;
    }

    for (pLinks = DomainList.Next;
         pLinks != &DomainList;
         pLinks = pLinks->Next)
    {
        PLSA_AD_BATCH_DOMAIN_ENTRY pEntry = LW_STRUCT_FROM_FIELD(pLinks, LSA_AD_BATCH_DOMAIN_ENTRY, DomainEntryListLinks);

        if (IsSetFlag(pEntry->Flags, LSA_AD_BATCH_DOMAIN_ENTRY_FLAG_SKIP))
        {
            continue;
        }

        dwError = AD_FindObjectsByListForDomain(
                      hProvider,
                      QueryType,
                      pEntry);
        BAIL_ON_LSA_ERROR(dwError);

        dwObjectsCount += pEntry->dwBatchItemCount;
    }

    dwError = LsaAllocateMemory(
                dwObjectsCount * sizeof(*ppObjects),
                (PVOID*)&ppObjects);
    BAIL_ON_LSA_ERROR(dwError);

    // Combine results
    dwCurrentIndex = 0;
    for (pLinks = DomainList.Next;
         pLinks != &DomainList;
         pLinks = pLinks->Next)
    {
        PLSA_AD_BATCH_DOMAIN_ENTRY pEntry = LW_STRUCT_FROM_FIELD(pLinks, LSA_AD_BATCH_DOMAIN_ENTRY, DomainEntryListLinks);
        DWORD dwDomainObjectsCount = 0;

        if (IsSetFlag(pEntry->Flags, LSA_AD_BATCH_DOMAIN_ENTRY_FLAG_SKIP))
        {
            continue;
        }

        dwError = LsaAdBatchMarshalList(
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
        PAD_SECURITY_OBJECT* ppTempObjects = NULL;

        dwError = LsaAllocateMemory(
                    dwCurrentIndex * sizeof(*ppTempObjects),
                    (PVOID*)&ppTempObjects);
        BAIL_ON_LSA_ERROR(dwError);

        memcpy(ppTempObjects, ppObjects, sizeof(*ppObjects) * dwCurrentIndex);
        LsaFreeMemory(ppObjects);
        ppObjects = ppTempObjects;
        dwObjectsCount = dwCurrentIndex;
    }

    *pdwObjectsCount = dwObjectsCount;
    *pppObjects = ppObjects;

cleanup:
    for (pLinks = DomainList.Next;
         pLinks != &DomainList;
         pLinks = pNextLinks)
    {
        PLSA_AD_BATCH_DOMAIN_ENTRY pEntry = LW_STRUCT_FROM_FIELD(pLinks, LSA_AD_BATCH_DOMAIN_ENTRY, DomainEntryListLinks);
        pNextLinks = pLinks->Next;
        DestroyBatchDomainEntry(&pEntry);
    }

    return dwError;

error:
    *pdwObjectsCount = 0;
    *pppObjects = NULL;

    ADCacheDB_SafeFreeObjectList(dwObjectsCount, &ppObjects);
    goto cleanup;
}

static
DWORD
AD_FindObjectsByListForDomain(
    IN HANDLE hProvider,
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN OUT PLSA_AD_BATCH_DOMAIN_ENTRY pEntry
    )
{
    return AD_FindObjectsByListForDomainInternal(
                hProvider,
                QueryType,
                pEntry->pszDnsDomainName,
                IsSetFlag(pEntry->Flags, LSA_AD_BATCH_DOMAIN_ENTRY_FLAG_IS_ONE_WAY_TRUST),
                pEntry->dwBatchItemCount,
                &pEntry->BatchItemList);
}

static
DWORD
AD_FindObjectsByListForDomainInternal(
    IN HANDLE hProvider,
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN PCSTR pszDnsDomainName,
    IN BOOLEAN bIsOneWayTrust,
    IN DWORD dwCount,
    IN OUT PLSA_LIST_LINKS pBatchItemList
    )
{
    DWORD dwError = 0;

    if (bIsOneWayTrust && LSA_AD_BATCH_QUERY_TYPE_BY_DN == QueryType)
    {
        // This should never happen, this domain should already be skipped.
        LSA_ASSERT(FALSE);
        dwError = LSA_ERROR_INTERNAL;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (bIsOneWayTrust && LSA_AD_BATCH_QUERY_TYPE_BY_SID == QueryType)
    {
        dwError = LsaAdBatchResolveRpcObjects(
                        QueryType,
                        pszDnsDomainName,
                        dwCount,
                        pBatchItemList);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        dwError = LsaAdBatchResolveRealObjects(
                        hProvider,
                        QueryType,
                        pszDnsDomainName,
                        dwCount,
                        pBatchItemList);
        BAIL_ON_LSA_ERROR(dwError);
    }

    // If Default schema, real objects already have the pseudo information
    // If Unprovisioned mode, no need to get PseudoMessages.
    if (!LsaAdBatchIsDefaultSchemaMode() &&
        !LsaAdBatchIsUnprovisionedMode())
    {
        dwError = LsaAdBatchResolvePseudoObjects(
                        hProvider,
                        QueryType,
                        pszDnsDomainName,
                        dwCount,
                        pBatchItemList);
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

static
DWORD
AD_GetMaxQuerySize(
    VOID
    )
{
    return BUILD_BATCH_QUERY_MAX_SIZE;
}

static
DWORD
AD_GetMaxQueryCount(
    VOID
    )
{
    return BUILD_BATCH_QUERY_MAX_COUNT;
}

static
DWORD
LsaAdBatchBuildQueryForRpc(
    // List of PLSA_AD_BATCH_ITEM
    IN PLSA_LIST_LINKS pFirstLinks,
    IN PLSA_LIST_LINKS pEndLinks,
    OUT PLSA_LIST_LINKS* ppNextLinks,
    IN DWORD dwMaxQueryCount,
    OUT PDWORD pdwQueryCount,
    OUT PSTR** pppszQueryList
    )
{
    DWORD dwError = 0;
    PLSA_LIST_LINKS pLinks = NULL;
    PSTR* ppszQueryList = NULL;
    PLSA_LIST_LINKS pLastLinks = pFirstLinks;
    DWORD dwQueryCount = 0;
    DWORD dwSavedQueryCount = 0;

    pLinks = pFirstLinks;
    for (pLinks = pFirstLinks; pLinks != pEndLinks; pLinks = pLinks->Next)
    {
        PLSA_AD_BATCH_ITEM pEntry = LW_STRUCT_FROM_FIELD(pLinks, LSA_AD_BATCH_ITEM, BatchItemListLinks);

        if (!IsNullOrEmptyString(pEntry->QueryTerm.pszString))
        {
            DWORD dwNewQueryCount = dwQueryCount + 1;

            if (dwMaxQueryCount && (dwNewQueryCount > dwMaxQueryCount))
            {
                break;
            }
            dwQueryCount = dwNewQueryCount;
        }
    }
    pLastLinks = pLinks;
    dwSavedQueryCount = dwQueryCount;

    if (dwQueryCount < 1)
    {
        goto cleanup;
    }

    dwError = LsaAllocateMemory(dwQueryCount*sizeof(*ppszQueryList), (PVOID*)&ppszQueryList);
    BAIL_ON_LSA_ERROR(dwError);

    dwQueryCount = 0;
    // Loop until we reach last links.
    for (pLinks = pFirstLinks; pLinks != pLastLinks; pLinks = pLinks->Next)
    {
        PLSA_AD_BATCH_ITEM pEntry = LW_STRUCT_FROM_FIELD(pLinks, LSA_AD_BATCH_ITEM, BatchItemListLinks);

        if (!IsNullOrEmptyString(pEntry->QueryTerm.pszString))
        {
            dwError = LsaAllocateString(pEntry->QueryTerm.pszString,
                                        &ppszQueryList[dwQueryCount]);
            BAIL_ON_LSA_ERROR(dwError);
            dwQueryCount++;
        }
    }

    assert(dwSavedQueryCount == dwQueryCount);

cleanup:
    // We handle error here instead of error label
    // because there is a goto cleanup above.
    if (dwError)
    {
        LsaFreeStringArray(ppszQueryList, dwSavedQueryCount);
        dwQueryCount = 0;
        dwSavedQueryCount = 0;
        pLastLinks = pFirstLinks;
    }

    *pppszQueryList = ppszQueryList;
    *pdwQueryCount = dwQueryCount;
    *ppNextLinks = pLastLinks;

    return dwError;

error:
    // Do not actually handle any errors here.
    goto cleanup;
}

static
DWORD
LsaAdBatchResolveRpcObjects(
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN PCSTR pszDnsDomainName,
    IN DWORD dwTotalItemCount,
    // List of PLSA_AD_BATCH_ITEM
    IN OUT PLSA_LIST_LINKS pBatchItemList
    )
{
    DWORD dwError = 0;
    DWORD dwMaxQueryCount = AD_GetMaxQueryCount();
    PSTR* ppszSidList = NULL;
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

        LsaFreeStringArray(ppszSidList, dwQueryCount);
        ppszSidList = NULL;

        if (ppTranslatedNames)
        {
            LsaFreeTranslatedNameList(ppTranslatedNames, dwCount);
        }
        ppTranslatedNames = NULL;

        dwError = LsaAdBatchBuildQueryForRpc(
                        pLinks,
                        pBatchItemList,
                        &pNextLinks,
                        dwMaxQueryCount,
                        &dwQueryCount,
                        &ppszSidList);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaDmWrapNetLookupNamesByObjectSids(
                       gpADProviderData->szDomain,
                       dwQueryCount,
                       ppszSidList,
                       &ppTranslatedNames,
                       &dwCount);
        BAIL_ON_LSA_ERROR(dwError);

        if (dwCount > dwQueryCount)
        {
            LSA_LOG_ERROR("Too many results returned (got %u, expected %u)",
                          dwCount, dwQueryCount);
            dwError = LSA_ERROR_RPC_ERROR;
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

            dwError = LsaAdBatchProcessRpcObject(
                            QueryType,
                            pLinks,
                            pNextLinks,
                            ppszSidList[i],
                            ppTranslatedNames[i]);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

cleanup:
    LsaFreeStringArray(ppszSidList, dwQueryCount);
    if (ppTranslatedNames)
    {
        LsaFreeTranslatedNameList(ppTranslatedNames, dwCount);
    }
    return dwError;

error:
    goto cleanup;
}

static
DWORD
LsaAdBatchResolveRealObjects(
    IN HANDLE hProvider,
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN PCSTR pszDnsDomainName,
    IN DWORD dwTotalItemCount,
    // List of PLSA_AD_BATCH_ITEM
    IN OUT PLSA_LIST_LINKS pBatchItemList
    )
{
    DWORD dwError = 0;
    HANDLE hDirectory = 0;
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
    DWORD dwMaxQuerySize = AD_GetMaxQuerySize();
    DWORD dwMaxQueryCount = AD_GetMaxQueryCount();

    dwError = LsaLdapConvertDomainToDN(
                       pszDnsDomainName,
                       &pszScopeDn);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaDmWrapLdapOpenDirectoryDomain(pszDnsDomainName,
                                               &hDirectory);
    BAIL_ON_LSA_ERROR(dwError);

    pLd = LsaLdapGetSession(hDirectory);

    for (pLinks = pBatchItemList->Next;
         pLinks != pBatchItemList;
         pLinks = pNextLinks)
    {
        DWORD dwQueryCount = 0;
        DWORD dwCount = 0;
        LDAPMessage* pCurrentMessage = NULL;

        pNextLinks = NULL;

        LSA_SAFE_FREE_STRING(pszQuery);
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

        dwError = LsaLdapDirectorySearch(
                        hDirectory,
                        pszScopeDn,
                        LDAP_SCOPE_SUBTREE,
                        pszQuery,
                        szAttributeList,
                        &pMessage);
        BAIL_ON_LSA_ERROR(dwError);

        dwCount = ldap_count_entries(pLd, pMessage);
        if (dwCount > dwQueryCount)
        {
            LSA_LOG_ERROR("Too many results returned (got %u, expected %u)",
                          dwCount, dwQueryCount);
            dwError = LSA_ERROR_LDAP_ERROR;
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
    LsaLdapCloseDirectory(hDirectory);
    LSA_SAFE_FREE_STRING(pszScopeDn);
    LSA_SAFE_FREE_STRING(pszQuery);
    if (pMessage)
    {
        ldap_msgfree(pMessage);
    }
    return dwError;

error:
    goto cleanup;
}

typedef DWORD (*LSA_AD_BATCH_BUILDER_GET_ATTR_VALUE_CALLBACK)(
    IN PVOID pCallbackContext,
    IN PVOID pItem,
    OUT PCSTR* ppszValue,
    OUT PVOID* ppFreeValueContext
    );

typedef VOID (*LSA_AD_BATCH_BUILDER_FREE_VALUE_CONTEXT_CALLBACK)(
    IN PVOID pCallbackContext,
    IN OUT PVOID* ppFreeValueContext
    );

typedef PVOID (*LSA_AD_BATCH_BUILDER_NEXT_ITEM_CALLBACK)(
    IN PVOID pCallbackContext,
    IN PVOID pItem
    );

static
DWORD
LsaAdBatchBuilderAppend(
    IN OUT PDWORD pdwQueryOffset,
    IN OUT PSTR pszQuery,
    IN DWORD dwQuerySize,
    IN PCSTR pszAppend,
    IN DWORD dwAppendLength
    )
{
    DWORD dwError = 0;
    DWORD dwQueryOffset = *pdwQueryOffset;
    DWORD dwNewQueryOffset = 0;

    if (dwAppendLength > 0)
    {
        dwNewQueryOffset = dwQueryOffset + dwAppendLength;
        if (dwNewQueryOffset < dwQueryOffset)
        {
            // overflow
            dwError = LSA_ERROR_DATA_ERROR;
            BAIL_ON_LSA_ERROR(dwError);
        }
        else if (dwNewQueryOffset - 1 >= dwQuerySize)
        {
            // overflow
            dwError = LSA_ERROR_DATA_ERROR;
            BAIL_ON_LSA_ERROR(dwError);
        }
        memcpy(pszQuery + dwQueryOffset, pszAppend, dwAppendLength);
        pszQuery[dwNewQueryOffset] = 0;
        *pdwQueryOffset = dwNewQueryOffset;
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

static
DWORD
LsaAdBatchBuilderCreateQuery(
    IN PCSTR pszQueryPrefix,
    IN PCSTR pszQuerySuffix,
    IN PCSTR pszAttributeName,
    IN PVOID pFirstItem,
    IN PVOID pEndItem,
    OUT PVOID* ppNextItem,
    IN OPTIONAL PVOID pCallbackContext,
    IN LSA_AD_BATCH_BUILDER_GET_ATTR_VALUE_CALLBACK pGetAttributeValueCallback,
    IN OPTIONAL LSA_AD_BATCH_BUILDER_FREE_VALUE_CONTEXT_CALLBACK pFreeValueContextCallback,
    IN LSA_AD_BATCH_BUILDER_NEXT_ITEM_CALLBACK pNextItemCallback,
    IN DWORD dwMaxQuerySize,
    IN DWORD dwMaxQueryCount,
    OUT PDWORD pdwQueryCount,
    OUT PSTR* ppszQuery
    )
{
    DWORD dwError = 0;
    PVOID pCurrentItem = NULL;
    PSTR pszQuery = NULL;
    PVOID pLastItem = pFirstItem;
    const char szOrPrefix[] = "(|";
    const char szOrSuffix[] = ")";
    const DWORD dwOrPrefixLength = sizeof(szOrPrefix)-1;
    const DWORD dwOrSuffixLength = sizeof(szOrSuffix)-1;
    DWORD dwAttributeNameLength = strlen(pszAttributeName);
    DWORD dwQuerySize = 0;
    DWORD dwQueryCount = 0;
    PVOID pFreeValueContext = NULL;
    DWORD dwQueryPrefixLength = 0;
    DWORD dwQuerySuffixLength = 0;
    DWORD dwQueryOffset = 0;
    DWORD dwSavedQueryCount = 0;

    if (pszQueryPrefix)
    {
        dwQueryPrefixLength = strlen(pszQueryPrefix);
    }
    if (pszQuerySuffix)
    {
        dwQuerySuffixLength = strlen(pszQuerySuffix);
    }

    // The overhead is:
    // prefix + orPrefix + <CONTENT> + orSuffix + suffix + NULL
    dwQuerySize = dwQueryPrefixLength + dwOrPrefixLength + dwOrSuffixLength + dwQuerySuffixLength + 1;

    pCurrentItem = pFirstItem;
    while (pCurrentItem != pEndItem)
    {
        PCSTR pszAttributeValue = NULL;

        if (pFreeValueContext)
        {
            pFreeValueContextCallback(pCallbackContext, &pFreeValueContext);
        }

        dwError = pGetAttributeValueCallback(
                        pCallbackContext,
                        pCurrentItem,
                        &pszAttributeValue,
                        &pFreeValueContext);
        BAIL_ON_LSA_ERROR(dwError);

        if (pszAttributeValue)
        {
            // "(" + attributeName + "=" + attributeValue + ")"
            DWORD dwAttributeValueLength = strlen(pszAttributeValue);
            DWORD dwItemLength = (1 + dwAttributeNameLength + 1 + dwAttributeValueLength + 1);
            DWORD dwNewQuerySize = dwQuerySize + dwItemLength;
            DWORD dwNewQueryCount = dwQueryCount + 1;

            if (dwNewQuerySize < dwQuerySize)
            {
                // overflow
                dwError = LSA_ERROR_DATA_ERROR;
                BAIL_ON_LSA_ERROR(dwError);
            }
            if (dwMaxQuerySize && (dwNewQuerySize > dwMaxQuerySize))
            {
                break;
            }
            if (dwMaxQueryCount && (dwNewQueryCount > dwMaxQueryCount))
            {
                break;
            }
            dwQuerySize = dwNewQuerySize;
            dwQueryCount = dwNewQueryCount;
        }

        pCurrentItem = pNextItemCallback(pCallbackContext, pCurrentItem);
    }
    pLastItem = pCurrentItem;
    dwSavedQueryCount = dwQueryCount;

    if (dwQueryCount < 1)
    {
        goto cleanup;
    }

    dwError = LsaAllocateMemory(dwQuerySize, (PVOID*)&pszQuery);
    BAIL_ON_LSA_ERROR(dwError);

    // Set up the query
    dwQueryOffset = 0;

    dwError = LsaAdBatchBuilderAppend(&dwQueryOffset, pszQuery, dwQuerySize,
                                      pszQueryPrefix, dwQueryPrefixLength);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAdBatchBuilderAppend(&dwQueryOffset, pszQuery, dwQuerySize,
                                      szOrPrefix, dwOrPrefixLength);
    BAIL_ON_LSA_ERROR(dwError);

    dwQueryCount = 0;
    pCurrentItem = pFirstItem;
    while (pCurrentItem != pLastItem)
    {
        PCSTR pszAttributeValue = NULL;

        if (pFreeValueContext)
        {
            pFreeValueContextCallback(pCallbackContext, &pFreeValueContext);
        }

        dwError = pGetAttributeValueCallback(
                        pCallbackContext,
                        pCurrentItem,
                        &pszAttributeValue,
                        &pFreeValueContext);
        BAIL_ON_LSA_ERROR(dwError);

        if (pszAttributeValue)
        {
            DWORD dwAttributeValueLength = strlen(pszAttributeValue);

            dwError = LsaAdBatchBuilderAppend(&dwQueryOffset, pszQuery, dwQuerySize,
                                              "(", 1);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaAdBatchBuilderAppend(&dwQueryOffset, pszQuery, dwQuerySize,
                                              pszAttributeName, dwAttributeNameLength);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaAdBatchBuilderAppend(&dwQueryOffset, pszQuery, dwQuerySize,
                                              "=", 1);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaAdBatchBuilderAppend(&dwQueryOffset, pszQuery, dwQuerySize,
                                              pszAttributeValue, dwAttributeValueLength);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaAdBatchBuilderAppend(&dwQueryOffset, pszQuery, dwQuerySize,
                                              ")", 1);
            BAIL_ON_LSA_ERROR(dwError);

            dwQueryCount++;
        }

        pCurrentItem = pNextItemCallback(pCallbackContext, pCurrentItem);
    }

    dwError = LsaAdBatchBuilderAppend(&dwQueryOffset, pszQuery, dwQuerySize,
                                      szOrSuffix, dwOrSuffixLength);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAdBatchBuilderAppend(&dwQueryOffset, pszQuery, dwQuerySize,
                                      pszQuerySuffix, dwQuerySuffixLength);
    BAIL_ON_LSA_ERROR(dwError);

    assert(dwQueryOffset + 1 == dwQuerySize);
    assert(dwSavedQueryCount == dwQueryCount);

cleanup:
    // We handle error here instead of error label
    // because there is a goto cleanup above.
    if (dwError)
    {
        LSA_SAFE_FREE_STRING(pszQuery);
        dwQueryCount = 0;
        pLastItem = pFirstItem;
    }

    if (pFreeValueContext)
    {
        pFreeValueContextCallback(pCallbackContext, &pFreeValueContext);
    }

    *ppszQuery = pszQuery;
    *pdwQueryCount = dwQueryCount;
    *ppNextItem = pLastItem;

    return dwError;

error:
    // Do not actually handle any errors here.
    goto cleanup;
}

static
VOID
LsaAdBatchBuilderGenericFreeValueContext(
    IN PVOID pCallbackContext,
    IN OUT PVOID* ppFreeValueContext
    )
{
    LSA_SAFE_FREE_MEMORY(*ppFreeValueContext);
}

typedef struct _LSA_AD_BATCH_BUILDER_BATCH_ITEM_CONTEXT {
    LSA_AD_BATCH_QUERY_TYPE QueryType;
    BOOLEAN bIsForRealObject;
    // Buffer needs to contain 32-bit unsigned number in decimal.
    // That's 10 digits plus a terminating NULL.
    char szIdBuffer[11];
} LSA_AD_BATCH_BUILDER_BATCH_ITEM_CONTEXT, *PLSA_AD_BATCH_BUILDER_BATCH_ITEM_CONTEXT;

static
DWORD
LsaAdBatchBuilderBatchItemGetAttributeValue(
    IN PVOID pCallbackContext,
    IN PVOID pItem,
    OUT PCSTR* ppszValue,
    OUT PVOID* ppFreeValueContext
    )
{
    DWORD dwError = 0;
    PLSA_AD_BATCH_BUILDER_BATCH_ITEM_CONTEXT pContext = (PLSA_AD_BATCH_BUILDER_BATCH_ITEM_CONTEXT)pCallbackContext;
    LSA_AD_BATCH_QUERY_TYPE QueryType = pContext->QueryType;
    BOOLEAN bIsForRealObject = pContext->bIsForRealObject;
    PLSA_LIST_LINKS pLinks = (PLSA_LIST_LINKS)pItem;
    PLSA_AD_BATCH_ITEM pBatchItem = LW_STRUCT_FROM_FIELD(pLinks, LSA_AD_BATCH_ITEM, BatchItemListLinks);
    PSTR pszValueToEscape = NULL;
    PSTR pszValue = NULL;
    PVOID pFreeValueContext = NULL;
    BOOLEAN bHaveReal = IsSetFlag(pBatchItem->Flags, LSA_AD_BATCH_ITEM_FLAG_HAVE_REAL);
    BOOLEAN bHavePseudo = IsSetFlag(pBatchItem->Flags, LSA_AD_BATCH_ITEM_FLAG_HAVE_PSEUDO);

    if ((bIsForRealObject && bHaveReal) ||
        (!bIsForRealObject && bHavePseudo))
    {
        // This can only happen in the linked cells case, but even
        // that should go away in the future as we just keep an
        // unresolved batch items list.
        LSA_ASSERT(!bIsForRealObject && (gpADProviderData->dwDirectoryMode == CELL_MODE));
        goto cleanup;
    }

    switch (QueryType)
    {
        case LSA_AD_BATCH_QUERY_TYPE_BY_DN:
            // Must be looking for real object
            LSA_ASSERT(bIsForRealObject);
            LSA_ASSERT(QueryType == pBatchItem->QueryTerm.Type);

            pszValueToEscape = (PSTR)pBatchItem->QueryTerm.pszString;
            LSA_ASSERT(pszValueToEscape);
            break;

        case LSA_AD_BATCH_QUERY_TYPE_BY_SID:
            if (pBatchItem->pszSid)
            {
                // This is case where we already got the SID by resolving
                // the pseudo object by id/alias.
                pszValue = pBatchItem->pszSid;
            }
            else if (QueryType == pBatchItem->QueryTerm.Type)
            {
                // This is the case where the original query was
                // a query by SID.
                pszValue = (PSTR)pBatchItem->QueryTerm.pszString;
                LSA_ASSERT(pszValue);
            }
            // Will be NULL if we cannot find a SID for which to query.
            // This can happen if this batch item is for an object
            // that we did not find real but are trying to look up pseudo.
            // In that case, we have NULL and will just skip it.

            // If we have a SID string, make sure it looks like a SID.
            // Note that we must do some sanity checking to make sure
            // that the string does not need escaping since we are
            // not setting pszValueToEscape.  (The SID check takes
            // care of that.)
            if (pszValue && !LsaAdBatchHasValidCharsForSid(pszValue))
            {
                LSA_ASSERT(FALSE);
                dwError = LSA_ERROR_INTERNAL;
                BAIL_ON_LSA_ERROR(dwError);
            }
            break;

        case LSA_AD_BATCH_QUERY_TYPE_BY_NT4:
            LSA_ASSERT(bIsForRealObject);
            if (pBatchItem->pszSamAccountName)
            {
                // Unprovisioned id/alias case where id mapper returned
                // a SAM account name (and domain name) but no SID.
                pszValueToEscape = pBatchItem->pszSamAccountName;
                // However, we currently do not have this sort of id mapper
                // support, so we LSA_ASSERT(FALSE) for now.
                LSA_ASSERT(FALSE);
            }
            else if (QueryType == pBatchItem->QueryTerm.Type)
            {
                pszValueToEscape = (PSTR)pBatchItem->QueryTerm.pszString;
                LSA_ASSERT(pszValueToEscape);
                // The query term will already have the domain stripped out.
                LSA_ASSERT(!index(pszValueToEscape, '\\'));
            }
            break;

        case LSA_AD_BATCH_QUERY_TYPE_BY_UID:
        case LSA_AD_BATCH_QUERY_TYPE_BY_GID:
            LSA_ASSERT(!bIsForRealObject);
            LSA_ASSERT(QueryType == pBatchItem->QueryTerm.Type);

            snprintf(pContext->szIdBuffer, sizeof(pContext->szIdBuffer),
                     "%u", (unsigned int)pBatchItem->QueryTerm.dwId);
            // There should have been enough space for the NULL termination.
            LSA_ASSERT(!pContext->szIdBuffer[sizeof(pContext->szIdBuffer) - 1]);
            pszValue = pContext->szIdBuffer;
            break;

        case LSA_AD_BATCH_QUERY_TYPE_BY_USER_ALIAS:
        case LSA_AD_BATCH_QUERY_TYPE_BY_GROUP_ALIAS:
            LSA_ASSERT(!bIsForRealObject);
            LSA_ASSERT(QueryType == pBatchItem->QueryTerm.Type);

            pszValueToEscape = (PSTR)pBatchItem->QueryTerm.pszString;
            LSA_ASSERT(pszValueToEscape);
            break;

        default:
            dwError = LSA_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
    }

    if (pszValueToEscape)
    {
        LSA_ASSERT(!pszValue);
        dwError = LsaLdapEscapeString(&pszValue, pszValueToEscape);
        BAIL_ON_LSA_ERROR(dwError);

        pFreeValueContext = pszValue;
    }

cleanup:
    // Note that the match value is different from the value,
    // which we need to escape.
    pBatchItem->pszQueryMatchTerm = pszValueToEscape ? pszValueToEscape : pszValue;
    *ppszValue = pszValue;
    *ppFreeValueContext = pFreeValueContext;

    return dwError;

error:
    // assing output in cleanup in case of goto cleanup in function.
    pszValueToEscape = NULL;
    pszValue = NULL;
    LSA_SAFE_FREE_STRING(pFreeValueContext);
    goto cleanup;
}

static
PVOID
LsaAdBatchBuilderBatchItemNextItem(
    IN PVOID pCallbackContext,
    IN PVOID pItem
    )
{
    PLSA_LIST_LINKS pLinks = (PLSA_LIST_LINKS)pItem;
    return pLinks->Next;
}

static
DWORD
LsaAdBatchBuildQueryForReal(
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    // List of PLSA_AD_BATCH_ITEM
    IN PLSA_LIST_LINKS pFirstLinks,
    IN PLSA_LIST_LINKS pEndLinks,
    OUT PLSA_LIST_LINKS* ppNextLinks,
    IN DWORD dwMaxQuerySize,
    IN DWORD dwMaxQueryCount,
    OUT PDWORD pdwQueryCount,
    OUT PSTR* ppszQuery
    )
{
    DWORD dwError = 0;
    PLSA_LIST_LINKS pNextLinks = NULL;
    DWORD dwQueryCount = 0;
    PSTR pszQuery = NULL;
    PCSTR pszAttributeName = NULL;
    PCSTR pszPrefix = NULL;
    LSA_AD_BATCH_BUILDER_BATCH_ITEM_CONTEXT context = { 0 };

    switch (QueryType)
    {
        case LSA_AD_BATCH_QUERY_TYPE_BY_DN:
            pszAttributeName = AD_LDAP_DN_TAG;
            break;
        case LSA_AD_BATCH_QUERY_TYPE_BY_SID:
            pszAttributeName = AD_LDAP_OBJECTSID_TAG;
            break;
        case LSA_AD_BATCH_QUERY_TYPE_BY_NT4:
            pszAttributeName = AD_LDAP_SAM_NAME_TAG;
            break;
        default:
            dwError = LSA_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
    }

    // In Default/schema case, filter out disabled users
    // when querying real objects.
    if ((gpADProviderData->dwDirectoryMode == DEFAULT_MODE) &&
        (gpADProviderData->adConfigurationMode == SchemaMode))
    {
        pszPrefix = "(&(|(&(objectClass=user)(uidNumber=*))(objectClass=group))(!(objectClass=computer))";
    }
    else
    {
        pszPrefix = "(&(|(objectClass=user)(objectClass=group))(!(objectClass=computer))";
    }

    context.QueryType = QueryType;
    context.bIsForRealObject = FALSE;

    dwError = LsaAdBatchBuilderCreateQuery(
                    pszPrefix,
                    ")",
                    pszAttributeName,
                    pFirstLinks,
                    pEndLinks,
                    (PVOID*)&pNextLinks,
                    &context,
                    LsaAdBatchBuilderBatchItemGetAttributeValue,
                    LsaAdBatchBuilderGenericFreeValueContext,
                    LsaAdBatchBuilderBatchItemNextItem,
                    dwMaxQuerySize,
                    dwMaxQueryCount,
                    &dwQueryCount,
                    &pszQuery);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    *ppNextLinks = pNextLinks;
    *pdwQueryCount = dwQueryCount;
    *ppszQuery = pszQuery;
    return dwError;

error:
    // set output on cleanup
    pNextLinks = pFirstLinks;
    dwQueryCount = 0;
    LSA_SAFE_FREE_STRING(pszQuery);
    goto cleanup;
}

#define AD_LDAP_QUERY_LW_USER  "(keywords=objectClass=centerisLikewiseUser)"
#define AD_LDAP_QUERY_LW_GROUP "(keywords=objectClass=centerisLikewiseGroup)"
#define AD_LDAP_QUERY_SCHEMA_USER "(objectClass=posixAccount)"
#define AD_LDAP_QUERY_SCHEMA_GROUP "(objectClass=posixGroup)"
#define AD_LDAP_QUERY_NON_SCHEMA "(objectClass=serviceConnectionPoint)"

static
PCSTR
LsaAdBatchBuilderGetPseudoQueryPrefix(
    IN BOOLEAN bIsSchemaMode,
    IN LSA_AD_BATCH_OBJECT_TYPE ObjectType,
    OUT PCSTR* ppszSuffix
    )
{
    PCSTR pszPrefix = NULL;

    if (bIsSchemaMode)
    {
        switch (ObjectType)
        {
            case LSA_AD_BATCH_OBJECT_TYPE_USER:
                pszPrefix =
                    "(&"
                    "(&" AD_LDAP_QUERY_SCHEMA_USER AD_LDAP_QUERY_LW_USER ")"
                    "";
                break;
            case LSA_AD_BATCH_OBJECT_TYPE_GROUP:
                pszPrefix =
                    "(&"
                    "(&" AD_LDAP_QUERY_SCHEMA_GROUP AD_LDAP_QUERY_LW_GROUP ")"
                    "";
                break;
            default:
                pszPrefix =
                    "(&"
                    "(|"
                    "(&" AD_LDAP_QUERY_SCHEMA_USER AD_LDAP_QUERY_LW_USER ")"
                    "(&" AD_LDAP_QUERY_SCHEMA_GROUP AD_LDAP_QUERY_LW_GROUP ")"
                    ")"
                    "";
        }
    }
    else
    {
        switch (ObjectType)
        {
            case LSA_AD_BATCH_OBJECT_TYPE_USER:
                pszPrefix =
                    "(&"
                    AD_LDAP_QUERY_NON_SCHEMA
                    AD_LDAP_QUERY_LW_USER
                    "";
                break;
            case LSA_AD_BATCH_OBJECT_TYPE_GROUP:
                pszPrefix =
                    "(&"
                    AD_LDAP_QUERY_NON_SCHEMA
                    AD_LDAP_QUERY_LW_GROUP
                    "";
                break;
            default:
                pszPrefix =
                    "(&"
                    AD_LDAP_QUERY_NON_SCHEMA
                    "(|"
                    AD_LDAP_QUERY_LW_USER
                    AD_LDAP_QUERY_LW_GROUP
                    ")"
                    "";
        }
    }

    *ppszSuffix = pszPrefix ? ")" : NULL;
    return pszPrefix;
}

static
PCSTR
LsaAdBatchBuilderGetPseudoQueryAttribute(
    IN BOOLEAN bIsSchemaMode,
    IN LSA_AD_BATCH_QUERY_TYPE QueryType
    )
{
    PCSTR pszAttributeName = NULL;

    switch (QueryType)
    {
        case LSA_AD_BATCH_QUERY_TYPE_BY_SID:
            pszAttributeName = "keywords=backLink";
            break;
        case LSA_AD_BATCH_QUERY_TYPE_BY_UID:
            if (bIsSchemaMode)
            {
                pszAttributeName = AD_LDAP_UID_TAG;
            }
            else
            {
                pszAttributeName = "keywords=" AD_LDAP_UID_TAG;
            }
            break;
        case LSA_AD_BATCH_QUERY_TYPE_BY_GID:
            if (bIsSchemaMode)
            {
                pszAttributeName = AD_LDAP_GID_TAG;
            }
            else
            {
                pszAttributeName = "keywords=" AD_LDAP_GID_TAG;
            }
            break;
        case LSA_AD_BATCH_QUERY_TYPE_BY_USER_ALIAS:
            if (bIsSchemaMode)
            {
                pszAttributeName = AD_LDAP_ALIAS_TAG;
            }
            else
            {
                pszAttributeName = "keywords=" AD_LDAP_ALIAS_TAG;
            }
            break;
        case LSA_AD_BATCH_QUERY_TYPE_BY_GROUP_ALIAS:
            if (bIsSchemaMode)
            {
                pszAttributeName = AD_LDAP_DISPLAY_NAME_TAG;
            }
            else
            {
                pszAttributeName = "keywords=" AD_LDAP_DISPLAY_NAME_TAG;
            }
            break;
    }

    return pszAttributeName;
}

static
DWORD
LsaAdBatchBuildQueryForPseudo(
    IN BOOLEAN bIsSchemaMode,
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    // List of PLSA_AD_BATCH_ITEM
    IN PLSA_LIST_LINKS pFirstLinks,
    IN PLSA_LIST_LINKS pEndLinks,
    OUT PLSA_LIST_LINKS* ppNextLinks,
    IN DWORD dwMaxQuerySize,
    IN DWORD dwMaxQueryCount,
    OUT PDWORD pdwQueryCount,
    OUT PSTR* ppszQuery
    )
{
    DWORD dwError = 0;
    PLSA_LIST_LINKS pNextLinks = NULL;
    DWORD dwQueryCount = 0;
    PSTR pszQuery = NULL;
    PCSTR pszAttributeName = NULL;
    PCSTR pszPrefix = NULL;
    PCSTR pszSuffix = NULL;
    LSA_AD_BATCH_BUILDER_BATCH_ITEM_CONTEXT context = { 0 };

    pszAttributeName = LsaAdBatchBuilderGetPseudoQueryAttribute(
                            bIsSchemaMode,
                            QueryType);
    if (!pszAttributeName)
    {
        LSA_ASSERT(FALSE);
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pszPrefix = LsaAdBatchBuilderGetPseudoQueryPrefix(
                        bIsSchemaMode,
                        LsaAdBatchGetObjectTypeFromQueryType(QueryType),
                        &pszSuffix);
    if (!pszPrefix || !pszSuffix)
    {
        LSA_ASSERT(FALSE);
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    context.QueryType = QueryType;
    context.bIsForRealObject = FALSE;

    dwError = LsaAdBatchBuilderCreateQuery(
                    pszPrefix,
                    pszSuffix,
                    pszAttributeName,
                    pFirstLinks,
                    pEndLinks,
                    (PVOID*)&pNextLinks,
                    &context,
                    LsaAdBatchBuilderBatchItemGetAttributeValue,
                    LsaAdBatchBuilderGenericFreeValueContext,
                    LsaAdBatchBuilderBatchItemNextItem,
                    dwMaxQuerySize,
                    dwMaxQueryCount,
                    &dwQueryCount,
                    &pszQuery);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    *ppNextLinks = pNextLinks;
    *pdwQueryCount = dwQueryCount;
    *ppszQuery = pszQuery;
    return dwError;

error:
    // set output on cleanup
    pNextLinks = pFirstLinks;
    dwQueryCount = 0;
    LSA_SAFE_FREE_STRING(pszQuery);
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
                dwError = LSA_ERROR_INTERNAL;
                BAIL_ON_LSA_ERROR(dwError);
            }

            dwError = LsaLdapConvertDomainToDN(pszDnsDomainName, &pszDcPart);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaAllocateStringPrintf(&pszScopeDn, "CN=$LikewiseIdentityCell,%s", pszDcPart);
            BAIL_ON_LSA_ERROR(dwError);
            break;

        case LSA_PROVISIONING_MODE_NON_DEFAULT_CELL:
            dwError = LsaAllocateString(pszCellDn, &pszScopeDn);
            BAIL_ON_LSA_ERROR(dwError);
            break;

        default:
            // ASSERT
            LSA_LOG_ERROR("Unexpected mode %u", Mode);
            dwError = LSA_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

    *ppszScopeDn = pszScopeDn;

cleanup:
    LSA_SAFE_FREE_STRING(pszDcPart);
    return dwError;

error:
    *ppszScopeDn = NULL;
    LSA_SAFE_FREE_STRING(pszScopeDn);
    goto cleanup;
}

static
DWORD
LsaAdBatchProcessRpcObject(
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    // List of PLSA_AD_BATCH_ITEM
    IN OUT PLSA_LIST_LINKS pStartBatchItemListLinks,
    IN PLSA_LIST_LINKS pEndBatchItemListLinks,
    IN PSTR pszObjectSid,
    IN PLSA_TRANSLATED_NAME_OR_SID pTranslatedName
    )
{
    DWORD dwError = 0;
    PSTR pszSid = NULL;
    ADAccountType accountType = 0;
    PLSA_LIST_LINKS pLinks = NULL;
    PCSTR pszCompare = NULL;
    PLSA_LOGIN_NAME_INFO pLoginNameInfo = NULL;

    dwError = LsaAllocateString(pszObjectSid, &pszSid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaCrackDomainQualifiedName(
                         pTranslatedName->pszNT4NameOrSid,
                         gpADProviderData->szDomain,
                         &pLoginNameInfo);
    BAIL_ON_LSA_ERROR(dwError);

    switch (QueryType)
    {
        case LSA_AD_BATCH_QUERY_TYPE_BY_SID:
            pszCompare = pszSid;
            break;
        default:
            LSA_ASSERT(FALSE);
            dwError = LSA_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
    }

    accountType = pTranslatedName->ObjectType;
    if ((accountType != AccountType_User) &&
        (accountType != AccountType_Group))
    {
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    for (pLinks = pStartBatchItemListLinks;
         pLinks != pEndBatchItemListLinks;
         pLinks = pLinks->Next)
    {
        PLSA_AD_BATCH_ITEM pItem = LW_STRUCT_FROM_FIELD(pLinks, LSA_AD_BATCH_ITEM, BatchItemListLinks);

        LSA_ASSERT(LSA_AD_BATCH_QUERY_TYPE_BY_SID == pItem->QueryTerm.Type);

        if (!strcasecmp(pItem->QueryTerm.pszString, pszCompare))
        {
            dwError = LsaAdBatchAccountTypeToObjectType(
                            accountType,
                            &pItem->ObjectType);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaAdBatchGatherRpcObject(
                            pItem,
                            pItem->ObjectType,
                            &pszSid,
                            &pLoginNameInfo->pszName);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        }
    }

cleanup:
    LSA_SAFE_FREE_STRING(pszSid);
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
    PSTR pszDn = NULL;
    PSTR pszSid = NULL;
    PCSTR pszCompare = NULL;
    ADAccountType accountType = 0;
    PLSA_LIST_LINKS pLinks = NULL;

    dwError = LsaLdapGetDN(hDirectory, pMessage, &pszDn);
    BAIL_ON_LSA_ERROR(dwError);

    if (IsNullOrEmptyString(pszDn))
    {
        dwError = LSA_ERROR_LDAP_FAILED_GETDN;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = ADLdap_GetObjectSid(hDirectory, pMessage, &pszSid);
    BAIL_ON_LSA_ERROR(dwError);

    switch (QueryType)
    {
        case LSA_AD_BATCH_QUERY_TYPE_BY_DN:
            pszCompare = pszDn;
            break;
        case LSA_AD_BATCH_QUERY_TYPE_BY_SID:
            pszCompare = pszSid;
            break;
        default:
            LSA_ASSERT(FALSE);
            dwError = LSA_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = ADLdap_GetAccountType(hDirectory, pMessage, &accountType);
    BAIL_ON_LSA_ERROR(dwError);

    if (accountType != AccountType_User && accountType != AccountType_Group)
    {
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    for (pLinks = pStartBatchItemListLinks;
         pLinks != pEndBatchItemListLinks;
         pLinks = pLinks->Next)
    {
        PLSA_AD_BATCH_ITEM pItem = LW_STRUCT_FROM_FIELD(pLinks, LSA_AD_BATCH_ITEM, BatchItemListLinks);

        XXX; // may want to just skip it no query term...hmm...
        LSA_ASSERT(pItem->pszQueryMatchTerm);

        if (!strcasecmp(pItem->pszQueryMatchTerm, pszCompare))
        {
            dwError = LsaAdBatchAccountTypeToObjectType(
                            accountType,
                            &pItem->ObjectType);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaAdBatchGatherRealObject(
                            pItem,
                            pItem->ObjectType,
                            &pszSid,
                            hDirectory,
                            pMessage);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        }
    }

cleanup:
    LSA_SAFE_FREE_STRING(pszDn);
    LSA_SAFE_FREE_STRING(pszSid);

    return dwError;

error:
    goto cleanup;
}

static
DWORD
LsaAdBatchResolvePseudoObjects(
    IN HANDLE hProvider,
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN PCSTR pszDnsDomainName,
    IN DWORD dwTotalItemCount,
    // List of PLSA_AD_BATCH_ITEM
    IN OUT PLSA_LIST_LINKS pBatchItemList
    )
{
    DWORD dwError = 0;

    if (gpADProviderData->dwDirectoryMode == CELL_MODE)
    {
        dwError = LsaAdBatchResolvePseudoObjectsWithLinkedCells(
                     hProvider,
                     QueryType,
                     dwTotalItemCount,
                     pBatchItemList);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        dwError = LsaAdBatchResolvePseudoObjectsInternal(
                     hProvider,
                     QueryType,
                     pszDnsDomainName,
                     NULL,
                     (gpADProviderData->adConfigurationMode == SchemaMode),
                     pBatchItemList,
                     NULL,
                     NULL);
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

static
DWORD
LsaAdBatchResolvePseudoObjectsWithLinkedCells(
    IN HANDLE hProvider,
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN DWORD dwTotalItemCount,
    // List of PLSA_AD_BATCH_ITEM
    IN OUT PLSA_LIST_LINKS pBatchItemList
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    PDLINKEDLIST pCellNode = NULL;
    DWORD dwTotalFoundCount = 0;
    DWORD dwFoundInCellCount = 0;
    HANDLE hDirectory = 0;

    dwError = LsaAdBatchResolvePseudoObjectsInternal(
                    hProvider,
                    QueryType,
                    NULL,
                    gpADProviderData->cell.szCellDN,
                    (gpADProviderData->adConfigurationMode == SchemaMode),
                    pBatchItemList,
                    &dwFoundInCellCount,
                    &hDirectory);
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
        dwError = GetCellConfigurationMode(
                       gpADProviderData->szDomain,
                       pCellInfo->pszCellDN,
                       &adMode);
        BAIL_ON_LSA_ERROR(dwError);

        if (adMode == UnknownMode)
        {
            continue;
        }

        dwError = LsaAdBatchResolvePseudoObjectsInternal(
                    hProvider,
                    QueryType,
                    NULL,
                    pCellInfo->pszCellDN,
                    (adMode == SchemaMode),
                    pBatchItemList,
                    &dwFoundInCellCount,
                    &hDirectory);
        BAIL_ON_LSA_ERROR(dwError);

        dwTotalFoundCount += dwFoundInCellCount;
    }

cleanup:
    LsaLdapCloseDirectory(hDirectory);
    return dwError;

error:
    goto cleanup;
}

static
DWORD
LsaAdBatchResolvePseudoObjectsInternal(
    IN HANDLE hProvider,
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN OPTIONAL PCSTR pszDnsDomainName,
    IN OPTIONAL PCSTR pszCellDn,
    IN BOOLEAN bIsSchemaMode,
    // List of PLSA_AD_BATCH_ITEM
    IN OUT PLSA_LIST_LINKS pBatchItemList,
    OUT OPTIONAL PDWORD pdwTotalItemFoundCount,
    IN OUT OPTIONAL PHANDLE phDirectory
    )
{
    DWORD dwError = 0;
    HANDLE hDirectory = phDirectory ? *phDirectory : 0;
    LDAP* pLd = NULL;
    PSTR pszScopeDn = NULL;
    PSTR pszQuery = 0;
    PSTR szAttributeList[] =
    {
        // non-schema mode:
        AD_LDAP_KEYWORDS_TAG,
        // schema mode:
        // - group alias:
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
    DWORD dwMaxQuerySize = AD_GetMaxQuerySize();
    DWORD dwMaxQueryCount = AD_GetMaxQueryCount();
    PSTR pszDomainName = NULL;
    DWORD dwTotalItemFoundCount = 0;

    dwError = LsaAdBatchBuildQueryScopeForPseudo(
                    bIsSchemaMode,
                    gpADProviderData->dwDirectoryMode,
                    pszDnsDomainName,
                    pszCellDn,
                    &pszScopeDn);
    BAIL_ON_LSA_ERROR(dwError);

    if (!hDirectory)
    {
        // Need to do this because the pseudo-object domain
        // could be different from the real object's domain
        // (e.g., non-default cell).
        dwError = LsaLdapConvertDNToDomain(
                           pszScopeDn,
                           &pszDomainName);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaDmWrapLdapOpenDirectoryDomain(pszDomainName,
                                                   &hDirectory);
        BAIL_ON_LSA_ERROR(dwError);
    }

    pLd = LsaLdapGetSession(hDirectory);

    for (pLinks = pBatchItemList->Next;
         pLinks != pBatchItemList;
         pLinks = pNextLinks)
    {
        DWORD dwQueryCount = 0;
        DWORD dwCount = 0;
        LDAPMessage* pCurrentMessage = NULL;

        pNextLinks = NULL;

        LSA_SAFE_FREE_STRING(pszQuery);
        if (pMessage)
        {
            ldap_msgfree(pMessage);
            pMessage = NULL;
        }

        dwError = LsaAdBatchBuildQueryForPseudo(
                        bIsSchemaMode,
                        QueryType,
                        pLinks,
                        pBatchItemList,
                        &pNextLinks,
                        dwMaxQuerySize,
                        dwMaxQueryCount,
                        &dwQueryCount,
                        &pszQuery);
        BAIL_ON_LSA_ERROR(dwError);

        if (IsNullOrEmptyString(pszQuery))
        {
            break;
        }

        dwError = LsaLdapDirectorySearch(
                        hDirectory,
                        pszScopeDn,
                        LDAP_SCOPE_SUBTREE,
                        pszQuery,
                        szAttributeList,
                        &pMessage);
        BAIL_ON_LSA_ERROR(dwError);

        dwCount = ldap_count_entries(pLd, pMessage);
        if (dwCount > dwQueryCount)
        {
            LSA_LOG_ERROR("Too many results returned (got %u, expected %u)",
                          dwCount, dwQueryCount);
            dwError = LSA_ERROR_LDAP_ERROR;
            BAIL_ON_LSA_ERROR(dwError);
        }
        else if (dwCount == 0)
        {
            continue;
        }

        dwTotalItemFoundCount += dwCount;

        pCurrentMessage = ldap_first_entry(pLd, pMessage);
        while (pCurrentMessage)
        {
            dwError = LsaAdBatchProcessPseudoObject(
                            QueryType,
                            pLinks,
                            pNextLinks,
                            bIsSchemaMode,
                            hDirectory,
                            pCurrentMessage);
            BAIL_ON_LSA_ERROR(dwError);

            pCurrentMessage = ldap_next_entry(pLd, pCurrentMessage);
        }
    }

    if (pdwTotalItemFoundCount)
    {
       *pdwTotalItemFoundCount = dwTotalItemFoundCount;
    }

    if (phDirectory)
    {
        *phDirectory = hDirectory;
        hDirectory = 0;
    }

cleanup:
    LsaLdapCloseDirectory(hDirectory);
    LSA_SAFE_FREE_STRING(pszDomainName);
    LSA_SAFE_FREE_STRING(pszScopeDn);
    LSA_SAFE_FREE_STRING(pszQuery);
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
    if (phDirectory)
    {
        *phDirectory = 0;
    }

    goto cleanup;
}

#if 0
// Will need this for pseudo...
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
#endif

static
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

#define LsaAdBatchFindKeywordAttributeStatic(Count, Keywords, StaticString) \
    LsaAdBatchFindKeywordAttributeWithEqual(Count, Keywords, StaticString "=", sizeof(StaticString "=") - 1)

static
DWORD
LsaAdBatchProcessPseudoObject(
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    // List of PLSA_AD_BATCH_ITEM
    IN OUT PLSA_LIST_LINKS pStartBatchItemListLinks,
    IN PLSA_LIST_LINKS pEndBatchItemListLinks,
    IN BOOLEAN bIsSchemaMode,
    IN HANDLE hDirectory,
    IN LDAPMessage* pMessage
    )
{
    DWORD dwError = 0;
    BOOLEAN bIsValid = FALSE;
    PSTR pszDn = NULL;
    PSTR pszSid = NULL;
    PLSA_LIST_LINKS pLinks = NULL;
    PSTR* ppszKeywordValues = NULL;
    DWORD dwKeywordValuesCount = 0;
    PCSTR pszObjectSid = NULL;

    dwError = LsaLdapIsValidADEntry(
                    hDirectory,
                    pMessage,
                    &bIsValid);
    BAIL_ON_LSA_ERROR(dwError);

    if (!bIsValid)
    {
        dwError = LSA_ERROR_LDAP_FAILED_GETDN;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaLdapGetStrings(
                    hDirectory,
                    pMessage,
                    AD_LDAP_KEYWORDS_TAG,
                    &ppszKeywordValues,
                    &dwKeywordValuesCount);
    BAIL_ON_LSA_ERROR(dwError);

    pszObjectSid = LsaAdBatchFindKeywordAttributeStatic(
                        dwKeywordValuesCount,
                        ppszKeywordValues,
                        "backLink");
    if (IsNullOrEmptyString(pszObjectSid))
    {
        dwError = LSA_ERROR_INVALID_SID;
        BAIL_ON_LSA_ERROR(dwError);
    }

    XXX; // Add code to get object type info from pseudo so we can verify it below.

    for (pLinks = pStartBatchItemListLinks;
         pLinks != pEndBatchItemListLinks;
         pLinks = pLinks->Next)
    {
        PLSA_AD_BATCH_ITEM pItem = LW_STRUCT_FROM_FIELD(pLinks, LSA_AD_BATCH_ITEM, BatchItemListLinks);

        if (LSA_AD_BATCH_OBJECT_TYPE_UNDEFINED == pItem->ObjectType)
        {
            continue;
        }

        if (strcasecmp(pItem->pszSid, pszObjectSid))
        {
            continue;
        }

        dwError = LsaAdBatchGatherPseudoObject(
                        pItem,
                        pszObjectSid,
                        pItem->ObjectType,
                        bIsSchemaMode,
                        bIsSchemaMode ? 0 : dwKeywordValuesCount,
                        bIsSchemaMode ? NULL : ppszKeywordValues,
                        hDirectory,
                        pMessage);
        BAIL_ON_LSA_ERROR(dwError);
        break;
    }

cleanup:
    LSA_SAFE_FREE_STRING(pszDn);
    LSA_SAFE_FREE_STRING(pszSid);
    LsaFreeStringArray(ppszKeywordValues, dwKeywordValuesCount);

    return dwError;

error:
    goto cleanup;
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
    PCSTR pszType = NULL;
    BOOLEAN bIsString = FALSE;
    PCSTR pszString = NULL;
    DWORD dwId = 0;

    switch (pQueryTerm->Type)
    {
        case LSA_AD_BATCH_QUERY_TYPE_BY_DN:
            pszType = "by DN";
            bIsString = TRUE;
            break;
        case LSA_AD_BATCH_QUERY_TYPE_BY_SID:
            pszType = "by SID";
            bIsString = TRUE;
            break;
        case LSA_AD_BATCH_QUERY_TYPE_BY_NT4:
            pszType = "by NT4 name";
            bIsString = TRUE;
            break;
        case LSA_AD_BATCH_QUERY_TYPE_BY_USER_ALIAS:
            pszType = "by user alias";
            bIsString = TRUE;
            break;
        case LSA_AD_BATCH_QUERY_TYPE_BY_GROUP_ALIAS:
            pszType = "by group alias";
            bIsString = TRUE;
            break;
        case LSA_AD_BATCH_QUERY_TYPE_BY_UID:
            pszType = "by uid";
            break;
        case LSA_AD_BATCH_QUERY_TYPE_BY_GID:
            pszType = "by gid";
            break;
        default:
            pszType = "by <unknown>";
            break;
    }

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

static
DWORD
LsaAdBatchAccountTypeToObjectType(
    IN ADAccountType AccountType,
    OUT PLSA_AD_BATCH_OBJECT_TYPE pObjectType
    )
{
    DWORD dwError = 0;
    LSA_AD_BATCH_OBJECT_TYPE objectType = 0;

    switch (AccountType)
    {
        case AccountType_User:
            objectType = LSA_AD_BATCH_OBJECT_TYPE_USER;
            break;
        case AccountType_Group:
            objectType = LSA_AD_BATCH_OBJECT_TYPE_GROUP;
            break;
        default:
            LSA_ASSERT(FALSE);
            dwError = LSA_ERROR_INTERNAL;
    }

    *pObjectType = objectType;
    return dwError;
}

static
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

static
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

