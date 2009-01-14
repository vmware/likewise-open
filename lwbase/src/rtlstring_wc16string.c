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

#include "includes.h"
#include <lw/rtlstring.h>
#include <lw/rtlmemory.h>
#include <lw/rtlgoto.h>
#include <wc16str.h>

size_t
LwRtlWC16StringNumChars(
    IN PCWSTR pszString
    )
{
    return wc16slen(pszString);
}

NTSTATUS
LwRtlWC16StringAllocateFromCString(
    OUT PWSTR* ppszNewString,
    IN PCSTR pszOriginalString
    )
{
    NTSTATUS status = 0;
    PWSTR pszNewString = NULL;

    if (pszOriginalString)
    {
        pszNewString = ambstowc16s(pszOriginalString);
        if (!pszNewString)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            GOTO_CLEANUP();
        }
    }

cleanup:
    if (status)
    {
        free(pszNewString);
        pszNewString = NULL;
    }
    *ppszNewString = pszNewString;
    return status;
}

NTSTATUS
LwRtlWC16StringDuplicate(
    OUT PWSTR* ppszNewString,
    IN PCWSTR pszOriginalString
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    size_t size = 0;
    PWSTR pszNewString = NULL;

    if (!pszOriginalString)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }

    size = (LwRtlWC16StringNumChars(pszOriginalString) + 1) * sizeof(pszOriginalString[0]);

    status = RTL_ALLOCATE(&pszNewString, wchar16_t, size);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    memcpy(pszNewString, pszOriginalString, size);

cleanup:
    if (status)
    {
        RTL_FREE(&pszNewString);
    }

    *ppszNewString = pszNewString;

    return status;
}

VOID
LwRtlWC16StringFree(
    OUT PWSTR* ppszString
    )
{
    RTL_FREE(ppszString);
}
