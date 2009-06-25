/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        utils.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Server share utilities
 *
 * Authors: Krishna Ganugapati (kganugapati@likewise.com)
 *          Sriram Nambakam (snambakam@likewise.com)
 *
 *
 */

#include "includes.h"

static
NTSTATUS
SrvGetShareNameCheckHostname(
    PCSTR  pszHostname,
    PCSTR  pszDomain,
    PWSTR  pwszPath,
    PWSTR* ppwszSharename
    );

static
NTSTATUS
SrvGetShareNameCheckFQDN(
    PCSTR  pszHostname,
    PCSTR  pszDomain,
    PWSTR  pwszPath,
    PWSTR* ppwszSharename
    );

NTSTATUS
SrvGetShareName(
    IN  PCSTR  pszHostname,
    IN  PCSTR  pszDomain,
    IN  PWSTR  pwszPath,
    OUT PWSTR* ppwszSharename
    )
{
    NTSTATUS ntStatus = 0;
    PWSTR    pwszSharename = NULL;

    ntStatus = SrvGetShareNameCheckHostname(
                    pszHostname,
                    pszDomain,
                    pwszPath,
                    &pwszSharename);
    if (ntStatus)
    {
        ntStatus = SrvGetShareNameCheckFQDN(
                        pszHostname,
                        pszDomain,
                        pwszPath,
                        &pwszSharename);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppwszSharename = pwszSharename;

cleanup:

    return ntStatus;

error:

    *ppwszSharename = NULL;

    goto cleanup;
}

static
NTSTATUS
SrvGetShareNameCheckHostname(
    PCSTR  pszHostname,
    PCSTR  pszDomain,
    PWSTR  pwszPath,
    PWSTR* ppwszSharename
    )
{
    NTSTATUS  ntStatus = 0;
    PSTR      pszHostPrefix = NULL;
    PWSTR     pwszHostPrefix = NULL;
    PWSTR     pwszPath_copy = NULL;
    PWSTR     pwszSharename = NULL;
    size_t    len = 0, len_prefix = 0, len_sharename = 0;

    len = wc16slen(pwszPath);
    if (!len)
    {
        ntStatus = STATUS_OBJECT_PATH_INVALID;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SrvAllocateMemory(
                    (len + 1) * sizeof(wchar16_t),
                    (PVOID*)&pwszPath_copy);
    BAIL_ON_NT_STATUS(ntStatus);

    memcpy(pwszPath_copy, pwszPath, len * sizeof(wchar16_t));

    wc16supper(pwszPath_copy);

    ntStatus = SrvAllocateStringPrintf(
                    &pszHostPrefix,
                    "\\\\%s\\",
                    pszHostname);
    BAIL_ON_NT_STATUS(ntStatus);

    SMBStrToUpper(pszHostPrefix);

    ntStatus = SrvMbsToWc16s(
                    pszHostPrefix,
                    &pwszHostPrefix);
    BAIL_ON_NT_STATUS(ntStatus);

    len_prefix = wc16slen(pwszHostPrefix);
    if (len <= len_prefix)
    {
        ntStatus = STATUS_OBJECT_PATH_INVALID;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (memcmp((PBYTE)pwszPath_copy, (PBYTE)pwszHostPrefix, len_prefix * sizeof(wchar16_t)) != 0)
    {
        ntStatus = STATUS_OBJECT_PATH_INVALID;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    len_sharename = wc16slen(pwszPath_copy + len_prefix);
    if (!len_sharename)
    {
        ntStatus = STATUS_OBJECT_PATH_INVALID;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SrvAllocateMemory(
                    (len_sharename + 1) * sizeof(wchar16_t),
                    (PVOID*)&pwszSharename);
    BAIL_ON_NT_STATUS(ntStatus);

    // copy from original path
    memcpy((PBYTE)pwszSharename, (PBYTE)pwszPath + len_prefix * sizeof(wchar16_t), len_sharename * sizeof(wchar16_t));

    *ppwszSharename = pwszSharename;

cleanup:

    if (pszHostPrefix)
    {
        SrvFreeMemory(pszHostPrefix);
    }
    if (pwszHostPrefix)
    {
        SrvFreeMemory(pwszHostPrefix);
    }
    if (pwszPath_copy)
    {
        SrvFreeMemory(pwszPath_copy);
    }

    return ntStatus;

error:

    *ppwszSharename = NULL;

    if (pwszSharename)
    {
        SrvFreeMemory(pwszSharename);
    }

    goto cleanup;
}

static
NTSTATUS
SrvGetShareNameCheckFQDN(
    PCSTR  pszHostname,
    PCSTR  pszDomain,
    PWSTR  pwszPath,
    PWSTR* ppwszSharename
    )
{
    NTSTATUS  ntStatus = 0;
    PSTR      pszHostPrefix = NULL;
    PWSTR     pwszHostPrefix = NULL;
    PWSTR     pwszPath_copy = NULL;
    PWSTR     pwszSharename = NULL;
    size_t    len = 0, len_prefix = 0, len_sharename = 0;

    len = wc16slen(pwszPath);
    if (!len)
    {
        ntStatus = STATUS_OBJECT_PATH_INVALID;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SrvAllocateMemory(
                    (len + 1) * sizeof(wchar16_t),
                    (PVOID*)&pwszPath_copy);
    BAIL_ON_NT_STATUS(ntStatus);

    memcpy(pwszPath_copy, pwszPath, len * sizeof(wchar16_t));

    wc16supper(pwszPath_copy);

    ntStatus = SrvAllocateStringPrintf(
                    &pszHostPrefix,
                    "\\\\%s.%s\\",
                    pszHostname,
                    pszDomain);
    BAIL_ON_NT_STATUS(ntStatus);

    SMBStrToUpper(pszHostPrefix);

    ntStatus = SrvMbsToWc16s(
                    pszHostPrefix,
                    &pwszHostPrefix);
    BAIL_ON_NT_STATUS(ntStatus);

    len_prefix = wc16slen(pwszHostPrefix);
    if (len <= len_prefix)
    {
        ntStatus = STATUS_OBJECT_PATH_INVALID;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (memcmp((PBYTE)pwszPath_copy, (PBYTE)pwszHostPrefix, len_prefix * sizeof(wchar16_t)) != 0)
    {
        ntStatus = STATUS_OBJECT_PATH_INVALID;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    len_sharename = wc16slen(pwszPath_copy + len_prefix);
    if (!len_sharename)
    {
        ntStatus = STATUS_OBJECT_PATH_INVALID;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SrvAllocateMemory(
                    (len_sharename + 1) * sizeof(wchar16_t),
                    (PVOID*)&pwszSharename);
    BAIL_ON_NT_STATUS(ntStatus);

    // copy from original path
    memcpy((PBYTE)pwszSharename, (PBYTE)pwszPath + len_prefix * sizeof(wchar16_t), len_sharename * sizeof(wchar16_t));

    *ppwszSharename = pwszSharename;

cleanup:

    if (pszHostPrefix)
    {
        SrvFreeMemory(pszHostPrefix);
    }
    if (pwszHostPrefix)
    {
        SrvFreeMemory(pwszHostPrefix);
    }
    if (pwszPath_copy)
    {
        SrvFreeMemory(pwszPath_copy);
    }

    return ntStatus;

error:

    *ppwszSharename = NULL;

    if (pwszSharename)
    {
        SrvFreeMemory(pwszSharename);
    }

    goto cleanup;
}

NTSTATUS
SrvGetMaximalShareAccessMask(
    PSRV_SHARE_INFO pShareInfo,
    ACCESS_MASK*   pMask
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN  bInLock = FALSE;
    ACCESS_MASK mask = 0;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pShareInfo->mutex);

    switch (pShareInfo->service)
    {
        case SHARE_SERVICE_NAMED_PIPE:

            mask = (FILE_READ_DATA |
                    FILE_WRITE_DATA |
                    FILE_APPEND_DATA |
                    FILE_READ_EA |
                    FILE_WRITE_EA |
                    FILE_EXECUTE |
                    FILE_DELETE_CHILD |
                    FILE_READ_ATTRIBUTES |
                    FILE_WRITE_ATTRIBUTES);

            break;

        case SHARE_SERVICE_DISK_SHARE:

            mask = 0x1FF;

            break;

        case SHARE_SERVICE_PRINTER:
        case SHARE_SERVICE_COMM_DEVICE:
        case SHARE_SERVICE_ANY:

            mask = GENERIC_READ;

            break;

        default:

            mask = 0;

            break;
    }

    *pMask = mask;

    LWIO_UNLOCK_RWMUTEX(bInLock, &pShareInfo->mutex);

    return ntStatus;
}

NTSTATUS
SrvGetGuestShareAccessMask(
    PSRV_SHARE_INFO pShareInfo,
    ACCESS_MASK*   pMask
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN  bInLock = FALSE;
    ACCESS_MASK mask = 0;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pShareInfo->mutex);

    switch (pShareInfo->service)
    {
        case SHARE_SERVICE_NAMED_PIPE:

            mask = (FILE_READ_DATA |
                    FILE_WRITE_DATA |
                    FILE_APPEND_DATA |
                    FILE_READ_EA |
                    FILE_WRITE_EA |
                    FILE_EXECUTE |
                    FILE_DELETE_CHILD |
                    FILE_READ_ATTRIBUTES |
                    FILE_WRITE_ATTRIBUTES);

            break;

        case SHARE_SERVICE_DISK_SHARE:

            mask = 0x1FF;

            break;

        case SHARE_SERVICE_PRINTER:
        case SHARE_SERVICE_COMM_DEVICE:
        case SHARE_SERVICE_ANY:

            mask = 0;

            break;

        default:

            mask = 0;

            break;
    }

    *pMask = mask;

    LWIO_UNLOCK_RWMUTEX(bInLock, &pShareInfo->mutex);

    return ntStatus;
}

