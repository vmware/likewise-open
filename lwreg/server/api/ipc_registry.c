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
 *        ipc_registry.c
 *
 * Abstract:
 *
 *        Registry
 *
 *        Inter-process communication (Server) API for Users
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
 */
#include "api.h"

static
DWORD
RegSrvIpcCheckPermissions(
    LWMsgSecurityToken* token,
    uid_t* puid,
    gid_t* pgid
    )
{
    DWORD dwError = 0;
    uid_t euid;
    gid_t egid;

    if (strcmp(lwmsg_security_token_get_type(token), "local"))
    {
        REG_LOG_WARNING("Unsupported authentication type");
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_REG_ERROR(dwError);
    }

    dwError = MAP_LWMSG_ERROR(lwmsg_local_token_get_eid(token, &euid, &egid));
    BAIL_ON_REG_ERROR(dwError);

    REG_LOG_VERBOSE("Permission granted for (uid = %i, gid = %i) to open RegIpcServer",
                    (int) euid,
                    (int) egid);

    *puid = euid;
    *pgid = egid;

error:
    return dwError;
}

static
DWORD
RegSrvIpcRegisterHandle(
    LWMsgCall* pCall,
    PCSTR pszHandleType,
    PVOID pHandle,
    LWMsgHandleCleanupFunction pfnCleanup
    )
{
    DWORD dwError = 0;
    LWMsgSession* pSession = lwmsg_call_get_session(pCall);

    dwError = MAP_LWMSG_ERROR(lwmsg_session_register_handle(pSession, pszHandleType, pHandle, pfnCleanup));
    BAIL_ON_REG_ERROR(dwError);

error:

    return dwError;
}

static
DWORD
RegSrvIpcRetainHandle(
    LWMsgCall* pCall,
    PVOID pHandle
    )
{
    DWORD dwError = 0;
    LWMsgSession* pSession = lwmsg_call_get_session(pCall);

    dwError = MAP_LWMSG_ERROR(lwmsg_session_retain_handle(pSession, pHandle));
    BAIL_ON_REG_ERROR(dwError);

error:

    return dwError;
}

static
VOID
RegSrvIpcCloseHandle(
    PVOID pHandle
    )
{
    return RegSrvCloseKey((HKEY)pHandle);
}

static
DWORD
RegSrvIpcUnregisterHandle(
    LWMsgCall* pCall,
    PVOID pHandle
    )
{
    DWORD dwError = 0;
    LWMsgSession* pSession = lwmsg_call_get_session(pCall);

    dwError = MAP_LWMSG_ERROR(lwmsg_session_unregister_handle(pSession, pHandle));
    BAIL_ON_REG_ERROR(dwError);

error:

    return dwError;
}


static
HANDLE
RegSrvIpcGetSessionData(
    LWMsgCall* pCall
    )
{
    LWMsgSession* pSession = lwmsg_call_get_session(pCall);

    return lwmsg_session_get_data(pSession);
}

void
RegSrvIpcDestructSession(
    LWMsgSecurityToken* pToken,
    void* pSessionData
    )
{
    RegSrvCloseServer(pSessionData);
}

LWMsgStatus
RegSrvIpcConstructSession(
    LWMsgSecurityToken* pToken,
    void* pData,
    void** ppSessionData
    )
{
    DWORD dwError = 0;
    HANDLE Handle = (HANDLE)NULL;
    uid_t UID;
    gid_t GID;

    dwError = RegSrvIpcCheckPermissions(pToken, &UID, &GID);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegSrvOpenServer(UID, GID, &Handle);
    BAIL_ON_REG_ERROR(dwError);

    *ppSessionData = Handle;

cleanup:

    return MAP_REG_ERROR_IPC(dwError);

error:

    goto cleanup;
}

DWORD
RegSrvOpenServer(
    uid_t peerUID,
    gid_t peerGID,
    PHANDLE phServer
    )
{
    DWORD dwError = 0;
    PREG_SRV_API_STATE pServerState = NULL;

    dwError = LwAllocateMemory(
                    sizeof(*pServerState),
                    (PVOID*)&pServerState);
    BAIL_ON_REG_ERROR(dwError);

    pServerState->peerUID = peerUID;
    pServerState->peerGID = peerGID;

    *phServer = (HANDLE)pServerState;

cleanup:

    return dwError;

error:

    *phServer = (HANDLE)NULL;

    if (pServerState) {
        RegSrvCloseServer((HANDLE)pServerState);
    }

    goto cleanup;
}

