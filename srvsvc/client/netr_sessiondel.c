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

NET_API_STATUS
NetrSessionDel(
    PSRVSVC_CONTEXT pContext,  /* IN              */
    PCWSTR  pwszServername,    /* IN     OPTIONAL */
    PCWSTR  pwszUncClientname, /* IN     OPTIONAL */
    PCWSTR  pwszUsername       /* IN     OPTIONAL */
    )
{
    NET_API_STATUS status = ERROR_SUCCESS;
    dcethread_exc* pDceException  = NULL;

    BAIL_ON_INVALID_PTR(pContext, status);

    TRY
    {
        status = _NetrSessionDel(
                    pContext->hBinding,
                    pwszServername,
                    pwszUncClientname,
                    pwszUsername);
    }
    CATCH_ALL(pDceException)
    {
        NTSTATUS ntStatus = LwRpcStatusToNtStatus(pDceException->match.value);
        status = LwNtStatusToWin32Error(ntStatus);
    }
    ENDTRY;
    BAIL_ON_WIN_ERROR(status);

cleanup:

    return status;

error:
    goto cleanup;
}
