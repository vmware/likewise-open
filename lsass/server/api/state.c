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
 *        state.c
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
#include "api.h"

static
void
LsaFreeEnumRecordStateList(
   PLSA_SRV_RECORD_ENUM_STATE pStateList
   )
{
    PLSA_SRV_RECORD_ENUM_STATE pState = NULL;
    while (pStateList) {
        pState = pStateList;
        pStateList = pStateList->pNext;

        LSA_SAFE_FREE_STRING(pState->pszGUID);
        LSA_SAFE_FREE_STRING(pState->pszMapName);

        if (pState->pProviderStateList) {
            LsaSrvFreeProviderStateList(pState->pProviderStateList);
        }

        LEAVE_AUTH_PROVIDER_LIST_READER_LOCK(pState->bInLock);

        LsaFreeMemory(pState);
    }
}

void
LsaSrvCloseServer(
    HANDLE hServer
    )
{
    PLSA_SRV_API_STATE pServerState = (PLSA_SRV_API_STATE)hServer;

    if (pServerState->hEventLog != (HANDLE)NULL)
    {
       LsaSrvCloseEventLog(pServerState->hEventLog);
    }

    LsaFreeMemory(pServerState);
}

void
LsaSrvCloseServerEnum(
    HANDLE hServer
    )
{
    PLSA_SRV_ENUM_STATE pServerState = (PLSA_SRV_ENUM_STATE)hServer;

    LsaFreeEnumRecordStateList(pServerState->pGroupEnumStateList);

    LsaFreeEnumRecordStateList(pServerState->pNSSArtefactEnumStateList);

    LsaFreeEnumRecordStateList(pServerState->pUserEnumStateList);

    LsaFreeMemory(pServerState);
}

VOID
LsaSrvCloseProvider(
    PLSA_AUTH_PROVIDER pProvider,
    HANDLE hProvider
    )
{
    if (pProvider) {
        pProvider->pFnTable->pfnCloseHandle(hProvider);
    }
}

DWORD
LsaSrvOpenProvider(
    HANDLE  hServer,
    PLSA_AUTH_PROVIDER pProvider,
    PHANDLE phProvider
    )
{
    DWORD dwError = 0;
    HANDLE hProvider = (HANDLE)NULL;
    PLSA_SRV_API_STATE pServerState = (PLSA_SRV_API_STATE)hServer;

    dwError = pProvider->pFnTable->pfnOpenHandle(
                                        pServerState->peerUID,
                                        pServerState->peerGID,
                                        &hProvider);
    BAIL_ON_LSA_ERROR(dwError);

    *phProvider = hProvider;

cleanup:

    return dwError;

error:

    *phProvider = (HANDLE)NULL;

    goto cleanup;
}

DWORD
LsaSrvOpenServer(
    uid_t peerUID,
    gid_t peerGID,
    PHANDLE phServer
    )
{
    DWORD dwError = 0;
    PLSA_SRV_API_STATE pServerState = NULL;

    dwError = LsaAllocateMemory(
                    sizeof(*pServerState),
                    (PVOID*)&pServerState);
    BAIL_ON_LSA_ERROR(dwError);

    pServerState->peerUID = peerUID;
    pServerState->peerGID = peerGID;

    *phServer = (HANDLE)pServerState;

cleanup:

    return dwError;

error:

    *phServer = (HANDLE)NULL;

    if (pServerState) {
        LsaSrvCloseServer((HANDLE)pServerState);
    }

    goto cleanup;
}

DWORD
LsaSrvOpenServerEnum(
    PHANDLE phServer
    )
{
    DWORD dwError = 0;
    PLSA_SRV_ENUM_STATE pServerState = NULL;

    dwError = LsaAllocateMemory(
                    sizeof(*pServerState),
                    (PVOID*)&pServerState);
    BAIL_ON_LSA_ERROR(dwError);

    pServerState->pGroupEnumStateList = NULL;
    pServerState->pNSSArtefactEnumStateList = NULL;
    pServerState->pUserEnumStateList = NULL;

    *phServer = (HANDLE)pServerState;

cleanup:

    return dwError;

error:

    *phServer = (HANDLE)NULL;

    if (pServerState) {
        LsaSrvCloseServerEnum((HANDLE)pServerState);
    }

    goto cleanup;
}

