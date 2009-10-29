/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        clientipc.c
 *
 * Abstract:
 *
 *        Registry Subsystem
 *
 *        Inter-process Communication (Client) API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Wei Fu (wfu@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
 */
#include "client.h"

static
DWORD
RegIpcUnregisterHandle(
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

DWORD
RegOpenServer(
    PHANDLE phConnection
    )
{
    DWORD dwError = 0;
    PREG_CLIENT_CONNECTION_CONTEXT pContext = NULL;
    static LWMsgTime connectTimeout = {2, 0};

    BAIL_ON_INVALID_POINTER(phConnection);

    dwError = LwAllocateMemory(sizeof(REG_CLIENT_CONNECTION_CONTEXT),
                              (PVOID*)&pContext);
    BAIL_ON_REG_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_protocol_new(NULL, &pContext->pProtocol));
    BAIL_ON_REG_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_protocol_add_protocol_spec(pContext->pProtocol, RegIPCGetProtocolSpec()));
    BAIL_ON_REG_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_connection_new(NULL, pContext->pProtocol, &pContext->pAssoc));
    BAIL_ON_REG_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_connection_set_endpoint(
                                  pContext->pAssoc,
                                  LWMSG_CONNECTION_MODE_LOCAL,
                                  CACHEDIR "/" REG_SERVER_FILENAME));
    BAIL_ON_REG_ERROR(dwError);

    if (getenv("LW_DISABLE_CONNECT_TIMEOUT") == NULL)
    {
        /* Give up connecting within 2 seconds in case lsassd
           is unresponsive (e.g. it's being traced in a debugger) */
        dwError = MAP_LWMSG_ERROR(lwmsg_assoc_set_timeout(
                                      pContext->pAssoc,
                                      LWMSG_TIMEOUT_ESTABLISH,
                                      &connectTimeout));
        BAIL_ON_REG_ERROR(dwError);
    }

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_establish(pContext->pAssoc));
    BAIL_ON_REG_ERROR(dwError);

    *phConnection = (HANDLE)pContext;

cleanup:
    return dwError;

error:
    if (pContext)
    {
        if (pContext->pAssoc)
        {
            lwmsg_assoc_delete(pContext->pAssoc);
        }

        if (pContext->pProtocol)
        {
            lwmsg_protocol_delete(pContext->pProtocol);
        }

        LwFreeMemory(pContext);
    }

    if (phConnection)
    {
        *phConnection = (HANDLE)NULL;
    }

    goto cleanup;
}

VOID
RegCloseServer(
    HANDLE hConnection
    )
{
    PREG_CLIENT_CONNECTION_CONTEXT pContext =
                     (PREG_CLIENT_CONNECTION_CONTEXT)hConnection;

    if (!pContext)
        return;

    if (pContext->pAssoc)
    {
        lwmsg_assoc_close(pContext->pAssoc);
        lwmsg_assoc_delete(pContext->pAssoc);
    }

    if (pContext->pProtocol)
    {
        lwmsg_protocol_delete(pContext->pProtocol);
    }

    LwFreeMemory(pContext);
}

DWORD
RegIpcAcquireCall(
    HANDLE hConnection,
    LWMsgCall** ppCall
    )
{
    DWORD dwError = 0;
    PREG_CLIENT_CONNECTION_CONTEXT pContext = hConnection;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_acquire_call(pContext->pAssoc, ppCall));
    BAIL_ON_REG_ERROR(dwError);

error:

    return dwError;
}

DWORD
RegTransactEnumRootKeysW(
    IN HANDLE hConnection,
    OUT PWSTR** pppwszRootKeyNames,
    OUT PDWORD pdwNumRootKey
    )
{
    DWORD dwError = 0;
    // Do not free pError
    PREG_IPC_ERROR pError = NULL;
    PREG_IPC_ENUM_ROOTKEYS_RESPONSE pEnumRootKeysResp = NULL;

    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    dwError = RegIpcAcquireCall(hConnection, &pCall);
    BAIL_ON_REG_ERROR(dwError);

    in.tag = REG_Q_ENUM_ROOT_KEYSW;
    in.data = NULL;

    dwError = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_REG_ERROR(dwError);

    switch (out.tag)
    {
        case REG_R_ENUM_ROOT_KEYSW_SUCCESS:
            pEnumRootKeysResp = (PREG_IPC_ENUM_ROOTKEYS_RESPONSE)out.data;
            *pppwszRootKeyNames = pEnumRootKeysResp->ppwszRootKeyNames;
            pEnumRootKeysResp->ppwszRootKeyNames = NULL;
            *pdwNumRootKey = pEnumRootKeysResp->dwNumRootKeys;
            pEnumRootKeysResp->dwNumRootKeys = 0;

            break;
        case REG_R_ENUM_ROOT_KEYSW_FAILURE:
            pError = (PREG_IPC_ERROR) out.data;
            dwError = pError->dwError;
            BAIL_ON_REG_ERROR(dwError);
            break;
        default:
            dwError = EINVAL;
            BAIL_ON_REG_ERROR(dwError);
    }

cleanup:
    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:
    goto cleanup;
}

