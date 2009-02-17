/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
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
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software 2007
 * All rights reserved.
 *
 * SrvSvc Server
 *
 */

#include "includes.h"


NET_API_STATUS
SrvSvcNetFileEnum(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *server_name,
    /* [in] */ wchar16_t *basepath,
    /* [in] */ wchar16_t *username,
    /* [in, out] */ uint32 *level,
    /* [in, out] */ srvsvc_NetFileCtr *ctr,
    /* [in] */ uint32 prefered_maximum_length,
    /* [out] */ uint32 *total_entries,
    /* [in, out] */ uint32 *resume_handle
    )
{
    return ERROR_NOT_SUPPORTED;
}

NET_API_STATUS
SrvSvcNetFileGetInfo(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *server_name,
    /* [in] */ uint32 fileid,
    /* [in] */ uint32 level,
    /* [out] */ srvsvc_NetFileInfo *info
    )
{
    return ERROR_NOT_SUPPORTED;
}

NET_API_STATUS
SrvSvcNetFileClose(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *server_name,
    /* [in] */ uint32 fileid
    )
{

    return ERROR_NOT_SUPPORTED;
}

NET_API_STATUS
SrvSvcNetSessionEnum(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *server_name,
    /* [in] */ wchar16_t *unc_client_name,
    /* [in] */ wchar16_t *username,
    /* [in, out] */ uint32 *level,
    /* [in, out] */ srvsvc_NetSessCtr *ctr,
    /* [in] */ uint32 prefered_maximum_length,
    /* [out] */ uint32 *total_entries,
    /* [in, out] */ uint32 *resume_handle
    )
{

    return ERROR_NOT_SUPPORTED;
}

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
    DWORD dwOutLength = 0;
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

