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

static
DWORD
SrvSvcMarshalSessionInfoResults(
    PSESSION_INFO_ENUM_OUT_PREAMBLE pOutPreamble, /* IN     */
    PSESSION_INFO_UNION             pSessionInfo, /* IN     */
    srvsvc_NetSessCtr*              pInfo         /* IN OUT */
    );

static
DWORD
SrvSvcFreeSessionInfoResults(
    DWORD              dwInfoLevel,
    srvsvc_NetSessCtr* pInfo
    );

static
DWORD
SrvSvcSrvMarshalSessionInfo_level_0(
    PSESSION_INFO_0  pSessionInfoIn,
    DWORD            dwNumEntries,
    PSESSION_INFO_0* ppSessionInfoOut,
    PDWORD           pdwNumEntries
    );

static
VOID
SrvSvcSrvFreeSessionInfo_level_0(
    PSESSION_INFO_0 pSessionInfo,
    DWORD           dwNumEntries
    );

static
DWORD
SrvSvcSrvMarshalSessionInfo_level_1(
    PSESSION_INFO_1  pSessionInfoIn,
    DWORD            dwNumEntries,
    PSESSION_INFO_1* ppSessionInfoOut,
    PDWORD           pdwNumEntries
    );

static
VOID
SrvSvcSrvFreeSessionInfo_level_1(
    PSESSION_INFO_1 pSessionInfo,
    DWORD           dwNumEntries
    );

static
DWORD
SrvSvcSrvMarshalSessionInfo_level_2(
    PSESSION_INFO_2  pSessionInfoIn,
    DWORD            dwNumEntries,
    PSESSION_INFO_2* ppSessionInfoOut,
    PDWORD           pdwNumEntries
    );

static
VOID
SrvSvcSrvFreeSessionInfo_level_2(
    PSESSION_INFO_2 pSessionInfo,
    DWORD           dwNumEntries
    );

static
DWORD
SrvSvcSrvMarshalSessionInfo_level_10(
    PSESSION_INFO_10  pSessionInfoIn,
    DWORD             dwNumEntries,
    PSESSION_INFO_10* ppSessionInfoOut,
    PDWORD            pdwNumEntries
    );

static
VOID
SrvSvcSrvFreeSessionInfo_level_10(
    PSESSION_INFO_10 pSessionInfo,
    DWORD            dwNumEntries
    );

static
DWORD
SrvSvcSrvMarshalSessionInfo_level_502(
    PSESSION_INFO_502  pSessionInfoIn,
    DWORD              dwNumEntries,
    PSESSION_INFO_502* ppSessionInfoOut,
    PDWORD             pdwNumEntries
    );

