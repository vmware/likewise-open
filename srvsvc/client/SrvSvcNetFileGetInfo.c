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

#include "srvsvc_h.h"

#include <wc16str.h>

#include "SrvSvcUtil.h"
#include "SrvSvcMemory.h"
#include "SrvSvcStubMemory.h"


NET_API_STATUS NetFileGetInfo(
    handle_t b,
    const wchar16_t *servername,
    uint32 fileid,
    uint32 level,
    uint8 **bufptr
    )
{
    NET_API_STATUS status = ERROR_SUCCESS;
    NET_API_STATUS memerr = ERROR_SUCCESS;
    srvsvc_NetFileInfo info;

    goto_if_invalid_param_err(b, done);
    goto_if_invalid_param_err(bufptr, done);

    memset(&info, 0, sizeof(info));
    *bufptr = NULL;

    DCERPC_CALL(_NetrFileGetInfo(b, (wchar16_t *)servername,
                                 fileid, level, &info));

    memerr = SrvSvcCopyNetFileInfo(level, &info, bufptr);
    goto_if_err_not_success(memerr, done);

done:
    SrvSvcClearNetFileInfo(level, &info);
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
