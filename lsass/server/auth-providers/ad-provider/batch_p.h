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
 *        batch_p.h
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
#ifndef __BATCH_P_H__
#define __BATCH_P_H__

#include "batch_common.h"

static
DWORD
LsaAdBatchFindObjects(
    IN HANDLE hProvider,
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN DWORD dwQueryItemsCount,
    IN PSTR* ppszQueryList,
    OUT PDWORD pdwObjectsCount,
    OUT PAD_SECURITY_OBJECT** pppObjects
    );

static
DWORD
CreateBatchDomainEntry(
    OUT PLSA_AD_BATCH_DOMAIN_ENTRY* ppEntry,
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN PCSTR pszQueryTerm
    );

static
VOID
DestroyBatchDomainEntry(
    IN OUT PLSA_AD_BATCH_DOMAIN_ENTRY* ppEntry
    );

static
DWORD
CreateBatchItem(
    OUT PLSA_AD_BATCH_ITEM* ppItem,
    IN PLSA_AD_BATCH_DOMAIN_ENTRY pDomainEntry,
    IN LSA_AD_BATCH_QUERY_TYPE QueryTermType,
    IN OPTIONAL PCSTR pszString,
    IN OPTIONAL PDWORD pdwId
    );

static
VOID
DestroyBatchItem(
    IN OUT PLSA_AD_BATCH_ITEM* ppItem
    );

static
DWORD
GetDomainEntryType(
    IN PCSTR pszDomainName,
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN OPTIONAL PCSTR pszDomainDN,
    OUT PBOOLEAN pbSkip,
    OUT PBOOLEAN pbIsOneWayTrust
    );

static
DWORD
CheckDomainModeCompatibility(
    IN PCSTR pszDnsDomainName,
    IN BOOLEAN bIsExternalTrust,
    IN OPTIONAL PCSTR pszDomainDN
    );

static
DWORD
GetCellConfigurationMode(
    IN PCSTR pszDnsDomainName,
    IN PCSTR pszCellDN,
    OUT ADConfigurationMode* pAdMode
    );

static
DWORD
AD_FindObjectsByListForDomain(
    IN HANDLE hProvider,
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN OUT PLSA_AD_BATCH_DOMAIN_ENTRY pEntry
    );

static
DWORD
AD_FindObjectsByListForDomainInternal(
    IN HANDLE hProvider,
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN PCSTR pszDnsDomainName,
    IN BOOLEAN bIsOneWayTrust,
    IN DWORD dwCount,
    IN OUT PLSA_LIST_LINKS pBatchItemList
    );

// Fills in real and SIDs.
static
DWORD
LsaAdBatchResolveRpcObjects(
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN PCSTR pszDnsDomainName,
    IN DWORD dwTotalItemCount,
    // List of PLSA_AD_BATCH_ITEM
    IN OUT PLSA_LIST_LINKS pBatchItemList
    );

static
DWORD
LsaAdBatchResolveRealObjects(
    IN HANDLE hProvider,
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN PCSTR pszDnsDomainName,
    IN DWORD dwTotalItemCount,
    // List of PLSA_AD_BATCH_ITEM
    IN OUT PLSA_LIST_LINKS pBatchItemList
    );

static
DWORD
LsaAdBatchResolvePseudoObjects(
    IN HANDLE hProvider,
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN PCSTR pszDnsDomainName,
    IN DWORD dwTotalItemCount,
    // List of PLSA_AD_BATCH_ITEM
    IN OUT PLSA_LIST_LINKS pBatchItemList
    );

static
DWORD
LsaAdBatchResolvePseudoObjectsWithLinkedCells(
    IN HANDLE hProvider,
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN DWORD dwTotalItemCount,
    // List of PLSA_AD_BATCH_ITEM
    IN OUT PLSA_LIST_LINKS pBatchItemList
    );

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
    );

// zero means unlimited
#define BUILD_BATCH_QUERY_MAX_SIZE 0
#define BUILD_BATCH_QUERY_MAX_COUNT 1000

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
    );

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
    );

static
DWORD
LsaAdBatchProcessRpcObject(
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    // List of PLSA_AD_BATCH_ITEM
    IN OUT PLSA_LIST_LINKS pStartBatchItemListLinks,
    IN PLSA_LIST_LINKS pEndBatchItemListLinks,
    IN PSTR pszObjectSid,
    IN PLSA_TRANSLATED_NAME_OR_SID pTranslatedName
    );

static
DWORD
LsaAdBatchProcessRealObject(
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    // List of PLSA_AD_BATCH_ITEM
    IN OUT PLSA_LIST_LINKS pStartBatchItemListLinks,
    IN PLSA_LIST_LINKS pEndBatchItemListLinks,
    IN HANDLE hDirectory,
    IN LDAPMessage* pMessage
    );

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
    );

typedef DWORD LSA_PROVISIONING_MODE, *PLSA_PROVISIONING_MODE;
#define LSA_PROVISIONING_MODE_DEFAULT_CELL     1
#define LSA_PROVISIONING_MODE_NON_DEFAULT_CELL 2
#define LSA_PROVISIONING_MODE_UNPROVISIONED    3

static
DWORD
LsaAdBatchAccountTypeToObjectType(
    IN ADAccountType AccountType,
    OUT PLSA_AD_BATCH_OBJECT_TYPE pObjectType
    );

static
LSA_AD_BATCH_OBJECT_TYPE
LsaAdBatchGetObjectTypeFromQueryType(
    IN LSA_AD_BATCH_QUERY_TYPE QueryType
    );

static
BOOLEAN
LsaAdBatchHasValidCharsForSid(
    IN PCSTR pszSidString
    );

#endif /* __BATCH_P_H__ */
