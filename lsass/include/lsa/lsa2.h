/*
 * Copyright Likewise Software
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */


/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        lsa2.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Public Client API (version 2)
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 */

#ifndef __LSA2_H__
#define __LSA2_H__

#include <inttypes.h>
#include <lsa/lsa.h>

typedef struct __LSA_SECURITY_OBJECT_VERSION_INFO
{
    // This value is set to -1 if the value is not stored in the
    // database (it only exists in memory). Otherwise, this is an index into
    // the database.
    int64_t qwDbId;
    time_t tLastUpdated;
    // Sum of the size of all objects that use this version info (only used by
    // memory backend)
    DWORD dwObjectSize;
    // Importance of this object (for internal use by the memory backend)
    float fWeight;
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
    BOOLEAN bIsAccountInfoKnown;
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
        union
        {
            LSA_SECURITY_OBJECT_USER_INFO userInfo;
            LSA_SECURITY_OBJECT_GROUP_INFO groupInfo;
        } typeInfo;
    };
} LSA_SECURITY_OBJECT, *PLSA_SECURITY_OBJECT;

typedef const LSA_SECURITY_OBJECT * PCLSA_SECURITY_OBJECT;

typedef UINT8 LSA_QUERY_TYPE, *PLSA_QUERY_TYPE;
#define LSA_QUERY_TYPE_UNDEFINED      0
#define LSA_QUERY_TYPE_BY_DN          1
#define LSA_QUERY_TYPE_BY_SID         2
#define LSA_QUERY_TYPE_BY_NT4         3
#define LSA_QUERY_TYPE_BY_UPN         4
#define LSA_QUERY_TYPE_BY_ALIAS       5
#define LSA_QUERY_TYPE_BY_UNIX_ID     6
#define LSA_QUERY_TYPE_BY_NAME        7

//
// 2009/11/13-dalmeida -- Can use ADAccountType for now instead?
// Easy to convert, though...
//
typedef UINT8 LSA_OBJECT_TYPE, *PLSA_OBJECT_TYPE;
#define LSA_OBJECT_TYPE_UNDEFINED 0
#define LSA_OBJECT_TYPE_USER      1
#define LSA_OBJECT_TYPE_GROUP     2
#define LSA_OBJECT_TYPE_COMPUTER  3
#if 0
#define LSA_OBJECT_TYPE_DOMAIN    4
#endif

typedef union _LSA_QUERY_ITEM {
    PCSTR pszString;
    DWORD dwId;
} LSA_QUERY_ITEM, *PLSA_QUERY_ITEM;

typedef union _LSA_QUERY_LIST {
    PCSTR* ppszStrings;
    PDWORD pdwIds;
} LSA_QUERY_LIST, *PLSA_QUERY_LIST;

DWORD
LsaFindObjects(
    IN HANDLE hLsa,
    IN PCSTR pszTargetProvider,
    IN LSA_FIND_FLAGS FindFlags,
    IN OPTIONAL LSA_OBJECT_TYPE ObjectType,
    IN LSA_QUERY_TYPE QueryType,
    IN DWORD dwCount,
    IN LSA_QUERY_LIST QueryList,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    );

DWORD
LsaOpenEnumObjects(
    IN HANDLE hLsa,
    IN PCSTR pszTargetProvider,
    OUT PHANDLE phEnum,
    IN LSA_FIND_FLAGS FindFlags,
    IN LSA_OBJECT_TYPE ObjectType,
    IN OPTIONAL PCSTR pszDomainName
    );

DWORD
LsaEnumObjects(
    IN HANDLE hLsa,
    IN HANDLE hEnum,
    IN DWORD dwMaxObjectsCount,
    OUT PDWORD pdwObjectsCount,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    );

DWORD
LsaOpenEnumMembers(
    IN HANDLE hLsa,
    IN PCSTR pszTargetProvider,
    OUT PHANDLE phEnum,
    IN LSA_FIND_FLAGS FindFlags,
    IN PCSTR pszSid
    );

DWORD
LsaEnumMembers(
    IN HANDLE hLsa,
    IN HANDLE hEnum,
    IN DWORD dwMaxObjectsCount,
    OUT PDWORD pdwObjectsCount,
    OUT PSTR** pppszMember
    );

DWORD
LsaQueryMemberOf(
    IN HANDLE hLsa,
    IN PCSTR pszTargetProvider,
    IN LSA_FIND_FLAGS FindFlags,
    DWORD dwSidCount,
    IN PSTR* ppszSids,
    OUT PDWORD pdwGroupSidCount,
    OUT PSTR** pppszGroupSids
    );

DWORD
LsaCloseEnum(
    IN HANDLE hLsa,
    IN OUT HANDLE hEnum
    );

VOID
LsaFreeSidList(
    IN DWORD dwSidCount,
    IN OUT PSTR* ppszSids
    );

VOID
LsaFreeSecurityObjectList(
    IN DWORD dwObjectCount,
    IN OUT PLSA_SECURITY_OBJECT* ppObjects
    );

#endif