DWORD
RegTransactCreateKeyExW(
    IN HANDLE hConnection,
    IN HKEY hKey,
    IN PCWSTR pSubKey,
    IN DWORD Reserved,
    IN OPTIONAL PWSTR pClass,
    IN DWORD dwOptions,
    IN REGSAM samDesired,
    IN OPTIONAL PSECURITY_ATTRIBUTES pSecurityAttributes,
    OUT PHKEY phkResult,
    OUT OPTIONAL PDWORD pdwDisposition
    )
{
    DWORD dwError = 0;
    REG_IPC_CREATE_KEY_EX_REQ CreateKeyExReq;
    PREG_IPC_CREATE_KEY_EX_RESPONSE pCreateKeyExResp = NULL;
    // Do not free pError
    PREG_IPC_ERROR pError = NULL;

    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    dwError = RegIpcAcquireCall(hConnection, &pCall);
    BAIL_ON_REG_ERROR(dwError);

    CreateKeyExReq.hKey = hKey;
    CreateKeyExReq.pSubKey = pSubKey;
    CreateKeyExReq.pClass = pClass;
    CreateKeyExReq.dwOptions = dwOptions;
    CreateKeyExReq.samDesired = samDesired;
    CreateKeyExReq.pSecurityAttributes = pSecurityAttributes;

    in.tag = REG_Q_CREATE_KEY_EX;
    in.data = &CreateKeyExReq;

    dwError = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_REG_ERROR(dwError);

    switch (out.tag)
    {
        case REG_R_CREATE_KEY_EX_SUCCESS:
            pCreateKeyExResp = (PREG_IPC_CREATE_KEY_EX_RESPONSE)out.data;
            *phkResult = pCreateKeyExResp->hkResult;
            pCreateKeyExResp->hkResult = NULL;

            if(pdwDisposition)
            {
                *pdwDisposition = pCreateKeyExResp->dwDisposition;
            }

            break;
        case REG_R_CREATE_KEY_EX_FAILURE:
            pError = (PREG_IPC_ERROR) out.data;
            dwError = pError->dwError;
            BAIL_ON_REG_ERROR(dwError);
            break;
        default:
            dwError = EINVAL;
            BAIL_ON_REG_ERROR(dwError);
    }

cleanup:
    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:
    goto cleanup;
}

DWORD
RegTransactOpenKeyExA(
    IN HANDLE hConnection,
    IN HKEY hKey,
    IN OPTIONAL PCSTR pszSubKey,
    IN DWORD ulOptions,
    IN REGSAM samDesired,
    OUT PHKEY phkResult
    )
{
    DWORD dwError = 0;
    REG_IPC_OPEN_KEYA_EX_REQ OpenKeyExReq;
    // Do not free pError
    PREG_IPC_ERROR pError = NULL;
    PREG_IPC_OPEN_KEY_EX_RESPONSE pOpenKeyExResp = NULL;

    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    dwError = RegIpcAcquireCall(hConnection, &pCall);
    BAIL_ON_REG_ERROR(dwError);

    OpenKeyExReq.hKey = hKey;
    OpenKeyExReq.pszSubKey = pszSubKey;
    OpenKeyExReq.samDesired = samDesired;

    in.tag = REG_Q_OPEN_KEYA_EX;
    in.data = &OpenKeyExReq;

    dwError = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_REG_ERROR(dwError);

    switch (out.tag)
    {
        case REG_R_OPEN_KEYA_EX_SUCCESS:
            pOpenKeyExResp = (PREG_IPC_OPEN_KEY_EX_RESPONSE) out.data;

            *phkResult = pOpenKeyExResp->hkResult;
            pOpenKeyExResp->hkResult = NULL;

            break;
        case REG_R_OPEN_KEYA_EX_FAILURE:
            pError = (PREG_IPC_ERROR) out.data;
            dwError = pError->dwError;
            BAIL_ON_REG_ERROR(dwError);
            break;
        default:
            dwError = EINVAL;
            BAIL_ON_REG_ERROR(dwError);
    }

cleanup:
    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:
    goto cleanup;
}

DWORD
RegTransactOpenKeyExW(
    IN HANDLE hConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pwszSubKey,
    IN DWORD ulOptions,
    IN REGSAM samDesired,
    OUT PHKEY phkResult
    )
{
    DWORD dwError = 0;
    REG_IPC_OPEN_KEY_EX_REQ OpenKeyExReq;
    // Do not free pError
    PREG_IPC_ERROR pError = NULL;
    PREG_IPC_OPEN_KEY_EX_RESPONSE pOpenKeyExResp = NULL;

    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    dwError = RegIpcAcquireCall(hConnection, &pCall);
    BAIL_ON_REG_ERROR(dwError);

    OpenKeyExReq.hKey = hKey;
    OpenKeyExReq.pSubKey = pwszSubKey;
    OpenKeyExReq.samDesired = samDesired;

    in.tag = REG_Q_OPEN_KEYW_EX;
    in.data = &OpenKeyExReq;

    dwError = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_REG_ERROR(dwError);

    switch (out.tag)
    {
        case REG_R_OPEN_KEYW_EX_SUCCESS:
            pOpenKeyExResp = (PREG_IPC_OPEN_KEY_EX_RESPONSE) out.data;

            *phkResult = pOpenKeyExResp->hkResult;
            pOpenKeyExResp->hkResult = NULL;

            break;
        case REG_R_OPEN_KEYW_EX_FAILURE:
            pError = (PREG_IPC_ERROR) out.data;
            dwError = pError->dwError;
            BAIL_ON_REG_ERROR(dwError);
            break;
        default:
            dwError = EINVAL;
            BAIL_ON_REG_ERROR(dwError);
    }

cleanup:
    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:
    goto cleanup;
}

