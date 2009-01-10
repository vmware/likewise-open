/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
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
 *        lsaclient.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS) Client API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */
#ifndef __LSACLIENT_H__
#define __LSACLIENT_H__

#include "lsautils.h"

DWORD
LsaOpenServer(
    PHANDLE phConnection
    );

DWORD
LsaGetLogInfo(
    HANDLE         hLsaConnection,
    PLSA_LOG_INFO* ppLogInfo
    );

DWORD
LsaSetLogInfo(
    HANDLE        hLsaConnection,
    PLSA_LOG_INFO pLogInfo
    );

DWORD
LsaAddGroup(
    HANDLE hLsaConnection,
    PVOID  pGroupInfo,
    DWORD  dwGroupInfoLevel
    );

DWORD
LsaDeleteGroupById(
    HANDLE hLsaConnection,
    gid_t  gid
    );

DWORD
LsaDeleteGroupByName(
    HANDLE hLsaConnection,
    PCSTR  pszName
    );

DWORD
LsaGetGidsForUserByName(
    HANDLE  hLsaConnection,
    PCSTR   pszUserName,
    PDWORD  pdwGroupFound,
    gid_t** ppGidResults
    );

DWORD
LsaBeginEnumGroups(
    HANDLE  hLsaConnection,
    DWORD   dwGroupInfoLevel,
    DWORD   dwMaxNumGroups,
    PHANDLE phResume
    );

DWORD
LsaBeginEnumGroupsWithCheckOnlineOption(
    HANDLE  hLsaConnection,
    DWORD   dwGroupInfoLevel,
    DWORD   dwMaxNumGroups,
    BOOLEAN bCheckGroupMembersOnline,
    PHANDLE phResume
    );

DWORD
LsaEnumGroups(
    HANDLE  hLsaConnection,
    HANDLE  hResume,
    PDWORD  pdwNumGroupsFound,
    PVOID** pppGroupsInfoList
    );

DWORD
LsaEndEnumGroups(
    HANDLE  hLsaConnection,
    HANDLE  hResume
    );

DWORD
LsaAddUser(
    HANDLE hLsaConnection,
    PVOID  pUserInfo,
    DWORD  dwUserInfoLevel
    );

DWORD
LsaModifyUser(
    HANDLE hLsaConnection,
    PLSA_USER_MOD_INFO pUserModInfo
    );

DWORD
LsaChangeUser(
    HANDLE hLsaConnection,
    PVOID  pUserInfo,
    DWORD  dwUserInfoLevel
    );

DWORD
LsaDeleteUserById(
    HANDLE hLsaConnection,
    uid_t  uid
    );

DWORD
LsaDeleteUserByName(
    HANDLE hLsaConnection,
    PCSTR  pszName
    );

DWORD
LsaFindUserByName(
    HANDLE hLsaConnection,
    PCSTR  pszName,
    DWORD  dwUserInfoLevel,
    PVOID* ppUserInfo
    );

DWORD
LsaFindUserById(
    HANDLE hLsaConnection,
    uid_t  uid,
    DWORD  dwUserInfoLevel,
    PVOID* ppUserInfo
    );

DWORD
LsaBeginEnumUsers(
    HANDLE  hLsaConnection,
    DWORD   dwUserInfoLevel,
    DWORD   dwMaxNumUsers,
    PHANDLE phResume
    );

DWORD
LsaEnumUsers(
    HANDLE  hLsaConnection,
    HANDLE  hResume,
    PDWORD  pdwNumUsersFound,
    PVOID** pppUserInfoList
    );

DWORD
LsaEndEnumUsers(
    HANDLE hLsaConnection,
    HANDLE hResume
    );

DWORD
LsaAuthenticateUser(
    HANDLE hLsaConnection,
    PCSTR  pszLoginName,
    PCSTR  pszPassword
    );

DWORD
LsaValidateUser(
    HANDLE hLsaConnection,
    PCSTR  pszLoginName,
    PCSTR  pszPassword
    );

DWORD
LsaChangePassword(
    HANDLE hLsaConnection,
    PCSTR  pszLoginName,
    PCSTR  pszNewPassword,
    PCSTR  pszOldPassword
    );

DWORD
LsaOpenSession(
    HANDLE hLsaConnection,
    PCSTR  pszLoginId
    );

DWORD
LsaCloseUserLogonSession(
    HANDLE hLsaConnection,
    PCSTR  pszLoginId
    );

DWORD
LsaCloseServer(
    HANDLE hConnection
    );

DWORD
LsaFindNSSArtefactByKey(
    HANDLE   hLsaConnection,
    DWORD    dwMapInfoLevel,
    PCSTR    pszKeyName,
    PCSTR    pszMapName,
    LSA_NIS_MAP_QUERY_FLAGS dwFlags,
    PVOID*   ppNSSArtefactInfo
    );

DWORD
LsaBeginEnumNSSArtefacts(
    HANDLE  hLsaConnection,
    DWORD   dwInfoLevel,
    PCSTR   pszMapName,
    LSA_NIS_MAP_QUERY_FLAGS dwFlags,
    DWORD   dwMaxNumNSSArtefacts,
    PHANDLE phResume
    );

DWORD
LsaEnumNSSArtefacts(
    HANDLE  hLsaConnection,
    HANDLE  hResume,
    PDWORD  pdwNumNSSArtefactsFound,
    PVOID** pppNSSArtefactInfoList
    );

DWORD
LsaEndEnumNSSArtefacts(
    HANDLE hLsaConnection,
    HANDLE hResume
    );

DWORD
LsaProviderIoControl(
    IN HANDLE    hLsaConnection,
    IN PCSTR     pszProviderId,
    IN DWORD     dwIoControlCode,
    IN DWORD     dwInputBufferSize,
    IN PVOID     pInputBuffer,
    OUT OPTIONAL DWORD* pdwOutputBufferSize,
    OUT OPTIONAL PVOID* ppOutputBuffer
    );

//
// GSS routines
//

DWORD
LsaGSSBuildAuthMessage(
    HANDLE          hLsaConnection,
    PSEC_BUFFER     credentials,
    PSEC_BUFFER_S   serverChallenge,
    PSEC_BUFFER     targetInfo,
    ULONG           negotiateFlags,
    PSEC_BUFFER     authenticateMessage,
    PSEC_BUFFER_S   baseSessionKey
    );

DWORD
LsaGSSValidateAuthMessage(
    HANDLE          hLsaConnection,
    ULONG           negFlags,
    PSEC_BUFFER_S   serverChallenge,
    PSEC_BUFFER     targetInfo,
    PSEC_BUFFER     authenticateMessage,
    PSEC_BUFFER_S   baseSessionKey
    );

DWORD
LsaGetMetrics(
    HANDLE hLsaConnection,
    DWORD  dwInfoLevel,
    PVOID* ppMetricPack
    );

#endif /* __LSACLIENT_H__ */
