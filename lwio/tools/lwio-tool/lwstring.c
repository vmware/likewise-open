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
 *        lwstring.c
 *
 * Abstract:
 *
 *        LW RTL String Routines
 *
 * Authors: Danilo Almeida (dalmeida@likewisesoftware.com)
 */

#include "config.h"
#include "lwiosys.h"
#include "lwstring.h"
#include "goto.h"

VOID
LwCStringFree(
    IN OUT PSTR* ppszString
    )
{
    // TODO -- Memory model? (currenlty using free)
    if (*ppszString)
    {
        free(*ppszString);
        *ppszString = NULL;
    }
}


NTSTATUS
LwCStringPrintfV(
    OUT PSTR* ppszString,
    IN PCSTR pszFormat,
    IN va_list Args
    )
{
    NTSTATUS status = 0;
    PSTR pszNewString = NULL;
    int count = 0;

    // TODO -- Memory model? (currenlty using free)
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
LwCStringPrintf(
    OUT PSTR* ppszString,
    IN PCSTR pszFormat,
    IN ...
    )
{
    NTSTATUS status = 0;
    va_list args;

    va_start(args, pszFormat);
    status = LwCStringPrintfV(ppszString, pszFormat, args);
    va_end(args);

    return status;
}

NTSTATUS
LwCStringAppendPrintfV(
    IN OUT PSTR* ppszString,
    IN PCSTR pszFormat,
    IN va_list Args
    )
{
    NTSTATUS status = 0;
    PSTR pszAddString = NULL;
    PSTR pszNewString = NULL;
    PSTR pszResultString = *ppszString;

    status = LwCStringPrintfV(&pszAddString, pszFormat, Args);
    GOTO_CLEANUP_ON_STATUS(status);

    if (pszResultString)
    {
        status = LwCStringPrintf(&pszNewString, "%s%s", pszResultString, pszAddString);
        GOTO_CLEANUP_ON_STATUS(status);
        LwCStringFree(&pszResultString);
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
        LwCStringFree(&pszNewString);
    }
    else
    {
        *ppszString = pszResultString;
    }

    LwCStringFree(&pszAddString);

    return status;
}

NTSTATUS
LwCStringAppendPrintf(
    IN OUT PSTR* ppszString,
    IN PCSTR pszFormat,
    ...
    )
{
    NTSTATUS status = 0;
    va_list args;

    va_start(args, pszFormat);
    status = LwCStringAppendPrintfV(ppszString, pszFormat, args);
    va_end(args);

    return status;
}