DWORD
RegTransactCloseKey(
    IN HANDLE hConnection,
    IN HKEY hKey
    )
{
    DWORD dwError = 0;
    REG_IPC_CLOSE_KEY_REQ CloseKeyReq;
    // Do not free pError
    PREG_IPC_ERROR pError = NULL;

    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    dwError = RegIpcAcquireCall(hConnection, &pCall);
    BAIL_ON_REG_ERROR(dwError);

    CloseKeyReq.hKey = hKey;

    in.tag = REG_Q_CLOSE_KEY;
    in.data = &CloseKeyReq;

    dwError = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_REG_ERROR(dwError);

    switch (out.tag)
    {
        case REG_R_CLOSE_KEY_SUCCESS:
            dwError = RegIpcUnregisterHandle(pCall, hKey);
            BAIL_ON_REG_ERROR(dwError);

            break;

        case REG_R_CLOSE_KEY_FAILURE:
            pError = (PREG_IPC_ERROR) out.data;
            dwError = pError->dwError;
            BAIL_ON_REG_ERROR(dwError);
            break;

        default:
            dwError = EINVAL;
            BAIL_ON_REG_ERROR(dwError);
    }

cleanup:
    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:
    goto cleanup;
}

DWORD
RegTransactDeleteKeyW(
    IN HANDLE hConnection,
    IN HKEY hKey,
    IN PCWSTR pSubKey
    )
{
    DWORD dwError = 0;
    REG_IPC_DELETE_KEY_REQ DeleteKeyReq;
    // Do not free pError
    PREG_IPC_ERROR pError = NULL;

    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    dwError = RegIpcAcquireCall(hConnection, &pCall);
    BAIL_ON_REG_ERROR(dwError);

    DeleteKeyReq.hKey = hKey;
    DeleteKeyReq.pSubKey = pSubKey;

    in.tag = REG_Q_DELETE_KEY;
    in.data = &DeleteKeyReq;

    dwError = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_REG_ERROR(dwError);

    switch (out.tag)
    {
        case REG_R_DELETE_KEY_SUCCESS:
            break;

        case REG_R_DELETE_KEY_FAILURE:
            pError = (PREG_IPC_ERROR) out.data;
            dwError = pError->dwError;
            BAIL_ON_REG_ERROR(dwError);
            break;

        default:
            dwError = EINVAL;
            BAIL_ON_REG_ERROR(dwError);
    }

cleanup:
    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:
    goto cleanup;
}

DWORD
RegTransactQueryInfoKeyW(
    IN HANDLE hConnection,
    IN HKEY hKey,
    OUT PWSTR pClass,
    IN OUT OPTIONAL PDWORD pcClass,
    IN PDWORD pReserved,
    OUT OPTIONAL PDWORD pcSubKeys,
    OUT OPTIONAL PDWORD pcMaxSubKeyLen,
    OUT OPTIONAL PDWORD pcMaxClassLen,
    OUT OPTIONAL PDWORD pcValues,
    OUT OPTIONAL PDWORD pcMaxValueNameLen,
    OUT OPTIONAL PDWORD pcMaxValueLen,
    OUT OPTIONAL PDWORD pcbSecurityDescriptor,
    OUT OPTIONAL PFILETIME pftLastWriteTime
    )
{
    DWORD dwError = 0;
    REG_IPC_QUERY_INFO_KEY_REQ QueryInfoKeyReq;
    // Do not free pError
    PREG_IPC_ERROR pError = NULL;
    PREG_IPC_QUERY_INFO_KEY_RESPONSE pQueryInfoKeyResp = NULL;

    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    dwError = RegIpcAcquireCall(hConnection, &pCall);
    BAIL_ON_REG_ERROR(dwError);

    QueryInfoKeyReq.hKey = hKey;
    QueryInfoKeyReq.pcClass = pcClass;

    in.tag = REG_Q_QUERY_INFO_KEYW;
    in.data = &QueryInfoKeyReq;

    dwError = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_REG_ERROR(dwError);

    switch (out.tag)
    {
        case REG_R_QUERY_INFO_KEYW_SUCCESS:
            pQueryInfoKeyResp = (PREG_IPC_QUERY_INFO_KEY_RESPONSE) out.data;

            if (pcSubKeys)
            {
                *pcSubKeys = pQueryInfoKeyResp->cSubKeys;
            }
            if (pcMaxSubKeyLen)
            {
                *pcMaxSubKeyLen = pQueryInfoKeyResp->cMaxSubKeyLen;
            }
            if (pcValues)
            {
                *pcValues = pQueryInfoKeyResp->cValues;
            }
            if (pcMaxValueNameLen)
            {
                *pcMaxValueNameLen = pQueryInfoKeyResp->cMaxValueNameLen;
            }
            if (pcMaxValueLen)
            {
                *pcMaxValueLen = pQueryInfoKeyResp->cMaxValueLen;
            }

            break;
        case REG_R_QUERY_INFO_KEYW_FAILURE:
            pError = (PREG_IPC_ERROR) out.data;
            dwError = pError->dwError;
            BAIL_ON_REG_ERROR(dwError);
            break;
        default:
            dwError = EINVAL;
            BAIL_ON_REG_ERROR(dwError);
    }

cleanup:
    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:
    goto cleanup;
}

