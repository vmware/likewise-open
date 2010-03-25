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
SrvSvcCopyNetSessCtr(
    UINT32             level,
    srvsvc_NetSessCtr* ctr,
    UINT32*            entriesread,
    UINT8**            bufptr
    );

NET_API_STATUS
NetrSessionEnum(
    PSRVSVC_CONTEXT pContext,
    const wchar16_t *servername,
    const wchar16_t *unc_client_name,
    const wchar16_t *username,
    UINT32 level,
    UINT8 **bufptr,
    UINT32 prefmaxlen,
    UINT32 *entriesread,
    UINT32 *totalentries,
    UINT32 *resume_handle
    )
{
    NET_API_STATUS status = ERROR_SUCCESS;
    dcethread_exc* pDceException  = NULL;
    srvsvc_NetSessCtr ctr;
    srvsvc_NetSessCtr0 ctr0;
    srvsvc_NetSessCtr1 ctr1;
    srvsvc_NetSessCtr2 ctr2;
    srvsvc_NetSessCtr10 ctr10;
    srvsvc_NetSessCtr502 ctr502;
    UINT32 l = level;

    BAIL_ON_INVALID_PTR(pContext, status);
    BAIL_ON_INVALID_PTR(bufptr, status);
    BAIL_ON_INVALID_PTR(entriesread, status);
    BAIL_ON_INVALID_PTR(totalentries, status);

    memset(&ctr, 0, sizeof(ctr));
    memset(&ctr0, 0, sizeof(ctr0));
    memset(&ctr1, 0, sizeof(ctr1));
    memset(&ctr2, 0, sizeof(ctr2));
    memset(&ctr10, 0, sizeof(ctr10));
    memset(&ctr502, 0, sizeof(ctr502));

    *entriesread = 0;
    *bufptr = NULL;

    switch (level) {
    case 0:
        ctr.ctr0 = &ctr0;
        break;
    case 1:
        ctr.ctr1 = &ctr1;
        break;
    case 2:
        ctr.ctr2 = &ctr2;
        break;
    case 10:
        ctr.ctr10 = &ctr10;
        break;
    case 502:
        ctr.ctr502 = &ctr502;
        break;
    }

    TRY
    {
        status = _NetrSessionEnum(
                        pContext->hBinding,
                        (wchar16_t *)servername,
                        (wchar16_t *)unc_client_name,
                        (wchar16_t *)username,
                        &l,
                        &ctr,
                        prefmaxlen,
                        totalentries,
                        resume_handle);
    }
    CATCH_ALL(pDceException)
    {
        NTSTATUS ntStatus = LwRpcStatusToNtStatus(pDceException->match.value);
        status = LwNtStatusToWin32Error(ntStatus);
    }
    ENDTRY;
    BAIL_ON_WIN_ERROR(status);

    if (l != level) {
        status = ERROR_BAD_NET_RESP;
        BAIL_ON_WIN_ERROR(status);
    }

    status = SrvSvcCopyNetSessCtr(l, &ctr, entriesread, bufptr);
    BAIL_ON_WIN_ERROR(status);

cleanup:
    switch (level) {
    case 0:
        if (ctr.ctr0 == &ctr0) {
            ctr.ctr0 = NULL;
        }
        break;
    case 1:
        if (ctr.ctr1 == &ctr1) {
            ctr.ctr1 = NULL;
        }
        break;
    case 2:
        if (ctr.ctr2 == &ctr2) {
            ctr.ctr2 = NULL;
        }
        break;
    case 10:
        if (ctr.ctr10 == &ctr10) {
            ctr.ctr10 = NULL;
        }
        break;
    case 502:
        if (ctr.ctr502 == &ctr502) {
            ctr.ctr502 = NULL;
        }
        break;
    }
    SrvSvcClearNetSessCtr(l, &ctr);

    return status;

error:
    goto cleanup;
}

