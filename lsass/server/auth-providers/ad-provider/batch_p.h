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
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Wei Fu (wfu@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 */
#ifndef __BATCH_P_H__
#define __BATCH_P_H__

#define LSA_SAFE_FREE_LOGIN_NAME_INFO(pLoginNameInfo) \
        do {                      \
           if (pLoginNameInfo) {             \
              LsaFreeNameInfo(pLoginNameInfo); \
              (pLoginNameInfo) = NULL;       \
           }                      \
        } while(0);

typedef struct _LSA_AD_BATCH_DOMAIN_ENTRY {
    // pszDcPart is not allocated, but points to
    // strings that last longer than this structure.
    PCSTR pszDcPart;
    PSTR pszDnsDomainName;
    DWORD dwCount;
    // List of PCSTR
    PDLINKEDLIST pDnList;
    // Results
    DWORD dwObjectsCount;
    PAD_SECURITY_OBJECT* ppObjects;
} LSA_AD_BATCH_DOMAIN_ENTRY, *PLSA_AD_BATCH_DOMAIN_ENTRY;

static
DWORD
CreateBatchDomainEntry(
    OUT PLSA_AD_BATCH_DOMAIN_ENTRY* ppEntry,
    IN PCSTR pszDcPart
    );

static
VOID
DestroyBatchDomainEntry(
    IN OUT PLSA_AD_BATCH_DOMAIN_ENTRY* ppEntry
    );

typedef struct _LSA_AD_BATCH_ITEM {
    // pszDn is not allocated, but points to
    // strings that last longer than this structure.
    PCSTR pszDn;
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
CheckDomainModeCompatibilityForDefaultMode(
    PCSTR pszDnsDomainName,
    PCSTR pszDomainDN
    );

static
DWORD
CheckCellConfigurationMode(
    IN PCSTR pszDnsDomainName,
    IN PCSTR pszCellDN,
    OUT ADConfigurationMode* pAdMode
    );

static
DWORD
MakeObjectLoginNameInfo(
    IN PLSA_AD_BATCH_ITEM pBatchItem,
    OUT PLSA_LOGIN_NAME_INFO* ppLoginNameInfo
    );

static
DWORD
AD_FindObjectsByDNListForDomain(
    IN HANDLE hProvider,
    IN PCSTR pszDnsDomainName,
    IN DWORD dwCount,
    // List of PCSTR
    IN PDLINKEDLIST pDnList,
    OUT PDWORD pdwCount,
    OUT PAD_SECURITY_OBJECT **pppObjects
    );

// Fills in real and SIDs.
static
DWORD
AD_ResolveRealObjectsByDNList(
    IN HANDLE hProvider,
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
    IN PCSTR pszDnsDomainName,
    IN DWORD dwTotalItemCount,
    // List of PLSA_AD_BATCH_ITEM
    IN OUT PDLINKEDLIST pBatchItemList,
    OUT PLSA_AD_BATCH_MESSAGES* ppMessages
    );

static
DWORD
AD_ResolvePseudoObjectsByRealObjectsInternal(
    IN HANDLE hProvider,
    IN PCSTR pszDnsDomainName,
    IN DWORD dwTotalItemCount,
    IN BOOLEAN bIsSchemaMode,
    IN PCSTR pszCellDn,
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
AD_BuildBatchQueryForRealByDn(
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
AD_RecordRealObjectByDn(
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

#endif /* __BATCH_P_H__ */
