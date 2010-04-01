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
    UINT32             dwInfoLevel,
    srvsvc_NetSessCtr* ctr,
    UINT32*            pdwEntriesRead,
    UINT8**            ppBuffer
    );

NET_API_STATUS
NetrSessionEnum(
    PSRVSVC_CONTEXT pContext,
    PCWSTR          pwszServername,
    PCWSTR          pwszUncClientname,
    PCWSTR          pwszUsername,
    DWORD           dwInfoLevel,
    PBYTE*          ppBuffer,
    DWORD           dwPrefmaxlen,
    PDWORD          pdwEntriesRead,
    PDWORD          pdwTotalEntries,
    PDWORD          pdwResumeHandle
    )
{
    NET_API_STATUS status         = ERROR_SUCCESS;
    dcethread_exc* pDceException  = NULL;
    DWORD   dwInfoLevel2          = dwInfoLevel;
    BOOLEAN bMoreDataAvailable    = FALSE;
    DWORD   dwEntriesRead         = 0;
    DWORD   dwTotalEntries        = 0;
    DWORD   dwResumeHandle        = pdwResumeHandle ? *pdwResumeHandle : 0;
    PBYTE   pBuffer               = NULL;
    srvsvc_NetSessCtr    ctr;
    srvsvc_NetSessCtr0   ctr0;
    srvsvc_NetSessCtr1   ctr1;
    srvsvc_NetSessCtr2   ctr2;
    srvsvc_NetSessCtr10  ctr10;
    srvsvc_NetSessCtr502 ctr502;

    memset(&ctr,    0, sizeof(ctr));
    memset(&ctr0,   0, sizeof(ctr0));
    memset(&ctr1,   0, sizeof(ctr1));
    memset(&ctr2,   0, sizeof(ctr2));
    memset(&ctr10,  0, sizeof(ctr10));
    memset(&ctr502, 0, sizeof(ctr502));

    BAIL_ON_INVALID_PTR(pContext, status);
    BAIL_ON_INVALID_PTR(ppBuffer, status);
    BAIL_ON_INVALID_PTR(pdwEntriesRead, status);
    BAIL_ON_INVALID_PTR(pdwTotalEntries, status);

    switch (dwInfoLevel)
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
        case 10:
            ctr.ctr10 = &ctr10;
            break;
        case 502:
            ctr.ctr502 = &ctr502;
            break;
        default:
            status = ERROR_INVALID_LEVEL;
            BAIL_ON_WIN_ERROR(status);
    }

    TRY
    {
        status = _NetrSessionEnum(
                        pContext->hBinding,
                        (PWSTR)pwszServername,
                        (PWSTR)pwszUncClientname,
                        (PWSTR)pwszUsername,
                        &dwInfoLevel2,
                        &ctr,
                        dwPrefmaxlen,
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

            if (dwInfoLevel2 != dwInfoLevel)
            {
                status = ERROR_BAD_NET_RESP;
                BAIL_ON_WIN_ERROR(status);
            }

            status = SrvSvcCopyNetSessCtr(
                            dwInfoLevel2,
                            &ctr,
                            &dwEntriesRead,
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

    *pdwEntriesRead  = dwEntriesRead;
    *pdwTotalEntries = dwTotalEntries;
    if (pdwResumeHandle)
    {
        *pdwResumeHandle = dwResumeHandle;
    }
    *ppBuffer        = pBuffer;

cleanup:

    switch (dwInfoLevel2)
    {
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
    SrvSvcClearNetSessCtr(dwInfoLevel2, &ctr);

    return status;

error:

    *pdwEntriesRead  = 0;
    *pdwTotalEntries = 0;
    *ppBuffer        = NULL;

    if (pBuffer)
    {
        SrvSvcFreeMemory(pBuffer);
    }

    goto cleanup;
}

static
NET_API_STATUS
SrvSvcCopyNetSessCtr(
    DWORD              dwInfoLevel,
    srvsvc_NetSessCtr* ctr,
    PDWORD             pdwEntriesRead,
    PBYTE*             ppBuffer
    )
{
    NET_API_STATUS status = ERROR_SUCCESS;
    int i;
    int dwEntriesRead = 0;
    void *pBuffer = NULL;

    BAIL_ON_INVALID_PTR(pdwEntriesRead, status);
    BAIL_ON_INVALID_PTR(ppBuffer, status);
    BAIL_ON_INVALID_PTR(ctr, status);

    switch (dwInfoLevel)
    {
    case 0:
        if (ctr->ctr0) {
            PSESSION_INFO_0 pSessionInfo;

            dwEntriesRead = ctr->ctr0->count;

            status = SrvSvcAllocateMemory(&pBuffer,
                                          sizeof(SESSION_INFO_0) * dwEntriesRead,
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            pSessionInfo = (PSESSION_INFO_0)pBuffer;

            for (i=0; i < dwEntriesRead; i++)
            {
                 pSessionInfo[i] = ctr->ctr0->array[i];

                 pSessionInfo[i].sesi0_cname = NULL;

                 if (ctr->ctr0->array[i].sesi0_cname)
                 {
                     status = SrvSvcAddDepStringW(
                                 pSessionInfo,
                                 ctr->ctr0->array[i].sesi0_cname,
                                 &pSessionInfo[i].sesi0_cname);
                     BAIL_ON_WIN_ERROR(status);
                 }
            }
        }
        break;
    case 1:
        if (ctr->ctr1) {
            PSESSION_INFO_1 pSessionInfo;

            dwEntriesRead = ctr->ctr1->count;

            status = SrvSvcAllocateMemory(&pBuffer,
                                          sizeof(SESSION_INFO_1) * dwEntriesRead,
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            pSessionInfo = (PSESSION_INFO_1)pBuffer;

            for (i=0; i < dwEntriesRead; i++)
            {
                 pSessionInfo[i] = ctr->ctr1->array[i];

                 pSessionInfo[i].sesi1_cname    = NULL;
                 pSessionInfo[i].sesi1_username = NULL;

                 if (ctr->ctr1->array[i].sesi1_cname)
                 {
                     status = SrvSvcAddDepStringW(
                                 pSessionInfo,
                                 ctr->ctr1->array[i].sesi1_cname,
                                 &pSessionInfo[i].sesi1_cname);
                     BAIL_ON_WIN_ERROR(status);
                 }
                 if (ctr->ctr1->array[i].sesi1_username)
                 {
                     status = SrvSvcAddDepStringW(
                                 pSessionInfo,
                                 ctr->ctr1->array[i].sesi1_username,
                                 &pSessionInfo[i].sesi1_username);
                     BAIL_ON_WIN_ERROR(status);
                 }
            }
        }
        break;
    case 2:
        if (ctr->ctr2) {
            PSESSION_INFO_2 pSessionInfo;

            dwEntriesRead = ctr->ctr2->count;

            status = SrvSvcAllocateMemory(&pBuffer,
                                          sizeof(SESSION_INFO_2) * dwEntriesRead,
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            pSessionInfo = (PSESSION_INFO_2)pBuffer;

            for (i=0; i < dwEntriesRead; i++)
            {
                 pSessionInfo[i] = ctr->ctr2->array[i];

                 pSessionInfo[i].sesi2_cname       = NULL;
                 pSessionInfo[i].sesi2_username    = NULL;
                 pSessionInfo[i].sesi2_cltype_name = NULL;

                 if (ctr->ctr2->array[i].sesi2_cname)
                 {
                     status = SrvSvcAddDepStringW(
                                 pSessionInfo,
                                 ctr->ctr2->array[i].sesi2_cname,
                                 &pSessionInfo[i].sesi2_cname);
                     BAIL_ON_WIN_ERROR(status);
                 }
                 if (ctr->ctr2->array[i].sesi2_username)
                 {
                     status = SrvSvcAddDepStringW(
                                 pSessionInfo,
                                 ctr->ctr2->array[i].sesi2_username,
                                 &pSessionInfo[i].sesi2_username);
                     BAIL_ON_WIN_ERROR(status);
                 }
                 if (ctr->ctr2->array[i].sesi2_cltype_name)
                 {
                     status = SrvSvcAddDepStringW(
                                 pSessionInfo,
                                 ctr->ctr2->array[i].sesi2_cltype_name,
                                 &pSessionInfo[i].sesi2_cltype_name);
                     BAIL_ON_WIN_ERROR(status);
                 }
            }
        }
        break;
    case 10:
        if (ctr->ctr10) {
            PSESSION_INFO_10 pSessionInfo;

            dwEntriesRead = ctr->ctr10->count;

            status = SrvSvcAllocateMemory(&pBuffer,
                                          sizeof(SESSION_INFO_10) * dwEntriesRead,
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            pSessionInfo = (PSESSION_INFO_10)pBuffer;

            for (i=0; i < dwEntriesRead; i++)
            {
                 pSessionInfo[i] = ctr->ctr10->array[i];

                 pSessionInfo[i].sesi10_cname    = NULL;
                 pSessionInfo[i].sesi10_username = NULL;

                 if (ctr->ctr10->array[i].sesi10_cname)
                 {
                     status = SrvSvcAddDepStringW(
                                 pSessionInfo,
                                 ctr->ctr10->array[i].sesi10_cname,
                                 &pSessionInfo[i].sesi10_cname);
                     BAIL_ON_WIN_ERROR(status);
                 }
                 if (ctr->ctr10->array[i].sesi10_username)
                 {
                     status = SrvSvcAddDepStringW(
                                 pSessionInfo,
                                 ctr->ctr10->array[i].sesi10_username,
                                 &pSessionInfo[i].sesi10_username);
                     BAIL_ON_WIN_ERROR(status);
                 }
            }
        }
        break;
    case 502:
        if (ctr->ctr502) {
            PSESSION_INFO_502 pSessionInfo;

            dwEntriesRead = ctr->ctr502->count;

            status = SrvSvcAllocateMemory(&pBuffer,
                                          sizeof(SESSION_INFO_502) * dwEntriesRead,
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            pSessionInfo = (PSESSION_INFO_502)pBuffer;

            for (i=0; i < dwEntriesRead; i++)
            {
                 pSessionInfo[i] = ctr->ctr502->array[i];

                 pSessionInfo[i].sesi502_cname       = NULL;
                 pSessionInfo[i].sesi502_username    = NULL;
                 pSessionInfo[i].sesi502_cltype_name = NULL;
                 pSessionInfo[i].sesi502_transport   = NULL;

                 if (ctr->ctr502->array[i].sesi502_cname)
                 {
                     status = SrvSvcAddDepStringW(
                                 pSessionInfo,
                                 ctr->ctr502->array[i].sesi502_cname,
                                 &pSessionInfo[i].sesi502_cname);
                     BAIL_ON_WIN_ERROR(status);
                 }
                 if (ctr->ctr502->array[i].sesi502_username)
                 {
                     status = SrvSvcAddDepStringW(
                                 pSessionInfo,
                                 ctr->ctr502->array[i].sesi502_username,
                                 &pSessionInfo[i].sesi502_username);
                     BAIL_ON_WIN_ERROR(status);
                 }
                 if (ctr->ctr502->array[i].sesi502_cltype_name)
                 {
                     status = SrvSvcAddDepStringW(
                                 pSessionInfo,
                                 ctr->ctr502->array[i].sesi502_cltype_name,
                                 &pSessionInfo[i].sesi502_cltype_name);
                     BAIL_ON_WIN_ERROR(status);
                 }
                 if (ctr->ctr502->array[i].sesi502_transport)
                 {
                     status = SrvSvcAddDepStringW(
                                 pSessionInfo,
                                 ctr->ctr502->array[i].sesi502_transport,
                                 &pSessionInfo[i].sesi502_transport);
                     BAIL_ON_WIN_ERROR(status);
                 }
            }
        }
        break;
    }

    *pdwEntriesRead = dwEntriesRead;
    *ppBuffer = (UINT8 *)pBuffer;

cleanup:

    return status;

error:

    if (pdwEntriesRead)
    {
        *pdwEntriesRead = 0;
    }
    if (ppBuffer)
    {
        *ppBuffer = NULL;
    }

    if (pBuffer) {
        SrvSvcFreeMemory(pBuffer);
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
