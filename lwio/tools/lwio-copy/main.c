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

#include "includes.h"

#define ACTION_NONE 0
#define ACTION_FILE 1
#define ACTION_DIR 2

#define NONE_NT    0
#define COPY_FROM_NT 1
#define COPY_TO_NT   2

#define MAX_ARGS	7

NTSTATUS
ParseArgs(
    int    argc,
    char*  argv[],
    PDWORD pdwFileOrDir,
    PDWORD pdwToRFromNt,
    PSTR*  ppszCachePath,
    PSTR*  ppszSourcePath,
    PSTR*  ppszTargetPath
    );


VOID
ShowUsage();

DWORD
MapErrorCode(
    DWORD status
    );

BOOLEAN
Krb5TicketHasExpired(
    VOID
    );

NTSTATUS
GetSystemName(
    PSTR* ppszHostName
    );

NTSTATUS
GetDirectionToCopy(
    PSTR pszSrcPath,
    PDWORD pdwToRFromNt
    );

NTSTATUS
GetCurrentDomain(
    PSTR* ppszDomain
    );

int
main(
    int argc,
    char* argv[]
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN bPrintOrigError = TRUE;
    DWORD dwErrorBufferSize = 0;
    PSTR pszSourcePath = NULL;
    PSTR pszTargetPath = NULL;
    PSTR pszCachePath = NULL;
    PSTR pszEnvString = NULL;

    DWORD dwFileOrDir = ACTION_NONE;
    DWORD dwToRFromNt = NONE_NT;

    status = ParseArgs(
                argc,
                argv,
                &dwFileOrDir,
                &dwToRFromNt,
                &pszCachePath,
                &pszSourcePath,
                &pszTargetPath);
    BAIL_ON_NT_STATUS(status);

    if(pszCachePath)
    {
        status = LwRtlCStringAllocatePrintf(
                    &pszEnvString,
                    "KRB5CCNAME=%s",
                    pszCachePath);
        BAIL_ON_NT_STATUS(status);

        if (putenv(pszEnvString) < 0)
        {
            status = UnixErrnoToNtStatus(errno);
        }
        BAIL_ON_NT_STATUS(status);

        // The string is owned by the environ variable, and cannot be
        // deleted.
        pszEnvString = NULL;
    }

    if(dwToRFromNt == COPY_FROM_NT)
    {
        if(dwFileOrDir == ACTION_FILE)
        {
            status = CopyFileFromNt(
                            pszSourcePath,
                            pszTargetPath);
            BAIL_ON_NT_STATUS(status);
        }
        else if(dwFileOrDir == ACTION_DIR)
        {
            status = CopyDirFromNt(
                            pszSourcePath,
                            pszTargetPath);
            BAIL_ON_NT_STATUS(status);
        }
    }
    else if(dwToRFromNt == COPY_TO_NT)
    {
        if(dwFileOrDir == ACTION_FILE)
        {
            status = CopyFileToNt(
                            pszSourcePath,
                            pszTargetPath);
            BAIL_ON_NT_STATUS(status);
        }
        else if(dwFileOrDir == ACTION_DIR)
        {
            status = CopyDirToNt(
                            pszSourcePath,
                            pszTargetPath);
            BAIL_ON_NT_STATUS(status);
        }
    }


cleanup:

    LWIO_SAFE_FREE_STRING(pszTargetPath);
    LWIO_SAFE_FREE_STRING(pszSourcePath);
    LWIO_SAFE_FREE_STRING(pszCachePath);
    LWIO_SAFE_FREE_STRING(pszEnvString);

    return (status);

error:

    status = MapErrorCode(status);

    dwErrorBufferSize = SMBStrError(status, NULL, 0);

    if (dwErrorBufferSize > 0)
    {
        DWORD dwError2 = 0;
        PSTR   pszErrorBuffer = NULL;

        dwError2 = SMBAllocateMemory(
                    dwErrorBufferSize,
                    (PVOID*)&pszErrorBuffer);

        if (!dwError2)
        {
            DWORD dwLen = SMBStrError(status, pszErrorBuffer, dwErrorBufferSize);

            if ((dwLen == dwErrorBufferSize) && !IsNullOrEmptyString(pszErrorBuffer))
            {
                fprintf(stderr, "Failed to query status from SMB service.  %s\n", pszErrorBuffer);
                bPrintOrigError = FALSE;
            }
        }

        LWIO_SAFE_FREE_STRING(pszErrorBuffer);
    }

    goto cleanup;
}


