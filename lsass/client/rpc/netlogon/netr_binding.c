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
 *        netr_binding.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        DCE/RPC binding functions
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


static
NTSTATUS
NetrInitBindingDefaultA(
    OUT PNETR_BINDING   phBinding,
    IN  PCSTR           pszHostname,
    IN  LW_PIO_CREDS    pCreds
    );


static
NTSTATUS
NetrInitBindingFullA(
    OUT PNETR_BINDING   phBinding,
    IN  PCSTR           pszProtSeq,
    IN  PCSTR           pszHostname,
    IN  PCSTR           pszEndpoint,
    IN  PCSTR           pszUuid,
    IN  PCSTR           pszOptions,
    IN  LW_PIO_CREDS    pCreds
    );


NTSTATUS
NetrInitBindingDefault(
    OUT PNETR_BINDING   phBinding,
    IN  PCWSTR          pwszHostname,
    IN  LW_PIO_CREDS    pCreds
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PSTR pszHostname = NULL;

    if (pwszHostname)
    {
        dwError = LwWc16sToMbs(pwszHostname, &pszHostname);
        BAIL_ON_WIN_ERROR(dwError);
    }

    ntStatus = NetrInitBindingDefaultA(phBinding,
                                       pszHostname,
                                       pCreds);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:
    LW_SAFE_FREE_MEMORY(pszHostname);

    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
NetrInitBindingDefaultA(
    OUT PNETR_BINDING  phBinding,
    IN  PCSTR          pszHostname,
    IN  LW_PIO_CREDS   pCreds
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSTR pszProtSeq = (PSTR)NETLOGON_DEFAULT_PROT_SEQ;
    PSTR pszLpcProtSeq = (PSTR)"ncalrpc";
    PSTR pszEndpoint = NETLOGON_DEFAULT_ENDPOINT;
    PSTR pszLpcEndpoint = (PSTR)NETLOGON_LOCAL_ENDPOINT;
    PSTR pszUuid = NULL;
    PSTR pszOptions = NULL;
    NETR_BINDING hBinding = NULL;

    ntStatus = NetrInitBindingFullA(
                      &hBinding,
                      (pszHostname) ? pszProtSeq : pszLpcProtSeq,
                      pszHostname,
                      (pszHostname) ? pszEndpoint : pszLpcEndpoint,
                      pszUuid,
                      pszOptions,
                      pCreds);
    BAIL_ON_NT_STATUS(ntStatus);

    *phBinding = hBinding;

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


NTSTATUS
NetrInitBindingFull(
    OUT PNETR_BINDING   phBinding,
    IN  PCWSTR          pwszProtSeq,
    IN  PCWSTR          pwszHostname,
    IN  PCWSTR          pwszEndpoint,
    IN  PCWSTR          pwszUuid,
    IN  PCWSTR          pwszOptions,
    IN  LW_PIO_CREDS    pCreds
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PSTR pszProtSeq = NULL;
    PSTR pszHostname = NULL;
    PSTR pszEndpoint = NULL;
    PSTR pszUuid = NULL;
    PSTR pszOptions = NULL;
    NETR_BINDING hBinding = NULL;

    dwError = LwWc16sToMbs(pwszProtSeq, &pszProtSeq);
    BAIL_ON_WIN_ERROR(dwError);

    if (pwszHostname)
    {
        dwError = LwWc16sToMbs(pwszHostname, &pszHostname);
        BAIL_ON_WIN_ERROR(dwError);
    }

    dwError = LwWc16sToMbs(pwszEndpoint, &pszEndpoint);
    BAIL_ON_WIN_ERROR(dwError);

    dwError = LwWc16sToMbs(pwszUuid, &pszUuid);
    BAIL_ON_WIN_ERROR(dwError);

    dwError = LwWc16sToMbs(pwszOptions, &pszOptions);
    BAIL_ON_WIN_ERROR(dwError);

    ntStatus = NetrInitBindingFullA(&hBinding,
                                    pszProtSeq,
                                    pszHostname,
                                    pszEndpoint,
                                    pszUuid,
                                    pszOptions,
                                    pCreds);
    BAIL_ON_NT_STATUS(ntStatus);

    *phBinding = hBinding;

cleanup:
    LW_SAFE_FREE_MEMORY(pszProtSeq);
    LW_SAFE_FREE_MEMORY(pszHostname);
    LW_SAFE_FREE_MEMORY(pszEndpoint);
    LW_SAFE_FREE_MEMORY(pszUuid);
    LW_SAFE_FREE_MEMORY(pszOptions);

    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    *phBinding = NULL;

    goto cleanup;
}


static
NTSTATUS
NetrInitBindingFullA(
    OUT PNETR_BINDING   phBinding,
    IN  PCSTR           pszProtSeq,
    IN  PCSTR           pszHostname,
    IN  PCSTR           pszEndpoint,
    IN  PCSTR           pszUuid,
    IN  PCSTR           pszOptions,
    IN  LW_PIO_CREDS    pCreds
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    unsigned32 rpcStatus = RPC_S_OK;
    unsigned32 rpcRetStatus = RPC_S_OK;
    unsigned char *binding_string = NULL;
    unsigned char *prot_seq       = NULL;
    unsigned char *endpoint       = NULL;
    unsigned char *uuid           = NULL;
    unsigned char *options        = NULL;
    unsigned char *address        = NULL;
    handle_t hBinding = NULL;
    rpc_transport_info_handle_t hInfo = NULL;

    BAIL_ON_INVALID_PTR(phBinding, ntStatus);
    BAIL_ON_INVALID_PTR(pszHostname, ntStatus);
    BAIL_ON_INVALID_PTR(pszProtSeq, ntStatus);

    prot_seq = (unsigned char*) strdup(pszProtSeq);
    BAIL_ON_NULL_PTR(prot_seq, ntStatus);

    if (pszEndpoint != NULL)
    {
        endpoint = (unsigned char*) strdup(pszEndpoint);
        BAIL_ON_NULL_PTR(endpoint, ntStatus);
    }

    if (pszUuid != NULL)
    {
        uuid = (unsigned char*) strdup(pszUuid);
        BAIL_ON_NULL_PTR(uuid, ntStatus);
    }

    if (pszOptions != NULL)
    {
        options = (unsigned char*) strdup(pszOptions);
        BAIL_ON_NULL_PTR(options, ntStatus);
    }

    if (pszHostname)
    {
        address = (unsigned char*) strdup(pszHostname);
        BAIL_ON_NULL_PTR(address, ntStatus);
    }

    rpc_string_binding_compose(
        uuid,
        prot_seq,
        address,
        endpoint,
        options,
        &binding_string,
        &rpcStatus);
    BAIL_ON_RPC_STATUS(rpcStatus);

    rpc_binding_from_string_binding(
        binding_string,
        &hBinding,
        &rpcStatus);
    BAIL_ON_RPC_STATUS(rpcStatus);

    if (strcmp(pszProtSeq, "ncacn_np") == 0)
    {
        rpc_smb_transport_info_from_lwio_creds(
            pCreds,
            &hInfo,
            &rpcStatus);
        BAIL_ON_RPC_STATUS(rpcStatus);

        rpc_binding_set_transport_info(
            hBinding,
            hInfo,
            &rpcStatus);
        BAIL_ON_RPC_STATUS(rpcStatus);

        hInfo = NULL;
    }

    rpc_mgmt_set_com_timeout(hBinding, 6, &rpcStatus);
    BAIL_ON_RPC_STATUS(rpcStatus);

    *phBinding = (NETR_BINDING)hBinding;

cleanup:
    LW_SAFE_FREE_MEMORY(prot_seq);
    LW_SAFE_FREE_MEMORY(endpoint);
    LW_SAFE_FREE_MEMORY(uuid);
    LW_SAFE_FREE_MEMORY(options);
    LW_SAFE_FREE_MEMORY(address);

    if (binding_string)
    {
        rpc_string_free(&binding_string, &rpcRetStatus);
    }

    if (hInfo)
    {
        rpc_smb_transport_info_free(hInfo);
    }

    if (rpcStatus == RPC_S_OK &&
        rpcRetStatus != RPC_S_OK)
    {
        rpcStatus = rpcRetStatus;
    }

    if (ntStatus == STATUS_SUCCESS &&
        rpcStatus != RPC_S_OK)
    {
        ntStatus = LwRpcStatusToNtStatus(rpcStatus);
    }

    return ntStatus;

error:
    if (hBinding)
    {
        rpc_binding_free(&hBinding, &rpcRetStatus);
    }

    *phBinding = NULL;

    goto cleanup;
}


VOID
NetrFreeBinding(
    IN OUT  PNETR_BINDING  phBinding
    )
{
    unsigned32 rpcStatus = RPC_S_OK;

    /* Free the binding itself */
    if (phBinding && *phBinding)
    {
        rpc_binding_free((handle_t*)phBinding, &rpcStatus);
        BAIL_ON_RPC_STATUS(rpcStatus);
    }

    *phBinding = NULL;

cleanup:
    return;

error:
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
