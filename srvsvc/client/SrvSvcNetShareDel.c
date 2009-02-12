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

#include "includes.h"


NET_API_STATUS NetShareDel(
    handle_t b,
    const wchar16_t *servername,
    const wchar16_t *netname,
    uint32 reserved
    )
{
    NET_API_STATUS status = ERROR_SUCCESS;
    wchar16_t *srv_name = NULL;
    wchar16_t *net_name = NULL;

    goto_if_invalid_param_err(b, done);
    goto_if_invalid_param_err(netname, done);

    if (servername) {
        srv_name = wc16sdup(servername);
        if (srv_name == NULL) {
            status = SRVSVC_ERROR_OUT_OF_MEMORY;
            goto done;
        }
    }

    net_name = wc16sdup(netname);
    if (net_name) {
        status = SRVSVC_ERROR_OUT_OF_MEMORY;
	goto done;
    }

    DCERPC_CALL(_NetrShareDel(b, srv_name, net_name, reserved));

done:
    SAFE_FREE(srv_name);
    SAFE_FREE(net_name);

    return status;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
