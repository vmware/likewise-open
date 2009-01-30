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
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        main.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO)
 *
 *        Test Program for exercising SMB Client API
 *
 * Authors: Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */

#define _POSIX_PTHREAD_SEMANTICS 1

#include "config.h"
#include "lwiosys.h"
#include "lwio/lwio.h"
#include "lwiodef.h"
#include "lwioutils.h"

NTSTATUS
CreatePipeClientThread(
    const char *pipename
    );

int
main(
    int    argc,
    char** argv
    )
{
    NTSTATUS ntStatus = 0;
    const char* pipename = NULL;
    ULONG ulNumConnections = 0;

    if (argc < 3)
    {
        printf("Usage: test_npserver <pipename> <numconnections>\n");
        exit(1);
    }

    pipename = argv[1];
    ulNumConnections = atoi(argv[2]);

    ntStatus = CreatePipeClientThread(pipename);
    BAIL_ON_NT_STATUS(ntStatus);

error:

    return ntStatus;
}

NTSTATUS
CreatePipeClientThread(
    const char *pipename
    )
{
    NTSTATUS ntStatus = 0;
    PSTR smbpath = NULL;
    //PIO_ACCESS_TOKEN acctoken = NULL;
    IO_FILE_NAME filename;
    IO_STATUS_BLOCK io_status;
    IO_FILE_HANDLE FileHandle = 0;
    BYTE InBuffer[2048];
    BYTE OutBuffer[2048];
    ULONG InLength = sizeof(InBuffer);
    // ULONG OutLength = sizeof(OutBuffer);
    ULONG ulStringLength = 0;

    strcpy((PSTR)OutBuffer,"This is an extremely long sentence sent from the client");
    ulStringLength = strlen((PSTR)OutBuffer);
    ulStringLength++;

    ntStatus = LwRtlCStringAllocatePrintf(
                    &smbpath,
                    "\\npvfs\\%s",
                    (char*) pipename);
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
                        &io_status,
                        &filename,
                        NULL,
                        NULL,
                        GENERIC_READ | GENERIC_WRITE,
                        0,
                        0,
                        SHARE_WRITE | SHARE_READ,
                        OPEN_EXISTING,
                        0,
                        NULL,
                        0,
                        NULL
                        );
    BAIL_ON_NT_STATUS(ntStatus);

    while (1) {

        printf("Client Write to Server: %s\n", (PSTR)OutBuffer);
        ntStatus = NtWriteFile(
                        FileHandle,
                        NULL,
                        &io_status,
                        OutBuffer,
                        ulStringLength,
                        NULL,
                        NULL
                        );
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = NtReadFile(
                        FileHandle,
                        NULL,
                        &io_status,
                        InBuffer,
                        InLength,
                        NULL,
                        NULL
                        );
        BAIL_ON_NT_STATUS(ntStatus);
        if (io_status.BytesTransferred) {
            printf("Client Read from Server: %s\n", (PSTR)InBuffer);

        }


    }

error:

    return ntStatus;
}

