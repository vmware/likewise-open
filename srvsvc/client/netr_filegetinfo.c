/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

#include "includes.h"

static
NET_API_STATUS
SrvSvcCopyNetFileInfo(
    UINT32              level,
    srvsvc_NetFileInfo* info,
    UINT8**             bufptr
    );


NET_API_STATUS
NetrFileGetInfo(
    PSRVSVC_CONTEXT pContext,          /* IN              */
    PCWSTR          pwszServername,    /* IN    OPTIONAL  */
    DWORD           dwFileId,          /* IN              */
    DWORD           dwInfoLevel,       /* IN              */
    PBYTE*          ppBuffer           /*    OUT          */
    )
{
    NET_API_STATUS status = ERROR_SUCCESS;
    dcethread_exc* pDceException = NULL;
    srvsvc_NetFileInfo info;

    BAIL_ON_INVALID_PTR(pContext, status);
    BAIL_ON_INVALID_PTR(ppBuffer, status);

    memset(&info, 0, sizeof(info));
    *ppBuffer = NULL;

    TRY
    {
        status = _NetrFileGetInfo(
                    pContext->hBinding,
                    (PWSTR)pwszServername,
                    dwFileId,
                    dwInfoLevel,
                    &info);
    }
    CATCH_ALL(pDceException)
    {
        NTSTATUS ntStatus = LwRpcStatusToNtStatus(pDceException->match.value);
        status = LwNtStatusToWin32Error(ntStatus);
    }
    ENDTRY;
    BAIL_ON_WIN_ERROR(status);

    status = SrvSvcCopyNetFileInfo(dwInfoLevel, &info, ppBuffer);
    BAIL_ON_WIN_ERROR(status);

cleanup:

    SrvSvcClearNetFileInfo(dwInfoLevel, &info);

    return status;

error:

    goto cleanup;
}

static
NET_API_STATUS
SrvSvcCopyNetFileInfo(
    UINT32              level,
    srvsvc_NetFileInfo* info,
    UINT8**             bufptr
    )
{
    NET_API_STATUS status = ERROR_SUCCESS;
    void *ptr = NULL;

    BAIL_ON_INVALID_PTR(bufptr, status);
    BAIL_ON_INVALID_PTR(info, status);

    *bufptr = NULL;

    switch (level) {
    case 2:
        if (info->info2) {
            PFILE_INFO_2 a2;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(FILE_INFO_2),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            a2 = (PFILE_INFO_2)ptr;

            *a2 = *info->info2;
        }
        break;
    case 3:
        if (info->info3) {
            PFILE_INFO_3 pFileInfo;

            status = SrvSvcAllocateMemory(&ptr,
                                          sizeof(FILE_INFO_3),
                                          NULL);
            BAIL_ON_WIN_ERROR(status);

            pFileInfo = (PFILE_INFO_3)ptr;

            *pFileInfo = *info->info3;

            pFileInfo->fi3_path_name = NULL;
            pFileInfo->fi3_username  = NULL;

            if (info->info3->fi3_path_name)
            {
                status = SrvSvcAddDepStringW(
                            pFileInfo,
                            info->info3->fi3_path_name,
                            &pFileInfo->fi3_path_name);
                BAIL_ON_WIN_ERROR(status);
            }
            if (info->info3->fi3_username)
            {
                status = SrvSvcAddDepStringW(
                            pFileInfo,
                            info->info3->fi3_username,
                            &pFileInfo->fi3_username);
                BAIL_ON_WIN_ERROR(status);
            }
        }
        break;
    }

    *bufptr = (UINT8 *)ptr;

cleanup:
    return status;

error:
    if (ptr) {
        SrvSvcFreeMemory(ptr);
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