static
VOID
SrvSvcSrvFreeSessionInfo_level_502(
    PSESSION_INFO_502 pSessionInfo,
    DWORD             dwNumEntries
    );

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
    BOOLEAN bMoreDataAvailable = FALSE;

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

            dwError = SrvSvcMarshalSessionInfoResults(
                            pOutPreamble,
                            pSessionInfo,
                            pInfo);
            BAIL_ON_SRVSVC_ERROR(dwError);

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

    if (pInfo)
    {
        SrvSvcFreeSessionInfoResults(pOutPreamble->dwInfoLevel, pInfo);

        memset(pInfo, 0, sizeof(*pInfo));
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

static
DWORD
SrvSvcMarshalSessionInfoResults(
    PSESSION_INFO_ENUM_OUT_PREAMBLE pOutPreamble, /* IN     */
    PSESSION_INFO_UNION             pSessionInfo, /* IN     */
    srvsvc_NetSessCtr*              pInfo         /* IN OUT */
    )
{
    DWORD dwError = 0;

    if (pOutPreamble->dwEntriesRead)
    {
        switch (pOutPreamble->dwInfoLevel)
        {
            case 0:

                dwError = SrvSvcSrvMarshalSessionInfo_level_0(
                                pSessionInfo->p0,
                                pOutPreamble->dwEntriesRead,
                                &pInfo->ctr0->array,
                                &pInfo->ctr0->count);

                break;

            case 1:

                dwError = SrvSvcSrvMarshalSessionInfo_level_1(
                                pSessionInfo->p1,
                                pOutPreamble->dwEntriesRead,
                                &pInfo->ctr1->array,
                                &pInfo->ctr1->count);

                break;

            case 2:

                dwError = SrvSvcSrvMarshalSessionInfo_level_2(
                                pSessionInfo->p2,
                                pOutPreamble->dwEntriesRead,
                                &pInfo->ctr2->array,
                                &pInfo->ctr2->count);

                break;

            case 10:

                dwError = SrvSvcSrvMarshalSessionInfo_level_10(
                                pSessionInfo->p10,
                                pOutPreamble->dwEntriesRead,
                                &pInfo->ctr10->array,
                                &pInfo->ctr10->count);

                break;

            case 502:

                dwError = SrvSvcSrvMarshalSessionInfo_level_502(
                                pSessionInfo->p502,
                                pOutPreamble->dwEntriesRead,
                                &pInfo->ctr502->array,
                                &pInfo->ctr502->count);

                break;

            default:

                dwError = ERROR_INVALID_LEVEL;
                break;
        }
    }

    return dwError;
}

static
DWORD
SrvSvcFreeSessionInfoResults(
    DWORD              dwInfoLevel,
    srvsvc_NetSessCtr* pInfo
    )
{
    switch (dwInfoLevel)
    {
        case 0:

            if (pInfo->ctr0 && pInfo->ctr0->array)
            {
                SrvSvcSrvFreeSessionInfo_level_0(
                        pInfo->ctr0->array,
                        pInfo->ctr0->count);
            }
            break;

        case 1:


            if (pInfo->ctr1 && pInfo->ctr1->array)
            {
                SrvSvcSrvFreeSessionInfo_level_1(
                                            pInfo->ctr1->array,
                                            pInfo->ctr1->count);
            }
            break;

        case 2:

            if (pInfo->ctr2 && pInfo->ctr2->array)
            {
                SrvSvcSrvFreeSessionInfo_level_2(
                                            pInfo->ctr2->array,
                                            pInfo->ctr2->count);
            }
            break;

        case 10:

            if (pInfo->ctr10 && pInfo->ctr10->array)
            {
                SrvSvcSrvFreeSessionInfo_level_10(
                                            pInfo->ctr10->array,
                                            pInfo->ctr10->count);
            }
            break;

        case 502:

            if (pInfo->ctr502 && pInfo->ctr502->array)
            {
                SrvSvcSrvFreeSessionInfo_level_502(
                                            pInfo->ctr502->array,
                                            pInfo->ctr502->count);
            }
            break;

        default:

            SRVSVC_LOG_ERROR("Unsupported info level [%u]", dwInfoLevel);

            break;
    }
}

static
DWORD
SrvSvcSrvMarshalSessionInfo_level_0(
    PSESSION_INFO_0  pSessionInfoIn,
    DWORD            dwNumEntries,
    PSESSION_INFO_0* ppSessionInfoOut,
    PDWORD           pdwNumEntries
    )
{
    DWORD dwError = 0;
    PSESSION_INFO_0 pSessionInfoOut = NULL;

    if (dwNumEntries)
    {
        DWORD idx = 0;

        dwError = SrvSvcSrvAllocateMemory(
                            sizeof(SESSION_INFO_0) * dwNumEntries,
                            (PVOID*)&pSessionInfoOut);
        BAIL_ON_SRVSVC_ERROR(dwError);

        for (; idx < dwNumEntries; idx++)
        {
            PSESSION_INFO_0 pInfoIn = &pSessionInfoIn[idx];
            PSESSION_INFO_0 pInfoOut = &pSessionInfoOut[idx];

            if (pInfoIn->sesi0_cname)
            {
                dwError = SrvSvcSrvAllocateWC16String(
                                &pInfoOut->sesi0_cname,
                                pInfoIn->sesi0_cname);
                BAIL_ON_SRVSVC_ERROR(dwError);
            }
        }
    }

    *ppSessionInfoOut = pSessionInfoOut;
    *pdwNumEntries    = dwNumEntries;

cleanup:

    return dwError;

error:

    *ppSessionInfoOut = NULL;
    *pdwNumEntries    = 0;

    if (pSessionInfoOut)
    {
        SrvSvcSrvFreeSessionInfo_level_0(pSessionInfoOut, dwNumEntries);
    }

    goto cleanup;
}

static
VOID
SrvSvcSrvFreeSessionInfo_level_0(
    PSESSION_INFO_0 pSessionInfo,
    DWORD           dwNumEntries
    )
{
    DWORD idx = 0;

    for (; idx < dwNumEntries; idx++)
    {
        PSESSION_INFO_0 pInfo = &pSessionInfo[idx];

        if (pInfo->sesi0_cname)
        {
            SrvSvcSrvFreeMemory(pInfo->sesi0_cname);
        }
    }

    SrvSvcSrvFreeMemory(pSessionInfo);
}

static
DWORD
SrvSvcSrvMarshalSessionInfo_level_1(
    PSESSION_INFO_1  pSessionInfoIn,
    DWORD            dwNumEntries,
    PSESSION_INFO_1* ppSessionInfoOut,
    PDWORD           pdwNumEntries
    )
{
    DWORD dwError = 0;
    PSESSION_INFO_1 pSessionInfoOut = NULL;

    if (dwNumEntries)
    {
        DWORD idx = 0;

        dwError = SrvSvcSrvAllocateMemory(
                            sizeof(SESSION_INFO_1) * dwNumEntries,
                            (PVOID*)&pSessionInfoOut);
        BAIL_ON_SRVSVC_ERROR(dwError);

        for (; idx < dwNumEntries; idx++)
        {
            PSESSION_INFO_1 pInfoIn = &pSessionInfoIn[idx];
            PSESSION_INFO_1 pInfoOut = &pSessionInfoOut[idx];

            if (pInfoIn->sesi1_cname)
            {
                dwError = SrvSvcSrvAllocateWC16String(
                                &pInfoOut->sesi1_cname,
                                pInfoIn->sesi1_cname);
                BAIL_ON_SRVSVC_ERROR(dwError);
            }
            if (pInfoIn->sesi1_username)
            {
                dwError = SrvSvcSrvAllocateWC16String(
                                &pInfoOut->sesi1_username,
                                pInfoIn->sesi1_username);
                BAIL_ON_SRVSVC_ERROR(dwError);
            }
            pInfoOut->sesi1_num_opens  = pInfoIn->sesi1_num_opens;
            pInfoOut->sesi1_time       = pInfoIn->sesi1_time;
            pInfoOut->sesi1_idle_time  = pInfoIn->sesi1_idle_time;
            pInfoOut->sesi1_user_flags = pInfoIn->sesi1_user_flags;
        }
    }

    *ppSessionInfoOut = pSessionInfoOut;
    *pdwNumEntries    = dwNumEntries;

cleanup:

    return dwError;

error:

    *ppSessionInfoOut = NULL;
    *pdwNumEntries    = 0;

    if (pSessionInfoOut)
    {
        SrvSvcSrvFreeSessionInfo_level_1(pSessionInfoOut, dwNumEntries);
    }

    goto cleanup;
}

static
VOID
SrvSvcSrvFreeSessionInfo_level_1(
    PSESSION_INFO_1 pSessionInfo,
    DWORD           dwNumEntries
    )
{
    DWORD idx = 0;

    for (; idx < dwNumEntries; idx++)
    {
        PSESSION_INFO_1 pInfo = &pSessionInfo[idx];

        if (pInfo->sesi1_cname)
        {
            SrvSvcSrvFreeMemory(pInfo->sesi1_cname);
        }
        if (pInfo->sesi1_username)
        {
            SrvSvcSrvFreeMemory(pInfo->sesi1_username);
        }
    }

    SrvSvcSrvFreeMemory(pSessionInfo);
}

static
DWORD
SrvSvcSrvMarshalSessionInfo_level_2(
    PSESSION_INFO_2  pSessionInfoIn,
    DWORD            dwNumEntries,
    PSESSION_INFO_2* ppSessionInfoOut,
    PDWORD           pdwNumEntries
    )
{
    DWORD dwError = 0;
    PSESSION_INFO_2 pSessionInfoOut = NULL;

    if (dwNumEntries)
    {
        DWORD idx = 0;

        dwError = SrvSvcSrvAllocateMemory(
                            sizeof(SESSION_INFO_2) * dwNumEntries,
                            (PVOID*)&pSessionInfoOut);
        BAIL_ON_SRVSVC_ERROR(dwError);

        for (; idx < dwNumEntries; idx++)
        {
            PSESSION_INFO_2 pInfoIn = &pSessionInfoIn[idx];
            PSESSION_INFO_2 pInfoOut = &pSessionInfoOut[idx];

            if (pInfoIn->sesi2_cname)
            {
                dwError = SrvSvcSrvAllocateWC16String(
                                &pInfoOut->sesi2_cname,
                                pInfoIn->sesi2_cname);
                BAIL_ON_SRVSVC_ERROR(dwError);
            }
            if (pInfoIn->sesi2_username)
            {
                dwError = SrvSvcSrvAllocateWC16String(
                                &pInfoOut->sesi2_username,
                                pInfoIn->sesi2_username);
                BAIL_ON_SRVSVC_ERROR(dwError);
            }
            if (pInfoIn->sesi2_cltype_name)
            {
                dwError = SrvSvcSrvAllocateWC16String(
                                &pInfoOut->sesi2_cltype_name,
                                pInfoIn->sesi2_cltype_name);
                BAIL_ON_SRVSVC_ERROR(dwError);
            }

            pInfoOut->sesi2_num_opens  = pInfoIn->sesi2_num_opens;
            pInfoOut->sesi2_time       = pInfoIn->sesi2_time;
            pInfoOut->sesi2_idle_time  = pInfoIn->sesi2_idle_time;
            pInfoOut->sesi2_user_flags = pInfoIn->sesi2_user_flags;
        }
    }

    *ppSessionInfoOut = pSessionInfoOut;
    *pdwNumEntries    = dwNumEntries;

cleanup:

    return dwError;

error:

    *ppSessionInfoOut = NULL;
    *pdwNumEntries    = 0;

    if (pSessionInfoOut)
    {
        SrvSvcSrvFreeSessionInfo_level_2(pSessionInfoOut, dwNumEntries);
    }

    goto cleanup;
}

static
VOID
SrvSvcSrvFreeSessionInfo_level_2(
    PSESSION_INFO_2 pSessionInfo,
    DWORD           dwNumEntries
    )
{
    DWORD idx = 0;

    for (; idx < dwNumEntries; idx++)
    {
        PSESSION_INFO_2 pInfo = &pSessionInfo[idx];

        if (pInfo->sesi2_cname)
        {
            SrvSvcSrvFreeMemory(pInfo->sesi2_cname);
        }
        if (pInfo->sesi2_username)
        {
            SrvSvcSrvFreeMemory(pInfo->sesi2_username);
        }
        if (pInfo->sesi2_cltype_name)
        {
            SrvSvcSrvFreeMemory(pInfo->sesi2_cltype_name);
        }
    }

    SrvSvcSrvFreeMemory(pSessionInfo);
}

static
DWORD
SrvSvcSrvMarshalSessionInfo_level_10(
    PSESSION_INFO_10  pSessionInfoIn,
    DWORD             dwNumEntries,
    PSESSION_INFO_10* ppSessionInfoOut,
    PDWORD            pdwNumEntries
    )
{
    DWORD dwError = 0;
    PSESSION_INFO_10 pSessionInfoOut = NULL;

    if (dwNumEntries)
    {
        DWORD idx = 0;

        dwError = SrvSvcSrvAllocateMemory(
                            sizeof(SESSION_INFO_10) * dwNumEntries,
                            (PVOID*)&pSessionInfoOut);
        BAIL_ON_SRVSVC_ERROR(dwError);

        for (; idx < dwNumEntries; idx++)
        {
            PSESSION_INFO_10 pInfoIn = &pSessionInfoIn[idx];
            PSESSION_INFO_10 pInfoOut = &pSessionInfoOut[idx];

            if (pInfoIn->sesi10_cname)
            {
                dwError = SrvSvcSrvAllocateWC16String(
                                &pInfoOut->sesi10_cname,
                                pInfoIn->sesi10_cname);
                BAIL_ON_SRVSVC_ERROR(dwError);
            }
            if (pInfoIn->sesi10_username)
            {
                dwError = SrvSvcSrvAllocateWC16String(
                                &pInfoOut->sesi10_username,
                                pInfoIn->sesi10_username);
                BAIL_ON_SRVSVC_ERROR(dwError);
            }

            pInfoOut->sesi10_time       = pInfoIn->sesi10_time;
            pInfoOut->sesi10_idle_time  = pInfoIn->sesi10_idle_time;
        }
    }

    *ppSessionInfoOut = pSessionInfoOut;
    *pdwNumEntries    = dwNumEntries;

cleanup:

    return dwError;

error:

    *ppSessionInfoOut = NULL;
    *pdwNumEntries    = 0;

    if (pSessionInfoOut)
    {
        SrvSvcSrvFreeSessionInfo_level_10(pSessionInfoOut, dwNumEntries);
    }

    goto cleanup;
}

static
VOID
SrvSvcSrvFreeSessionInfo_level_10(
    PSESSION_INFO_10 pSessionInfo,
    DWORD            dwNumEntries
    )
{
    DWORD idx = 0;

    for (; idx < dwNumEntries; idx++)
    {
        PSESSION_INFO_10 pInfo = &pSessionInfo[idx];

        if (pInfo->sesi10_cname)
        {
            SrvSvcSrvFreeMemory(pInfo->sesi10_cname);
        }
        if (pInfo->sesi10_username)
        {
            SrvSvcSrvFreeMemory(pInfo->sesi10_username);
        }
    }

    SrvSvcSrvFreeMemory(pSessionInfo);
}

static
DWORD
SrvSvcSrvMarshalSessionInfo_level_502(
    PSESSION_INFO_502  pSessionInfoIn,
    DWORD              dwNumEntries,
    PSESSION_INFO_502* ppSessionInfoOut,
    PDWORD             pdwNumEntries
    )
{
    DWORD dwError = 0;
    PSESSION_INFO_502 pSessionInfoOut = NULL;

    if (dwNumEntries)
    {
        DWORD idx = 0;

        dwError = SrvSvcSrvAllocateMemory(
                            sizeof(SESSION_INFO_502) * dwNumEntries,
                            (PVOID*)&pSessionInfoOut);
        BAIL_ON_SRVSVC_ERROR(dwError);

        for (; idx < dwNumEntries; idx++)
        {
            PSESSION_INFO_502 pInfoIn  = &pSessionInfoIn[idx];
            PSESSION_INFO_502 pInfoOut = &pSessionInfoOut[idx];

            if (pInfoIn->sesi502_cname)
            {
                dwError = SrvSvcSrvAllocateWC16String(
                                &pInfoOut->sesi502_cname,
                                pInfoIn->sesi502_cname);
                BAIL_ON_SRVSVC_ERROR(dwError);
            }
            if (pInfoIn->sesi502_username)
            {
                dwError = SrvSvcSrvAllocateWC16String(
                                &pInfoOut->sesi502_username,
                                pInfoIn->sesi502_username);
                BAIL_ON_SRVSVC_ERROR(dwError);
            }

            pInfoOut->sesi502_num_opens  = pInfoIn->sesi502_num_opens;
            pInfoOut->sesi502_time       = pInfoIn->sesi502_time;
            pInfoOut->sesi502_idle_time  = pInfoIn->sesi502_idle_time;
            pInfoOut->sesi502_user_flags = pInfoIn->sesi502_user_flags;

            if (pInfoIn->sesi502_cltype_name)
            {
                dwError = SrvSvcSrvAllocateWC16String(
                                &pInfoOut->sesi502_cltype_name,
                                pInfoIn->sesi502_cltype_name);
                BAIL_ON_SRVSVC_ERROR(dwError);
            }
            if (pInfoIn->sesi502_transport)
            {
                dwError = SrvSvcSrvAllocateWC16String(
                                &pInfoOut->sesi502_transport,
                                pInfoIn->sesi502_transport);
                BAIL_ON_SRVSVC_ERROR(dwError);
            }
        }
    }

    *ppSessionInfoOut = pSessionInfoOut;
    *pdwNumEntries    = dwNumEntries;

cleanup:

    return dwError;

error:

    *ppSessionInfoOut = NULL;
    *pdwNumEntries    = 0;

    if (pSessionInfoOut)
    {
        SrvSvcSrvFreeSessionInfo_level_502(pSessionInfoOut, dwNumEntries);
    }

    goto cleanup;
}

static
VOID
SrvSvcSrvFreeSessionInfo_level_502(
    PSESSION_INFO_502 pSessionInfo,
    DWORD             dwNumEntries
    )
{
    DWORD idx = 0;

    for (; idx < dwNumEntries; idx++)
    {
        PSESSION_INFO_502 pInfo = &pSessionInfo[idx];

        if (pInfo->sesi502_cname)
        {
            SrvSvcSrvFreeMemory(pInfo->sesi502_cname);
        }
        if (pInfo->sesi502_username)
        {
            SrvSvcSrvFreeMemory(pInfo->sesi502_username);
        }
        if (pInfo->sesi502_cltype_name)
        {
            SrvSvcSrvFreeMemory(pInfo->sesi502_cltype_name);
        }
        if (pInfo->sesi502_transport)
        {
            SrvSvcSrvFreeMemory(pInfo->sesi502_transport);
        }
    }

    SrvSvcSrvFreeMemory(pSessionInfo);
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