VOID
LsaSrvGetUid(
    HANDLE hServer,
    uid_t* pUid
    )
{
    PLSA_SRV_API_STATE pServerState = (PLSA_SRV_API_STATE)hServer;

    *pUid = pServerState->peerUID;
}

DWORD
LsaSrvAddUserEnumState(
    HANDLE  hServer,
    HANDLE  hEnumServer,
    DWORD   dwUserInfoLevel,
    DWORD   dwMaxNumUsers,
    PLSA_SRV_RECORD_ENUM_STATE* ppEnumState
    )
{
    DWORD dwError = 0;
    uuid_t uuid = {0};
    CHAR  szUUID[37] = "";
    PLSA_SRV_ENUM_STATE pServerState = (PLSA_SRV_ENUM_STATE)hEnumServer;
    PLSA_SRV_RECORD_ENUM_STATE pEnumState = NULL;
    PLSA_SRV_PROVIDER_STATE pProviderStateList = NULL;
    PLSA_SRV_PROVIDER_STATE pProviderState = NULL;
    PLSA_AUTH_PROVIDER pProvider = NULL;

    dwError = LsaAllocateMemory(
                       sizeof(LSA_SRV_RECORD_ENUM_STATE),
                       (PVOID*)&pEnumState);
    BAIL_ON_LSA_ERROR(dwError);

    uuid_generate(uuid);

    uuid_unparse(uuid, szUUID);

    dwError = LsaAllocateString(
                       szUUID,
                       &pEnumState->pszGUID);
    BAIL_ON_LSA_ERROR(dwError);

    pEnumState->dwInfoLevel = dwUserInfoLevel;
    pEnumState->dwNumMaxRecords = dwMaxNumUsers;

    ENTER_AUTH_PROVIDER_LIST_READER_LOCK(pEnumState->bInLock);

    for (pProvider = gpAuthProviderList; pProvider; pProvider = pProvider->pNext)
    {
        dwError = LsaAllocateMemory(
                            sizeof(LSA_SRV_PROVIDER_STATE),
                            (PVOID*)&pProviderState);
        BAIL_ON_LSA_ERROR(dwError);

        pProviderState->pProvider = pProvider;

        dwError = LsaSrvOpenProvider(
                            hServer,
                            pProvider,
                            &pProviderState->hProvider);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = pProvider->pFnTable->pfnBeginEnumUsers(
                                            pProviderState->hProvider,
                                            pEnumState->pszGUID,
                                            pEnumState->dwInfoLevel,
                                            &pProviderState->hResume);
        if (!dwError) {

           pProviderState->pNext = pProviderStateList;
           pProviderStateList = pProviderState;
           pProviderState = NULL;

        } else if (dwError == LSA_ERROR_NOT_HANDLED) {

           if (pProviderState) {
               LsaSrvFreeProviderStateList(pProviderState);
               pProviderState = NULL;
           }

           dwError = 0;

           continue;

        } else {

           BAIL_ON_LSA_ERROR(dwError);

        }
    }

    pEnumState->pProviderStateList =
               LsaSrvReverseProviderStateList(pProviderStateList);
    pProviderStateList = NULL;
    pEnumState->pCurProviderState = pEnumState->pProviderStateList;

    pEnumState->pNext = pServerState->pUserEnumStateList;
    pServerState->pUserEnumStateList = pEnumState;


    *ppEnumState = pEnumState;

cleanup:

    return dwError;

error:

    *ppEnumState = NULL;

    if (pProviderState) {
        LsaSrvFreeProviderStateList(pProviderState);
    }
    if (pProviderStateList) {
        LsaSrvFreeProviderStateList(pProviderStateList);
    }

    if (pEnumState) {
        LsaFreeEnumRecordStateList(pEnumState);
    }

    goto cleanup;
}

static
PLSA_SRV_RECORD_ENUM_STATE
LsaSrvFindEnumState(
    PLSA_SRV_RECORD_ENUM_STATE pStateList,
    PCSTR pszGUID
    )
{
    PLSA_SRV_RECORD_ENUM_STATE pEnumState = NULL;

    while (pStateList) {
        if (!strcasecmp(pszGUID, pStateList->pszGUID)) {
            pEnumState = pStateList;
            break;
        }
    }

    return pEnumState;
}

PLSA_SRV_RECORD_ENUM_STATE
LsaSrvFindUserEnumState(
    HANDLE hServer,
    PCSTR  pszGUID
    )
{
    PLSA_SRV_ENUM_STATE pServerState = (PLSA_SRV_ENUM_STATE)hServer;

    return LsaSrvFindEnumState(pServerState->pUserEnumStateList, pszGUID);
}

