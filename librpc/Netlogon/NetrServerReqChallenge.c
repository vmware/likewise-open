/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: */

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


NTSTATUS NetrServerReqChallenge(handle_t b, const wchar16_t *server,
                                const wchar16_t *computer,
                                uint8 cli_challenge[8], uint8 srv_challenge[8])
{
    NTSTATUS status = STATUS_SUCCESS;
    NetrCred creds = {0};
    wchar16_t *srv = NULL;
    wchar16_t *comp = NULL;

    goto_if_invalid_param_ntstatus(b, cleanup);
    goto_if_invalid_param_ntstatus(server, cleanup);
    goto_if_invalid_param_ntstatus(computer, cleanup);
    goto_if_invalid_param_ntstatus(cli_challenge, cleanup);
    goto_if_invalid_param_ntstatus(srv_challenge, cleanup);

    memcpy(creds.data, cli_challenge, sizeof(creds.data));

    srv = wc16sdup(server);
    goto_if_no_memory_ntstatus(srv, error);

    comp = wc16sdup(computer);
    goto_if_no_memory_ntstatus(comp, error);

    DCERPC_CALL(status, _NetrServerReqChallenge(b, srv, comp, &creds));
    goto_if_ntstatus_not_success(status, error);

    memcpy(srv_challenge, creds.data, sizeof(creds.data));

cleanup:
    memset(&creds, 0, sizeof(creds));

    SAFE_FREE(srv);
    SAFE_FREE(comp);

    return status;

error:
    memset(srv_challenge, 0, sizeof(creds.data));

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
