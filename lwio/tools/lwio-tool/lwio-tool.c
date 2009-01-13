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
#include "goto.h"
#include "ntlogmacros.h"

// TODO -- common lib
static
PCSTR
LwGetProgramName(
    IN PCSTR pszProgramPath
    )
{
    PCSTR pszProgramName = pszProgramPath;
    PCSTR pszCurrent = pszProgramName;

    while (pszCurrent[0])
    {
        if ('/' == pszCurrent[0])
        {
            pszProgramName = pszCurrent + 1;
        }
        pszCurrent++;
    }

    return pszProgramName;
}

typedef struct _LW_PARSE_ARGS {
    PCSTR* Args;
    int Count;
    int Index;
} LW_PARSE_ARGS, *PLW_PARSE_ARGS;

static
VOID
LwParseArgsInit(
    OUT PLW_PARSE_ARGS pParseArgs,
    IN int argc,
    IN PCSTR argv[]
    )
{
    pParseArgs->Args = argv;
    pParseArgs->Count = argc;
    pParseArgs->Index = 0;
}

int
LwParseArgsGetRemaining(
    IN PLW_PARSE_ARGS pParseArgs
    )
{
    return pParseArgs->Count - pParseArgs->Index;
}

static
PCSTR
LwParseArgsGetAt(
    IN PLW_PARSE_ARGS pParseArgs,
    IN int Index
    )
{
    return (Index < pParseArgs->Count) ? pParseArgs->Args[Index] : NULL;
}

int
LwParseArgsGetIndex(
    IN PLW_PARSE_ARGS pParseArgs
    )
{
    return pParseArgs->Index;
}

PCSTR
LwParseArgsGetCurrent(
    IN OUT PLW_PARSE_ARGS pParseArgs
    )
{
    return LwParseArgsGetAt(pParseArgs, pParseArgs->Index);
}

static
PCSTR
LwParseArgsNext(
    IN OUT PLW_PARSE_ARGS pParseArgs
    )
{
    PCSTR pszNext = LwParseArgsGetAt(pParseArgs, pParseArgs->Index + 1);
    if (pszNext)
    {
        pParseArgs->Index++;
    }
    return pszNext;
}

static
NTSTATUS
TestCreateFile(
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

    // TODO -- Rtl-type stuff for strings...
    fileName.FileName = ambstowc16s(pszPath);
    if (!fileName.FileName)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        GOTO_CLEANUP_EE(EE);
    }

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

    status = 2;

cleanup:
    if (fileHandle)
    {
        NtCloseFile(fileHandle);
    }

    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return status;
}

static
VOID
Usage(
    IN PCSTR pszProgramName
    )
{
    printf("Usage: %s <path>\n",
           pszProgramName);
    // TODO--We really want something like:
    //
    // <TOOL> testapi createfile <path> [options]
    // <TOOL> load <drivername>
    // <TOOL> unload <drivername>
    // etc..
}

#if 0
NTSTATUS
LwCStringAppendV(
    IN OUT PSTR* pszUsageError,
    IN PCSTR pszFormat,
    IN va_list Args
    )
{
}

NTSTATUS
LwCStringAppend(
    IN OUT PSTR* pszUsageError,
    IN PCSTR pszFormat,
    ...
    )
{
}
#endif

int
main(
    IN int argc,
    IN PCSTR argv[]
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    PSTR pszUsageError = NULL;
    PCSTR pszProgramName = LwGetProgramName(argv[0]);
    PCSTR pszPath = argv[1];
    LW_PARSE_ARGS args = { 0 };
    PCSTR pszCommand = NULL;
    BOOLEAN bShowUsage = FALSE;

    LwParseArgsInit(&args, argc, argv);
    pszProgramName = LwGetProgramName(LwParseArgsNext(&args));

    // TODO-clean up logging stuff used here
    if (SMBInitLogging(pszProgramName,
                       SMB_LOG_TARGET_CONSOLE,
                       SMB_LOG_LEVEL_DEBUG,
                       NULL))
    {
        fprintf(stderr, "Cannot log\n");
        exit(1);
    }

    pszCommand = LwParseArgsNext(&args);
    if (!pszCommand)
    {
        asprintf(&pszUsageError, "Missing command.\n");
        assert(pszUsageError);
        GOTO_CLEANUP_EE(EE);
    }

    // TODO -- add arg parsing (need to argc/argv walker to common lib)

    if (!pszPath || !pszPath[0])
    {
        bShowUsage = TRUE;
        GOTO_CLEANUP_EE(EE);
    }

    status = TestCreateFile(pszPath);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

cleanup:
    if (bShowUsage)
    {
        Usage(pszProgramName);
        status = STATUS_INVALID_PARAMETER;
    }

    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return status ? 1 : 0;
}
