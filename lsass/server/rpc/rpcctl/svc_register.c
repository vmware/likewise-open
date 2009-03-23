#include "includes.h"


static
DWORD
RpcSvcInitServerBinding(
    rpc_binding_vector_p_t *ppSrvBinding,
    PENDPOINT pEndPoints
    );


DWORD
RpcSvcRegisterRpcInterface(
    rpc_if_handle_t SrvInterface
    )
{
    DWORD dwError = 0;
    DWORD rpcstatus = rpc_s_ok;

    DCETHREAD_TRY
    {
        rpc_server_register_if(SrvInterface,
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

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
RpcSvcBindRpcInterface(
    rpc_binding_vector_p_t pSrvBinding,
    rpc_if_handle_t SrvInterface,
    PENDPOINT pEndPoints,
    PCSTR pszSrvDescription
    )
{
    DWORD dwError = 0;
    DWORD rpcstatus = rpc_s_ok;

    DCETHREAD_TRY
    {
        dwError = RpcSvcInitServerBinding(&pSrvBinding, pEndPoints);
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
        rpc_ep_register(SrvInterface,
                        pSrvBinding,
                        NULL,
                        pszSrvDescription,
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


static
DWORD
RpcSvcInitServerBinding(
    rpc_binding_vector_p_t *ppSrvBinding,
    PENDPOINT pEndPoints
    )
{
    DWORD dwError = 0;
    DWORD rpcstatus = rpc_s_ok;
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


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
