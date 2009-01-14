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
#include <stdio.h>
#include <stdarg.h>

size_t
LwRtlCStringNumChars(
    IN PCSTR pszString
    )
{
    return strlen(pszString);
}

NTSTATUS
LwRtlCStringAllocateFromWC16String(
    OUT PSTR* ppszNewString,
    IN PCWSTR pszOriginalString
    )
{
    NTSTATUS status = 0;
    PSTR pszNewString = NULL;

    if (pszOriginalString)
    {
        pszNewString = awc16stombs(pszOriginalString);
        if (!pszNewString)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            GOTO_CLEANUP();
        }
    }

cleanup:
    if (status)
    {
        LwRtlCStringFree(&pszNewString);
    }
    *ppszNewString = pszNewString;
    return status;
}

NTSTATUS
LwRtlCStringDuplicate(
    OUT PSTR* ppszNewString,
    IN PCSTR pszOriginalString
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    size_t size = 0;
    PSTR pszNewString = NULL;

    if (!pszOriginalString)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }

    size = (strlen(pszOriginalString) + 1) * sizeof(pszOriginalString[0]);

    status = RTL_ALLOCATE(&pszNewString, CHAR, size);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    memcpy(pszNewString, pszOriginalString, size);

cleanup:
    if (status)
    {
        LwRtlCStringFree(&pszNewString);
    }

    *ppszNewString = pszNewString;

    return status;
}

VOID
LwRtlCStringFree(
    IN OUT PSTR* ppszString
    )
{
    RTL_FREE(ppszString);
}

static
NTSTATUS
LwRtlCStringAllocatePrintfV(
    OUT PSTR* ppszString,
    IN PCSTR pszFormat,
    IN va_list Args
    )
{
    NTSTATUS status = 0;
    PSTR pszNewString = NULL;
    int count = 0;

    // TODO -- Memory model? (currenlty using free)
    // TODO -- Enhance with %Z, %wZ, etc.
    count = vasprintf(&pszNewString, pszFormat, Args);
    if (count < 0)
    {
        pszNewString = NULL;
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    *ppszString = pszNewString;

    return status;
}

NTSTATUS
LwRtlCStringAllocatePrintf(
    OUT PSTR* ppszString,
    IN PCSTR pszFormat,
    IN ...
    )
{
    NTSTATUS status = 0;
    va_list args;

    va_start(args, pszFormat);
    status = LwRtlCStringAllocatePrintfV(ppszString, pszFormat, args);
    va_end(args);

    return status;
}

static
NTSTATUS
LwRtlCStringAllocateAppendPrintfV(
    IN OUT PSTR* ppszString,
    IN PCSTR pszFormat,
    IN va_list Args
    )
{
    NTSTATUS status = 0;
    PSTR pszAddString = NULL;
    PSTR pszNewString = NULL;
    PSTR pszResultString = *ppszString;

    status = LwRtlCStringAllocatePrintfV(&pszAddString, pszFormat, Args);
    GOTO_CLEANUP_ON_STATUS(status);

    if (pszResultString)
    {
        status = LwRtlCStringAllocatePrintf(&pszNewString, "%s%s", pszResultString, pszAddString);
        GOTO_CLEANUP_ON_STATUS(status);
        LwRtlCStringFree(&pszResultString);
    }
    else
    {
        pszNewString = pszAddString;
        pszAddString = NULL;
    }

    pszResultString = pszNewString;

cleanup:
    if (status)
    {
        LwRtlCStringFree(&pszNewString);
    }
    else
    {
        *ppszString = pszResultString;
    }

    LwRtlCStringFree(&pszAddString);

    return status;
}

NTSTATUS
LwRtlCStringAllocateAppendPrintf(
    IN OUT PSTR* ppszString,
    IN PCSTR pszFormat,
    ...
    )
{
    NTSTATUS status = 0;
    va_list args;

    va_start(args, pszFormat);
    status = LwRtlCStringAllocateAppendPrintfV(ppszString, pszFormat, args);
    va_end(args);

    return status;
}