NTSTATUS
ParseArgs(
    int    argc,
    char*  argv[],
    PDWORD pdwFileOrDir,
    PDWORD pdwToRFromNt,
    PSTR*  ppszCachePath,
    PSTR*  ppszSourcePath,
    PSTR*  ppszTargetPath
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int iArg = 1;
    DWORD dwMaxIndex = argc - 1;
    PSTR pszArg = NULL;
    PSTR pszSourcePath = NULL;
    PSTR pszSrcPath = NULL;
    PSTR pszTargetPath = NULL;
    PSTR pszDestPath = NULL;
    PSTR pszCachePath = NULL;
    DWORD dwFileOrDir = ACTION_NONE;
    DWORD dwToRFromNt = NONE_NT;
    PSTR pszSlash = NULL;
    PSTR pszLast = NULL;

    if( dwMaxIndex > MAX_ARGS)
    {
        ShowUsage();
        exit(0);
    }

    pszArg = argv[iArg++];
    if (pszArg == NULL || *pszArg == '\0')
    {
        ShowUsage();
        exit(0);
    }
    else if ((strcmp(pszArg, "--help") == 0) || (strcmp(pszArg, "-h") == 0))
    {
        ShowUsage();
        exit(0);
    }
    else if ((strcmp(pszArg, "--file") == 0)|| (strcmp(pszArg, "-f") == 0))
    {
        dwFileOrDir = ACTION_FILE;
    }
    else if ((strcmp(pszArg, "--dir") == 0)|| (strcmp(pszArg, "-d") == 0))
    {
        dwFileOrDir = ACTION_DIR;
    }
    else
    {
        ShowUsage();
        exit(0);
    }

    pszArg = argv[iArg++];
    if (pszArg == NULL || *pszArg == '\0')
    {
        ShowUsage();
        exit(0);
    }
    else if ((strcmp(pszArg, "--krb") == 0) || (strcmp(pszArg, "-k") == 0))
    {
        status = SMBAllocateString(
                    argv[iArg++],
                    &pszCachePath);
        BAIL_ON_NT_STATUS(status);

        *ppszCachePath = pszCachePath;
    }
    else
    {
        iArg--;
    }

    pszArg = argv[iArg++];
    if (pszArg == NULL || *pszArg == '\0')
    {
        ShowUsage();
        exit(0);
    }
    else if ((strcmp(pszArg, "--src") == 0) || (strcmp(pszArg, "-s") == 0))
    {
        status = SMBAllocateString(
                    argv[iArg++],
                    &pszSourcePath);
        BAIL_ON_NT_STATUS(status);

        status = GetDirectionToCopy(
                    pszSourcePath,
                    &dwToRFromNt);
        BAIL_ON_NT_STATUS(status);
    }

    pszArg = argv[iArg++];
    if (pszArg == NULL || *pszArg == '\0')
    {
        ShowUsage();
        exit(0);
    }
    else if ((strcmp(pszArg, "--dest") == 0) || (strcmp(pszArg, "-d") == 0))
    {
        status = SMBAllocateString(
                    argv[iArg++],
                    &pszTargetPath);
        BAIL_ON_NT_STATUS(status);

    }

    //strip the hostname for the local host
    if(dwToRFromNt == COPY_FROM_NT)
    {
        pszSlash = (char*)strtok_r (pszTargetPath,"/",&pszLast);
        status = LwRtlCStringAllocatePrintf(
                    &pszDestPath,
                    "/%s",
                    pszLast);
        BAIL_ON_NT_STATUS(status);

        *ppszSourcePath = pszSourcePath;
        *ppszTargetPath = pszDestPath;

        LWIO_SAFE_FREE_STRING(pszTargetPath);
    }
    else
    {
        pszSlash = (char*)strtok_r (pszSourcePath,"/",&pszLast);
        status = LwRtlCStringAllocatePrintf(
                    &pszSrcPath,
                    "/%s",
                    pszLast);
        BAIL_ON_NT_STATUS(status);

        *ppszSourcePath = pszSrcPath;
        *ppszTargetPath = pszTargetPath;

        LWIO_SAFE_FREE_STRING(pszSourcePath);
    }

    *pdwFileOrDir = dwFileOrDir;
    *pdwToRFromNt = dwToRFromNt;

cleanup:

    return status;

error:

    *ppszCachePath = NULL;
    *ppszSourcePath = NULL;
    *ppszTargetPath = NULL;

    LWIO_SAFE_FREE_STRING(pszTargetPath);
    LWIO_SAFE_FREE_STRING(pszSourcePath);
    LWIO_SAFE_FREE_STRING(pszCachePath);
    LWIO_SAFE_FREE_STRING(pszSrcPath);
    LWIO_SAFE_FREE_STRING(pszDestPath);

    goto cleanup;
}

