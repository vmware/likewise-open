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
 *        netfileenum.c
 *
 * Abstract:
 *
 *        Likewise Server Service (srvsvc) RPC client and server
 *
 *        SrvSvcNetFileEnum    (Server API)
 *        SrvSvcNetFileGetInfo (Server API)
 *        SrvSvcNetFileClose   (Server API)
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 */

#include "includes.h"

NET_API_STATUS
SrvSvcNetFileEnum(
    handle_t           IDL_handle,           /* [in]      */
    PWSTR              pwszServername,       /* [in]      */
    PWSTR              pwszBasepath,         /* [in]      */
    PWSTR              pwszUsername,         /* [in]      */
    DWORD              dwInfoLevel,          /* [in, out] */
    srvsvc_NetFileCtr* pInfo,                /* [in, out] */
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
    ULONG                   dwIoControlCode     = SRV_DEVCTL_ENUM_FILES;
    FILE_INFO_ENUM_PARAMS   fileEnumParamsIn    =
    {
            .pwszBasepath         = pwszBasepath,
            .pwszUsername         = pwszUsername,
            .dwInfoLevel          = dwInfoLevel,
            .dwPreferredMaxLength = dwPreferredMaxLength,
            .dwEntriesRead        = 0,
            .dwTotalEntries       = 0,
            .pdwResumeHandle      = pdwResumeHandle ? pdwResumeHandle : NULL,
            .info                 = {0}
    };
    PFILE_INFO_ENUM_PARAMS  pFileEnumParamsOut = NULL;
    srvsvc_NetFileCtr2*     ctr2 = NULL;
    srvsvc_NetFileCtr3*     ctr3 = NULL;
    BOOLEAN                 bMoreDataAvailable = FALSE;

    ntStatus = LwFileInfoMarshalEnumParameters(
                            &fileEnumParamsIn,
                            &pInBuffer,
                            &dwInLength);
    BAIL_ON_NT_STATUS(ntStatus);

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

            ntStatus = LwFileInfoUnmarshalEnumParameters(
                                    pOutBuffer,
                                    dwOutLength,
                                    &pFileEnumParamsOut);
            BAIL_ON_NT_STATUS(ntStatus);

            switch (pFileEnumParamsOut->dwInfoLevel)
            {
                case 2:

                    ctr2 = pInfo->ctr2;
                    ctr2->count = pFileEnumParamsOut->dwEntriesRead;

                    dwError = SrvSvcSrvAllocateMemory(
                                        sizeof(*ctr2->array) * ctr2->count,
                                        (void**)&ctr2->array);
                    BAIL_ON_SRVSVC_ERROR(dwError);

                    memcpy(
                        (void*)ctr2->array,
                        (void*)pFileEnumParamsOut->info.p2,
                           sizeof(*ctr2->array) * ctr2->count);

                    break;

                case 3:

                    ctr3 = pInfo->ctr3;
                    ctr3->count = pFileEnumParamsOut->dwEntriesRead;

                    dwError = SrvSvcSrvAllocateMemory(
                                        sizeof(*ctr3->array) * ctr3->count,
                                        (void**)&ctr3->array);
                    BAIL_ON_SRVSVC_ERROR(dwError);

                    memcpy(
                        (void*)ctr3->array,
                        (void*)pFileEnumParamsOut->info.p3,
                        sizeof(*ctr3->array) * ctr2->count);

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

    *pdwEntriesRead  = pFileEnumParamsOut->dwEntriesRead;
    *pdwTotalEntries = pFileEnumParamsOut->dwTotalEntries;
    if (pdwResumeHandle)
    {
        *pdwResumeHandle = *pFileEnumParamsOut->pdwResumeHandle;
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
    LW_SAFE_FREE_MEMORY(pFileEnumParamsOut);

    return dwError;

error:

    if (pFileEnumParamsOut && pInfo)
    {
        switch (pFileEnumParamsOut->dwInfoLevel)
        {
            case 2:

                if (ctr2 && ctr2->array)
                {
                    SrvSvcSrvFreeMemory(ctr2->array);
                }
                break;

            case 3:

                if (ctr3 && ctr3->array)
                {
                    SrvSvcSrvFreeMemory(ctr3->array);
                }
                break;

            default:

                SRVSVC_LOG_ERROR("Unsupported info level [%u]",
                                 pFileEnumParamsOut->dwInfoLevel);
                break;
        }
    }

    if (ntStatus != STATUS_SUCCESS)
    {
        dwError = LwNtStatusToWin32Error(ntStatus);
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
SrvSvcNetFileGetInfo(
    handle_t            IDL_handle,     /* [in] */
    PWSTR               pwszServername, /* [in] */
    DWORD               dwFileId,       /* [in] */
    DWORD               dwInfoLevel,    /* [in] */
    srvsvc_NetFileInfo* pInfo           /* [out] */
    )
{
    DWORD     dwError     = 0;
    NTSTATUS  ntStatus    = 0;
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
    ULONG                   dwIoControlCode     = SRV_DEVCTL_GET_FILE_INFO;
    FILE_INFO_GET_INFO_PARAMS  fileGetInfoParamsIn = {0};
    PFILE_INFO_GET_INFO_PARAMS pFileGetInfoParamsOut = NULL;
    PFILE_INFO pFileInfo   = NULL;

    switch (dwInfoLevel)
    {
        case 2:
        case 3:

            break;

        default:

            ntStatus = STATUS_INVALID_LEVEL;
            BAIL_ON_NT_STATUS(ntStatus);
    }

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

    memset(&fileGetInfoParamsIn, 0, sizeof(fileGetInfoParamsIn));

    fileGetInfoParamsIn.dwFileId    = dwFileId;
    fileGetInfoParamsIn.dwInfoLevel = dwInfoLevel;

    ntStatus = LwFileInfoMarshalGetInfoParameters(
                        &fileGetInfoParamsIn,
                        &pInBuffer,
                        &dwInLength);
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
    BAIL_ON_NT_STATUS(ntStatus);

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
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwFileInfoUnmarshalGetInfoParameters(
                        pOutBuffer,
                        dwOutLength,
                        &pFileGetInfoParamsOut);
    BAIL_ON_NT_STATUS(ntStatus);

    dwError = SrvSvcSrvAllocateMemory(sizeof(*pFileInfo), (PVOID*)&pFileInfo);
    BAIL_ON_SRVSVC_ERROR(dwError);

    switch (pFileGetInfoParamsOut->dwInfoLevel)
    {
        case 2:

            pInfo->info2   = &pFileInfo->info2;
            memcpy(pInfo->info2, pFileGetInfoParamsOut->info.p2, sizeof(*pInfo->info2));

            break;

        case 3:

            pInfo->info3   = &pFileInfo->info3;
            memcpy(pInfo->info3, pFileGetInfoParamsOut->info.p3, sizeof(*pInfo->info3));

            break;

        default:

            ntStatus = STATUS_INVALID_LEVEL;
            BAIL_ON_NT_STATUS(ntStatus);

            break;
    }

cleanup:

    if (hFile)
    {
        NtCloseFile(hFile);
    }

    LW_SAFE_FREE_MEMORY(pInBuffer);
    LW_SAFE_FREE_MEMORY(pOutBuffer);
    LW_SAFE_FREE_MEMORY(pFileGetInfoParamsOut);

    return dwError;

error:

    if (pInfo)
    {
        memset(pInfo, 0x0, sizeof(*pInfo));
    }

    if (ntStatus != STATUS_SUCCESS)
    {
        dwError = LwNtStatusToWin32Error(ntStatus);
    }

    if (pFileInfo)
    {
        SrvSvcSrvFreeMemory(pFileInfo);
    }

    goto cleanup;
}

NET_API_STATUS
SrvSvcNetFileClose(
    handle_t IDL_handle,     /* [in] */
    PWSTR    pwszServername, /* [in] */
    DWORD    dwFileId        /* [in] */
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
    ULONG                   dwIoControlCode     = SRV_DEVCTL_CLOSE_FILE;
    FILE_INFO_CLOSE_PARAMS  fileCloseParams =
    {
        .dwFileId          = dwFileId
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

    ntStatus = LwFileInfoMarshalCloseParameters(
                    &fileCloseParams,
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


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
