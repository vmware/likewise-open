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
 *        Likewise Server Message Block (LSMB)
 *
 *        Tool to get the LSMB log information
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#define _POSIX_PTHREAD_SEMANTICS 1

#include "config.h"
#include "lsmbsys.h"
#include "lwrdr/lwrdr.h"
#include "smbdef.h"
#include "smbutils.h"

DWORD
ParseArgs(
    int    argc,
    char*  argv[]
    );

VOID
ShowUsage();

DWORD
PrintLogInfo(
    PSMB_LOG_INFO pLogInfo
    );

DWORD
MapErrorCode(
    DWORD dwError
    );

int
main(
    int argc,
    char* argv[]
    )
{
    DWORD dwError = 0;
    PSMB_LOG_INFO pLogInfo = NULL;
    HANDLE hConnection = (HANDLE)NULL;
    size_t dwErrorBufferSize = 0;
    BOOLEAN bPrintOrigError = TRUE;

    dwError = ParseArgs(argc, argv);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBInitialize();
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBOpenServer(&hConnection);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBGetLogInfo(
                    hConnection,
                    &pLogInfo);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = PrintLogInfo(pLogInfo);
    BAIL_ON_SMB_ERROR(dwError);

cleanup:

    if (pLogInfo) {
       SMBFreeLogInfo(pLogInfo);
    }

    if (hConnection != (HANDLE)NULL) {
        SMBCloseServer(hConnection);
    }

    SMBShutdown();

    return (dwError);

error:

    dwError = MapErrorCode(dwError);

    dwErrorBufferSize = SMBStrError(dwError, NULL, 0);

    if (dwErrorBufferSize > 0)
    {
        DWORD dwError2 = 0;
        PSTR   pszErrorBuffer = NULL;

        dwError2 = SMBAllocateMemory(
                    dwErrorBufferSize,
                    (PVOID*)&pszErrorBuffer);

        if (!dwError2)
        {
            DWORD dwLen = SMBStrError(dwError, pszErrorBuffer, dwErrorBufferSize);

            if ((dwLen == dwErrorBufferSize) && !IsNullOrEmptyString(pszErrorBuffer))
            {
                fprintf(stderr, "Failed to get LSMB log setting information.  %s\n", pszErrorBuffer);
                bPrintOrigError = FALSE;
            }
        }

        SMB_SAFE_FREE_STRING(pszErrorBuffer);
    }

    if (bPrintOrigError)
    {
        fprintf(stderr, "Failed to get LSMB log setting information. Error code [%d]\n", dwError);
    }

    goto cleanup;
}

DWORD
ParseArgs(
    int    argc,
    char*  argv[]
    )
{
    typedef enum {
            PARSE_MODE_OPEN = 0
        } ParseMode;

    DWORD dwError = 0;
    int iArg = 1;
    PSTR pszArg = NULL;
    ParseMode parseMode = PARSE_MODE_OPEN;

    do {
        pszArg = argv[iArg++];
        if (pszArg == NULL || *pszArg == '\0')
        {
            break;
        }

        switch (parseMode)
        {
            case PARSE_MODE_OPEN:

                if ((strcmp(pszArg, "--help") == 0) ||
                    (strcmp(pszArg, "-h") == 0))
                {
                    ShowUsage();
                    exit(0);
                }
                else
                {
                    ShowUsage();
                    exit(1);
                }
                break;
        }

    } while (iArg < argc);

    return dwError;
}

void
ShowUsage()
{
    printf("Usage: lw-smb-get-log-info\n");
}

DWORD
PrintLogInfo(
    PSMB_LOG_INFO pLogInfo
    )
{
    DWORD dwError = 0;

    fprintf(stdout, "Current log settings:\n");
    fprintf(stdout, "=================\n");
    switch(pLogInfo->logTarget)
    {
        case SMB_LOG_TARGET_DISABLED:
            fprintf(stdout, "Logging is currently disabled\n");
            break;
        case SMB_LOG_TARGET_CONSOLE:
            fprintf(stdout, "SMB Server is logging to console\n");
            break;
        case SMB_LOG_TARGET_FILE:
            fprintf(stdout, "SMB Server is logging to file.\n");
            fprintf(stdout, "Log file path: %s\n", pLogInfo->pszPath);
            break;
        case SMB_LOG_TARGET_SYSLOG:
            fprintf(stdout, "SMB Server is logging to syslog\n");
            break;
        default:
            dwError = EINVAL;
            BAIL_ON_SMB_ERROR(dwError);
    }

    fprintf(stdout, "Maximum allowed log level: ");
    switch(pLogInfo->maxAllowedLogLevel)
    {
        case SMB_LOG_LEVEL_ERROR:
            fprintf(stdout, "%s\n", "error");
            break;
        case SMB_LOG_LEVEL_WARNING:
            fprintf(stdout, "%s\n", "warning");
            break;
        case SMB_LOG_LEVEL_INFO:
            fprintf(stdout, "%s\n", "info");
            break;
        case SMB_LOG_LEVEL_VERBOSE:
            fprintf(stdout, "%s\n", "verbose");
            break;
        case SMB_LOG_LEVEL_DEBUG:
            fprintf(stdout, "%s\n", "debug");
            break;
        default:
            dwError = EINVAL;
            BAIL_ON_SMB_ERROR(dwError);
    }

error:

    return dwError;
}

DWORD
MapErrorCode(
    DWORD dwError
    )
{
    DWORD dwError2 = dwError;

    switch (dwError)
    {
        case ECONNREFUSED:
        case ENETUNREACH:
        case ETIMEDOUT:

            dwError2 = SMB_ERROR_SERVER_UNREACHABLE;

            break;

        default:

            break;
    }

    return dwError2;
}