NTSTATUS
GetDirectionToCopy(
    PSTR pszSrcPath,
    PDWORD pdwToRFromNt
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PSTR pszSlash = NULL;
    PSTR pszLast = NULL;
    PSTR pszTmpPath = NULL;
    PSTR pszHostname = NULL;

    *pdwToRFromNt = NONE_NT;

    status = SMBAllocateString(
                    pszSrcPath,
                    &pszTmpPath);
    BAIL_ON_NT_STATUS(status);

    status = GetSystemName(&pszHostname);
    BAIL_ON_NT_STATUS(status);

    pszSlash = (char*)strtok_r (pszTmpPath,"/",&pszLast);

    if(pszSlash)
    {
        if(!strcmp(pszHostname,pszSlash))
            *pdwToRFromNt = COPY_TO_NT;
        else
            *pdwToRFromNt = COPY_FROM_NT;
    }


error:

    LWIO_SAFE_FREE_STRING(pszTmpPath);
    LWIO_SAFE_FREE_STRING(pszHostname);

    return status ;
}

NTSTATUS
GetSystemName(
    PSTR* ppszHostName
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    CHAR buffer[256];
    PSTR pszLocal = NULL;
    PSTR pszDot = NULL;
    PSTR pszHostname = NULL;
    int len = 0;

    if ( gethostname(buffer, sizeof(buffer)) != 0 )
    {
        printf("gethostname failed\n");
        status = LW_STATUS_UNSUCCESSFUL;
        goto error;
    }

    len = strlen(buffer);
    if ( len > strlen(".local") )
    {
        pszLocal = &buffer[len - strlen(".local")];
        if ( !strcasecmp( pszLocal, ".local" ) )
        {
            pszLocal[0] = '\0';
        }
    }

    pszDot = strchr(buffer, '.');
    if ( pszDot )
    {
        pszDot[0] = '\0';
    }

    status = LwRtlCStringAllocatePrintf(
                &pszHostname,
                "%s",
                buffer);
    BAIL_ON_NT_STATUS(status);

    *ppszHostName = pszHostname;

cleanup:

    return status;

error:

    LWIO_SAFE_FREE_STRING(pszHostname);

    goto cleanup;
}

void
ShowUsage()
{
    printf("Usage: lwio-copy --file|dir --setkrb <cache path> --src <source path> --dest <target path>\n");
    printf("\t--help    | -h    Show help\n");
    printf("\t--file 	| -f	Operate file\n");
    printf("\t--dir 	| -d    Operate directory\n");
    printf("\t--setkrb  | -k    Set KRB5CCNAME env \n");
    printf("\t--src 	| -s    Set source path \n");
    printf("\t--dest 	| -d    Set destination path \n");
    printf("<source path>/<dest path> should be of the form '/<hostname>/<Sharename>/<path>'\n");
}

DWORD
MapErrorCode(
    DWORD status
    )
{
    DWORD dwError2 = status;

    switch (status)
    {
        case ECONNREFUSED:
        case ENETUNREACH:
        case ETIMEDOUT:

            dwError2 = LWIO_ERROR_SERVER_UNREACHABLE;

            break;

        default:

            break;
    }

    return dwError2;
}

