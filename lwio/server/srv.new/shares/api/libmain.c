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
 *        mapping.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Share Repository API
 *
 *        Library Main
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

NTSTATUS
SrvShareInit(
    VOID
    )
{
	NTSTATUS status = STATUS_NOT_IMPLEMENTED;

#if defined(SRV_SHAREAPI_USE_LWSHARE)

	status = LwShareRepositoryInit(&gSrvShareApi.pFnTable);

#endif

#if 0
    PSTR pszFileSystemRoot = NULL;
    CHAR szTmpFileSystemRoot[] = LWIO_SRV_FILE_SYSTEM_ROOT_A;
    CHAR szPipeSystemRoot[] = LWIO_SRV_PIPE_SYSTEM_ROOT_A;

    ntStatus = SMBAllocateStringPrintf(
                    &pszFileSystemRoot,
                    "%s%s%s",
                    &szTmpFileSystemRoot[0],
                    (((szTmpFileSystemRoot[strlen(&szTmpFileSystemRoot[0])-1] == '/') ||
                      (szTmpFileSystemRoot[strlen(&szTmpFileSystemRoot[0])-1] == '\\')) ? "" : "\\"),
                    LWIO_SRV_DEFAULT_SHARE_PATH_A);
    BAIL_ON_NT_STATUS(ntStatus);

	// TODO: Add shares if they don't exist
    ntStatus = SrvShareDbAdd(
                    "IPC$",
                    &szPipeSystemRoot[0],
                    "Remote IPC",
                    NULL,
                    0,
                    "IPC");
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvShareDbAdd(
                    "C$",
                    pszFileSystemRoot,
                    "Default Share",
                    NULL,
                    0,
                    "A:");
    BAIL_ON_NT_STATUS(ntStatus);


    if (pszFileSystemRoot)
    {
        LwRtlMemoryFree(pszFileSystemRoot);
    }
#endif

	return status;
}

NTSTATUS
SrvShareShutdown(
    VOID
    )
{
	NTSTATUS status = STATUS_NOT_IMPLEMENTED;

#if defined(SRV_SHAREAPI_USE_LWSHARE)

	status = LwShareRepositoryShutdown(gSrvShareApi.pFnTable);

#endif
	BAIL_ON_NT_STATUS(status);

	gSrvShareApi.pFnTable = NULL;

error:

	return status;
}

