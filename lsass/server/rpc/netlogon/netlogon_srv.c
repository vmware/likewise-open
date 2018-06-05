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
 *        netlogon_srv.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        Netlogon rpc server module initialisation and shutdown
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 *          Adam Bernstein (abernstein@vmware.com)
 */


#include "includes.h"


DWORD
LsaInitializeRpcSrv(
    PSTR* ppszRpcSrvName,
    PLSA_RPCSRV_FUNCTION_TABLE* ppFnTable
    )
{
    DWORD dwError = 0;

    pthread_mutex_init(&gNetlogonSrvDataMutex, NULL);

    dwError = RpcSvcRegisterRpcInterface(netlogon_v1_0_s_ifspec);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszRpcSrvName = (PSTR)gpszNetlogonRpcSrvName;
    *ppFnTable      = &gNetlogonRpcFuncTable;

    dwError = NetlogonSrvInitialiseConfig(&gNetlogonSrvConfig);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = NetlogonSrvReadRegistry(&gNetlogonSrvConfig);
    BAIL_ON_LSA_ERROR(dwError);

    bNetlogonSrvInitialised = TRUE;

cleanup:
    return dwError;

error:
    goto cleanup;
}


DWORD
LsaShutdownRpcSrv(
    PCSTR pszProviderName,
    PLSA_RPCSRV_FUNCTION_TABLE pFnTable
    )
{
    DWORD dwError = 0;

    dwError = RpcSvcUnregisterRpcInterface(netlogon_v1_0_s_ifspec);
    BAIL_ON_LSA_ERROR(dwError);

#if 0 /* TBD:Adam */
    dwError = NetlogonSrvDestroyServerSecurityDescriptor(&gpNetlogonSecDesc);
    BAIL_ON_LSA_ERROR(dwError);
#endif

    pthread_mutex_destroy(&gNetlogonSrvDataMutex);

    bNetlogonSrvInitialised = FALSE;

error:
    return dwError;
}


DWORD
NetlogonRpcStartServer(
    void
    )
{
    PCSTR pszDescription = "Security Accounts Manager";
    ENDPOINT EndPoints[] = {
        { "ncacn_np",      "\\\\pipe\\\\netlogon" },
        { "ncalrpc",       NULL },  /* endpoint is fetched from config parameter */
        { NULL,            NULL },
        { NULL,            NULL }
    };

    DWORD dwError = 0;
    DWORD i = 0;
    PSTR pszLpcSocketPath = NULL;
    BOOLEAN bRegisterTcpIp = FALSE;
    dwError = NetlogonGetBindProtocol(&gNetlogonGlobals.bindProtocol);
    BAIL_ON_NETLOGON_LDAP_ERROR(dwError);

#if 1 /* For now, force SRP, as this will always work, and do not need to get a KRBTGT  */
/* TBD:Adam "GSSAPI" is hard-disabled for now; figure this out once SSF is disabled */
    gNetlogonGlobals.bindProtocol = NETLOGON_LDAP_BIND_PROTOCOL_SRP;
#endif

    dwError = NetlogonSrvConfigGetLpcSocketPath(&pszLpcSocketPath);
    BAIL_ON_LSA_ERROR(dwError);

    while (EndPoints[i].pszProtocol) {
        if (strcmp(EndPoints[i].pszProtocol, "ncalrpc") == 0 &&
            pszLpcSocketPath) {
            EndPoints[i].pszEndpoint = pszLpcSocketPath;
        }

        i++;
    }

    dwError = NetlogonSrvConfigShouldRegisterTcpIp(&bRegisterTcpIp);
    BAIL_ON_LSA_ERROR(dwError);
    if (bRegisterTcpIp)
    {
        EndPoints[i++].pszProtocol = "ncacn_ip_tcp";
    }

    dwError = RpcSvcBindRpcInterface(&gpNetlogonSrvBinding,
                                     netlogon_v1_0_s_ifspec,
                                     EndPoints,
                                     pszDescription);
    BAIL_ON_LSA_ERROR(dwError);

error:
    LW_SAFE_FREE_STRING(pszLpcSocketPath);

    return dwError;
}


DWORD
NetlogonRpcStopServer(
    void
    )
{
    DWORD dwError = 0;

    dwError = RpcSvcUnbindRpcInterface(&gpNetlogonSrvBinding,
                                       netlogon_v1_0_s_ifspec);
    BAIL_ON_LSA_ERROR(dwError);

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
