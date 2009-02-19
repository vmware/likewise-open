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
 *        smbfileapi.h
 *
 * Abstract:
 *
 *        SMB-specific API functions
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 */

#include "includes.h"

#define MAX_KEY_LENGTH (1024*10)

LW_NTSTATUS
LwIoCtxGetSessionKey(
    LW_PIO_CONTEXT pContext,
    IO_FILE_HANDLE File,
    LW_PUSHORT pKeyLength,
    LW_PBYTE* ppKeyBuffer
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    IO_STATUS_BLOCK IoStatus;
    union
    {
        BYTE Buffer[sizeof(IO_FSCTL_SMB_SESSION_KEY) + MAX_KEY_LENGTH];
        IO_FSCTL_SMB_SESSION_KEY Key;
    } u;
    PBYTE pKeyBuffer = NULL;

    Status = 
        LwNtCtxFsControlFile(
            pContext,
            File,
            NULL,
            &IoStatus,
            IO_FSCTL_SMB_GET_SESSION_KEY,
            NULL,
            0,
            u.Buffer,
            sizeof(u.Buffer));
    BAIL_ON_NT_STATUS(Status);

    Status = LwIoAllocateMemory(u.Key.SessionKeyLength, OUT_PPVOID(&pKeyBuffer));
    BAIL_ON_NT_STATUS(Status);

    memcpy(pKeyBuffer, u.Key.Buffer, (size_t) u.Key.SessionKeyLength);
    
    *pKeyLength = u.Key.SessionKeyLength;
    *ppKeyBuffer = pKeyBuffer;

error:

    return Status;
}

LW_NTSTATUS
LwIoCtxConnectNamedPipe(
    LW_PIO_CONTEXT pContext,
    IO_FILE_HANDLE File
    )
{
    IO_STATUS_BLOCK IoStatusBlock = {0};

    return NtFsControlFile(
        File,
        NULL,
        &IoStatusBlock,
        0x2,
        NULL,
        0,
        NULL,
        0
        );
}
