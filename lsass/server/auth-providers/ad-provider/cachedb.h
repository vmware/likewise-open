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
 *        cachedb.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 * 
 *        Caching for AD Provider Database Interface
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 * 
 */
#ifndef __CACHEDB_H__
#define __CACHEDB_H__

struct _CACHE_CONNECTION;
typedef struct _CACHE_CONNECTION *CACHE_CONNECTION_HANDLE;

typedef struct __AD_CACHE_INFO
{
    // This value is set to -1 if the value is not stored in the cache
    // database (it only exists in memory). Otherwise, this is an index into
    // the cache table.
    int64_t qwCacheId;
    // This value is set to (time_t)-1 if the value does not expire following
    // the normal algorithm. So far this only occurs if the data is obtained
    // from the PAC and can only be obtained from the PAC.
    //
    // Set it to (time_t)-2 if geting the members of a group cannot expire
    // this entry.
    time_t tLastUpdated;
} AD_CACHE_INFO;

typedef struct __AD_SECURITY_OBJECT
{
    AD_CACHE_INFO cache;
    PSTR    pszDN;
    // The object SID is stored in printed form
    PSTR    pszObjectSid;
    //This is false if the object has not been enabled in the cell
    BOOLEAN enabled;

    PSTR    pszNetbiosDomainName;
    PSTR    pszSamAccountName;

    ADAccountType type;

    // These fields are only set if the object is enabled
    union
    {
        struct
        {
            uid_t   uid;
            gid_t   gid;
            PSTR    pszUPN;
            PSTR    pszAliasName;
            PSTR    pszPasswd;
            PSTR    pszGecos;
            PSTR    pszShell;
            PSTR    pszHomedir;
            uint64_t qwPwdLastSet;
            uint64_t qwAccountExpires;

            //Calculated from userAccountControl, accountExpires, and pwdLastSet fields
            BOOLEAN bIsGeneratedUPN;
            BOOLEAN bPasswordExpired;
            BOOLEAN bPasswordNeverExpires;
            BOOLEAN bPromptPasswordChange;
            BOOLEAN bUserCanChangePassword;
            BOOLEAN bAccountDisabled;
            BOOLEAN bAccountExpired;
            BOOLEAN bAccountLocked;
        } userInfo;
        struct {
            gid_t   gid;
            PSTR    pszAliasName;
            PSTR    pszPasswd;
        } groupInfo;
    };
} AD_SECURITY_OBJECT, *PAD_SECURITY_OBJECT;

typedef const AD_SECURITY_OBJECT * PCAD_SECURITY_OBJECT;

typedef struct __AD_GROUP_MEMBERSHIP
{
    AD_CACHE_INFO cache;
    PSTR    pszParentSid;
    PSTR    pszChildSid;
} AD_GROUP_MEMBERSHIP, *PAD_GROUP_MEMBERSHIP;

typedef struct __AD_PASSWORD_VERIFIER
{
    AD_CACHE_INFO cache;
    PSTR    pszObjectSid;
    // sequence of hexadecimal digits
    PSTR    pszPasswordVerifier;
} AD_PASSWORD_VERIFIER, *PAD_PASSWORD_VERIFIER;

DWORD
ADCacheDB_Initialize(
    VOID
    );

DWORD
ADCacheDB_Shutdown(
    VOID);

DWORD
ADCacheDB_OpenDb(
    PHANDLE phDb
    );

// Sets the handle to null after closing it. If a null handle is passed in,
// it is ignored.
void
ADCacheDB_SafeCloseDb(
    PHANDLE phDb
    );

// returns LSA_ERROR_NOT_HANDLED if the user is not in the cache
DWORD
ADCacheDB_FindUserByName(
    HANDLE hDb,
    PLSA_LOGIN_NAME_INFO pUserNameInfo,
    PAD_SECURITY_OBJECT* ppObject
    );

// returns LSA_ERROR_NOT_HANDLED if the user is not in the cache
DWORD
ADCacheDB_FindUserById(
    HANDLE hDb,
    uid_t uid,
    PAD_SECURITY_OBJECT* ppObject
    );

DWORD
ADCacheDB_FindGroupByName(
    HANDLE hDb,
    PLSA_LOGIN_NAME_INFO pGroupNameInfo,
    PAD_SECURITY_OBJECT* ppObject
    );

DWORD
ADCacheDB_FindGroupById(
    HANDLE hDb,
    uid_t uid,
    PAD_SECURITY_OBJECT* ppObject
    );

