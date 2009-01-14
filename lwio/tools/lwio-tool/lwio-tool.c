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
 *        lwio-tool.c
 *
 * Abstract:
 *
 *        LW IO Tool
 *
 * Authors: Danilo Almeida (dalmeida@likewisesoftware.com)
 */

#define _POSIX_PTHREAD_SEMANTICS 1

#include "config.h"
#include "lwiosys.h"
#include <lwio/ntfileapi.h>
#include "lwiodef.h"
#include "lwioutils.h"
#include "lwparseargs.h"
#include <lw/rtlgoto.h>
#include "ntlogmacros.h"

static
NTSTATUS
DoTestFileApiCreateFile(
    IN PCSTR pszPath
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    IO_FILE_HANDLE fileHandle = NULL;
    IO_FILE_NAME fileName = { 0 };
    IO_STATUS_BLOCK ioStatusBlock = { 0 };
    ACCESS_MASK desiredAccess = FILE_GENERIC_READ;
    LONG64 allocationSize = 0;
    FILE_ATTRIBUTES fileAttributes = 0;
    FILE_SHARE_FLAGS shareAccess = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    FILE_CREATE_DISPOSITION createDisposition = FILE_OPEN;
    FILE_CREATE_OPTIONS createOptions = 0;
    PIO_EA_BUFFER pEaBuffer = NULL;
    PVOID pSecurityDescriptor = NULL;
    PVOID pSecurityQualityOfService = NULL;

    if (IsNullOrEmptyString(pszPath))
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_EE(EE);
    }

    status = RtlWC16StringAllocateFromCString(&fileName.FileName, pszPath);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = NtCreateFile(
                    &fileHandle,
                    NULL,
                    &ioStatusBlock,
                    &fileName,
                    desiredAccess,
                    allocationSize,
                    fileAttributes,
                    shareAccess,
                    createDisposition,
                    createOptions,
                    pEaBuffer,
                    pSecurityDescriptor,
                    pSecurityQualityOfService);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    SMB_LOG_ALWAYS("Opened file '%s'", pszPath);

cleanup:
    RtlWC16StringFree(&fileName.FileName);

    if (fileHandle)
    {
        NtCloseFile(fileHandle);
    }

    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return status;
}

static
NTSTATUS
DoTestFileApi(
    IN OUT PLW_PARSE_ARGS pParseArgs,
    OUT PSTR* ppszUsageError
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    PSTR pszUsageError = NULL;
    PCSTR pszCommand = NULL;
    PCSTR pszPath = NULL;

    pszCommand = LwParseArgsNext(pParseArgs);
    if (!pszCommand)
    {
        status = RtlCStringAllocateAppendPrintf(&pszUsageError, "Missing command.\n");
        assert(!status && pszUsageError);
        GOTO_CLEANUP_EE(EE);
    }

    if (!strcmp(pszCommand, "create"))
    {
        pszPath = LwParseArgsNext(pParseArgs);
        if (!pszPath)
        {
            status = RtlCStringAllocateAppendPrintf(&pszUsageError, "Missing path argument.\n");
            assert(!status && pszUsageError);
            GOTO_CLEANUP_EE(EE);
        }

        if (LwParseArgsGetRemaining(pParseArgs) > 1)
        {
            status = RtlCStringAllocateAppendPrintf(&pszUsageError, "Too many arguments.\n");
            assert(!status && pszUsageError);
            GOTO_CLEANUP_EE(EE);
        }

        status = DoTestFileApiCreateFile(pszPath);
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }
    else
    {
        status = RtlCStringAllocateAppendPrintf(&pszUsageError, "Invalid command '%s'\n", pszCommand);
        assert(!status);
        GOTO_CLEANUP_EE(EE);
    }

cleanup:
    if (pszUsageError)
    {
        status = STATUS_INVALID_PARAMETER;
    }

    *ppszUsageError = pszUsageError;

    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return status;
}

static
VOID
Usage(
    IN PCSTR pszProgramName
    )
{
    printf("Usage: %s <command> [command-args]\n"
           "\n"
           "  commands:\n"
           "\n"
           "    testfileapi create <path>\n"
           "\n",
           pszProgramName);
    // TODO--We really want something like:
    //
    // <TOOL> testfileapi createfile <path> [options]
    // <TOOL> load <drivername>
    // <TOOL> unload <drivername>
    // etc..
}

int
main(
    IN int argc,
    IN PCSTR argv[]
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    PSTR pszUsageError = NULL;
    LW_PARSE_ARGS args = { 0 };
    PCSTR pszProgramName = NULL;
    PCSTR pszCommand = NULL;

    LwParseArgsInit(&args, argc, argv);
    pszProgramName = LwGetProgramName(LwParseArgsGetAt(&args, 0));

    // TODO-clean up logging stuff used here
    // We should really be using printf and just doing logging
    // for diagnostics.
    if (SMBInitLogging(pszProgramName,
                       SMB_LOG_TARGET_CONSOLE,
                       SMB_LOG_LEVEL_DEBUG,
                       NULL))
    {
        fprintf(stderr, "Failed to initialize logging.\n");
        exit(1);
    }

    pszCommand = LwParseArgsNext(&args);
    if (!pszCommand)
    {
        status = RtlCStringAllocateAppendPrintf(&pszUsageError, "Missing command.\n");
        assert(!status && pszUsageError);
        GOTO_CLEANUP_EE(EE);
    }

    if (!strcmp(pszCommand, "testfileapi"))
    {
        status = DoTestFileApi(&args, &pszUsageError);
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }
    else
    {
        status = RtlCStringAllocateAppendPrintf(&pszUsageError, "Invalid command '%s'\n", pszCommand);
        assert(!status && pszUsageError);
        GOTO_CLEANUP_EE(EE);
    }

cleanup:
    if (pszUsageError)
    {
        printf("%s", pszUsageError);
        RtlCStringFree(&pszUsageError);
        Usage(pszProgramName);
        status = STATUS_INVALID_PARAMETER;
    }

    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return status ? 1 : 0;
}
