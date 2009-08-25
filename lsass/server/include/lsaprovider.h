/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        lsaprovider.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Authentication Provider Interface
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __LSAPROVIDER_H__
#define __LSAPROVIDER_H__

#include "lsautils.h"

typedef DWORD (*PFNOPENHANDLE)(
                    uid_t peerUID,
                    gid_t peerGID,
                    pid_t peerPID,
                    PHANDLE phProvider
                    );

typedef VOID  (*PFNCLOSEHANDLE)(HANDLE hProvider);

typedef BOOLEAN (*PFNSERVICESDOMAIN)(PCSTR pszDomain);

typedef DWORD (*PFNAUTHENTICATEUSER)(
                        HANDLE hProvider,
                        PCSTR  pszLoginId,
                        PCSTR  pszPassword
                        );

typedef DWORD (*PFNAUTHENTICATEUSEREX)(
                        HANDLE hProvider,
                        PLSA_AUTH_USER_PARAMS pUserParams,
                        PLSA_AUTH_USER_INFO *ppUserInfo
                        );

typedef DWORD (*PFNVALIDATEUSER)(
                        HANDLE hProvider,
                        PCSTR  pszLoginId,
                        PCSTR  pszPassword
                        );

typedef DWORD (*PFNCHECKUSERINLIST)(
                        HANDLE hProvider,
                        PCSTR  pszLoginId,
                        PCSTR  pszListName);

typedef DWORD (*PFNLOOKUPUSERBYNAME)(
                        HANDLE  hProvider,
                        PCSTR   pszLoginId,
                        DWORD   dwUserInfoLevel,
                        PVOID*  ppUserInfo
                        );

typedef DWORD (*PFNLOOKUPUSERBYID)(
                        HANDLE  hProvider,
                        uid_t   uid,
                        DWORD   dwUserInfoLevel,
                        PVOID*  ppUserInfo
                        );

typedef DWORD (*PFNLOOKUPGROUPBYNAME)(
                        HANDLE  hProvider,
                        PCSTR   pszLoginId,
                        LSA_FIND_FLAGS FindFlags,
                        DWORD   dwUserInfoLevel,
                        PVOID*  ppGroupInfo
                        );

typedef DWORD (*PFNLOOKUPGROUPBYID)(
                        HANDLE  hProvider,
                        gid_t   gid,
                        LSA_FIND_FLAGS FindFlags,
                        DWORD   dwGroupInfoLevel,
                        PVOID*  ppGroupInfo
                        );

typedef DWORD (*PFNGETGROUPSFORUSER)(
                        IN HANDLE hProvider,
                        IN OPTIONAL PCSTR pszUserName,
                        IN OPTIONAL uid_t uid,
                        IN LSA_FIND_FLAGS FindFlags,
                        IN DWORD dwGroupInfoLevel,
                        IN PDWORD pdwGroupsFound,
                        IN PVOID** pppGroupInfoList
                        );

typedef DWORD (*PFNBEGIN_ENUM_USERS)(
                        HANDLE  hProvider,
                        DWORD   dwInfoLevel,
                        LSA_FIND_FLAGS FindFlags,
                        PHANDLE phResume
                        );

typedef DWORD (*PFNENUMUSERS) (
                        HANDLE  hProvider,
                        HANDLE  hResume,
                        DWORD   dwMaxUsers,
                        PDWORD  pdwUsersFound,
                        PVOID** pppUserInfoList
                        );

typedef VOID (*PFNEND_ENUM_USERS)(
                        HANDLE hProvider,
                        HANDLE hResume
                        );

typedef DWORD (*PFNBEGIN_ENUM_GROUPS)(
                        HANDLE  hProvider,
                        DWORD   dwInfoLevel,
                        BOOLEAN bCheckGroupMembersOnline,
                        LSA_FIND_FLAGS FindFlags,
                        PHANDLE phResume
                        );

typedef DWORD (*PFNENUMGROUPS) (
                        HANDLE  hProvider,
                        HANDLE  hResume,
                        DWORD   dwMaxNumGroups,
                        PDWORD  pdwGroupsFound,
                        PVOID** pppGroupInfoList
                        );

typedef VOID (*PFNEND_ENUM_GROUPS)(
                        HANDLE hProvider,
                        HANDLE hResume
                        );

typedef DWORD (*PFNCHANGEPASSWORD) (
                        HANDLE hProvider,
                        PCSTR  pszLoginId,
                        PCSTR  pszPassword,
                        PCSTR  pszOldPassword
                        );

