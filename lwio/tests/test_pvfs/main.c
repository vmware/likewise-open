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

#include <stdio.h>
#include <errno.h>

/******************************************************
 *****************************************************/

static void
PrintUsage(
    char *pszProgName
    )
{
    fprintf(stderr, "Usage: %s <command> [command options]\n", pszProgName);
    fprintf(stderr, "(All pvfs files should be given in the format \"/pvfs/path/...\")\n");
    fprintf(stderr, "    -c <src> <dst>    Copy src to the Pvfs dst file\n");
    fprintf(stderr, "    -C <src> <dst>    Copy the pvfs src file to the local dst file\n");
    fprintf(stderr, "    -S <file>         Stat a Pvfs file\n");
    fprintf(stderr, "\n");

    return;
}

/******************************************************
 *****************************************************/

static NTSTATUS
CopyFileToPvfs(
    int argc,
    char *argv[]
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    IO_FILE_HANDLE hFile = (IO_FILE_HANDLE)NULL;
    IO_STATUS_BLOCK StatusBlock = {0};
    IO_FILE_NAME DstFilename = {0};
    PSTR pszSrcPath = NULL;
    PSTR pszDstPath = NULL;
    size_t bytes = 0;
    int fd = -1;
    BYTE pBuffer[1024];

    if (argc != 2)
    {
        fprintf(stderr, "Missing parameters. Requires <src> and <dst>\n");
        ntError = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntError);
    }

    pszSrcPath = argv[0];
    pszDstPath = argv[1];

    DstFilename.RootFileHandle = (IO_FILE_HANDLE)NULL;
    DstFilename.IoNameOptions = 0;
    ntError = RtlWC16StringAllocateFromCString(&DstFilename.FileName, pszDstPath);
    BAIL_ON_NT_STATUS(ntError);

    /* Open the remote Destination file */

    ntError = NtCreateFile(&hFile,
                           NULL,
                           &StatusBlock,
                           &DstFilename,
                           NULL,
                           NULL,
                           FILE_ALL_ACCESS,
                           0,
                           FILE_ATTRIBUTE_NORMAL,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           FILE_OVERWRITE,
                           FILE_NON_DIRECTORY_FILE,
                           NULL,
                           0,
                           NULL);
    BAIL_ON_NT_STATUS(ntError);

    /* Open the local source */

    if ((fd = open(pszSrcPath, O_RDONLY, 0)) == -1)
    {
        fprintf(stderr, "Failed to open local file \"%s\" for copy (%s).\n",
                pszDstPath, strerror(errno));
        ntError = STATUS_UNSUCCESSFUL;
        BAIL_ON_NT_STATUS(ntError);
    }

    /* Copy the file */

    do {
        if ((bytes = read(fd, pBuffer, sizeof(pBuffer))) == -1)
        {
            fprintf(stderr, "Read failed! (%s)\n", strerror(errno));
            ntError = STATUS_UNSUCCESSFUL;
            BAIL_ON_NT_STATUS(ntError);
        }

        if (bytes == 0) {
            break;
        }

        ntError = NtWriteFile(hFile,
                             NULL,
                             &StatusBlock,
                             pBuffer,
                             bytes,
                             0, 0);
        BAIL_ON_NT_STATUS(ntError);

    } while (bytes != 0);

    close(fd);

    ntError = NtCloseFile(hFile);
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    return ntError;

error:
    if (hFile) {
        NtCloseFile(hFile);
    }

    if (fd != -1) {
        close(fd);
    }
    goto cleanup;
}

/******************************************************
 *****************************************************/

