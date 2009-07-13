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
 *        diruser.c
 *
 * Abstract:
 *
 *
 *      Likewise Directory Wrapper Interface
 *
 *      User Password Change Routines
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *
 */
#include "includes.h"

DWORD
DirectorySetPassword(
    HANDLE hDirectory,
    PWSTR  pwszUserDN,
    PWSTR  pwszPassword
    )
{
    DWORD dwError = 0;
    PDIRECTORY_CONTEXT pContext = (PDIRECTORY_CONTEXT)hDirectory;

    if (!pContext || !pContext->pProvider)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_DIRECTORY_ERROR(dwError);
    }

    dwError = pContext->pProvider->pProviderFnTbl->pfnDirectorySetPassword(
                    pContext->hBindHandle,
                    pwszUserDN,
                    pwszPassword);

error:

    return dwError;
}

DWORD
DirectoryChangePassword(
    HANDLE hDirectory,
    PWSTR  pwszUserDN,
    PWSTR  pwszOldPassword,
    PWSTR  pwszNewPassword
    )
{
    DWORD dwError = 0;
    PDIRECTORY_CONTEXT pContext = (PDIRECTORY_CONTEXT)hDirectory;

    if (!pContext || !pContext->pProvider)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_DIRECTORY_ERROR(dwError);
    }

    dwError = pContext->pProvider->pProviderFnTbl->pfnDirectoryChangePassword(
                    pContext->hBindHandle,
                    pwszUserDN,
                    pwszOldPassword,
                    pwszNewPassword);

error:

    return dwError;
}

DWORD
DirectoryVerifyPassword(
    HANDLE hDirectory,
    PWSTR  pwszUserDN,
    PWSTR  pwszPassword
    )
{
    DWORD dwError = 0;
    PDIRECTORY_CONTEXT pContext = (PDIRECTORY_CONTEXT)hDirectory;

    if (!pContext || !pContext->pProvider)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_DIRECTORY_ERROR(dwError);
    }

    dwError = pContext->pProvider->pProviderFnTbl->pfnDirectoryVerifyPassword(
                    pContext->hBindHandle,
                    pwszUserDN,
                    pwszPassword);

error:

    return dwError;
}

DWORD
DirectoryGetGroupMembers(
    HANDLE            hDirectory,
    PWSTR             pwszGroupDN,
    PWSTR             pwszAttrs[],
    PDIRECTORY_ENTRY* ppDirectoryEntries,
    PDWORD            pdwNumEntries
    )
{
    DWORD dwError = 0;
    PDIRECTORY_CONTEXT pContext = (PDIRECTORY_CONTEXT)hDirectory;

    if (!pContext || !pContext->pProvider)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_DIRECTORY_ERROR(dwError);
    }

    dwError = pContext->pProvider->pProviderFnTbl->pfnDirectoryGetGroupMembers(
                    pContext->hBindHandle,
                    pwszGroupDN,
                    pwszAttrs,
                    ppDirectoryEntries,
                    pdwNumEntries);

error:

    return dwError;
}

DWORD
DirectoryGetMemberships(
    HANDLE            hDirectory,
    PWSTR             pwszUserDN,
    PWSTR             pwszAttrs[],
    PDIRECTORY_ENTRY* ppDirectoryEntries,
    PDWORD            pdwNumEntries
    )
{
    DWORD dwError = 0;
    PDIRECTORY_CONTEXT pContext = (PDIRECTORY_CONTEXT)hDirectory;

    if (!pContext || !pContext->pProvider)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_DIRECTORY_ERROR(dwError);
    }

    dwError = pContext->pProvider->pProviderFnTbl->pfnDirectoryGetMemberships(
                    pContext->hBindHandle,
                    pwszUserDN,
                    pwszAttrs,
                    ppDirectoryEntries,
                    pdwNumEntries);

error:

    return dwError;
}

DWORD
DirectoryAddToGroup(
    HANDLE            hDirectory,
    PWSTR             pwszGroupDN,
    PDIRECTORY_ENTRY  pDirectoryEntries
    )
{
    DWORD dwError = 0;
    PDIRECTORY_CONTEXT pContext = (PDIRECTORY_CONTEXT)hDirectory;

    if (!pContext || !pContext->pProvider)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_DIRECTORY_ERROR(dwError);
    }

    dwError = pContext->pProvider->pProviderFnTbl->pfnDirectoryAddToGroup(
                    pContext->hBindHandle,
                    pwszGroupDN,
                    pDirectoryEntries);

error:

    return dwError;
}

DWORD
DirectoryRemoveFromGroup(
    HANDLE            hDirectory,
    PWSTR             pwszGroupDN,
    PDIRECTORY_ENTRY  pDirectoryEntries
    )
{
    DWORD dwError = 0;
    PDIRECTORY_CONTEXT pContext = (PDIRECTORY_CONTEXT)hDirectory;

    if (!pContext || !pContext->pProvider)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_DIRECTORY_ERROR(dwError);
    }

    dwError = pContext->pProvider->pProviderFnTbl->pfnDirectoryRemoveFromGroup(
                    pContext->hBindHandle,
                    pwszGroupDN,
                    pDirectoryEntries);

error:

    return dwError;
}

DWORD
DirectoryGetUserCount(
    HANDLE hBindHandle,
    PDWORD pdwNumUsers
    )
{
    DWORD dwError = 0;
    PDIRECTORY_CONTEXT pContext = (PDIRECTORY_CONTEXT)hBindHandle;

    if (!pContext || !pContext->pProvider)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_DIRECTORY_ERROR(dwError);
    }

    dwError = pContext->pProvider->pProviderFnTbl->pfnDirectoryGetUserCount(
                    pContext->hBindHandle,
                    pdwNumUsers);

error:

    return dwError;
}