VOID
LsaSrvFreeUserEnumState(
    HANDLE hServer,
    PCSTR  pszGUID
    )
{
    PLSA_SRV_ENUM_STATE pServerState = (PLSA_SRV_ENUM_STATE)hServer;
    PLSA_SRV_RECORD_ENUM_STATE pStateList = pServerState->pUserEnumStateList;
    PLSA_SRV_RECORD_ENUM_STATE pEnumState = NULL;
    PLSA_SRV_RECORD_ENUM_STATE pPrevState = NULL;

    while (pStateList) {
        if (!strcasecmp(pszGUID, pStateList->pszGUID)) {
            pEnumState = pStateList;
            break;
        }
        else {
            pPrevState = pStateList;
            pStateList = pStateList->pNext;
        }
    }

    if (pEnumState) {
        if (!pPrevState) {
            pServerState->pUserEnumStateList = pEnumState->pNext;
        } else {
            pPrevState->pNext = pEnumState->pNext;
        }

        pEnumState->pNext = NULL;

        LsaFreeEnumRecordStateList(pEnumState);
    }
}

DWORD
LsaSrvAddGroupEnumState(
    HANDLE hServer,
    HANDLE hEnumServer,
    DWORD   dwGroupInfoLevel,
    DWORD   dwMaxNumGroups,
    BOOLEAN bCheckOnline,
    PLSA_SRV_RECORD_ENUM_STATE* ppEnumState
    )
{
    DWORD dwError = 0;
    uuid_t uuid = {0};
    CHAR  szUUID[37] = "";
    PLSA_SRV_ENUM_STATE pServerState = (PLSA_SRV_ENUM_STATE)hEnumServer;
    PLSA_SRV_RECORD_ENUM_STATE pEnumState = NULL;
    PLSA_SRV_PROVIDER_STATE pProviderStateList = NULL;
    PLSA_SRV_PROVIDER_STATE pProviderState = NULL;
    PLSA_AUTH_PROVIDER pProvider = NULL;

    dwError = LsaAllocateMemory(
                       sizeof(LSA_SRV_RECORD_ENUM_STATE),
                       (PVOID*)&pEnumState);
    BAIL_ON_LSA_ERROR(dwError);

    uuid_generate(uuid);

    uuid_unparse(uuid, szUUID);

    dwError = LsaAllocateString(
                       szUUID,
                       &pEnumState->pszGUID);
    BAIL_ON_LSA_ERROR(dwError);

    pEnumState->dwInfoLevel = dwGroupInfoLevel;
    pEnumState->dwNumMaxRecords = dwMaxNumGroups;
    pEnumState->bCheckOnline = bCheckOnline;

    ENTER_AUTH_PROVIDER_LIST_READER_LOCK(pEnumState->bInLock);

    for (pProvider = gpAuthProviderList; pProvider; pProvider = pProvider->pNext)
    {
        dwError = LsaAllocateMemory(
                            sizeof(LSA_SRV_PROVIDER_STATE),
                            (PVOID*)&pProviderState);
        BAIL_ON_LSA_ERROR(dwError);

        pProviderState->pProvider = pProvider;

        dwError = LsaSrvOpenProvider(
                            hServer,
                            pProvider,
                            &pProviderState->hProvider);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = pProvider->pFnTable->pfnBeginEnumGroups(
                                            pProviderState->hProvider,
                                            pEnumState->pszGUID,
                                            pEnumState->dwInfoLevel,
                                            pEnumState->bCheckOnline,
                                            &pProviderState->hResume);
        if (!dwError) {

           pProviderState->pNext = pProviderStateList;
           pProviderStateList = pProviderState;
           pProviderState = NULL;

        } else if (dwError == LSA_ERROR_NOT_HANDLED) {

           if (pProviderState) {
               LsaSrvFreeProviderStateList(pProviderState);
               pProviderState = NULL;
           }

           dwError = 0;

           continue;

        } else {

           BAIL_ON_LSA_ERROR(dwError);

        }
    }

    pEnumState->pProviderStateList =
               LsaSrvReverseProviderStateList(pProviderStateList);
    pProviderStateList = NULL;
    pEnumState->pCurProviderState = pEnumState->pProviderStateList;

    pEnumState->pNext = pServerState->pGroupEnumStateList;
    pServerState->pGroupEnumStateList = pEnumState;

    *ppEnumState = pEnumState;

cleanup:

    return dwError;

error:

    *ppEnumState = NULL;

    if (pProviderState) {
        LsaSrvFreeProviderStateList(pProviderState);
    }
    if (pProviderStateList) {
        LsaSrvFreeProviderStateList(pProviderStateList);
    }

    if (pEnumState) {
        LsaFreeEnumRecordStateList(pEnumState);
    }

    goto cleanup;
}

