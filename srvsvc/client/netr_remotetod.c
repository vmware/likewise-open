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

static
NET_API_STATUS
SrvSvcCopyTIME_OF_DAY_INFO(
    PTIME_OF_DAY_INFO info,
    UINT8**           bufptr
    );

NET_API_STATUS
NetrRemoteTOD(
    PSRVSVC_CONTEXT pContext,
    const wchar16_t *servername,
    UINT8 **bufptr
    )
{
    NET_API_STATUS status  = ERROR_SUCCESS;
    dcethread_exc* pDceException  = NULL;
    PTIME_OF_DAY_INFO info = NULL;

    BAIL_ON_INVALID_PTR(pContext, status);
    BAIL_ON_INVALID_PTR(bufptr, status);

    TRY
    {
        status = _NetrRemoteTOD(
                        pContext->hBinding,
                        (wchar16_t *)servername,
                        &info);
    }
    CATCH_ALL(pDceException)
    {
        NTSTATUS ntStatus = LwRpcStatusToNtStatus(pDceException->match.value);
        status = LwNtStatusToWin32Error(ntStatus);
    }
    ENDTRY;
    BAIL_ON_WIN_ERROR(status);

    status = SrvSvcCopyTIME_OF_DAY_INFO(info, bufptr);
    BAIL_ON_WIN_ERROR(status);

cleanup:
    SRVSVC_SAFE_FREE(info);
    return status;

error:
    goto cleanup;
}

static
NET_API_STATUS
SrvSvcCopyTIME_OF_DAY_INFO(
    PTIME_OF_DAY_INFO info,
    UINT8**           bufptr
    )
{
    NET_API_STATUS status = ERROR_SUCCESS;
    void *ptr = NULL;

    BAIL_ON_INVALID_PTR(bufptr, status);

    if (info) {
        PTIME_OF_DAY_INFO a;

        status = SrvSvcAllocateMemory(&ptr,
                                      sizeof(TIME_OF_DAY_INFO),
                                      NULL);
        BAIL_ON_WIN_ERROR(status);

        a = (PTIME_OF_DAY_INFO)ptr;

        *a = *info;
    }

    *bufptr = (UINT8 *)ptr;

cleanup:
    return status;

error:
    *bufptr = NULL;

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
