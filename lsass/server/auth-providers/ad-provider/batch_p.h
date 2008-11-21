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

#define XXX
#include "../../../utils/lsalist.h"

typedef UINT8 LSA_AD_BATCH_QUERY_TYPE, *PLSA_AD_BATCH_QUERY_TYPE;

#define LSA_AD_BATCH_QUERY_TYPE_UNDEFINED      0
#define LSA_AD_BATCH_QUERY_TYPE_BY_DN          1
#define LSA_AD_BATCH_QUERY_TYPE_BY_SID         2
#define LSA_AD_BATCH_QUERY_TYPE_BY_NT4         3
#define LSA_AD_BATCH_QUERY_TYPE_BY_USER_ALIAS  4
#define LSA_AD_BATCH_QUERY_TYPE_BY_GROUP_ALIAS 5
#define LSA_AD_BATCH_QUERY_TYPE_BY_UID         6
#define LSA_AD_BATCH_QUERY_TYPE_BY_GID         7

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

typedef UINT8 LSA_AD_BATCH_DOMAIN_ENTRY_FLAGS;

// If this is set, we are not supposed to process
// this domain.
#define LSA_AD_BATCH_DOMAIN_ENTRY_FLAG_SKIP             0x01

// If this is set, we are dealing with one-way trust scenario.
#define LSA_AD_BATCH_DOMAIN_ENTRY_FLAG_IS_ONE_WAY_TRUST 0x02


typedef struct _LSA_AD_BATCH_DOMAIN_ENTRY {
    PSTR pszDnsDomainName;
    PSTR pszNetbiosDomainName;
    LSA_AD_BATCH_DOMAIN_ENTRY_FLAGS Flags;
    LSA_AD_BATCH_QUERY_TYPE QueryType;

    // The presence of these depend on the query type.
    union {
        struct {
            // pszDcPart is not allocated, but points to
            // strings that last longer than this structure.
            PCSTR pszDcPart;
        } ByDn;
        struct {
            // Allocated
            PSTR pszDomainSid;
            size_t sDomainSidLength;
        } BySid;
    } QueryMatch;

    // Number of items in BatchItemList.
    DWORD dwBatchItemCount;
    // List of LSA_AD_BATCH_ITEM.
    LSA_LIST_LINKS BatchItemList;

    // Links for list of domain entry.
    LSA_LIST_LINKS DomainEntryListLinks;
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

typedef UINT8 LSA_AD_BATCH_ITEM_FLAGS, *PLSA_AD_BATCH_ITEM_FLAGS;

#define LSA_AD_BATCH_ITEM_FLAG_HAVE_PSEUDO  0x01
#define LSA_AD_BATCH_ITEM_FLAG_HAVE_REAL    0x02
#define LSA_AD_BATCH_ITEM_FLAG_DISABLED     0x04
#define LSA_AD_BATCH_ITEM_FLAG_ERROR        0x08

typedef struct _LSA_AD_BATCH_QUERY_TERM {
    LSA_AD_BATCH_QUERY_TYPE Type;
    union {
        // This can be a DN, SID, NT4 name, or alias.
        // It is not allocated.  Rather, it points to data
        // that lasts longer than this structure.
        PCSTR pszString;
        // This can be a uid or gid.
        DWORD dwId;
    };
} LSA_AD_BATCH_QUERY_TERM, *PLSA_AD_BATCH_QUERY_TERM;

typedef UINT8 LSA_AD_BATCH_OBJECT_TYPE, *PLSA_AD_BATCH_OBJECT_TYPE;

#define LSA_AD_BATCH_OBJECT_TYPE_UNDEFINED 0
#define LSA_AD_BATCH_OBJECT_TYPE_USER      1
#define LSA_AD_BATCH_OBJECT_TYPE_GROUP     2

typedef struct _LSA_AD_BATCH_ITEM_USER_INFO {
    // Unix fields in struct passwd order:
    PSTR pszAlias;
    PSTR pszPasswd;
    uid_t uid;
    gid_t gid;
    PSTR pszGecos;
    PSTR pszHomeDirectory;
    PSTR pszShell;
    // AD-specific fields:
    PSTR pszUserPrincipalName;
    DWORD dwPrimaryGroupRid;
    UINT32 UserAccountControl;
    UINT64 AccountExpires;
    UINT64 PasswordLastSet;
} LSA_AD_BATCH_ITEM_USER_INFO, *PLSA_AD_BATCH_ITEM_USER_INFO;

typedef struct _LSA_AD_BATCH_ITEM_GROUP_INFO {
    // Unix fields in struct group order:
    PSTR pszAlias;
    PSTR pszPasswd;
    gid_t gid;
} LSA_AD_BATCH_ITEM_GROUP_INFO, *PLSA_AD_BATCH_ITEM_GROUP_INFO;

XXX; // remove pDomainEntry as we should not need it.
XXX; // eventually remove DN field...
typedef struct _LSA_AD_BATCH_ITEM {
    LSA_AD_BATCH_QUERY_TERM QueryTerm;
    PCSTR pszQueryMatchTerm;
    PLSA_AD_BATCH_DOMAIN_ENTRY pDomainEntry;
    LSA_LIST_LINKS BatchItemListLinks;
    LSA_AD_BATCH_ITEM_FLAGS Flags;

    // Non-specific fields:
    PSTR pszSid;
    PSTR pszSamAccountName;
    PSTR pszDn;
    LSA_AD_BATCH_OBJECT_TYPE ObjectType;
    // User/Group-specific fields:
    union {
        LSA_AD_BATCH_ITEM_USER_INFO UserInfo;
        LSA_AD_BATCH_ITEM_GROUP_INFO GroupInfo;
    };
} LSA_AD_BATCH_ITEM, *PLSA_AD_BATCH_ITEM;

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
AD_ResolveRealObjectsByListRpc(
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN PCSTR pszDnsDomainName,
    IN DWORD dwTotalItemCount,
    // List of PLSA_AD_BATCH_ITEM
    IN OUT PLSA_LIST_LINKS pBatchItemList
    );

static
DWORD
AD_ResolveRealObjectsByList(
    IN HANDLE hProvider,
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    IN PCSTR pszDnsDomainName,
    IN DWORD dwTotalItemCount,
    // List of PLSA_AD_BATCH_ITEM
    IN OUT PLSA_LIST_LINKS pBatchItemList
    );

static
DWORD
AD_ResolvePseudoObjectsByRealObjects(
    IN HANDLE hProvider,
    IN PCSTR pszDnsDomainName,
    IN DWORD dwTotalItemCount,    
    // List of PLSA_AD_BATCH_ITEM
    IN OUT PLSA_LIST_LINKS pBatchItemList
    );

static
DWORD
AD_ResolvePseudoObjectsByRealObjectsWithLinkedCell(
    IN HANDLE hProvider,
    IN DWORD dwTotalItemCount,
    // List of PLSA_AD_BATCH_ITEM
    IN OUT PLSA_LIST_LINKS pBatchItemList
    );

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
    );

