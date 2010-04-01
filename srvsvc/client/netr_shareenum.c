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
    OUT PDWORD   pdwResumeHandle
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
    DWORD dwResumeHandle = (pdwResumeHandle ? *pdwResumeHandle : 0);
    DWORD dwReturnedLevel = dwLevel;
    BOOLEAN bMoreDataAvailable = FALSE;

    memset(&ctr,    0, sizeof(ctr));
    memset(&ctr0,   0, sizeof(ctr0));
    memset(&ctr1,   0, sizeof(ctr1));
    memset(&ctr2,   0, sizeof(ctr2));
    memset(&ctr501, 0, sizeof(ctr501));
    memset(&ctr502, 0, sizeof(ctr502));

    BAIL_ON_INVALID_PTR(pContext, status);
    BAIL_ON_INVALID_PTR(ppBuffer, status);
    BAIL_ON_INVALID_PTR(pdwNumEntries, status);
    BAIL_ON_INVALID_PTR(pdwTotalEntries, status);

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
                    pdwResumeHandle ? &dwResumeHandle : NULL);
    }
    CATCH_ALL(pDceException)
    {
        NTSTATUS ntStatus = LwRpcStatusToNtStatus(pDceException->match.value);
        status = LwNtStatusToWin32Error(ntStatus);
    }
    ENDTRY;

    switch (status)
    {
        case ERROR_MORE_DATA:

            bMoreDataAvailable = TRUE;

            // intentional fall through

        case ERROR_SUCCESS:

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

            if (bMoreDataAvailable)
            {
                status = ERROR_MORE_DATA;
            }

            break;

        default:

            BAIL_ON_WIN_ERROR(status);

            break;
    }

    *pdwNumEntries   = dwNumEntries;
    *pdwTotalEntries = dwTotalEntries;
    if (pdwResumeHandle)
    {
        *pdwResumeHandle = dwResumeHandle;
    }
    *ppBuffer        = pBuffer;

cleanup:

    SrvSvcClearNetShareCtr(dwLevel, &ctr);

    return status;

