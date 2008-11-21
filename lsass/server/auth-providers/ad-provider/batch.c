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
#include "batch_p.h"

#define SetFlag(Variable, Flags)   ((Variable) |= (Flags))
#define ClearFlag(Variable, Flags) ((Variable) &= ~(Flags))
#define IsSetFlag(Variable, Flags) (((Variable) & (Flags)) != 0)

#define LSA_XFER_STRING(Source, Target) \
    ((Target) = (Source), (Source) = NULL)

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

static
BOOLEAN
LsaAdBatchIsDefaultSchemaMode(
    VOID
    )
{
    return ((DEFAULT_MODE == gpADProviderData->dwDirectoryMode) &&
            (SchemaMode == gpADProviderData->adConfigurationMode));
}

static
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

DWORD
CreateObjectLoginNameInfo(
    OUT PLSA_LOGIN_NAME_INFO* ppLoginNameInfo,
    IN PCSTR pszDnsDomainName,
    IN PCSTR pszSamAccountName,
    IN PCSTR pszSid
    )
{
    DWORD dwError = 0;
    PLSA_LOGIN_NAME_INFO pLoginNameInfo = NULL;

    dwError = LsaAllocateMemory(
                    sizeof(LSA_LOGIN_NAME_INFO),
                    (PVOID*)&pLoginNameInfo);
    BAIL_ON_LSA_ERROR(dwError);

    pLoginNameInfo->nameType = NameType_NT4;

    dwError = LsaAllocateString(
                    pszDnsDomainName,
                    &pLoginNameInfo->pszFullDomainName);

    dwError = LsaDmWrapGetDomainName(
                    pszDnsDomainName,
                    NULL,
                    &pLoginNameInfo->pszDomainNetBiosName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAllocateString(pszSamAccountName, &pLoginNameInfo->pszName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAllocateString(pszSid, &pLoginNameInfo->pszObjectSid);
    BAIL_ON_LSA_ERROR(dwError);

    *ppLoginNameInfo = pLoginNameInfo;

cleanup:
    return dwError;

error:
    *ppLoginNameInfo = NULL;

    LSA_SAFE_FREE_LOGIN_NAME_INFO(pLoginNameInfo);
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
        dwError = AD_ResolveRealObjectsByListRpc(
                        QueryType,
                        pszDnsDomainName,
                        dwCount,
                        pBatchItemList);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        dwError = AD_ResolveRealObjectsByList(
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
        dwError = AD_ResolvePseudoObjectsByRealObjects(
                        hProvider,
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
AD_BuildBatchQueryForRealRpc(
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
AD_ResolveRealObjectsByListRpc(
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

        dwError = AD_BuildBatchQueryForRealRpc(
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

            dwError = AD_RecordRealObjectFromRpc(
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
AD_ResolveRealObjectsByList(
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

        dwError = AD_BuildBatchQueryForReal(
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
            dwError = AD_RecordRealObject(
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

typedef DWORD (*LSA_AD_BUILD_BATCH_QUERY_GET_ATTR_VALUE_CALLBACK)(
    IN PVOID pCallbackContext,
    IN PVOID pItem,
    OUT PCSTR* ppszValue,
    OUT PVOID* ppFreeContext
    );

typedef VOID (*LSA_AD_BUILD_BATCH_QUERY_FREE_CONTEXT_CALLBACK)(
    IN PVOID pCallbackContext,
    IN OUT PVOID* ppFreeContext
    );

typedef PVOID (*LSA_AD_BUILD_BATCH_QUERY_NEXT_ITEM_CALLBACK)(
    IN PVOID pCallbackContext,
    IN PVOID pItem
    );

static
DWORD
AD_BuildBatchQueryAppend(
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
AD_BuildBatchQuery(
    IN PCSTR pszQueryPrefix,
    IN PCSTR pszQuerySuffix,
    IN PCSTR pszAttributeName,
    IN PVOID pFirstItem,
    IN PVOID pEndItem,
    OUT PVOID* ppNextItem,
    IN OPTIONAL PVOID pCallbackContext,
    IN LSA_AD_BUILD_BATCH_QUERY_GET_ATTR_VALUE_CALLBACK pGetAttributeValueCallback,
    IN OPTIONAL LSA_AD_BUILD_BATCH_QUERY_FREE_CONTEXT_CALLBACK pFreeContextCallback,
    IN LSA_AD_BUILD_BATCH_QUERY_NEXT_ITEM_CALLBACK pNextItemCallback,
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
    PVOID pFreeContext = NULL;
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

        if (pFreeContext)
        {
            pFreeContextCallback(pCallbackContext, &pFreeContext);
        }

        dwError = pGetAttributeValueCallback(
                        pCallbackContext,
                        pCurrentItem,
                        &pszAttributeValue,
                        &pFreeContext);
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

    dwError = AD_BuildBatchQueryAppend(&dwQueryOffset, pszQuery, dwQuerySize,
                                       pszQueryPrefix, dwQueryPrefixLength);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_BuildBatchQueryAppend(&dwQueryOffset, pszQuery, dwQuerySize,
                                       szOrPrefix, dwOrPrefixLength);
    BAIL_ON_LSA_ERROR(dwError);

    dwQueryCount = 0;
    pCurrentItem = pFirstItem;
    while (pCurrentItem != pLastItem)
    {
        PCSTR pszAttributeValue = NULL;

        if (pFreeContext)
        {
            pFreeContextCallback(pCallbackContext, &pFreeContext);
        }

        dwError = pGetAttributeValueCallback(
                        pCallbackContext,
                        pCurrentItem,
                        &pszAttributeValue,
                        &pFreeContext);
        BAIL_ON_LSA_ERROR(dwError);

        if (pszAttributeValue)
        {
            DWORD dwAttributeValueLength = strlen(pszAttributeValue);

            dwError = AD_BuildBatchQueryAppend(&dwQueryOffset, pszQuery, dwQuerySize,
                                               "(", 1);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = AD_BuildBatchQueryAppend(&dwQueryOffset, pszQuery, dwQuerySize,
                                               pszAttributeName, dwAttributeNameLength);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = AD_BuildBatchQueryAppend(&dwQueryOffset, pszQuery, dwQuerySize,
                                               "=", 1);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = AD_BuildBatchQueryAppend(&dwQueryOffset, pszQuery, dwQuerySize,
                                               pszAttributeValue, dwAttributeValueLength);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = AD_BuildBatchQueryAppend(&dwQueryOffset, pszQuery, dwQuerySize,
                                               ")", 1);
            BAIL_ON_LSA_ERROR(dwError);

            dwQueryCount++;
        }

        pCurrentItem = pNextItemCallback(pCallbackContext, pCurrentItem);
    }

    dwError = AD_BuildBatchQueryAppend(&dwQueryOffset, pszQuery, dwQuerySize,
                                       szOrSuffix, dwOrSuffixLength);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_BuildBatchQueryAppend(&dwQueryOffset, pszQuery, dwQuerySize,
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

    if (pFreeContext)
    {
        pFreeContextCallback(pCallbackContext, &pFreeContext);
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
AD_BuildBatchQueryLsaFreeMemoryFreeContext(
    IN PVOID pCallbackContext,
    IN OUT PVOID* ppFreeContext
    )
{
    LSA_SAFE_FREE_MEMORY(*ppFreeContext);
}

static
DWORD
AD_BuildBatchQueryFromBatchItemListGetQueryValueForReal(
    IN PVOID pCallbackContext,
    IN PVOID pItem,
    OUT PCSTR* ppszValue,
    OUT PVOID* ppFreeContext
    )
{
    DWORD dwError = 0;
    LSA_AD_BATCH_QUERY_TYPE QueryType = *(PLSA_AD_BATCH_QUERY_TYPE)pCallbackContext;
    PLSA_LIST_LINKS pLinks = (PLSA_LIST_LINKS)pItem;
    PLSA_AD_BATCH_ITEM pBatchItem = LW_STRUCT_FROM_FIELD(pLinks, LSA_AD_BATCH_ITEM, BatchItemListLinks);
    PSTR pszValue = NULL;
    PVOID pFreeContext = NULL;

    switch (QueryType)
    {
        case LSA_AD_BATCH_QUERY_TYPE_BY_DN:
            pBatchItem->pszQueryMatchTerm = pBatchItem->QueryTerm.pszString;

            dwError = LsaLdapEscapeString(&pszValue, pBatchItem->QueryTerm.pszString);
            BAIL_ON_LSA_ERROR(dwError);

            pFreeContext = pszValue;

            break;

        case LSA_AD_BATCH_QUERY_TYPE_BY_SID:
            if (pBatchItem->pszSid)
            {
                pszValue = pBatchItem->pszSid;
            }
            else if (LSA_AD_BATCH_QUERY_TYPE_BY_SID == pBatchItem->QueryTerm.Type)
            {
                pszValue = (PSTR)pBatchItem->QueryTerm.pszString;
            }
            pBatchItem->pszQueryMatchTerm = pszValue;
            // Will be NULL if we cannot find a SID for which to query.
            // This is fine as it will cause us to skip this item.
            break;

        default:
            pBatchItem->pszQueryMatchTerm = NULL;
            dwError = LSA_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
    }

    *ppszValue = pszValue;
    *ppFreeContext = pFreeContext;

cleanup:
    return dwError;

error:
    *ppszValue = NULL;
    *ppFreeContext = NULL;

    LSA_SAFE_FREE_STRING(pszValue);
    goto cleanup;
}

static
DWORD
AD_BuildBatchQueryFromBatchItemListGetSidValueForPseudo(
    IN PVOID pCallbackContext,
    IN PVOID pItem,
    OUT PCSTR* ppszValue,
    OUT PVOID* ppFreeContext
    )
{
    PLSA_LIST_LINKS pLinks = (PLSA_LIST_LINKS)pItem;
    PLSA_AD_BATCH_ITEM pBatchItem = LW_STRUCT_FROM_FIELD(pLinks, LSA_AD_BATCH_ITEM, BatchItemListLinks);

    *ppszValue = IsSetFlag(pBatchItem->Flags, LSA_AD_BATCH_ITEM_FLAG_HAVE_PSEUDO) ? NULL : pBatchItem->pszSid;
    pBatchItem->pszQueryMatchTerm = *ppszValue;
    *ppFreeContext = NULL;

    return 0;
}

static
PVOID
AD_BuildBatchQueryFromBatchItemListNextItem(
    IN PVOID pCallbackContext,
    IN PVOID pItem
    )
{
    PLSA_LIST_LINKS pLinks = (PLSA_LIST_LINKS)pItem;
    return pLinks->Next;
}

static
DWORD
AD_BuildBatchQueryForReal(
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
    PSTR pszPrefix = NULL;
    PSTR pszAttributeName = NULL;

    switch (QueryType)
    {
        case LSA_AD_BATCH_QUERY_TYPE_BY_DN:
            pszAttributeName = "distinguishedName";
            break;
        case LSA_AD_BATCH_QUERY_TYPE_BY_SID:
            pszAttributeName = "objectSid";
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

    dwError = AD_BuildBatchQuery(
                    pszPrefix,
                    ")",
                    pszAttributeName,
                    pFirstLinks,
                    pEndLinks,
                    (PVOID*)ppNextLinks,
                    &QueryType,
                    AD_BuildBatchQueryFromBatchItemListGetQueryValueForReal,
                    AD_BuildBatchQueryLsaFreeMemoryFreeContext,
                    AD_BuildBatchQueryFromBatchItemListNextItem,
                    dwMaxQuerySize,
                    dwMaxQueryCount,
                    pdwQueryCount,
                    ppszQuery);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    return dwError;

error:
    *ppNextLinks = pFirstLinks;
    *pdwQueryCount = 0;
    *ppszQuery = NULL;
    goto cleanup;
}

#define AD_LDAP_QUERY_LW_USER  "(keywords=objectClass=centerisLikewiseUser)"
#define AD_LDAP_QUERY_LW_GROUP "(keywords=objectClass=centerisLikewiseGroup)"
#define AD_LDAP_QUERY_SCHEMA_USER "(objectClass=posixAccount)"
#define AD_LDAP_QUERY_SCHEMA_GROUP "(objectClass=posixGroup)"
#define AD_LDAP_QUERY_NON_SCHEMA "(objectClass=serviceConnectionPoint)"

static
DWORD
AD_BuildBatchQueryForPseudoBySid(
    IN BOOLEAN bIsSchemaMode,
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
    PCSTR pszPrefix = NULL;

    if (bIsSchemaMode)
    {
        pszPrefix =
            "(&"
            "(|"
            "(&" AD_LDAP_QUERY_SCHEMA_USER AD_LDAP_QUERY_LW_USER ")"
            "(&" AD_LDAP_QUERY_SCHEMA_GROUP AD_LDAP_QUERY_LW_GROUP ")"
            ")";
    }
    else
    {
        pszPrefix =
            "(&"
            AD_LDAP_QUERY_NON_SCHEMA
            "(|"
            AD_LDAP_QUERY_LW_USER
            AD_LDAP_QUERY_LW_GROUP
            ")";
    }

    dwError = AD_BuildBatchQuery(
                    pszPrefix,
                    ")",
                    "keywords=backLink",
                    pFirstLinks,
                    pEndLinks,
                    (PVOID*)ppNextLinks,
                    NULL,
                    AD_BuildBatchQueryFromBatchItemListGetSidValueForPseudo,
                    NULL,
                    AD_BuildBatchQueryFromBatchItemListNextItem,
                    dwMaxQuerySize,
                    dwMaxQueryCount,
                    pdwQueryCount,
                    ppszQuery);
    return dwError;
}

static
DWORD
AD_BuildBatchQueryScopeForPseudoBySid(
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
LsaAdBatchGatherRpcObject(
    IN OUT PLSA_AD_BATCH_ITEM pItem,
    IN LSA_AD_BATCH_OBJECT_TYPE ObjectType,
    IN OUT PSTR* ppszSid,
    IN OUT PSTR* ppszSamAccountName
    )
{
    DWORD dwError = 0;

    SetFlag(pItem->Flags, LSA_AD_BATCH_ITEM_FLAG_HAVE_REAL);

    dwError = LsaAdBatchGatherObjectType(pItem, ObjectType);
    BAIL_ON_LSA_ERROR(dwError);

    LSA_XFER_STRING(*ppszSid, pItem->pszSid);
    LSA_XFER_STRING(*ppszSamAccountName, pItem->pszSamAccountName);

    if (LSA_AD_BATCH_OBJECT_TYPE_USER == ObjectType)
    {
        pItem->UserInfo.dwPrimaryGroupRid = WELLKNOWN_SID_DOMAIN_USER_GROUP_RID;
        XXX; // verify that we do not need an LsaBatchGatherRpcUser()...
    }

cleanup:
    return dwError;

error:
    SetFlag(pItem->Flags, LSA_AD_BATCH_ITEM_FLAG_ERROR);
    goto cleanup;
}

static
DWORD
AD_RecordRealObjectFromRpc(
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
LsaAdBatchGatherSchemaModeUser(
    IN OUT PLSA_AD_BATCH_ITEM pItem,
    IN HANDLE hDirectory,
    IN LDAPMessage* pMessage
    )
{
    DWORD dwError = 0;
    DWORD dwValue = 0;

    dwError = LsaLdapGetUInt32(
                    hDirectory,
                    pMessage,
                    AD_LDAP_UID_TAG,
                    &dwValue);
    if (LSA_ERROR_INVALID_LDAP_ATTR_VALUE == dwError)
    {
        SetFlag(pItem->Flags, LSA_AD_BATCH_ITEM_FLAG_DISABLED);
        dwError = LSA_ERROR_SUCCESS;
    }
    BAIL_ON_LSA_ERROR(dwError);

    if (IsSetFlag(pItem->Flags, LSA_AD_BATCH_ITEM_FLAG_DISABLED))
    {
        goto cleanup;
    }

    if (!dwValue)
    {
        LSA_LOG_DEBUG("uid must be non-zero for SID '%s'", pItem->pszSid);
        // SetFlag(pItem->Flags, LSA_AD_BATCH_ITEM_FLAG_SKIP);
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pItem->UserInfo.uid = (uid_t)dwValue;

    dwError = LsaLdapGetUInt32(
                    hDirectory,
                    pMessage,
                    AD_LDAP_GID_TAG,
                    &dwValue);
    BAIL_ON_LSA_ERROR(dwError);

    if (!dwValue)
    {
        LSA_LOG_DEBUG("gid must be non-zero for SID '%s'", pItem->pszSid);
        // SetFlag(pItem->Flags, LSA_AD_BATCH_ITEM_FLAG_SKIP);
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pItem->UserInfo.gid = (gid_t)dwValue;

    dwError = LsaLdapGetString(
                    hDirectory,
                    pMessage,
                    AD_LDAP_ALIAS_TAG,
                    &pItem->UserInfo.pszAlias);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLdapGetString(
                    hDirectory,
                    pMessage,
                    AD_LDAP_PASSWD_TAG,
                    &pItem->UserInfo.pszPasswd);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLdapGetString(
                    hDirectory,
                    pMessage,
                    AD_LDAP_GECOS_TAG,
                    &pItem->UserInfo.pszGecos);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLdapGetString(
                    hDirectory,
                    pMessage,
                    AD_LDAP_HOMEDIR_TAG,
                    &pItem->UserInfo.pszHomeDirectory);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLdapGetString(
                    hDirectory,
                    pMessage,
                    AD_LDAP_SHELL_TAG,
                    &pItem->UserInfo.pszShell);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    return dwError;

error:
    goto cleanup;
}

static
DWORD
LsaAdBatchGatherSchemaModeGroup(
    IN OUT PLSA_AD_BATCH_ITEM pItem,
    IN HANDLE hDirectory,
    IN LDAPMessage* pMessage
    )
{
    DWORD dwError = 0;
    DWORD dwValue = 0;

    dwError = LsaLdapGetUInt32(
                    hDirectory,
                    pMessage,
                    AD_LDAP_GID_TAG,
                    &dwValue);
    if (LSA_ERROR_INVALID_LDAP_ATTR_VALUE == dwError)
    {
        SetFlag(pItem->Flags, LSA_AD_BATCH_ITEM_FLAG_DISABLED);
        dwError = LSA_ERROR_SUCCESS;
    }
    BAIL_ON_LSA_ERROR(dwError);

    if (IsSetFlag(pItem->Flags, LSA_AD_BATCH_ITEM_FLAG_DISABLED))
    {
        goto cleanup;
    }

    if (!dwValue)
    {
        LSA_LOG_DEBUG("gid must be non-zero for SID '%s'", pItem->pszSid);
        // SetFlag(pItem->Flags, LSA_AD_BATCH_ITEM_FLAG_SKIP);
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pItem->GroupInfo.gid = (gid_t)dwValue;

    dwError = LsaLdapGetString(
                    hDirectory,
                    pMessage,
                    AD_LDAP_ALIAS_TAG,
                    &pItem->GroupInfo.pszAlias);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLdapGetString(
                    hDirectory,
                    pMessage,
                    AD_LDAP_PASSWD_TAG,
                    &pItem->GroupInfo.pszPasswd);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    return dwError;

error:
    goto cleanup;
}

static
DWORD
LsaAdBatchGatherSchemaMode(
    IN OUT PLSA_AD_BATCH_ITEM pItem,
    IN HANDLE hDirectory,
    IN LDAPMessage* pMessage
    )
{
    DWORD dwError = 0;

    switch (pItem->ObjectType)
    {
        case LSA_AD_BATCH_OBJECT_TYPE_USER:
            dwError = LsaAdBatchGatherSchemaModeUser(
                            pItem,
                            hDirectory,
                            pMessage);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        case LSA_AD_BATCH_OBJECT_TYPE_GROUP:
            dwError = LsaAdBatchGatherSchemaModeGroup(
                            pItem,
                            hDirectory,
                            pMessage);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            LSA_ASSERT(FALSE);
            dwError = LSA_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

static
DWORD
LsaAdBatchGatherNonSchemaModeUser(
    IN OUT PLSA_AD_BATCH_ITEM pItem,
    IN DWORD dwKeywordValuesCount,
    IN PSTR* ppszKeywordValues
    )
{
    DWORD dwError = 0;
    DWORD dwValue = 0;

    dwError = ADNonSchemaKeywordGetUInt32(
                    ppszKeywordValues,
                    dwKeywordValuesCount,
                    AD_LDAP_UID_TAG,
                    &dwValue);
    if (LSA_ERROR_INVALID_LDAP_ATTR_VALUE == dwError)
    {
        SetFlag(pItem->Flags, LSA_AD_BATCH_ITEM_FLAG_DISABLED);
        dwError = LSA_ERROR_SUCCESS;
    }
    BAIL_ON_LSA_ERROR(dwError);

    if (IsSetFlag(pItem->Flags, LSA_AD_BATCH_ITEM_FLAG_DISABLED))
    {
        goto cleanup;
    }

    if (!dwValue)
    {
        LSA_LOG_DEBUG("uid must be non-zero for SID '%s'", pItem->pszSid);
        // SetFlag(pItem->Flags, LSA_AD_BATCH_ITEM_FLAG_SKIP);
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pItem->UserInfo.uid = (uid_t)dwValue;

    dwError = ADNonSchemaKeywordGetUInt32(
                    ppszKeywordValues,
                    dwKeywordValuesCount,
                    AD_LDAP_GID_TAG,
                    &dwValue);
    BAIL_ON_LSA_ERROR(dwError);

    if (!dwValue)
    {
        LSA_LOG_DEBUG("gid must be non-zero for SID '%s'", pItem->pszSid);
        // SetFlag(pItem->Flags, LSA_AD_BATCH_ITEM_FLAG_SKIP);
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pItem->UserInfo.gid = (gid_t)dwValue;

    dwError = ADNonSchemaKeywordGetString(
                    ppszKeywordValues,
                    dwKeywordValuesCount,
                    AD_LDAP_DISPLAY_NAME_TAG,
                    &pItem->GroupInfo.pszAlias);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADNonSchemaKeywordGetString(
                    ppszKeywordValues,
                    dwKeywordValuesCount,
                    AD_LDAP_ALIAS_TAG,
                    &pItem->UserInfo.pszAlias);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADNonSchemaKeywordGetString(
                    ppszKeywordValues,
                    dwKeywordValuesCount,
                    AD_LDAP_PASSWD_TAG,
                    &pItem->UserInfo.pszPasswd);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADNonSchemaKeywordGetString(
                    ppszKeywordValues,
                    dwKeywordValuesCount,
                    AD_LDAP_GECOS_TAG,
                    &pItem->UserInfo.pszGecos);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADNonSchemaKeywordGetString(
                    ppszKeywordValues,
                    dwKeywordValuesCount,
                    AD_LDAP_HOMEDIR_TAG,
                    &pItem->UserInfo.pszHomeDirectory);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADNonSchemaKeywordGetString(
                    ppszKeywordValues,
                    dwKeywordValuesCount,
                    AD_LDAP_SHELL_TAG,
                    &pItem->UserInfo.pszShell);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    return dwError;

error:
    goto cleanup;
}

static
DWORD
LsaAdBatchGatherNonSchemaModeGroup(
    IN OUT PLSA_AD_BATCH_ITEM pItem,
    IN DWORD dwKeywordValuesCount,
    IN PSTR* ppszKeywordValues
    )
{
    DWORD dwError = 0;
    DWORD dwValue = 0;

    dwError = ADNonSchemaKeywordGetUInt32(
                    ppszKeywordValues,
                    dwKeywordValuesCount,
                    AD_LDAP_GID_TAG,
                    &dwValue);
    if (LSA_ERROR_INVALID_LDAP_ATTR_VALUE == dwError)
    {
        SetFlag(pItem->Flags, LSA_AD_BATCH_ITEM_FLAG_DISABLED);
        dwError = LSA_ERROR_SUCCESS;
    }
    BAIL_ON_LSA_ERROR(dwError);

    if (IsSetFlag(pItem->Flags, LSA_AD_BATCH_ITEM_FLAG_DISABLED))
    {
        goto cleanup;
    }

    if (!dwValue)
    {
        LSA_LOG_DEBUG("gid must be non-zero for SID '%s'", pItem->pszSid);
        // SetFlag(pItem->Flags, LSA_AD_BATCH_ITEM_FLAG_SKIP);
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pItem->GroupInfo.gid = (gid_t)dwValue;

    dwError = ADNonSchemaKeywordGetString(
                    ppszKeywordValues,
                    dwKeywordValuesCount,
                    AD_LDAP_DISPLAY_NAME_TAG,
                    &pItem->GroupInfo.pszAlias);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADNonSchemaKeywordGetString(
                    ppszKeywordValues,
                    dwKeywordValuesCount,
                    AD_LDAP_PASSWD_TAG,
                    &pItem->GroupInfo.pszPasswd);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    return dwError;

error:
    goto cleanup;
}

static
DWORD
LsaAdBatchGatherNonSchemaMode(
    IN OUT PLSA_AD_BATCH_ITEM pItem,
    IN DWORD dwKeywordValuesCount,
    IN PSTR* ppszKeywordValues
    )
{
    DWORD dwError = 0;

    switch (pItem->ObjectType)
    {
        case LSA_AD_BATCH_OBJECT_TYPE_USER:
            dwError = LsaAdBatchGatherNonSchemaModeUser(
                            pItem,
                            dwKeywordValuesCount,
                            ppszKeywordValues);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        case LSA_AD_BATCH_OBJECT_TYPE_GROUP:
            dwError = LsaAdBatchGatherNonSchemaModeGroup(
                            pItem,
                            dwKeywordValuesCount,
                            ppszKeywordValues);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            LSA_ASSERT(FALSE);
            dwError = LSA_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

static
DWORD
LsaAdBatchGatherUnprovisionedModeUser(
    IN OUT PLSA_AD_BATCH_ITEM pItem,
    IN HANDLE hDirectory,
    IN LDAPMessage* pMessage
    )
{
    DWORD dwError = 0;

    // Need the primary group RID to marshal gid later.
    dwError = LsaLdapGetUInt32(
                hDirectory,
                pMessage,
                AD_LDAP_PRIMEGID_TAG,
                &pItem->UserInfo.dwPrimaryGroupRid);
    BAIL_ON_LSA_ERROR(dwError);
#if 0
    if (dwError != 0)
    {
        LSA_LOG_ERROR("Failed to get primary group ID for SID '%s'",
                      pItem->pszSid);
        pItem->UserInfo.dwPrimaryGroupRid = WELLKNOWN_SID_DOMAIN_USER_GROUP_RID;
        dwError = 0;
    }
#endif

    // Use display name for user gecos.
    dwError = LsaLdapGetString(
                    hDirectory,
                    pMessage,
                    AD_LDAP_DISPLAY_NAME_TAG,
                    &pItem->UserInfo.pszGecos);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    return dwError;

error:
    goto cleanup;
}

static
DWORD
LsaAdBatchGatherUnprovisionedMode(
    IN OUT PLSA_AD_BATCH_ITEM pItem,
    IN HANDLE hDirectory,
    IN LDAPMessage* pMessage
    )
{
    DWORD dwError = 0;

    switch (pItem->ObjectType)
    {
        case LSA_AD_BATCH_OBJECT_TYPE_USER:
            dwError = LsaAdBatchGatherUnprovisionedModeUser(
                            pItem,
                            hDirectory,
                            pMessage);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        case LSA_AD_BATCH_OBJECT_TYPE_GROUP:
            // Nothing special for groups.
            break;
        default:
            LSA_ASSERT(FALSE);
            dwError = LSA_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

static
DWORD
LsaAdBatchGatherRealUser(
    IN OUT PLSA_AD_BATCH_ITEM pItem,
    IN HANDLE hDirectory,
    IN LDAPMessage* pMessage
    )
{
    DWORD dwError = 0;

    dwError = LsaLdapGetString(
                    hDirectory,
                    pMessage,
                    AD_LDAP_UPN_TAG,
                    &pItem->UserInfo.pszUserPrincipalName);
    BAIL_ON_LSA_ERROR(dwError);

    XXX; // Get rid of ADGetLDAPUPNString or refactor it and use refactor...
    if (pItem->UserInfo.pszUserPrincipalName)
    {
        if (!index(pItem->UserInfo.pszUserPrincipalName, '@'))
        {
            dwError = LSA_ERROR_DATA_ERROR;
            BAIL_ON_LSA_ERROR(dwError);
        }

        // Do not touch the non-realm part, just the realm part
        // to make sure the realm conforms to spec.
        LsaPrincipalRealmToUpper(pItem->UserInfo.pszUserPrincipalName);
    }

    dwError = LsaLdapGetUInt32(
                    hDirectory,
                    pMessage,
                    AD_LDAP_USER_CTRL_TAG,
                    &pItem->UserInfo.UserAccountControl);
    if (dwError == LSA_ERROR_INVALID_LDAP_ATTR_VALUE)
    {
        LSA_LOG_ERROR(
                "User %s has an invalid value for the userAccountControl"
                " attribute. Please check that it is set and that the "
                "machine account has permission to read it.",
                pItem->pszSid);
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLdapGetUInt64(
                    hDirectory,
                    pMessage,
                    AD_LDAP_ACCOUT_EXP_TAG,
                    &pItem->UserInfo.AccountExpires);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLdapGetUInt64(
                    hDirectory,
                    pMessage,
                    AD_LDAP_PWD_LASTSET_TAG,
                    &pItem->UserInfo.PasswordLastSet);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    return dwError;

error:
    goto cleanup;
}

static
DWORD
LsaAdBatchGatherObjectType(
    IN OUT PLSA_AD_BATCH_ITEM pItem,
    IN LSA_AD_BATCH_OBJECT_TYPE ObjectType
    )
{
    DWORD dwError = 0;

    // Check that the object type is not changing under us.
    if ((ObjectType != LSA_AD_BATCH_OBJECT_TYPE_USER) &&
        (ObjectType != LSA_AD_BATCH_OBJECT_TYPE_GROUP))
    {
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }
    else if (!pItem->ObjectType)
    {
        pItem->ObjectType = ObjectType;
    }
    else if (pItem->ObjectType != ObjectType)
    {
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

static
DWORD
LsaAdBatchGatherRealObject(
    IN OUT PLSA_AD_BATCH_ITEM pItem,
    IN LSA_AD_BATCH_OBJECT_TYPE ObjectType,
    IN OUT PSTR* ppszSid,
    IN HANDLE hDirectory,
    IN LDAPMessage* pMessage
    )
{
    DWORD dwError = 0;

    SetFlag(pItem->Flags, LSA_AD_BATCH_ITEM_FLAG_HAVE_REAL);

    dwError = LsaAdBatchGatherObjectType(pItem, ObjectType);
    BAIL_ON_LSA_ERROR(dwError);

    LSA_XFER_STRING(*ppszSid, pItem->pszSid);
    if (IsNullOrEmptyString(pItem->pszSid))
    {
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaLdapGetString(
                    hDirectory,
                    pMessage,
                    AD_LDAP_SAM_NAME_TAG,
                    &pItem->pszSamAccountName);
    BAIL_ON_LSA_ERROR(dwError);
    if (IsNullOrEmptyString(pItem->pszSamAccountName))
    {
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaLdapGetString(
                    hDirectory,
                    pMessage,
                    AD_LDAP_DN_TAG,
                    &pItem->pszDn);
    BAIL_ON_LSA_ERROR(dwError);
    if (IsNullOrEmptyString(pItem->pszDn))
    {
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    // Handle cases where real contains pseudo.
    if (LsaAdBatchIsDefaultSchemaMode())
    {
        SetFlag(pItem->Flags, LSA_AD_BATCH_ITEM_FLAG_HAVE_PSEUDO);
        dwError = LsaAdBatchGatherSchemaMode(
                        pItem,
                        hDirectory,
                        pMessage);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else if (LsaAdBatchIsUnprovisionedMode())
    {
        dwError = LsaAdBatchGatherUnprovisionedMode(
                        pItem,
                        hDirectory,
                        pMessage);
        BAIL_ON_LSA_ERROR(dwError);
    }

    // User object has some AD-specific fields.
    if (LSA_AD_BATCH_OBJECT_TYPE_USER == pItem->ObjectType)
    {
        dwError = LsaAdBatchGatherRealUser(
                        pItem,
                        hDirectory,
                        pMessage);
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    return dwError;

error:
    SetFlag(pItem->Flags, LSA_AD_BATCH_ITEM_FLAG_ERROR);
    goto cleanup;
}

static
DWORD
AD_RecordRealObject(
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
AD_ResolvePseudoObjectsByRealObjects(
    IN HANDLE hProvider,
    IN PCSTR pszDnsDomainName,
    IN DWORD dwTotalItemCount,
    // List of PLSA_AD_BATCH_ITEM
    IN OUT PLSA_LIST_LINKS pBatchItemList
    )
{
    DWORD dwError = 0;

    if (gpADProviderData->dwDirectoryMode == CELL_MODE)
    {
        dwError = AD_ResolvePseudoObjectsByRealObjectsWithLinkedCell(
                     hProvider,
                     dwTotalItemCount,
                     pBatchItemList);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        dwError = AD_ResolvePseudoObjectsByRealObjectsInternal(
                     hProvider,
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
AD_ResolvePseudoObjectsByRealObjectsWithLinkedCell(
    IN HANDLE hProvider,
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

    dwError = AD_ResolvePseudoObjectsByRealObjectsInternal(
                    hProvider,
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

        dwError = AD_ResolvePseudoObjectsByRealObjectsInternal(
                    hProvider,
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
AD_ResolvePseudoObjectsByRealObjectsInternal(
    IN HANDLE hProvider,
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

    dwError = AD_BuildBatchQueryScopeForPseudoBySid(
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

        dwError = AD_BuildBatchQueryForPseudoBySid(
                        bIsSchemaMode,
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
            dwError = AD_RecordPseudoObjectBySid(
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
LsaAdBatchGatherPseudoObject(
    IN OUT PLSA_AD_BATCH_ITEM pItem,
    IN OUT PCSTR pszSid,
    IN LSA_AD_BATCH_OBJECT_TYPE ObjectType,
    IN BOOLEAN bIsSchemaMode,
    IN OPTIONAL DWORD dwKeywordValuesCount,
    IN OPTIONAL PSTR* ppszKeywordValues,
    IN HANDLE hDirectory,
    IN LDAPMessage* pMessage
    )
{
    DWORD dwError = 0;

    SetFlag(pItem->Flags, LSA_AD_BATCH_ITEM_FLAG_HAVE_PSEUDO);

    dwError = LsaAdBatchGatherObjectType(pItem, ObjectType);
    BAIL_ON_LSA_ERROR(dwError);

    if (!pItem->pszSid)
    {
        dwError = LsaAllocateString(pszSid, &pItem->pszSid);
    }
    else
    {
        if (strcasecmp(pszSid, pItem->pszSid))
        {
            LSA_ASSERT(FALSE);
            dwError = LSA_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    if (bIsSchemaMode)
    {
        dwError = LsaAdBatchGatherSchemaMode(
                        pItem,
                        hDirectory,
                        pMessage);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        dwError = LsaAdBatchGatherNonSchemaMode(
                        pItem,
                        dwKeywordValuesCount,
                        ppszKeywordValues);
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    return dwError;

error:
    SetFlag(pItem->Flags, LSA_AD_BATCH_ITEM_FLAG_ERROR);
    goto cleanup;
}

static
DWORD
AD_RecordPseudoObjectBySid(
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

#if 0
// OLD CODE....can get rid of it...
static
DWORD
AD_MarshalBatchItem(
    IN PCSTR pszDnsDomainName,
    IN BOOLEAN bIsOneWayTrust,
    IN PLSA_AD_BATCH_ITEM pItem,
    IN HANDLE hRealLdapHandle,
    IN HANDLE hPseudoLdapHandle,
    OUT PAD_SECURITY_OBJECT* ppObject
    )
{
    DWORD dwError = 0;
    PLSA_LOGIN_NAME_INFO pLoginNameInfo = NULL;

    // In case of one way trust, we have no access of real object hence,
    // pItem->pRealMessage should be NULL.
    if (bIsOneWayTrust && pItem->pRealMessage)
    {
        dwError = LSA_ERROR_INTERNAL;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (!bIsOneWayTrust && (IsNullOrEmptyString(pItem->pszSid) || !pItem->pRealMessage))
    {
        LSA_LOG_DEBUG("Did not find object '%s'",
                      LSA_SAFE_LOG_STRING(pItem->pszQuery));
        dwError = 0;
        goto cleanup;
    }

    if ((gpADProviderData->dwDirectoryMode == DEFAULT_MODE) &&
        (gpADProviderData->adConfigurationMode == SchemaMode))
    {
        // Default/Schema mode, the PseudoMessage should be NULL
        if (pItem->pPseudoMessage)
        {
            dwError = LSA_ERROR_INTERNAL;
        }
        BAIL_ON_LSA_ERROR(dwError);

        pItem->pPseudoMessage = pItem->pRealMessage;
    }
    else if (gpADProviderData->dwDirectoryMode == UNPROVISIONED_MODE)
    {
        if (pItem->pPseudoMessage)
        {
           dwError = LSA_ERROR_INTERNAL;
        }
        BAIL_ON_LSA_ERROR(dwError);
    }
    // Skip any disabled non-groups but not disabled groups.
    else if (!pItem->pPseudoMessage &&
             (pItem->objectType != AccountType_Group))
    {
        // skip any disabled non-groups.
        dwError = 0;
        goto cleanup;
    }

    dwError = CreateObjectLoginNameInfo(
                    &pLoginNameInfo,
                    pszDnsDomainName,
                    pItem->pszSamAccountName,
                    pItem->pszSid);
    BAIL_ON_LSA_ERROR(dwError);

    switch (pItem->objectType)
    {
        case AccountType_User:
            dwError = ADMarshalToUserCache(
                        hPseudoLdapHandle,
                        hRealLdapHandle,
                        gpADProviderData->dwDirectoryMode,
                        pItem->adConfMode,
                        pLoginNameInfo,
                        pItem->pRealMessage,
                        pItem->pPseudoMessage,
                        ppObject);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        case AccountType_Group:
            dwError = ADMarshalToGroupCache(
                        hPseudoLdapHandle,
                        hRealLdapHandle,
                        gpADProviderData->dwDirectoryMode,
                        pItem->adConfMode,
                        pLoginNameInfo,
                        pItem->pRealMessage,
                        pItem->pPseudoMessage,
                        ppObject);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = LSA_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    LSA_SAFE_FREE_LOGIN_NAME_INFO(pLoginNameInfo);
    return dwError;

error:
    goto cleanup;
}
#endif

static
DWORD
LsaAdBatchMarshalUserInfoFixHomeDirectory(
    IN OUT PSTR* ppszHomeDirectory,
    IN PCSTR pszNetbiosDomainName,
    IN PCSTR pszSamAccountName
    )
{
    DWORD dwError = 0;
    PSTR pszHomeDirectory = *ppszHomeDirectory;
    PSTR pszNewHomeDirectory = NULL;

    if (!pszHomeDirectory)
    {
        dwError = AD_GetUnprovisionedModeHomedirTemplate(&pszHomeDirectory);
        BAIL_ON_LSA_ERROR(dwError);
        BAIL_ON_INVALID_STRING(pszHomeDirectory);
    }

    if (strstr(pszHomeDirectory, "%"))
    {
        dwError = AD_BuildHomeDirFromTemplate(
                    pszHomeDirectory,
                    pszNetbiosDomainName,
                    pszSamAccountName,
                    &pszNewHomeDirectory);
        BAIL_ON_LSA_ERROR(dwError);

        LSA_SAFE_FREE_STRING(pszHomeDirectory);
        LSA_XFER_STRING(pszNewHomeDirectory, pszHomeDirectory);
    }

    LsaStrCharReplace(pszHomeDirectory, ' ', '_');

cleanup:
    *ppszHomeDirectory = pszHomeDirectory;
    return dwError;

error:
    goto cleanup;
}

static
DWORD
LsaAdBatchMarshalUserInfoFixShell(
    IN OUT PSTR* ppszShell
    )
{
    DWORD dwError = 0;
    PSTR pszShell = *ppszShell;

    if (!pszShell)
    {
        dwError = AD_GetUnprovisionedModeShell(&pszShell);
        BAIL_ON_LSA_ERROR(dwError);
        BAIL_ON_INVALID_STRING(pszShell);
    }

cleanup:
    *ppszShell = pszShell;
    return dwError;

error:
    goto cleanup;
}

// This function fillls in all of the booleans in pObjectUserInfo except for
// bPromptPasswordChange and bAccountExpired
static
VOID
LsaAdBatchMarshalUserInfoAccountControl(
    IN UINT32 AccountControl,
    IN OUT PAD_SECURITY_OBJECT_USER_INFO pObjectUserInfo
    )
{
    pObjectUserInfo->bPasswordNeverExpires = IsSetFlag(AccountControl, LSA_AD_UF_DONT_EXPIRE_PASSWD);
    if (pObjectUserInfo->bPasswordNeverExpires)
    {
        pObjectUserInfo->bPasswordExpired = FALSE;
    }
    else
    {
        pObjectUserInfo->bPasswordExpired = IsSetFlag(AccountControl, LSA_AD_UF_PASSWORD_EXPIRED);
    }
    pObjectUserInfo->bUserCanChangePassword = IsSetFlag(AccountControl, LSA_AD_UF_CANT_CHANGE_PASSWD);
    pObjectUserInfo->bAccountDisabled = IsSetFlag(AccountControl, LSA_AD_UF_ACCOUNTDISABLE);
    pObjectUserInfo->bAccountLocked = IsSetFlag(AccountControl, LSA_AD_UF_LOCKOUT);
}

static
DWORD
LsaAdBatchMarshalUserInfoAccountExpires(
    IN UINT64 AccountExpires,
    IN OUT PAD_SECURITY_OBJECT_USER_INFO pObjectUserInfo
    )
{
    DWORD dwError = 0;

    if (AccountExpires == 0LL ||
        AccountExpires == 9223372036854775807LL)
    {
        // This means the account will never expire.
        pObjectUserInfo->bAccountExpired = FALSE;
    }
    else
    {
        // in 100ns units:
        UINT64 currentNtTime = 0;

        dwError = ADGetCurrentNtTime(&currentNtTime);
        BAIL_ON_LSA_ERROR(dwError);

        if (currentNtTime <= AccountExpires)
        {
            pObjectUserInfo->bAccountExpired = FALSE;
        }
        else
        {
            pObjectUserInfo->bAccountExpired = TRUE;
        }
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}


static
DWORD
LsaAdBatchMarshalUserInfoPasswordLastSet(
    IN UINT64 PasswordLastSet,
    IN OUT PAD_SECURITY_OBJECT_USER_INFO pObjectUserInfo
    )
{
    DWORD dwError = 0;
    // in 100ns units:
    int64_t passwordExpiry = 0;
    UINT64 currentNtTime = 0;

    dwError = ADGetCurrentNtTime(&currentNtTime);
    BAIL_ON_LSA_ERROR(dwError);

    // ISSUE-2008/11/18-dalmeida -- Don't we need to check
    // for the max password age of the user domain rather
    // than the machine domain?
    passwordExpiry = gpADProviderData->adMaxPwdAge -
        (currentNtTime - PasswordLastSet);
    // ISSUE-2008/11/18-dalmeida -- The number of days
    // should be a setting.
    if (passwordExpiry / (10000000LL * 24*60*60) <= 14)
    {
        //The password will expire in 14 days or less
        pObjectUserInfo->bPromptPasswordChange = TRUE;
    }
    else
    {
        pObjectUserInfo->bPromptPasswordChange = FALSE;
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

static
DWORD
LsaAdBatchMarshalUnprovisionedUser(
    IN OUT PLSA_AD_BATCH_ITEM_USER_INFO pUserInfo,
    IN PCSTR pszDnsDomainName,
    IN PCSTR pszNetbiosDomainName,
    IN PCSTR pszSamAccountName,
    IN PCSTR pszSid
    )
{
    DWORD dwError = 0;
    DWORD dwId = 0;
    PLSA_SECURITY_IDENTIFIER pSid = 0;

    // uid
    dwError = LsaAllocSecurityIdentifierFromString(pszSid, &pSid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaHashSecurityIdentifierToId(pSid, &dwId);
    BAIL_ON_LSA_ERROR(dwError);

    pUserInfo->uid = (uid_t)dwId;

    // gid
    dwError = LsaSetSecurityIdentifierRid(pSid, pUserInfo->dwPrimaryGroupRid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaHashSecurityIdentifierToId(pSid, &dwId);
    BAIL_ON_LSA_ERROR(dwError);

    pUserInfo->gid = (gid_t)dwId;

    // can add alias here

cleanup:
    if (pSid)
    {
        LsaFreeSecurityIdentifier(pSid);
    }
    return dwError;

error:
    goto cleanup;
}

static
DWORD
LsaAdBatchMarshalUnprovisionedGroup(
    IN OUT PLSA_AD_BATCH_ITEM_GROUP_INFO pGroupInfo,
    IN PCSTR pszDnsDomainName,
    IN PCSTR pszNetbiosDomainName,
    IN PCSTR pszSamAccountName,
    IN PCSTR pszSid
    )
{
    DWORD dwError = 0;
    DWORD dwId = 0;

    // gid
    dwError = LsaHashSidStringToId(pszSid, &dwId);
    BAIL_ON_LSA_ERROR(dwError);

    pGroupInfo->gid = (gid_t)dwId;

    // can add alias here

cleanup:
    return dwError;

error:
    goto cleanup;
}

static
DWORD
LsaAdBatchMarshalUserInfo(
    IN OUT PLSA_AD_BATCH_ITEM_USER_INFO pUserInfo,
    OUT PAD_SECURITY_OBJECT_USER_INFO pObjectUserInfo,
    IN PCSTR pszDnsDomainName,
    IN PCSTR pszNetbiosDomainName,
    IN PCSTR pszSamAccountName,
    IN PCSTR pszSid
    )
{
    DWORD dwError = 0;

    pObjectUserInfo->bIsGeneratedUPN = FALSE;

    if (LsaAdBatchIsUnprovisionedMode())
    {
        dwError = LsaAdBatchMarshalUnprovisionedUser(
                        pUserInfo,
                        pszDnsDomainName,
                        pszNetbiosDomainName,
                        pszSamAccountName,
                        pszSid);
        BAIL_ON_LSA_ERROR(dwError);
    }

    pObjectUserInfo->uid = pUserInfo->uid;
    pObjectUserInfo->gid = pUserInfo->gid;

    LSA_XFER_STRING(pUserInfo->pszAlias, pObjectUserInfo->pszAliasName);
    LSA_XFER_STRING(pUserInfo->pszPasswd, pObjectUserInfo->pszPasswd);
    LSA_XFER_STRING(pUserInfo->pszGecos, pObjectUserInfo->pszGecos);
    LSA_XFER_STRING(pUserInfo->pszShell, pObjectUserInfo->pszShell);
    LSA_XFER_STRING(pUserInfo->pszHomeDirectory, pObjectUserInfo->pszHomedir);
    LSA_XFER_STRING(pUserInfo->pszUserPrincipalName, pObjectUserInfo->pszUPN);

    pObjectUserInfo->qwPwdLastSet = pUserInfo->PasswordLastSet;
    pObjectUserInfo->qwAccountExpires = pUserInfo->AccountExpires;

    // Handle shell.
    dwError = LsaAdBatchMarshalUserInfoFixShell(&pObjectUserInfo->pszShell);
    BAIL_ON_LSA_ERROR(dwError);

    // Handle home directory.
    dwError = LsaAdBatchMarshalUserInfoFixHomeDirectory(
                    &pObjectUserInfo->pszHomedir,
                    pszNetbiosDomainName,
                    pszSamAccountName);
    BAIL_ON_LSA_ERROR(dwError);

    // Handle UPN.
    if (!pObjectUserInfo->pszUPN)
    {
        dwError = ADGetLDAPUPNString(
                        0,
                        NULL,
                        pszDnsDomainName,
                        pszSamAccountName,
                        &pObjectUserInfo->pszUPN,
                        &pObjectUserInfo->bIsGeneratedUPN);
        BAIL_ON_LSA_ERROR(dwError);
    }

    // Decode account control flags.
    LsaAdBatchMarshalUserInfoAccountControl(
            pUserInfo->UserAccountControl,
            pObjectUserInfo);

    // Figure out account expiration.
    dwError = LsaAdBatchMarshalUserInfoAccountExpires(
                    pUserInfo->AccountExpires,
                    pObjectUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    // Figure out password prompting.
    dwError = LsaAdBatchMarshalUserInfoPasswordLastSet(
                    pUserInfo->PasswordLastSet,
                    pObjectUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    return dwError;

error:
    goto cleanup;
}

static
DWORD
LsaAdBatchMarshalGroupInfo(
    IN OUT PLSA_AD_BATCH_ITEM_GROUP_INFO pGroupInfo,
    OUT PAD_SECURITY_OBJECT_GROUP_INFO pObjectGroupInfo,
    IN PCSTR pszDnsDomainName,
    IN PCSTR pszNetbiosDomainName,
    IN PCSTR pszSamAccountName,
    IN PCSTR pszSid
    )
{
    DWORD dwError = 0;

    if (LsaAdBatchIsUnprovisionedMode())
    {
        dwError = LsaAdBatchMarshalUnprovisionedGroup(
                        pGroupInfo,
                        pszDnsDomainName,
                        pszNetbiosDomainName,
                        pszSamAccountName,
                        pszSid);
        BAIL_ON_LSA_ERROR(dwError);
    }

    pObjectGroupInfo->gid = pGroupInfo->gid;

    LSA_XFER_STRING(pGroupInfo->pszAlias, pObjectGroupInfo->pszAliasName);
    LSA_XFER_STRING(pGroupInfo->pszPasswd, pObjectGroupInfo->pszPasswd);

cleanup:
    return dwError;

error:
    goto cleanup;
}

static
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
LsaAdBatchMarshal(
    IN OUT PLSA_AD_BATCH_ITEM pItem,
    OUT PAD_SECURITY_OBJECT* ppObject
    )
{
    DWORD dwError = 0;
    PAD_SECURITY_OBJECT pObject = NULL;

    // To marshal, the following conditions to be satisfied:
    //
    // 1) Object must have user or group type.
    // 2) Object must have real information.
    // 3) Object must have either:
    //    a) Pseudo information.
    //    b) Or be in unprovisioned mode.
    if ((LSA_AD_BATCH_OBJECT_TYPE_UNDEFINED == pItem->ObjectType) ||
        !IsSetFlag(pItem->Flags, LSA_AD_BATCH_ITEM_FLAG_HAVE_REAL) ||
        !(IsSetFlag(pItem->Flags, LSA_AD_BATCH_ITEM_FLAG_HAVE_PSEUDO) || LsaAdBatchIsUnprovisionedMode()))
    {
        PCSTR pszType = NULL;
        BOOLEAN bIsString = FALSE;
        PCSTR pszString = NULL;
        DWORD dwId = 0;

        LsaAdBatchQueryTermDebugInfo(
                &pItem->QueryTerm,
                &pszType,
                &bIsString,
                &pszString,
                &dwId);
        if (bIsString)
        {
            LSA_LOG_DEBUG("Did not find object %s '%s'", pszType, pszString);
        }
        else
        {
            LSA_LOG_DEBUG("Did not find object %s %u", pszType, dwId);
        }
        dwError = 0;
        goto cleanup;
    }

    LSA_ASSERT(LSA_IS_XOR(IsSetFlag(pItem->Flags, LSA_AD_BATCH_ITEM_FLAG_HAVE_PSEUDO), LsaAdBatchIsUnprovisionedMode()));

    if (IsSetFlag(pItem->Flags, LSA_AD_BATCH_ITEM_FLAG_DISABLED) &&
        (pItem->ObjectType != LSA_AD_BATCH_OBJECT_TYPE_GROUP))
    {
        // Skip any disabled non-groups.
        dwError = 0;
        goto cleanup;
    }

    dwError = LsaAllocateMemory(sizeof(*pObject), (PVOID*)&pObject);
    BAIL_ON_LSA_ERROR(dwError);

    pObject->cache.qwCacheId = -1;

    pObject->enabled = !IsSetFlag(pItem->Flags, LSA_AD_BATCH_ITEM_FLAG_DISABLED);

    // Transfer the data
    LSA_XFER_STRING(pItem->pszSid, pObject->pszObjectSid);
    LSA_XFER_STRING(pItem->pszSamAccountName, pObject->pszSamAccountName);
    LSA_XFER_STRING(pItem->pszDn, pObject->pszDN);

    dwError = LsaAllocateString(
                    pItem->pDomainEntry->pszNetbiosDomainName,
                    &pObject->pszNetbiosDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    switch (pItem->ObjectType)
    {
        case LSA_AD_BATCH_OBJECT_TYPE_USER:
            pObject->type = AccountType_User;
            dwError = LsaAdBatchMarshalUserInfo(
                            &pItem->UserInfo,
                            &pObject->userInfo,
                            pItem->pDomainEntry->pszDnsDomainName,
                            pObject->pszNetbiosDomainName,
                            pObject->pszSamAccountName,
                            pObject->pszObjectSid);
            BAIL_ON_LSA_ERROR(dwError);
            break;

        case LSA_AD_BATCH_OBJECT_TYPE_GROUP:
            pObject->type = AccountType_Group;
            dwError = LsaAdBatchMarshalGroupInfo(
                            &pItem->GroupInfo,
                            &pObject->groupInfo,
                            pItem->pDomainEntry->pszDnsDomainName,
                            pObject->pszNetbiosDomainName,
                            pObject->pszSamAccountName,
                            pObject->pszObjectSid);
            BAIL_ON_LSA_ERROR(dwError);
            break;

        default:
            LSA_ASSERT(FALSE);
            dwError = LSA_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    *ppObject = pObject;
    return dwError;

error:
    if (pObject)
    {
        ADCacheDB_SafeFreeObject(&pObject);
    }
    goto cleanup;
}

static
DWORD
LsaAdBatchMarshalList(
    IN OUT PLSA_LIST_LINKS pBatchItemList,
    IN DWORD dwAvailableCount,
    OUT PAD_SECURITY_OBJECT* ppObjects,
    OUT PDWORD pdwUsedCount
    )
{
    DWORD dwError = 0;
    PLSA_LIST_LINKS pLinks = NULL;
    DWORD dwIndex = 0;

    for (pLinks = pBatchItemList->Next;
         pLinks != pBatchItemList;
         pLinks = pLinks->Next)
    {
        PLSA_AD_BATCH_ITEM pItem = LW_STRUCT_FROM_FIELD(pLinks, LSA_AD_BATCH_ITEM, BatchItemListLinks);

        if (dwIndex >= dwAvailableCount)
        {
            LSA_ASSERT(FALSE);
            dwError = LSA_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
        }

        dwError = LsaAdBatchMarshal(pItem, &ppObjects[dwIndex]);
        BAIL_ON_LSA_ERROR(dwError);
        if (ppObjects[dwIndex])
        {
            dwIndex++;
        }
    }

cleanup:
    *pdwUsedCount = dwIndex;
    return dwError;

error:
    goto cleanup;
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

