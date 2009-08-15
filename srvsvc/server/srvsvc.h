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
 *        srvsvc.h
 *
 * Abstract:
 *
 *        Likewise Server Service (srvsvc) RPC client and server
 *
 *        SrvSvc rpc server functions
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#ifndef _SRVSVCSRV_H_
#define _SRVSVCSRV_H_


NET_API_STATUS
SrvSvcNetShareAdd(
    handle_t IDL_handle,
    wchar16_t *server_name,
    uint32 level,
    srvsvc_NetShareInfo info,
    uint32 *parm_error
    );


NET_API_STATUS
SrvSvcNetShareEnum(
    handle_t IDL_handle,
    wchar16_t *server_name,
    uint32 *level,
    srvsvc_NetShareCtr *ctr,
    uint32 preferred_maximum_length,
    uint32 *total_entries,
    uint32 *resume_handle
    );


NET_API_STATUS
SrvSvcNetShareGetInfo(
    handle_t IDL_handle,
    wchar16_t *server_name,
    wchar16_t *netname,
    uint32 level,
    srvsvc_NetShareInfo *info
    );


NET_API_STATUS
SrvSvcNetShareSetInfo(
    handle_t IDL_handle,
    wchar16_t *server_name,
    wchar16_t *netname,
    uint32 level,
    srvsvc_NetShareInfo info,
    uint32 *parm_error
    );


NET_API_STATUS
SrvSvcNetShareDel(
    handle_t IDL_handle,
    wchar16_t *server_name,
    wchar16_t *netname,
    uint32 reserved
    );


NET_API_STATUS
SrvSvcNetrServerGetInfo(
    handle_t b,
    wchar16_t *server_name,
    uint32 level,
    srvsvc_NetSrvInfo *info
    );


NET_API_STATUS
SrvSvcNetNameValidate(
    handle_t IDL_handle,
    wchar16_t *server_name,
    wchar16_t *name,
    uint32 type,
    uint32 flags
    );


#endif /* _SRVSVCSRV_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
