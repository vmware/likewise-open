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

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        main.c
 *
 * Abstract:
 *
 *        LWIO client interface program
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 */

#include "includes.h"

static
VOID
PrintServerStats_level_0(
    PIO_STATISTICS_INFO_0 pStats
    );

DWORD
ShowServerStats(
    VOID
    )
{
    DWORD                           dwError   = 0;
    NTSTATUS                        ntStatus  = STATUS_SUCCESS;
    wchar16_t                       wszName[] = {'\\','s','r','v',0};
    IO_FILE_NAME                    fileName  =
                                        {
                                                .RootFileHandle = NULL,
                                                .FileName       = &wszName[0],
                                                .IoNameOptions  = 0
                                        };
    IO_FILE_HANDLE                  hDevice       = NULL;
    IO_STATUS_BLOCK                 ioStatusBlock = {0};
    PIO_ASYNC_CONTROL_BLOCK         pAcb     = NULL;
    IO_STATISTICS_INFO_INPUT_BUFFER inBuf    = {0};
    IO_STATISTICS_INFO_0            stats    = {0};

    ntStatus = NtCreateFile(
                  &hDevice,
                  pAcb,
                  &ioStatusBlock,
                  &fileName,
                  NULL,
                  NULL,
                  0,
                  0,
                  0,
                  0,
                  0,
                  0,
                  NULL,
                  0,
                  NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NtDeviceIoControlFile(
                    hDevice,
                    pAcb,
                    &ioStatusBlock,
                    IO_DEVICE_CTL_STATISTICS,
                    &inBuf,
                    sizeof(inBuf),
                    &stats,
                    sizeof(stats));
    BAIL_ON_NT_STATUS(ntStatus);

    switch (inBuf.dwInfoLevel)
    {
        case 0:

            PrintServerStats_level_0(&stats);

            break;

        default:

            LWIO_LOG_WARNING("Unsupported info level [%d]\n", inBuf.dwInfoLevel);

            break;
    }

cleanup:

    if (hDevice)
    {
        NtCloseFile(hDevice);
    }

    return dwError;

error:

    fprintf(stderr,
            "Failed to get server statistics [status %s = 0x%08X (%d); %s]\n",
            NtStatusToName(ntStatus),
            ntStatus,
            ntStatus,
            NtStatusToDescription(ntStatus));

    dwError = LwNtStatusToWin32Error(ntStatus);

    goto cleanup;
}

static
VOID
PrintServerStats_level_0(
    PIO_STATISTICS_INFO_0 pStats
    )
{
    printf("Server statistics [level 0]: \n\n");

    printf("Number of connections:           [%llu]\n",
           (unsigned long long)pStats->ullNumConnections);

    printf("Maximum Number of connections:   [%llu]\n",
            (unsigned long long)pStats->ullMaxNumConnections);

    printf("Number of sessions:              [%llu]\n",
            (unsigned long long)pStats->ullNumSessions);

    printf("Maximum Number of sessions:      [%llu]\n",
            (unsigned long long)pStats->ullMaxNumSessions);

    printf("Number of tree connects:         [%llu]\n",
            (unsigned long long)pStats->ullNumTreeConnects);

    printf("Maximum Number of tree connects: [%llu]\n",
            (unsigned long long)pStats->ullMaxNumTreeConnects);

    printf("Number of open files:            [%llu]\n",
            (unsigned long long)pStats->ullNumOpenFiles);

    printf("Maximum Number of open files:    [%llu]\n",
            (unsigned long long)pStats->ullMaxNumOpenFiles);
}
