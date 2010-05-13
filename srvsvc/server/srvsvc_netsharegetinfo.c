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
 *        NetShareGetInfo server API
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
    SHARE_INFO_1005 info1005;

} SHARE_INFO, *PSHARE_INFO;


NET_API_STATUS
SrvSvcNetShareGetInfo(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *server_name,
    /* [in] */ wchar16_t *netname,
    /* [in] */ UINT32 level,
    /* [out,ref] */ srvsvc_NetShareInfo *info
    )
{
    NTSTATUS ntStatus = 0;
    DWORD dwError = 0;
    PBYTE pInBuffer = NULL;
    DWORD dwInLength = 0;
    PBYTE pOutBuffer = NULL;
    DWORD dwOutLength = 4096;
    IO_FILE_HANDLE hFile = NULL;
    IO_STATUS_BLOCK IoStatusBlock = {0};
    ACCESS_MASK DesiredAccess = 0;
    LONG64 AllocationSize = 0;
    FILE_ATTRIBUTES FileAttributes = 0;
    FILE_SHARE_FLAGS ShareAccess = 0;
    FILE_CREATE_DISPOSITION CreateDisposition = 0;
    FILE_CREATE_OPTIONS CreateOptions = 0;
    ULONG IoControlCode = SRV_DEVCTL_GET_SHARE_INFO;
    wchar16_t wszDriverName[] = SRV_DRIVER_NAME_W;
    IO_FILE_NAME filename =
                        {
                              .RootFileHandle = NULL,
                              .FileName = &wszDriverName[0],
                              .IoNameOptions = 0
                        };
    SHARE_INFO_GETINFO_PARAMS GetParamsIn = {0};
    PSHARE_INFO_GETINFO_PARAMS pGetParamsOut = NULL;
    PSHARE_INFO pShareInfo = NULL;

    /* Validate info level */

    switch (level){
    case 0:
    case 1:
    case 2:
    case 501:
    case 502:
    case 1005:
        break;

    default:
        ntStatus = STATUS_INVALID_LEVEL;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    memset(&GetParamsIn, 0, sizeof(GetParamsIn));
    memset(info, 0x0, sizeof(*info));

    GetParamsIn.pwszNetname = netname;
    GetParamsIn.dwInfoLevel = level;

    ntStatus = LwShareInfoMarshalGetParameters(
                        &GetParamsIn,
                        &pInBuffer,
                        &dwInLength
                        );
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
                        NULL
                        );
    BAIL_ON_NT_STATUS(ntStatus);

    dwError = LwAllocateMemory(
                    dwOutLength,
                    (void**)&pOutBuffer
                    );
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
    BAIL_ON_NT_STATUS(ntStatus);

    while (ntStatus == STATUS_BUFFER_TOO_SMALL) {
        /* We need more space in output buffer to make this call */

        LW_SAFE_FREE_MEMORY(pOutBuffer);
        dwOutLength *= 2;

        dwError = LwAllocateMemory(
                        dwOutLength,
                        (void**)&pOutBuffer
                        );
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

    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwShareInfoUnmarshalGetParameters(
                        pOutBuffer,
                        dwOutLength,
                        &pGetParamsOut
                        );
    BAIL_ON_NT_STATUS(ntStatus);

    dwError = SrvSvcSrvAllocateMemory(sizeof(*pShareInfo),
                                    (PVOID*)&pShareInfo);
    BAIL_ON_SRVSVC_ERROR(dwError);

    switch (pGetParamsOut->dwInfoLevel) {
    case 0:
        info->info0   = &pShareInfo->info0;

        dwError = SrvSvcSrvAllocateWC16String(
                      &info->info0->shi0_netname,
                      pGetParamsOut->Info.p0->shi0_netname);
        BAIL_ON_SRVSVC_ERROR(dwError);

        break;

    case 1:
        info->info1   = &pShareInfo->info1;

        dwError = SrvSvcSrvAllocateWC16String(
                      &info->info1->shi1_netname,
                      pGetParamsOut->Info.p1->shi1_netname);
        BAIL_ON_SRVSVC_ERROR(dwError);

        dwError = SrvSvcSrvAllocateWC16String(
                      &info->info1->shi1_remark,
                      pGetParamsOut->Info.p1->shi1_remark);
        BAIL_ON_SRVSVC_ERROR(dwError);

        info->info1->shi1_type = pGetParamsOut->Info.p1->shi1_type;

        break;

    case 2:
        info->info2   = &pShareInfo->info2;

        dwError = SrvSvcSrvAllocateWC16String(
                      &info->info2->shi2_netname,
                      pGetParamsOut->Info.p2->shi2_netname);
        BAIL_ON_SRVSVC_ERROR(dwError);

        dwError = SrvSvcSrvAllocateWC16String(
                      &info->info2->shi2_remark,
                      pGetParamsOut->Info.p2->shi2_remark);
        BAIL_ON_SRVSVC_ERROR(dwError);

        dwError = SrvSvcSrvAllocateWC16String(
                      &info->info2->shi2_path,
                      pGetParamsOut->Info.p2->shi2_path);
        BAIL_ON_SRVSVC_ERROR(dwError);

        dwError = SrvSvcSrvAllocateWC16String(
                      &info->info2->shi2_password,
                      pGetParamsOut->Info.p2->shi2_password);
        BAIL_ON_SRVSVC_ERROR(dwError);

        info->info2->shi2_type         = pGetParamsOut->Info.p2->shi2_type;
        info->info2->shi2_permissions  = pGetParamsOut->Info.p2->shi2_permissions;
        info->info2->shi2_max_uses     = pGetParamsOut->Info.p2->shi2_max_uses;
        info->info2->shi2_current_uses = pGetParamsOut->Info.p2->shi2_current_uses;

        break;

    case 501:
        info->info501 = &pShareInfo->info501;

        dwError = SrvSvcSrvAllocateWC16String(
                      &info->info501->shi501_netname,
                      pGetParamsOut->Info.p501->shi501_netname);
        BAIL_ON_SRVSVC_ERROR(dwError);

        dwError = SrvSvcSrvAllocateWC16String(
                      &info->info501->shi501_remark,
                      pGetParamsOut->Info.p501->shi501_remark);
        BAIL_ON_SRVSVC_ERROR(dwError);

        info->info501->shi501_type  = pGetParamsOut->Info.p501->shi501_type;
        info->info501->shi501_flags = pGetParamsOut->Info.p501->shi501_flags;

        break;

    case 502:
        info->info502 = &pShareInfo->info502;

        dwError = SrvSvcSrvAllocateWC16String(
                      &info->info502->shi502_netname,
                      pGetParamsOut->Info.p502->shi502_netname);
        BAIL_ON_SRVSVC_ERROR(dwError);

        dwError = SrvSvcSrvAllocateWC16String(
                      &info->info502->shi502_remark,
                      pGetParamsOut->Info.p502->shi502_remark);
        BAIL_ON_SRVSVC_ERROR(dwError);

        dwError = SrvSvcSrvAllocateWC16String(
                      &info->info502->shi502_path,
                      pGetParamsOut->Info.p502->shi502_path);
        BAIL_ON_SRVSVC_ERROR(dwError);

        dwError = SrvSvcSrvAllocateWC16String(
                      &info->info502->shi502_password,
                      pGetParamsOut->Info.p502->shi502_password);
        BAIL_ON_SRVSVC_ERROR(dwError);

        dwError = SrvSvcSrvAllocateMemory(
                      pGetParamsOut->Info.p502->shi502_reserved,
                      (PVOID*)&info->info502->shi502_security_descriptor);
        BAIL_ON_SRVSVC_ERROR(dwError);

        memcpy(&info->info502->shi502_security_descriptor,
               pGetParamsOut->Info.p502->shi502_security_descriptor,
               pGetParamsOut->Info.p502->shi502_reserved);

        info->info502->shi502_type         = pGetParamsOut->Info.p502->shi502_type;
        info->info502->shi502_permissions  = pGetParamsOut->Info.p502->shi502_permissions;
        info->info502->shi502_max_uses     = pGetParamsOut->Info.p502->shi502_max_uses;
        info->info502->shi502_current_uses = pGetParamsOut->Info.p502->shi502_current_uses;

        break;

    case 1005:
        info->info1005 = &pShareInfo->info1005;

        info->info1005->shi1005_flags = pGetParamsOut->Info.p1005->shi1005_flags;

        break;

    default:
        ntStatus = STATUS_INVALID_INFO_CLASS;
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
    LW_SAFE_FREE_MEMORY(pGetParamsOut);

    return dwError;

error:

    memset(info, 0x0, sizeof(*info));

    if (pShareInfo) {
        SrvSvcSrvFreeMemory(pShareInfo);
    }

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