void
RegSrvCloseServer(
    HANDLE hServer
    )
{
    PREG_SRV_API_STATE pServerState = (PREG_SRV_API_STATE)hServer;

    if (pServerState->hEventLog != (HANDLE)NULL)
    {
       //RegSrvCloseEventLog(pServerState->hEventLog);
    }

    LwFreeMemory(pServerState);
}

DWORD
RegSrvIpcCreateError(
    DWORD dwErrorCode,
    PREG_IPC_ERROR* ppError
    )
{
    DWORD dwError = 0;
    PREG_IPC_ERROR pError = NULL;

    dwError = LwAllocateMemory(sizeof(*pError), (void**) (void*) &pError);
    BAIL_ON_REG_ERROR(dwError);

    pError->dwError = dwErrorCode;

    *ppError = pError;

error:
    return dwError;
}

LWMsgStatus
RegSrvIpcEnumRootKeysW(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PREG_IPC_ENUM_ROOTKEYS_RESPONSE pRegResp = NULL;
    PREG_IPC_ERROR pError = NULL;
    PWSTR* ppwszRootKeyNames = NULL;
    DWORD dwNumRootKeys = 0;
    int iCount = 0;

    dwError = RegSrvEnumRootKeysW(
        RegSrvIpcGetSessionData(pCall),
        &ppwszRootKeyNames,
        &dwNumRootKeys
        );

    if (!dwError)
    {
        dwError = LwAllocateMemory(
            sizeof(*pRegResp),
            OUT_PPVOID(&pRegResp));
        BAIL_ON_REG_ERROR(dwError);

        pRegResp->ppwszRootKeyNames = ppwszRootKeyNames;
        ppwszRootKeyNames = NULL;
        pRegResp->dwNumRootKeys = dwNumRootKeys;

        pOut->tag = REG_R_ENUM_ROOT_KEYSW;
        pOut->data = pRegResp;
    }
    else
    {
        dwError = RegSrvIpcCreateError(dwError, &pError);
        BAIL_ON_REG_ERROR(dwError);

        pOut->tag = REG_R_ERROR;
        pOut->data = pError;
    }

cleanup:
    if (ppwszRootKeyNames)
    {
        for (iCount=0; iCount<dwNumRootKeys; iCount++)
        {
            LW_SAFE_FREE_MEMORY(ppwszRootKeyNames[iCount]);
        }
        ppwszRootKeyNames = NULL;
    }

    return MAP_REG_ERROR_IPC(dwError);

error:
    goto cleanup;
}

LWMsgStatus
RegSrvIpcCreateKeyEx(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PREG_IPC_CREATE_KEY_EX_REQ pReq = pIn->data;
    PREG_IPC_CREATE_KEY_EX_RESPONSE pRegResp = NULL;
    PREG_IPC_ERROR pError = NULL;
    HKEY hkResult = NULL;
    DWORD dwDisposition = 0;

    dwError = RegSrvCreateKeyEx(
        RegSrvIpcGetSessionData(pCall),
        pReq->hKey,
        pReq->pSubKey,
        0,
        pReq->pClass,
        pReq->dwOptions,
        pReq->samDesired,
        pReq->pSecurityAttributes,
        &hkResult,
        &dwDisposition
        );

    if (!dwError)
    {
        dwError = LwAllocateMemory(
            sizeof(*pRegResp),
            OUT_PPVOID(&pRegResp));
        BAIL_ON_REG_ERROR(dwError);

        pRegResp->dwDisposition= dwDisposition;
        pRegResp->hkResult = hkResult;
        hkResult = NULL;

        dwError = RegSrvIpcRegisterHandle(
                                      pCall,
                                      "HKEY",
                                      (PVOID)pRegResp->hkResult,
                                      RegSrvIpcCloseHandle);
        BAIL_ON_REG_ERROR(dwError);

        pOut->tag = REG_R_CREATE_KEY_EX;
        pOut->data = pRegResp;

        dwError = RegSrvIpcRetainHandle(pCall, pRegResp->hkResult);
        BAIL_ON_REG_ERROR(dwError);
    }
    else
    {
        dwError = RegSrvIpcCreateError(dwError, &pError);
        BAIL_ON_REG_ERROR(dwError);

        pOut->tag = REG_R_ERROR;
        pOut->data = pError;
    }

cleanup:
    RegSrvIpcCloseHandle((PVOID)hkResult);

    return MAP_REG_ERROR_IPC(dwError);

error:
    goto cleanup;
}

