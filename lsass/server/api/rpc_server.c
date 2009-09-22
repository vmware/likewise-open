/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        rpc_server.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "api.h"


static
DWORD
LsaStartRpcSrv(
    PLSA_RPC_SERVER pRpc
    );


static
DWORD
LsaStopRpcSrv(
    PLSA_RPC_SERVER pRpc
    );


DWORD
LsaCheckInvalidRpcServer(
    PVOID pSymbol,
    PCSTR pszLibPath
    )
{
    DWORD dwError = 0;
    PCSTR pszError = NULL;

    if (pSymbol == NULL) {
        LSA_LOG_ERROR("Ignoring invalid rpc server at path [%s]",
                      (pszLibPath ? pszLibPath : "(unknown)"));

        pszError = dlerror();
        if (!LW_IS_NULL_OR_EMPTY_STR(pszError)) {
            LSA_LOG_ERROR("%s", pszError);
        }

        dwError = LW_ERROR_INVALID_RPC_SERVER;
        BAIL_ON_LSA_ERROR(dwError);
    }


cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
LsaInitRpcServer(
    PLSA_RPC_SERVER pRpc
    )
{
    DWORD dwError = 0;
    PFNINITIALIZERPCSRV pfnInitRpc = NULL;
    PCSTR pszError = NULL;
    PCSTR pszSrvLibPath = NULL;

    if (LW_IS_NULL_OR_EMPTY_STR(pRpc->pszSrvLibPath)) {
        dwError = ENOENT;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pszSrvLibPath = pRpc->pszSrvLibPath;

    dlerror();
    pRpc->phLib = dlopen(pszSrvLibPath, RTLD_NOW | RTLD_GLOBAL);
    if (pRpc->phLib == NULL) {
        LSA_LOG_ERROR("Failed to open rpc server at path [%s]", pszSrvLibPath);

        pszError = dlerror();
        if (!LW_IS_NULL_OR_EMPTY_STR(pszError)) {
            LSA_LOG_ERROR("%s", pszError);
        }

        dwError = LW_ERROR_INVALID_RPC_SERVER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dlerror();
    pfnInitRpc = (PFNINITIALIZERPCSRV)dlsym(
                                          pRpc->phLib,
                                          LSA_SYMBOL_NAME_INITIALIZE_RPCSRV);

    dwError = LsaCheckInvalidRpcServer(
                   pfnInitRpc,
                   pszSrvLibPath);
    BAIL_ON_LSA_ERROR(dwError);

    dlerror();
    pRpc->pfnShutdownSrv = (PFNSHUTDOWNRPCSRV)dlsym(
                                               pRpc->phLib,
                                               LSA_SYMBOL_NAME_SHUTDOWN_RPCSRV);

    dwError = LsaCheckInvalidRpcServer(
                   pRpc->pfnShutdownSrv,
                   pszSrvLibPath);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = pfnInitRpc(
                  &pRpc->pszName,
                  &pRpc->pfnTable);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaValidateRpcServer(pRpc);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    return dwError;

error:
    goto cleanup;
}



DWORD
LsaInitRpcServers(
    )
{
    DWORD dwError = 0;
    PLSA_RPC_SERVER pRpc = NULL;
    PLSA_RPC_SERVER pRpcList = NULL;
    PLSA_STACK pRpcSrvStack = NULL;
    BOOLEAN bLocked = TRUE;

    dwError = LsaRpcReadRegistry(&pRpcSrvStack);
    BAIL_ON_LSA_ERROR(dwError);

    pRpc = (PLSA_RPC_SERVER) LsaStackPop(&pRpcSrvStack);

    while (pRpc) {
        dwError = LsaInitRpcServer(pRpc);
        if (dwError)
        {
            LSA_LOG_ERROR("Failed to load rpc server [%s] at [%s] [error code:%d]",
                (pRpc->pszName ? pRpc->pszName : "<null>"),
                (pRpc->pszSrvLibPath ? pRpc->pszSrvLibPath : "<null>"),
                dwError);

            LsaFreeRpcServer(pRpc);
            pRpc = NULL;
            dwError = 0;
        }
        else
        {
            pRpc->pNext = pRpcList;
            pRpcList = pRpc;
        }
        pRpc = (PLSA_RPC_SERVER) LsaStackPop(&pRpcSrvStack);
    }

    ENTER_RPC_SERVER_LIST_WRITER_LOCK(bLocked);

    LsaFreeRpcServerList(gpRpcServerList);

    gpRpcServerList = pRpcList;
    pRpcList        = NULL;

    LsaStartRpcServers(gpRpcServerList);

    LEAVE_RPC_SERVER_LIST_WRITER_LOCK(bLocked);

    /* Start rpc service control worker thread to start listening
       for incoming rpc calls */
    dwError = RpcSvcStartWorker();
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    if (pRpcSrvStack) {
        LsaStackForeach(
            pRpcSrvStack,
            &LsaCfgFreeRpcServerInStack,
            NULL);

        LsaStackFree(pRpcSrvStack);
    }

    return dwError;

error:
    if (pRpcList) {
        LsaFreeRpcServerList(pRpcList);
    }

    goto cleanup;
}


void
LsaStartRpcServers(
    PLSA_RPC_SERVER pRpcServerList
    )
{
    DWORD dwError = 0;
    PLSA_RPC_SERVER pRpc = NULL;

    while (pRpcServerList) {
        pRpc = pRpcServerList;
        pRpcServerList = pRpcServerList->pNext;

        dwError = LsaStartRpcSrv(pRpc);
    }
}


static
DWORD
LsaStartRpcSrv(
    PLSA_RPC_SERVER pRpc
    )
{
    DWORD dwError = 0;

    dwError = pRpc->pfnTable->pfnStart();
    if (dwError) {
        LSA_LOG_ERROR("Couldn't start %s rpc server (error: %d)",
                      pRpc->pszName, dwError);

    } else {
        LSA_LOG_INFO("%s rpc server successfully started",
                     pRpc->pszName);
    }

    return dwError;
}


void
LsaStopRpcServers(
    PLSA_RPC_SERVER pRpcServerList
    )
{
    DWORD dwError = 0;
    PLSA_RPC_SERVER pRpc = NULL;

    while (pRpcServerList) {
        pRpc = pRpcServerList;
        pRpcServerList = pRpcServerList->pNext;

        dwError = LsaStopRpcSrv(pRpc);
    }
}


static
DWORD
LsaStopRpcSrv(
    PLSA_RPC_SERVER pRpc
    )
{
    DWORD dwError = 0;

    dwError = pRpc->pfnTable->pfnStop();
    if (dwError) {
        LSA_LOG_ERROR("Couldn't stop %s rpc server (error: %d)",
                      pRpc->pszName, dwError);

    } else {
        LSA_LOG_INFO("%s rpc server successfully stopped",
                     pRpc->pszName);
    }

    return dwError;
}


DWORD
LsaValidateRpcServer(
    PLSA_RPC_SERVER pRpc
    )
{
    DWORD dwError = 0;

    if (pRpc == NULL ||
        pRpc->pfnTable == NULL ||
        !pRpc->pfnTable->pfnStart ||
        !pRpc->pfnTable->pfnStop) {

        dwError = LW_ERROR_INVALID_RPC_SERVER;
    }

    return dwError;
}

static
DWORD
LsaRpcReadServer(
    PCSTR   pszServerName,
    PCSTR   pszServerKey,
    PLSA_STACK *ppRpcSrvStack
    )
{
    DWORD dwError = 0;

    PLSA_CONFIG_REG pReg = NULL;

    PLSA_RPC_SERVER pRpcSrv = NULL;

    PSTR pszPath = NULL;

    dwError = LsaOpenConfig(
                pszServerKey,
                pszServerKey,
                &pReg);
    BAIL_ON_LSA_ERROR(dwError);

    if (pReg == NULL)
    {
        goto error;
    }

    dwError = LsaReadConfigString(
                pReg,
                "Path",
                FALSE,
                &pszPath);
    BAIL_ON_LSA_ERROR(dwError);

    if(!LW_IS_NULL_OR_EMPTY_STR(pszPath))
    {
        dwError = LwAllocateMemory(
                    sizeof(LSA_RPC_SERVER),
                    (PVOID*)&pRpcSrv);
        BAIL_ON_LSA_ERROR(dwError);

        pRpcSrv->pszSrvLibPath = pszPath;
        pszPath = NULL;

        dwError = LwAllocateString(pszServerName, &pRpcSrv->pszName);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaStackPush(pRpcSrv, ppRpcSrvStack);
        BAIL_ON_LSA_ERROR(dwError);
        pRpcSrv = NULL;
    }

cleanup:

    LsaCloseConfig(pReg);
    pReg = NULL;

    return dwError;

error:

    LW_SAFE_FREE_STRING(pszPath);
    if (pRpcSrv)
    {
        LsaFreeRpcServer(pRpcSrv);
        pRpcSrv = NULL;
    }
    goto cleanup;
}

DWORD
LsaRpcReadRegistry(
    PLSA_STACK *ppRpcSrvStack
    )
{
    DWORD dwError = 0;

    PLSA_CONFIG_REG pReg = NULL;
    PSTR pszServers = NULL;
    PSTR pszServerKey = NULL;

    PSTR pszServer = NULL;
    PSTR pszTokenState = NULL;

    BAIL_ON_INVALID_POINTER(ppRpcSrvStack);

    dwError = LsaOpenConfig(
                "Services\\lsass\\Parameters\\RpcServers",
                "Policy\\Services\\lsass\\Parameters\\RpcServers",
                &pReg);
    BAIL_ON_LSA_ERROR(dwError);

    if (pReg == NULL)
    {
        goto error;
    }

    dwError = LsaReadConfigString(
                pReg,
                "Load",
                FALSE,
                &pszServers);
    BAIL_ON_LSA_ERROR(dwError);

    LsaCloseConfig(pReg);
    pReg = NULL;

    if (LW_IS_NULL_OR_EMPTY_STR(pszServers) )
    {
        goto error;
    }

    pszServer = strtok_r(pszServers, ",", &pszTokenState);
    while ( pszServer != NULL )
    {
        if(!LW_IS_NULL_OR_EMPTY_STR(pszServer))
        {
            dwError = LwAllocateStringPrintf(
                        &pszServerKey,
                        "Services\\lsass\\Parameters\\RpcServers\\%s",
                        pszServer);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaRpcReadServer(
                        pszServer,
                        pszServerKey,
                        ppRpcSrvStack);
            BAIL_ON_LSA_ERROR(dwError);
        }
        pszServer = strtok_r(NULL, ",", &pszTokenState);
    }

cleanup:
    LW_SAFE_FREE_STRING(pszServers);
    LW_SAFE_FREE_STRING(pszServerKey);

    LsaCloseConfig(pReg);
    pReg = NULL;

    return dwError;

error:
    goto cleanup;
}

void
LsaFreeRpcServer(
    PLSA_RPC_SERVER pSrv
    )
{
    if (pSrv == NULL) return;

    LW_SAFE_FREE_STRING(pSrv->pszName);

    if (pSrv->pfnShutdownSrv) {
        pSrv->pfnShutdownSrv(
                    pSrv->pszName,
                    pSrv->pfnTable);
    }

    if (pSrv->phLib) {
        dlclose(pSrv->phLib);
    }

    LW_SAFE_FREE_STRING(pSrv->pszSrvLibPath);

    LwFreeMemory(pSrv);
}


void
LsaFreeRpcServerList(
    PLSA_RPC_SERVER pRpcServerList
    )
{
    PLSA_RPC_SERVER pRpc = NULL;

    LsaStopRpcServers(pRpcServerList);

    while (pRpcServerList) {
        pRpc = pRpcServerList;
        pRpcServerList = pRpcServerList->pNext;
        LsaFreeRpcServer(pRpc);
        pRpc = NULL;
    }
}


DWORD
LsaCfgFreeRpcServerInStack(
    PVOID pItem,
    PVOID pUserData
    )
{
    DWORD dwError = 0;

    if (pItem) {
        LsaFreeRpcServer((PLSA_RPC_SERVER)pItem);
    }

    return dwError;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
