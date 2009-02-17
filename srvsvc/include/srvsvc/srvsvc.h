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
 *        Client API
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#ifndef _SRVSVC_H_
#define _SRVSVC_H_


#include <srvsvcbinding.h>
#include <srvsvcdefs.h>


NET_API_STATUS
NetConnectionEnum(
    handle_t b,
    const wchar16_t *servername,
    const wchar16_t *qualifier,
    uint32 level,
    uint8 **bufptr,
    uint32 prefmaxlen,
    uint32 *entriesread,
    uint32 *totalentries,
    uint32 *resume_handle
    );


NET_API_STATUS
NetFileEnum(
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
    );


NET_API_STATUS
NetFileGetInfo(
    handle_t b,
    const wchar16_t *servername,
    uint32 fileid,
    uint32 level,
    uint8 **bufptr
    );


NET_API_STATUS
NetFileClose(
    handle_t b,
    const wchar16_t *servername,
    uint32 fileid
    );


NET_API_STATUS
NetSessionEnum(
    handle_t b,
    const wchar16_t *servername,
    const wchar16_t *unc_client_name,
    const wchar16_t *username,
    uint32 level,
    uint8 **bufptr,
    uint32 prefmaxlen,
    uint32 *entriesread,
    uint32 *totalentries,
    uint32 *resume_handle
    );


NET_API_STATUS
NetShareAdd(
    handle_t b,
    const wchar16_t *servername,
    uint32 level,
    uint8 *bufptr,
    uint32 *parm_err
    );


NET_API_STATUS
NetShareEnum(
    handle_t b,
    const wchar16_t *servername,
    uint32 level,
    uint8 **bufptr,
    uint32 prefmaxlen,
    uint32 *entriesread,
    uint32 *totalentries,
    uint32 *resume_handle
    );


NET_API_STATUS
NetShareGetInfo(
    handle_t b,
    const wchar16_t *servername,
    const wchar16_t *netname,
    uint32 level,
    uint8 **bufptr
    );


NET_API_STATUS
NetShareSetInfo(
    handle_t b,
    const wchar16_t *servername,
    const wchar16_t *netname,
    uint32 level,
    uint8 *bufptr,
    uint32 *parm_err
    );


NET_API_STATUS
NetShareDel(
    handle_t b,
    const wchar16_t *servername,
    const wchar16_t *netname,
    uint32 reserved
    );


NET_API_STATUS
NetServerGetInfo(
    handle_t b,
    const wchar16_t *servername,
    uint32 level,
    uint8 **bufptr
    );


NET_API_STATUS
NetServerSetInfo(
    handle_t b,
    const wchar16_t *servername,
    uint32 level,
    uint8 *bufptr,
    uint32 *parm_err
    );


NET_API_STATUS
NetRemoteTOD(
    handle_t b,
    const wchar16_t *servername,
    uint8 **bufptr
    );


NET_API_STATUS
SrvSvcInitMemory(
    void
    );


NET_API_STATUS
SrvSvcDestroyMemory(
    void
    );


NET_API_STATUS
SrvSvcFreeMemory(
    void *ptr
    );


#endif /* _SRVSVC_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