LWMsgStatus
RegSrvIpcOpenKeyExW(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PREG_IPC_OPEN_KEY_EX_REQ pReq = pIn->data;
    PREG_IPC_OPEN_KEY_EX_RESPONSE pRegResp = NULL;
    PREG_IPC_ERROR pError = NULL;
    HKEY hkResult = NULL;

    dwError = RegSrvOpenKeyExW(
        RegSrvIpcGetSessionData(pCall),
        pReq->hKey,
        pReq->pSubKey,
        0,
        pReq->samDesired,
        &hkResult
        );

    if (!dwError)
    {
        dwError = LwAllocateMemory(sizeof(*pRegResp), OUT_PPVOID(&pRegResp));
        BAIL_ON_REG_ERROR(dwError);

        pRegResp->hkResult = hkResult;
        hkResult = NULL;

        dwError = RegSrvIpcRegisterHandle(
                                      pCall,
                                      "HKEY",
                                      (PVOID)pRegResp->hkResult,
                                      RegSrvIpcCloseHandle);
        BAIL_ON_REG_ERROR(dwError);

        pOut->tag = REG_R_OPEN_KEYW_EX;
        pOut->data = pRegResp;

        dwError = RegSrvIpcRetainHandle(pCall, pRegResp->hkResult);
        BAIL_ON_REG_ERROR(dwError);
    }
    else
    {
        dwError = RegSrvIpcCreateError(dwError, &pError);
        BAIL_ON_REG_ERROR(dwError);

        pOut->tag = REG_R_ERROR;
        pOut->data = pError;
    }

cleanup:
    RegSrvIpcCloseHandle((PVOID)hkResult);

    return MAP_REG_ERROR_IPC(dwError);

error:
    goto cleanup;
}

LWMsgStatus
RegSrvIpcCloseKey(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PREG_IPC_CLOSE_KEY_REQ pReq = pIn->data;
    PREG_IPC_ERROR pError = NULL;

    dwError = RegSrvIpcUnregisterHandle(pCall, pReq->hKey);
    if (!dwError)
    {
        pOut->tag = REG_R_CLOSE_KEY;
        pOut->data = NULL;
    }
    else
    {
        dwError = RegSrvIpcCreateError(dwError, &pError);
        BAIL_ON_REG_ERROR(dwError);

        pOut->tag = REG_R_ERROR;
        pOut->data = pError;
    }

cleanup:
    return MAP_REG_ERROR_IPC(dwError);

error:
    goto cleanup;
}

LWMsgStatus
RegSrvIpcDeleteKey(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PREG_IPC_DELETE_KEY_REQ pReq = pIn->data;
    PREG_IPC_ERROR pError = NULL;

    dwError = RegSrvDeleteKey(
        RegSrvIpcGetSessionData(pCall),
        pReq->hKey,
        pReq->pSubKey
        );

    if (!dwError)
    {
        pOut->tag = REG_R_DELETE_KEY;
        pOut->data = NULL;
    }
    else
    {
        dwError = RegSrvIpcCreateError(dwError, &pError);
        BAIL_ON_REG_ERROR(dwError);

        pOut->tag = REG_R_ERROR;
        pOut->data = pError;
    }

cleanup:
    return MAP_REG_ERROR_IPC(dwError);

error:
    goto cleanup;
}

