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
    PDWORD             pdwTotalEntries,      /* [out]     */
    PDWORD             pdwResumeHandle       /* [in, out] */
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
    ULONG                   dwIoControlCode     = SRV_DEVCTL_ENUM_SESSIONS;

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

cleanup:

    if (hFile)
    {
        NtCloseFile(hFile);
    }

    return dwError;

error:

    if (ntStatus != STATUS_SUCCESS)
    {
        dwError = LwNtStatusToWin32Error(ntStatus);
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
    return ERROR_NOT_SUPPORTED;
}
