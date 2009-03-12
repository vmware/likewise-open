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
 * Abstract: Samr rpc server management functions (rpc server library)
 *
 * Authors: Rafal Szczesniak (rafal@likewisesoftware.com)
 */

#include "includes.h"


DWORD
SamrRpcBindInterface(
    rpc_binding_vector_p_t *ppSrvBinding,
    PENDPOINT pEndPoints
    )
{
    DWORD dwError = 0;
    RPCSTATUS rpcstatus = 0;
    DWORD i = 0;

    for (i = 0; pEndPoints[i].pszProtocol != NULL; i++)
    {
        if (!pEndPoints[i].pszEndpoint)
        {
            rpc_server_use_protseq((unsigned char*) pEndPoints[i].pszProtocol,
                                   rpc_c_protseq_max_calls_default,
                                   (unsigned32*)&rpcstatus);
            BAIL_ON_DCERPC_ERROR(rpcstatus);
        }
        else
        {
            rpc_server_use_protseq_ep((unsigned char*)pEndPoints[i].pszProtocol,
                                      rpc_c_protseq_max_calls_default,
                                      (unsigned char*)pEndPoints[i].pszEndpoint,
                                      (unsigned32*)&rpcstatus);
            BAIL_ON_DCERPC_ERROR(rpcstatus);
        }
    }

    rpc_server_inq_bindings(ppSrvBinding, (unsigned32*)&rpcstatus);
    BAIL_ON_DCERPC_ERROR(rpcstatus);

error:

    return dwError;
}


DWORD
SamrRpcRegisterRpcInterface(
    rpc_binding_vector_p_t pSrvBinding
    )
{
    const ENDPOINT endpoints[] = {
        { "ncacn_np", "\\\\pipe\\\\samr" },
        { "ncacn_ip_tcp", NULL }
    };

    DWORD dwError = 0;
    RPCSTATUS rpcstatus = rpc_s_ok;
    int i = 0;

    DCETHREAD_TRY
    {
        rpc_server_register_if(samr_v1_0_s_ifspec,
                               NULL,
                               NULL,
                               &rpcstatus);
    }
    DCETHREAD_CATCH_ALL(THIS_CATCH)
    {
        if (!rpcstatus) {
            rpcstatus = dcethread_exc_getstatus(THIS_CATCH);
        }

        if (!rpcstatus) {
                dwError = LSA_ERROR_RPC_SERVER_REGISTRATION_ERROR;
        }
    }
    DCETHREAD_ENDTRY;

    BAIL_ON_DCERPC_ERROR(rpcstatus);
    BAIL_ON_LSA_ERROR(dwError);

    DCETHREAD_TRY
    {
        dwError = SamrRpcBindInterface(&pSrvBinding, (PENDPOINT)endpoints);
    }
    DCETHREAD_CATCH_ALL(THIS_CATCH)
    {
        if (!dwError) {
            rpcstatus = dcethread_exc_getstatus(THIS_CATCH);
        }

        if (!rpcstatus) {
            dwError = LSA_ERROR_RPC_SERVER_REGISTRATION_ERROR;
        }
    }
    DCETHREAD_ENDTRY;

    BAIL_ON_DCERPC_ERROR(rpcstatus);
    BAIL_ON_LSA_ERROR(dwError);

    DCETHREAD_TRY
    {
        rpc_ep_register(samr_v1_0_s_ifspec,
                        pSrvBinding,
                        NULL,
                        "",
                        &rpcstatus);
    }
    DCETHREAD_CATCH_ALL(THIS_CATCH)
    {
        if (!dwError) {
            rpcstatus = dcethread_exc_getstatus(THIS_CATCH);
        }

        if (!rpcstatus) {
            dwError = LSA_ERROR_RPC_SERVER_REGISTRATION_ERROR;
        }
    }
    DCETHREAD_ENDTRY;

    BAIL_ON_DCERPC_ERROR(rpcstatus);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
SamrRpcUnregisterRpcInterface(
    rpc_binding_vector_p_t pSrvBinding
    )
{
    DWORD dwError = 0;
    RPCSTATUS rpcstatus = rpc_s_ok;

    if (pSrvBinding) {
        rpc_ep_unregister(samr_v1_0_s_ifspec,
                          pSrvBinding,
                          NULL,
                          &rpcstatus);
        BAIL_ON_DCERPC_ERROR(rpcstatus);

        rpc_binding_vector_free(&pSrvBinding, &rpcstatus);
        pSrvBinding = NULL;

        BAIL_ON_DCERPC_ERROR(rpcstatus);
    }

    rpc_server_unregister_if(samr_v1_0_s_ifspec,
                             NULL,
                             &rpcstatus);
    BAIL_ON_DCERPC_ERROR(rpcstatus);

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
SamrRpcStartWorker(
    void
    )
{
    DWORD dwError = 0;
    int ret = 0;

    ret = pthread_create(&gWorker.worker,
                         NULL,
                         SamrRpcWorkerMain,
                         (void*)&gWorker.context);
    if (ret) {
        dwError = LSA_ERROR_INVALID_RPC_SERVER;
        goto error;
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}


void*
SamrRpcWorkerMain(
    void* pCtx
    )
{
    DWORD dwError = 0;
    RPCSTATUS rpcstatus = rpc_s_ok;

    DCETHREAD_TRY
    {
        rpc_server_listen(rpc_c_listen_max_calls_default, &rpcstatus);
    }
    DCETHREAD_CATCH_ALL(THIS_CATCH)
    {
        if (!rpcstatus) {
            rpcstatus = dcethread_exc_getstatus(THIS_CATCH);
        }

        if (!rpcstatus) {
                dwError = LSA_ERROR_RPC_SERVER_RUNTIME_ERROR;
        }
    }
    DCETHREAD_ENDTRY
}


DWORD
SamrRpcStartServer(
    void
    )
{
    DWORD dwError = 0;
    RPCSTATUS rpcstatus = rpc_s_ok;

    dwError = SamrRpcRegisterRpcInterface(gpSamrSrvBinding);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = SamrRpcStartWorker();
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
SamrRpcStopServer(
    void
    )
{
    DWORD dwError = 0;

    dwError = SamrRpcUnregisterRpcInterface(gpSamrSrvBinding);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
SamrInitializeRpcSrv(
    PCSTR pszConfigFilePath,
    PSTR* ppszRpcSrvName,
    PLSA_RPCSRV_FUNCTION_TABLE* ppFnTable
    )
{
    DWORD dwError = 0;

    pthread_mutex_init(&gSamrDataMutex, NULL);

    dwError = SamrRpcStartServer();
    BAIL_ON_LSA_ERROR(dwError);

    *ppszRpcSrvName = (PSTR)gpszRpcSrvName;
    *ppFnTable      = &gSamrRpcFuncTable;

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
SamrShutdownRpcSrv(
    PCSTR pszProviderName,
    PLSA_RPCSRV_FUNCTION_TABLE pFnTable
    )
{
    DWORD dwError = 0;
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