LWMsgStatus
RegSrvIpcEnumKeyExW(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PREG_IPC_ENUM_KEY_EX_REQ pReq = pIn->data;
    PREG_IPC_ENUM_KEY_EX_RESPONSE pRegResp = NULL;
    PREG_IPC_ERROR pError = NULL;

    dwError = RegSrvEnumKeyExW(
        RegSrvIpcGetSessionData(pCall),
        pReq->hKey,
        pReq->dwIndex,
        pReq->pName,
        &pReq->cName,
        NULL,
        pReq->pClass,
        pReq->pcClass,
        NULL
        );
    if (!dwError)
    {
        dwError = LwAllocateMemory(
            sizeof(*pRegResp),
            OUT_PPVOID(&pRegResp));
        BAIL_ON_REG_ERROR(dwError);

        pRegResp->pName= pReq->pName;
        pRegResp->cName = pReq->cName;

        pOut->tag = REG_R_ENUM_KEYW_EX;
        pOut->data = pRegResp;
    }
    else
    {
        dwError = RegSrvIpcCreateError(dwError, &pError);
        BAIL_ON_REG_ERROR(dwError);

        pOut->tag = REG_R_ERROR;
        pOut->data = pError;
    }

    pReq->pName = NULL;

cleanup:
    return MAP_REG_ERROR_IPC(dwError);

error:
    goto cleanup;
}

LWMsgStatus
RegSrvIpcQueryInfoKeyA(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PREG_IPC_QUERY_INFO_KEY_REQ pReq = pIn->data;
    PREG_IPC_QUERY_INFO_KEY_RESPONSE pRegResp = NULL;
    PREG_IPC_ERROR pError = NULL;
    DWORD dwSubKeyCount = 0;
    DWORD dwMaxKeyLength = 0;
    DWORD dwValueCount = 0;
    DWORD dwMaxValueNameLen = 0;
    DWORD dwMaxValueLen = 0;

    dwError = RegSrvQueryInfoKeyA(
        RegSrvIpcGetSessionData(pCall),
        pReq->hKey,
        NULL,
        pReq->pcClass,
        NULL,
        &dwSubKeyCount,
        &dwMaxKeyLength,
        NULL,
        &dwValueCount,
        &dwMaxValueNameLen,
        &dwMaxValueLen,
        NULL,
        NULL
        );
    if (!dwError)
    {
        dwError = LwAllocateMemory(sizeof(*pRegResp), OUT_PPVOID(&pRegResp));
        BAIL_ON_REG_ERROR(dwError);

        pRegResp->cSubKeys = dwSubKeyCount;
        pRegResp->cMaxSubKeyLen = dwMaxKeyLength;
        pRegResp->cValues = dwValueCount;
        pRegResp->cMaxValueNameLen = dwMaxValueNameLen;
        pRegResp->cMaxValueLen = dwMaxValueLen;

        pOut->tag = REG_R_QUERY_INFO_KEYA;
        pOut->data = pRegResp;
    }
    else
    {
        dwError = RegSrvIpcCreateError(dwError, &pError);
        BAIL_ON_REG_ERROR(dwError);

        pOut->tag = REG_R_ERROR;
        pOut->data = pError;
    }

cleanup:
    return MAP_REG_ERROR_IPC(dwError);

error:
    goto cleanup;
}

LWMsgStatus
RegSrvIpcQueryInfoKeyW(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PREG_IPC_QUERY_INFO_KEY_REQ pReq = pIn->data;
    PREG_IPC_QUERY_INFO_KEY_RESPONSE pRegResp = NULL;
    PREG_IPC_ERROR pError = NULL;
    DWORD dwSubKeyCount = 0;
    DWORD dwMaxKeyLength = 0;
    DWORD dwValueCount = 0;
    DWORD dwMaxValueNameLen = 0;
    DWORD dwMaxValueLen = 0;

    dwError = RegSrvQueryInfoKeyW(
        RegSrvIpcGetSessionData(pCall),
        pReq->hKey,
        NULL,
        pReq->pcClass,
        NULL,
        &dwSubKeyCount,
        &dwMaxKeyLength,
        NULL,
        &dwValueCount,
        &dwMaxValueNameLen,
        &dwMaxValueLen,
        NULL,
        NULL
        );
    if (!dwError)
    {
        dwError = LwAllocateMemory(sizeof(*pRegResp), OUT_PPVOID(&pRegResp));
        BAIL_ON_REG_ERROR(dwError);

        pRegResp->cSubKeys = dwSubKeyCount;
        pRegResp->cMaxSubKeyLen = dwMaxKeyLength;
        pRegResp->cValues = dwValueCount;
        pRegResp->cMaxValueNameLen = dwMaxValueNameLen;
        pRegResp->cMaxValueLen = dwMaxValueLen;

        pOut->tag = REG_R_QUERY_INFO_KEYW;
        pOut->data = pRegResp;
    }
    else
    {
        dwError = RegSrvIpcCreateError(dwError, &pError);
        BAIL_ON_REG_ERROR(dwError);

        pOut->tag = REG_R_ERROR;
        pOut->data = pError;
    }

cleanup:
    return MAP_REG_ERROR_IPC(dwError);

error:
    goto cleanup;
}

