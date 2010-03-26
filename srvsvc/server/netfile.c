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
 *        netfileenum.c
 *
 * Abstract:
 *
 *        Likewise Server Service (srvsvc) RPC client and server
 *
 *        SrvSvcNetFileEnum    (Server API)
 *        SrvSvcNetFileGetInfo (Server API)
 *        SrvSvcNetFileClose   (Server API)
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 */

#include "includes.h"

NET_API_STATUS
SrvSvcNetFileEnum(
    handle_t           IDL_handle,           /* [in]      */
    PWSTR              pwszServername,       /* [in]      */
    PWSTR              pwszBasepath,         /* [in]      */
    PWSTR              pwszUsername,         /* [in]      */
    DWORD              dwInfoLevel,          /* [in, out] */
    srvsvc_NetFileCtr* pInfo,                /* [in, out] */
    DWORD              dwPreferredMaxLength, /* [in]      */
    PDWORD             pdwTotalEntries,      /* [out]     */
    PDWORD             pdwResumeHandle       /* [in, out] */
    )
{
    return ERROR_NOT_SUPPORTED;
}

NET_API_STATUS
SrvSvcNetFileGetInfo(
    handle_t            IDL_handle,     /* [in] */
    PWSTR               pwszServername, /* [in] */
    DWORD               dwFileId,       /* [in] */
    DWORD               dwInfoLevel,    /* [in] */
    srvsvc_NetFileInfo* pInfo           /* [out] */
    )
{
    return ERROR_NOT_SUPPORTED;
}

NET_API_STATUS
SrvSvcNetFileClose(
    handle_t IDL_handle,     /* [in] */
    PWSTR    pwszServername, /* [in] */
    DWORD    dwFileId        /* [in] */
    )
{
    return ERROR_NOT_SUPPORTED;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
