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
 * Abstract: Lsa rpc server management functions (rpc server library)
 *
 * Authors: Rafal Szczesniak (rafal@likewisesoftware.com)
 */

#include "includes.h"


DWORD
LsaRpcRegisterRpcInterface(
    rpc_binding_vector_p_t pSrvBinding
    )
{
    const PSTR pszProtSeq[] = { "ncacn_ip_tcp", NULL };

    DWORD dwError = 0;
    RPCSTATUS rpcstatus = rpc_s_ok;
    int i = 0;

    rpc_server_register_if(lsa_v0_0_s_ifspec,
                           NULL,
                           NULL,
                           &rpcstatus);
    BAIL_ON_DCERPC_ERROR(rpcstatus);

    while (pszProtSeq[i] != NULL) {
        rpc_server_use_protseq_if((unsigned char)pszProtSeq[i++],
                                  rpc_c_protseq_max_calls_default,
                                  lsa_v0_0_s_ifspec,
                                  &rpcstatus);
        BAIL_ON_DCERPC_ERROR(rpcstatus);
    }

    rpc_server_inq_bindings(&pSrvBinding, &rpcstatus);
    BAIL_ON_DCERPC_ERROR(rpcstatus);

    rpc_ep_register_no_replace(lsa_v0_0_s_ifspec,
                               pSrvBinding,
                               NULL,
                               "",
                               &rpcstatus);
    BAIL_ON_DCERPC_ERROR(rpcstatus);

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
LsaRpcUnregisteRpcInterface(
    rpc_binding_vector_p_t pSrvBinding
    )
{
    DWORD dwError = 0;
    RPCSTATUS rpcstatus = rpc_s_ok;

    if (pSrvBinding) {
        rpc_ep_unregister(lsa_v0_0_s_ifspec,
                          pSrvBinding,
                          NULL,
                          &rpcstatus);
        BAIL_ON_DCERPC_ERROR(rpcstatus);

        rpc_binding_vector_free(&pSrvBinding, &rpcstatus);
        pSrvBinding = NULL;

        BAIL_ON_DCERPC_ERROR(rpcstatus);
    }

    rpc_server_unregister_if(lsa_v0_0_s_ifspec,
                             NULL,
                             &rpcstatus);
    BAIL_ON_DCERPC_ERROR(rpcstatus);

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD LsaRpcStartServer(void)
{
    DWORD dwError = 0;

    dwError = LsaRpcRegisterRpcInterface(&gpLsaSrvBinding);
    BAIL_ON_LSA_ERRROR(dwError);

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD LsaRpcStopServer(void)
{
    DWORD dwError = 0;

    dwError = LsaRpcUnregisterRpcInterface(&gpLsaSrvBinding);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD LsaInitializeRpcSrv(
    PCSTR pszConfigFilePath,
    PSTR* ppszRpcSrvName,
    PLSA_RPCSRV_FUNCTION_TABLE* ppFnTable
    )
{
    DWORD dwError = 0;
    return dwError;
}


DWORD LsaShutdownRpcSrv(
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
