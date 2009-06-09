/*
 * Copyright Likewise Software    2004-2009
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
 *        path.c
 *
 * Abstract:
 *
 *        Likewise Input Output (LWIO) - SRV
 *
 *        Utilities
 *
 *        Paths
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

NTSTATUS
SrvBuildFilePath(
    PWSTR  pwszPrefix,
    PWSTR  pwszSuffix,
    PWSTR* ppwszFilename
    )
{
    NTSTATUS ntStatus = 0;
    size_t              len_prefix = 0;
    size_t              len_suffix = 0;
    size_t              len_separator = 0;
    PWSTR               pDataCursor = NULL;
    wchar16_t           wszFwdSlash;
    wchar16_t           wszBackSlash;
    PWSTR               pwszFilename = NULL;

    if (!pwszPrefix)
    {
        ntStatus = STATUS_INVALID_PARAMETER_1;
    }
    if (!pwszSuffix)
    {
        ntStatus = STATUS_INVALID_PARAMETER_2;
    }
    if (!ppwszFilename)
    {
        ntStatus = STATUS_INVALID_PARAMETER_3;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    wcstowc16s(&wszFwdSlash, L"/", 1);
    wcstowc16s(&wszBackSlash, L"\\", 1);

    len_prefix = wc16slen(pwszPrefix);
    len_suffix = wc16slen(pwszSuffix);

    if (len_suffix && *pwszSuffix &&
        (*pwszSuffix != wszFwdSlash) && (*pwszSuffix != wszBackSlash))
    {
        len_separator = sizeof(wszBackSlash);
    }

    ntStatus = SrvAllocateMemory(
                    (len_prefix + len_suffix + len_separator + 1 ) * sizeof(wchar16_t),
                    (PVOID*)&pwszFilename);
    BAIL_ON_NT_STATUS(ntStatus);

    pDataCursor = pwszFilename;
    while (pwszPrefix && *pwszPrefix)
    {
        *pDataCursor++ = *pwszPrefix++;
    }

    if (len_separator)
    {
        *pDataCursor++ = wszBackSlash;
    }

    while (pwszSuffix && *pwszSuffix)
    {
        *pDataCursor++ = *pwszSuffix++;
    }

    pDataCursor = pwszFilename;
    while (pDataCursor && *pDataCursor)
    {
        if (*pDataCursor == wszFwdSlash)
        {
            *pDataCursor = wszBackSlash;
        }
        pDataCursor++;
    }

    *ppwszFilename = pwszFilename;

cleanup:

    return ntStatus;

error:

    *ppwszFilename = NULL;

    if (pwszFilename)
    {
        SrvFreeMemory(pwszFilename);
    }

    goto cleanup;
}
