/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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

#include "includes.h"

NTSTATUS
SrvMarshallPipeInfo(
    IN     PFILE_PIPE_INFORMATION       pPipeInfo,
    IN     PFILE_PIPE_LOCAL_INFORMATION pPipeLocalInfo,
    IN OUT PUSHORT                      pusDeviceState
    )
{
    NTSTATUS ntStatus = 0;
    USHORT   deviceState = 0;

    switch (pPipeInfo->CompletionMode)
    {
        case PIPE_NOWAIT:

            deviceState |= 0x8000;

            break;

        case PIPE_WAIT: /* blocking */

            break;

        default:

            ntStatus = STATUS_DATA_ERROR;

            break;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    switch (pPipeLocalInfo->NamedPipeEnd)
    {
        case FILE_PIPE_CLIENT_END:

            break;

        case FILE_PIPE_SERVER_END:

            deviceState |= 0x4000;

            break;

        default:

            ntStatus = STATUS_DATA_ERROR;

            break;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    switch (pPipeLocalInfo->NamedPipeType)
    {
        case FILE_PIPE_BYTE_STREAM_TYPE:

            break;

        case FILE_PIPE_MESSAGE_TYPE:

            deviceState |= 0x400;

            break;

        default:

            ntStatus = STATUS_DATA_ERROR;

            break;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    switch (pPipeInfo->ReadMode)
    {
        case PIPE_READMODE_MESSAGE:

            deviceState |= 0x100;

            break;

        case PIPE_READMODE_BYTE:

            break;

        default:

            ntStatus = STATUS_DATA_ERROR;

            break;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    deviceState |= (BYTE)SMB_MIN(pPipeLocalInfo->CurrentInstances, 0xFF);

    *pusDeviceState = deviceState;

cleanup:

    return ntStatus;

error:

    *pusDeviceState = 0;

    goto cleanup;
}
