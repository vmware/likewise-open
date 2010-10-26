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
 *        srvsvc_netconnectionenum.c
 *
 * Abstract:
 *
 *        Likewise Server Service (srvsvc) RPC client and server
 *
 *        SrvSvcNetrConnectionEnum server API
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NET_API_STATUS
SrvSvcNetrConnectionEnum(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *pwszServerName,
    /* [in] */ wchar16_t *pwszQualifier,
    /* [in, out] */ PDWORD pdwLevel,
    /* [in, out] */ srvsvc_NetConnCtr *pCtr,
    /* [in] */ DWORD dwPrefMaxLength,
    /* [out] */ PDWORD pdwTotalEntries,
    /* [in, out] */ PDWORD pdwResume
    )
{
    DWORD dwError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PBYTE pInBuffer = NULL;
    DWORD dwInLength = 0;
    PBYTE pOutBuffer = NULL;
    DWORD dwOutLength = 4096;
    IO_FILE_HANDLE hFile = NULL;
    IO_STATUS_BLOCK IoStatusBlock = { 0 };
    ACCESS_MASK DesiredAccess = 0;
    LONG64 AllocationSize = 0;
    FILE_ATTRIBUTES FileAttributes = 0;
    FILE_SHARE_FLAGS ShareAccess = 0;
    FILE_CREATE_DISPOSITION CreateDisposition = 0;
    FILE_CREATE_OPTIONS CreateOptions = 0;
    ULONG IoControlCode = SRV_DEVCTL_ENUM_CONNECTION;
    wchar16_t wszDriverName[] = SRV_DRIVER_NAME_W;
    IO_FILE_NAME filename =
                        {
                              .RootFileHandle = NULL,
                              .FileName = &wszDriverName[0],
                              .IoNameOptions = 0
                        };
    CONNECTION_INFO_ENUM_IN_PARAMS EnumParamsIn = { 0 };
    PCONNECTION_INFO_ENUM_OUT_PARAMS pEnumParamsOut = NULL;
    PCONNECTION_INFO_UNION pConnectionInfo = NULL;
    srvsvc_NetConnCtr0 *pCtr0 = NULL;
    srvsvc_NetConnCtr1 *pCtr1 = NULL;
    SHORT i = 0;
    BOOLEAN bMoreData = FALSE;

    BAIL_ON_INVALID_PTR(pdwLevel, dwError);
    BAIL_ON_INVALID_PTR(pCtr, dwError);
    BAIL_ON_INVALID_PTR(pdwTotalEntries, dwError);

    EnumParamsIn.pwszQualifier     = pwszQualifier;
    EnumParamsIn.dwLevel           = *pdwLevel;
    EnumParamsIn.dwPreferredMaxLen = dwPrefMaxLength;
    if (pdwResume)
    {
        EnumParamsIn.dwResume      = *pdwResume;
    }

    ntStatus = LwConnectionInfoMarshalEnumInputParameters(
                        &EnumParamsIn,
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
                        DesiredAccess,
                        AllocationSize,
                        FileAttributes,
                        ShareAccess,
                        CreateDisposition,
                        CreateOptions,
                        NULL,
                        0,
                        NULL,
                        NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    dwError = LwAllocateMemory(dwOutLength,
                               OUT_PPVOID(&pOutBuffer));
    BAIL_ON_SRVSVC_ERROR(dwError);

    ntStatus = NtDeviceIoControlFile(
                    hFile,
                    NULL,
                    &IoStatusBlock,
                    IoControlCode,
                    pInBuffer,
                    dwInLength,
                    pOutBuffer,
                    dwOutLength
                    );
    while (ntStatus == STATUS_BUFFER_TOO_SMALL)
    {
        LW_SAFE_FREE_MEMORY(pOutBuffer);
        dwOutLength *= 2;

        dwError = LwAllocateMemory(dwOutLength,
                                   OUT_PPVOID(&pOutBuffer));
        BAIL_ON_SRVSVC_ERROR(dwError);

        ntStatus = NtDeviceIoControlFile(
                        hFile,
                        NULL,
                        &IoStatusBlock,
                        IoControlCode,
                        pInBuffer,
                        dwInLength,
                        pOutBuffer,
                        dwOutLength
                        );
    }
    if (ntStatus == STATUS_MORE_ENTRIES)
    {
        bMoreData = TRUE;
        ntStatus  = STATUS_SUCCESS;
    }
    else
    {
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = LwConnectionInfoUnmarshalEnumOutputParameters(
                    pOutBuffer,
                    IoStatusBlock.BytesTransferred,
                    &pEnumParamsOut,
                    &pConnectionInfo);
    BAIL_ON_NT_STATUS(ntStatus);

    switch (pEnumParamsOut->dwLevel)
    {
    case 0:
        pCtr0 = pCtr->ctr0;
        pCtr0->count = pEnumParamsOut->dwNumEntries;

        dwError = SrvSvcSrvAllocateMemory(
                      sizeof(*pCtr0->array) * pCtr0->count,
                      OUT_PPVOID(&pCtr0->array));
        BAIL_ON_SRVSVC_ERROR(dwError);

        for (i = 0; i < pCtr0->count; i++)
        {
            dwError = SrvSvcSrvCopyConnectionInfo0(
                          &pCtr0->array[i],
                          &pConnectionInfo->p0[i]);
            BAIL_ON_SRVSVC_ERROR(dwError);
        }
        break;

    case 1:
        pCtr1 = pCtr->ctr1;
        pCtr1->count = pEnumParamsOut->dwNumEntries;

        dwError = SrvSvcSrvAllocateMemory(
                      sizeof(*pCtr1->array) * pCtr1->count,
                      OUT_PPVOID(&pCtr1->array));
        BAIL_ON_SRVSVC_ERROR(dwError);

        for (i = 0; i < pCtr1->count; i++)
        {
            dwError = SrvSvcSrvCopyConnectionInfo1(
                          &pCtr1->array[i],
                          &pConnectionInfo->p1[i]);
            BAIL_ON_SRVSVC_ERROR(dwError);
        }
        break;

    default:
        dwError = ERROR_NOT_SUPPORTED;
        BAIL_ON_SRVSVC_ERROR(dwError);
    }

    *pdwLevel        = pEnumParamsOut->dwLevel;
    *pdwTotalEntries = pEnumParamsOut->dwTotalNumEntries;

    if (pdwResume)
    {
        *pdwResume   = pEnumParamsOut->dwResume;
    }

cleanup:
    if (hFile)
    {
        NtCloseFile(hFile);
    }

    LW_SAFE_FREE_MEMORY(pInBuffer);
    LW_SAFE_FREE_MEMORY(pOutBuffer);
    LW_SAFE_FREE_MEMORY(pEnumParamsOut);
    LW_SAFE_FREE_MEMORY(pConnectionInfo);

    if (dwError == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        dwError = LwNtStatusToWin32Error(ntStatus);
    }

    if (dwError == ERROR_SUCCESS && bMoreData)
    {
        dwError = ERROR_MORE_DATA;
    }

    return (NET_API_STATUS)dwError;

error:
    *pdwLevel        = 0;
    *pdwTotalEntries = 0;

    if (pdwResume)
    {
        *pdwResume   = 0;
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
