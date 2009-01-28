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


NET_API_STATUS NetShareGetInfo(
    handle_t b,
    const wchar16_t *servername,
    const wchar16_t *netname,
    uint32 level,
    uint8 **bufptr
    )
{
    NET_API_STATUS status = ERROR_SUCCESS;
    NET_API_STATUS memerr = ERROR_SUCCESS;
    srvsvc_NetShareInfo info;

    goto_if_invalid_param_err(b, done);
    goto_if_invalid_param_err(netname, done);
    goto_if_invalid_param_err(bufptr, done);

    memset(&info, 0, sizeof(info));
    *bufptr = NULL;

    DCERPC_CALL(_NetrShareGetInfo(b,
                                  (wchar16_t *)servername,
                                  (wchar16_t *)netname,
                                  level, &info));

    memerr = SrvSvcCopyNetShareInfo(level, &info, bufptr);
    goto_if_err_not_success(memerr, done);

done:
    SrvSvcClearNetShareInfo(level, &info);
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
