/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

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
SrvBuildFilePathString(
    IN PWSTR pwszPrefix,
    IN PWSTR pwszSuffix,
    OUT PWSTR* ppwszFilename
    )
{
    NTSTATUS  ntStatus       = 0;
    size_t    len_prefix     = 0;
    size_t    len_suffix     = 0;
    size_t    len_separator  = 0;
    PWSTR     pDataCursor    = NULL;
    wchar16_t wszFwdSlash[]  = {'/',  0};
    wchar16_t wszBackSlash[] = {'\\', 0};
    PWSTR     pwszFilename   = NULL;

    if (!pwszSuffix)
    {
        ntStatus = STATUS_INVALID_PARAMETER_2;
    }
    if (!ppwszFilename)
    {
        ntStatus = STATUS_INVALID_PARAMETER_3;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    len_prefix = pwszPrefix ? wc16slen(pwszPrefix) : 0;
    len_suffix = wc16slen(pwszSuffix);

    if (len_prefix && len_suffix && *pwszSuffix &&
        (*pwszSuffix != wszFwdSlash[0]) && (*pwszSuffix != wszBackSlash[0]))
    {
        len_separator = sizeof(wszBackSlash[0]);
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
        *pDataCursor++ = wszBackSlash[0];
    }

    while (pwszSuffix && *pwszSuffix)
    {
        *pDataCursor++ = *pwszSuffix++;
    }

    pDataCursor = pwszFilename;
    while (pDataCursor && *pDataCursor)
    {
        if (*pDataCursor == wszFwdSlash[0])
        {
            *pDataCursor = wszBackSlash[0];
        }
        pDataCursor++;
    }

    *ppwszFilename = pwszFilename;

cleanup:

    return ntStatus;

error:

    if (ppwszFilename)
    {
        *ppwszFilename = NULL;
    }

    if (pwszFilename)
    {
        SrvFreeMemory(pwszFilename);
    }

    goto cleanup;
}

NTSTATUS
SrvBuildFilePath(
    IN PWSTR pwszPrefix,
    IN PWSTR pwszSuffix,
    OUT PUNICODE_STRING pFilename
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PWSTR pwszFilename = NULL;
    UNICODE_STRING filename = { 0 };

    ntStatus = SrvBuildFilePathString(pwszPrefix, pwszSuffix, &pwszFilename);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvInitializeUnicodeString(pwszFilename, &filename);
    BAIL_ON_NT_STATUS(ntStatus);

    pwszFilename = NULL;

cleanup:
    if (pwszFilename)
    {
        SrvFreeMemory(pwszFilename);
    }

    *pFilename = filename;

    return ntStatus;

error:
    SRV_FREE_UNICODE_STRING(&filename);

    goto cleanup;
}

NTSTATUS
SrvGetParentPath(
    IN PUNICODE_STRING pPath,
    OUT PUNICODE_STRING pParentPath
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    int i = 0;
    PWCHAR pSeparator = NULL;
    UNICODE_STRING parentPath = { 0 };

    if (!RTL_STRING_NUM_CHARS(pPath) ||
        !IoRtlPathIsSeparator(pPath->Buffer[0]))
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    for (i = RTL_STRING_NUM_CHARS(pPath) - 1; i >= 0; i--)
    {
        if (IoRtlPathIsSeparator(pPath->Buffer[i]))
        {
            pSeparator = &pPath->Buffer[i];
            break;
        }
    }

    if (pSeparator)
    {
        UNICODE_STRING found = { 0 };

        found.Buffer = pPath->Buffer;
        found.Length = LwRtlPointerToOffset(pPath->Buffer, &pPath->Buffer[i]);
        found.MaximumLength = found.Length;

        ntStatus = SrvAllocateUnicodeString(&found, &parentPath);
        BAIL_ON_NT_STATUS(ntStatus);
    }
    else
    {
        WCHAR wszBackslash[] = { '\\', 0 };

        ntStatus = SrvAllocateUnicodeStringW(wszBackslash, &parentPath);
        BAIL_ON_NT_STATUS(ntStatus);
    }

cleanup:

    *pParentPath = parentPath;

    return ntStatus;

error:

    SRV_FREE_UNICODE_STRING(&parentPath);

    goto cleanup;
}

PCSTR
SrvPathGetFileName(
    PCSTR  pszPath
    )
{
    PCSTR  pszCursor     = pszPath;
    size_t sLen          = 0;
    CHAR   szBackSlash[] = { '\\', 0 };
    CHAR   szFwdSlash[]  = { '/',  0 };

    if (pszPath && (sLen = strlen(pszPath)))
    {
        pszCursor = pszPath + sLen - 1;

        while (!IsNullOrEmptyString(pszCursor) && (pszCursor != pszPath))
        {
            if ((*pszCursor == szBackSlash[0]) || (*pszCursor == szFwdSlash[0]))
            {
                pszCursor++;
                break;
            }

            pszCursor--;
        }
    }

    return pszCursor;
}

NTSTATUS
SrvMatchPathPrefix(
    PWSTR pwszPath,
    ULONG ulPathLength,
    PWSTR pwszPrefix
    )
{
    NTSTATUS ntStatus = STATUS_NO_MATCH;
    ULONG   ulPrefixLength = wc16slen(pwszPrefix);
    PWSTR   pwszTmp = NULL;

    if (ulPathLength >= ulPrefixLength)
    {
        ntStatus = SrvAllocateMemory(
                        (ulPrefixLength + 1) * sizeof(wchar16_t),
                        (PVOID*)&pwszTmp);
        BAIL_ON_NT_STATUS(ntStatus);

        memcpy( (PBYTE)pwszTmp,
                (PBYTE)pwszPath,
                ulPrefixLength * sizeof(wchar16_t));

        if (!SMBWc16sCaseCmp(pwszTmp, pwszPrefix))
        {
            ntStatus = STATUS_SUCCESS;
        }
    }

error:

    SRV_SAFE_FREE_MEMORY(pwszTmp);

    return ntStatus;
}
