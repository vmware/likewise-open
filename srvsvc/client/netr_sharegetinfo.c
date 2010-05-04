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

            a0->shi0_netname = NULL;

            if (info->info0->shi0_netname)
            {
                status = SrvSvcAddDepStringW(
                            a0,
                            info->info0->shi0_netname,
                            &a0->shi0_netname);
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

            a1->shi1_netname = NULL;
            a1->shi1_remark = NULL;

            if (info->info1->shi1_netname)
            {
                status = SrvSvcAddDepStringW(
                            a1,
                            info->info1->shi1_netname,
                            &a1->shi1_netname);
                BAIL_ON_WIN_ERROR(status);
            }

            if (info->info1->shi1_remark)
            {
                status = SrvSvcAddDepStringW(
                            a1,
                            info->info1->shi1_remark,
                            &a1->shi1_remark);
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

            a2->shi2_netname = NULL;
            a2->shi2_remark = NULL;
            a2->shi2_path = NULL;

            if (info->info2->shi2_netname)
            {
                status = SrvSvcAddDepStringW(
                            a2,
                            info->info2->shi2_netname,
                            &a2->shi2_netname);
                BAIL_ON_WIN_ERROR(status);
            }
            if (info->info2->shi2_remark)
            {
                status = SrvSvcAddDepStringW(
                            a2,
                            info->info2->shi2_remark,
                            &a2->shi2_remark);
                BAIL_ON_WIN_ERROR(status);
            }
            if (info->info2->shi2_path)
            {
                status = SrvSvcAddDepStringW(
                            a2,
                            info->info2->shi2_path,
                            &a2->shi2_path);
                BAIL_ON_WIN_ERROR(status);
            }
            if (info->info2->shi2_password)
            {
                status = SrvSvcAddDepStringW(
                            a2,
                            info->info2->shi2_password,
                            &a2->shi2_password);
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

            a501->shi501_netname = NULL;
            a501->shi501_remark = NULL;

            if (info->info501->shi501_netname)
            {
                status = SrvSvcAddDepStringW(
                            a501,
                            info->info501->shi501_netname,
                            &a501->shi501_netname);
                BAIL_ON_WIN_ERROR(status);
            }
            if (info->info501->shi501_remark)
            {
                status = SrvSvcAddDepStringW(
                            a501,
                            info->info501->shi501_remark,
                            &a501->shi501_remark);
                BAIL_ON_WIN_ERROR(status);
            }
        }
        break;
    case 502:
        if (info->info502) {
            PSHARE_INFO_502 a502;
            PSHARE_INFO_502_I pShareInfo;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SHARE_INFO_502),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a502 = (PSHARE_INFO_502)ptr;

            pShareInfo = info->info502;

            a502->shi502_netname      = NULL;
            a502->shi502_type         = pShareInfo->shi502_type;
            a502->shi502_remark       = NULL;
            a502->shi502_permissions  = pShareInfo->shi502_permissions;
            a502->shi502_max_uses     = pShareInfo->shi502_max_uses;
            a502->shi502_current_uses = pShareInfo->shi502_current_uses;
            a502->shi502_path         = NULL;
            a502->shi502_password     = NULL;
            a502->shi502_reserved     = pShareInfo->shi502_reserved;

            if (pShareInfo->shi502_reserved)
            {
                status = SrvSvcAllocateMemory(OUT_PPVOID(&a502->shi502_security_descriptor),
                                              pShareInfo->shi502_reserved,
                                              a502);
                BAIL_ON_WIN_ERROR(status);

                memcpy(a502->shi502_security_descriptor,
                       pShareInfo->shi502_security_descriptor,
                       pShareInfo->shi502_reserved);
            }

            if (pShareInfo->shi502_netname)
            {
                status = SrvSvcAddDepStringW(
                            a502,
                            pShareInfo->shi502_netname,
                            &a502->shi502_netname);
                BAIL_ON_WIN_ERROR(status);
            }
            if (pShareInfo->shi502_remark)
            {
                status = SrvSvcAddDepStringW(
                            a502,
                            pShareInfo->shi502_remark,
                            &a502->shi502_remark);
                BAIL_ON_WIN_ERROR(status);
            }
            if (pShareInfo->shi502_path)
            {
                status = SrvSvcAddDepStringW(
                            a502,
                            pShareInfo->shi502_path,
                            &a502->shi502_path);
                BAIL_ON_WIN_ERROR(status);
            }
            if (pShareInfo->shi502_password)
            {
                status = SrvSvcAddDepStringW(
                            a502,
                            pShareInfo->shi502_password,
                            &a502->shi502_password);
                BAIL_ON_WIN_ERROR(status);
            }
        }
        break;

    case 1005:
        if (info->info1005)
        {
            PSHARE_INFO_1005 a1005;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SHARE_INFO_1005),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1005 = (PSHARE_INFO_1005)ptr;

            *a1005 = *info->info1005;

            a1005->shi1005_flags = info->info1005->shi1005_flags;
        }
        break;
    }

    *bufptr = (UINT8 *)ptr;

cleanup:

    return status;

error:

    if (ptr)
    {
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