typedef DWORD (*PFNSETPASSWORD) (
                        HANDLE hProvider,
                        PCSTR  pszLoginId,
                        PCSTR  pszPassword
                        );

typedef DWORD (*PFNADDUSER) (
                        HANDLE hProvider,
                        DWORD  dwUserInfoLevel,
                        PVOID  pUserInfo
                        );

typedef DWORD (*PFNMODIFYUSER)(
                        HANDLE hProvider,
                        PLSA_USER_MOD_INFO pUserModInfo
                        );

typedef DWORD (*PFNDELETEUSER) (
                        HANDLE hProvider,
                        uid_t  uid
                        );

typedef DWORD (*PFNADDGROUP) (
                        HANDLE hProvider,
                        DWORD  dwGroupInfoLevel,
                        PVOID  pGroupInfo
                        );

typedef DWORD (*PFNMODIFYGROUP) (
                        HANDLE hProvider,
                        PLSA_GROUP_MOD_INFO pGroupModInfo
                        );

typedef DWORD (*PFNDELETEGROUP) (
                        HANDLE hProvider,
                        gid_t  gid
                        );

typedef DWORD (*PFNOPENSESSION) (
                        HANDLE hProvider,
                        PCSTR  pszLoginId
                        );

typedef DWORD (*PFNCLOSESESSION) (
                        HANDLE hProvider,
                        PCSTR  pszLoginId
                        );

typedef DWORD (*PFNGETNAMESBYSIDLIST) (
                        HANDLE          hProvider,
                        size_t          sCount,
                        PSTR*           ppszSidList,
                        PSTR**          pppszDomainNames,
                        PSTR**          pppszSamAccounts,
                        ADAccountType** ppTypes
                        );

typedef DWORD (*PFNLOOKUP_NSS_ARTEFACT_BY_KEY)(
                        HANDLE hProvider,
                        PCSTR  pszKeyName,
                        PCSTR  pszMapName,
                        DWORD  dwInfoLevel,
                        LSA_NIS_MAP_QUERY_FLAGS dwFlags,
                        PVOID* ppNSSArtefactInfo
                        );

typedef DWORD (*PFNBEGIN_ENUM_NSS_ARTEFACTS)(
                        HANDLE  hProvider,
                        DWORD   dwInfoLevel,
                        PCSTR   pszMapName,
                        LSA_NIS_MAP_QUERY_FLAGS dwFlags,
                        PHANDLE phResume
                        );

typedef DWORD (*PFNENUMNSS_ARTEFACTS) (
                        HANDLE  hProvider,
                        HANDLE  hResume,
                        DWORD   dwMaxNumGroups,
                        PDWORD  pdwGroupsFound,
                        PVOID** pppGroupInfoList
                        );

typedef VOID (*PFNEND_ENUM_NSS_ARTEFACTS)(
                        HANDLE hProvider,
                        HANDLE hResume
                        );

typedef DWORD (*PFNGET_STATUS)(
                HANDLE hProvider,
                PLSA_AUTH_PROVIDER_STATUS* ppAuthProviderStatus
                );

typedef VOID (*PFNFREE_STATUS)(
                PLSA_AUTH_PROVIDER_STATUS pAuthProviderStatus
                );

typedef DWORD (*PFNREFRESH_CONFIGURATION)();

typedef DWORD (*PFNPROVIDER_IO_CONTROL) (
                HANDLE hProvider,
                uid_t  peerUid,
                gid_t  peerGID,
                DWORD  dwIoControlCode,
                DWORD  dwInputBufferSize,
                PVOID  pInputBuffer,
                DWORD* pdwOutputBufferSize,
                PVOID* ppOutputBuffer
                );

typedef DWORD (*PFNGETGROUPMEMBERSHIPBYPROV)(
                IN HANDLE     hProvider,
                IN PCSTR      pszSid,
                IN DWORD      dwGroupInfoLevel,
                OUT PDWORD    pdwGroupsCount,
                OUT PVOID   **pppMembershipInfo
                );

