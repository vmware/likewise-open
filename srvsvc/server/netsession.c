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
 *        netsession.c
 *
 * Abstract:
 *
 *        Likewise Server Service (srvsvc) RPC client and server
 *
 *        SrvSvcNetSessionEnum (Server API)
 *        SrvSvcNetSessionDel  (Server API)
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

NET_API_STATUS
SrvSvcNetSessionEnum(
    handle_t           IDL_handle,           /* [in]      */
    PWSTR              pwszServername,       /* [in]      */
    PWSTR              pwszUncClientname,    /* [in]      */
    PWSTR              pwszUsername,         /* [in]      */
    DWORD              dwInfoLevel,          /* [in, out] */
    srvsvc_NetSessCtr* pInfo,                /* [in, out] */
    DWORD              dwPreferredMaxLength, /* [in]      */
    PDWORD             pdwEntriesRead,       /* [out]     */
    PDWORD             pdwTotalEntries,      /* [out]     */
    PDWORD             pdwResumeHandle       /* [in, out] */
    )
{
    DWORD     dwError  = 0;
    NTSTATUS  ntStatus = 0;
    PBYTE     pInBuffer   = NULL;
    DWORD     dwInLength  = 0;
    PBYTE     pOutBuffer  = NULL;
    DWORD     dwOutLength = 4096;
    wchar16_t       wszDriverName[] = SRV_DRIVER_NAME_W;
    IO_FILE_HANDLE  hFile           = NULL;
    IO_STATUS_BLOCK IoStatusBlock   = { 0 };
    IO_FILE_NAME    filename =
                        {
                              .RootFileHandle = NULL,
                              .FileName = &wszDriverName[0],
                              .IoNameOptions = 0
                        };
    IO_STATUS_BLOCK         ioStatusBlock       = {0};
    ACCESS_MASK             dwDesiredAccess     = 0;
    LONG64                  llAllocationSize    = 0;
    FILE_ATTRIBUTES         dwFileAttributes    = 0;
    FILE_SHARE_FLAGS        dwShareAccess       = 0;
    FILE_CREATE_DISPOSITION dwCreateDisposition = 0;
    FILE_CREATE_OPTIONS     dwCreateOptions     = 0;
    ULONG                   dwIoControlCode     = SRV_DEVCTL_ENUM_SESSIONS;
    SESSION_INFO_ENUM_IN_PARAMS sessionEnumParamsIn    =
    {
            .pwszServername       = pwszServername,
            .pwszUncClientname    = pwszUncClientname,
            .pwszUsername         = pwszUsername,
            .dwInfoLevel          = dwInfoLevel,
            .dwPreferredMaxLength = dwPreferredMaxLength,
            .pdwResumeHandle      = pdwResumeHandle ? pdwResumeHandle : NULL
    };
    PSESSION_INFO_ENUM_OUT_PREAMBLE pOutPreamble = NULL;
    PSESSION_INFO_UNION             pSessionInfo = NULL;
    srvsvc_NetSessCtr0*     ctr0 = NULL;
    srvsvc_NetSessCtr1*     ctr1= NULL;
    srvsvc_NetSessCtr2*     ctr2 = NULL;
    srvsvc_NetSessCtr10*    ctr10 = NULL;
    srvsvc_NetSessCtr502*   ctr502 = NULL;
    BOOLEAN                 bMoreDataAvailable = FALSE;

    ntStatus = LwSessionInfoMarshalEnumInputParameters(
                            &sessionEnumParamsIn,
                            &pInBuffer,
                            &dwInLength);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NtCreateFile(
                    &hFile,
                    NULL,
                    &ioStatusBlock,
                    &filename,
                    NULL,
                    NULL,
                    dwDesiredAccess,
                    llAllocationSize,
                    dwFileAttributes,
                    dwShareAccess,
                    dwCreateDisposition,
                    dwCreateOptions,
                    NULL,
                    0,
                    NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    dwError = LwAllocateMemory(dwOutLength, (void**)&pOutBuffer);
    BAIL_ON_SRVSVC_ERROR(dwError);

    ntStatus = NtDeviceIoControlFile(
                    hFile,
                    NULL,
                    &ioStatusBlock,
                    dwIoControlCode,
                    pInBuffer,
                    dwInLength,
                    pOutBuffer,
                    dwOutLength);

    while (ntStatus == STATUS_BUFFER_TOO_SMALL)
    {
        /* We need more space in output buffer to make this call */

        LW_SAFE_FREE_MEMORY(pOutBuffer);
        dwOutLength *= 2;

        dwError = LwAllocateMemory(dwOutLength, (void**)&pOutBuffer);
        BAIL_ON_SRVSVC_ERROR(dwError);

        ntStatus = NtDeviceIoControlFile(
                        hFile,
                        NULL,
                        &ioStatusBlock,
                        dwIoControlCode,
                        pInBuffer,
                        dwInLength,
                        pOutBuffer,
                        dwOutLength);
    }
    switch (ntStatus)
    {
        case STATUS_MORE_ENTRIES:

            bMoreDataAvailable = TRUE;

            // intentional fall through

        case STATUS_SUCCESS:

            ntStatus = LwSessionInfoUnmarshalEnumOutputParameters(
                            pOutBuffer,
                            dwOutLength,
                            &pOutPreamble,
                            &pSessionInfo);
            BAIL_ON_NT_STATUS(ntStatus);

            switch (pOutPreamble->dwInfoLevel)
            {
                case 0:

                    ctr0 = pInfo->ctr0;
                    ctr0->count = pOutPreamble->dwEntriesRead;

                    if (ctr0->count)
                    {
                        dwError = SrvSvcSrvAllocateMemory(
                                            sizeof(*ctr0->array) * ctr0->count,
                                            (void**)&ctr0->array);
                        BAIL_ON_SRVSVC_ERROR(dwError);

                        memcpy(
                            (void*)ctr0->array,
                            (void*)pSessionInfo->p0,
                               sizeof(*ctr0->array) * ctr0->count);
                    }

                    break;

                case 1:

                    ctr1 = pInfo->ctr1;
                    ctr1->count = pOutPreamble->dwEntriesRead;

                    if (ctr1->count)
                    {
                        dwError = SrvSvcSrvAllocateMemory(
                                            sizeof(*ctr1->array) * ctr1->count,
                                            (void**)&ctr1->array);
                        BAIL_ON_SRVSVC_ERROR(dwError);

                        memcpy(
                            (void*)ctr1->array,
                            (void*)pSessionInfo->p1,
                               sizeof(*ctr1->array) * ctr1->count);
                    }

                    break;

                case 2:

                    ctr2 = pInfo->ctr2;
                    ctr2->count = pOutPreamble->dwEntriesRead;

                    if (ctr2->count)
                    {
                        dwError = SrvSvcSrvAllocateMemory(
                                            sizeof(*ctr2->array) * ctr2->count,
                                            (void**)&ctr2->array);
                        BAIL_ON_SRVSVC_ERROR(dwError);

                        memcpy(
                            (void*)ctr2->array,
                            (void*)pSessionInfo->p2,
                               sizeof(*ctr2->array) * ctr2->count);
                    }

                    break;

                case 10:

                    ctr10 = pInfo->ctr10;
                    ctr10->count = pOutPreamble->dwEntriesRead;

                    if (ctr10->count)
                    {
                        dwError = SrvSvcSrvAllocateMemory(
                                        sizeof(*ctr10->array) * ctr10->count,
                                        (void**)&ctr10->array);
                        BAIL_ON_SRVSVC_ERROR(dwError);

                        memcpy(
                            (void*)ctr10->array,
                            (void*)pSessionInfo->p10,
                            sizeof(*ctr10->array) * ctr10->count);
                    }

                    break;

                case 502:

                    ctr502 = pInfo->ctr502;
                    ctr502->count = pOutPreamble->dwEntriesRead;

                    if (ctr502->count)
                    {
                        dwError = SrvSvcSrvAllocateMemory(
                                        sizeof(*ctr502->array) * ctr502->count,
                                        (void**)&ctr502->array);
                        BAIL_ON_SRVSVC_ERROR(dwError);

                        memcpy(
                            (void*)ctr502->array,
                            (void*)pSessionInfo->p502,
                            sizeof(*ctr502->array) * ctr502->count);
                    }

                    break;

                default:

                    ntStatus = STATUS_NOT_SUPPORTED;
                    break;
            }
            BAIL_ON_NT_STATUS(ntStatus);

            break;

        default:

            BAIL_ON_NT_STATUS(ntStatus);

            break;
    }

    *pdwEntriesRead  = pOutPreamble->dwEntriesRead;
    *pdwTotalEntries = pOutPreamble->dwTotalEntries;
    if (pdwResumeHandle)
    {
        *pdwResumeHandle = *pOutPreamble->pdwResumeHandle;
    }

    if (bMoreDataAvailable)
    {
        dwError = ERROR_MORE_DATA;
    }

cleanup:

    if (hFile)
    {
        NtCloseFile(hFile);
    }

    LW_SAFE_FREE_MEMORY(pInBuffer);
    LW_SAFE_FREE_MEMORY(pOutBuffer);

    if (pSessionInfo)
    {
        LwSessionInfoFree(
                pOutPreamble->dwInfoLevel,
                pOutPreamble->dwEntriesRead,
                pSessionInfo);
    }
    if (pOutPreamble)
    {
        LwSessionInfoFreeEnumOutPreamble(pOutPreamble);
    }

    return dwError;

error:

    if (pSessionInfo && pInfo)
    {
        switch (pOutPreamble->dwInfoLevel)
        {
            case 0:

                if (ctr0 && ctr0->array)
                {
                    SrvSvcSrvFreeMemory(ctr0->array);
                }
                break;

            case 1:


                if (ctr1 && ctr1->array)
                {
                    SrvSvcSrvFreeMemory(ctr1->array);
                }
                break;

            case 2:

                if (ctr2 && ctr2->array)
                {
                    SrvSvcSrvFreeMemory(ctr2->array);
                }
                break;

            case 10:

                if (ctr10 && ctr10->array)
                {
                    SrvSvcSrvFreeMemory(ctr10->array);
                }
                break;

            case 502:

                if (ctr502 && ctr502->array)
                {
                    SrvSvcSrvFreeMemory(ctr502->array);
                }
                break;

            default:

                SRVSVC_LOG_ERROR("Unsupported info level [%u]",
                                 pOutPreamble->dwInfoLevel);
                break;
        }
    }

    switch (ntStatus)
    {
        case STATUS_SUCCESS:

            break;

        case STATUS_INVALID_COMPUTER_NAME:

            dwError = NERR_InvalidComputer;

            break;

        default:

            dwError = LwNtStatusToWin32Error(ntStatus);

            break;
    }

    if (pInfo)
    {
        memset(pInfo, 0, sizeof(*pInfo));
    }

    if (pdwEntriesRead)
    {
        *pdwEntriesRead = 0;
    }
    if (pdwTotalEntries)
    {
        *pdwTotalEntries = 0;
    }

    goto cleanup;
}

NET_API_STATUS
SrvSvcNetSessionDel(
    handle_t IDL_handle,        /* [in] */
    PWSTR    pwszServername,    /* [in] */
    PWSTR    pwszUncClientname, /* [in] */
    PWSTR    pwszUsername       /* [in] */
    )
{
    DWORD     dwError  = 0;
    NTSTATUS  ntStatus = 0;
    wchar16_t       wszDriverName[] = SRV_DRIVER_NAME_W;
    IO_FILE_HANDLE  hFile           = NULL;
    IO_STATUS_BLOCK IoStatusBlock   = { 0 };
    IO_FILE_NAME    filename =
                        {
                              .RootFileHandle = NULL,
                              .FileName = &wszDriverName[0],
                              .IoNameOptions = 0
                        };
    ACCESS_MASK             dwDesiredAccess     = 0;
    LONG64                  llAllocationSize    = 0;
    FILE_ATTRIBUTES         dwFileAttributes    = 0;
    FILE_SHARE_FLAGS        dwShareAccess       = 0;
    FILE_CREATE_DISPOSITION dwCreateDisposition = 0;
    FILE_CREATE_OPTIONS     dwCreateOptions     = 0;
    ULONG                   dwIoControlCode     = SRV_DEVCTL_DELETE_SESSION;
    SESSION_INFO_DELETE_PARAMS deleteParams =
    {
        .pwszServername    = pwszServername,
        .pwszUncClientname = pwszUncClientname,
        .pwszUncUsername   = pwszUsername
    };
    PBYTE                    pInBuffer = NULL;
    DWORD                    dwInBufferLength = 0;
    PBYTE                    pOutBuffer = NULL;
    DWORD                    dwOutBufferLength = 0;

    ntStatus = NtCreateFile(
                    &hFile,
                    NULL,
                    &IoStatusBlock,
                    &filename,
                    NULL,
                    NULL,
                    dwDesiredAccess,
                    llAllocationSize,
                    dwFileAttributes,
                    dwShareAccess,
                    dwCreateDisposition,
                    dwCreateOptions,
                    NULL,
                    0,
                    NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwSessionInfoMarshalDeleteParameters(
                    &deleteParams,
                    &pInBuffer,
                    &dwInBufferLength);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NtDeviceIoControlFile(
                    hFile,
                    NULL,
                    &IoStatusBlock,
                    dwIoControlCode,
                    pInBuffer,
                    dwInBufferLength,
                    pOutBuffer,
                    dwOutBufferLength);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    if (hFile)
    {
        NtCloseFile(hFile);
    }

    LW_SAFE_FREE_MEMORY(pInBuffer);
    LW_SAFE_FREE_MEMORY(pOutBuffer);

    return dwError;

error:

    if (ntStatus != STATUS_SUCCESS)
    {
        dwError = LwNtStatusToWin32Error(ntStatus);
    }

    goto cleanup;
}
