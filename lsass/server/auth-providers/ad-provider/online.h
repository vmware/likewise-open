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
 *        provider-main.h
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
#ifndef __ONLINE_H__
#define __ONLINE_H__

#define PRIMARY_GROUP_EXPIRATION    (void *)0
#define STANDARD_GROUP_EXPIRATION   (void *)1
#define PAC_GROUP_EXPIRATION        (void *)2

typedef struct _AD_ONLINE_DOMAIN_ENUM_STATE
{
    PDLINKEDLIST pDomainList;
    DWORD dwError;
} AD_ONLINE_DOMAIN_ENUM_STATE, *PAD_ONLINE_DOMAIN_ENUM_STATE;

DWORD
AD_TestNetworkConnection(
    PCSTR pszDomain
    );

DWORD
AD_AddAllowedGroup(
    PCSTR    pszGroupname
    );

DWORD
AD_OnlineInitializeOperatingMode(
    PCSTR pszDomain,
    PCSTR pszHostName
    );

DWORD
AD_DetermineTrustModeandDomainName(
    IN PCSTR pszDomain,
    OUT TrustMode* pTrustMode,
    OUT OPTIONAL PSTR* ppszDnsDomainName,
    OUT OPTIONAL PSTR* ppszNetbiosDomainName
    );

DWORD
AD_OnlineAuthenticateUser(
    HANDLE hProvider,
    PCSTR  pszUserName,
    PCSTR  pszPassword
    );

DWORD
AD_CheckUserIsAllowedLogin(
    HANDLE hProvider,
    uid_t  uid
    );

DWORD
AD_CrackDomainQualifiedName(
    PCSTR pszId,
    PLSA_LOGIN_NAME_INFO* ppNameInfo
    );

DWORD
AD_OnlineFindUserByName(
    HANDLE  hProvider,
    PCSTR   pszLoginId,
    DWORD   dwUserInfoLevel,
    PVOID*  ppUserInfo
    );

DWORD
AD_OnlineFindUserById(
    HANDLE  hProvider,
    uid_t   uid,
    DWORD   dwUserInfoLevel,
    PVOID*  ppUserInfo
    );

DWORD
AD_OnlineGetUserGroupMembership(
    HANDLE  hProvider,
    uid_t   uid,
    DWORD   dwGroupInfoLevel,
    PDWORD  pdwNumGroupsFound,
    PVOID** pppGroupInfoList
    );

DWORD
AD_OnlineEnumUsers(
    HANDLE  hProvider,
    HANDLE  hResume,
    DWORD   dwMaxNumUsers,
    PDWORD  pdwUsersFound,
    PVOID** pppUserInfoList
    );

DWORD
AD_OnlineFindGroupByName(
    HANDLE  hProvider,
    PCSTR   pszLoginId,
    DWORD   dwGroupInfoLevel,
    PVOID*  ppGroupInfo
    );

DWORD
AD_OnlineFindGroupById(
    HANDLE  hProvider,
    gid_t   gid,
    DWORD   dwGroupInfoLevel,
    PVOID*  ppGroupInfo
    );

DWORD
AD_OnlineEnumGroups(
    HANDLE  hProvider,
    HANDLE  hResume,
    DWORD   dwMaxGroups,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
    );

DWORD
AD_OnlineChangePassword(
    HANDLE hProvider,
    PCSTR pszUserName,
    PCSTR pszPassword,
    PCSTR pszOldPassword
    );

DWORD
AD_CreateHomeDirectory(
    PLSA_USER_INFO_1 pUserInfo
    );

#if defined (__LWI_DARWIN__)
DWORD
AD_CreateHomeDirectory_Mac(
    PLSA_USER_INFO_1 pUserInfo
    );
#endif

DWORD
AD_CreateHomeDirectory_Generic(
    PLSA_USER_INFO_1 pUserInfo
    );

DWORD
AD_ProvisionHomeDir(
    uid_t ownerUid,
    gid_t ownerGid,
    PCSTR pszHomedirPath
    );

DWORD
AD_CreateK5Login(
    PLSA_USER_INFO_1 pUserInfo
    );

DWORD
AD_CheckExpiredObject(
    PAD_SECURITY_OBJECT *ppCachedUser
    );

int
AD_CompareObjectSids(
    IN PCVOID pObjectA,
    IN PCVOID pObjectB
    );

