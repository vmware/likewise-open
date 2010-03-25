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
SrvSvcCopyNetShareInfo(
    UINT32               level,
    srvsvc_NetShareInfo* info,
    UINT8**              bufptr
    );

NET_API_STATUS
NetrShareGetInfo(
    IN  PSRVSVC_CONTEXT pContext,
    IN  PCWSTR          pwszServername,
    IN  PCWSTR          pwszNetname,
    IN  DWORD           dwLevel,
    OUT PBYTE          *ppBuffer
    )
{
    NET_API_STATUS status = ERROR_SUCCESS;
    dcethread_exc* pDceException  = NULL;
    srvsvc_NetShareInfo Info;
    PBYTE pBuffer = NULL;

    BAIL_ON_INVALID_PTR(pContext, status);
    BAIL_ON_INVALID_PTR(pwszNetname, status);
    BAIL_ON_INVALID_PTR(ppBuffer, status);

    memset(&Info, 0, sizeof(Info));

    TRY
    {
        status = _NetrShareGetInfo(
                    pContext->hBinding,
                    pwszServername,
                    pwszNetname,
                    dwLevel,
                    &Info);
    }
    CATCH_ALL(pDceException)
    {
        NTSTATUS ntStatus = LwRpcStatusToNtStatus(pDceException->match.value);
        status = LwNtStatusToWin32Error(ntStatus);
    }
    ENDTRY;
    BAIL_ON_WIN_ERROR(status);

    status = SrvSvcCopyNetShareInfo(dwLevel, &Info, &pBuffer);
    BAIL_ON_WIN_ERROR(status);

    *ppBuffer = pBuffer;

cleanup:
    SrvSvcClearNetShareInfo(dwLevel, &Info);

    return status;

error:
    if (pBuffer)
    {
        SrvSvcFreeMemory(pBuffer);
    }

    *ppBuffer = NULL;

    goto cleanup;
}

static
NET_API_STATUS
SrvSvcCopyNetShareInfo(
    UINT32               level,
    srvsvc_NetShareInfo* info,
    UINT8**              bufptr
    )
{
    NET_API_STATUS status = ERROR_SUCCESS;
    void *ptr = NULL;

    BAIL_ON_INVALID_PTR(bufptr, status);
    *bufptr = NULL;

    BAIL_ON_INVALID_PTR(info, status);

    switch (level) {
    case 0:
        if (info->info0) {
            PSHARE_INFO_0 a0;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SHARE_INFO_0),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a0 = (PSHARE_INFO_0)ptr;

            *a0 = *info->info0;

            if (a0->shi0_netname)
            {
                status = SrvSvcAddDepStringW(a0, a0->shi0_netname);
                BAIL_ON_WIN_ERROR(status);
            }
        }
        break;
    case 1:
        if (info->info1) {
            PSHARE_INFO_1 a1;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SHARE_INFO_1),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1 = (PSHARE_INFO_1)ptr;

            *a1 = *info->info1;

            if (a1->shi1_netname)
            {
                status = SrvSvcAddDepStringW(a1, a1->shi1_netname);
                BAIL_ON_WIN_ERROR(status);
            }

            if (a1->shi1_remark)
            {
                status = SrvSvcAddDepStringW(a1, a1->shi1_remark);
                BAIL_ON_WIN_ERROR(status);
            }
        }
        break;
    case 2:
        if (info->info2) {
            PSHARE_INFO_2 a2;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SHARE_INFO_2),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a2 = (PSHARE_INFO_2)ptr;

            *a2 = *info->info2;

            if (a2->shi2_netname)
            {
                status = SrvSvcAddDepStringW(a2, a2->shi2_netname);
                BAIL_ON_WIN_ERROR(status);
            }
            if (a2->shi2_remark)
            {
                status = SrvSvcAddDepStringW(a2, a2->shi2_remark);
                BAIL_ON_WIN_ERROR(status);
            }
            if (a2->shi2_path)
            {
                status = SrvSvcAddDepStringW(a2, a2->shi2_path);
                BAIL_ON_WIN_ERROR(status);
            }
            if (a2->shi2_password)
            {
                status = SrvSvcAddDepStringW(a2, a2->shi2_password);
                BAIL_ON_WIN_ERROR(status);
            }
        }
        break;
    case 501:
        if (info->info501) {
            PSHARE_INFO_501 a501;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SHARE_INFO_501),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a501 = (PSHARE_INFO_501)ptr;

            *a501 = *info->info501;

            if (a501->shi501_netname)
            {
                status = SrvSvcAddDepStringW(a501, a501->shi501_netname);
                BAIL_ON_WIN_ERROR(status);
            }
            if (a501->shi501_remark)
            {
                status = SrvSvcAddDepStringW(a501, a501->shi501_remark);
                BAIL_ON_WIN_ERROR(status);
            }
        }
        break;
    case 502:
        if (info->info502) {
            PSHARE_INFO_502 a502;
            PSHARE_INFO_502_I e;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SHARE_INFO_502),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a502 = (PSHARE_INFO_502)ptr;

            e = info->info502;

            a502->shi502_netname      = e->shi502_netname;
            a502->shi502_type         = e->shi502_type;
            a502->shi502_remark       = e->shi502_remark;
            a502->shi502_permissions  = e->shi502_permissions;
            a502->shi502_max_uses     = e->shi502_max_uses;
            a502->shi502_current_uses = e->shi502_current_uses;
            a502->shi502_path         = e->shi502_path;
            a502->shi502_password     = e->shi502_password;
            a502->shi502_reserved     = e->shi502_reserved;

            if (e->shi502_reserved)
            {
                status = SrvSvcAllocateMemory(OUT_PPVOID(&a502->shi502_security_descriptor),
                                              e->shi502_reserved,
                                              a502);
                BAIL_ON_WIN_ERROR(status);

                memcpy(a502->shi502_security_descriptor,
                       e->shi502_security_descriptor,
                       e->shi502_reserved);
            }

            if (a502->shi502_netname)
            {
                status = SrvSvcAddDepStringW(a502, a502->shi502_netname);
                BAIL_ON_WIN_ERROR(status);
            }
            if (a502->shi502_remark)
            {
                status = SrvSvcAddDepStringW(a502, a502->shi502_remark);
                BAIL_ON_WIN_ERROR(status);
            }
            if (a502->shi502_path)
            {
                status = SrvSvcAddDepStringW(a502, a502->shi502_path);
                BAIL_ON_WIN_ERROR(status);
            }
            if (a502->shi502_password)
            {
                status = SrvSvcAddDepStringW(a502, a502->shi502_password);
                BAIL_ON_WIN_ERROR(status);
            }
        }
        break;
    }

    *bufptr = (UINT8 *)ptr;
cleanup:
    return status;
error:
    if (ptr) {
        SrvSvcFreeMemory(ptr);
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
