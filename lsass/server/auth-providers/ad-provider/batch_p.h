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
 *        batch.h
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

typedef size_t LSA_AD_BATCH_QUERY_TYPE;

#define LSA_AD_BATCH_QUERY_TYPE_UNDEFINED 0
#define LSA_AD_BATCH_QUERY_TYPE_BY_DN     1
#define LSA_AD_BATCH_QUERY_TYPE_BY_SID    2

static
DWORD
ADLdap_FindObjectsByList(
    IN HANDLE hProvider,
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN DWORD dwQueryItemsCount,
    IN PSTR* ppszQueryList,
    OUT PDWORD pdwObjectsCount,
    OUT PAD_SECURITY_OBJECT** pppObjects
    );

typedef struct _LSA_AD_BATCH_DOMAIN_ENTRY {
    PSTR pszDnsDomainName;

    // If this is set, we are not supposed to process
    // this domain.
    BOOLEAN bSkip;

    // If this is set, we are dealing with one-way trust scenario
    BOOLEAN bIsOneWayTrust;

    // Either pszDcPart of pszDomainSid is used
    // depending on the query type.

    // pszDcPart is not allocated, but points to
    // strings that last longer than this structure.
    PCSTR pszDcPart;

    // Allocated
    PSTR pszDomainSid;
    size_t sDomainSidLength;

    // Number of query items
    DWORD dwQueryItemsCount;
    // List of PCSTR
    PDLINKEDLIST pQueryList;

    // Results
    DWORD dwObjectsCount;
    PAD_SECURITY_OBJECT* ppObjects;
} LSA_AD_BATCH_DOMAIN_ENTRY, *PLSA_AD_BATCH_DOMAIN_ENTRY;

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

typedef struct _LSA_AD_BATCH_ITEM {
    // pszQuery is not allocated, but points to
    // strings that last longer than this structure.
    PCSTR pszQuery;
    // This needs to be freed.
    PSTR pszSid;
    // This needs to be freed
    PSTR pszSamAccountName;
    // The LDAP messages are not allocated, but point
    // to structures that last longer than this structure.
    LDAPMessage* pRealMessage;
    LDAPMessage* pPseudoMessage;
    ADAccountType objectType;
    ADConfigurationMode adConfMode;
    // bFound is set if and only if
    // The pseudo information for batch item object has been found.
    BOOLEAN bFound;
} LSA_AD_BATCH_ITEM, *PLSA_AD_BATCH_ITEM;

static
DWORD
CreateBatchItem(
    OUT PLSA_AD_BATCH_ITEM* ppItem,
    IN PCSTR pszDn
    );

static
VOID
DestroyBatchItem(
    IN OUT PLSA_AD_BATCH_ITEM* ppItem
    );

typedef struct _LSA_AD_BATCH_MESSAGES {
    HANDLE hLdapHandle;
    // List of LDAPMessage*
    PDLINKEDLIST pLdapMessageList;
} LSA_AD_BATCH_MESSAGES, *PLSA_AD_BATCH_MESSAGES;

static
DWORD
CreateBatchMessages(
    OUT PLSA_AD_BATCH_MESSAGES* ppMessages,
    IN HANDLE hLdapHandle
    );

static
VOID
DestroyBatchMessages(
    IN OUT PLSA_AD_BATCH_MESSAGES* ppMessages
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
    // List of PCSTR
    IN PDLINKEDLIST pQueryList,
    OUT PDWORD pdwCount,
    OUT PAD_SECURITY_OBJECT** pppObjects
    );

// Fills in real and SIDs.

static
DWORD
AD_ResolveRealObjectsByListRpc(
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN PCSTR pszDnsDomainName,
    IN DWORD dwTotalItemCount,
    // List of PLSA_AD_BATCH_ITEM
    IN OUT PDLINKEDLIST pBatchItemList
    );

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
    );

static
DWORD
AD_ResolvePseudoObjectsByRealObjects(
    IN HANDLE hProvider,
    IN PCSTR pszDnsDomainName,
    IN DWORD dwTotalItemCount,
    // List of PLSA_AD_BATCH_ITEM
    IN OUT PDLINKEDLIST pBatchItemList,
    OUT PLSA_AD_BATCH_MESSAGES* ppMessages
    );

static
DWORD
AD_ResolvePseudoObjectsByRealObjectsWithLinkedCell(
    IN HANDLE hProvider,
    IN DWORD dwTotalItemCount,
    // List of PLSA_AD_BATCH_ITEM
    IN OUT PDLINKEDLIST pBatchItemList,
    OUT PLSA_AD_BATCH_MESSAGES* ppMessages
    );

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
    );

// zero means unlimited
#define BUILD_BATCH_QUERY_MAX_SIZE 0
#define BUILD_BATCH_QUERY_MAX_COUNT 1000

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
    );

static
DWORD
AD_BuildBatchQueryForRealRpc(
    IN PDLINKEDLIST pBatchItemList,
    OUT PDLINKEDLIST* ppNextBatchItemList,
    IN DWORD dwMaxQueryCount,
    OUT PDWORD pdwQueryCount,
    OUT PSTR** pppszQueryList
    );

static
DWORD
AD_RecordRealObjectFromRpc(
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN OUT PDLINKEDLIST pStartBatchItemList,
    IN PDLINKEDLIST pEndBatchItemList,
    IN PSTR pszObjectSid,
    IN PLSA_TRANSLATED_NAME_OR_SID pTranslatedName
    );

static
DWORD
AD_RecordRealObject(
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN OUT PDLINKEDLIST pStartBatchItemList,
    IN PDLINKEDLIST pEndBatchItemList,
    IN HANDLE hDirectory,
    IN LDAPMessage* pMessage
    );

static
DWORD
AD_RecordPseudoObjectBySid(
    IN OUT PDLINKEDLIST pStartBatchItemList,
    IN PDLINKEDLIST pEndBatchItemList,
    IN HANDLE hDirectory,
    IN ADConfigurationMode adMode,
    IN LDAPMessage* pMessage
    );

typedef DWORD LSA_PROVISIONING_MODE, *PLSA_PROVISIONING_MODE;

#define LSA_PROVISIONING_MODE_DEFAULT_CELL     1
#define LSA_PROVISIONING_MODE_NON_DEFAULT_CELL 2
#define LSA_PROVISIONING_MODE_UNPROVISIONED    3

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
    );

static
DWORD
AD_BuildBatchQueryScopeForPseudoBySid(
    IN BOOLEAN bIsSchemaMode,
    IN LSA_PROVISIONING_MODE Mode,
    IN OPTIONAL PCSTR pszDnsDomainName,
    IN OPTIONAL PCSTR pszCellDn,
    OUT PSTR* ppszScopeDn
    );

static
DWORD
AD_MarshalBatchItem(
    IN PCSTR pszDnsDomainName,
    IN BOOLEAN bIsOneWayTrust,
    IN PLSA_AD_BATCH_ITEM pItem,
    IN HANDLE hRealLdapHandle,
    IN HANDLE hPseudoLdapHandle,
    OUT PAD_SECURITY_OBJECT* ppObject
    );

#endif /* __BATCH_P_H__ */
