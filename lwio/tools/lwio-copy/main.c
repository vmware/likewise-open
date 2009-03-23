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
 *        Tool to copy files/directories
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#define _POSIX_PTHREAD_SEMANTICS 1

#include "config.h"
#include "lwiosys.h"
#include "lwio/lwio.h"
#include "lwiodef.h"
#include "lwioutils.h"
#include "lwio/ntfileapi.h"

DWORD
ParseArgs(
    int    argc,
    char*  argv[],
    PSTR*  ppszSourcePath,
    PSTR*  ppszTargetPath
    );

VOID
ShowUsage();

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
    DWORD dwErrorBufferSize = 0;
    BOOLEAN bPrintOrigError = TRUE;
    PSTR pszSourcePath = NULL;
    PSTR pszTargetPath = NULL;

    dwError = ParseArgs(
                argc,
                argv,
                &pszSourcePath,
                &pszTargetPath);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = LwIoInitialize();
    BAIL_ON_SMB_ERROR(dwError);

    // TODO:
    // Call CreateFile to open the file
    // Call ReadFile to read contents from the file
    // Write to local file which is the target path
cleanup:

    LwIoShutdown();

    if (pszSourcePath)
    {
        SMBFreeString(pszSourcePath);
    }
    if (pszTargetPath)
    {
        SMBFreeString(pszTargetPath);
    }

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
                fprintf(stderr, "Failed to query status from SMB service.  %s\n", pszErrorBuffer);
                bPrintOrigError = FALSE;
            }
        }

        SMB_SAFE_FREE_STRING(pszErrorBuffer);
    }

    if (bPrintOrigError)
    {
        fprintf(stderr, "Failed to query status from SMB service. Error code [%d]\n", dwError);
    }

    goto cleanup;
}

DWORD
ParseArgs(
    int    argc,
    char*  argv[],
    PSTR*  ppszSourcePath,
    PSTR*  ppszTargetPath
    )
{
    typedef enum {
        PARSE_MODE_OPEN = 0
    } ParseMode;

    DWORD dwError = 0;
    int iArg = 1;
    PSTR pszArg = NULL;
    ParseMode parseMode = PARSE_MODE_OPEN;
    PSTR pszSourcePath = NULL;
    PSTR pszTargetPath = NULL;

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

                if (!pszSourcePath)
                {
                    dwError = SMBAllocateString(
                                pszArg,
                                &pszSourcePath);
                    BAIL_ON_SMB_ERROR(dwError);
                }
                else if (!pszTargetPath)
                {
                    dwError = SMBAllocateString(
                                pszArg,
                                &pszTargetPath);
                    BAIL_ON_SMB_ERROR(dwError);
                }
                else
                {
                    ShowUsage();
                    exit(1);
                }

                break;
        }

    } while (iArg < argc);

    *ppszSourcePath = pszSourcePath;
    *ppszTargetPath = pszTargetPath;

cleanup:

    return dwError;

error:

    *ppszSourcePath = NULL;
    *ppszTargetPath = NULL;

    if (pszSourcePath)
    {
        SMBFreeString(pszSourcePath);
    }
    if (pszTargetPath)
    {
        SMBFreeString(pszTargetPath);
    }

    goto cleanup;
}

void
ShowUsage()
{
    printf("Usage: lwio-copy <source path> <target path>\n");
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
