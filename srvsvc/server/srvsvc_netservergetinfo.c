/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software
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
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        srvsvc_netservergetinfo.c
 *
 * Abstract:
 *
 *        Likewise Server Service (srvsvc) RPC client and server
 *
 *        SrvSvcNetServerGetInfo server API
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NET_API_STATUS
SrvSvcNetrServerGetInfo(
    /* [in] */ handle_t b,
    /* [in] */ wchar16_t *server_name,
    /* [in] */ uint32 level,
    /* [out] */ srvsvc_NetSrvInfo *info
    )
{
    DWORD dwError = 0;
    SERVER_INFO_101 *pInfo101 = NULL;
    SERVER_INFO_102 *pInfo102 = NULL;

    if (level == 101) {
        dwError = SrvSvcSrvAllocateMemory(sizeof(*pInfo101),
                                       (void**)&pInfo101);
        BAIL_ON_ERROR(dwError);

        pInfo101->sv101_platform_id    = 500;
        pInfo101->sv101_name           = ambstowc16s("UBUNTU8-DESKTOP");
        pInfo101->sv101_version_major  = 5;
        pInfo101->sv101_version_minor  = 1;
        pInfo101->sv101_type           = 0x0001003;
        pInfo101->sv101_comment        = ambstowc16s("Likewise RPC");

        info->info101 = pInfo101;

    } else if (level == 102) {
        dwError = SrvSvcSrvAllocateMemory(sizeof(*pInfo102),
                                       (void**)&pInfo102);
        BAIL_ON_ERROR(dwError);

        pInfo102->sv102_platform_id    = 500;
        pInfo102->sv102_name           = ambstowc16s("UBUNTU8-DESKTOP");
        pInfo102->sv102_version_major  = 5;
        pInfo102->sv102_version_minor  = 1;
        pInfo102->sv102_type           = 0x0001003;
        pInfo102->sv102_comment        = ambstowc16s("Likewise RPC");
        pInfo102->sv102_users          = 0;
        pInfo102->sv102_disc           = 0;
        pInfo102->sv102_hidden         = 0;
        pInfo102->sv102_announce       = 0;
        pInfo102->sv102_anndelta       = 0;
        pInfo102->sv102_licenses       = 5;
        pInfo102->sv102_userpath       = ambstowc16s("/testpath");

        info->info102 = pInfo102;

    } else {
        dwError = ERROR_NOT_SUPPORTED;
    }

cleanup:
    return dwError;

error:
    if (pInfo101) {
        if (pInfo101->sv101_name) {
            SrvSvcSrvFreeMemory(pInfo101->sv101_name);
        }

        if (pInfo101->sv101_comment) {
            SrvSvcSrvFreeMemory(pInfo101->sv101_comment);
        }

        SrvSvcSrvFreeMemory(pInfo101);

    } else if (pInfo102) {
        if (pInfo102->sv102_name) {
            SrvSvcSrvFreeMemory(pInfo102->sv102_name);
        }

        if (pInfo102->sv102_comment) {
            SrvSvcSrvFreeMemory(pInfo102->sv102_comment);
        }

        if (pInfo102->sv102_userpath) {
            SrvSvcSrvFreeMemory(pInfo102->sv102_userpath);
        }

        SrvSvcSrvFreeMemory(pInfo102);
    }

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