DWORD
RegTransactQueryInfoKeyA(
    IN HANDLE hConnection,
    IN HKEY hKey,
    OUT PSTR pszClass,
    IN OUT OPTIONAL PDWORD pcClass,
    IN PDWORD pReserved,
    OUT OPTIONAL PDWORD pcSubKeys,
    OUT OPTIONAL PDWORD pcMaxSubKeyLen,
    OUT OPTIONAL PDWORD pcMaxClassLen,
    OUT OPTIONAL PDWORD pcValues,
    OUT OPTIONAL PDWORD pcMaxValueNameLen,
    OUT OPTIONAL PDWORD pcMaxValueLen,
    OUT OPTIONAL PDWORD pcbSecurityDescriptor,
    OUT OPTIONAL PFILETIME pftLastWriteTime
    )
{
    DWORD dwError = 0;
    REG_IPC_QUERY_INFO_KEY_REQ QueryInfoKeyReq;
    // Do not free pError
    PREG_IPC_ERROR pError = NULL;
    PREG_IPC_QUERY_INFO_KEY_RESPONSE pQueryInfoKeyResp = NULL;

    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    dwError = RegIpcAcquireCall(hConnection, &pCall);
    BAIL_ON_REG_ERROR(dwError);

    QueryInfoKeyReq.hKey = hKey;
    QueryInfoKeyReq.pcClass = pcClass;

    in.tag = REG_Q_QUERY_INFO_KEYA;
    in.data = &QueryInfoKeyReq;

    dwError = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_REG_ERROR(dwError);

    switch (out.tag)
    {
        case REG_R_QUERY_INFO_KEYA_SUCCESS:
            pQueryInfoKeyResp = (PREG_IPC_QUERY_INFO_KEY_RESPONSE) out.data;

            if (pcSubKeys)
            {
                *pcSubKeys = pQueryInfoKeyResp->cSubKeys;
            }
            if (pcMaxSubKeyLen)
            {
                *pcMaxSubKeyLen = pQueryInfoKeyResp->cMaxSubKeyLen;
            }
            if (pcValues)
            {
                *pcValues = pQueryInfoKeyResp->cValues;
            }
            if (pcMaxValueNameLen)
            {
                *pcMaxValueNameLen = pQueryInfoKeyResp->cMaxValueNameLen;
            }
            if (pcMaxValueLen)
            {
                *pcMaxValueLen = pQueryInfoKeyResp->cMaxValueLen;
            }

            break;
        case REG_R_QUERY_INFO_KEYA_FAILURE:
            pError = (PREG_IPC_ERROR) out.data;
            dwError = pError->dwError;
            BAIL_ON_REG_ERROR(dwError);
            break;
        default:
            dwError = EINVAL;
            BAIL_ON_REG_ERROR(dwError);
    }

cleanup:
    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:
    goto cleanup;
}

DWORD
RegTransactEnumKeyEx(
    IN HANDLE hConnection,
    IN HKEY hKey,
    IN DWORD dwIndex,
    IN OUT PWSTR pName,
    IN OUT PDWORD pcName,
    IN PDWORD pReserved,
    IN OUT PWSTR pClass,
    IN OUT OPTIONAL PDWORD pcClass,
    OUT OPTIONAL PFILETIME pftLastWriteTime
    )
{
    DWORD dwError = 0;

    REG_IPC_ENUM_KEY_EX_REQ EnumKeyExReq;
    // Do not free pError
    PREG_IPC_ERROR pError = NULL;
    PREG_IPC_ENUM_KEY_EX_RESPONSE pEnumKeyExResp = NULL;

    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    dwError = RegIpcAcquireCall(hConnection, &pCall);
    BAIL_ON_REG_ERROR(dwError);

    EnumKeyExReq.hKey = hKey;
    EnumKeyExReq.dwIndex = dwIndex;
    EnumKeyExReq.pName = pName;
    EnumKeyExReq.cName = *pcName;
    EnumKeyExReq.pClass = pClass;
    EnumKeyExReq.pcClass = pcClass;

    in.tag = REG_Q_ENUM_KEY_EX;
    in.data = &EnumKeyExReq;

    dwError = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_REG_ERROR(dwError);

    switch (out.tag)
    {
        case REG_R_ENUM_KEY_EX_SUCCESS:
            pEnumKeyExResp = (PREG_IPC_ENUM_KEY_EX_RESPONSE) out.data;

            memcpy(pName, pEnumKeyExResp->pName, (pEnumKeyExResp->cName+1)*sizeof(*pName));
            *pcName = pEnumKeyExResp->cName;

            break;

        case REG_R_ENUM_KEY_EX_FAILURE:
            pError = (PREG_IPC_ERROR) out.data;
            dwError = pError->dwError;
            BAIL_ON_REG_ERROR(dwError);
            break;
        default:
            dwError = EINVAL;
            BAIL_ON_REG_ERROR(dwError);
    }

cleanup:
    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:
    goto cleanup;
}

DWORD
RegTransactGetValueA(
    IN HANDLE hConnection,
    IN HKEY hKey,
    IN OPTIONAL PCSTR pszSubKey,
    IN OPTIONAL PCSTR pszValue,
    IN OPTIONAL REG_DATA_TYPE_FLAGS Flags,
    OUT OPTIONAL PDWORD pdwType,
    OUT OPTIONAL PVOID pvData,
    IN OUT OPTIONAL PDWORD pcbData
    )
{
    DWORD dwError = 0;
    REG_IPC_GET_VALUEA_REQ GetValueReq;
    PREG_IPC_GET_VALUE_RESPONSE pGetValueResp = NULL;
    // Do not free pError
    PREG_IPC_ERROR pError = NULL;

    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    dwError = RegIpcAcquireCall(hConnection, &pCall);
    BAIL_ON_REG_ERROR(dwError);

    GetValueReq.hKey = hKey;
    GetValueReq.pszSubKey = pszSubKey;
    GetValueReq.pszValue = pszValue;
    GetValueReq.Flags = Flags;
    GetValueReq.pData = pvData;
    GetValueReq.cbData = *pcbData;

    in.tag = REG_Q_GET_VALUEA;
    in.data = &GetValueReq;

    dwError = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_REG_ERROR(dwError);

    switch (out.tag)
    {
        case REG_R_GET_VALUEA_SUCCESS:
            pGetValueResp = (PREG_IPC_GET_VALUE_RESPONSE) out.data;

            if (pdwType)
            {
                *pdwType = pGetValueResp->dwType;
            }

            if (pvData)
            {
                memcpy(pvData, pGetValueResp->pvData, pGetValueResp->cbData);
            }

            if (pcbData)
            {
                *pcbData = pGetValueResp->cbData;
            }

            break;

        case REG_R_GET_VALUEA_FAILURE:
            pError = (PREG_IPC_ERROR) out.data;
            dwError = pError->dwError;
            BAIL_ON_REG_ERROR(dwError);
            break;
        default:
            dwError = EINVAL;
            BAIL_ON_REG_ERROR(dwError);
    }

cleanup:
    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:
    goto cleanup;
}