PLSA_SRV_RECORD_ENUM_STATE
LsaSrvFindGroupEnumState(
    HANDLE hServer,
    PCSTR  pszGUID
    )
{
    PLSA_SRV_ENUM_STATE pServerState = (PLSA_SRV_ENUM_STATE)hServer;

    return LsaSrvFindEnumState((PLSA_SRV_RECORD_ENUM_STATE)pServerState->pGroupEnumStateList, pszGUID);
}

VOID
LsaSrvFreeGroupEnumState(
    HANDLE hServer,
    PCSTR  pszGUID
    )
{
    PLSA_SRV_ENUM_STATE pServerState = (PLSA_SRV_ENUM_STATE)hServer;
    PLSA_SRV_RECORD_ENUM_STATE pStateList = pServerState->pGroupEnumStateList;
    PLSA_SRV_RECORD_ENUM_STATE pEnumState = NULL;
    PLSA_SRV_RECORD_ENUM_STATE pPrevState = NULL;

    while (pStateList) {
        if (!strcasecmp(pszGUID, pStateList->pszGUID)) {
            pEnumState = pStateList;
            break;
        }
        else {
            pPrevState = pStateList;
            pStateList = pStateList->pNext;
        }
    }

    if (pEnumState) {
        if (!pPrevState) {
            pServerState->pGroupEnumStateList = pEnumState->pNext;
        } else {
            pPrevState->pNext = pEnumState->pNext;
        }

        pEnumState->pNext = NULL;

        LsaFreeEnumRecordStateList(pEnumState);
    }
}

DWORD
LsaSrvAddNSSArtefactEnumState(
    HANDLE  hServer,
    HANDLE  hServerEnum,
    PCSTR   pszMapName,
    LSA_NIS_MAP_QUERY_FLAGS dwFlags,
    DWORD   dwNSSArtefactInfoLevel,
    DWORD   dwMaxNumArtefacts,
    PLSA_SRV_RECORD_ENUM_STATE* ppEnumState
    )
{
    DWORD dwError = 0;
    uuid_t uuid = {0};
    CHAR  szUUID[37] = "";
    PLSA_SRV_ENUM_STATE pServerState = (PLSA_SRV_ENUM_STATE)hServerEnum;
    PLSA_SRV_RECORD_ENUM_STATE pEnumState = NULL;
    PLSA_SRV_PROVIDER_STATE pProviderStateList = NULL;
    PLSA_SRV_PROVIDER_STATE pProviderState = NULL;
    PLSA_AUTH_PROVIDER pProvider = NULL;

    dwError = LsaAllocateMemory(
                       sizeof(LSA_SRV_RECORD_ENUM_STATE),
                       (PVOID*)&pEnumState);
    BAIL_ON_LSA_ERROR(dwError);

    uuid_generate(uuid);

    uuid_unparse(uuid, szUUID);

    dwError = LsaAllocateString(
                       szUUID,
                       &pEnumState->pszGUID);
    BAIL_ON_LSA_ERROR(dwError);

    pEnumState->dwInfoLevel = dwNSSArtefactInfoLevel;
    pEnumState->dwNumMaxRecords = dwMaxNumArtefacts;
    pEnumState->dwMapFlags = dwFlags;

    dwError = LsaAllocateString(
                    pszMapName,
                    &pEnumState->pszMapName);
    BAIL_ON_LSA_ERROR(dwError);

    ENTER_AUTH_PROVIDER_LIST_READER_LOCK(pEnumState->bInLock);

    for (pProvider = gpAuthProviderList; pProvider; pProvider = pProvider->pNext)
    {
        dwError = LsaAllocateMemory(
                            sizeof(LSA_SRV_PROVIDER_STATE),
                            (PVOID*)&pProviderState);
        BAIL_ON_LSA_ERROR(dwError);

        pProviderState->pProvider = pProvider;

        dwError = LsaSrvOpenProvider(
                            hServer,
                            pProvider,
                            &pProviderState->hProvider);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = pProvider->pFnTable->pfnBeginEnumNSSArtefacts(
                                            pProviderState->hProvider,
                                            pEnumState->pszGUID,
                                            pEnumState->dwInfoLevel,
                                            pEnumState->pszMapName,
                                            pEnumState->dwMapFlags,
                                            &pProviderState->hResume);
        if (!dwError) {

           pProviderState->pNext = pProviderStateList;
           pProviderStateList = pProviderState;
           pProviderState = NULL;

        } else if (dwError == LSA_ERROR_NOT_HANDLED) {

           if (pProviderState) {
               LsaSrvFreeProviderStateList(pProviderState);
               pProviderState = NULL;
           }

           dwError = 0;

           continue;

        } else {

           BAIL_ON_LSA_ERROR(dwError);

        }
    }

    pEnumState->pProviderStateList =
               LsaSrvReverseProviderStateList(pProviderStateList);
    pProviderStateList = NULL;
    pEnumState->pCurProviderState = pEnumState->pProviderStateList;

    pEnumState->pNext = pServerState->pNSSArtefactEnumStateList;
    pServerState->pNSSArtefactEnumStateList = pEnumState;

    *ppEnumState = pEnumState;

cleanup:

    return dwError;

error:

    *ppEnumState = NULL;

    if (pProviderState) {
        LsaSrvFreeProviderStateList(pProviderState);
    }
    if (pProviderStateList) {
        LsaSrvFreeProviderStateList(pProviderStateList);
    }

    if (pEnumState) {
        LsaFreeEnumRecordStateList(pEnumState);
    }

    goto cleanup;
}

