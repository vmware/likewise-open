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
                    PHANDLE phProvider
                    );

typedef VOID  (*PFNCLOSEHANDLE)(HANDLE hProvider);

typedef BOOLEAN (*PFNSERVICESDOMAIN)(PCSTR pszDomain);

typedef DWORD (*PFNAUTHENTICATEUSER)(
                        HANDLE hProvider,
                        PCSTR  pszLoginId,
                        PCSTR  pszPassword
                        );

typedef DWORD (*PFNVALIDATEUSER)(
                        HANDLE hProvider,
                        PCSTR  pszLoginId,
                        PCSTR  pszPassword
                        );

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
                        DWORD   dwUserInfoLevel,
                        PVOID*  ppGroupInfo
                        );

typedef DWORD (*PFNLOOKUPGROUPBYID)(
                        HANDLE  hProvider,
                        gid_t   gid,
                        DWORD   dwGroupInfoLevel,
                        PVOID*  ppGroupInfo
                        );

typedef DWORD (*PFNGETGROUPSFORUSER)(
                        HANDLE  hProvider,
                        uid_t   uid,
                        DWORD   dwGroupInfoLevel,
                        PDWORD  pdwGroupsFound,
                        PVOID** pppGroupInfoList
                        );

typedef DWORD (*PFNBEGIN_ENUM_USERS)(
                        HANDLE  hProvider,
                        PCSTR   pszGUID,
                        DWORD   dwInfoLevel,
                        PHANDLE phResume
                        );

typedef DWORD (*PFNENUMUSERS) (
                        HANDLE  hProvider,
                        HANDLE  hResume,
                        DWORD   ndwMaxUsers,
                        PDWORD  pdwUsersFound,
                        PVOID** pppUserInfoList
                        );

typedef VOID (*PFNEND_ENUM_USERS)(
                        HANDLE hProvider,
                        PCSTR  pszGUID
                        );

typedef DWORD (*PFNBEGIN_ENUM_GROUPS)(
                        HANDLE  hProvider,
                        PCSTR   pszGUID,
                        DWORD   dwInfoLevel,
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
                        PCSTR  pszGUID
                        );

typedef DWORD (*PFNCHANGEPASSWORD) (
                        HANDLE hProvider,
                        PCSTR  pszLoginId,
                        PCSTR  pszPassword,
                        PCSTR  pszOldPassword
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
                        ADAccountType** ppTypes);



typedef DWORD (*PFNBEGIN_ENUM_NSS_ARTEFACTS)(
                        HANDLE  hProvider,
                        PCSTR   pszGUID,
                        DWORD   dwInfoLevel,
                        DWORD   dwMapType,
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
                        PCSTR  pszGUID
                        );

typedef DWORD (*PFNGET_STATUS)(
                HANDLE hProvider,
                PLSA_AUTH_PROVIDER_STATUS* ppAuthProviderStatus
                );

typedef VOID (*PFNFREE_STATUS)(
                PLSA_AUTH_PROVIDER_STATUS pAuthProviderStatus
                );

typedef DWORD (*PFNREFRESH_CONFIGURATION)();

typedef struct __LSA_PROVIDER_FUNCTION_TABLE
{
    PFNOPENHANDLE                  pfnOpenHandle;
    PFNCLOSEHANDLE                 pfnCloseHandle;
    PFNSERVICESDOMAIN              pfnServicesDomain;
    PFNAUTHENTICATEUSER            pfnAuthenticateUser;
    PFNVALIDATEUSER                pfnValidateUser;
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
    PFNADDUSER                     pfnAddUser;
    PFNMODIFYUSER                  pfnModifyUser;
    PFNDELETEUSER                  pfnDeleteUser;
    PFNADDGROUP                    pfnAddGroup;
    PFNDELETEGROUP                 pfnDeleteGroup;
    PFNOPENSESSION                 pfnOpenSession;
    PFNCLOSESESSION                pfnCloseSession;
    PFNGETNAMESBYSIDLIST           pfnGetNamesBySidList;
    PFNBEGIN_ENUM_NSS_ARTEFACTS    pfnBeginEnumNSSArtefacts;
    PFNENUMNSS_ARTEFACTS           pfnEnumNSSArtefacts;
    PFNEND_ENUM_NSS_ARTEFACTS      pfnEndEnumNSSArtefacts;
    PFNGET_STATUS                  pfnGetStatus;
    PFNFREE_STATUS                 pfnFreeStatus;
    PFNREFRESH_CONFIGURATION       pfnRefreshConfiguration;
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

#endif /* __LSAPROVIDER_H__ */