DWORD
RegTransactGetValueW(
    IN HANDLE hConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pSubKey,
    IN OPTIONAL PCWSTR pValue,
    IN OPTIONAL REG_DATA_TYPE_FLAGS Flags,
    OUT OPTIONAL PDWORD pdwType,
    OUT OPTIONAL PVOID pvData,
    IN OUT OPTIONAL PDWORD pcbData
    )
{
    DWORD dwError = 0;
    REG_IPC_GET_VALUE_REQ GetValueReq;
    PREG_IPC_GET_VALUE_RESPONSE pGetValueResp = NULL;
    // Do not free pError
    PREG_IPC_ERROR pError = NULL;

    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    dwError = RegIpcAcquireCall(hConnection, &pCall);
    BAIL_ON_REG_ERROR(dwError);

    GetValueReq.hKey = hKey;
    GetValueReq.pSubKey = pSubKey;
    GetValueReq.pValue = pValue;
    GetValueReq.Flags = Flags;
    GetValueReq.pData = pvData;
    GetValueReq.cbData = *pcbData;

    in.tag = REG_Q_GET_VALUEW;
    in.data = &GetValueReq;

    dwError = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_REG_ERROR(dwError);

    switch (out.tag)
    {
        case REG_R_GET_VALUEW_SUCCESS:
            pGetValueResp = (PREG_IPC_GET_VALUE_RESPONSE) out.data;

            if (pdwType)
            {
                *pdwType = pGetValueResp->dwType;
            }

            if (pvData)
            {
                memcpy(pvData, pGetValueResp->pvData, pGetValueResp->cbData);
            }

            if (pcbData)
            {
                *pcbData = pGetValueResp->cbData;
            }

            break;

        case REG_R_GET_VALUEW_FAILURE:
            pError = (PREG_IPC_ERROR) out.data;
            dwError = pError->dwError;
            BAIL_ON_REG_ERROR(dwError);
            break;
        default:
            dwError = EINVAL;
            BAIL_ON_REG_ERROR(dwError);
    }

cleanup:
    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:
    goto cleanup;
}

DWORD
RegTransactQueryValueExA(
    IN HANDLE hConnection,
    IN HKEY hKey,
    IN OPTIONAL PCSTR pszValueName,
    IN PDWORD pReserved,
    OUT OPTIONAL PDWORD pType,
    OUT OPTIONAL PBYTE pData,
    IN OUT OPTIONAL PDWORD pcbData
    )
{
    DWORD dwError = 0;
    REG_IPC_GET_VALUEA_REQ QueryValueExReq;
    PREG_IPC_GET_VALUE_RESPONSE pQueryValueResp = NULL;
    // Do not free pError
    PREG_IPC_ERROR pError = NULL;

    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    dwError = RegIpcAcquireCall(hConnection, &pCall);
    BAIL_ON_REG_ERROR(dwError);

    QueryValueExReq.hKey = hKey;
    QueryValueExReq.pszValue = pszValueName;
    QueryValueExReq.pData = pData;
    QueryValueExReq.cbData = *pcbData;


    in.tag = REG_Q_QUERY_VALUEA_EX;
    in.data = &QueryValueExReq;

    dwError = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_REG_ERROR(dwError);

    switch (out.tag)
    {
        case REG_R_QUERY_VALUEA_EX_SUCCESS:
            pQueryValueResp = (PREG_IPC_GET_VALUE_RESPONSE) out.data;

            if (pType)
            {
                *pType = pQueryValueResp->dwType;
            }

            if (pData)
            {
                memcpy(pData, pQueryValueResp->pvData, pQueryValueResp->cbData);
            }

            if (pcbData)
            {
                *pcbData = pQueryValueResp->cbData;
            }
            break;
        case REG_R_QUERY_VALUEA_EX_FAILURE:
            pError = (PREG_IPC_ERROR) out.data;
            dwError = pError->dwError;
            BAIL_ON_REG_ERROR(dwError);
            break;
        default:
            dwError = EINVAL;
            BAIL_ON_REG_ERROR(dwError);
    }

cleanup:
    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:
    goto cleanup;
}

DWORD
RegTransactQueryValueExW(
    IN HANDLE hConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pValueName,
    IN PDWORD pReserved,
    OUT OPTIONAL PDWORD pType,
    OUT OPTIONAL PBYTE pData,
    IN OUT OPTIONAL PDWORD pcbData
    )
{
    DWORD dwError = 0;
    REG_IPC_GET_VALUE_REQ QueryValueExReq;
    PREG_IPC_GET_VALUE_RESPONSE pQueryValueResp = NULL;
    // Do not free pError
    PREG_IPC_ERROR pError = NULL;

    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    dwError = RegIpcAcquireCall(hConnection, &pCall);
    BAIL_ON_REG_ERROR(dwError);

    QueryValueExReq.hKey = hKey;
    QueryValueExReq.pValue = pValueName;
    QueryValueExReq.pData = pData;
    QueryValueExReq.cbData = *pcbData;


    in.tag = REG_Q_QUERY_VALUEW_EX;
    in.data = &QueryValueExReq;

    dwError = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_REG_ERROR(dwError);

    switch (out.tag)
    {
        case REG_R_QUERY_VALUEW_EX_SUCCESS:
            pQueryValueResp = (PREG_IPC_GET_VALUE_RESPONSE) out.data;

            if (pType)
            {
                *pType = pQueryValueResp->dwType;
            }

            if (pData)
            {
                memcpy(pData, pQueryValueResp->pvData, pQueryValueResp->cbData);
            }

            if (pcbData)
            {
                *pcbData = pQueryValueResp->cbData;
            }
            break;
        case REG_R_QUERY_VALUEW_EX_FAILURE:
            pError = (PREG_IPC_ERROR) out.data;
            dwError = pError->dwError;
            BAIL_ON_REG_ERROR(dwError);
            break;
        default:
            dwError = EINVAL;
            BAIL_ON_REG_ERROR(dwError);
    }

cleanup:
    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:
    goto cleanup;
}

