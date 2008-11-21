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
AD_GetDomainNameAndSidStringByObjectSid(
    IN PCSTR pszObjectSid,
    OUT PSTR* ppszDnsDomainName,
    OUT PSTR* ppszDomainSid
    )
{
    DWORD dwError = 0;
    PSID pObjectSid = NULL;
    PSTR pszDnsDomainName = NULL;
    PSID pDomainSid = NULL;
    PSTR pszDomainSid = NULL;

    dwError = ParseSidString(&pObjectSid, pszObjectSid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaDmQueryDomainInfoByObjectSid(
                    pObjectSid,
                    &pszDnsDomainName,
                    NULL,
                    &pDomainSid,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_SidToString(pDomainSid, &pszDomainSid);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    if (pObjectSid)
    {
        SidFree(pObjectSid);
    }
    LSA_SAFE_FREE_MEMORY(pDomainSid);

    *ppszDnsDomainName = pszDnsDomainName;
    *ppszDomainSid = pszDomainSid;

    return dwError;

error:
    LSA_SAFE_FREE_STRING(pszDnsDomainName);
    LSA_SAFE_FREE_STRING(pszDomainSid);
    goto cleanup;
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
            switch(gpADProviderData->dwDirectoryMode)
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
    IN PCSTR pszQueryTerm
    )
{
    DWORD dwError = 0;
    PLSA_AD_BATCH_DOMAIN_ENTRY pEntry = NULL;
    PSTR pszDnsDomainName = NULL;
    PCSTR pszDcPart = NULL;
    PSTR pszDomainSid = NULL;
    BOOLEAN bSkip = FALSE;
    BOOLEAN bIsOneWayTrust = FALSE;

    switch (QueryType)
    {
        case LSA_AD_BATCH_QUERY_TYPE_BY_DN:
            dwError = LsaLdapConvertDNToDomain(pszQueryTerm,
                                               &pszDnsDomainName);
            BAIL_ON_LSA_ERROR(dwError);

            pszDcPart = pszQueryTerm;

            break;

        case LSA_AD_BATCH_QUERY_TYPE_BY_SID:
            dwError = AD_GetDomainNameAndSidStringByObjectSid(
                            pszQueryTerm,
                            &pszDnsDomainName,
                            &pszDomainSid);
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

    // Can only have one or the other
    if (!LSA_IS_XOR(pszDcPart, pszDomainSid))
    {
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pEntry->pszDnsDomainName = pszDnsDomainName;
    pszDnsDomainName = NULL;

    pEntry->bSkip = bSkip;
    pEntry->bIsOneWayTrust = bIsOneWayTrust;

    if (pszDcPart)
    {
        pEntry->pszDcPart = pszDcPart;
        pszDcPart = NULL;
    }

    if (pszDomainSid)
    {
        pEntry->pszDomainSid = pszDomainSid;
        pszDomainSid = NULL;

        pEntry->sDomainSidLength = strlen(pEntry->pszDomainSid);
    }

    *ppEntry = pEntry;

cleanup:
    LSA_SAFE_FREE_STRING(pszDnsDomainName);
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
        LSA_SAFE_FREE_STRING(pEntry->pszDnsDomainName);
        LSA_SAFE_FREE_STRING(pEntry->pszDomainSid);
        LsaDLinkedListFree(pEntry->pQueryList);
        ADCacheDB_SafeFreeObjectList(pEntry->dwObjectsCount, &pEntry->ppObjects);
        LsaFreeMemory(pEntry);
        *ppEntry = NULL;
    }
}

static
DWORD
CreateBatchItem(
    OUT PLSA_AD_BATCH_ITEM* ppItem,
    IN PCSTR pszQuery
    )
{
    DWORD dwError = 0;
    PLSA_AD_BATCH_ITEM pItem = NULL;

    dwError = LsaAllocateMemory(sizeof(*pItem), (PVOID*)&pItem);
    BAIL_ON_LSA_ERROR(dwError);

    pItem->pszQuery = pszQuery;
    pItem->objectType = AccountType_NotFound;
    pItem->adConfMode = UnknownMode;
    pItem->bFound = FALSE;

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
        LsaFreeMemory(pItem);
        *ppItem = NULL;
    }
}

static
DWORD
CreateBatchMessages(
    OUT PLSA_AD_BATCH_MESSAGES* ppMessages,
    IN HANDLE hLdapHandle
    )
{
    DWORD dwError = 0;
    PLSA_AD_BATCH_MESSAGES pMessages = NULL;

    dwError = LsaAllocateMemory(sizeof(*pMessages), (PVOID*)&pMessages);
    BAIL_ON_LSA_ERROR(dwError);

    pMessages->hLdapHandle = hLdapHandle;

    *ppMessages = pMessages;

cleanup:
    return dwError;

error:
    *ppMessages = NULL;
    DestroyBatchMessages(&pMessages);
    goto cleanup;
}

static
VOID
DestroyBatchMessages(
    IN OUT PLSA_AD_BATCH_MESSAGES* ppMessages
    )
{
    PLSA_AD_BATCH_MESSAGES pMessages = *ppMessages;
    if (pMessages)
    {
        PDLINKEDLIST pNode;
        LsaLdapCloseDirectory(pMessages->hLdapHandle);
        for (pNode = pMessages->pLdapMessageList; pNode; pNode = pNode->pNext)
        {
            ldap_msgfree((LDAPMessage*)pNode->pItem);
        }
        LsaDLinkedListFree(pMessages->pLdapMessageList);
        LsaFreeMemory(pMessages);
        *ppMessages = NULL;
    }
}

DWORD
ADBatch_GetQueryTerm(
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN PCSTR pszQueryListItem,
    OUT PCSTR* ppszQueryTerm
    )
{
    DWORD dwError = 0;
    PCSTR pszQueryTerm = NULL;

    switch (QueryType)
    {
        case LSA_AD_BATCH_QUERY_TYPE_BY_DN:
            pszQueryTerm = ParseDcPart(pszQueryListItem);
            break;
        case LSA_AD_BATCH_QUERY_TYPE_BY_SID:
            pszQueryTerm = pszQueryListItem;
            break;
        default:
            dwError = LSA_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
    }

    *ppszQueryTerm = pszQueryTerm;

cleanup:
    return dwError;

error:
    *ppszQueryTerm = FALSE;
    goto cleanup;
}

static
BOOLEAN
AD_MatchesDomainSid(
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
ADBatch_MatchQuery(
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN PCSTR pszQueryTerm,
    IN PLSA_AD_BATCH_DOMAIN_ENTRY pEntry,
    OUT PBOOLEAN pbIsMatch
    )
{
    DWORD dwError = 0;
    BOOLEAN bIsMatch = FALSE;

    switch (QueryType)
    {
        case LSA_AD_BATCH_QUERY_TYPE_BY_DN:
            if (!strcasecmp(pEntry->pszDcPart, pszQueryTerm))
            {
                bIsMatch = TRUE;
            }
            break;
        case LSA_AD_BATCH_QUERY_TYPE_BY_SID:
            if (AD_MatchesDomainSid(pEntry->pszDomainSid,
                                    pEntry->sDomainSidLength,
                                    pszQueryTerm))
            {
                bIsMatch = TRUE;
            }
            break;
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
    return ADLdap_FindObjectsByList(hProvider,
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
    return ADLdap_FindObjectsByList(hProvider,
                                    LSA_AD_BATCH_QUERY_TYPE_BY_SID,
                                    dwCount, ppszSidList,
                                    pdwCount, pppObjects);
}

static
DWORD
ADLdap_FindObjectsByList(
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
    PDLINKEDLIST pDomainList = NULL;
    PDLINKEDLIST pNode = NULL;
    PLSA_AD_BATCH_DOMAIN_ENTRY pNewEntry = NULL;
    DWORD dwObjectsCount = 0;
    PAD_SECURITY_OBJECT* ppObjects = NULL;
    DWORD dwCurrentIndex = 0;

    for (i = 0; i < dwQueryItemsCount; i++)
    {
        PCSTR pszQueryTerm = NULL;
        PLSA_AD_BATCH_DOMAIN_ENTRY pFoundEntry = NULL;

        dwError = ADBatch_GetQueryTerm(QueryType, ppszQueryList[i],
                                       &pszQueryTerm);
        BAIL_ON_LSA_ERROR(dwError);

        for (pNode = pDomainList; pNode; pNode = pNode->pNext)
        {
            PLSA_AD_BATCH_DOMAIN_ENTRY pEntry = (PLSA_AD_BATCH_DOMAIN_ENTRY)pNode->pItem;
            BOOLEAN bIsMatch = FALSE;

            dwError = ADBatch_MatchQuery(QueryType, pszQueryTerm,
                                         pEntry, &bIsMatch);
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
                            &pNewEntry,
                            QueryType,
                            pszQueryTerm);
            if (LSA_ERROR_NO_SUCH_DOMAIN == dwError)
            {
                LSA_LOG_DEBUG("Domain not found for query item - '%s'", pszQueryTerm);
                continue;
            }
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaDLinkedListPrepend(&pDomainList, pNewEntry);
            BAIL_ON_LSA_ERROR(dwError);

            pFoundEntry = pNewEntry;
            pNewEntry = NULL;
        }

        // Use prepend because it is faster in this linked list
        // implementation.
        dwError = LsaDLinkedListPrepend(&pFoundEntry->pQueryList, ppszQueryList[i]);
        BAIL_ON_LSA_ERROR(dwError);

        pFoundEntry->dwQueryItemsCount++;
        pFoundEntry = NULL;
    }

    for (pNode = pDomainList; pNode; pNode = pNode->pNext)
    {
        PLSA_AD_BATCH_DOMAIN_ENTRY pEntry = (PLSA_AD_BATCH_DOMAIN_ENTRY)pNode->pItem;

        if (pEntry->bSkip)
        {
            continue;
        }

        dwError = AD_FindObjectsByListForDomain(
                      hProvider,
                      QueryType,
                      pEntry);
        BAIL_ON_LSA_ERROR(dwError);

        dwObjectsCount += pEntry->dwObjectsCount;
    }

    dwError = LsaAllocateMemory(
                dwObjectsCount * sizeof(*ppObjects),
                (PVOID*)&ppObjects);
    BAIL_ON_LSA_ERROR(dwError);

    // Combine results
    dwCurrentIndex = 0;
    for (pNode = pDomainList; pNode; pNode = pNode->pNext)
    {
        PLSA_AD_BATCH_DOMAIN_ENTRY pEntry = (PLSA_AD_BATCH_DOMAIN_ENTRY)pNode->pItem;

        if (pEntry->bSkip)
        {
            continue;
        }

        memcpy(ppObjects + dwCurrentIndex,
               pEntry->ppObjects,
               sizeof(*pEntry->ppObjects) * pEntry->dwObjectsCount);

        memset(pEntry->ppObjects,
               0,
               sizeof(*pEntry->ppObjects) * pEntry->dwObjectsCount);

        dwCurrentIndex += pEntry->dwObjectsCount;
        pEntry->dwObjectsCount = 0;
    }

    *pdwObjectsCount = dwObjectsCount;
    *pppObjects = ppObjects;

cleanup:
    DestroyBatchDomainEntry(&pNewEntry);

    for (pNode = pDomainList; pNode; pNode = pNode->pNext)
    {
        PLSA_AD_BATCH_DOMAIN_ENTRY pEntry = (PLSA_AD_BATCH_DOMAIN_ENTRY)pNode->pItem;
        DestroyBatchDomainEntry(&pEntry);
    }
    LsaDLinkedListFree(pDomainList);

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
                pEntry->bIsOneWayTrust,
                pEntry->dwQueryItemsCount,
                pEntry->pQueryList,
                &pEntry->dwObjectsCount,
                &pEntry->ppObjects);
}

static
DWORD
AD_FindObjectsByListForDomainInternal(
    IN HANDLE hProvider,
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN PCSTR pszDnsDomainName,
    IN BOOLEAN bIsOneWayTrust,
    IN DWORD dwCount,
    // List of PCSTR
    IN PDLINKEDLIST pQueryList,
    OUT PDWORD pdwCount,
    OUT PAD_SECURITY_OBJECT** pppObjects
    )
{
    DWORD dwError = 0;
    PDLINKEDLIST pNode = NULL;
    PDLINKEDLIST pBatchItemList = NULL;
    PLSA_AD_BATCH_ITEM pNewBatchItem = NULL;
    PLSA_AD_BATCH_MESSAGES pRealMessages = NULL;
    PLSA_AD_BATCH_MESSAGES pPseudoMessages = NULL;
    PAD_SECURITY_OBJECT* ppObjects = NULL;
    size_t sIndex = 0;

    if (bIsOneWayTrust && LSA_AD_BATCH_QUERY_TYPE_BY_DN == QueryType)
    {
        // This should never happen, this domain should already be skipped.
        dwError = LSA_ERROR_INTERNAL;
        BAIL_ON_LSA_ERROR(dwError);
    }

    for (pNode = pQueryList; pNode; pNode = pNode->pNext)
    {
        PCSTR pszQuery = (PCSTR)pNode->pItem;

        dwError = CreateBatchItem(&pNewBatchItem, pszQuery);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaDLinkedListAppend(&pBatchItemList, pNewBatchItem);
        BAIL_ON_LSA_ERROR(dwError);
        pNewBatchItem = NULL;
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
                    pBatchItemList,
                    &pRealMessages);
        BAIL_ON_LSA_ERROR(dwError);
    }

    // If Default schema, real objects already have the pseudo information
    // If Unprovisioned mode, no need to get PseudoMessages.
    if (!(((gpADProviderData->dwDirectoryMode == DEFAULT_MODE) &&
           (gpADProviderData->adConfigurationMode == SchemaMode)) ||
          (gpADProviderData->dwDirectoryMode == UNPROVISIONED_MODE)))
    {
        dwError = AD_ResolvePseudoObjectsByRealObjects(
                        hProvider,
                        pszDnsDomainName,
                        dwCount,
                        pBatchItemList,
                        &pPseudoMessages);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaAllocateMemory(
                    sizeof(PAD_SECURITY_OBJECT) * dwCount,
                    (PVOID*)&ppObjects);
    BAIL_ON_LSA_ERROR(dwError);

    // Marshalling
    sIndex = 0;
    for (pNode = pBatchItemList; pNode; pNode = pNode->pNext)
    {
        PLSA_AD_BATCH_ITEM pBatchItem = (PLSA_AD_BATCH_ITEM)pNode->pItem;

        dwError = AD_MarshalBatchItem(
                        pszDnsDomainName,
                        bIsOneWayTrust,
                        pBatchItem,
                        pRealMessages ? pRealMessages->hLdapHandle : (HANDLE)NULL,
                        pPseudoMessages ? pPseudoMessages->hLdapHandle : (pRealMessages ? pRealMessages->hLdapHandle : (HANDLE)NULL),
                        &ppObjects[sIndex]);
         BAIL_ON_LSA_ERROR(dwError);

         if (ppObjects[sIndex])
         {
            sIndex++;
         }
    }

    *pppObjects = ppObjects;
    *pdwCount = (DWORD)sIndex;

cleanup:
    DestroyBatchItem(&pNewBatchItem);

    for (pNode = pBatchItemList; pNode; pNode = pNode->pNext)
    {
        PLSA_AD_BATCH_ITEM pEntry = (PLSA_AD_BATCH_ITEM)pNode->pItem;
        DestroyBatchItem(&pEntry);
    }
    LsaDLinkedListFree(pBatchItemList);

    DestroyBatchMessages(&pRealMessages);
    DestroyBatchMessages(&pPseudoMessages);

    return dwError;

error:
    *pppObjects = NULL;
    *pdwCount = 0;

    ADCacheDB_SafeFreeObjectList(sIndex, &ppObjects);

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
    IN PDLINKEDLIST pBatchItemList,
    OUT PDLINKEDLIST* ppNextBatchItemList,
    IN DWORD dwMaxQueryCount,
    OUT PDWORD pdwQueryCount,
    OUT PSTR** pppszQueryList
    )
{
    DWORD dwError = 0;
    PDLINKEDLIST pCurrentItem = NULL;
    PSTR* ppszQueryList = NULL;
    PDLINKEDLIST pLastItem = pBatchItemList;
    DWORD dwQueryCount = 0;
    DWORD dwSavedQueryCount = 0;

    pCurrentItem = pBatchItemList;
    while (pCurrentItem)
    {
        PLSA_AD_BATCH_ITEM pEntry = (PLSA_AD_BATCH_ITEM)pCurrentItem->pItem;

        if (pEntry && !IsNullOrEmptyString(pEntry->pszQuery))
        {
            DWORD dwNewQueryCount = dwQueryCount + 1;

            if (dwMaxQueryCount && (dwNewQueryCount > dwMaxQueryCount))
            {
                break;
            }
            dwQueryCount = dwNewQueryCount;
        }

        pCurrentItem = pCurrentItem->pNext;
    }
    pLastItem = pCurrentItem;
    dwSavedQueryCount = dwQueryCount;

    if (dwQueryCount < 1)
    {
        goto cleanup;
    }

    dwError = LsaAllocateMemory(dwQueryCount*sizeof(*ppszQueryList), (PVOID*)&ppszQueryList);
    BAIL_ON_LSA_ERROR(dwError);

    dwQueryCount = 0;
    pCurrentItem = pBatchItemList;
    while (pCurrentItem != pLastItem)
    {
        PLSA_AD_BATCH_ITEM pEntry = (PLSA_AD_BATCH_ITEM)pCurrentItem->pItem;

        if (pEntry && !IsNullOrEmptyString(pEntry->pszQuery))
        {
            dwError = LsaAllocateString(pEntry->pszQuery,
                                        &ppszQueryList[dwQueryCount]);
            BAIL_ON_LSA_ERROR(dwError);
            dwQueryCount++;
        }

        pCurrentItem = pCurrentItem->pNext;
    }

    assert(dwSavedQueryCount == dwQueryCount);


cleanup:
    // We handle error here instead of error label
    // because there is a goto cleanup above.
    if (dwError)
    {
        LsaFreeStringArray(ppszQueryList,dwSavedQueryCount);
        dwQueryCount = 0;
        dwSavedQueryCount = 0;
        pLastItem = pBatchItemList;
    }

    *pppszQueryList = ppszQueryList;
    *pdwQueryCount = dwQueryCount;
    *ppNextBatchItemList = pLastItem;

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
    IN OUT PDLINKEDLIST pBatchItemList
    )
{
    DWORD dwError = 0;
    PDLINKEDLIST pCurrentBatchItemList = pBatchItemList;
    PDLINKEDLIST pNextBatchItemList = NULL;
    DWORD dwMaxQueryCount = AD_GetMaxQueryCount();
    PSTR* ppszSidList = NULL;
    PLSA_TRANSLATED_NAME_OR_SID* ppTranslatedNames = NULL;
    DWORD dwQueryCount = 0;
    DWORD dwCount = 0;
    DWORD i = 0;

    while (pCurrentBatchItemList)
    {
        LsaFreeStringArray(ppszSidList, dwQueryCount);
        ppszSidList = NULL;

        if (ppTranslatedNames)
        {
            LsaFreeTranslatedNameList(ppTranslatedNames, dwCount);
        }
        ppTranslatedNames = NULL;

        dwError = AD_BuildBatchQueryForRealRpc(
                    pCurrentBatchItemList,
                    &pNextBatchItemList,
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
            pCurrentBatchItemList = pNextBatchItemList;
            pNextBatchItemList = NULL;

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
                         pCurrentBatchItemList,
                         pNextBatchItemList,
                         ppszSidList[i],
                         ppTranslatedNames[i]);
            BAIL_ON_LSA_ERROR(dwError);
        }

        pCurrentBatchItemList = pNextBatchItemList;
        pNextBatchItemList = NULL;
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
    IN OUT PDLINKEDLIST pBatchItemList,
    OUT PLSA_AD_BATCH_MESSAGES* ppMessages
    )
{
    DWORD dwError = 0;
    HANDLE hDirectory = 0;
    LDAP* pLd = NULL;
    PSTR pszScopeDn = NULL;
    PSTR pszQuery = 0;
    PSTR szAttributeList[] =
    {
        AD_LDAP_OBJECTCLASS_TAG,
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
        AD_LDAP_DISPLAY_NAME_TAG,
        NULL
    };
    PLSA_AD_BATCH_MESSAGES pMessages = NULL;
    LDAPMessage* pMessage = NULL;
    LDAPMessage* pFreeMessage = NULL;
    PDLINKEDLIST pCurrentBatchItemList = pBatchItemList;
    PDLINKEDLIST pNextBatchItemList = NULL;
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

    dwError = CreateBatchMessages(&pMessages, hDirectory);
    BAIL_ON_LSA_ERROR(dwError);
    hDirectory = 0;

    while (pCurrentBatchItemList)
    {
        DWORD dwQueryCount = 0;
        DWORD dwCount = 0;
        LDAPMessage* pCurrentMessage = NULL;

        LSA_SAFE_FREE_STRING(pszQuery);

        dwError = AD_BuildBatchQueryForReal(
                    QueryType,
                    pCurrentBatchItemList,
                    &pNextBatchItemList,
                    dwMaxQuerySize,
                    dwMaxQueryCount,
                    &dwQueryCount,
                    &pszQuery);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaLdapDirectorySearch(
                   pMessages->hLdapHandle,
                   pszScopeDn,
                   LDAP_SCOPE_SUBTREE,
                   pszQuery,
                   szAttributeList,
                   &pFreeMessage);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaDLinkedListPrepend(&pMessages->pLdapMessageList,
                                        pFreeMessage);
        BAIL_ON_LSA_ERROR(dwError);
        pMessage = pFreeMessage;
        pFreeMessage = NULL;

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
            pCurrentBatchItemList = pNextBatchItemList;
            pNextBatchItemList = NULL;

            continue;
        }

        pCurrentMessage = ldap_first_entry(pLd, pMessage);
        while (pCurrentMessage)
        {
            dwError = AD_RecordRealObject(
                        QueryType,
                        pCurrentBatchItemList,
                        pNextBatchItemList,
                        pMessages->hLdapHandle,
                        pCurrentMessage);
            BAIL_ON_LSA_ERROR(dwError);

            pCurrentMessage = ldap_next_entry(pLd, pCurrentMessage);
        }

        pCurrentBatchItemList = pNextBatchItemList;
        pNextBatchItemList = NULL;
    }

    *ppMessages = pMessages;

cleanup:
    LsaLdapCloseDirectory(hDirectory);
    LSA_SAFE_FREE_STRING(pszScopeDn);
    LSA_SAFE_FREE_STRING(pszQuery);
    if (pFreeMessage)
    {
        ldap_msgfree(pFreeMessage);
    }
    return dwError;

error:
    *ppMessages = NULL;
    DestroyBatchMessages(&pMessages);

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
    while (pCurrentItem)
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
AD_BuildBatchQueryFromBatchItemListGetQueryValue(
    IN PVOID pCallbackContext,
    IN PVOID pItem,
    OUT PCSTR* ppszValue,
    OUT PVOID* ppFreeContext
    )
{
    DWORD dwError = 0;
    LSA_AD_BATCH_QUERY_TYPE QueryType = (LSA_AD_BATCH_QUERY_TYPE)pCallbackContext;
    PDLINKEDLIST pNode = (PDLINKEDLIST)pItem;
    PLSA_AD_BATCH_ITEM pBatchItem = (PLSA_AD_BATCH_ITEM)pNode->pItem;
    PSTR pszValue = NULL;
    PVOID pFreeContext = NULL;

    switch (QueryType)
    {
        case LSA_AD_BATCH_QUERY_TYPE_BY_DN:
            dwError = LsaLdapEscapeString(&pszValue, pBatchItem->pszQuery);
            BAIL_ON_LSA_ERROR(dwError);

            pFreeContext = pszValue;

            break;

        case LSA_AD_BATCH_QUERY_TYPE_BY_SID:
            pszValue = (PSTR)pBatchItem->pszQuery;
            break;

        default:
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
AD_BuildBatchQueryFromBatchItemListGetSidValue(
    IN PVOID pCallbackContext,
    IN PVOID pItem,
    OUT PCSTR* ppszValue,
    OUT PVOID* ppFreeContext
    )
{
    PDLINKEDLIST pNode = (PDLINKEDLIST)pItem;
    PLSA_AD_BATCH_ITEM pBatchItem = (PLSA_AD_BATCH_ITEM)pNode->pItem;

    *ppszValue = pBatchItem->bFound ? NULL : pBatchItem->pszSid;
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
    PDLINKEDLIST pNode = (PDLINKEDLIST)pItem;
    return pNode->pNext;
}

static
DWORD
AD_BuildBatchQueryForReal(
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    // List of PLSA_AD_BATCH_ITEM
    IN PDLINKEDLIST pBatchItemList,
    OUT PDLINKEDLIST* ppNextBatchItemList,
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

    dwError = AD_BuildBatchQuery(pszPrefix,
                              ")",
                              pszAttributeName,
                              pBatchItemList,
                              (PVOID*)ppNextBatchItemList,
                              (PVOID)QueryType,
                              AD_BuildBatchQueryFromBatchItemListGetQueryValue,
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
    *ppNextBatchItemList = pBatchItemList;
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
    IN PDLINKEDLIST pBatchItemList,
    OUT PDLINKEDLIST* ppNextBatchItemList,
    IN DWORD dwMaxQuerySize,
    IN DWORD dwMaxQueryCount,
    OUT PDWORD pdwQueryCount,
    OUT PSTR* ppszQuery
    )
{
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

    return AD_BuildBatchQuery(
                              pszPrefix,
                              ")",
                              "keywords=backLink",
                              pBatchItemList,
                              (PVOID*)ppNextBatchItemList,
                              NULL,
                              AD_BuildBatchQueryFromBatchItemListGetSidValue,
                              NULL,
                              AD_BuildBatchQueryFromBatchItemListNextItem,
                              dwMaxQuerySize,
                              dwMaxQueryCount,
                              pdwQueryCount,
                              ppszQuery);
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
AD_RecordRealObjectFromRpc(
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN OUT PDLINKEDLIST pStartBatchItemList,
    IN PDLINKEDLIST pEndBatchItemList,
    IN PSTR pszObjectSid,
    IN PLSA_TRANSLATED_NAME_OR_SID pTranslatedName
    )
{
    DWORD dwError = 0;
    PSTR pszSid = NULL;
    PSTR pszSamAccountName = NULL;
    ADAccountType objectType = 0;
    PDLINKEDLIST pBatchItemNode = NULL;
    PCSTR pszCompare = NULL;
    PLSA_LOGIN_NAME_INFO pLoginNameInfo = NULL;

    BAIL_ON_INVALID_POINTER(pTranslatedName);

    dwError = LsaAllocateString(
                         pszObjectSid,
                         &pszSid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaCrackDomainQualifiedName(
                         pTranslatedName->pszNT4NameOrSid,
                         gpADProviderData->szDomain,
                         &pLoginNameInfo);
    BAIL_ON_LSA_ERROR(dwError);

    BAIL_ON_INVALID_POINTER(pLoginNameInfo);
    dwError = LsaAllocateString(
                         pLoginNameInfo->pszName,
                         &pszSamAccountName);
    BAIL_ON_LSA_ERROR(dwError);

    objectType = pTranslatedName->ObjectType;

    switch (QueryType)
    {
        case LSA_AD_BATCH_QUERY_TYPE_BY_SID:
            pszCompare = pszSid;
            break;

        default:
            dwError = LSA_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
    }

    if (objectType != AccountType_User && objectType != AccountType_Group)
    {
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    for (pBatchItemNode = pStartBatchItemList;
         pBatchItemNode != pEndBatchItemList;
         pBatchItemNode = pBatchItemNode->pNext)
    {
        PLSA_AD_BATCH_ITEM pItem = (PLSA_AD_BATCH_ITEM)pBatchItemNode->pItem;

        if (strcasecmp(pItem->pszQuery, pszCompare))
        {
            continue;
        }

        pItem->pszSid = pszSid;
        pItem->pszSamAccountName = pszSamAccountName;
        pItem->objectType = objectType;
        // Setting the conf mode to the provider's conf mode is valid
        // until we do linked list processing where we then determine
        // the conf mode for that particular cell.
        pItem->adConfMode = gpADProviderData->adConfigurationMode;
        pszSid = NULL;
        pszSamAccountName = NULL;

        break;
    }

cleanup:

    LSA_SAFE_FREE_STRING(pszSid);
    LSA_SAFE_FREE_STRING(pszSamAccountName);
    LSA_SAFE_FREE_LOGIN_NAME_INFO(pLoginNameInfo);

    return dwError;

error:

    goto cleanup;
}

static
DWORD
AD_RecordRealObject(
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN OUT PDLINKEDLIST pStartBatchItemList,
    IN PDLINKEDLIST pEndBatchItemList,
    IN HANDLE hDirectory,
    IN LDAPMessage* pMessage
    )
{
    DWORD dwError = 0;
    PSTR pszDn = NULL;
    PSTR pszSid = NULL;
    PSTR pszSamAccountName = NULL;
    PDLINKEDLIST pBatchItemNode = NULL;
    DWORD iValue = 0;
    ADAccountType objectType = 0;
    PSTR* ppszValues = NULL;
    DWORD dwNumValues = 0;
    PCSTR pszCompare = NULL;

    dwError = LsaLdapGetDN(
                     hDirectory,
                     pMessage,
                     &pszDn);
    BAIL_ON_LSA_ERROR(dwError);

    if (IsNullOrEmptyString(pszDn))
    {
        dwError = LSA_ERROR_LDAP_FAILED_GETDN;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = ADLdap_GetObjectSid(
                    hDirectory,
                    pMessage,
                    &pszSid);
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
            dwError = LSA_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaLdapGetString(
                    hDirectory,
                    pMessage,
                    AD_LDAP_SAM_NAME_TAG,
                    &pszSamAccountName);
    BAIL_ON_LSA_ERROR(dwError);

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
           objectType = AccountType_User;
           break;
        }
        else if (!strncasecmp(ppszValues[iValue], "group", sizeof("group")-1))
        {
            objectType = AccountType_Group;
            break;
        }
    }

    if (objectType != AccountType_User && objectType != AccountType_Group)
    {
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    for (pBatchItemNode = pStartBatchItemList;
         pBatchItemNode != pEndBatchItemList;
         pBatchItemNode = pBatchItemNode->pNext)
    {
        PLSA_AD_BATCH_ITEM pItem = (PLSA_AD_BATCH_ITEM)pBatchItemNode->pItem;

        if (strcasecmp(pItem->pszQuery, pszCompare))
        {
            continue;
        }

        pItem->pszSid = pszSid;
        pItem->pszSamAccountName = pszSamAccountName;
        pItem->objectType = objectType;
        // Setting the conf mode to the provider's conf mode is valid
        // until we do linked list processing where we then determine
        // the conf mode for that particular cell.
        pItem->adConfMode = gpADProviderData->adConfigurationMode;
        pszSid = NULL;
        pszSamAccountName = NULL;

        pItem->pRealMessage = pMessage;
        break;
    }

cleanup:
    LSA_SAFE_FREE_STRING(pszDn);
    LSA_SAFE_FREE_STRING(pszSid);
    LSA_SAFE_FREE_STRING(pszSamAccountName);

    if (ppszValues) {
        LsaFreeStringArray(ppszValues, dwNumValues);
    }

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
    IN OUT PDLINKEDLIST pBatchItemList,
    OUT PLSA_AD_BATCH_MESSAGES* ppMessages
    )
{
    DWORD dwError = 0;
    PLSA_AD_BATCH_MESSAGES pMessages = NULL;

    if (gpADProviderData->dwDirectoryMode == CELL_MODE)
    {
        dwError = AD_ResolvePseudoObjectsByRealObjectsWithLinkedCell(
                     hProvider,
                     dwTotalItemCount,
                     pBatchItemList,
                     &pMessages);
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
                     &pMessages);
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppMessages = pMessages;

cleanup:
    return dwError;

error:
    *ppMessages = NULL;
    DestroyBatchMessages(&pMessages);

    goto cleanup;
}

static
DWORD
AD_ResolvePseudoObjectsByRealObjectsWithLinkedCell(
    IN HANDLE hProvider,
    IN DWORD dwTotalItemCount,
    // List of PLSA_AD_BATCH_ITEM
    IN OUT PDLINKEDLIST pBatchItemList,
    OUT PLSA_AD_BATCH_MESSAGES* ppMessages
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    PLSA_AD_BATCH_MESSAGES pMessages = NULL;
    PDLINKEDLIST pCellNode = NULL;
    DWORD dwTotalFoundCount = 0;
    DWORD dwFoundInCellCount = 0;

    dwError = AD_ResolvePseudoObjectsByRealObjectsInternal(
                hProvider,
                NULL,
                gpADProviderData->cell.szCellDN,
                (gpADProviderData->adConfigurationMode == SchemaMode),
                pBatchItemList,
                &dwFoundInCellCount,
                &pMessages);
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
                    &pMessages);
        BAIL_ON_LSA_ERROR(dwError);

        dwTotalFoundCount += dwFoundInCellCount;
    }

    *ppMessages = pMessages;

cleanup:
    return dwError;

error:
    *ppMessages = NULL;
    DestroyBatchMessages(&pMessages);

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
    IN OUT PDLINKEDLIST pBatchItemList,
    OUT OPTIONAL PDWORD pdwTotalItemFoundCount,
    IN OUT PLSA_AD_BATCH_MESSAGES* ppMessages
    )
{
    DWORD dwError = 0;
    HANDLE hDirectory = 0;
    LDAP* pLd = NULL;
    PSTR pszScopeDn = NULL;
    PSTR pszQuery = 0;
    PSTR szAttributeList[] =
    {
        AD_LDAP_UID_TAG,
        AD_LDAP_GID_TAG,
        AD_LDAP_NAME_TAG,
        AD_LDAP_PASSWD_TAG,
        AD_LDAP_KEYWORDS_TAG,
        AD_LDAP_HOMEDIR_TAG,
        AD_LDAP_SHELL_TAG,
        AD_LDAP_GECOS_TAG,
        AD_LDAP_SEC_DESC_TAG,
        AD_LDAP_ALIAS_TAG,
        AD_LDAP_DISPLAY_NAME_TAG,
        NULL
    };
    PLSA_AD_BATCH_MESSAGES pMessages = *ppMessages;
    LDAPMessage* pMessage = NULL;
    LDAPMessage* pFreeMessage = NULL;
    PDLINKEDLIST pCurrentBatchItemList = pBatchItemList;
    PDLINKEDLIST pNextBatchItemList = NULL;
    PSTR pszDomainName = NULL;
    DWORD dwTotalItemFoundCount = 0;
    DWORD dwMaxQuerySize = AD_GetMaxQuerySize();
    DWORD dwMaxQueryCount = AD_GetMaxQueryCount();

    dwError = AD_BuildBatchQueryScopeForPseudoBySid(
                    bIsSchemaMode,
                    gpADProviderData->dwDirectoryMode,
                    pszDnsDomainName,
                    pszCellDn,
                    &pszScopeDn);
    BAIL_ON_LSA_ERROR(dwError);

    if (!pMessages)
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

        dwError = CreateBatchMessages(&pMessages, hDirectory);
        BAIL_ON_LSA_ERROR(dwError);
        hDirectory = 0;
    }

    pLd = LsaLdapGetSession(pMessages->hLdapHandle);

    while (pCurrentBatchItemList)
    {
        DWORD dwQueryCount = 0;
        DWORD dwCount = 0;
        LDAPMessage* pCurrentMessage = NULL;

        LSA_SAFE_FREE_STRING(pszQuery);

        dwError = AD_BuildBatchQueryForPseudoBySid(
                     bIsSchemaMode,
                     pCurrentBatchItemList,
                     &pNextBatchItemList,
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
                   pMessages->hLdapHandle,
                   pszScopeDn,
                   LDAP_SCOPE_SUBTREE,
                   pszQuery,
                   szAttributeList,
                   &pFreeMessage);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaDLinkedListPrepend(&pMessages->pLdapMessageList,
                                        pFreeMessage);
        BAIL_ON_LSA_ERROR(dwError);
        pMessage = pFreeMessage;
        pFreeMessage = NULL;

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
            pCurrentBatchItemList = pNextBatchItemList;
            pNextBatchItemList = NULL;

            continue;
        }

        dwTotalItemFoundCount += dwCount;

        pCurrentMessage = ldap_first_entry(pLd, pMessage);
        while (pCurrentMessage)
        {
            dwError = AD_RecordPseudoObjectBySid(
                        pCurrentBatchItemList,
                        pNextBatchItemList,
                        pMessages->hLdapHandle,
                        bIsSchemaMode ? SchemaMode : NonSchemaMode,
                        pCurrentMessage);
            BAIL_ON_LSA_ERROR(dwError);

            pCurrentMessage = ldap_next_entry(pLd, pCurrentMessage);
        }

        pCurrentBatchItemList = pNextBatchItemList;
        pNextBatchItemList = NULL;
    }

    *ppMessages = pMessages;

    if (pdwTotalItemFoundCount)
    {
       *pdwTotalItemFoundCount = dwTotalItemFoundCount;
    }

cleanup:
    LsaLdapCloseDirectory(hDirectory);
    LSA_SAFE_FREE_STRING(pszDomainName);
    LSA_SAFE_FREE_STRING(pszScopeDn);
    LSA_SAFE_FREE_STRING(pszQuery);
    if (pFreeMessage)
    {
        ldap_msgfree(pFreeMessage);
    }
    return dwError;

error:
    *ppMessages = NULL;

    if (pdwTotalItemFoundCount)
    {
       *pdwTotalItemFoundCount = 0;
    }

    DestroyBatchMessages(&pMessages);

    goto cleanup;
}

static
DWORD
AD_RecordPseudoObjectBySid(
    IN OUT PDLINKEDLIST pStartBatchItemList,
    IN PDLINKEDLIST pEndBatchItemList,
    IN HANDLE hDirectory,
    IN ADConfigurationMode adMode,
    IN LDAPMessage* pMessage
    )
{
    DWORD dwError = 0;
    BOOLEAN bIsValid = FALSE;
    PSTR pszDn = NULL;
    PSTR pszSid = NULL;
    PDLINKEDLIST pBatchItemNode = NULL;
    DWORD iValue = 0;
    PSTR* ppszValues = NULL;
    DWORD dwNumValues = 0;
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
                    &ppszValues,
                    &dwNumValues);
    BAIL_ON_LSA_ERROR(dwError);

    for (iValue = 0; iValue < dwNumValues; iValue++)
    {
        if (!strncasecmp(ppszValues[iValue],
                         "backLink=",
                         sizeof("backLink=")-1))
        {
            pszObjectSid = ppszValues[iValue] + sizeof("backLink=") - 1;
            break;
        }
    }

    if (IsNullOrEmptyString(pszObjectSid))
    {
        dwError = LSA_ERROR_INVALID_SID;
        BAIL_ON_LSA_ERROR(dwError);
    }

    for (pBatchItemNode = pStartBatchItemList;
         pBatchItemNode != pEndBatchItemList;
         pBatchItemNode = pBatchItemNode->pNext)
    {
        PLSA_AD_BATCH_ITEM pItem = (PLSA_AD_BATCH_ITEM)pBatchItemNode->pItem;

        if (AccountType_NotFound == pItem->objectType)
        {
            continue;
        }

        if (strcasecmp(pItem->pszSid, pszObjectSid))
        {
            continue;
        }

        pItem->pPseudoMessage = pMessage;
        pItem->bFound = TRUE;
        pItem->adConfMode = adMode;

        break;
    }

cleanup:
    LSA_SAFE_FREE_STRING(pszDn);
    LSA_SAFE_FREE_STRING(pszSid);
    LsaFreeStringArray(ppszValues, dwNumValues);

    return dwError;

error:
    goto cleanup;
}

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
