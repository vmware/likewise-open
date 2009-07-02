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
 * Abstract: Samr interface binding (rpc client library)
 *
 * Authors: Rafal Szczesniak (rafal@likewisesoftware.com)
 */

#include "includes.h"

#define BAIL_ON_NO_MEMORY_RPCSTATUS(p)     \
    if ((p) == NULL) {                     \
        rpcstatus = RPC_S_OUT_OF_MEMORY;   \
        goto error;                        \
    }

#define BAIL_ON_INVALID_PTR_RPCSTATUS(p)   \
    if ((p) == NULL) {                     \
        rpcstatus = RPC_S_INVALID_ARG;     \
        goto error;                        \
    }


RPCSTATUS
InitSamrBindingDefault(
    handle_t *binding,
    const char *hostname,
    PIO_ACCESS_TOKEN access_token
    )
{
    RPCSTATUS rpcstatus = RPC_S_OK;
    char *prot_seq = (char*)SAMR_DEFAULT_PROT_SEQ;
    char *endpoint = (char*)SAMR_DEFAULT_ENDPOINT;
    char *uuid = NULL;
    char *options = NULL;
    handle_t b = NULL;

    rpcstatus = InitSamrBindingFull(&b, prot_seq, hostname, endpoint,
                                    uuid, options, access_token);
    BAIL_ON_RPCSTATUS_ERROR(rpcstatus);

    *binding = b;

cleanup:
    return rpcstatus;

error:
    *binding = NULL;
    goto cleanup;
}


RPCSTATUS
InitSamrBindingFull(
    handle_t *binding,
    const char *prot_seq,
    const char *hostname,
    const char *endpoint,
    const char *uuid,
    const char *options,
    PIO_ACCESS_TOKEN access_token
    )
{
    RPCSTATUS rpcstatus = RPC_S_OK;
    RPCSTATUS st = RPC_S_OK;
    unsigned char *binding_string = NULL;
    unsigned char *ps   = NULL;
    unsigned char *ep   = NULL;
    unsigned char *u    = NULL;
    unsigned char *opts = NULL;
    unsigned char *addr = NULL;
    handle_t b = NULL;
    rpc_transport_info_handle_t info;

    BAIL_ON_INVALID_PTR_RPCSTATUS(binding);
    BAIL_ON_INVALID_PTR_RPCSTATUS(prot_seq);

    ps = (unsigned char*) strdup(prot_seq);
    BAIL_ON_NO_MEMORY_RPCSTATUS(ps);

    if (endpoint != NULL) {
        ep = (unsigned char*) strdup(endpoint);
        BAIL_ON_NO_MEMORY_RPCSTATUS(ep);
    }

    if (uuid != NULL) {
        u = (unsigned char*) strdup(uuid);
        BAIL_ON_NO_MEMORY_RPCSTATUS(u);
    }

    if (options != NULL) {
        opts = (unsigned char*) strdup(options);
        BAIL_ON_NO_MEMORY_RPCSTATUS(opts);
    }

    if (hostname != NULL) {
        addr = (unsigned char*) strdup(hostname);
        BAIL_ON_NO_MEMORY_RPCSTATUS(addr);
    }

    rpc_string_binding_compose(u, ps, addr, ep, opts, &binding_string,
                               &rpcstatus);
    BAIL_ON_RPCSTATUS_ERROR(rpcstatus);

    rpc_binding_from_string_binding(binding_string, &b, &rpcstatus);
    BAIL_ON_RPCSTATUS_ERROR(rpcstatus);

    rpc_smb_transport_info_from_lwio_token(access_token, FALSE, &info, &rpcstatus);
    BAIL_ON_RPCSTATUS_ERROR(rpcstatus);

    rpc_binding_set_transport_info(b, info, &rpcstatus);
    BAIL_ON_RPCSTATUS_ERROR(rpcstatus);

	info = NULL;

    rpc_mgmt_set_com_timeout(b, 6, &rpcstatus);
    BAIL_ON_RPCSTATUS_ERROR(rpcstatus);

    *binding = b;

cleanup:
    SAFE_FREE(ps);
    SAFE_FREE(ep);
    SAFE_FREE(u);
    SAFE_FREE(opts);
    SAFE_FREE(addr);

    if (binding_string) {
        rpc_string_free(&binding_string, &st);
    }

    if (rpcstatus == RPC_S_OK &&
        st != RPC_S_OK) {
        rpcstatus = st;
    }

    if (info)
    {
        rpc_smb_transport_info_free(info);
    }

    return rpcstatus;

error:
    if (b) {
        rpc_binding_free(&b, &st);
    }

    goto cleanup;
}


RPCSTATUS
FreeSamrBinding(
    handle_t *binding
    )
{
    RPCSTATUS rpcstatus = RPC_S_OK;

    /* Free the binding itself */
    if (binding && *binding) {
        rpc_binding_free(binding, &rpcstatus);
        BAIL_ON_RPCSTATUS_ERROR(rpcstatus);
    }

cleanup:
    return rpcstatus;

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
