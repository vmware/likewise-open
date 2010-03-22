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
 *        share_p.c
 *
 * Abstract:
 *
 *        Likewise System NET Utilities
 *
 *        Share Helper Routines
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Wei Fu (wfu@likewise.com)
 */

#include "includes.h"

NET_SHARE_STATE gState = {0};

DWORD
LwUtilNetShareEnum(
    VOID
    )
{
    static const DWORD dwLevel = 2;
    static const DWORD dwMaxLen = 128;

    DWORD dwError = 0;
    PSHARE_INFO_2 pShareInfo = NULL;
    DWORD dwNumShares = 0;
    DWORD dwTotalShares = 0;
    DWORD dwVisitedShares = 0;
    DWORD dwResume = 0;
    DWORD dwIndex = 0;

    PSTR* ppszShareName = NULL;
    PSTR* ppszSharePath = NULL;
    PSTR* ppszShareComment = NULL;
    DWORD dwShareNameLenMax = 0;
    DWORD dwSharePathLenMax = 0;
    DWORD dwShareCommentLenMax = 0;
    DWORD dwShareNameLen = 0;
    DWORD dwSharePathLen = 0;
    DWORD dwShareCommentLen = 0;


    do
    {
        dwError = NetShareEnumW(
            gState.pwszServerName,
            dwLevel,
            (PBYTE*)&pShareInfo,
            dwMaxLen,
            &dwNumShares,
            &dwTotalShares,
            &dwResume);
        BAIL_ON_LWUTIL_ERROR(dwError);

        if (!ppszShareName)
        {
            dwError = LwAllocateMemory((dwTotalShares+1)*sizeof(PCSTR), (PVOID *)&ppszShareName);
            BAIL_ON_LWUTIL_ERROR(dwError);
        }

        if (!ppszSharePath)
        {
            dwError = LwAllocateMemory((dwTotalShares+1)*sizeof(PCSTR), (PVOID *)&ppszSharePath);
            BAIL_ON_LWUTIL_ERROR(dwError);
        }

        if (!ppszShareComment)
        {
        	dwError = LwAllocateMemory((dwTotalShares+1)*sizeof(PCSTR), (PVOID *)&ppszShareComment);
            BAIL_ON_LWUTIL_ERROR(dwError);
        }

        for (dwIndex = 0; dwIndex < dwNumShares; dwIndex++)
        {
            dwError = LwWc16sToMbs(pShareInfo[dwIndex].shi2_netname,
            		               &ppszShareName[dwIndex+dwVisitedShares]);
            BAIL_ON_LWUTIL_ERROR(dwError);

            dwError = LwWc16sToMbs(pShareInfo[dwIndex].shi2_path,
            		               &ppszSharePath[dwIndex+dwVisitedShares]);
            BAIL_ON_LWUTIL_ERROR(dwError);

            if (pShareInfo[dwIndex].shi2_remark)
            {
                dwError = LwWc16sToMbs(pShareInfo[dwIndex].shi2_remark,
                		               &ppszShareComment[dwIndex+dwVisitedShares]);
                BAIL_ON_LWUTIL_ERROR(dwError);
            }
        }

        if (pShareInfo)
        {
            NetApiBufferFree(pShareInfo);
            pShareInfo = NULL;
        }

        dwVisitedShares += dwNumShares;

    } while (dwVisitedShares < dwTotalShares);

    dwError = LwAllocateString(NET_SHARE_NAME_TITLE, &ppszShareName[dwTotalShares]);
    BAIL_ON_LWUTIL_ERROR(dwError);

    dwError = LwAllocateString(NET_SHARE_PATH_TITLE, &ppszSharePath[dwTotalShares]);
    BAIL_ON_LWUTIL_ERROR(dwError);

    dwError = LwAllocateString(NET_SHARE_COMMENT_TITLE, &ppszShareComment[dwTotalShares]);
    BAIL_ON_LWUTIL_ERROR(dwError);

    for (dwIndex = 0; dwIndex < dwTotalShares + 1; dwIndex++)
    {
        dwShareNameLen = strlen(ppszShareName[dwIndex]);
        if (dwShareNameLen>dwShareNameLenMax)
        {
        	dwShareNameLenMax = dwShareNameLen;
        }

        dwSharePathLen = strlen(ppszSharePath[dwIndex]);
        if (dwSharePathLen>dwSharePathLenMax)
        {
        	dwSharePathLenMax = dwSharePathLen;
        }

        if (ppszShareComment[dwIndex])
        {
            dwShareCommentLen = strlen(ppszShareComment[dwIndex]);
            if (dwShareCommentLen>dwShareCommentLenMax)
            {
            	dwShareCommentLenMax = dwShareCommentLen;
            }
        }
    }

    //print share enum header

    printf("  %s%*s",
    		NET_SHARE_NAME_TITLE,
           (int) (strlen(NET_SHARE_NAME_TITLE)-dwShareNameLenMax),
           "");
    printf("  %s%*s",
    		NET_SHARE_PATH_TITLE,
           (int) (strlen(NET_SHARE_PATH_TITLE)-dwSharePathLenMax),
           "");
    printf("  %s%*s\n",
    		NET_SHARE_COMMENT_TITLE,
           (int) (strlen(NET_SHARE_COMMENT_TITLE)-dwShareCommentLenMax),
           "");

    for (dwIndex = 0; dwIndex < dwShareNameLenMax+dwSharePathLenMax+dwShareCommentLenMax+10; dwIndex++)
    	printf("%s", "-");

    printf("\n");


    for (dwIndex = 0; dwIndex < dwTotalShares; dwIndex++)
    {
        printf("  %s%*s",
        		ppszShareName[dwIndex],
               (int) (strlen(ppszShareName[dwIndex])-dwShareNameLenMax),
               "");

        printf("  %s%*s",
        		ppszSharePath[dwIndex],
               (int) (strlen(ppszSharePath[dwIndex])-dwSharePathLenMax),
               "");

        if (ppszShareComment[dwIndex])
        {
        	printf("  %s%*s",
        		ppszShareComment[dwIndex],
               (int) (strlen(ppszShareComment[dwIndex])-dwShareCommentLenMax),
               "");
        }

        printf("\n");
    }

cleanup:

    if (ppszShareName)
    {
    	LwFreeStringArray(ppszShareName, dwTotalShares);
    }
    if (ppszSharePath)
    {
    	LwFreeStringArray(ppszSharePath, dwTotalShares);
    }
    if (ppszShareComment)
    {
    	LwFreeStringArray(ppszShareComment, dwTotalShares);
    }

    if (pShareInfo)
    {
    	NetApiBufferFree(pShareInfo);
        pShareInfo = NULL;
    }

    return dwError;

error:

    goto cleanup;
}


DWORD
LwUtilNetShareAdd(
	NET_SHARE_ADD_INFO_PARAMS ShareAddInfo
    )
{
    return ERROR_CALL_NOT_IMPLEMENTED;
}