NET_API_STATUS
SrvSvcNetShareEnum(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *server_name,
    /* [in, out] */ uint32 *level,
    /* [in, out] */ srvsvc_NetShareCtr *ctr,
    /* [in] */ uint32 prefered_maximum_length,
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
    DWORD dwOutLength = 0;
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
    ULONG IoControlCode = 0;

    dwError = MarshallShareInfotoFlatBuffer(
                    *level,
                    ctr,
                    &pInBuffer,
                    &dwInLength
                    );
    BAIL_ON_ERROR(dwError);

    ntStatus = NtCreateFile(
                        &FileHandle,
                        NULL,
                        &IoStatusBlock,
                        FileName,
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

    dwError = UnmarshallAddSetResponse(
                    pOutBuffer,
                    &dwReturnCode,
                    &dwParmError);

#if 0
    *parm_error = dwParmError;
    dwError = dwReturnCode;
#endif

cleanup:

    if(pInBuffer) {
        SrvSvcFreeMemory(pInBuffer);
    }

    return(dwError);

error:


    if (pOutBuffer) {
        SrvSvcFreeMemory(pOutBuffer);
    }

    goto cleanup;
    return ERROR_NOT_SUPPORTED;
}

NET_API_STATUS
SrvSvcNetShareGetInfo(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *server_name,
    /* [in] */ wchar16_t *netname,
    /* [in] */ uint32 level,
    /* [out] */ srvsvc_NetShareInfo *info
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
    DWORD dwOutLength = 0;
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
    ULONG IoControlCode = 0;

    dwError = MarshallShareInfotoFlatBuffer(
                    level,
                    info,
                    &pInBuffer,
                    &dwInLength
                    );
    BAIL_ON_ERROR(dwError);

    ntStatus = NtCreateFile(
                        &FileHandle,
                        NULL,
                        &IoStatusBlock,
                        FileName,
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

    dwError = UnmarshallAddSetResponse(
                    pOutBuffer,
                    &dwReturnCode,
                    &dwParmError);

#if 0
    *parm_error = dwParmError;
    dwError = dwReturnCode;
#endif

cleanup:

    if(pInBuffer) {
        SrvSvcFreeMemory(pInBuffer);
    }

    return(dwError);

error:


    if (pOutBuffer) {
        SrvSvcFreeMemory(pOutBuffer);
    }

    goto cleanup;
}

NET_API_STATUS
SrvSvcNetShareSetInfo(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *server_name,
    /* [in] */ wchar16_t *netname,
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
    DWORD dwOutLength = 0;
    DWORD dwBytesReturned = 0;
    HANDLE hDevice = (HANDLE)NULL;
    BOOLEAN bRet = FALSE;
    DWORD dwParmError = 0;
    DWORD dwReturnCode = 0;
    IO_FILE_HANDLE FileHandle;
    IO_STATUS_BLOCK IoStatusBlock;
    PIO_FILE_NAME FileName = NULL;
    ACCESS_MASK DesiredAccess = 0;
    LONG64 AllocationSize = 0;
    FILE_ATTRIBUTES FileAttributes = 0;
    FILE_SHARE_FLAGS ShareAccess = 0;
    FILE_CREATE_DISPOSITION CreateDisposition = 0;
    FILE_CREATE_OPTIONS CreateOptions = 0;
    ULONG IoControlCode = 0;

    dwError = MarshallShareInfotoFlatBuffer(
                    level,
                    &info,
                    &pInBuffer,
                    &dwInLength
                    );
    BAIL_ON_ERROR(dwError);

    ntStatus = NtCreateFile(
                        &FileHandle,
                        NULL,
                        &IoStatusBlock,
                        FileName,
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

    dwError = UnmarshallAddSetResponse(
                    pOutBuffer,
                    &dwReturnCode,
                    &dwParmError);

    *parm_error = dwParmError;
    dwError = dwReturnCode;

cleanup:

    if(pInBuffer) {
        SrvSvcFreeMemory(pInBuffer);
    }

    return(dwError);

error:


    if (pOutBuffer) {
        SrvSvcFreeMemory(pOutBuffer);
    }

    goto cleanup;
}

NET_API_STATUS
SrvSvcNetShareDel(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *server_name,
    /* [in] */ wchar16_t *netname,
    /* [in] */ uint32 reserved
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
    DWORD dwOutLength = 0;
    DWORD dwBytesReturned = 0;
    HANDLE hDevice = (HANDLE)NULL;
    BOOLEAN bRet = FALSE;
    DWORD dwParmError = 0;
    DWORD dwReturnCode = 0;
    IO_FILE_HANDLE FileHandle;
    IO_STATUS_BLOCK IoStatusBlock;
    PIO_FILE_NAME FileName = NULL;
    ACCESS_MASK DesiredAccess = 0;
    LONG64 AllocationSize = 0;
    FILE_ATTRIBUTES FileAttributes = 0;
    FILE_SHARE_FLAGS ShareAccess = 0;
    FILE_CREATE_DISPOSITION CreateDisposition = 0;
    FILE_CREATE_OPTIONS CreateOptions = 0;
    ULONG IoControlCode = 0;

    ntStatus = NtCreateFile(
                        &FileHandle,
                        NULL,
                        &IoStatusBlock,
                        FileName,
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

    dwError = UnmarshallAddSetResponse(
                    pOutBuffer,
                    &dwReturnCode,
                    &dwParmError);

#if 0
    *parm_error = dwParmError;
    dwError = dwReturnCode;
#endif

cleanup:

    if(pInBuffer) {
        SrvSvcFreeMemory(pInBuffer);
    }

    return(dwError);

error:


    if (pOutBuffer) {
        SrvSvcFreeMemory(pOutBuffer);
    }

    goto cleanup;
}

NET_API_STATUS
SrvSvcNetServerGetInfo(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *server_name,
    /* [in] */ uint32 level,
    /* [out] */ srvsvc_NetSrvInfo *info
    )
{
    return ERROR_NOT_SUPPORTED;
}

NET_API_STATUS
SrvSvcNetServerSetInfo(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *server_name,
    /* [in] */ uint32 level,
    /* [in] */ srvsvc_NetSrvInfo info,
    /* [in, out] */ uint32 *parm_error
    )
{
    return ERROR_NOT_SUPPORTED;
}

NET_API_STATUS
SrvSvcNetRemoteTOD(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *server_name,
    /* [out] */ TIME_OF_DAY_INFO **info
    )
{
    return ERROR_NOT_SUPPORTED;
}

