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


NET_API_STATUS NetFileEnum(
    handle_t b,
    const wchar16_t *servername,
    const wchar16_t *basepath,
    const wchar16_t *username,
    uint32 level,
    uint8 **bufptr,
    uint32 prefmaxlen,
    uint32 *entriesread,
    uint32 *totalentries,
    uint32 *resume_handle
    )
{
    NET_API_STATUS status = ERROR_SUCCESS;
    NET_API_STATUS memerr = ERROR_SUCCESS;
    srvsvc_NetFileCtr ctr;
    srvsvc_NetFileCtr2 ctr2;
    srvsvc_NetFileCtr3 ctr3;
    uint32 l = level;

    goto_if_invalid_param_err(b, done);
    goto_if_invalid_param_err(bufptr, done);
    goto_if_invalid_param_err(entriesread, done);
    goto_if_invalid_param_err(totalentries, done);

    memset(&ctr, 0, sizeof(ctr));
    memset(&ctr2, 0, sizeof(ctr2));
    memset(&ctr3, 0, sizeof(ctr3));
    *entriesread = 0;
    *bufptr = NULL;

    switch (level) {
    case 2:
        ctr.ctr2 = &ctr2;
        break;
    case 3:
        ctr.ctr3 = &ctr3;
        break;
    }

    DCERPC_CALL(_NetrFileEnum(b,
                              (wchar16_t *)servername,
                              (wchar16_t *)basepath,
                              (wchar16_t *)username,
                              &l, &ctr,
                              prefmaxlen, totalentries,
                              resume_handle));

    if (l != level) {
        status = ERROR_BAD_NET_RESP;
        goto done;
    }

    memerr = SrvSvcCopyNetFileCtr(l, &ctr, entriesread, bufptr);
    goto_if_err_not_success(memerr, done);

done:
    switch (level) {
    case 2:
        if (ctr.ctr2 == &ctr2) {
            ctr.ctr2 = NULL;
        }
        break;
    case 3:
        if (ctr.ctr3 == &ctr3) {
            ctr.ctr3 = NULL;
        }
        break;
    }
    SrvSvcClearNetFileCtr(l, &ctr);
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
