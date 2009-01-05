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
 *        createnp.c
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem (SMB)
 *
 *        CreateNamedPipe API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "includes.h"

static
DWORD
ParseSharePath(
    LPCWSTR pwszPath,
    PSTR*   ppszServer,
    PSTR*   ppszShare,
    PSTR*   ppszFilename
    );

DWORD
RdrCreateFileEx(
    PSMB_SECURITY_TOKEN_REP pSecurityToken,
    LPCWSTR pwszFileName,
    DWORD   dwDesiredAccess,
    DWORD   dwSharedMode,
    DWORD   dwCreationDisposition,
    DWORD   dwFlagsAndAttributes,
    PSECURITY_ATTRIBUTES pSecurityAttributes,
    PHANDLE phFile
    )
{
    DWORD dwError = 0;
    PSTR   pszServer = NULL;
    PSTR   pszShare = NULL;
    PSTR   pszFilename = NULL;
    PWSTR  pwszFilename = NULL;
    PSMB_CLIENT_FILE_HANDLE pFile = NULL;

    if (!pSecurityToken ||
        pSecurityToken->type != SMB_SECURITY_TOKEN_TYPE_KRB5)
    {
        dwError = EACCES;
        BAIL_ON_SMB_ERROR(dwError);
    }

    dwError = SMBAllocateMemory(
                    sizeof(SMB_CLIENT_FILE_HANDLE),
                    (PVOID*)&pFile);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = ParseSharePath(
                    pwszFileName,
                    &pszServer,
                    &pszShare,
                    &pszFilename);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBWc16sToMbs(
                    pSecurityToken->payload.krb5.pwszPrincipal,
                    &pFile->pszPrincipal);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBWc16sToMbs(
                    pSecurityToken->payload.krb5.pwszCachePath,
                    &pFile->pszCachePath);
    BAIL_ON_SMB_ERROR(dwError);

    SMB_LOG_DEBUG("Principal [%s] Cache Path [%s]",
                  SMB_SAFE_LOG_STRING(pFile->pszPrincipal),
                  SMB_SAFE_LOG_STRING(pFile->pszCachePath));

    dwError = SMBKrb5SetDefaultCachePath(
                    pFile->pszCachePath,
                    NULL);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBSrvClientTreeOpen(
                    pszServer,
                    pFile->pszPrincipal,
                    pszShare,
                    &pFile->pTree);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBMbsToWc16s(
                    pszFilename,
                    &pwszFilename);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = NPOpen(
                    pFile->pTree,
                    pwszFilename,
                    dwDesiredAccess,
                    dwSharedMode,
                    dwCreationDisposition,
                    dwFlagsAndAttributes,
                    &pFile->fid);
    BAIL_ON_SMB_ERROR(dwError);

    *phFile = (HANDLE)pFile;

cleanup:

    SMB_SAFE_FREE_STRING(pszServer);
    SMB_SAFE_FREE_STRING(pszShare);
    SMB_SAFE_FREE_STRING(pszFilename);
    SMB_SAFE_FREE_MEMORY(pwszFilename);

    return dwError;

error:

    *phFile = NULL;

    goto cleanup;
}

static
DWORD
ParseSharePath(
    LPCWSTR pwszPath,
    PSTR*   ppszServer,
    PSTR*   ppszShare,
    PSTR*   ppszFilename
    )
{
    DWORD dwError = 0;
    PSTR  pszPath = NULL;
    PSTR  pszIndex = NULL;
    PSTR  pszServer = NULL;
    PSTR  pszShare  = NULL;
    PSTR  pszFilename = NULL;
    size_t sLen = 0;

    dwError = SMBWc16sToMbs(
                    pwszPath,
                    &pszPath);
    BAIL_ON_SMB_ERROR(dwError);

    SMBStripWhitespace(pszPath, TRUE, TRUE);

    pszIndex = pszPath;

    // Skip optional initial decoration
    if (!strncmp(pszIndex, "//", sizeof("//") - 1) ||
        !strncmp(pszIndex, "\\\\", sizeof("\\\\") - 1))
    {
        pszIndex += 2;
    }

    if (IsNullOrEmptyString(pszIndex) || !isalpha((int)*pszIndex))
    {
        dwError = SMB_ERROR_INVALID_PARAMETER;
        BAIL_ON_SMB_ERROR(dwError);
    }

    // Seek server name
    sLen = strcspn(pszIndex, "\\/");
    if (!sLen)
    {
        dwError = SMB_ERROR_INVALID_PARAMETER;
        BAIL_ON_SMB_ERROR(dwError);
    }

    dwError = SMBStrndup(
                    pszIndex,
                    sLen,
                    &pszServer);
    BAIL_ON_SMB_ERROR(dwError);

    pszIndex += sLen;

    // Skip delimiter
    sLen = strspn(pszIndex, "\\/");
    if (!sLen)
    {
        dwError = SMB_ERROR_INVALID_PARAMETER;
        BAIL_ON_SMB_ERROR(dwError);
    }

    pszIndex += sLen;

    // Seek share name
    sLen = strcspn(pszIndex, "\\/");
    if (!sLen)
    {
        dwError = SMB_ERROR_INVALID_PARAMETER;
        BAIL_ON_SMB_ERROR(dwError);
    }

    if (!strncasecmp(pszIndex, "pipe", sLen) ||
        !strncasecmp(pszIndex, "ipc$", sLen))
    {
        dwError = SMBAllocateStringPrintf(
                        &pszShare,
                        "\\\\%s\\IPC$",
                        pszServer);
        BAIL_ON_SMB_ERROR(dwError);
    }
    else
    {
        dwError = SMBAllocateMemory(
                        sizeof("\\\\") - 1 + strlen(pszServer) + sizeof("\\") - 1 + sLen + 1,
                        (PVOID*)&pszShare);
        BAIL_ON_SMB_ERROR(dwError);

        sprintf(pszShare, "\\\\%s\\", pszServer);
        strncat(pszShare, pszIndex, sLen);
    }

    pszIndex += sLen;

    // Skip delimiter
    sLen = strspn(pszIndex, "\\/");
    if (!sLen)
    {
        dwError = SMB_ERROR_INVALID_PARAMETER;
        BAIL_ON_SMB_ERROR(dwError);
    }

    pszIndex += sLen;

    // Seek file name
    sLen = strcspn(pszIndex, "\\/");
    if (!sLen)
    {
        dwError = SMB_ERROR_INVALID_PARAMETER;
        BAIL_ON_SMB_ERROR(dwError);
    }

    dwError = SMBAllocateMemory(
                    sLen + 2,
                    (PVOID*)&pszFilename);
    BAIL_ON_SMB_ERROR(dwError);

    snprintf(pszFilename, sLen + 2, "\\%s", pszIndex);

    pszIndex += sLen;

    if (!IsNullOrEmptyString(pszIndex))
    {
        dwError = SMB_ERROR_INVALID_PARAMETER;
        BAIL_ON_SMB_ERROR(dwError);
    }

    *ppszServer = pszServer;
    *ppszShare  = pszShare;
    *ppszFilename = pszFilename;

cleanup:

    SMB_SAFE_FREE_STRING(pszPath);

    return dwError;

error:

    SMB_SAFE_FREE_STRING(pszServer);
    SMB_SAFE_FREE_STRING(pszShare);
    SMB_SAFE_FREE_STRING(pszFilename);

    *ppszServer = NULL;
    *ppszShare = NULL;
    *ppszFilename = NULL;

    goto cleanup;
}

