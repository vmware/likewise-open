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
 *        externs.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 * 
 *        Local Authentication Provider
 * 
 *        Main
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __PROVIDER_MAIN_H__
#define __PROVIDER_MAIN_H__

DWORD
LsaInitializeProvider(
    PCSTR pszConfigFilePath,
    PSTR* ppszProviderName,
    PLSA_PROVIDER_FUNCTION_TABLE* ppFunctionTable
    );

DWORD
LsaProviderLocal_ConfigStartSection(
    PCSTR    pszSectionName,
    PVOID    pData,
    PBOOLEAN pbSkipSection,
    PBOOLEAN pbContinue
    );

DWORD
LsaProviderLocal_ConfigNameValuePair(
    PCSTR    pszName,
    PCSTR    pszValue,
    PVOID    pData,
    PBOOLEAN pbContinue
    );

DWORD
LsaProviderLocal_OpenHandle(
    uid_t uid,
    gid_t gid,
    PHANDLE phProvider
    );

void
LsaProviderLocal_CloseHandle(
    HANDLE hProvider
    );

BOOLEAN
LsaProviderLocal_ServicesDomain(
    PCSTR pszDomain
    );

DWORD
LsaProviderLocal_AuthenticateUser(
    HANDLE hProvider,
    PCSTR  pszLoginId,
    PCSTR  pszPassword
    );

DWORD
LsaProviderLocal_ValidateUser(
    HANDLE hProvider,
    PCSTR  pszLoginId,
    PCSTR  pszPassword
    );

DWORD
LsaProviderLocal_FindUserByName(
    HANDLE  hProvider,
    PCSTR   pszLoginId,
    DWORD   dwUserInfoLevel,
    PVOID*  ppUserInfo
    );

DWORD
LsaProviderLocal_FindUserById(
    HANDLE  hProvider,
    uid_t   uid,
    DWORD   dwUserInfoLevel,
    PVOID*  ppUserInfo
    );

DWORD
LsaProviderLocal_BeginEnumUsers(
    HANDLE  hProvider,
    PCSTR   pszGUID,
    DWORD   dwInfoLevel,
    PHANDLE phResume
    );

DWORD
LsaProviderLocal_EnumUsers(
    HANDLE   hProvider,
    HANDLE   hResume,
    DWORD    dwMaxNumRecords,
    PDWORD   pdwUsersFound,
    PVOID**  pppUserInfoList
    );

VOID
LsaProviderLocal_EndEnumUsers(
    HANDLE hProvider,
    PCSTR  pszGUID
    );

DWORD
LsaProviderLocal_FindGroupByName(
    HANDLE  hProvider,
    PCSTR   pszGroupName,
    DWORD   dwGroupInfoLevel,
    PVOID*  ppGroupInfo
    );

DWORD
LsaProviderLocal_FindGroupById(
    HANDLE  hProvider,
    gid_t   gid,
    DWORD   dwGroupInfoLevel,
    PVOID*  ppGroupInfo
    );

DWORD
LsaProviderLocal_GetGroupsForUser(
    HANDLE  hProvider,
    uid_t   uid,
    DWORD   dwGroupInfoLevel,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
    );

DWORD
LsaProviderLocal_BeginEnumGroups(
    HANDLE  hProvider,
    PCSTR   pszGUID,
    DWORD   dwInfoLevel,
    PHANDLE phResume
    );

DWORD
LsaProviderLocal_EnumGroups(
    HANDLE   hProvider,
    HANDLE   hResume,
    DWORD    dwMaxGroups,
    PDWORD   pdwGroupsFound,
    PVOID**  pppGroupInfoList
    );

VOID
LsaProviderLocal_EndEnumGroups(
    HANDLE hProvider,
    PCSTR  pszGUID
    );

DWORD
LsaProviderLocal_ChangePassword(
    HANDLE hProvider,
    PCSTR  pszLoginId,
    PCSTR  pszPassword,
    PCSTR  pszOldPassword
    );

DWORD
LsaProviderLocal_AddUser(
    HANDLE hProvider,
    DWORD  dwUserInfoLevel,
    PVOID  pUserInfo
    );

DWORD
LsaProviderLocal_ModifyUser(
    HANDLE hProvider,
    PLSA_USER_MOD_INFO pUserModInfo
    );

DWORD
LsaProviderLocal_DeleteUser(
    HANDLE hProvider,
    uid_t  uid
    );

DWORD
LsaProviderLocal_AddGroup(
    HANDLE hProvider,
    DWORD  dwGroupInfoLevel,
    PVOID  pGroupInfo
    );

DWORD
LsaProviderLocal_DeleteGroup(
    HANDLE hProvider,
    gid_t  gid
    );

DWORD
LsaProviderLocal_OpenSession(
    HANDLE hProvider,
    PCSTR  pszLoginId
    );

DWORD
LsaProviderLocal_CreateHomeDirectory(
    PLSA_USER_INFO_0 pUserInfo
    );

DWORD
LsaProviderLocal_ProvisionHomeDir(
    uid_t ownerUid,
    gid_t ownerGid,
    PCSTR pszHomedirPath
    );

DWORD
LsaProviderLocal_CloseSession(
    HANDLE hProvider,
    PCSTR  pszLoginId
    );

DWORD
LsaShutdownProvider(
    PSTR pszProviderName,
    PLSA_PROVIDER_FUNCTION_TABLE pFnTable
    );

DWORD
LsaProviderLocal_GetNamesBySidList(
    HANDLE          hProvider,
    size_t          sCount,
    PSTR*           ppszSidList,
    PSTR**          pppszDomainNames,
    PSTR**          pppszSamAccounts,
    ADAccountType** ppTypes);

DWORD
LsaProviderLocal_BeginEnumNSSArtefacts(
    HANDLE  hProvider,
    PCSTR   pszGUID,
    DWORD   dwInfoLevel,
    DWORD   dwMapType,
    PHANDLE phResume
    );

DWORD
LsaProviderLocal_EnumNSSArtefacts(
    HANDLE  hProvider,
    HANDLE  hResume,
    DWORD   dwMaxNSSArtefacts,
    PDWORD  pdwNSSArtefactsFound,
    PVOID** pppNSSArtefactInfoList
    );

VOID
LsaProviderLocal_EndEnumNSSArtefacts(
    HANDLE hProvider,
    PCSTR  pszGUID
    );

DWORD
LsaProviderLocal_GetStatus(
    HANDLE hProvider,
    PLSA_AUTH_PROVIDER_STATUS* ppProviderStatus
    );

VOID
LsaProviderLocal_FreeStatus(
    PLSA_AUTH_PROVIDER_STATUS pProviderStatus
    );

DWORD
LsaProviderLocal_RefreshConfiguration(
    HANDLE hProvider
    );

#endif /* __PROVIDER_MAIN_H__ */
