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
#include "rdr.h"

static
NTSTATUS
ParseSharePath(
    PCWSTR pwszPath,
    PSTR*   ppszServer,
    PSTR*   ppszShare,
    PSTR*   ppszFilename
    );

NTSTATUS
RdrCreateFileEx(
    PIO_ACCESS_TOKEN pSecurityToken,
    PCWSTR pwszFileName,
    DWORD   dwDesiredAccess,
    DWORD   dwSharedMode,
    DWORD   dwCreationDisposition,
    DWORD   dwFlagsAndAttributes,
    PSECURITY_ATTRIBUTES pSecurityAttributes,
    PHANDLE phFile
    )
{
    NTSTATUS ntStatus = 0;
    PSTR   pszServer = NULL;
    PSTR   pszShare = NULL;
    PSTR   pszFilename = NULL;
    PWSTR  pwszFilename = NULL;
    PSMB_CLIENT_FILE_HANDLE pFile = NULL;

    if (!pSecurityToken ||
        pSecurityToken->type != IO_ACCESS_TOKEN_TYPE_KRB5)
    {
        ntStatus = EACCES;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SMBAllocateMemory(
                    sizeof(SMB_CLIENT_FILE_HANDLE),
                    (PVOID*)&pFile);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = ParseSharePath(
                    pwszFileName,
                    &pszServer,
                    &pszShare,
                    &pszFilename);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBWc16sToMbs(
                    pSecurityToken->payload.krb5.pwszPrincipal,
                    &pFile->pszPrincipal);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBWc16sToMbs(
                    pSecurityToken->payload.krb5.pwszCachePath,
                    &pFile->pszCachePath);
    BAIL_ON_NT_STATUS(ntStatus);

    SMB_LOG_DEBUG("Principal [%s] Cache Path [%s]",
                  SMB_SAFE_LOG_STRING(pFile->pszPrincipal),
                  SMB_SAFE_LOG_STRING(pFile->pszCachePath));

    ntStatus = SMBKrb5SetDefaultCachePath(
                    pFile->pszCachePath,
                    NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBSrvClientTreeOpen(
                    pszServer,
                    pFile->pszPrincipal,
                    pszShare,
                    &pFile->pTree);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBMbsToWc16s(
                    pszFilename,
                    &pwszFilename);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NPOpen(
                    pFile->pTree,
                    pwszFilename,
                    dwDesiredAccess,
                    dwSharedMode,
                    dwCreationDisposition,
                    dwFlagsAndAttributes,
                    &pFile->fid);
    BAIL_ON_NT_STATUS(ntStatus);

    *phFile = (HANDLE)pFile;

cleanup:

    SMB_SAFE_FREE_STRING(pszServer);
    SMB_SAFE_FREE_STRING(pszShare);
    SMB_SAFE_FREE_STRING(pszFilename);
    SMB_SAFE_FREE_MEMORY(pwszFilename);

    return ntStatus;

error:

    *phFile = NULL;

    goto cleanup;
}

static
NTSTATUS
ParseSharePath(
    PCWSTR pwszPath,
    PSTR*   ppszServer,
    PSTR*   ppszShare,
    PSTR*   ppszFilename
    )
{
    NTSTATUS ntStatus = 0;
    PSTR  pszPath = NULL;
    PSTR  pszIndex = NULL;
    PSTR  pszServer = NULL;
    PSTR  pszShare  = NULL;
    PSTR  pszFilename = NULL;
    size_t sLen = 0;

    ntStatus = SMBWc16sToMbs(
                    pwszPath,
                    &pszPath);
    BAIL_ON_NT_STATUS(ntStatus);

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
        ntStatus = SMB_ERROR_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    // Seek server name
    sLen = strcspn(pszIndex, "\\/");
    if (!sLen)
    {
        ntStatus = SMB_ERROR_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SMBStrndup(
                    pszIndex,
                    sLen,
                    &pszServer);
    BAIL_ON_NT_STATUS(ntStatus);

    pszIndex += sLen;

    // Skip delimiter
    sLen = strspn(pszIndex, "\\/");
    if (!sLen)
    {
        ntStatus = SMB_ERROR_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pszIndex += sLen;

    // Seek share name
    sLen = strcspn(pszIndex, "\\/");
    if (!sLen)
    {
        ntStatus = SMB_ERROR_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (!strncasecmp(pszIndex, "pipe", sLen) ||
        !strncasecmp(pszIndex, "ipc$", sLen))
    {
        ntStatus = SMBAllocateStringPrintf(
                        &pszShare,
                        "\\\\%s\\IPC$",
                        pszServer);
        BAIL_ON_NT_STATUS(ntStatus);
    }
    else
    {
        ntStatus = SMBAllocateMemory(
                        sizeof("\\\\") - 1 + strlen(pszServer) + sizeof("\\") - 1 + sLen + 1,
                        (PVOID*)&pszShare);
        BAIL_ON_NT_STATUS(ntStatus);

        sprintf(pszShare, "\\\\%s\\", pszServer);
        strncat(pszShare, pszIndex, sLen);
    }

    pszIndex += sLen;

    // Skip delimiter
    sLen = strspn(pszIndex, "\\/");
    if (!sLen)
    {
        ntStatus = SMB_ERROR_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pszIndex += sLen;

    // Seek file name
    sLen = strcspn(pszIndex, "\\/");
    if (!sLen)
    {
        ntStatus = SMB_ERROR_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SMBAllocateMemory(
                    sLen + 2,
                    (PVOID*)&pszFilename);
    BAIL_ON_NT_STATUS(ntStatus);

    snprintf(pszFilename, sLen + 2, "\\%s", pszIndex);

    pszIndex += sLen;

    if (!IsNullOrEmptyString(pszIndex))
    {
        ntStatus = SMB_ERROR_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppszServer = pszServer;
    *ppszShare  = pszShare;
    *ppszFilename = pszFilename;

cleanup:

    SMB_SAFE_FREE_STRING(pszPath);

    return ntStatus;

error:

    SMB_SAFE_FREE_STRING(pszServer);
    SMB_SAFE_FREE_STRING(pszShare);
    SMB_SAFE_FREE_STRING(pszFilename);

    *ppszServer = NULL;
    *ppszShare = NULL;
    *ppszFilename = NULL;

    goto cleanup;
}