DWORD
RegTransactDeleteKeyValue(
    IN HANDLE hConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pSubKey,
    IN OPTIONAL PCWSTR pValueName
    )
{
    DWORD dwError = 0;
    REG_IPC_DELETE_KEY_VALUE_REQ DeleteKeyValueReq;
    // Do not free pError
    PREG_IPC_ERROR pError = NULL;

    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    dwError = RegIpcAcquireCall(hConnection, &pCall);
    BAIL_ON_REG_ERROR(dwError);

    DeleteKeyValueReq.hKey = hKey;
    DeleteKeyValueReq.pSubKey = pSubKey;
    DeleteKeyValueReq.pValueName = pValueName;

    in.tag = REG_Q_DELETE_KEY_VALUE;
    in.data = &DeleteKeyValueReq;

    dwError = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_REG_ERROR(dwError);

    switch (out.tag)
    {
        case REG_R_DELETE_KEY_VALUE_SUCCESS:
            break;

        case REG_R_DELETE_KEY_VALUE_FAILURE:
            pError = (PREG_IPC_ERROR) out.data;
            dwError = pError->dwError;
            BAIL_ON_REG_ERROR(dwError);
            break;

        default:
            dwError = EINVAL;
            BAIL_ON_REG_ERROR(dwError);
    }

cleanup:
    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }
    return dwError;

error:
    goto cleanup;
}

DWORD
RegTransactDeleteTree(
    IN HANDLE hConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pSubKey
    )
{
    DWORD dwError = 0;
    REG_IPC_DELETE_TREE_REQ DeleteTreeReq;
    // Do not free pError
    PREG_IPC_ERROR pError = NULL;

    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    dwError = RegIpcAcquireCall(hConnection, &pCall);
    BAIL_ON_REG_ERROR(dwError);

    DeleteTreeReq.hKey = hKey;
    DeleteTreeReq.pSubKey = pSubKey;

    in.tag = REG_Q_DELETE_TREE;
    in.data = &DeleteTreeReq;

    dwError = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_REG_ERROR(dwError);

    switch (out.tag)
    {
        case REG_R_DELETE_TREE_SUCCESS:
            break;

        case REG_R_DELETE_TREE_FAILURE:
            pError = (PREG_IPC_ERROR) out.data;
            dwError = pError->dwError;
            BAIL_ON_REG_ERROR(dwError);
            break;
        default:
            dwError = EINVAL;
            BAIL_ON_REG_ERROR(dwError);
    }

cleanup:
    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:
    goto cleanup;
}

DWORD
RegTransactDeleteValue(
    IN HANDLE hConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pValueName
    )
{
    DWORD dwError = 0;
    REG_IPC_DELETE_VALUE_REQ DeleteValueReq;
    // Do not free pError
    PREG_IPC_ERROR pError = NULL;

    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    dwError = RegIpcAcquireCall(hConnection, &pCall);
    BAIL_ON_REG_ERROR(dwError);

    DeleteValueReq.hKey = hKey;
    DeleteValueReq.pValueName = pValueName;

    in.tag = REG_Q_DELETE_VALUE;
    in.data = &DeleteValueReq;

    dwError = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_REG_ERROR(dwError);

    switch (out.tag)
    {
        case REG_R_DELETE_VALUE_SUCCESS:
            break;
        case REG_R_DELETE_VALUE_FAILURE:
            pError = (PREG_IPC_ERROR) out.data;
            dwError = pError->dwError;
            BAIL_ON_REG_ERROR(dwError);
            break;
        default:
            dwError = EINVAL;
            BAIL_ON_REG_ERROR(dwError);
    }

cleanup:
    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:
    goto cleanup;
}

DWORD
RegTransactEnumValueA(
    IN HANDLE hConnection,
    IN HKEY hKey,
    IN DWORD dwIndex,
    OUT PSTR pszValueName,
    IN OUT PDWORD pcchValueName,
    IN PDWORD pReserved,
    OUT OPTIONAL PDWORD pType,
    OUT OPTIONAL PBYTE pData,
    IN OUT OPTIONAL PDWORD pcbData
    )
{
    DWORD dwError = 0;
    REG_IPC_ENUM_VALUEA_REQ EnumValueReq;
    PREG_IPC_ENUM_VALUEA_RESPONSE pEnumValueResp = NULL;
    // Do not free pError
    PREG_IPC_ERROR pError = NULL;

    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    dwError = RegIpcAcquireCall(hConnection, &pCall);
    BAIL_ON_REG_ERROR(dwError);

    EnumValueReq.hKey = hKey;
    EnumValueReq.dwIndex = dwIndex;
    EnumValueReq.pszName = pszValueName;
    EnumValueReq.cName = *pcchValueName;
    EnumValueReq.pValue = pData;
    EnumValueReq.cValue = pcbData == NULL ? 0 : *pcbData;


    in.tag = REG_Q_ENUM_VALUEA;
    in.data = &EnumValueReq;

    dwError = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_REG_ERROR(dwError);

    switch (out.tag)
    {
        case REG_R_ENUM_VALUEA_SUCCESS:
            pEnumValueResp = (PREG_IPC_ENUM_VALUEA_RESPONSE) out.data;

            memcpy(pszValueName, pEnumValueResp->pszName, (pEnumValueResp->cName+1)*sizeof(*pszValueName));
            *pcchValueName = pEnumValueResp->cName;

            if (pData)
            {
                memcpy(pData, pEnumValueResp->pValue, pEnumValueResp->cValue*sizeof(*pData));
            }

            if (pcbData)
            {
                *pcbData = pEnumValueResp->cValue;
            }

            if (pType)
            {
                *pType = pEnumValueResp->type;
            }

            break;

        case REG_R_ENUM_VALUEA_FAILURE:
            pError = (PREG_IPC_ERROR) out.data;
            dwError = pError->dwError;
            BAIL_ON_REG_ERROR(dwError);
            break;
        default:
            dwError = EINVAL;
            BAIL_ON_REG_ERROR(dwError);
    }

cleanup:
    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:
    goto cleanup;
}

