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
 *        state_p.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Server State Management API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __STATE_P_H__
#define __STATE_P_H__

#include "auth_provider_p.h"

typedef struct __LSA_SRV_PROVIDER_STATE {

    PLSA_AUTH_PROVIDER pProvider;
    HANDLE hProvider;
    HANDLE hResume;

    struct __LSA_SRV_PROVIDER_STATE *pNext;

} LSA_SRV_PROVIDER_STATE, *PLSA_SRV_PROVIDER_STATE;

typedef struct __LSA_SRV_RECORD_ENUM_STATE {

    PSTR    pszGUID;
    DWORD   dwInfoLevel;
    DWORD   dwNumMaxRecords;
    BOOLEAN bCheckOnline;
    DWORD   dwMapFlags;
    PSTR    pszMapName;

    BOOLEAN bInLock;

    PLSA_SRV_PROVIDER_STATE pProviderStateList;
    PLSA_SRV_PROVIDER_STATE pCurProviderState;

    struct __LSA_SRV_RECORD_ENUM_STATE * pNext;

} LSA_SRV_RECORD_ENUM_STATE, *PLSA_SRV_RECORD_ENUM_STATE;

typedef struct __LSA_SRV_API_STATE
{
    uid_t  peerUID;
    gid_t  peerGID;
    HANDLE hEventLog;
} LSA_SRV_API_STATE, *PLSA_SRV_API_STATE;

typedef struct __LSA_SRV_ENUM_STATE
{
    PLSA_SRV_RECORD_ENUM_STATE pUserEnumStateList;

    PLSA_SRV_RECORD_ENUM_STATE pGroupEnumStateList;

    PLSA_SRV_RECORD_ENUM_STATE pNSSArtefactEnumStateList;

} LSA_SRV_ENUM_STATE, *PLSA_SRV_ENUM_STATE;

DWORD
LsaSrvOpenProvider(
    HANDLE  hServer,
    PLSA_AUTH_PROVIDER pProvider,
    PHANDLE phProvider
    );

VOID
LsaSrvCloseProvider(
    PLSA_AUTH_PROVIDER pProvider,
    HANDLE hProvider
    );

VOID
LsaSrvFreeProviderStateList(
    PLSA_SRV_PROVIDER_STATE pStateList
    );

PLSA_SRV_PROVIDER_STATE
LsaSrvReverseProviderStateList(
    PLSA_SRV_PROVIDER_STATE pStateList
    );

DWORD
LsaSrvAddUserEnumState(
    HANDLE  hServer,
    HANDLE  hServerEnum,
    DWORD   dwUserInfoLevel,
    DWORD   dwMaxNumUsers,
    PLSA_SRV_RECORD_ENUM_STATE* ppEnumState
    );

PLSA_SRV_RECORD_ENUM_STATE
LsaSrvFindUserEnumState(
    HANDLE hServer,
    PCSTR  pszGUID
    );

VOID
LsaSrvFreeUserEnumState(
    HANDLE hServer,
    PCSTR  pszGUID
    );

DWORD
LsaSrvAddGroupEnumState(
    HANDLE hServer,
    HANDLE hEnumServer,
    DWORD   dwGroupInfoLevel,
    DWORD   dwMaxNumGroups,
    BOOLEAN bCheckOnline,
    PLSA_SRV_RECORD_ENUM_STATE* ppEnumState
    );

PLSA_SRV_RECORD_ENUM_STATE
LsaSrvFindGroupEnumState(
    HANDLE hServer,
    PCSTR  pszGUID
    );

VOID
LsaSrvFreeGroupEnumState(
    HANDLE hServer,
    PCSTR  pszGUID
    );

DWORD
LsaSrvAddNSSArtefactEnumState(
    HANDLE  hServer,
    HANDLE  hEnumServer,
    PCSTR   pszMapName,
    LSA_NIS_MAP_QUERY_FLAGS dwFlags,
    DWORD   dwNSSArtefactInfoLevel,
    DWORD   dwMaxNumArtefacts,
    PLSA_SRV_RECORD_ENUM_STATE* ppEnumState
    );

PLSA_SRV_RECORD_ENUM_STATE
LsaSrvFindNSSArtefactEnumState(
    HANDLE hServer,
    PCSTR  pszGUID
    );

VOID
LsaSrvFreeNSSArtefactEnumState(
    HANDLE hServer,
    PCSTR  pszGUID
    );


#endif /* __STATE_P_H__ */

