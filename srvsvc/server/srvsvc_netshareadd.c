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
 *        srvsvc_netshareadd.c
 *
 * Abstract:
 *
 *        Likewise Server Service (srvsvc) RPC client and server
 *
 *        SrvSvcNetShareAdd server API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NET_API_STATUS
SrvSvcNetShareAdd(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *server_name,
    /* [in] */ uint32 level,
    /* [in] */ srvsvc_NetShareInfo info,
    /* [in, out] */ uint32 *parm_error
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
    HANDLE hDevice = (HANDLE)NULL;
    BOOLEAN bRet = FALSE;
    DWORD dwReturnCode = 0;
    DWORD dwParmError = 0;
    IO_FILE_HANDLE FileHandle = NULL;
    IO_STATUS_BLOCK IoStatusBlock;
    ACCESS_MASK DesiredAccess = 0;
    LONG64 AllocationSize = 0;
    FILE_ATTRIBUTES FileAttributes = 0;
    FILE_SHARE_FLAGS ShareAccess = 0;
    FILE_CREATE_DISPOSITION CreateDisposition = 0;
    FILE_CREATE_OPTIONS CreateOptions = 0;
    ULONG IoControlCode = 0;
    PSTR smbpath = NULL;
    //PIO_ACCESS_TOKEN acctoken = NULL;
    IO_FILE_NAME filename;
    IO_STATUS_BLOCK io_status;
    PSHARE_INFO_ADD_PARAMS pAddParams = NULL;

    ntStatus = LwShareInfoMarshalAddParameters(
                        pAddParams,
                        &pInBuffer,
                        &dwInLength
                        );
    BAIL_ON_NT_STATUS(ntStatus);

    dwError = SRVSVCAllocateMemory(
                        dwOutLength,
                        &pOutBuffer
                        );
    BAIL_ON_ERROR(dwError);

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



cleanup:

    if(pInBuffer) {
        SrvSvcFreeMemory(pInBuffer);
    }

    if (FileHandle) {

        NtCloseFile(FileHandle);

    }

    return(dwError);

error:


    if (pOutBuffer) {
        SrvSvcFreeMemory(pOutBuffer);
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