DWORD
RegTransactEnumValueW(
    IN HANDLE hConnection,
    IN HKEY hKey,
    IN DWORD dwIndex,
    OUT PWSTR pValueName,
    IN OUT PDWORD pcchValueName,
    IN PDWORD pReserved,
    OUT OPTIONAL PDWORD pType,
    OUT OPTIONAL PBYTE pData,
    IN OUT OPTIONAL PDWORD pcbData
    )
{
    DWORD dwError = 0;
    REG_IPC_ENUM_VALUE_REQ EnumValueReq;
    PREG_IPC_ENUM_VALUE_RESPONSE pEnumValueResp = NULL;
    // Do not free pError
    PREG_IPC_ERROR pError = NULL;

    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    dwError = RegIpcAcquireCall(hConnection, &pCall);
    BAIL_ON_REG_ERROR(dwError);

    EnumValueReq.hKey = hKey;
    EnumValueReq.dwIndex = dwIndex;
    EnumValueReq.pName = pValueName;
    EnumValueReq.cName = *pcchValueName;
    EnumValueReq.pValue = pData;
    EnumValueReq.cValue = pcbData == NULL ? 0 : *pcbData;


    in.tag = REG_Q_ENUM_VALUEW;
    in.data = &EnumValueReq;

    dwError = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_REG_ERROR(dwError);

    switch (out.tag)
    {
        case REG_R_ENUM_VALUEW_SUCCESS:
            pEnumValueResp = (PREG_IPC_ENUM_VALUE_RESPONSE) out.data;

            memcpy(pValueName, pEnumValueResp->pName, (pEnumValueResp->cName+1)*sizeof(*pValueName));
            *pcchValueName = pEnumValueResp->cName;

            if (pData)
            {
                memcpy(pData, pEnumValueResp->pValue, pEnumValueResp->cValue*sizeof(*pData));
            }

            if (pcbData)
            {
                *pcbData = pEnumValueResp->cValue;
            }

            if (pType)
            {
                *pType = pEnumValueResp->type;
            }

            break;

        case REG_R_ENUM_VALUEW_FAILURE:
            pError = (PREG_IPC_ERROR) out.data;
            dwError = pError->dwError;
            BAIL_ON_REG_ERROR(dwError);
            break;
        default:
            dwError = EINVAL;
            BAIL_ON_REG_ERROR(dwError);
    }

cleanup:
    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:
    goto cleanup;
}

DWORD
RegTransactQueryMultipleValues(
    IN HANDLE hConnection,
    IN HKEY hKey,
    OUT PVALENT val_list,
    IN DWORD num_vals,
    OUT OPTIONAL PWSTR pValueBuf,
    IN OUT OPTIONAL PDWORD pdwTotsize
    )
{
    DWORD dwError = 0;
    REG_IPC_QUERY_MULTIPLE_VALUES_REQ QueryMultipleValuesReq;
    PREG_IPC_QUERY_MULTIPLE_VALUES_RESPONSE pRegResp = NULL;
    // Do not free pError
    PREG_IPC_ERROR pError = NULL;
    int iCount = 0;
    int offSet = 0;

    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    dwError = RegIpcAcquireCall(hConnection, &pCall);
    BAIL_ON_REG_ERROR(dwError);

    QueryMultipleValuesReq.hKey = hKey;
    QueryMultipleValuesReq.num_vals = num_vals;
    QueryMultipleValuesReq.val_list = val_list;
    if (pValueBuf)
    {
        QueryMultipleValuesReq.pValue = pValueBuf;
    }
    if (pdwTotsize)
    {
        QueryMultipleValuesReq.dwTotalsize = *pdwTotsize;
    }

    in.tag = REG_Q_QUERY_MULTIPLE_VALUES;
    in.data = &QueryMultipleValuesReq;

    dwError = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_REG_ERROR(dwError);

    switch (out.tag)
    {
       case REG_R_QUERY_MULTIPLE_VALUES_SUCCESS:
            pRegResp = (PREG_IPC_QUERY_MULTIPLE_VALUES_RESPONSE) out.data;

            if (pValueBuf)
            {
                memcpy(pValueBuf, pRegResp->pValue, pRegResp->dwTotalsize*sizeof(*pRegResp->pValue));
            }

            for (iCount = 0; iCount < pRegResp->num_vals; iCount++)
            {
                offSet = iCount==0 ? 0 : (offSet + val_list[iCount-1].ve_valuelen);

                val_list[iCount].ve_type = pRegResp->val_list[iCount].ve_type;
                val_list[iCount].ve_valuelen = pRegResp->val_list[iCount].ve_valuelen;

                if (pValueBuf)
                {
                    val_list[iCount].ve_valueptr = (PDWORD)(pValueBuf+offSet);
                }
            }
            if (pdwTotsize)
            {
                *pdwTotsize = pRegResp->dwTotalsize;
            }

            break;

        case REG_R_QUERY_MULTIPLE_VALUES_FAILURE:
            pError = (PREG_IPC_ERROR) out.data;
            dwError = pError->dwError;
            BAIL_ON_REG_ERROR(dwError);
            break;
        default:
            dwError = EINVAL;
            BAIL_ON_REG_ERROR(dwError);
    }

cleanup:
    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:
    goto cleanup;
}

