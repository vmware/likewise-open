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
SrvSvcCopyNetFileCtr(
    DWORD              dwInfoLevel,
    srvsvc_NetFileCtr* ctr,
    PDWORD             pdwEntriesRead,
    PBYTE*             ppBuffer
    );

NET_API_STATUS
NetrFileEnum(
    PSRVSVC_CONTEXT pContext,
    PCWSTR          pwszServername,
    PCWSTR          pwszBasepath,
    PCWSTR          pwszUsername,
    UINT32          dwInfoLevel,
    PBYTE*          ppBuffer,
    DWORD           dwPrefmaxlen,
    PDWORD          pdwEntriesRead,
    PDWORD          pdwTotalEntries,
    PDWORD          pdwResumeHandle
    )
{
    NET_API_STATUS status = ERROR_SUCCESS;
    dcethread_exc* pDceException  = NULL;
    DWORD dwInfoLevel2   = dwInfoLevel;
    DWORD dwEntriesRead  = 0;
    DWORD dwTotalEntries = 0;
    DWORD dwResumeHandle = (pdwResumeHandle ? *pdwResumeHandle : 0);
    PBYTE pBuffer        = NULL;
    BOOLEAN bMoreDataAvailable = FALSE;
    srvsvc_NetFileCtr  ctr;
    srvsvc_NetFileCtr2 ctr2;
    srvsvc_NetFileCtr3 ctr3;

    memset(&ctr, 0, sizeof(ctr));
    memset(&ctr2, 0, sizeof(ctr2));
    memset(&ctr3, 0, sizeof(ctr3));

    BAIL_ON_INVALID_PTR(pContext, status);
    BAIL_ON_INVALID_PTR(ppBuffer, status);
    BAIL_ON_INVALID_PTR(pdwEntriesRead, status);
    BAIL_ON_INVALID_PTR(pdwTotalEntries, status);

    switch (dwInfoLevel)
    {
        case 2:

            ctr.ctr2 = &ctr2;
            break;

        case 3:

            ctr.ctr3 = &ctr3;
            break;

        default:

            status = ERROR_INVALID_LEVEL;
            BAIL_ON_WIN_ERROR(status);

            break;
    }

    TRY
    {
        status = _NetrFileEnum(
                    pContext->hBinding,
                    (PWSTR)pwszServername,
                    (PWSTR)pwszBasepath,
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

            status = SrvSvcCopyNetFileCtr(
                            dwInfoLevel,
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

    switch (dwInfoLevel)
    {
        case 2:
            if (ctr.ctr2 == &ctr2) {
                ctr.ctr2 = NULL;
            }
            break;
        case 3:
            if (ctr.ctr3 == &ctr3) {
                ctr.ctr3 = NULL;
            }
            break;
    }

    SrvSvcClearNetFileCtr(dwInfoLevel2, &ctr);

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
SrvSvcCopyNetFileCtr(
    DWORD              dwInfoLevel,
    srvsvc_NetFileCtr* ctr,
    PDWORD             pdwEntriesRead,
    PBYTE*             ppBuffer
    )
{
    NET_API_STATUS status = ERROR_SUCCESS;
    int i;
    DWORD dwEntriesRead = 0;
    void *pBuffer = NULL;

    BAIL_ON_INVALID_PTR(ppBuffer, status);
    BAIL_ON_INVALID_PTR(ctr, status);

    switch (dwInfoLevel)
    {
    case 2:

        if (ctr->ctr2)
        {
            PFILE_INFO_2 a2;

            dwEntriesRead = ctr->ctr2->count;

            status = SrvSvcAllocateMemory(&pBuffer,
                                          sizeof(FILE_INFO_2) * dwEntriesRead,
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a2 = (PFILE_INFO_2)pBuffer;

            for (i=0; i < dwEntriesRead; i++) {
                 a2[i] = ctr->ctr2->array[i];
            }
        }
        break;
    case 3:
        if (ctr->ctr3) {
            PFILE_INFO_3 a3;

            dwEntriesRead = ctr->ctr3->count;

            status = SrvSvcAllocateMemory(&pBuffer,
                                          sizeof(FILE_INFO_3) * dwEntriesRead,
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a3 = (PFILE_INFO_3)pBuffer;

            for (i=0; i < dwEntriesRead; i++)
            {
                 a3[i] = ctr->ctr3->array[i];

                 if (a3[i].fi3_path_name)
                 {
                     status = SrvSvcAddDepStringW(a3, a3[i].fi3_path_name);
                     BAIL_ON_WIN_ERROR(status);
                 }
                 if (a3[i].fi3_username)
                 {
                     status = SrvSvcAddDepStringW(a3, a3[i].fi3_username);
                     BAIL_ON_WIN_ERROR(status);
                 }
            }
        }
        break;
    }

    *pdwEntriesRead = dwEntriesRead;
    *ppBuffer = (PBYTE)pBuffer;

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

    if (pBuffer)
    {
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
