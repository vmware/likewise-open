/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2009
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
 * WKsSvc Server
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 *
 */

#include "includes.h"

/********************************************************************
 *******************************************************************/

NET_API_STATUS
_NetWkstaGetInfo(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *server_name,
    /* [in] */ uint32 level,
    /* [out] */ wkssvc_NetWkstaInfo *info
    )
{
    DWORD dwError = 0;

    dwError = WksSrvNetWkstaGetInfo(IDL_handle,
                                    server_name,
                                    level,
                                    info);
    return dwError;
}

/********************************************************************
 *******************************************************************/

void _wkssvc_Function0x1(
    /* [in] */ handle_t IDL_handle
    )
{
}

/********************************************************************
 *******************************************************************/

void _wkssvc_Function0x2(
    /* [in] */ handle_t IDL_handle
    )
{
}

/********************************************************************
 *******************************************************************/

void _wkssvc_Function0x3(
    /* [in] */ handle_t IDL_handle
    )
{
}

/********************************************************************
 *******************************************************************/

void _wkssvc_Function0x4(
    /* [in] */ handle_t IDL_handle
    )
{
}

/********************************************************************
 *******************************************************************/

void _wkssvc_Function0x5(
    /* [in] */ handle_t IDL_handle
    )
{
}

/********************************************************************
 *******************************************************************/

void _wkssvc_Function0x6(
    /* [in] */ handle_t IDL_handle
    )
{
}

/********************************************************************
 *******************************************************************/

void _wkssvc_Function0x7(
    /* [in] */ handle_t IDL_handle
    )
{
}

/********************************************************************
 *******************************************************************/

void _wkssvc_Function0x8(
    /* [in] */ handle_t IDL_handle
    )
{
}

/********************************************************************
 *******************************************************************/

void _wkssvc_Function0x9(
    /* [in] */ handle_t IDL_handle
    )
{
}

/********************************************************************
 *******************************************************************/

void _wkssvc_Function0xa(
    /* [in] */ handle_t IDL_handle
    )
{
}

/********************************************************************
 *******************************************************************/

void _wkssvc_Function0xb(
    /* [in] */ handle_t IDL_handle
    )
{
}

/********************************************************************
 *******************************************************************/

void _wkssvc_Function0xc(
    /* [in] */ handle_t IDL_handle
    )
{
}

/********************************************************************
 *******************************************************************/

void _wkssvc_Function0xd(
    /* [in] */ handle_t IDL_handle
    )
{
}

/********************************************************************
 *******************************************************************/

void _wkssvc_Function0xe(
    /* [in] */ handle_t IDL_handle
    )
{
}

/********************************************************************
 *******************************************************************/

void _wkssvc_Function0xf(
    /* [in] */ handle_t IDL_handle
    )
{
}

/********************************************************************
 *******************************************************************/

void _wkssvc_Function0x10(
    /* [in] */ handle_t IDL_handle
    )
{
}

/********************************************************************
 *******************************************************************/

void _wkssvc_Function0x11(
    /* [in] */ handle_t IDL_handle
    )
{
}

/********************************************************************
 *******************************************************************/

void _wkssvc_Function0x12(
    /* [in] */ handle_t IDL_handle
    )
{
}

/********************************************************************
 *******************************************************************/

void _wkssvc_Function0x13(
    /* [in] */ handle_t IDL_handle
    )
{
}

/********************************************************************
 *******************************************************************/

void _wkssvc_Function0x14(
    /* [in] */ handle_t IDL_handle
    )
{
}

/********************************************************************
 *******************************************************************/

void _wkssvc_Function0x15(
    /* [in] */ handle_t IDL_handle
    )
{
}

/********************************************************************
 *******************************************************************/

void _wkssvc_Function0x16(
    /* [in] */ handle_t IDL_handle
    )
{
}

/********************************************************************
 *******************************************************************/

void _wkssvc_Function0x17(
    /* [in] */ handle_t IDL_handle
    )
{
}

/********************************************************************
 *******************************************************************/

void _wkssvc_Function0x18(
    /* [in] */ handle_t IDL_handle
    )
{
}

/********************************************************************
 *******************************************************************/

void _wkssvc_Function0x19(
    /* [in] */ handle_t IDL_handle
    )
{
}

/********************************************************************
 *******************************************************************/

void _wkssvc_Function0x1a(
    /* [in] */ handle_t IDL_handle
    )
{
}

/********************************************************************
 *******************************************************************/

void _wkssvc_Function0x1b(
    /* [in] */ handle_t IDL_handle
    )
{
}

/********************************************************************
 *******************************************************************/

void _wkssvc_Function0x1c(
    /* [in] */ handle_t IDL_handle
    )
{
}

/********************************************************************
 *******************************************************************/

void _wkssvc_Function0x1d(
    /* [in] */ handle_t IDL_handle
    )
{
}

/********************************************************************
 *******************************************************************/

void _wkssvc_Function0x1e(
    /* [in] */ handle_t IDL_handle
    )
{
}



/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
