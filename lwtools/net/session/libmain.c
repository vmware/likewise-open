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
 *        netsession.c
 *
 * Abstract:
 *
 *        Likewise System NET Utilities
 *
 *        Session Module
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Wei Fu (wfu@likewise.com)
 */

#include "includes.h"

VOID
NetSessionShowUsage(
    VOID
    )
{
    printf(
        "Usage: lwnet session help\n"
        "       lwnet session [ <options> ... ]\n"
        "       lwnet session del [ <options> ... ] <name>\n");

    printf("\n"
           "Options:\n"
           "\n"
           "  --server <server>       Specify target server (default: local machine)\n"
           "\n");
}

static
VOID
NetSessionFreeCommandInfo(
    PNET_SESSION_COMMAND_INFO* ppCommandInfo
    );

static
DWORD
NetSessionDelParseArguments(
    int argc,
    char** argv,
    IN OUT PNET_SESSION_COMMAND_INFO pCommandInfo
    )
{
    DWORD dwError = 0;
    int indexSessionDelArg = 3;

    if (!argv[indexSessionDelArg])
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LWUTIL_ERROR(dwError);
    }

    if (!strcasecmp(argv[indexSessionDelArg], "--server"))
    {
        dwError = LwMbsToWc16s(argv[++indexSessionDelArg], &pCommandInfo->SessionDelInfo.pwszServerName);
        BAIL_ON_LWUTIL_ERROR(dwError);

        indexSessionDelArg++;
    }

    if (indexSessionDelArg > argc-1)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LWUTIL_ERROR(dwError);
    }

    dwError = LwMbsToWc16s(argv[indexSessionDelArg], &pCommandInfo->SessionDelInfo.pwszSessionName);
    BAIL_ON_LWUTIL_ERROR(dwError);

cleanup:

    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pCommandInfo->SessionDelInfo.pwszServerName);
    LW_SAFE_FREE_MEMORY(pCommandInfo->SessionDelInfo.pwszSessionName);

    goto cleanup;
}

static
DWORD
NetSessionEnumParseArguments(
    int argc,
    char ** argv,
    IN OUT PNET_SESSION_COMMAND_INFO pCommandInfo
    )
{
    DWORD dwError = 0;

    if (!argv[3])
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LWUTIL_ERROR(dwError);
    }

    dwError = LwMbsToWc16s(argv[3], &pCommandInfo->SessionEnumInfo.pwszServerName);
    BAIL_ON_LWUTIL_ERROR(dwError);

cleanup:

    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pCommandInfo->SessionEnumInfo.pwszServerName);

    goto cleanup;
}


static
DWORD
NetSessionParseArguments(
    int argc,
    char ** argv,
    PNET_SESSION_COMMAND_INFO* ppCommandInfo
    )
{
    DWORD dwError = 0;
    PNET_SESSION_COMMAND_INFO pCommandInfo = NULL;

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
        pCommandInfo->dwControlCode = NET_SESSION_ENUM;
        goto cleanup;
    }

    if (!strcasecmp(argv[2], NET_SESSION_COMMAND_HELP))
    {
        NetSessionShowUsage();
        goto cleanup;
    }
    else if (!strcasecmp(argv[2], NET_SESSION_COMMAND_DEL))
    {
        pCommandInfo->dwControlCode = NET_SESSION_DEL;

        dwError = NetSessionDelParseArguments(argc, argv, pCommandInfo);
        BAIL_ON_LWUTIL_ERROR(dwError);
    }
    else if (!strcasecmp(argv[2], "--server"))
    {
        pCommandInfo->dwControlCode = NET_SESSION_ENUM;

        dwError = NetSessionEnumParseArguments(argc, argv, pCommandInfo);
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
        NetSessionShowUsage();
    }

    LwNetUtilFreeMemory(pCommandInfo);
    pCommandInfo = NULL;

    goto cleanup;
}

DWORD
NetSessionInitialize(
    VOID
    )
{
    return NetApiInitialize();
}

DWORD
NetSession(
    int argc,
    char ** argv
    )
{
    DWORD dwError = 0;
    PNET_SESSION_COMMAND_INFO pCommandInfo = NULL;

    dwError = NetSessionParseArguments(
                    argc,
                    argv,
                    &pCommandInfo
                    );
    BAIL_ON_LWUTIL_ERROR(dwError);

    dwError = NetSessionInitialize();
    BAIL_ON_LWUTIL_ERROR(dwError);

    switch (pCommandInfo->dwControlCode)
    {
        case NET_SESSION_DEL:

            dwError = NetExecSessionDel(&pCommandInfo->SessionDelInfo);
            BAIL_ON_LWUTIL_ERROR(dwError);

            break;

        case NET_SESSION_ENUM:

            dwError = NetExecSessionEnum(&pCommandInfo->SessionEnumInfo);
            BAIL_ON_LWUTIL_ERROR(dwError);

            break;

        default:

            break;
    }

cleanup:

    NetSessionFreeCommandInfo(&pCommandInfo);

    return dwError;

error:
    goto cleanup;
}

DWORD
NetSessionShutdown(
    VOID
    )
{
    return NetApiShutdown();
}

static
VOID
NetSessionFreeCommandInfo(
    PNET_SESSION_COMMAND_INFO* ppCommandInfo
    )
{
    PNET_SESSION_COMMAND_INFO pCommandInfo = ppCommandInfo ? *ppCommandInfo : NULL;

    if (!pCommandInfo)
        return;

    switch (pCommandInfo->dwControlCode)
    {
        case NET_SESSION_DEL:

            LW_SAFE_FREE_MEMORY(pCommandInfo->SessionDelInfo.pwszServerName);
            LW_SAFE_FREE_MEMORY(pCommandInfo->SessionDelInfo.pwszSessionName);
            break;

        case NET_SESSION_ENUM:

            LW_SAFE_FREE_MEMORY(pCommandInfo->SessionEnumInfo.pwszServerName);

            break;

         default:
            break;
    }

    LW_SAFE_FREE_MEMORY(pCommandInfo);
    *ppCommandInfo = NULL;

    return;
}