typedef struct __LSA_PROVIDER_FUNCTION_TABLE
{
    PFNOPENHANDLE                  pfnOpenHandle;
    PFNCLOSEHANDLE                 pfnCloseHandle;
    PFNSERVICESDOMAIN              pfnServicesDomain;
    PFNAUTHENTICATEUSER            pfnAuthenticateUser;
    PFNAUTHENTICATEUSEREX          pfnAuthenticateUserEx;
    PFNVALIDATEUSER                pfnValidateUser;
    PFNCHECKUSERINLIST             pfnCheckUserInList;
    PFNLOOKUPUSERBYNAME            pfnLookupUserByName;
    PFNLOOKUPUSERBYID              pfnLookupUserById;
    PFNBEGIN_ENUM_USERS            pfnBeginEnumUsers;
    PFNENUMUSERS                   pfnEnumUsers;
    PFNEND_ENUM_USERS              pfnEndEnumUsers;
    PFNLOOKUPGROUPBYNAME           pfnLookupGroupByName;
    PFNLOOKUPGROUPBYID             pfnLookupGroupById;
    PFNGETGROUPSFORUSER            pfnGetGroupsForUser;
    PFNBEGIN_ENUM_GROUPS           pfnBeginEnumGroups;
    PFNENUMGROUPS                  pfnEnumGroups;
    PFNEND_ENUM_GROUPS             pfnEndEnumGroups;
    PFNCHANGEPASSWORD              pfnChangePassword;
    PFNSETPASSWORD                 pfnSetPassword;
    PFNADDUSER                     pfnAddUser;
    PFNMODIFYUSER                  pfnModifyUser;
    PFNDELETEUSER                  pfnDeleteUser;
    PFNADDGROUP                    pfnAddGroup;
    PFNMODIFYGROUP                 pfnModifyGroup;
    PFNDELETEGROUP                 pfnDeleteGroup;
    PFNOPENSESSION                 pfnOpenSession;
    PFNCLOSESESSION                pfnCloseSession;
    PFNGETNAMESBYSIDLIST           pfnGetNamesBySidList;
    PFNLOOKUP_NSS_ARTEFACT_BY_KEY  pfnLookupNSSArtefactByKey;
    PFNBEGIN_ENUM_NSS_ARTEFACTS    pfnBeginEnumNSSArtefacts;
    PFNENUMNSS_ARTEFACTS           pfnEnumNSSArtefacts;
    PFNEND_ENUM_NSS_ARTEFACTS      pfnEndEnumNSSArtefacts;
    PFNGET_STATUS                  pfnGetStatus;
    PFNFREE_STATUS                 pfnFreeStatus;
    PFNREFRESH_CONFIGURATION       pfnRefreshConfiguration;
    PFNPROVIDER_IO_CONTROL         pfnProviderIoControl;
    PFNGETGROUPMEMBERSHIPBYPROV    pfnGetGroupMembershipByProvider;
} LSA_PROVIDER_FUNCTION_TABLE, *PLSA_PROVIDER_FUNCTION_TABLE;

#define LSA_SYMBOL_NAME_INITIALIZE_PROVIDER "LsaInitializeProvider"

typedef DWORD (*PFNINITIALIZEPROVIDER)(
                    PCSTR pszConfigFilePath,
                    PSTR* ppszProviderName,
                    PLSA_PROVIDER_FUNCTION_TABLE* ppFnTable
                    );

#define LSA_SYMBOL_NAME_SHUTDOWN_PROVIDER "LsaShutdownProvider"

typedef DWORD (*PFNSHUTDOWNPROVIDER)(
                    PSTR pszProviderName,
                    PLSA_PROVIDER_FUNCTION_TABLE pFnTable
                    );

typedef struct _LSA_STATIC_PROVIDER
{
    PCSTR pszId;
    PFNINITIALIZEPROVIDER pInitialize;
    PFNSHUTDOWNPROVIDER pShutdown;
} LSA_STATIC_PROVIDER, *PLSA_STATIC_PROVIDER;

#ifdef ENABLE_STATIC_PROVIDERS
#define LSA_INITIALIZE_PROVIDER(name) LsaInitializeProvider_##name
#define LSA_SHUTDOWN_PROVIDER(name) LsaShutdownProvider_##name
#else
#define LSA_INITIALIZE_PROVIDER(name) LsaInitializeProvider
#define LSA_SHUTDOWN_PROVIDER(name) LsaShutdownProvider
#endif

#define LSA_STATIC_PROVIDER_ENTRY(name, id) \
    { #id, LSA_INITIALIZE_PROVIDER(name), LSA_SHUTDOWN_PROVIDER(name) }

#define LSA_STATIC_PROVIDER_END \
    { NULL, NULL, NULL }

#endif /* __LSAPROVIDER_H__ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
