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
 * Abstract: Lsa interface binding (rpc client library)
 *
 * Authors: Rafal Szczesniak (rafal@likewisesoftware.com)
 */

#include "includes.h"


RPCSTATUS
InitLsaBindingDefault(
    OUT handle_t *phBinding,
    IN  PCSTR pszHostname,
    IN  PIO_ACCESS_TOKEN pIoAccessToken
    )
{
    RPCSTATUS rpcStatus = RPC_S_OK;
    PSTR pszProtSeq = (PSTR)LSA_DEFAULT_PROT_SEQ;
    PSTR pszEndpoint = (PSTR)LSA_DEFAULT_ENDPOINT;
    PSTR pszUuid = NULL;
    PSTR pszOptions = NULL;
    handle_t hBinding = NULL;

    rpcStatus = InitLsaBindingFull(
                    &hBinding,
                    pszProtSeq,
                    pszHostname,
                    pszEndpoint,
                    pszUuid,
                    pszOptions,
                    pIoAccessToken);
    BAIL_ON_RPCSTATUS_ERROR(rpcStatus);

    *phBinding = hBinding;

cleanup:
    return rpcStatus;

error:
    *phBinding = NULL;
    goto cleanup;
}


RPCSTATUS
InitLsaBindingFull(
    handle_t *phBinding,
    PCSTR pszProtSeq,
    PCSTR pszHostname,
    PCSTR pszEndpoint,
    PCSTR pszUuid,
    PCSTR pszOptions,
    PIO_ACCESS_TOKEN IoAccessToken
    )
{
    RPCSTATUS rpcStatus = RPC_S_OK;
    RPCSTATUS rpcStatus2 = RPC_S_OK;
    PBYTE pbBindingString = NULL;
    PBYTE pbProtSeq = NULL;
    PBYTE pbEndpoint = NULL;
    PBYTE pbUuid = NULL;
    PBYTE pbOpts = NULL;
    PBYTE pbAddr = NULL;
    handle_t hBinding = NULL;
    rpc_transport_info_handle_t Info = NULL;

    BAIL_ON_INVALID_PTR_RPCSTATUS(phBinding, rpcStatus);
    BAIL_ON_INVALID_PTR_RPCSTATUS(pszHostname, rpcStatus);
    BAIL_ON_INVALID_PTR_RPCSTATUS(pszProtSeq, rpcStatus);

    pbProtSeq = (PBYTE)strdup(pszProtSeq);
    BAIL_ON_NO_MEMORY_RPCSTATUS(pbProtSeq, rpcStatus);

    if (pszEndpoint != NULL) {
        pbEndpoint = (PBYTE) strdup(pszEndpoint);
        BAIL_ON_NO_MEMORY_RPCSTATUS(pbEndpoint, rpcStatus);
    }

    if (pszUuid != NULL) {
        pbUuid = (PBYTE)strdup(pszUuid);
        BAIL_ON_NO_MEMORY_RPCSTATUS(pbUuid, rpcStatus);
    }

    if (pszOptions != NULL) {
        pbOpts = (PBYTE)strdup(pszOptions);
        BAIL_ON_NO_MEMORY_RPCSTATUS(pbOpts, rpcStatus);
    }

    pbAddr = (PBYTE)strdup(pszHostname);
    BAIL_ON_NO_MEMORY_RPCSTATUS(pbAddr, rpcStatus);

    rpc_string_binding_compose(
        pbUuid,
        pbProtSeq,
        pbAddr,
        pbEndpoint,
        pbOpts,
        &pbBindingString,
        &rpcStatus);
    BAIL_ON_RPCSTATUS_ERROR(rpcStatus);

    rpc_binding_from_string_binding(
        pbBindingString,
        &hBinding,
        &rpcStatus);
    BAIL_ON_RPCSTATUS_ERROR(rpcStatus);

    rpc_smb_transport_info_from_lwio_token(
        IoAccessToken,
        FALSE,
        &Info,
        &rpcStatus);
    BAIL_ON_RPCSTATUS_ERROR(rpcStatus);

    rpc_binding_set_transport_info(
        hBinding,
        Info,
        &rpcStatus);
    BAIL_ON_RPCSTATUS_ERROR(rpcStatus);

    Info = NULL;

    rpc_mgmt_set_com_timeout(hBinding, 6, &rpcStatus);
    BAIL_ON_RPCSTATUS_ERROR(rpcStatus);
    
    *phBinding = hBinding;

cleanup:
    SAFE_FREE(pbProtSeq);
    SAFE_FREE(pbEndpoint);
    SAFE_FREE(pbUuid);
    SAFE_FREE(pbOpts);
    SAFE_FREE(pbAddr);

    if (pbBindingString)
    {
        rpc_string_free(&pbBindingString, &rpcStatus2);
    }

    if ((rpcStatus == RPC_S_OK) && (rpcStatus2 != RPC_S_OK))
    {
        rpcStatus = rpcStatus2;
    }

    if (Info)
    {
        rpc_smb_transport_info_free(Info);
    }

    return rpcStatus;

error:
    if (hBinding)
    {
        rpc_binding_free(&hBinding, &rpcStatus2);
    }

    goto cleanup;
}


RPCSTATUS
FreeLsaBinding(
    handle_t *phBinding
    )
{
    RPCSTATUS rpcStatus = RPC_S_OK;

    /* Free the binding itself */

    if (phBinding && *phBinding)
    {
	    rpc_binding_free(phBinding, &rpcStatus);
        BAIL_ON_RPCSTATUS_ERROR(rpcStatus);
    }

cleanup:
    return rpcStatus;

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