DWORD
ADCacheDB_CacheObjectEntry(
    HANDLE hDb,
    PAD_SECURITY_OBJECT pObject
    );

DWORD
ADCacheDB_CacheObjectEntries(
    HANDLE hDb,
    size_t  sObjectCount,
    PAD_SECURITY_OBJECT* ppObjects
    );

void
ADCacheDB_SafeFreeObject(
    PAD_SECURITY_OBJECT* ppObject
    );

void
ADCacheDB_SafeFreeObjectList(
        size_t sCount,
        PAD_SECURITY_OBJECT** pppObjectList);

#define ADCACHEDB_SAFE_FREE_PASSWORD_VERIFIER(x) \
    if ((x) != NULL) { \
        ADCacheDB_FreePasswordVerifier(x); \
        (x) = NULL; \
    }

void
ADCacheDB_FreePasswordVerifier(
    IN OUT PAD_PASSWORD_VERIFIER pVerifier
    );

DWORD
ADCacheDB_CacheGroupMembership(
    HANDLE hDb,
    PCSTR pszParentSid,
    size_t sMemberCount,
    PAD_GROUP_MEMBERSHIP *ppMembers);

DWORD
ADCacheDB_CacheGroupsForUser(
    HANDLE hDb,
    PCSTR pszChildSid,
    size_t sMemberCount,
    PAD_GROUP_MEMBERSHIP* ppMembers,
    BOOLEAN bOverwritePacEntries);

DWORD
ADCacheDB_GetGroupMembers(
    HANDLE hDb,
    PCSTR pszSid,
    size_t* psCount,
    PAD_GROUP_MEMBERSHIP **pppResults);

DWORD
ADCacheDB_GetGroupsForUser(
    HANDLE hDb,
    PCSTR pszSid,
    size_t* psCount,
    PAD_GROUP_MEMBERSHIP **pppResults);

void
ADCacheDB_SafeFreeGroupMembership(
        PAD_GROUP_MEMBERSHIP* ppMembership);

void
ADCacheDB_SafeFreeGroupMembershipList(
        size_t sCount,
        PAD_GROUP_MEMBERSHIP** pppMembershipList);

DWORD
ADCacheDB_FindObjectsByDNList(
    HANDLE hDb,
    size_t sCount,
    PSTR* ppszDnList,
    PAD_SECURITY_OBJECT **pppResults);

DWORD
ADCacheDB_FindObjectsBySidList(
    HANDLE hDb,
    size_t sCount,
    PSTR* ppszSidList,
    PAD_SECURITY_OBJECT **pppResults);

DWORD
ADCacheDB_FindObjectByDN(
    HANDLE hDb,
    PCSTR pszDN,
    PAD_SECURITY_OBJECT *ppObject);

DWORD
ADCacheDB_FindObjectBySid(
    HANDLE hDb,
    PCSTR pszSid,
    PAD_SECURITY_OBJECT *ppObject);

// returns LSA_ERROR_NOT_HANDLED if the user is not in the cache
DWORD
ADCacheDB_GetPasswordVerifier(
    IN HANDLE hDb,
    IN PCSTR pszUserSid,
    OUT PAD_PASSWORD_VERIFIER *ppResult
    );

DWORD
ADCacheDB_CachePasswordVerifier(
    IN HANDLE hDb,
    IN PAD_PASSWORD_VERIFIER pVerifier
    );

DWORD
ADCacheDB_GetProviderData(
    IN HANDLE hDb,
    OUT PAD_PROVIDER_DATA* ppProvider
    );

DWORD
ADCacheDB_CacheProviderData(
    IN HANDLE hDb,
    IN PAD_PROVIDER_DATA pProvider
    );

DWORD
ADCacheDB_GetDomainTrustList(
    IN HANDLE hDb,
    // Contains type PLSA_DM_ENUM_DOMAIN_INFO
    OUT PDLINKEDLIST* ppList
    );

DWORD
ADCacheDB_CacheDomainTrustList(
    IN HANDLE hDb,
    IN PLSA_DM_ENUM_DOMAIN_INFO* ppDomainInfo,
    IN DWORD dwDomainInfoCount
    );

VOID
ADCacheDB_FreeEnumDomainInfoList(
    // Contains type PLSA_DM_ENUM_DOMAIN_INFO
    IN OUT PDLINKEDLIST pList
    );

#endif /* __CACHEDB_H__ */
