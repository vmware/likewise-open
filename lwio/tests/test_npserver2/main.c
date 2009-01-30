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
CreateServerPipe(
    char * pipename
    );

NTSTATUS
NtConnectNamedPipe(
    IO_FILE_HANDLE FileHandle
    );

int
main(int argc,
    char **argv
    )
{
    char *pipename = NULL;
    ULONG i = 0;
    ULONG ulNumConnections = 0;
    pthread_t thread;

    if (argc < 3)
    {
        printf("Usage: test_npserver <pipename> <num_connections>\n");
        exit(1);
    }

    pipename = argv[1];
    ulNumConnections = atoi(argv[2]);

    for (i = 0; i < 100; i++) {
        pthread_create(&thread, NULL, (void *)CreateServerPipe, pipename);

    }
    return(0);
}

NTSTATUS
CreateServerPipe(
    char * pipename
    )
{
    NTSTATUS ntStatus = 0;
    PSTR smbpath = NULL;
    //PIO_ACCESS_TOKEN acctoken = NULL;
    IO_FILE_NAME filename;
    IO_STATUS_BLOCK io_status;
    ULONG NamedPipeType = 0;
    ULONG CompletionMode = 0;
    ULONG MaximumInstances =0;
    ULONG ReadMode = 0;
    ULONG InboundQuota = 0;
    ULONG OutboundQuota = 0;
    LONG64 DefaultTimeOut = 0;
    IO_FILE_HANDLE FileHandle = 0;
    //BYTE Buffer[4096];
    //ULONG Length = 0;

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

    ntStatus = NtCreateNamedPipeFile(
                        &FileHandle,
                        NULL,
                        &io_status,
                        &filename,
                        NULL,
                        NULL,
                        GENERIC_READ | GENERIC_WRITE,
                        SHARE_WRITE | SHARE_READ,
                        OPEN_EXISTING,
                        0,
                        NamedPipeType,
                        ReadMode,
                        CompletionMode,
                        MaximumInstances,
                        InboundQuota,
                        OutboundQuota,
                        &DefaultTimeOut
                        );
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NtConnectNamedPipe(
                    FileHandle
                    );
    BAIL_ON_NT_STATUS(ntStatus);
/*
    while (1) {


        ntStatus = NtReadFile(
                        FileHandle,
                        NULL,
                        &io_status,
                        Buffer,
                        Length,
                        NULL,
                        NULL
                        );
        BAIL_ON_NT_STATUS(ntStatus);


        ntStatus = NtWriteFile(
                        FileHandle,
                        NULL,
                        &io_status,
                        Buffer,
                        Length,
                        NULL,
                        NULL
                        );
        BAIL_ON_NT_STATUS(ntStatus);

    }

    NtCloseFile(FileHandle);

    */

error:

    return(ntStatus);
}



NTSTATUS
NtConnectNamedPipe(
    IO_FILE_HANDLE FileHandle
    )
{

    NTSTATUS ntStatus = 0;
    IO_STATUS_BLOCK IoStatusBlock;

    ntStatus = NtFsControlFile(
                    FileHandle,
                    NULL,
                    &IoStatusBlock,
                    0x2,
                    NULL,
                    0,
                    NULL,
                    0
                    );
    return(ntStatus);
}