static
NET_API_STATUS
SrvSvcCopyNetSessCtr(
    UINT32             level,
    srvsvc_NetSessCtr* ctr,
    UINT32*            entriesread,
    UINT8**            bufptr
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
            PSESSION_INFO_0 a0;

            count = ctr->ctr0->count;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SESSION_INFO_0) * count,
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a0 = (PSESSION_INFO_0)ptr;

            for (i=0; i < count; i++)
            {
                 a0[i] = ctr->ctr0->array[i];

                 if (a0[i].sesi0_cname)
                 {
                     status = SrvSvcAddDepStringW(a0, a0[i].sesi0_cname);
                     BAIL_ON_WIN_ERROR(status);
                 }
            }
        }
        break;
    case 1:
        if (ctr->ctr1) {
            PSESSION_INFO_1 a1;

            count = ctr->ctr1->count;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SESSION_INFO_1) * count,
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a1 = (PSESSION_INFO_1)ptr;

            for (i=0; i < count; i++) {
                 a1[i] = ctr->ctr1->array[i];

                 if (a1[i].sesi1_cname)
                 {
                     status = SrvSvcAddDepStringW(a1, a1[i].sesi1_cname);
                     BAIL_ON_WIN_ERROR(status);
                 }
                 if (a1[i].sesi1_username)
                 {
                     status = SrvSvcAddDepStringW(a1, a1[i].sesi1_username);
                     BAIL_ON_WIN_ERROR(status);
                 }
            }
        }
        break;
    case 2:
        if (ctr->ctr2) {
            PSESSION_INFO_2 a2;

            count = ctr->ctr2->count;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SESSION_INFO_2) * count,
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a2 = (PSESSION_INFO_2)ptr;

            for (i=0; i < count; i++) {
                 a2[i] = ctr->ctr2->array[i];

                 if (a2[i].sesi2_cname)
                 {
                     status = SrvSvcAddDepStringW(a2, a2[i].sesi2_cname);
                     BAIL_ON_WIN_ERROR(status);
                 }
                 if (a2[i].sesi2_username)
                 {
                     status = SrvSvcAddDepStringW(a2, a2[i].sesi2_username);
                     BAIL_ON_WIN_ERROR(status);
                 }
                 if (a2[i].sesi2_cltype_name)
                 {
                     status = SrvSvcAddDepStringW(a2, a2[i].sesi2_cltype_name);
                     BAIL_ON_WIN_ERROR(status);
                 }
            }
        }
        break;
    case 10:
        if (ctr->ctr10) {
            PSESSION_INFO_10 a10;

            count = ctr->ctr10->count;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SESSION_INFO_10) * count,
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a10 = (PSESSION_INFO_10)ptr;

            for (i=0; i < count; i++) {
                 a10[i] = ctr->ctr10->array[i];

                 if (a10[i].sesi10_cname)
                 {
                     status = SrvSvcAddDepStringW(a10, a10[i].sesi10_cname);
                     BAIL_ON_WIN_ERROR(status);
                 }
                 if (a10[i].sesi10_username)
                 {
                     status = SrvSvcAddDepStringW(a10, a10[i].sesi10_username);
                     BAIL_ON_WIN_ERROR(status);
                 }
            }
        }
        break;
    case 502:
        if (ctr->ctr502) {
            PSESSION_INFO_502 a502;

            count = ctr->ctr502->count;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(SESSION_INFO_502) * count,
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a502 = (PSESSION_INFO_502)ptr;

            for (i=0; i < count; i++)
            {
                 a502[i] = ctr->ctr502->array[i];

                 if (a502[i].sesi502_cname)
                 {
                     status = SrvSvcAddDepStringW(a502, a502[i].sesi502_cname);
                     BAIL_ON_WIN_ERROR(status);
                 }
                 if (a502[i].sesi502_username)
                 {
                     status = SrvSvcAddDepStringW(a502, a502[i].sesi502_username);
                     BAIL_ON_WIN_ERROR(status);
                 }
                 if (a502[i].sesi502_cltype_name)
                 {
                     status = SrvSvcAddDepStringW(a502, a502[i].sesi502_cltype_name);
                     BAIL_ON_WIN_ERROR(status);
                 }
                 if (a502[i].sesi502_transport)
                 {
                     status = SrvSvcAddDepStringW(a502, a502[i].sesi502_transport);
                     BAIL_ON_WIN_ERROR(status);
                 }
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
