/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#ifdef __GNUC__
#include <dce/rpc.h>
#elif _WIN32
#include <rpc.h>
#endif

#include <compat/dcerpc.h>
#include <compat/rpcstatus.h>

#include <wc16str.h>
#include <srvsvcbinding.h>
//#include <lwrpc/unicodestring.h>
//#include <lwrpc/allocate.h>

#include "SrvSvcUtil.h"

RPCSTATUS InitSrvSvcBindingDefault(handle_t *binding, const CHAR_T *hostname)
{
    RPCSTATUS rpcstatus = RPC_S_OK;
    CHAR_T *binding_string = NULL;
    CHAR_T *prot_seq = SRVSVC_DEFAULT_PROT_SEQ;
    CHAR_T *endpoint = SRVSVC_DEFAULT_ENDPOINT;
    CHAR_T *hostname_prefix = TEXT("\\\\");
    CHAR_T *uuid = NULL;
    CHAR_T *options = NULL;
    CHAR_T *address = NULL;

    /* we can't create binding without a proper rpc server address */
    if (hostname == NULL || binding == NULL) {
        return RPC_S_INVALID_NET_ADDR;
    }

#ifdef _WIN32
    if (wcsstr(hostname, hostname_prefix) == hostname) {
        address = wcsdup(hostname);
    } else {
        size_t len = wcslen(hostname) + 3; /* add space for "\\"
					      and termination */
        size_t size = sizeof(char_t) * len;
        address = (CHAR_T*) malloc(size);
        _snwprintf_s(address, size, size, TEXT("\\\\%s"), hostname);
    }
#elif __GNUC__
    address = strdup(hostname);
#endif

    rpc_string_binding_compose(uuid, prot_seq, address, endpoint,
                               options, &binding_string, &rpcstatus);
    goto_if_rpcstatus_not_success(rpcstatus, done);

    rpc_binding_from_string_binding(binding_string, binding, &rpcstatus);
    goto_if_rpcstatus_not_success(rpcstatus, done);

    rpc_mgmt_set_com_timeout(*binding, 6, &rpcstatus);
    goto_if_rpcstatus_not_success(rpcstatus, done);
    
    // TODO: Possible memleak here
    if (binding_string != NULL) {
        rpc_string_free(&binding_string, &rpcstatus);
    }
    goto_if_rpcstatus_not_success(rpcstatus, done);

done:
    SAFE_FREE(address);

    return rpcstatus;
}


RPCSTATUS InitSrvSvcBindingFull()
{
    RPCSTATUS rpcstatus = 0;
    return rpcstatus;
}


RPCSTATUS FreeSrvSvcBinding(handle_t *binding)
{
    RPCSTATUS rpcstatus = RPC_S_OK;

    /* Free the binding itself */
    if (binding && *binding) {
	    rpc_binding_free(binding, &rpcstatus);
        goto_if_rpcstatus_not_success(rpcstatus, done);    
    }

    /* Clear the handle pointer */
    *binding = NULL;

    /* Free the host address if possible */
    // This crashes unexpectedly. I don't know why yet.
    //if (rpcstatus == RPC_S_OK && address != NULL) {
    // 	free(address);
    //}

done:
    return rpcstatus;
}


#ifdef _WIN32

/*
 * memory management functions for rpc internals use
 */
void __RPC_FAR * __RPC_USER midl_user_allocate(size_t len)
{
    return (malloc(len));
}

void __RPC_USER midl_user_free(void __RPC_FAR *ptr)
{
    free(ptr);
}

#endif


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
