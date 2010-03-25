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

#include "includes.h"

static
NET_API_STATUS
SrvSvcCopyNetShareCtr(
    UINT32              level,
    srvsvc_NetShareCtr* ctr,
    UINT32*             entriesread,
    UINT8**             bufptr
    );

NET_API_STATUS
NetrShareEnum(
    IN  PSRVSVC_CONTEXT pContext,
    IN  PCWSTR   pwszServername,
    IN  DWORD    dwLevel,
    OUT PBYTE   *ppBuffer,
    IN  DWORD    dwMaxLen,
    OUT PDWORD   pdwNumEntries,
    OUT PDWORD   pdwTotalEntries,
    OUT PDWORD   pdwResume
    )
{
    NET_API_STATUS status = ERROR_SUCCESS;
    dcethread_exc* pDceException  = NULL;
    srvsvc_NetShareCtr ctr;
    srvsvc_NetShareCtr0 ctr0;
    srvsvc_NetShareCtr1 ctr1;
    srvsvc_NetShareCtr2 ctr2;
    srvsvc_NetShareCtr501 ctr501;
    srvsvc_NetShareCtr502 ctr502;
    PBYTE pBuffer = NULL;
    DWORD dwNumEntries = 0;
    DWORD dwTotalEntries = 0;
    DWORD dwReturnedLevel = dwLevel;

    BAIL_ON_INVALID_PTR(pContext, status);
    BAIL_ON_INVALID_PTR(ppBuffer, status);
    BAIL_ON_INVALID_PTR(pdwNumEntries, status);
    BAIL_ON_INVALID_PTR(pdwTotalEntries, status);

    memset(&ctr, 0, sizeof(ctr));
    memset(&ctr0, 0, sizeof(ctr0));
    memset(&ctr1, 0, sizeof(ctr1));
    memset(&ctr2, 0, sizeof(ctr2));
    memset(&ctr501, 0, sizeof(ctr501));
    memset(&ctr502, 0, sizeof(ctr502));

    switch (dwLevel)
    {
        case 0:
            ctr.ctr0 = &ctr0;
            break;

        case 1:
            ctr.ctr1 = &ctr1;
            break;

        case 2:
            ctr.ctr2 = &ctr2;
            break;

        case 501:
            ctr.ctr501 = &ctr501;
            break;

        case 502:
            ctr.ctr502 = &ctr502;
            break;

        default:

            status = ERROR_INVALID_LEVEL;
            BAIL_ON_WIN_ERROR(status);

            break;
    }

    TRY
    {
        status = _NetrShareEnum(
                    pContext->hBinding,
                    pwszServername,
                    &dwReturnedLevel,
                    &ctr,
                    dwMaxLen,
                    &dwTotalEntries,
                    pdwResume);
    }
    CATCH_ALL(pDceException)
    {
        NTSTATUS ntStatus = LwRpcStatusToNtStatus(pDceException->match.value);
        status = LwNtStatusToWin32Error(ntStatus);
    }
    ENDTRY;
    BAIL_ON_WIN_ERROR(status);

    if (dwReturnedLevel != dwLevel)
    {
        status = ERROR_BAD_NET_RESP;
        BAIL_ON_WIN_ERROR(status);
    }

    status = SrvSvcCopyNetShareCtr(dwLevel,
                                   &ctr,
                                   &dwNumEntries,
                                   &pBuffer);
    BAIL_ON_WIN_ERROR(status);

    *pdwNumEntries   = dwNumEntries;
    *pdwTotalEntries = dwTotalEntries;
    *ppBuffer        = pBuffer;

cleanup:

    SrvSvcClearNetShareCtr(dwLevel, &ctr);

    return status;

error:

    *pdwNumEntries   = 0;
    *pdwTotalEntries = 0;
    *pdwResume       = 0;
    *ppBuffer        = NULL;

    goto cleanup;
}

