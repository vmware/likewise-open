/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        netfile.c
 *
 * Abstract:
 *
 *        Likewise System NET Utilities
 *
 *        File Module
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Wei Fu (wfu@likewise.com)
 */

#include "includes.h"

VOID
NetFileShowUsage(
    VOID
    )
{
    printf(
        "Usage: lwnet file help\n"
        "       lwnet file [ <options> ... ]\n"
        "       lwnet file close [ <options> ... ] <id>\n");

    printf("\n"
           "Options:\n"
           "\n"
           "  --server <server>       Specify target server (default: local machine)\n"
           "\n");
}

static
VOID
NetFileFreeCommandInfo(
    PNET_FILE_COMMAND_INFO* ppCommandInfo
    );

static
DWORD
NetFileCloseParseArguments(
    int argc,
    char** argv,
    IN OUT PNET_FILE_COMMAND_INFO pCommandInfo
    )
{
    DWORD dwError = 0;
    int indexFileDelArg = 3;

    if (!argv[indexFileDelArg])
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LWUTIL_ERROR(dwError);
    }

    if (!strcasecmp(argv[indexFileDelArg], "--server"))
    {
        dwError = LwMbsToWc16s(argv[++indexFileDelArg], &pCommandInfo->FileCloseInfo.pwszServerName);
        BAIL_ON_LWUTIL_ERROR(dwError);

        indexFileDelArg++;
    }

    if (indexFileDelArg > argc-1)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LWUTIL_ERROR(dwError);
    }

    pCommandInfo->FileCloseInfo.dwFileId = atoi(argv[indexFileDelArg]);
    BAIL_ON_LWUTIL_ERROR(dwError);

cleanup:

    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pCommandInfo->FileCloseInfo.pwszServerName);

    goto cleanup;
}

static
DWORD
NetFileEnumParseArguments(
    int argc,
    char ** argv,
    IN OUT PNET_FILE_COMMAND_INFO pCommandInfo
    )
{
    DWORD dwError = 0;

    if (!argv[3])
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LWUTIL_ERROR(dwError);
    }

    dwError = LwMbsToWc16s(argv[3], &pCommandInfo->FileEnumInfo.pwszServerName);
    BAIL_ON_LWUTIL_ERROR(dwError);

cleanup:

    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pCommandInfo->FileEnumInfo.pwszServerName);

    goto cleanup;
}


static
DWORD
NetFileParseArguments(
    int argc,
    char ** argv,
    PNET_FILE_COMMAND_INFO* ppCommandInfo
    )
{
    DWORD dwError = 0;
    PNET_FILE_COMMAND_INFO pCommandInfo = NULL;

    if (argc < 2)
    {
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_LWUTIL_ERROR(dwError);
    }

    dwError = LwNetUtilAllocateMemory(sizeof(*pCommandInfo),
                                   (PVOID*)&pCommandInfo);
    BAIL_ON_LWUTIL_ERROR(dwError);


    if (!argv[2])
    {
        pCommandInfo->dwControlCode = NET_FILE_ENUM;
        goto cleanup;
    }

    if (!strcasecmp(argv[2], NET_FILE_COMMAND_HELP))
    {
        NetFileShowUsage();
        goto cleanup;
    }
    else if (!strcasecmp(argv[2], NET_FILE_COMMAND_CLOSE))
    {
        pCommandInfo->dwControlCode = NET_FILE_CLOSE;

        dwError = NetFileCloseParseArguments(argc, argv, pCommandInfo);
        BAIL_ON_LWUTIL_ERROR(dwError);
    }
    else if (!strcasecmp(argv[2], "--server"))
    {
        pCommandInfo->dwControlCode = NET_FILE_ENUM;

        dwError = NetFileEnumParseArguments(argc, argv, pCommandInfo);
        BAIL_ON_LWUTIL_ERROR(dwError);
    }
    else
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LWUTIL_ERROR(dwError);
    }

cleanup:

    *ppCommandInfo = pCommandInfo;

    return dwError;

error:
    if (LW_ERROR_INVALID_PARAMETER == dwError)
    {
        NetFileShowUsage();
    }

    LwNetUtilFreeMemory(pCommandInfo);
    pCommandInfo = NULL;

    goto cleanup;
}

DWORD
NetFileInitialize(
    VOID
    )
{
    return NetApiInitialize();
}

DWORD
NetFile(
    int argc,
    char ** argv
    )
{
    DWORD dwError = 0;
    PNET_FILE_COMMAND_INFO pCommandInfo = NULL;

    dwError = NetFileParseArguments(
                    argc,
                    argv,
                    &pCommandInfo
                    );
    BAIL_ON_LWUTIL_ERROR(dwError);

    dwError = NetFileInitialize();
    BAIL_ON_LWUTIL_ERROR(dwError);

    switch (pCommandInfo->dwControlCode)
    {
        case NET_FILE_CLOSE:
            // dwError = LwUtilNetFileClose(pCommandInfo->FileCloseInfo);
            // BAIL_ON_LWUTIL_ERROR(dwError);
            break;

        case NET_FILE_ENUM:

            // dwError = LwUtilNetFileEnum(pCommandInfo->FileEnumInfo);
            // BAIL_ON_LWUTIL_ERROR(dwError);
            break;

        default:
            break;
    }

cleanup:
    NetFileFreeCommandInfo(&pCommandInfo);

    return dwError;

error:
    goto cleanup;
}

DWORD
NetFileShutdown(
    VOID
    )
{
    return NetApiShutdown();
}

static
VOID
NetFileFreeCommandInfo(
    PNET_FILE_COMMAND_INFO* ppCommandInfo
    )
{
    PNET_FILE_COMMAND_INFO pCommandInfo = ppCommandInfo ? *ppCommandInfo : NULL;

    if (!pCommandInfo)
        return;

    switch (pCommandInfo->dwControlCode)
    {
        case NET_FILE_CLOSE:

            LW_SAFE_FREE_MEMORY(pCommandInfo->FileCloseInfo.pwszServerName);
            break;

        case NET_FILE_ENUM:

            LW_SAFE_FREE_MEMORY(pCommandInfo->FileEnumInfo.pwszServerName);

            break;

         default:
            break;
    }

    LW_SAFE_FREE_MEMORY(pCommandInfo);
    *ppCommandInfo = NULL;

    return;
}
