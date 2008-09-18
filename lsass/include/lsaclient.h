/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */
 
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
LsaGetGroupsForUserName(
    HANDLE  hLsaConnection,
    PCSTR   pszUserName,    
    PDWORD  pdwGroupFound,
    gid_t** ppGidResults
    );

DWORD
LsaFindGroupByName(
    HANDLE hLsaConnection,
    PCSTR  pszGroupName,
    DWORD  dwGroupInfoLevel,
    PVOID* ppGroupInfo
    );

DWORD
LsaFindGroupById(
    HANDLE hLsaConnection,
    gid_t  gid,
    DWORD  dwGroupInfoLevel,
    PVOID* ppGroupInfo
    );

DWORD
LsaBeginEnumGroups(
    HANDLE  hLsaConnection,
    DWORD   dwGroupInfoLevel,
    DWORD   dwMaxNumGroups,
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
LsaGetNamesBySidList(
    HANDLE          hLsaConnection,
    size_t          sCount,
    PSTR*           ppszSidList,
    PLSA_SID_INFO*  ppSIDInfoList
    );

DWORD
LsaBeginEnumNSSArtefacts(
    HANDLE  hLsaConnection,
    DWORD   dwInfoLevel,
    DWORD   dwMapType,
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