LWMsgStatus
RegSrvIpcGetValueW(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PREG_IPC_GET_VALUE_REQ pReq = pIn->data;
    PREG_IPC_GET_VALUE_RESPONSE pRegResp = NULL;
    PREG_IPC_ERROR pError = NULL;
    DWORD dwType = 0;

    dwError = RegSrvGetValueW(
        RegSrvIpcGetSessionData(pCall),
        pReq->hKey,
        pReq->pSubKey,
        pReq->pValue,
        pReq->Flags,
        &dwType,
        pReq->pData,
        &pReq->cbData
        );

    if (!dwError)
    {
        dwError = LwAllocateMemory(
            sizeof(*pRegResp),
            OUT_PPVOID(&pRegResp));
        BAIL_ON_REG_ERROR(dwError);

        pRegResp->cbData = pReq->cbData;
        pRegResp->pvData = pReq->pData;
        pRegResp->dwType = dwType;

        pOut->tag = REG_R_GET_VALUEW;
        pOut->data = pRegResp;
    }
    else
    {
        dwError = RegSrvIpcCreateError(dwError, &pError);
        BAIL_ON_REG_ERROR(dwError);

        pOut->tag = REG_R_ERROR;
        pOut->data = pError;
    }
    pReq->pData = NULL;

cleanup:
    return MAP_REG_ERROR_IPC(dwError);

error:
    goto cleanup;
}

LWMsgStatus
RegSrvIpcDeleteKeyValue(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PREG_IPC_DELETE_KEY_VALUE_REQ pReq = pIn->data;
    PREG_IPC_ERROR pError = NULL;

    dwError = RegSrvDeleteKeyValue(
        RegSrvIpcGetSessionData(pCall),
        pReq->hKey,
        pReq->pSubKey,
        pReq->pValueName
        );

    if (!dwError)
    {
        pOut->tag = REG_R_DELETE_KEY_VALUE;
        pOut->data = NULL;
    }
    else
    {
        dwError = RegSrvIpcCreateError(dwError, &pError);
        BAIL_ON_REG_ERROR(dwError);

        pOut->tag = REG_R_ERROR;
        pOut->data = pError;
    }

cleanup:
    return MAP_REG_ERROR_IPC(dwError);

error:
    goto cleanup;
}

LWMsgStatus
RegSrvIpcDeleteTree(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PREG_IPC_DELETE_TREE_REQ pReq = pIn->data;
    PREG_IPC_ERROR pError = NULL;

    dwError = RegSrvDeleteTree(
        RegSrvIpcGetSessionData(pCall),
        pReq->hKey,
        pReq->pSubKey
        );
    if (!dwError)
    {
        pOut->tag = REG_R_DELETE_TREE;
        pOut->data = NULL;
    }
    else
    {
        dwError = RegSrvIpcCreateError(dwError, &pError);
        BAIL_ON_REG_ERROR(dwError);

        pOut->tag = REG_R_ERROR;
        pOut->data = pError;
    }

cleanup:
    return MAP_REG_ERROR_IPC(dwError);

error:
    goto cleanup;
}


LWMsgStatus
RegSrvIpcDeleteValue(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PREG_IPC_DELETE_VALUE_REQ pReq = pIn->data;
    PREG_IPC_ERROR pError = NULL;

    dwError = RegSrvDeleteValue(
        RegSrvIpcGetSessionData(pCall),
        pReq->hKey,
        pReq->pValueName
        );

    if (!dwError)
    {
        pOut->tag = REG_R_DELETE_VALUE;
        pOut->data = NULL;
    }
    else
    {
        dwError = RegSrvIpcCreateError(dwError, &pError);
        BAIL_ON_REG_ERROR(dwError);

        pOut->tag = REG_R_ERROR;
        pOut->data = pError;
    }

cleanup:
    return MAP_REG_ERROR_IPC(dwError);

error:
    goto cleanup;
}

