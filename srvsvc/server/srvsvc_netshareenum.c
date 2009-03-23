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
 *        srvsvc_netshareenum.c
 *
 * Abstract:
 *
 *        Likewise Server Service (srvsvc) RPC client and server
 *
 *        SrvSvcNetShareEnum server API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NET_API_STATUS
SrvSvcNetShareEnum(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *server_name,
    /* [in, out, ref] */ uint32 *level,
    /* [in, out, ref] */ srvsvc_NetShareCtr *ctr,
    /* [in] */ uint32 preferred_maximum_length,
    /* [out] */ uint32 *total_entries,
    /* [in, out] */ uint32 *resume_handle
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
    DWORD dwReturnCode = 0;
    DWORD dwParmError = 0;
    IO_FILE_HANDLE FileHandle;
    IO_STATUS_BLOCK IoStatusBlock;
    PIO_FILE_NAME FileName = NULL;
    ACCESS_MASK DesiredAccess = 0;
    LONG64 AllocationSize = 0;
    FILE_ATTRIBUTES FileAttributes = 0;
    FILE_SHARE_FLAGS ShareAccess = 0;
    FILE_CREATE_DISPOSITION CreateDisposition = 0;
    FILE_CREATE_OPTIONS CreateOptions = 0;
    ULONG IoControlCode = SRV_DEVCTL_ENUM_SHARE;
    PSTR smbpath = NULL;
    IO_FILE_NAME filename;
    IO_STATUS_BLOCK io_status;
    SHARE_INFO_ENUM_PARAMS EnumParamsIn;
    PSHARE_INFO_ENUM_PARAMS pEnumParamsOut = NULL;
    srvsvc_NetShareCtr0 *ctr0 = NULL;
    srvsvc_NetShareCtr1 *ctr1 = NULL;
    srvsvc_NetShareCtr2 *ctr2 = NULL;
    srvsvc_NetShareCtr501 *ctr501 = NULL;
    srvsvc_NetShareCtr502 *ctr502 = NULL;

    memset((void*)&EnumParamsIn, 0, sizeof(EnumParamsIn));

    EnumParamsIn.dwInfoLevel = *level;

    ntStatus = LwShareInfoMarshalEnumParameters(
                        &EnumParamsIn,
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

    ntStatus = LwShareInfoUnmarshalEnumParameters(
                        pOutBuffer,
                        dwOutLength,
                        &pEnumParamsOut
                        );

    switch (pEnumParamsOut->dwInfoLevel) {

    case 0:
        ctr0 = ctr->ctr0;
        ctr0->count = pEnumParamsOut->dwNumEntries;

        ntStatus = SRVSVCAllocateMemory(
                            sizeof(*ctr0->array) * ctr0->count,
                            (void**)&ctr0->array
                            );
        BAIL_ON_NT_STATUS(ntStatus);
        memcpy((void*)ctr0->array, (void*)pEnumParamsOut->info.p0,
               sizeof(*ctr0->array) * ctr0->count);
        break;

    case 1:
        ctr1 = ctr->ctr1;
        ctr1->count = pEnumParamsOut->dwNumEntries;

        ntStatus = SRVSVCAllocateMemory(
                            sizeof(*ctr1->array) * ctr1->count,
                            (void**)&ctr1->array
                            );
        BAIL_ON_NT_STATUS(ntStatus);
        memcpy((void*)ctr1->array, (void*)pEnumParamsOut->info.p1,
               sizeof(*ctr1->array) * ctr1->count);
        break;

    case 2:
        ctr2 = ctr->ctr2;
        ctr2->count = pEnumParamsOut->dwNumEntries;

        ntStatus = SRVSVCAllocateMemory(
                            sizeof(*ctr2->array) * ctr2->count,
                            (void**)&ctr2->array
                            );
        BAIL_ON_NT_STATUS(ntStatus);
        memcpy((void*)ctr2->array, (void*)pEnumParamsOut->info.p2,
               sizeof(*ctr2->array) * ctr2->count);
        break;

    case 501:
        ctr501 = ctr->ctr501;
        ctr501->count = pEnumParamsOut->dwNumEntries;

        ntStatus = SRVSVCAllocateMemory(
                            sizeof(*ctr501->array) * ctr501->count,
                            (void**)&ctr501->array
                            );
        BAIL_ON_NT_STATUS(ntStatus);
        memcpy((void*)ctr501->array, (void*)pEnumParamsOut->info.p501,
               sizeof(*ctr501->array) * ctr501->count);
        break;

    case 502:
        ctr502 = ctr->ctr502;
        ctr502->count = pEnumParamsOut->dwNumEntries;

        ntStatus = SRVSVCAllocateMemory(
                            sizeof(*ctr502->array) * ctr502->count,
                            (void**)&ctr502->array
                            );
        BAIL_ON_NT_STATUS(ntStatus);
        memcpy((void*)ctr502->array, (void*)pEnumParamsOut->info.p502,
               sizeof(*ctr502->array) * ctr502->count);
        break;
    }

    *level         = pEnumParamsOut->dwInfoLevel;
    *total_entries = pEnumParamsOut->dwNumEntries;

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

    if (pEnumParamsOut) {
        SrvSvcFreeMemory(pEnumParamsOut);
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