static
NET_API_STATUS
SrvSvcCopyNetShareCtr(
    UINT32              level,
    srvsvc_NetShareCtr* ctr,
    UINT32*             entriesread,
    UINT8**             bufptr
    )
{
    NET_API_STATUS status = ERROR_SUCCESS;
    int i;
    int count = 0;
    void *ptr = NULL;

    BAIL_ON_INVALID_PTR(entriesread, status);
    BAIL_ON_INVALID_PTR(bufptr, status);
    BAIL_ON_INVALID_PTR(ctr, status);

    *entriesread = 0;
    *bufptr = NULL;

    switch (level) {
    case 0:
        if (ctr->ctr0) {
            PSHARE_INFO_0 a0;

            count = ctr->ctr0->count;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SHARE_INFO_0) * count,
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a0 = (PSHARE_INFO_0)ptr;

            for (i=0; i < count; i++) {
                 a0[i] = ctr->ctr0->array[i];

                 if (a0[i].shi0_netname)
                 {
                     status = SrvSvcAddDepStringW(a0, a0[i].shi0_netname);
                     BAIL_ON_WIN_ERROR(status);
                 }
            }
        }
        break;
    case 1:
        if (ctr->ctr1) {
            PSHARE_INFO_1 a1;

            count = ctr->ctr1->count;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SHARE_INFO_1) * count,
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1 = (PSHARE_INFO_1)ptr;

            for (i=0; i < count; i++) {
                 a1[i] = ctr->ctr1->array[i];

                 if (a1[i].shi1_netname)
                 {
                     status = SrvSvcAddDepStringW(a1, a1[i].shi1_netname);
                     BAIL_ON_WIN_ERROR(status);
                 }
                 if (a1[i].shi1_remark)
                 {
                     status = SrvSvcAddDepStringW(a1, a1[i].shi1_remark);
                     BAIL_ON_WIN_ERROR(status);
                 }
            }
        }
        break;
    case 2:
        if (ctr->ctr2) {
            PSHARE_INFO_2 a2;

            count = ctr->ctr2->count;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SHARE_INFO_2) * count,
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a2 = (PSHARE_INFO_2)ptr;

            for (i=0; i < count; i++) {
                 a2[i] = ctr->ctr2->array[i];
                 if (a2[i].shi2_netname)
                 {
                     status = SrvSvcAddDepStringW(a2, a2[i].shi2_netname);
                     BAIL_ON_WIN_ERROR(status);
                 }
                 if (a2[i].shi2_remark)
                 {
                     status = SrvSvcAddDepStringW(a2, a2[i].shi2_remark);
                     BAIL_ON_WIN_ERROR(status);
                 }
                 if (a2[i].shi2_path)
                 {
                     status = SrvSvcAddDepStringW(a2, a2[i].shi2_path);
                     BAIL_ON_WIN_ERROR(status);
                 }
                 if (a2[i].shi2_password)
                 {
                     status = SrvSvcAddDepStringW(a2, a2[i].shi2_password);
                     BAIL_ON_WIN_ERROR(status);
                 }
            }
        }
        break;
    case 501:
        if (ctr->ctr501) {
            PSHARE_INFO_501 a501;

            count = ctr->ctr501->count;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SHARE_INFO_501) * count,
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a501 = (PSHARE_INFO_501)ptr;

            for (i=0; i < count; i++) {
                 a501[i] = ctr->ctr501->array[i];

                 if (a501[i].shi501_netname)
                 {
                     status = SrvSvcAddDepStringW(a501, a501[i].shi501_netname);
                     BAIL_ON_WIN_ERROR(status);
                 }
                 if (a501[i].shi501_remark)
                 {
                     status = SrvSvcAddDepStringW(a501, a501[i].shi501_remark);
                     BAIL_ON_WIN_ERROR(status);
                 }
            }
        }
        break;
    case 502:
        if (ctr->ctr502) {
            PSHARE_INFO_502 a502;

            count = ctr->ctr502->count;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SHARE_INFO_502) * count,
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a502 = (PSHARE_INFO_502)ptr;

            for (i=0; i < count; i++) {
                 PSHARE_INFO_502_I e;

                 e = &ctr->ctr502->array[i];

                 a502[i].shi502_netname      = e->shi502_netname;
                 a502[i].shi502_type         = e->shi502_type;
                 a502[i].shi502_remark       = e->shi502_remark;
                 a502[i].shi502_permissions  = e->shi502_permissions;
                 a502[i].shi502_max_uses     = e->shi502_max_uses;
                 a502[i].shi502_current_uses = e->shi502_current_uses;
                 a502[i].shi502_path         = e->shi502_path;
                 a502[i].shi502_password     = e->shi502_password;
                 a502[i].shi502_reserved     = e->shi502_reserved;

                 if (e->shi502_reserved)
                 {
                     status = SrvSvcAllocateMemory(OUT_PPVOID(&a502[i].shi502_security_descriptor),
                                                   e->shi502_reserved,
                                                   a502);
                     BAIL_ON_WIN_ERROR(status);
                 }

                 if (a502[i].shi502_netname)
                 {
                     status = SrvSvcAddDepStringW(a502, a502[i].shi502_netname);
                     BAIL_ON_WIN_ERROR(status);
                 }
                 if (a502[i].shi502_remark)
                 {
                     status = SrvSvcAddDepStringW(a502, a502[i].shi502_remark);
                     BAIL_ON_WIN_ERROR(status);
                 }
                 if (a502[i].shi502_path)
                 {
                     status = SrvSvcAddDepStringW(a502, a502[i].shi502_path);
                     BAIL_ON_WIN_ERROR(status);
                 }
                 if (a502[i].shi502_password)
                 {
                     status = SrvSvcAddDepStringW(a502, a502[i].shi502_password);
                     BAIL_ON_WIN_ERROR(status);
                 }

                 memcpy(a502[i].shi502_security_descriptor, e->shi502_security_descriptor, e->shi502_reserved);
            }
        }
        break;
    }

    *entriesread = count;
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