static NTSTATUS
CopyFileFromPvfs(
    int argc,
    char *argv[]
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    IO_FILE_HANDLE hFile = (IO_FILE_HANDLE)NULL;
    IO_STATUS_BLOCK StatusBlock = {0};
    IO_FILE_NAME SrcFilename = {0};
    PSTR pszSrcPath = NULL;
    PSTR pszDstPath = NULL;
    size_t bytes = 0;
    int fd = -1;
    BYTE pBuffer[1024];

    if (argc != 2)
    {
        fprintf(stderr, "Missing parameters. Requires <src> and <dst>\n");
        ntError = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntError);
    }

    pszSrcPath = argv[0];
    pszDstPath = argv[1];

    SrcFilename.RootFileHandle = (IO_FILE_HANDLE)NULL;
    SrcFilename.IoNameOptions = 0;
    ntError = RtlWC16StringAllocateFromCString(&SrcFilename.FileName, pszSrcPath);
    BAIL_ON_NT_STATUS(ntError);

    /* Open the remote source file */

    ntError = NtCreateFile(&hFile,
                           NULL,
                           &StatusBlock,
                           &SrcFilename,
                           NULL,
                           NULL,
                           FILE_ALL_ACCESS,
                           0,
                           FILE_ATTRIBUTE_NORMAL,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           FILE_OPEN,
                           FILE_NON_DIRECTORY_FILE,
                           NULL,
                           0,
                           NULL);
    BAIL_ON_NT_STATUS(ntError);

    /* Open the local destination */

    if ((fd = open(pszDstPath, O_WRONLY | O_TRUNC | O_CREAT, 0666)) == -1)
    {
        fprintf(stderr, "Failed to open local file \"%s\" for copy (%s).\n",
                pszDstPath, strerror(errno));
        ntError = STATUS_UNSUCCESSFUL;
        BAIL_ON_NT_STATUS(ntError);
    }

    /* Copy the file */

    do {
        ntError = NtReadFile(hFile,
                             NULL,
                             &StatusBlock,
                             pBuffer,
                             sizeof(pBuffer),
                             0, 0);
        BAIL_ON_NT_STATUS(ntError);

        if (StatusBlock.BytesTransferred == 0) {
            break;
        }

        if ((bytes = write(fd, pBuffer, StatusBlock.BytesTransferred)) == -1)
        {
            fprintf(stderr, "Write failed! (%s)\n", strerror(errno));
            ntError = STATUS_UNSUCCESSFUL;
            BAIL_ON_NT_STATUS(ntError);
        }


    } while (bytes != 0);

    close(fd);

    ntError = NtCloseFile(hFile);
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    return ntError;

error:
    if (hFile) {
        NtCloseFile(hFile);
    }

    if (fd != -1) {
        close(fd);
    }
    goto cleanup;
}


/******************************************************
 *****************************************************/

static NTSTATUS
StatRemoteFile(
    char *pszFilename
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    FILE_BASIC_INFORMATION FileBasicInfo = {0};
    FILE_STANDARD_INFORMATION FileStdInfo = {0};
    IO_FILE_NAME Filename = {0};
    IO_FILE_HANDLE hFile = (IO_FILE_HANDLE)NULL;
    IO_STATUS_BLOCK StatusBlock = {0};

    Filename.RootFileHandle = (IO_FILE_HANDLE)NULL;
    Filename.IoNameOptions = 0;
    ntError = RtlWC16StringAllocateFromCString(&Filename.FileName,
                                               pszFilename);
    BAIL_ON_NT_STATUS(ntError);

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
                           FILE_OPEN,
                           FILE_NON_DIRECTORY_FILE,
                           NULL,
                           0,
                           NULL);
    BAIL_ON_NT_STATUS(ntError);

    ntError = NtQueryInformationFile(hFile,
                                     NULL,
                                     &StatusBlock,
                                     &FileBasicInfo,
                                     sizeof(FileBasicInfo),
                                     FileBasicInformation);
    BAIL_ON_NT_STATUS(ntError);

    ntError = NtQueryInformationFile(hFile,
                                     NULL,
                                     &StatusBlock,
                                     &FileStdInfo,
                                     sizeof(FileStdInfo),
                                     FileStandardInformation);
    BAIL_ON_NT_STATUS(ntError);

    ntError = NtCloseFile(hFile);
    BAIL_ON_NT_STATUS(ntError);

    printf("Filename:             %s\n", pszFilename);
    printf("Allocation Size:      %lld\n", FileStdInfo.AllocationSize);
    printf("File Size:            %lld\n", FileStdInfo.EndOfFile);
    printf("Number of Links:      %d\n", FileStdInfo.NumberOfLinks);
    printf("Is Directory:         %s\n", FileStdInfo.Directory ? "yes" : "no");
    printf("Pending Delete:       %s\n", FileStdInfo.DeletePending ? "yes" : "no");
    printf("Attributes:           0x%x\n", FileBasicInfo.FileAttributes);
    printf("CreationTime:         %lld\n", FileBasicInfo.CreationTime);
    printf("Last Access Time:     %lld\n", FileBasicInfo.LastAccessTime);
    printf("Last Modification:    %lld\n", FileBasicInfo.LastWriteTime);
    printf("Change Time:          %lld\n", FileBasicInfo.ChangeTime);
    printf("\n");



cleanup:
    return ntError;

error:
    goto cleanup;
}

/******************************************************
 *****************************************************/

int
main(
    int argc,
    char *argv[]
)
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;

    /* Check Arg count */

    if (argc <= 2) {
        PrintUsage(argv[0]);
        ntError = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntError);
    }

    /* Process Args */

    if (strcmp(argv[1], "-c") == 0)
    {
        ntError = CopyFileToPvfs(argc-2, argv+2);
    }
    else if (strcmp(argv[1], "-C") == 0)
    {
        ntError = CopyFileFromPvfs(argc-2, argv+2);
    }
    else if (strcmp(argv[1], "-S") == 0)
    {
        ntError = StatRemoteFile(argv[2]);
    }
    else
    {
        PrintUsage(argv[0]);
        ntError = STATUS_INVALID_PARAMETER;
    }
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