error:

    if (pdwNumEntries)
    {
        *pdwNumEntries   = 0;
    }
    if (pdwTotalEntries)
    {
        *pdwTotalEntries = 0;
    }
    if (ppBuffer)
    {
        *ppBuffer        = NULL;
    }

    if (pBuffer)
    {
        SrvSvcFreeMemory(pBuffer);
    }

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
            PSHARE_INFO_0 pShareInfo;

            count = ctr->ctr0->count;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SHARE_INFO_0) * count,
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            pShareInfo = (PSHARE_INFO_0)ptr;

            for (i=0; i < count; i++)
            {
                 pShareInfo[i] = ctr->ctr0->array[i];

                 pShareInfo[i].shi0_netname = NULL;

                 if (ctr->ctr0->array[i].shi0_netname)
                 {
                     status = SrvSvcAddDepStringW(
                                 pShareInfo,
                                 ctr->ctr0->array[i].shi0_netname,
                                 &pShareInfo[i].shi0_netname);
                     BAIL_ON_WIN_ERROR(status);
                 }
            }
        }
        break;
    case 1:
        if (ctr->ctr1) {
            PSHARE_INFO_1 pShareInfo;

            count = ctr->ctr1->count;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SHARE_INFO_1) * count,
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            pShareInfo = (PSHARE_INFO_1)ptr;

            for (i=0; i < count; i++)
            {
                 pShareInfo[i] = ctr->ctr1->array[i];

                 pShareInfo[i].shi1_netname = NULL;
                 pShareInfo[i].shi1_remark = NULL;

                 if (ctr->ctr1->array[i].shi1_netname)
                 {
                     status = SrvSvcAddDepStringW(
                                 pShareInfo,
                                 ctr->ctr1->array[i].shi1_netname,
                                 &pShareInfo[i].shi1_netname);
                     BAIL_ON_WIN_ERROR(status);
                 }
                 if (ctr->ctr1->array[i].shi1_remark)
                 {
                     status = SrvSvcAddDepStringW(
                                 pShareInfo,
                                 ctr->ctr1->array[i].shi1_remark,
                                 &pShareInfo[i].shi1_remark);
                     BAIL_ON_WIN_ERROR(status);
                 }
            }
        }
        break;
    case 2:
        if (ctr->ctr2) {
            PSHARE_INFO_2 pShareInfo;

            count = ctr->ctr2->count;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SHARE_INFO_2) * count,
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            pShareInfo = (PSHARE_INFO_2)ptr;

            for (i=0; i < count; i++)
            {
                 pShareInfo[i] = ctr->ctr2->array[i];

                 pShareInfo[i].shi2_netname  = NULL;
                 pShareInfo[i].shi2_remark   = NULL;
                 pShareInfo[i].shi2_path     = NULL;
                 pShareInfo[i].shi2_password = NULL;

                 if (ctr->ctr2->array[i].shi2_netname)
                 {
                     status = SrvSvcAddDepStringW(
                                 pShareInfo,
                                 ctr->ctr2->array[i].shi2_netname,
                                 &pShareInfo[i].shi2_netname);
                     BAIL_ON_WIN_ERROR(status);
                 }
                 if (ctr->ctr2->array[i].shi2_remark)
                 {
                     status = SrvSvcAddDepStringW(
                                 pShareInfo,
                                 ctr->ctr2->array[i].shi2_remark,
                                 &pShareInfo[i].shi2_remark);
                     BAIL_ON_WIN_ERROR(status);
                 }
                 if (ctr->ctr2->array[i].shi2_path)
                 {
                     status = SrvSvcAddDepStringW(
                                 pShareInfo,
                                 ctr->ctr2->array[i].shi2_path,
                                 &pShareInfo[i].shi2_path);
                     BAIL_ON_WIN_ERROR(status);
                 }
                 if (ctr->ctr2->array[i].shi2_password)
                 {
                     status = SrvSvcAddDepStringW(
                                 pShareInfo,
                                 ctr->ctr2->array[i].shi2_password,
                                 &pShareInfo[i].shi2_password);
                     BAIL_ON_WIN_ERROR(status);
                 }
            }
        }
        break;
    case 501:
        if (ctr->ctr501) {
            PSHARE_INFO_501 pShareInfo;

            count = ctr->ctr501->count;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SHARE_INFO_501) * count,
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            pShareInfo = (PSHARE_INFO_501)ptr;

            for (i=0; i < count; i++)
            {
                 pShareInfo[i] = ctr->ctr501->array[i];

                 pShareInfo[i].shi501_netname = NULL;
                 pShareInfo[i].shi501_remark  = NULL;

                 if (ctr->ctr501->array[i].shi501_netname)
                 {
                     status = SrvSvcAddDepStringW(
                                 pShareInfo,
                                 ctr->ctr501->array[i].shi501_netname,
                                 &pShareInfo[i].shi501_netname);
                     BAIL_ON_WIN_ERROR(status);
                 }
                 if (ctr->ctr501->array[i].shi501_remark)
                 {
                     status = SrvSvcAddDepStringW(
                                 pShareInfo,
                                 ctr->ctr501->array[i].shi501_remark,
                                 &pShareInfo[i].shi501_remark);
                     BAIL_ON_WIN_ERROR(status);
                 }
            }
        }
        break;
    case 502:
        if (ctr->ctr502) {
            PSHARE_INFO_502 pShareInfoCopy;

            count = ctr->ctr502->count;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SHARE_INFO_502) * count,
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            pShareInfoCopy = (PSHARE_INFO_502)ptr;

            for (i=0; i < count; i++)
            {
                 PSHARE_INFO_502_I pShareInfo = &ctr->ctr502->array[i];

                 pShareInfoCopy[i].shi502_netname      = NULL;
                 pShareInfoCopy[i].shi502_type         =
                                             pShareInfo->shi502_type;
                 pShareInfoCopy[i].shi502_remark       = NULL;
                 pShareInfoCopy[i].shi502_permissions  =
                                             pShareInfo->shi502_permissions;
                 pShareInfoCopy[i].shi502_max_uses     =
                                             pShareInfo->shi502_max_uses;
                 pShareInfoCopy[i].shi502_current_uses =
                                             pShareInfo->shi502_current_uses;
                 pShareInfoCopy[i].shi502_path         = NULL;
                 pShareInfoCopy[i].shi502_password     = NULL;
                 pShareInfoCopy[i].shi502_reserved     =
                                             pShareInfo->shi502_reserved;

                 if (pShareInfo->shi502_reserved)
                 {
                     status = SrvSvcAllocateMemory(
                                     OUT_PPVOID(&pShareInfoCopy[i].shi502_security_descriptor),
                                     pShareInfo->shi502_reserved,
                                     pShareInfoCopy);
                     BAIL_ON_WIN_ERROR(status);
                 }

                 if (pShareInfo->shi502_netname)
                 {
                     status = SrvSvcAddDepStringW(
                                 pShareInfoCopy,
                                 pShareInfo->shi502_netname,
                                 &pShareInfoCopy[i].shi502_netname);
                     BAIL_ON_WIN_ERROR(status);
                 }
                 if (pShareInfo->shi502_remark)
                 {
                     status = SrvSvcAddDepStringW(
                                 pShareInfoCopy,
                                 pShareInfo->shi502_remark,
                                 &pShareInfoCopy[i].shi502_remark);
                     BAIL_ON_WIN_ERROR(status);
                 }
                 if (pShareInfo->shi502_path)
                 {
                     status = SrvSvcAddDepStringW(
                                 pShareInfoCopy,
                                 pShareInfo->shi502_path,
                                 &pShareInfoCopy[i].shi502_path);
                     BAIL_ON_WIN_ERROR(status);
                 }
                 if (pShareInfo->shi502_password)
                 {
                     status = SrvSvcAddDepStringW(
                                 pShareInfoCopy,
                                 pShareInfo->shi502_password,
                                 &pShareInfoCopy[i].shi502_password);
                     BAIL_ON_WIN_ERROR(status);
                 }

                 memcpy(pShareInfoCopy[i].shi502_security_descriptor,
                        pShareInfo->shi502_security_descriptor,
                        pShareInfo->shi502_reserved);
            }
        }
        break;
    }

    *entriesread = count;
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