// zero means unlimited
#define BUILD_BATCH_QUERY_MAX_SIZE 0
#define BUILD_BATCH_QUERY_MAX_COUNT 1000

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
    );

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
    );

static
DWORD
AD_RecordRealObjectFromRpc(
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    // List of PLSA_AD_BATCH_ITEM
    IN OUT PLSA_LIST_LINKS pStartBatchItemListLinks,
    IN PLSA_LIST_LINKS pEndBatchItemListLinks,
    IN PSTR pszObjectSid,
    IN PLSA_TRANSLATED_NAME_OR_SID pTranslatedName
    );

static
DWORD
AD_RecordRealObject(
    IN LSA_AD_BATCH_QUERY_TYPE QueryType,
    // List of PLSA_AD_BATCH_ITEM
    IN OUT PLSA_LIST_LINKS pStartBatchItemListLinks,
    IN PLSA_LIST_LINKS pEndBatchItemListLinks,
    IN HANDLE hDirectory,
    IN LDAPMessage* pMessage
    );

static
DWORD
AD_RecordPseudoObjectBySid(
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
LsaAdBatchMarshalList(
    IN OUT PLSA_LIST_LINKS pBatchItemList,
    IN DWORD dwAvailableCount,
    OUT PAD_SECURITY_OBJECT* ppObjects,
    OUT PDWORD pdwUsedCount
    );

static
DWORD
LsaAdBatchAccountTypeToObjectType(
    IN ADAccountType AccountType,
    OUT PLSA_AD_BATCH_OBJECT_TYPE pObjectType
    );

static
DWORD
LsaAdBatchGatherObjectType(
    IN OUT PLSA_AD_BATCH_ITEM pItem,
    IN LSA_AD_BATCH_OBJECT_TYPE ObjectType
    );

#endif /* __BATCH_P_H__ */
