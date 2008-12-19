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
 *        clientipc_p.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Private Header (Library)
 *
 *        Inter-process Communication (Client) API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __CLIENTIPC_P_H__
#define __CLIENTIPC_P_H__

#include <lwmsg/lwmsg.h>

#if 0
typedef struct __LSA_CLIENT_CONNECTION_CONTEXT {
    int    fd;
} LSA_CLIENT_CONNECTION_CONTEXT, *PLSA_CLIENT_CONNECTION_CONTEXT;
#endif

typedef struct __LSA_CLIENT_CONNECTION_CONTEXT
{
    LWMsgProtocol* pProtocol;
    LWMsgAssoc* pAssoc;
    HANDLE hServer; //PLSA_SRV_ENUM_STATE
} LSA_CLIENT_CONNECTION_CONTEXT, *PLSA_CLIENT_CONNECTION_CONTEXT;

DWORD
LsaTransactOpenServer(
   IN OUT HANDLE hServer
   );

DWORD
LsaTransactFindGroupByName(
   HANDLE hServer,
   PCSTR pszGroupName,
   LSA_FIND_FLAGS FindFlags,
   DWORD dwGroupInfoLevel,
   PVOID* ppGroupInfo
   );

DWORD
LsaTransactFindGroupById(
   HANDLE hServer,
   DWORD id,
   LSA_FIND_FLAGS FindFlags,
   DWORD dwGroupInfoLevel,
   PVOID* ppGroupInfo
   );

DWORD
LsaTransactBeginEnumGroups(
    HANDLE hServer,
    DWORD   dwGroupInfoLevel,
    DWORD   dwMaxNumGroups,
    PHANDLE phResume
    );

DWORD
LsaTransactEnumGroups(
    HANDLE hServer,
    HANDLE hResume,
    PDWORD pdwNumGroupsFound,
    PVOID** pppGroupInfoList
    );

DWORD
LsaTransactEndEnumGroups(
    HANDLE hServer,
    HANDLE hResume
    );

DWORD
LsaTransactAddGroup(
    HANDLE hServer,
    PVOID  pGroupInfo,
    DWORD  dwGroupInfoLevel
    );

DWORD
LsaTransactDeleteGroupById(
    HANDLE hServer,
    gid_t  gid
    );

DWORD
LsaTransactGetGroupsForUserById(
    HANDLE  hServer,
    uid_t   uid,
    LSA_FIND_FLAGS FindFlags,
    DWORD   dwGroupInfoLevel,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
    );

DWORD
LsaTransactFindUserByName(
    HANDLE hServer,
    PCSTR  pszName,
    DWORD  dwUserInfoLevel,
    PVOID* ppUserInfo
    );

DWORD
LsaTransactFindUserById(
    HANDLE hServer,
    uid_t uid,
    DWORD  dwUserInfoLevel,
    PVOID* ppUserInfo
    );

DWORD
LsaTransactBeginEnumUsers(
    HANDLE hServer,
    DWORD   dwUserInfoLevel,
    DWORD   dwMaxNumUsers,
    PHANDLE phResume
    );

DWORD
LsaTransactEnumUsers(
    HANDLE hServer,
    HANDLE hResume,
    PDWORD pdwNumUsersFound,
    PVOID** pppUserInfoList
    );

DWORD
LsaTransactEndEnumUsers(
    HANDLE hServer,
    HANDLE hResume
    );

DWORD
LsaTransactAddUser(
    HANDLE hServer,
    PVOID  pUserInfo,
    DWORD  dwUserInfoLevel
    );

DWORD
LsaTransactDeleteUserById(
    HANDLE hServer,
    uid_t  uid
    );

DWORD
LsaTransactAuthenticateUser(
    HANDLE hServer,
    PCSTR  pszLoginName,
    PCSTR  pszPassword
    );

DWORD
LsaTransactAuthenticateUserEx(
    IN HANDLE hServer,
    IN LSA_AUTH_USER_PARAMS* pParams,
    OUT LSA_AUTH_USER_INFO* pUserInfo
    );

DWORD
LsaTransactValidateUser(
    HANDLE hServer,
    PCSTR  pszLoginName,
    PCSTR  pszPassword
    );

DWORD
LsaTransactChangePassword(
    HANDLE hServer,
    PCSTR  pszLoginName,
    PCSTR  pszNewPassword,
    PCSTR  pszOldPassword
    );

DWORD
LsaTransactModifyUser(
    HANDLE hServer,
    PLSA_USER_MOD_INFO pUserModInfo
    );

DWORD
LsaTransactGetNamesBySidList(
    IN HANDLE hServer,
    IN size_t sCount,
    IN PSTR* ppszSidList,
    OUT PLSA_SID_INFO* ppSIDInfoList,
    OUT OPTIONAL CHAR *pchDomainSeparator
    );

#endif /* __CLIENTIPC_P_H__ */

