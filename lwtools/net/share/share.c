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
 *        share.c
 *
 * Abstract:
 *
 *        Likewise System NET Utilities
 *
 *        Share Module
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Wei Fu (wfu@likewise.com)
 */

#include "includes.h"

static
VOID
NetShareShowUsage(
	VOID
	)
{
	printf("Show net share usage\n");
}

static
VOID
NetShareFreeCommandInfo(
	PNET_SHARE_COMMAND_INFO* ppCommandInfo
	);

static
DWORD
NetShareAddParseArguments(
	IN PCSTR pszShareAddArg,
	IN OUT PNET_SHARE_COMMAND_INFO pCommandInfo
	)
{
	DWORD dwError = 0;
	PCSTR pszPath =  NULL;
	size_t sShareNameLen = 0;

	pszPath = strchr(pszShareAddArg, '=');
	if (LW_IS_NULL_OR_EMPTY_STR(pszPath))
	{
		dwError = LW_ERROR_INVALID_PARAMETER;
		BAIL_ON_LWUTIL_ERROR(dwError);
	}

    dwError = LwAllocateString(pszPath+1, &pCommandInfo->ShareAddInfo.pszPath);
    BAIL_ON_LWUTIL_ERROR(dwError);

    sShareNameLen = strlen(pszShareAddArg)-strlen(pszPath);

    if (!sShareNameLen)
    {
		dwError = LW_ERROR_INVALID_PARAMETER;
		BAIL_ON_LWUTIL_ERROR(dwError);
    }

    dwError = LwAllocateMemory(sShareNameLen+1,
    		                  (PVOID*)&pCommandInfo->ShareAddInfo.pszShareName);
    BAIL_ON_LWUTIL_ERROR(dwError);

    memcpy(pCommandInfo->ShareAddInfo.pszShareName, pszShareAddArg, sShareNameLen);

cleanup:

    return dwError;

error:
    LW_SAFE_FREE_STRING(pCommandInfo->ShareAddInfo.pszShareName);
    LW_SAFE_FREE_STRING(pCommandInfo->ShareAddInfo.pszPath);

    goto cleanup;
}

static
DWORD
NetShareDelParseArguments(
	IN PCSTR pszShareDelArg,
	IN OUT PNET_SHARE_COMMAND_INFO pCommandInfo
	)
{
	DWORD dwError = 0;

    dwError = LwAllocateString(pszShareDelArg, &pCommandInfo->ShareDelInfo.pszShareName);
    BAIL_ON_LWUTIL_ERROR(dwError);

cleanup:

    return dwError;

error:
    LW_SAFE_FREE_STRING(pCommandInfo->ShareDelInfo.pszShareName);

    goto cleanup;
}


static
DWORD
NetShareParseArguments(
    int argc,
	char ** argv,
	PNET_SHARE_COMMAND_INFO* ppCommandInfo
	)
{
	DWORD dwError = 0;
	PNET_SHARE_COMMAND_INFO pCommandInfo = NULL;

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
    	pCommandInfo->dwControlCode = NET_SHARE_ENUM;
    	goto cleanup;
    }

    if (!strcasecmp(argv[2], NET_SHARE_COMMAND_ADD))
	{
		pCommandInfo->dwControlCode = NET_SHARE_ADD;

		if (!argv[3])
		{
			dwError = LW_ERROR_INVALID_PARAMETER;
			BAIL_ON_LWUTIL_ERROR(dwError);
		}

		dwError = NetShareAddParseArguments(argv[3], pCommandInfo);
		BAIL_ON_LWUTIL_ERROR(dwError);
	}
	else if (!strcasecmp(argv[2], NET_SHARE_COMMAND_DEL))
	{
		pCommandInfo->dwControlCode = NET_SHARE_DEL;

		if (!argv[3])
		{
			dwError = LW_ERROR_INVALID_PARAMETER;
			BAIL_ON_LWUTIL_ERROR(dwError);
		}

		dwError = NetShareDelParseArguments(argv[3], pCommandInfo);
		BAIL_ON_LWUTIL_ERROR(dwError);
	}

cleanup:

    *ppCommandInfo = pCommandInfo;

    return dwError;

error:
    if (LW_ERROR_INVALID_PARAMETER == dwError)
    {
    	NetShareShowUsage();
    }

    LwNetUtilFreeMemory(pCommandInfo);
    pCommandInfo = NULL;

    goto cleanup;
}

DWORD
NetShareInitialize(
    VOID
    )
{
    return NetApiInitialize();
}

DWORD
NetShare(
    int argc,
    char ** argv
    )
{
    DWORD dwError = 0;
    PNET_SHARE_COMMAND_INFO pCommandInfo = NULL;

    dwError = NetShareParseArguments(
                    argc,
                    argv,
                    &pCommandInfo
                    );
    BAIL_ON_LWUTIL_ERROR(dwError);

    dwError = NetShareInitialize();
    BAIL_ON_LWUTIL_ERROR(dwError);

    switch (pCommandInfo->dwControlCode)
    {

        case NET_SHARE_ADD:

        	dwError = LwUtilNetShareAdd(pCommandInfo->ShareAddInfo);
        	BAIL_ON_LWUTIL_ERROR(dwError);
        	break;


        case NET_SHARE_DEL:
            dwError = LwUtilNetShareDel(pCommandInfo->ShareDelInfo);
		BAIL_ON_LWUTIL_ERROR(dwError);
        	break;

        case NET_SHARE_ENUM:

            dwError = LwUtilNetShareEnum();
        	BAIL_ON_LWUTIL_ERROR(dwError);
        	break;

        default:
        	break;
    }

cleanup:
    NetShareFreeCommandInfo(&pCommandInfo);

    return dwError;

error:
    goto cleanup;
}

DWORD
NetShareShutdown(
    VOID
    )
{
    return NetApiShutdown();
}

static
VOID
NetShareFreeCommandInfo(
	PNET_SHARE_COMMAND_INFO* ppCommandInfo
	)
{
	PNET_SHARE_COMMAND_INFO pCommandInfo = ppCommandInfo ? *ppCommandInfo : NULL;

	if (!pCommandInfo)
		return;

	switch (pCommandInfo->dwControlCode)
	{
	    case NET_SHARE_ADD:

	        LW_SAFE_FREE_STRING(pCommandInfo->ShareAddInfo.pszPath);
	        LW_SAFE_FREE_STRING(pCommandInfo->ShareAddInfo.pszShareName);
	        break;

	    case NET_SHARE_DEL:

	    	LW_SAFE_FREE_STRING(pCommandInfo->ShareDelInfo.pszShareName);
	        break;

	    case NET_SHARE_ENUM:

	        break;

	     default:
	        break;
	}

	LW_SAFE_FREE_MEMORY(pCommandInfo);
	*ppCommandInfo = NULL;

	return;
}
