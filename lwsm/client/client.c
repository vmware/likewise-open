/*
 * Copyright (c) Likewise Software.  All rights Reserved.
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
 * Module Name:
 *
 *        client.c
 *
 * Abstract:
 *
 *        Client API entry points
 *
 * Authors: Brian Koropoff (bkoropoff@likewise.com)
 *
 */

#include "includes.h"

DWORD
LwSmAcquireServiceHandle(
    LW_PCWSTR pwszServiceName,
    PLW_SERVICE_HANDLE phHandle
    )
{
    DWORD dwError = 0;
    LWMsgCall* pCall = NULL;
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;

    in.tag = SM_IPC_ACQUIRE_SERVICE_HANDLE_REQ;
    in.data = (PVOID) pwszServiceName;

    dwError = LwSmIpcAcquireCall(&pCall);
    BAIL_ON_ERROR(dwError);

    dwError = MAP_LWMSG_STATUS(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_ERROR(dwError);

    switch (out.tag)
    {
    case SM_IPC_ACQUIRE_SERVICE_HANDLE_RES:
        *phHandle = out.data;
        out.data = NULL;
        break;
    case SM_IPC_ERROR:
        dwError = *(PDWORD) out.data;
        BAIL_ON_ERROR(dwError);
        break;
    default:
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_ERROR(dwError);
        break;
    }

cleanup:

    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:

    *phHandle = NULL;

    goto cleanup;
}

DWORD
LwSmReleaseServiceHandle(
    LW_SERVICE_HANDLE hHandle
    )
{
    DWORD dwError = 0;
    LWMsgCall* pCall = NULL;
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;

    in.tag = SM_IPC_RELEASE_SERVICE_HANDLE_REQ;
    in.data = (PVOID) hHandle;

    dwError = LwSmIpcAcquireCall(&pCall);
    BAIL_ON_ERROR(dwError);

    dwError = MAP_LWMSG_STATUS(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_ERROR(dwError);

    switch (out.tag)
    {
    case SM_IPC_RELEASE_SERVICE_HANDLE_RES:
        break;
    case SM_IPC_ERROR:
        dwError = *(PDWORD) out.data;
        BAIL_ON_ERROR(dwError);
        break;
    default:
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_ERROR(dwError);
        break;
    }

cleanup:

    if (pCall)
    {
        /* Destroying the in parameters will free the handle */
        lwmsg_call_destroy_params(pCall, &in);
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
LwSmEnumerateServices(
    PWSTR** pppwszServiceNames
    )
{
    DWORD dwError = 0;
    LWMsgCall* pCall = NULL;
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;

    in.tag = SM_IPC_ENUMERATE_SERVICES_REQ;
    in.data = NULL;

    dwError = LwSmIpcAcquireCall(&pCall);
    BAIL_ON_ERROR(dwError);

    dwError = MAP_LWMSG_STATUS(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_ERROR(dwError);

    switch (out.tag)
    {
    case SM_IPC_ENUMERATE_SERVICES_RES:
        *pppwszServiceNames = (PWSTR*) out.data;
        out.data = NULL;
        break;
    case SM_IPC_ERROR:
        dwError = *(PDWORD) out.data;
        BAIL_ON_ERROR(dwError);
        break;
    default:
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_ERROR(dwError);
        break;
    }

cleanup:

    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:

    *pppwszServiceNames = NULL;

    goto cleanup;
}

VOID
LwSmFreeServiceNameList(
    PWSTR* ppwszServiceNames
    )
{
    return LwSmFreeStringList(ppwszServiceNames);
}

DWORD
LwSmAddService(
    PCLW_SERVICE_INFO pServiceInfo
    )
{
    DWORD dwError = 0;
    LWMsgCall* pCall = NULL;
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;

    in.tag = SM_IPC_ADD_SERVICE_REQ;
    in.data = (PVOID) pServiceInfo;

    dwError = LwSmIpcAcquireCall(&pCall);
    BAIL_ON_ERROR(dwError);

    dwError = MAP_LWMSG_STATUS(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_ERROR(dwError);

    switch (out.tag)
    {
    case SM_IPC_ADD_SERVICE_RES:
        break;
    case SM_IPC_ERROR:
        dwError = *(PDWORD) out.data;
        BAIL_ON_ERROR(dwError);
        break;
    default:
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_ERROR(dwError);
        break;
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
LwSmRemoveService(
    LW_PCWSTR pwszName
    )
{
    DWORD dwError = 0;
    LWMsgCall* pCall = NULL;
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;

    in.tag = SM_IPC_REMOVE_SERVICE_REQ;
    in.data = (PVOID) pwszName;

    dwError = LwSmIpcAcquireCall(&pCall);
    BAIL_ON_ERROR(dwError);

    dwError = MAP_LWMSG_STATUS(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_ERROR(dwError);

    switch (out.tag)
    {
    case SM_IPC_REMOVE_SERVICE_RES:
    case SM_IPC_ERROR:
        dwError = *(PDWORD) out.data;
        BAIL_ON_ERROR(dwError);
        break;
    default:
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_ERROR(dwError);
        break;
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

/**
 * Start a service
 */
DWORD
LwSmStartService(
    LW_SERVICE_HANDLE hHandle
    )
{
    DWORD dwError = 0;
    LWMsgCall* pCall = NULL;
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;

    in.tag = SM_IPC_START_SERVICE_REQ;
    in.data = hHandle;

    dwError = LwSmIpcAcquireCall(&pCall);
    BAIL_ON_ERROR(dwError);

    dwError = MAP_LWMSG_STATUS(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_ERROR(dwError);

    switch (out.tag)
    {
    case SM_IPC_START_SERVICE_RES:
        break;
    case SM_IPC_ERROR:
        dwError = *(PDWORD) out.data;
        BAIL_ON_ERROR(dwError);
        break;
    default:
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_ERROR(dwError);
        break;
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

/**
 * Stop a service
 */
DWORD
LwSmStopService(
    LW_SERVICE_HANDLE hHandle
    )
{
    DWORD dwError = 0;
    LWMsgCall* pCall = NULL;
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;

    in.tag = SM_IPC_STOP_SERVICE_REQ;
    in.data = hHandle;

    dwError = LwSmIpcAcquireCall(&pCall);
    BAIL_ON_ERROR(dwError);

    dwError = MAP_LWMSG_STATUS(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_ERROR(dwError);

    switch (out.tag)
    {
    case SM_IPC_STOP_SERVICE_RES:
        break;
    case SM_IPC_ERROR:
        dwError = *(PDWORD) out.data;
        BAIL_ON_ERROR(dwError);
        break;
    default:
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_ERROR(dwError);
        break;
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
LwSmGetServiceStatus(
    LW_SERVICE_HANDLE hHandle,
    PLW_SERVICE_STATUS pStatus
    )
{
    DWORD dwError = 0;
    LWMsgCall* pCall = NULL;
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;

    in.tag = SM_IPC_GET_SERVICE_STATUS_REQ;
    in.data = hHandle;

    dwError = LwSmIpcAcquireCall(&pCall);
    BAIL_ON_ERROR(dwError);

    dwError = MAP_LWMSG_STATUS(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_ERROR(dwError);

    switch (out.tag)
    {
    case SM_IPC_GET_SERVICE_STATUS_RES:
        *pStatus = *(PDWORD) out.data;
        break;
    case SM_IPC_ERROR:
        dwError = *(PDWORD) out.data;
        BAIL_ON_ERROR(dwError);
        break;
    default:
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_ERROR(dwError);
        break;
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

/**
 * Refresh service
 */
DWORD
LwSmRefreshService(
    LW_SERVICE_HANDLE hHandle
    )
{
    DWORD dwError = 0;
    LWMsgCall* pCall = NULL;
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;

    in.tag = SM_IPC_REFRESH_SERVICE_REQ;
    in.data = hHandle;

    dwError = LwSmIpcAcquireCall(&pCall);
    BAIL_ON_ERROR(dwError);

    dwError = MAP_LWMSG_STATUS(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_ERROR(dwError);

    switch (out.tag)
    {
    case SM_IPC_REFRESH_SERVICE_RES:
        break;
    case SM_IPC_ERROR:
        dwError = *(PDWORD) out.data;
        BAIL_ON_ERROR(dwError);
        break;
    default:
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_ERROR(dwError);
        break;
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
LwSmGetServiceInfo(
    LW_SERVICE_HANDLE hHandle,
    PLW_SERVICE_INFO* ppInfo
    )
{
    DWORD dwError = 0;
    LWMsgCall* pCall = NULL;
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;

    in.tag = SM_IPC_GET_SERVICE_INFO_REQ;
    in.data = hHandle;

    dwError = LwSmIpcAcquireCall(&pCall);
    BAIL_ON_ERROR(dwError);

    dwError = MAP_LWMSG_STATUS(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_ERROR(dwError);

    switch (out.tag)
    {
    case SM_IPC_GET_SERVICE_INFO_RES:
        *ppInfo = out.data;
        out.data = NULL;
        break;
    case SM_IPC_ERROR:
        dwError = *(PDWORD) out.data;
        BAIL_ON_ERROR(dwError);
        break;
    default:
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_ERROR(dwError);
        break;
    }

cleanup:

    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return dwError;

error:

    *ppInfo = NULL;

    goto cleanup;
}

VOID
LwSmFreeServiceInfo(
    PLW_SERVICE_INFO pInfo
    )
{
    LwSmCommonFreeServiceInfo(pInfo);
}

static
DWORD
LwSmGetServiceDependencyClosureHelper(
    LW_SERVICE_HANDLE hHandle,
    PWSTR** pppwszServiceList
    )
{
    DWORD dwError = 0;
    PLW_SERVICE_INFO pInfo = NULL;
    LW_SERVICE_HANDLE hDepHandle = NULL;
    PWSTR pwszDepName = NULL;
    size_t i = 0;

    dwError = LwSmGetServiceInfo(hHandle, &pInfo);
    BAIL_ON_ERROR(dwError);

    for (i = 0; pInfo->ppwszDependencies[i]; i++)
    {
        dwError = LwSmAcquireServiceHandle(pInfo->ppwszDependencies[i], &hDepHandle);
        BAIL_ON_ERROR(dwError);

        dwError = LwSmGetServiceDependencyClosureHelper(hDepHandle, pppwszServiceList);
        BAIL_ON_ERROR(dwError);

        if (!LwSmStringListContains(*pppwszServiceList, pInfo->ppwszDependencies[i]))
        {
            dwError = LwAllocateWc16String(&pwszDepName,  pInfo->ppwszDependencies[i]);
            BAIL_ON_ERROR(dwError);

            dwError = LwSmStringListAppend(pppwszServiceList, pwszDepName);
            BAIL_ON_ERROR(dwError);

            pwszDepName = NULL;
        }
        
        LwSmReleaseServiceHandle(hDepHandle);
        hDepHandle = NULL;
    }

cleanup:
    
    LW_SAFE_FREE_MEMORY(pwszDepName);
    
    if (pInfo)
    {
        LwSmCommonFreeServiceInfo(pInfo);
    }

    if (hDepHandle)
    {
        LwSmReleaseServiceHandle(hDepHandle);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
LwSmGetServiceDependencyClosure(
    LW_SERVICE_HANDLE hHandle,
    PWSTR** pppwszServiceList
    )
{
    DWORD dwError = 0;
    PWSTR* ppwszServiceList = NULL;

    dwError = LwAllocateMemory(sizeof(*ppwszServiceList) * 1, OUT_PPVOID(&ppwszServiceList));
    BAIL_ON_ERROR(dwError);

    dwError = LwSmGetServiceDependencyClosureHelper(hHandle, &ppwszServiceList);
    BAIL_ON_ERROR(dwError);

    *pppwszServiceList = ppwszServiceList;

cleanup:

    return dwError;

error:

    *pppwszServiceList = NULL;

    if (ppwszServiceList)
    {
        LwSmFreeStringList(ppwszServiceList);
    }

    goto cleanup;
}

static
DWORD
LwSmGetServiceReverseDependencyClosureHelper(
    LW_SERVICE_HANDLE hHandle,
    PWSTR* ppwszAllServices,
    PWSTR** pppwszServiceList
    )
{
    DWORD dwError = 0;
    PLW_SERVICE_INFO pInfo = NULL;
    PLW_SERVICE_INFO pDepInfo = NULL;
    size_t i = 0;
    LW_SERVICE_HANDLE hDepHandle = NULL;
    PWSTR pwszDepName = NULL;

    dwError = LwSmGetServiceInfo(hHandle, &pInfo);
    BAIL_ON_ERROR(dwError);

    for (i = 0; ppwszAllServices[i]; i++)
    {
        dwError = LwSmAcquireServiceHandle(ppwszAllServices[i], &hDepHandle);
        BAIL_ON_ERROR(dwError);

        dwError = LwSmGetServiceInfo(hDepHandle, &pDepInfo);
        BAIL_ON_ERROR(dwError);

        if (LwSmStringListContains(pDepInfo->ppwszDependencies, pInfo->pwszName))
        {
            dwError = LwSmGetServiceReverseDependencyClosureHelper(
                hDepHandle,
                ppwszAllServices,
                pppwszServiceList);
            BAIL_ON_ERROR(dwError);
            
            dwError = LwAllocateWc16String(&pwszDepName, pDepInfo->pwszName);
            BAIL_ON_ERROR(dwError);

            dwError = LwSmStringListAppend(pppwszServiceList, pwszDepName);
            BAIL_ON_ERROR(dwError);

            pwszDepName = NULL;
        }

        LwSmCommonFreeServiceInfo(pDepInfo);
        pDepInfo = NULL;

        dwError = LwSmReleaseServiceHandle(hDepHandle);
        hDepHandle = NULL;
        BAIL_ON_ERROR(dwError);
    }

cleanup:

    LW_SAFE_FREE_MEMORY(pwszDepName);

    if (pInfo)
    {
        LwSmCommonFreeServiceInfo(pInfo);
    }

    if (pDepInfo)
    {
        LwSmCommonFreeServiceInfo(pInfo);
    }

    if (hDepHandle)
    {
        LwSmReleaseServiceHandle(hDepHandle);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
LwSmGetServiceReverseDependencyClosure(
    LW_SERVICE_HANDLE hHandle,
    PWSTR** pppwszServiceList
    )
{
    DWORD dwError = 0;
    PWSTR* ppwszServiceList = NULL;
    PWSTR* ppwszAllServices = NULL;

    dwError = LwAllocateMemory(sizeof(*ppwszServiceList) * 1, OUT_PPVOID(&ppwszServiceList));
    BAIL_ON_ERROR(dwError);

    dwError = LwSmEnumerateServices(&ppwszAllServices);
    BAIL_ON_ERROR(dwError);

    dwError = LwSmGetServiceReverseDependencyClosureHelper(
        hHandle,
        ppwszAllServices,
        &ppwszServiceList);
    BAIL_ON_ERROR(dwError);

    *pppwszServiceList = ppwszServiceList;

cleanup:

    if (ppwszAllServices)
    {
        LwSmFreeStringList(ppwszAllServices);
    }

    return dwError;

error:

    *pppwszServiceList = NULL;

    if (ppwszServiceList)
    {
        LwSmFreeStringList(ppwszServiceList);
    }

    goto cleanup;
}