DWORD
RegTransactSetKeyValue(
    IN HANDLE hConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pSubKey,
    IN OPTIONAL PCWSTR pValueName,
    IN DWORD dwType,
    IN OPTIONAL PCVOID pData,
    IN DWORD cbData
    )
{
    DWORD dwError = 0;
    REG_IPC_SET_KEY_VALUE_REQ SetValueExReq;
    // Do not free pError
    PREG_IPC_ERROR pError = NULL;

    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;
    LWMsgCall* pCall = NULL;

    dwError = RegIpcAcquireCall(hConnection, &pCall);
    BAIL_ON_REG_ERROR(dwError);

    SetValueExReq.hKey = hKey;
    SetValueExReq.pValueName = pValueName;
    SetValueExReq.dwType = dwType;
    SetValueExReq.pData = pData;
    SetValueExReq.cbData = cbData;

    in.tag = REG_Q_SET_KEY_VALUE;
    in.data = &SetValueExReq;

    dwError = MAP_LWMSG_ERROR(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_REG_ERROR(dwError);

    switch (out.tag)
    {
        case REG_R_SET_KEY_VALUE_SUCCESS:
            break;

        case REG_R_SET_KEY_VALUE_FAILURE:
            pError = (PREG_IPC_ERROR) out.data;
            dwError = pError->dwError;
            BAIL_ON_REG_ERROR(dwError);
            break;

        default:
            dwError = EINVAL;
            BAIL_ON_REG_ERROR(dwError);
    }

cleanup:
    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:
    goto cleanup;
}

DWORD
RegTransactSetValueExA(
    IN HANDLE hConnection,
    IN HKEY hKey,
    IN OPTIONAL PCSTR pszValueName,
    IN DWORD Reserved,
    IN DWORD dwType,
    IN OPTIONAL const BYTE *pData,
    IN DWORD cbData
    )
{
    DWORD dwError = 0;
    PREG_CLIENT_CONNECTION_CONTEXT pContext =
                     (PREG_CLIENT_CONNECTION_CONTEXT)hConnection;

    REG_IPC_SET_VALUEA_EX_REQ SetValueExReq;
    // Do not free pError
    PREG_IPC_ERROR pError = NULL;

    LWMsgMessage request = LWMSG_MESSAGE_INITIALIZER;
    LWMsgMessage response = LWMSG_MESSAGE_INITIALIZER;

    SetValueExReq.hKey = hKey;
    SetValueExReq.pszValueName = pszValueName;
    SetValueExReq.dwType = dwType;
    SetValueExReq.pData = pData;
    SetValueExReq.cbData = cbData;

    request.tag = REG_Q_SET_VALUEA_EX;
    request.object = &SetValueExReq;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_send_message_transact(
                              pContext->pAssoc,
                              &request,
                              &response));
    BAIL_ON_REG_ERROR(dwError);

    switch (response.tag)
    {
        case REG_R_SET_VALUEA_EX_SUCCESS:
            break;

        case REG_R_SET_VALUEA_EX_FAILURE:
            pError = (PREG_IPC_ERROR) response.object;
            dwError = pError->dwError;
            BAIL_ON_REG_ERROR(dwError);
            break;

        default:
            dwError = EINVAL;
            BAIL_ON_REG_ERROR(dwError);
    }

cleanup:

    return dwError;

error:

    if (response.object)
    {
        lwmsg_assoc_free_message(pContext->pAssoc, &response);
    }

    goto cleanup;
}

DWORD
RegTransactSetValueExW(
    IN HANDLE hConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pValueName,
    IN DWORD Reserved,
    IN DWORD dwType,
    IN OPTIONAL const BYTE *pData,
    IN DWORD cbData
    )
{
    DWORD dwError = 0;
    PREG_CLIENT_CONNECTION_CONTEXT pContext =
                     (PREG_CLIENT_CONNECTION_CONTEXT)hConnection;

    REG_IPC_SET_VALUE_EX_REQ SetValueExReq;
    // Do not free pError
    PREG_IPC_ERROR pError = NULL;

    LWMsgMessage request = LWMSG_MESSAGE_INITIALIZER;
    LWMsgMessage response = LWMSG_MESSAGE_INITIALIZER;

    SetValueExReq.hKey = hKey;
    SetValueExReq.pValueName = pValueName;
    SetValueExReq.dwType = dwType;
    SetValueExReq.pData = pData;
    SetValueExReq.cbData = cbData;

    request.tag = REG_Q_SET_VALUEW_EX;
    request.object = &SetValueExReq;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_send_message_transact(
                              pContext->pAssoc,
                              &request,
                              &response));
    BAIL_ON_REG_ERROR(dwError);

    switch (response.tag)
    {
        case REG_R_SET_VALUEW_EX_SUCCESS:
            break;

        case REG_R_SET_VALUEW_EX_FAILURE:
            pError = (PREG_IPC_ERROR) response.object;
            dwError = pError->dwError;
            BAIL_ON_REG_ERROR(dwError);
            break;

        default:
            dwError = EINVAL;
            BAIL_ON_REG_ERROR(dwError);
    }

cleanup:

    return dwError;

error:

    if (response.object)
    {
        lwmsg_assoc_free_message(pContext->pAssoc, &response);
    }

    goto cleanup;
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
