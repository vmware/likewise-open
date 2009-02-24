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
    PSHARE_INFO_0 info0 = NULL;
    PSHARE_INFO_1 info1 = NULL;
    PSHARE_INFO_2 info2 = NULL;
    PSHARE_INFO_501 info501 = NULL;
    PSHARE_INFO_502 info502 = NULL;

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
        ntStatus = SRVSVCAllocateMemory(
                            sizeof(*info0),
                            (void**)&info0
                            );
        BAIL_ON_NT_STATUS(ntStatus);

        memcpy(info0, pGetParamsOut->Info.p0, sizeof(*info0));
        info->info0 = info0;
        break;

    case 1:
        ntStatus = SRVSVCAllocateMemory(
                            sizeof(*info1),
                            (void**)&info1
                            );
        BAIL_ON_NT_STATUS(ntStatus);

        memcpy(info1, pGetParamsOut->Info.p1, sizeof(*info1));
        info->info1 = info1;
        break;

    case 2:
        ntStatus = SRVSVCAllocateMemory(
                            sizeof(*info2),
                            (void**)&info2
                            );
        BAIL_ON_NT_STATUS(ntStatus);

        memcpy(info2, pGetParamsOut->Info.p2, sizeof(*info2));
        info->info2 = info2;
        break;

    case 501:
        ntStatus = SRVSVCAllocateMemory(
                            sizeof(*info501),
                            (void**)&info501
                            );
        BAIL_ON_NT_STATUS(ntStatus);

        memcpy(info501, pGetParamsOut->Info.p501, sizeof(*info501));
        info->info501 = info501;
        break;

    case 502:
        ntStatus = SRVSVCAllocateMemory(
                            sizeof(*info502),
                            (void**)&info502
                            );
        BAIL_ON_NT_STATUS(ntStatus);

        memcpy(info502, pGetParamsOut->Info.p502, sizeof(*info502));
        info->info502 = info502;
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

    if (info0) {
        SrvSvcFreeMemory(info0);
    }

    if (info1) {
        SrvSvcFreeMemory(info1);
    }

    if (info2) {
        SrvSvcFreeMemory(info2);
    }

    if (info501) {
        SrvSvcFreeMemory(info501);
    }

    if (info502) {
        SrvSvcFreeMemory(info502);
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
