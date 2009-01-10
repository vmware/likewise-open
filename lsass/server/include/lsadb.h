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
 *        lsadb.h
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
#ifndef __LSADB_H__
#define __LSADB_H__

struct _LSA_DB_CONNECTION;
typedef struct _LSA_DB_CONNECTION *LSA_DB_HANDLE;
typedef LSA_DB_HANDLE *PLSA_DB_HANDLE;

typedef struct __LSA_SECURITY_OBJECT_VERSION_INFO
{
    // This value is set to -1 if the value is not stored in the
    // database (it only exists in memory). Otherwise, this is an index into
    // the database.
    int64_t qwDbId;
    time_t tLastUpdated;
} LSA_SECURITY_OBJECT_VERSION_INFO, *PLSA_SECURITY_OBJECT_VERSION_INFO;

typedef struct _LSA_SECURITY_OBJECT_USER_INFO
{
    uid_t uid;
    gid_t gid;
    PSTR pszUPN;
    PSTR pszAliasName;
    PSTR pszPasswd;
    PSTR pszGecos;
    PSTR pszShell;
    PSTR pszHomedir;
    uint64_t qwPwdLastSet;
    uint64_t qwAccountExpires;

    BOOLEAN bIsGeneratedUPN;
    // Calculated from userAccountControl, accountExpires, and pwdLastSet
    // attributes from AD.
    BOOLEAN bPasswordExpired;
    BOOLEAN bPasswordNeverExpires;
    BOOLEAN bPromptPasswordChange;
    BOOLEAN bUserCanChangePassword;
    BOOLEAN bAccountDisabled;
    BOOLEAN bAccountExpired;
    BOOLEAN bAccountLocked;
} LSA_SECURITY_OBJECT_USER_INFO, *PLSA_SECURITY_OBJECT_USER_INFO;

typedef struct _LSA_SECURITY_OBJECT_GROUP_INFO
{
    gid_t gid;
    PSTR pszAliasName;
    PSTR pszPasswd;
} LSA_SECURITY_OBJECT_GROUP_INFO, *PLSA_SECURITY_OBJECT_GROUP_INFO;

typedef struct __LSA_DB_SECURITY_OBJECT
{
    LSA_SECURITY_OBJECT_VERSION_INFO version;
    PSTR    pszDN;
    // The object SID is stored in printed form
    PSTR    pszObjectSid;
    //This is false if the object has not been enabled in the cell
    BOOLEAN enabled;

    PSTR    pszNetbiosDomainName;
    PSTR    pszSamAccountName;

    ADAccountType type;

    // These fields are only set if the object is enabled base on the type.
    union
    {
        LSA_SECURITY_OBJECT_USER_INFO userInfo;
        LSA_SECURITY_OBJECT_GROUP_INFO groupInfo;
    };
} LSA_SECURITY_OBJECT, *PLSA_SECURITY_OBJECT;

typedef const LSA_SECURITY_OBJECT * PCLSA_SECURITY_OBJECT;

typedef struct __LSA_GROUP_MEMBERSHIP
{
    LSA_SECURITY_OBJECT_VERSION_INFO version;
    PSTR    pszParentSid;
    PSTR    pszChildSid;
    BOOLEAN bIsInPac;
    BOOLEAN bIsInPacOnly;
    BOOLEAN bIsInLdap;
    BOOLEAN bIsDomainPrimaryGroup;
} LSA_GROUP_MEMBERSHIP, *PLSA_GROUP_MEMBERSHIP;

typedef struct __LSA_DB_PASSWORD_VERIFIER
{
    LSA_SECURITY_OBJECT_VERSION_INFO version;
    PSTR    pszObjectSid;
    // sequence of hexadecimal digits
    PSTR    pszPasswordVerifier;
} LSA_PASSWORD_VERIFIER, *PLSA_PASSWORD_VERIFIER;

DWORD
LsaDbOpen(
    IN PCSTR pszDbPath,
    PLSA_DB_HANDLE phDb
    );

// Sets the handle to null after closing it. If a null handle is passed in,
// it is ignored.
void
LsaDbSafeClose(
    PLSA_DB_HANDLE phDb
    );

// returns LSA_ERROR_NOT_HANDLED if the user is not in the database
DWORD
LsaDbFindUserByName(
    LSA_DB_HANDLE hDb,
    PLSA_LOGIN_NAME_INFO pUserNameInfo,
    PLSA_SECURITY_OBJECT* ppObject
    );

// returns LSA_ERROR_NOT_HANDLED if the user is not in the database
DWORD
LsaDbFindUserById(
    LSA_DB_HANDLE hDb,
    uid_t uid,
    PLSA_SECURITY_OBJECT* ppObject
    );

DWORD
LsaDbFindGroupByName(
    LSA_DB_HANDLE hDb,
    PLSA_LOGIN_NAME_INFO pGroupNameInfo,
    PLSA_SECURITY_OBJECT* ppObject
    );