size_t
AD_HashObjectSid(
    IN PCVOID pObject
    );

void
AD_FreeHashObject(
    IN OUT const LSA_HASH_ENTRY *pEntry
    );

DWORD
AD_GetExpandedGroupUsers(
    HANDLE  hProvider,
    PCSTR pszDomainName,
    PCSTR pszDomainNetBiosName,
    PCSTR pszGroupSid,
    int iMaxDepth,
    BOOLEAN *pbAllExpanded,
    size_t* psCount,
    PAD_SECURITY_OBJECT** pppResults);

DWORD
AD_GetGroupMembers(
    HANDLE hProvider,
    PCSTR pszDomainName,
    PCSTR pszDomainNetBiosName,
    PCSTR pszSid,
    size_t* psCount,
    PAD_SECURITY_OBJECT** pppResults);

DWORD
AD_FindObjectsByDNList(
    HANDLE hProvider,
    size_t sCount,
    PSTR* ppszDnList,
    PAD_SECURITY_OBJECT **pppResults);

DWORD
AD_FindObjectBySid(
    HANDLE hProvider,
    PCSTR pszDomainName,
    PCSTR pszDomainNetBiosName,
    PCSTR pszSid,
    PAD_SECURITY_OBJECT *ppResult);

DWORD
AD_FindObjectsBySidList(
    HANDLE hProvider,
    PCSTR  pszDomainName,
    PCSTR  pszDomainNetBiosName,
    size_t sCount,
    PSTR* ppszSidList,
    PAD_SECURITY_OBJECT **pppResults);

DWORD
AD_OnlineGetNamesBySidList(
    HANDLE          hProvider,
    size_t          sCount,
    PSTR*           ppszSidList,
    PSTR**          pppszDomainNames,
    PSTR**          pppszSamAccounts,
    ADAccountType** ppTypes);

DWORD
AD_GetLinkedCellInfo(
    HANDLE hDirectory,
    PCSTR  pszCellDN);

DWORD
LsaValidateSeparatorCharacter(
    CHAR cSeparator,
    CHAR* pcValidatedSeparator
    );

VOID
AD_FreeLinkedCellInfoInList(
    PVOID pLinkedCellInfo,
    PVOID pUserData
    );

VOID
AD_FreeLinkedCellInfo(
    PAD_LINKED_CELL_INFO pLinkedCellInfo
    );

void
AD_FreeHashStringKey(
    const LSA_HASH_ENTRY *pEntry);

DWORD
AD_CacheGroupMembershipFromPac(
    IN HANDLE              hProvider,
    IN TrustMode           trustMode,
    IN PAD_SECURITY_OBJECT pUserInfo,
    IN PAC_LOGON_INFO *    pPac);

void
AD_FilterNullEntries(
    PAD_SECURITY_OBJECT* ppEntries,
    size_t  *psCount);

DWORD
AD_OnlineFindUserObjectByName(
    HANDLE  hProvider,
    PCSTR   pszLoginId,
    PAD_SECURITY_OBJECT* ppCachedUser);

DWORD
AD_FindCachedGroupByNT4Name(
    HANDLE  hProvider,
    PCSTR   pszGroupName,
    PAD_SECURITY_OBJECT* ppCachedGroup);

DWORD
AD_OnlineEnumNSSArtefacts(
    HANDLE  hProvider,
    HANDLE  hResume,
    DWORD   dwMaxNSSArtefacts,
    PDWORD  pdwNSSArtefactsFound,
    PVOID** pppNSSArtefactInfoList
    );

DWORD
AD_GetCachedPasswordHash(
    IN PCSTR pszSamAccount,
    IN PCSTR pszPassword,
    OUT PBYTE *ppbHash
    );

DWORD
AD_CanonicalizeDomainsInCrackedNameInfo(
    IN OUT PLSA_LOGIN_NAME_INFO pNameInfo
    );

DWORD
AD_UpdateUserObjectFlags(
    IN OUT PAD_SECURITY_OBJECT pUser
    );

BOOLEAN
AD_OnlineCopyDomainListCallback(
    IN OPTIONAL PCSTR pszEnumDomainName,
    IN OPTIONAL PVOID pContext,
    IN PLSA_DM_ENUM_DOMAIN_CALLBACK_INFO pDomainInfo
    );

#endif /* __ONLINE_H__ */