LWMsgStatus
RegSrvIpcEnumValueW(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PREG_IPC_ENUM_VALUE_REQ pReq = pIn->data;
    PREG_IPC_ENUM_VALUE_RESPONSE pRegResp = NULL;
    PREG_IPC_ERROR pError = NULL;
    REG_DATA_TYPE type = REG_UNKNOWN;

    dwError = RegSrvEnumValueW(
        RegSrvIpcGetSessionData(pCall),
        pReq->hKey,
        pReq->dwIndex,
        pReq->pName,
        &pReq->cName,
        NULL,
        &type,
        pReq->pValue,
        &pReq->cValue);

    if (!dwError)
    {
        dwError = LwAllocateMemory(
            sizeof(*pRegResp),
            OUT_PPVOID(&pRegResp));
        BAIL_ON_REG_ERROR(dwError);

        pRegResp->pName= pReq->pName;
        pRegResp->cName = pReq->cName;
        pRegResp->pValue = pReq->pValue;
        pRegResp->cValue = pReq->cValue;
        pRegResp->type = type;

        pOut->tag = REG_R_ENUM_VALUEW;
        pOut->data = pRegResp;
    }
    else
    {
        dwError = RegSrvIpcCreateError(dwError, &pError);
        BAIL_ON_REG_ERROR(dwError);

        pOut->tag = REG_R_ERROR;
        pOut->data = pError;
    }

    pReq->pName = NULL;
    pReq->pValue = NULL;

cleanup:
    return MAP_REG_ERROR_IPC(dwError);

error:
    goto cleanup;
}

LWMsgStatus
RegSrvIpcQueryMultipleValues(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PREG_IPC_QUERY_MULTIPLE_VALUES_REQ pReq = pIn->data;
    PREG_IPC_QUERY_MULTIPLE_VALUES_RESPONSE pRegResp = NULL;
    PREG_IPC_ERROR pError = NULL;

    dwError = RegSrvQueryMultipleValues(
        RegSrvIpcGetSessionData(pCall),
        pReq->hKey,
        pReq->val_list,
        pReq->num_vals,
        pReq->pValue,
        &pReq->dwTotalsize
        );

    if (!dwError)
    {
        dwError = LwAllocateMemory(sizeof(*pRegResp), OUT_PPVOID(&pRegResp));
        BAIL_ON_REG_ERROR(dwError);

        pRegResp->dwTotalsize = pReq->dwTotalsize;
        pRegResp->num_vals = pReq->num_vals;
        pRegResp->val_list = pReq->val_list;
        pRegResp->pValue = pReq->pValue;

        pReq->val_list = NULL;
        pReq->pValue = NULL;

        pOut->tag = REG_R_QUERY_MULTIPLE_VALUES;
        pOut->data = pRegResp;
    }
    else
    {
        dwError = RegSrvIpcCreateError(dwError, &pError);
        BAIL_ON_REG_ERROR(dwError);

        pOut->tag = REG_R_ERROR;
        pOut->data = pError;
    }

cleanup:

    return MAP_REG_ERROR_IPC(dwError);

error:
    goto cleanup;
}

LWMsgStatus
RegSrvIpcSetValueExW(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    void* data
    )
{
    DWORD dwError = 0;
    PREG_IPC_SET_VALUE_EX_REQ pReq = pIn->data;
    PREG_IPC_ERROR pError = NULL;

    dwError = RegSrvSetValueExW(
        RegSrvIpcGetSessionData(pCall),
        pReq->hKey,
        pReq->pValueName,
        0,
        pReq->dwType,
        pReq->pData,
        pReq->cbData
        );

    if (!dwError)
    {
        pOut->tag = REG_R_SET_VALUEW_EX;
        pOut->data = NULL;
    }
    else
    {
        dwError = RegSrvIpcCreateError(dwError, &pError);
        BAIL_ON_REG_ERROR(dwError);

        pOut->tag = REG_R_ERROR;
        pOut->data = pError;
    }

cleanup:
    return MAP_REG_ERROR_IPC(dwError);

error:
    goto cleanup;
}
