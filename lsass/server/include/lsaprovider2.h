/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright (c) Likewise Software.  All rights reserved.
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
 * license@likewise.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        lsaprovider2.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Authentication Provider Interface Version 2
 *
 * Authors: Krishna Ganugapati (krishnag@likewisee.com)
 *          Sriram Nambakam (snambakam@likewise.com)
 *          Danilo Almeida (dalmeida@likewise.com)
 */
#ifndef __LSAPROVIDER_2_H__
#define __LSAPROVIDER_2_H__

#include <lsa/lsa2.h>

#include "lsaprovider.h"

//
// New Interfaces
//

typedef UINT8 LSA_QUERY_TYPE, *PLSA_QUERY_TYPE;
#define LSA_QUERY_TYPE_UNDEFINED      0
#define LSA_QUERY_TYPE_BY_DN          1
#define LSA_QUERY_TYPE_BY_SID         2
#define LSA_QUERY_TYPE_BY_NT4         3
#define LSA_QUERY_TYPE_BY_USER_ALIAS  4
#define LSA_QUERY_TYPE_BY_GROUP_ALIAS 5
#define LSA_QUERY_TYPE_BY_UID         6
#define LSA_QUERY_TYPE_BY_GID         7

//
// 2009/11/13-dalmeida -- Can use ADAccountType for now instead?
// Easy to convert, though...
//
typedef UINT8 LSA_OBJECT_TYPE, *PLSA_OBJECT_TYPE;
#define LSA_OBJECT_TYPE_UNDEFINED 0
#define LSA_OBJECT_TYPE_USER      1
#define LSA_OBJECT_TYPE_GROUP     2
#if 0
// See SID_NAME_USE -- See that WKG is different type...
#define LSA_OBJECT_TYPE_DOMAIN    4
#define LSA_OBJECT_TYPE_COMPUTER  3
#endif

typedef union _LSA_QUERY_ITEM {
    PCSTR pszString;
    DWORD pdwId;
} LSA_QUERY_ITEM, *PLSA_QUERY_ITEM;

typedef union _LSA_QUERY_LIST {
    PCSTR* ppszStrings;
    PDWORD pdwIds;
} LSA_QUERY_LIST, *PLSA_QUERY_LIST;

//
// Lookup objects.
//
// Objects returned in same order as query with NULL
// entries for objects that are not found.
//
typedef DWORD (*PFN_LSA_PROVIDER_FIND_OBJECTS)(
    IN HANDLE hProvider,
    IN LSA_FIND_FLAGS FindFlags,
    IN OPTIONAL LSA_OBJECT_TYPE ObjectType,
    IN LSA_QUERY_TYPE QueryType,
    IN DWORD dwCount,
    IN LSA_QUERY_LIST QueryList,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    );

//
// Enumerate Users/Groups
//
typedef DWORD (*PFN_LSA_PROVIDER_OPEN_ENUM_OBJECTS)(
    IN HANDLE hProvider,
    OUT PHANDLE phEnum,
    IN LSA_FIND_FLAGS FindFlags,
    IN LSA_OBJECT_TYPE ObjectType,
    IN OPTIONAL PCSTR pszDomainName
    );

typedef DWORD (*PFN_LSA_PROVIDER_ENUM_OBJECTS)(
    IN HANDLE hEnum,
    IN DWORD dwMaxObjectsCount,
    OUT PDWORD pdwObjectsCount,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    );

//
// Enumerate members of a group.
//
// Note that the group object must be resolved first
// (by the SRV/API layer, in any case).  The provider
// can use any field as a key to do the enumeration
// of members.
//
// TODO: Perhaps replace pGroup w/QueryType + QueryItem.
//
typedef DWORD (*PFN_LSA_PROVIDER_OPEN_ENUM_MEMBERS)(
    IN HANDLE hProvider,
    OUT PHANDLE phEnum,
    IN LSA_FIND_FLAGS FindFlags,
    IN PLSA_SECURITY_OBJECT pGroup
    );

typedef DWORD (*PFN_LSA_PROVIDER_ENUM_MEMBERS)(
    IN HANDLE hEnum,
    IN DWORD dwMaxObjectsCount,
    OUT PDWORD pdwObjectsCount,
    OUT PSTR** pppszMember
    );

//
// Enumerate groups that users/groups belong to.
//
// NOTE: It probably makes most sense to remove paging
// from this interface.  Note that AD already has to deal
// with this being non-paged in that the PAC returns
// all this information in one shot.
//
typedef DWORD (*PFN_LSA_PROVIDER_OPEN_ENUM_MEMBER_OF)(
    IN HANDLE hProvider,
    OUT PHANDLE phEnum,
    IN LSA_FIND_FLAGS FindFlags,
    IN DWORD dwCount,
    IN PSTR* ppszSids
    );

typedef DWORD (*PFN_LSA_PROVIDER_ENUM_MEMBER_OF)(
    IN HANDLE hEnum,
    IN DWORD dwMaxObjectsCount,
    OUT PDWORD pdwObjectsCount,
    OUT PSTR** pppszSids
    );

//
// Close any enumeration handle.
//
typedef DWORD (*PFN_LSA_PROVIDER_CLOSE_ENUM)(
    IN OUT HANDLE hEnum
    );

