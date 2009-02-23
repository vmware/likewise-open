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

void _srvsvc_Function0(
    /* [in] */ handle_t IDL_handle
    )
{
}

void _srvsvc_Function1(
    /* [in] */ handle_t IDL_handle
    )
{
}

void _srvsvc_Function2(
    /* [in] */ handle_t IDL_handle
    )
{
}

void _srvsvc_Function3(
    /* [in] */ handle_t IDL_handle
    )
{
}

void _srvsvc_Function4(
    /* [in] */ handle_t IDL_handle
    )
{
}

void _srvsvc_Function5(
    /* [in] */ handle_t IDL_handle
    )
{
}

void _srvsvc_Function6(
    /* [in] */ handle_t IDL_handle
    )
{
}

void _srvsvc_Function7(
    /* [in] */ handle_t IDL_handle
    )
{
}

NET_API_STATUS _NetrConnectionEnum(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *server_name,
    /* [in] */ wchar16_t *qualifier,
    /* [in, out] */ uint32 *level,
    /* [in, out] */ srvsvc_NetConnCtr *ctr,
    /* [in] */ uint32 prefered_maximum_length,
    /* [out] */ uint32 *total_entries,
    /* [in, out] */ uint32 *resume_handle
    )
{
    DWORD dwError = ERROR_NOT_SUPPORTED;

    return dwError;
}

NET_API_STATUS _NetrFileEnum(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *server_name,
    /* [in] */ wchar16_t *basepath,
    /* [in] */ wchar16_t *username,
    /* [in, out] */ uint32 *level,
    /* [in, out] */ srvsvc_NetFileCtr *ctr,
    /* [in] */ uint32 prefered_maximum_length,
    /* [out] */ uint32 *total_entries,
    /* [in, out] */ uint32 *resume_handle
    )
{
    DWORD dwError = ERROR_NOT_SUPPORTED;

    return dwError;
}


NET_API_STATUS _NetrFileGetInfo(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *server_name,
    /* [in] */ uint32 fileid,
    /* [in] */ uint32 level,
    /* [out] */ srvsvc_NetFileInfo *info
    )
{
    DWORD dwError = ERROR_NOT_SUPPORTED;

    return dwError;
}

NET_API_STATUS _NetrFileClose(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *server_name,
    /* [in] */ uint32 fileid
    )
{
    DWORD dwError = ERROR_NOT_SUPPORTED;

    return dwError;
}

NET_API_STATUS _NetrSessionEnum(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *server_name,
    /* [in] */ wchar16_t *unc_client_name,
    /* [in] */ wchar16_t *username,
    /* [in, out] */ uint32 *level,
    /* [in, out] */ srvsvc_NetSessCtr *ctr,
    /* [in] */ uint32 prefered_maximum_length,
    /* [out] */ uint32 *total_entries,
    /* [in, out] */ uint32 *resume_handle
    )
{
    DWORD dwError = ERROR_NOT_SUPPORTED;

    return dwError;
}

void _srvsvc_FunctionD(
    /* [in] */ handle_t IDL_handle
    )
{
}

NET_API_STATUS _NetrShareAdd(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *server_name,
    /* [in] */ uint32 level,
    /* [in] */ srvsvc_NetShareInfo info,
    /* [in, out] */ uint32 *parm_error
    )
{
    DWORD dwError = 0;

    dwError = SrvSvcNetShareAdd(
                    IDL_handle,
                    server_name,
                    level,
                    info,
                    parm_error
                    );
    return dwError;
}

NET_API_STATUS _NetrShareEnum(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *server_name,
    /* [in, out] */ uint32 *level,
    /* [in, out] */ srvsvc_NetShareCtr *ctr,
    /* [in] */ uint32 prefered_maximum_length,
    /* [out] */ uint32 *total_entries,
    /* [in, out] */ uint32 *resume_handle
    )
{
    DWORD dwError = 0;

    dwError = SrvSvcNetShareEnum(
                    IDL_handle,
                    server_name,
                    level,
                    ctr,
                    prefered_maximum_length,
                    total_entries,
                    resume_handle
                    );
    return dwError;
}

NET_API_STATUS _NetrShareGetInfo(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *server_name,
    /* [in] */ wchar16_t *netname,
    /* [in] */ uint32 level,
    /* [out] */ srvsvc_NetShareInfo *info
    )
{
    return ERROR_NOT_SUPPORTED;
}

NET_API_STATUS _NetrShareSetInfo(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *server_name,
    /* [in] */ wchar16_t *netname,
    /* [in] */ uint32 level,
    /* [in] */ srvsvc_NetShareInfo info,
    /* [in, out] */ uint32 *parm_error
    )
{
    return ERROR_NOT_SUPPORTED;
}

NET_API_STATUS _NetrShareDel(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *server_name,
    /* [in] */ wchar16_t *netname,
    /* [in] */ uint32 reserved
    )
{
    DWORD dwError = 0;

    dwError = SrvSvcNetShareDel(
                    IDL_handle,
                    server_name,
                    netname,
                    reserved
                    );
    return dwError;
}

void _srvsvc_Function13(
    /* [in] */ handle_t IDL_handle
    )
{
}

void _srvsvc_Function14(
    /* [in] */ handle_t IDL_handle
    )
{
}

NET_API_STATUS _NetrServerGetInfo(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *server_name,
    /* [in] */ uint32 level,
    /* [out] */ srvsvc_NetSrvInfo *info
    )
{
    DWORD dwError = 0;

    dwError = SrvSvcNetrServerGetInfo(
                    IDL_handle,
                    server_name,
                    level,
                    info
                    );
    return dwError;
}

NET_API_STATUS _NetrServerSetInfo(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *server_name,
    /* [in] */ uint32 level,
    /* [in] */ srvsvc_NetSrvInfo info,
    /* [in, out] */ uint32 *parm_error
    )
{
    DWORD dwError = ERROR_NOT_SUPPORTED;

    return dwError;

}

void _srvsvc_Function17(
    /* [in] */ handle_t IDL_handle
    )
{
}

void _srvsvc_Function18(
    /* [in] */ handle_t IDL_handle
    )
{
}

void _srvsvc_Function19(
    /* [in] */ handle_t IDL_handle
    )
{
}

void _srvsvc_Function1a(
    /* [in] */ handle_t IDL_handle
    )
{
}

void _srvsvc_Function1b(
    /* [in] */ handle_t IDL_handle
    )
{
}

NET_API_STATUS _NetrRemoteTOD(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *server_name,
    /* [out] */ TIME_OF_DAY_INFO **info
    )
{
    DWORD dwError = ERROR_NOT_SUPPORTED;

    return dwError;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