PLSA_SRV_RECORD_ENUM_STATE
LsaSrvFindNSSArtefactEnumState(
    HANDLE hServer,
    PCSTR  pszGUID
    )
{
    PLSA_SRV_ENUM_STATE pServerState = (PLSA_SRV_ENUM_STATE)hServer;

    return LsaSrvFindEnumState(pServerState->pNSSArtefactEnumStateList, pszGUID);
}

VOID
LsaSrvFreeNSSArtefactEnumState(
    HANDLE hServer,
    PCSTR  pszGUID
    )
{
    PLSA_SRV_ENUM_STATE pServerState = (PLSA_SRV_ENUM_STATE)hServer;
    PLSA_SRV_RECORD_ENUM_STATE pStateList = pServerState->pNSSArtefactEnumStateList;
    PLSA_SRV_RECORD_ENUM_STATE pEnumState = NULL;
    PLSA_SRV_RECORD_ENUM_STATE pPrevState = NULL;

    while (pStateList) {
        if (!strcasecmp(pszGUID, pStateList->pszGUID)) {
            pEnumState = pStateList;
            break;
        }
        else {
            pPrevState = pStateList;
            pStateList = pStateList->pNext;
        }
    }

    if (pEnumState) {
        if (!pPrevState) {
            pServerState->pNSSArtefactEnumStateList = pEnumState->pNext;
        } else {
            pPrevState->pNext = pEnumState->pNext;
        }

        pEnumState->pNext = NULL;

        LsaFreeEnumRecordStateList(pEnumState);
    }
}

VOID
LsaSrvFreeProviderStateList(
    PLSA_SRV_PROVIDER_STATE pStateList
    )
{
    while (pStateList) {
        PLSA_SRV_PROVIDER_STATE pState = pStateList;
        pStateList = pStateList->pNext;

        if (pState->pProvider && (pState->hProvider != (HANDLE)NULL)) {
            pState->pProvider->pFnTable->pfnCloseHandle(pState->hProvider);
        }
        LsaFreeMemory(pState);
    }
}

PLSA_SRV_PROVIDER_STATE
LsaSrvReverseProviderStateList(
    PLSA_SRV_PROVIDER_STATE pStateList
    )
{
    PLSA_SRV_PROVIDER_STATE pP = NULL;
    PLSA_SRV_PROVIDER_STATE pQ  = pStateList;
    PLSA_SRV_PROVIDER_STATE pR = NULL;

    while (pQ) {
        pR = pQ->pNext;
        pQ->pNext = pP;
        pP = pQ;
        pQ = pR;
    }

    return pP;
}