typedef struct _LSA_PROVIDER_FUNCTION_TABLE_2 {

    PFN_LSA_PROVIDER_FIND_OBJECTS pfnFindObjects;
    //
    // Deprecates:
    //
    // PFNLOOKUPUSERBYNAME            pfnLookupUserByName;
    // PFNLOOKUPUSERBYID              pfnLookupUserById;
    // PFNLOOKUPGROUPBYNAME           pfnLookupGroupByName;
    // PFNLOOKUPGROUPBYID             pfnLookupGroupById;
    // PFNGETNAMESBYSIDLIST           pfnGetNamesBySidList;
    //
    // Adds:
    //
    // FindUserBySid
    // FindGroupBySid
    // FindUserByBn
    // FindGroupByDn
    //

    PFN_LSA_PROVIDER_OPEN_ENUM_OBJECTS pfnOpenEnumObjects;
    //
    // Deprecates:
    //
    // PFNBEGIN_ENUM_USERS            pfnBeginEnumUsers;
    // PFNBEGIN_ENUM_GROUPS           pfnBeginEnumGroups;
    //

    PFN_LSA_PROVIDER_OPEN_ENUM_MEMBERS pfnOpenEnumGroupMembers;
    PFN_LSA_PROVIDER_OPEN_ENUM_MEMBER_OF pfnOpenEnumMemberOf;
    //
    // Adds:
    //
    // BeginEnumUserGroups (paging functionality)
    // BeginEnumGroupMembers (paging functionality)
    //

    PFN_LSA_PROVIDER_CLOSE_ENUM pfnCloseEnum;
    //
    // Deprecates:
    //
    // PFNEND_ENUM_USERS              pfnEndEnumUsers;
    // PFNEND_ENUM_GROUPS             pfnEndEnumGroups;
    //
    // Adds:
    //
    // EndEnumUserGroups (paging functionality)
    // EndEnumGroupMembers (paging functionality)
    //

    PFN_LSA_PROVIDER_ENUM_OBJECTS pfnEnumObjects;
    PFN_LSA_PROVIDER_ENUM_MEMBERS pfnEnumGroupMembers;
    PFN_LSA_PROVIDER_ENUM_MEMBER_OF pfnEnumMemberOf;
    //
    // Deprecates:
    //
    // PFNENUMUSERS                   pfnEnumUsers;
    // PFNGETGROUPSFORUSER            pfnGetGroupsForUser;
    // PFNENUMGROUPS                  pfnEnumGroups;
    // PFNGETGROUPMEMBERSHIPBYPROV    pfnGetGroupMembershipByProvider;
    // group nfo level 1
    //
    // Adds:
    //
    // ability to get members of groups as something other than aliases.
    //

#if 1
    //
    // Untouched for now -- will at least change type names for readability/consistency.
    //
    PFNSHUTDOWNPROVIDER            pfnShutdownProvider; // ok
    PFNOPENHANDLE                  pfnOpenHandle; // we should be able to get rid of this and just pass in a LSA_PROVIDER_HANDLE that is created by SRV/API but that provider can attach context.
    PFNCLOSEHANDLE                 pfnCloseHandle; // "
    PFNSERVICESDOMAIN              pfnServicesDomain; // is it necessary?  if we can lookup domains, it is not.
    PFNAUTHENTICATEUSER            pfnAuthenticateUser; // ok
    PFNAUTHENTICATEUSEREX          pfnAuthenticateUserEx; // ok
    PFNVALIDATEUSER                pfnValidateUser; // This can be combined with the below (removing password)
    PFNCHECKUSERINLIST             pfnCheckUserInList; // see above
    PFNCHANGEPASSWORD              pfnChangePassword; // ok
    PFNSETPASSWORD                 pfnSetPassword; // ok -- local only unless we support set password protocol to set password as domain admin

    // local only?:
    PFNADDUSER                     pfnAddUser; // remove info level
    PFNMODIFYUSER                  pfnModifyUser; // tweak interface -- SID is primary
    PFNDELETEUSER                  pfnDeleteUser; // tweak interface -- SID is primary
    PFNADDGROUP                    pfnAddGroup; // remove info level
    PFNMODIFYGROUP                 pfnModifyGroup; // tweak interface -- SID is primary
    PFNDELETEGROUP                 pfnDeleteGroup; // tweak interface -- SID is primary

    // PAM
    PFNOPENSESSION                 pfnOpenSession;
    PFNCLOSESESSION                pfnCloseSession;

    // NSS -- pretty good, perhaps module info level/flags?
    PFNLOOKUP_NSS_ARTEFACT_BY_KEY  pfnLookupNSSArtefactByKey;
    PFNBEGIN_ENUM_NSS_ARTEFACTS    pfnBeginEnumNSSArtefacts;
    PFNENUMNSS_ARTEFACTS           pfnEnumNSSArtefacts;
    PFNEND_ENUM_NSS_ARTEFACTS      pfnEndEnumNSSArtefacts; // can use new close enum function

    PFNGET_STATUS                  pfnGetStatus;
    PFNFREE_STATUS                 pfnFreeStatus;

    PFNREFRESH_CONFIGURATION       pfnRefreshConfiguration;
    PFNPROVIDER_IO_CONTROL         pfnProviderIoControl; // fix interface wrt uid/gid stuff
#endif

} LSA_PROVIDER_FUNCTION_TABLE_2, *PLSA_PROVIDER_FUNCTION_TABLE_2;

#endif /* __LSAPROVIDER_2_H__ */

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
