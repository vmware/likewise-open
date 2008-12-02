/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software 2007
 * All rights reserved.
 *
 * SrvSvc Server
 *
 */

#include "includes.h"

int main(int argc, const char *argv[])
{
    RPCSTATUS rpcstatus = RPC_S_OK;
    rpc_binding_vector_p_t server_b = NULL;

    rpc_server_register_if(srvsvc_v3_0_s_ifspec, NULL, NULL, &rpcstatus);

    rpc_server_use_protseq("ncacn_ip_tcp", rpc_c_protseq_max_calls_default, &rpcstatus);

    rpc_server_inq_bindings(&server_b, &rpcstatus);

    rpc_ep_register(srvsvc_v3_0_s_ifspec, server_b, NULL, "srvsvc server",
                    &rpcstatus);

    DCETHREAD_TRY
    {
        rpc_server_listen(rpc_c_listen_max_calls_default, &rpcstatus);
    }
    DCETHREAD_CATCH_ALL(THIS_CATCH)
    {
    }
    DCETHREAD_ENDTRY;

    rpc_ep_unregister(srvsvc_v3_0_s_ifspec, server_b, NULL, &rpcstatus);

    rpc_server_unregister_if(srvsvc_v3_0_s_ifspec, NULL, &rpcstatus);

    return 0;
}

