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
 *        Test Program for exercising the PVFS driver
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 *
 */

#define _POSIX_PTHREAD_SEMANTICS 1

#include "config.h"
#include <lw/base.h>
#include "lwiosys.h"
#include "lwio/lwio.h"
#include "lwiodef.h"
#include "lwioutils.h"

int main(int argc, char *argv[])
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    IO_FILE_HANDLE hFile = (IO_FILE_HANDLE)NULL;
    IO_STATUS_BLOCK StatusBlock;
    IO_FILE_NAME Filename;
    PSTR pszPath;
    size_t bytes = 0;
    int fd = -1;
    BYTE pBuffer[1024];
    IO_STATUS_BLOCK statusBlock = {0};


    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <filename> <src file>\n", argv[0]);
        return -1;
    }

    pszPath = argv[1];

    Filename.RootFileHandle = (IO_FILE_HANDLE)NULL;
    Filename.IoNameOptions = 0;
    ntError = RtlWC16StringAllocateFromCString(&Filename.FileName, pszPath);


    ntError = NtCreateFile(&hFile,
                           NULL,
                           &StatusBlock,
                           &Filename,
                           NULL,
                           NULL,
                           FILE_ALL_ACCESS,
                           0,
                           FILE_ATTRIBUTE_NORMAL,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           FILE_CREATE,
                           FILE_NON_DIRECTORY_FILE,
                           NULL,
                           0,
                           NULL);
    BAIL_ON_NT_STATUS(ntError);

    if ((fd = open(argv[2], O_RDONLY, 0)) == -1) {
        fprintf(stderr, "Failed to open local file \"%s\" for copy.\n",
                argv[2]);
        ntError = STATUS_UNSUCCESSFUL;
        BAIL_ON_NT_STATUS(ntError);
    }

    do {
        if ((bytes = read(fd, pBuffer, sizeof(pBuffer))) == -1) {
            fprintf(stderr, "Read failed!\n");
            ntError = STATUS_UNSUCCESSFUL;
            BAIL_ON_NT_STATUS(ntError);
        }

        ntError = NtWriteFile(hFile,
                              NULL,
                              &statusBlock,
                              pBuffer,
                              bytes,
                              0, 0);
        BAIL_ON_NT_STATUS(ntError);
    } while (bytes != 0);

    ntError = NtCloseFile(hFile);
    BAIL_ON_NT_STATUS(ntError);


cleanup:
    printf("Final NTSTATUS was %s (%s)\n",
           NtStatusToDescription(ntError),
           NtStatusToSymbolicName(ntError));

    return ntError == STATUS_SUCCESS;

error:
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
