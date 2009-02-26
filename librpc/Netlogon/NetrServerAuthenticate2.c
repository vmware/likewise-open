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
 * Authors: Rafal Szczesniak (rafal@likewisesoftware.com)
 */

#include "includes.h"


NTSTATUS NetrServerAuthenticate2(handle_t b, const wchar16_t *server,
                                 const wchar16_t *account, uint16 sec_chan_type,
                                 const wchar16_t *computer, uint8 cli_creds[8],
                                 uint8 srv_creds[8], uint32 *neg_flags)
{
    NTSTATUS status = STATUS_SUCCESS;
    NetrCred creds;
    wchar16_t *srv = NULL;
    wchar16_t *acc = NULL;
    wchar16_t *comp = NULL;

    memset((void*)&creds, 0, sizeof(creds));

    goto_if_invalid_param_ntstatus(b, cleanup);
    goto_if_invalid_param_ntstatus(server, cleanup);
    goto_if_invalid_param_ntstatus(account, cleanup);
    goto_if_invalid_param_ntstatus(computer, cleanup);
    goto_if_invalid_param_ntstatus(cli_creds, cleanup);
    goto_if_invalid_param_ntstatus(srv_creds, cleanup);
    goto_if_invalid_param_ntstatus(neg_flags, cleanup);

    memcpy(creds.data, cli_creds, sizeof(creds.data));

    srv = wc16sdup(server);
    goto_if_no_memory_ntstatus(srv, error);

    acc = wc16sdup(account);
    goto_if_no_memory_ntstatus(acc, error);

    comp = wc16sdup(computer);
    goto_if_no_memory_ntstatus(comp, error);

    DCERPC_CALL(status, _NetrServerAuthenticate2(b, srv, acc, sec_chan_type,
                                                 comp, &creds, neg_flags));
    goto_if_ntstatus_not_success(status, error);

    memcpy(srv_creds, creds.data, sizeof(creds.data));

cleanup:
    memset(&creds, 0, sizeof(creds));

    SAFE_FREE(srv);
    SAFE_FREE(acc);
    SAFE_FREE(comp);

    return status;

error:
    memset(srv_creds, 0, sizeof(creds.data));
    *neg_flags = 0;

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
