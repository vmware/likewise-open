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
 *        srvsvc_netsharegetinfo.c
 *
 * Abstract:
 *
 *        Likewise Server Service (srvsvc) RPC client and server
 *
 *        SrvSvcNetShareGetInfo server API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"

/* Make memory allocation easier */
typedef union __srvsvc_NetShareInfo
{
    SHARE_INFO_0 info0;
    SHARE_INFO_1 info1;
    SHARE_INFO_2 info2;
    SHARE_INFO_501 info501;
    SHARE_INFO_502 info502;

} SHARE_INFO, *PSHARE_INFO;


NET_API_STATUS
SrvSvcNetShareGetInfo(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *server_name,
    /* [in] */ wchar16_t *netname,
    /* [in] */ uint32 level,
    /* [out,ref] */ srvsvc_NetShareInfo *info
    )
{
    NTSTATUS ntStatus = 0;
    DWORD dwError = 0;
    PBYTE pInBuffer = NULL;
    DWORD dwInLength = 0;
    PWSTR lpFileName = NULL;
    DWORD dwDesiredAccess = 0;
    DWORD dwShareMode = 0;
    DWORD dwCreationDisposition = 0;
    DWORD dwFlagsAndAttributes = 0;
    PBYTE pOutBuffer = NULL;
    DWORD dwOutLength = 4096;
    DWORD dwBytesReturned = 0;
    HANDLE hDevice = (HANDLE)NULL;
    BOOLEAN bRet = FALSE;
    DWORD  dwReturnCode = 0;
    DWORD  dwParmError = 0;
    IO_FILE_HANDLE FileHandle;
    IO_STATUS_BLOCK IoStatusBlock;
    PIO_FILE_NAME FileName = NULL;
    ACCESS_MASK DesiredAccess = 0;
    LONG64 AllocationSize = 0;
    FILE_ATTRIBUTES FileAttributes = 0;
    FILE_SHARE_FLAGS ShareAccess = 0;
    FILE_CREATE_DISPOSITION CreateDisposition = 0;
    FILE_CREATE_OPTIONS CreateOptions = 0;
    ULONG IoControlCode = SRV_DEVCTL_GET_SHARE_INFO;
    PSTR smbpath = NULL;
    IO_FILE_NAME filename;
    IO_STATUS_BLOCK io_status;
    SHARE_INFO_GETINFO_PARAMS GetParamsIn;
    PSHARE_INFO_GETINFO_PARAMS pGetParamsOut = NULL;
    PSHARE_INFO pShareInfo = NULL;

    memset(&GetParamsIn, 0, sizeof(GetParamsIn));

    GetParamsIn.pwszNetname = netname;
    GetParamsIn.dwInfoLevel = level;

    ntStatus = LwShareInfoMarshalGetParameters(
                        &GetParamsIn,
                        &pInBuffer,
                        &dwInLength
                        );
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwRtlCStringAllocatePrintf(
                    &smbpath,
                    "\\srv"
                    );
    BAIL_ON_NT_STATUS(ntStatus);

    filename.RootFileHandle = NULL;
    filename.IoNameOptions = 0;

    ntStatus = LwRtlWC16StringAllocateFromCString(
                        &filename.FileName,
                        smbpath
                        );
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NtCreateFile(
                        &FileHandle,
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
                        NULL
                        );
    BAIL_ON_NT_STATUS(ntStatus);

    dwError = SRVSVCAllocateMemory(
                    dwOutLength,
                    (void**)&pOutBuffer
                    );
    BAIL_ON_ERROR(dwError);

    /* Allocate memory since it will be needed even in the failure case */

    ntStatus = RTL_ALLOCATE(&pShareInfo, SHARE_INFO, sizeof(*pShareInfo));
    BAIL_ON_NT_STATUS(ntStatus);

    info->info0   = &pShareInfo->info0;
    info->info1   = &pShareInfo->info1;
    info->info2   = &pShareInfo->info2;
    info->info501 = &pShareInfo->info501;
    info->info502 = &pShareInfo->info502;

    ntStatus = NtDeviceIoControlFile(
                    FileHandle,
                    NULL,
                    &IoStatusBlock,
                    IoControlCode,
                    pInBuffer,
                    dwInLength,
                    pOutBuffer,
                    dwOutLength
                    );
    BAIL_ON_NT_STATUS(ntStatus);

    while (ntStatus == STATUS_MORE_ENTRIES) {
        /* We need more space in output buffer to make this call */

        SrvSvcFreeMemory((void*)pOutBuffer);
        dwOutLength *= 2;

        dwError = SRVSVCAllocateMemory(
                        dwOutLength,
                        (void**)&pOutBuffer
                        );
        BAIL_ON_ERROR(dwError);

        ntStatus = NtDeviceIoControlFile(
                        FileHandle,
                        NULL,
                        &IoStatusBlock,
                        IoControlCode,
                        pInBuffer,
                        dwInLength,
                        pOutBuffer,
                        dwOutLength
                        );
    }

    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwShareInfoUnmarshalGetParameters(
                        pOutBuffer,
                        dwOutLength,
                        &pGetParamsOut
                        );

    switch (pGetParamsOut->dwInfoLevel) {
    case 0:
        memcpy(info->info0, pGetParamsOut->Info.p0, sizeof(*info->info0));
        break;

    case 1:
        memcpy(info->info1, pGetParamsOut->Info.p1, sizeof(*info->info1));
        break;

    case 2:
        memcpy(info->info2, pGetParamsOut->Info.p2, sizeof(*info->info2));
        break;

    case 501:
        memcpy(info->info501, pGetParamsOut->Info.p501, sizeof(*info->info501));
        break;

    case 502:
        memcpy(info->info502, pGetParamsOut->Info.p502, sizeof(*info->info502));
        break;
    }

cleanup:
    if (FileHandle) {
        NtCloseFile(FileHandle);
    }

    if(pInBuffer) {
        SrvSvcFreeMemory(pInBuffer);
    }

    if (pOutBuffer) {
        SrvSvcFreeMemory(pOutBuffer);
    }

    if (pGetParamsOut) {
        SrvSvcFreeMemory(pGetParamsOut);
    }

    if (ntStatus == STATUS_NOT_FOUND) {
        dwError = 2310;
    } else if (ntStatus == STATUS_SUCCESS) {
        dwError = 0;
    } else {
        dwError = -1;
    }

    return dwError;

error:
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