DWORD
LsaDbFindGroupById(
    LSA_DB_HANDLE hDb,
    uid_t uid,
    PLSA_SECURITY_OBJECT* ppObject
    );

DWORD
LsaDbRemoveUserBySid(
    IN LSA_DB_HANDLE hDb,
    IN PCSTR pszSid
    );

DWORD
LsaDbRemoveGroupBySid(
    IN LSA_DB_HANDLE hDb,
    IN PCSTR pszSid
    );

DWORD
LsaDbEmptyCache(
    IN LSA_DB_HANDLE hDb
    );

DWORD
LsaDbStoreObjectEntry(
    LSA_DB_HANDLE hDb,
    PLSA_SECURITY_OBJECT pObject
    );

DWORD
LsaDbStoreObjectEntries(
    LSA_DB_HANDLE hDb,
    size_t  sObjectCount,
    PLSA_SECURITY_OBJECT* ppObjects
    );

void
LsaDbSafeFreeObject(
    PLSA_SECURITY_OBJECT* ppObject
    );

void
LsaDbSafeFreeObjectList(
        size_t sCount,
        PLSA_SECURITY_OBJECT** pppObjectList);

#define LSA_DB_SAFE_FREE_PASSWORD_VERIFIER(x) \
    if ((x) != NULL) { \
        LsaDbFreePasswordVerifier(x); \
        (x) = NULL; \
    }

void
LsaDbFreePasswordVerifier(
    IN OUT PLSA_PASSWORD_VERIFIER pVerifier
    );

DWORD
LsaDbStoreGroupMembership(
    IN LSA_DB_HANDLE hDb,
    IN PCSTR pszParentSid,
    IN size_t sMemberCount,
    IN PLSA_GROUP_MEMBERSHIP* ppMembers
    );

DWORD
LsaDbStoreGroupsForUser(
    IN LSA_DB_HANDLE hDb,
    IN PCSTR pszChildSid,
    IN size_t sMemberCount,
    IN PLSA_GROUP_MEMBERSHIP* ppMembers,
    IN BOOLEAN bIsPacAuthoritative
    );

DWORD
LsaDbGetGroupMembers(
    IN LSA_DB_HANDLE hDb,
    IN PCSTR pszSid,
    IN BOOLEAN bFilterNotInPacNorLdap,
    OUT size_t* psCount,
    OUT PLSA_GROUP_MEMBERSHIP** pppResults
    );

DWORD
LsaDbGetGroupsForUser(
    IN LSA_DB_HANDLE hDb,
    IN PCSTR pszSid,
    IN BOOLEAN bFilterNotInPacNorLdap,
    OUT size_t* psCount,
    OUT PLSA_GROUP_MEMBERSHIP** pppResults
    );

void
LsaDbSafeFreeGroupMembership(
        PLSA_GROUP_MEMBERSHIP* ppMembership);

void
LsaDbSafeFreeGroupMembershipList(
        size_t sCount,
        PLSA_GROUP_MEMBERSHIP** pppMembershipList);

DWORD
LsaDbFindObjectsByDNList(
    IN LSA_DB_HANDLE hDb,
    IN size_t sCount,
    IN PSTR* ppszDnList,
    OUT PLSA_SECURITY_OBJECT** pppResults
    );

DWORD
LsaDbFindObjectsBySidList(
    IN LSA_DB_HANDLE hDb,
    IN size_t sCount,
    IN PSTR* ppszSidList,
    OUT PLSA_SECURITY_OBJECT** pppResults
    );

DWORD
LsaDbFindObjectByDN(
    LSA_DB_HANDLE hDb,
    PCSTR pszDN,
    PLSA_SECURITY_OBJECT *ppObject);

DWORD
LsaDbFindObjectBySid(
    LSA_DB_HANDLE hDb,
    PCSTR pszSid,
    PLSA_SECURITY_OBJECT *ppObject);

DWORD
LsaDbEnumUsersCache(
    IN LSA_DB_HANDLE           hDb,
    IN DWORD                   dwMaxNumUsers,
    IN PCSTR                   pszResume,
    OUT DWORD*                 dwNumUsersFound,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    );

DWORD
LsaDbEnumGroupsCache(
    IN LSA_DB_HANDLE           hDb,
    IN DWORD                   dwMaxNumGroups,
    IN PCSTR                   pszResume,
    OUT DWORD *                dwNumGroupsFound,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    );

// returns LSA_ERROR_NOT_HANDLED if the user is not in the database
DWORD
LsaDbGetPasswordVerifier(
    IN LSA_DB_HANDLE hDb,
    IN PCSTR pszUserSid,
    OUT PLSA_PASSWORD_VERIFIER *ppResult
    );

DWORD
LsaDbStorePasswordVerifier(
    IN LSA_DB_HANDLE hDb,
    IN PLSA_PASSWORD_VERIFIER pVerifier
    );

#endif /* __LSADB_H__ */
