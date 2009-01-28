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

VOID
LwRtlAnsiStringInit(
    OUT PANSI_STRING pString,
    IN PSTR pszString
    )
{
    pString->Buffer = pszString;
    pString->Length = pszString ? strlen(pszString) * sizeof(pString->Buffer[0]): 0;
    pString->MaximumLength = pString->Length;
}

NTSTATUS
LwRtlAnsiStringAllocateFromCString(
    OUT PANSI_STRING pNewString,
    IN PCSTR pszString
    )
{
    NTSTATUS status = 0;
    PSTR pszNewString = NULL;
    ANSI_STRING newString = { 0 };

    status = RtlCStringDuplicate(&pszNewString, pszString);
    GOTO_CLEANUP_ON_STATUS(status);

    newString.Buffer = pszNewString;
    pszNewString = 0;
    newString.Length = strlen(newString.Buffer) * sizeof(newString.Buffer[0]);
    newString.MaximumLength = newString.Length + sizeof(newString.Buffer[0]);

cleanup:
    if (status)
    {
        RtlCStringFree(&pszNewString);
        RtlAnsiStringFree(&newString);
    }

    *pNewString = newString;

    return status;
}

#if 0
NTSTATUS
LwRtlAnsiStringAllocateFromWC16String(
    OUT PANSI_STRING pNewString,
    IN PCWSTR pszString
    )
{
    NTSTATUS status = 0;
    PSTR pszNewString = NULL;
    ANSI_STRING newString = { 0 };

    status = RtlCStringDuplicate(&pszNewString, pszString);
    GOTO_CLEANUP_ON_STATUS(status);

    newString.Buffer = pszNewString;
    pszNewString = 0;
    newString.Length = wc16slen(newString.Buffer) * sizeof(newString.Buffer[0]);
    newString.MaximumLength = newString.Length + sizeof(newString.Buffer[0]);

cleanup:
    if (status)
    {
        RtlCStringFree(&pszNewString);
        RtlAnsiStringFree(&newString);
    }

    *pString = newString;

    return status;
}

NTSTATUS
LwRtlAnsiStringAllocateFromUnicodeString(
    OUT PANSI_STRING pNewString,
    IN PUNICODE_STRING pString
    )
{
    NTSTATUS status = 0;
    PSTR pszNewString = NULL;
    ANSI_STRING newString = { 0 };

    status = RtlCStringDuplicate(&pszNewString, pszString);
    GOTO_CLEANUP_ON_STATUS(status);

    newString.Buffer = pszNewString;
    pszNewString = 0;
    newString.Length = wc16slen(newString.Buffer) * sizeof(newString.Buffer[0]);
    newString.MaximumLength = newString.Length + sizeof(newString.Buffer[0]);

cleanup:
    if (status)
    {
        RtlCStringFree(&pszNewString);
        RtlAnsiStringFree(&newString);
    }

    *pString = newString;

    return status;
}
#endif

NTSTATUS
LwRtlAnsiStringDuplicate(
    OUT PANSI_STRING pNewString,
    IN PANSI_STRING pOriginalString
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    ANSI_STRING newString = { 0 };

    if (!pOriginalString || !pNewString)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }

    if (pOriginalString->Buffer && pOriginalString->Length > 0)
    {
        // Add a NULL anyhow.

        newString.Length = pOriginalString->Length;
        newString.MaximumLength = pOriginalString->Length + sizeof(pOriginalString->Buffer[0]);

        status = RTL_ALLOCATE(&newString.Buffer, CHAR, newString.MaximumLength);
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);

        memcpy(newString.Buffer, pOriginalString->Buffer, pOriginalString->Length);
        newString.Buffer[newString.Length/sizeof(newString.Buffer[0])] = 0;
    }

cleanup:
    if (status)
    {
        RtlAnsiStringFree(&newString);
    }

    *pNewString = newString;

    return status;
}

VOID
LwRtlAnsiStringFree(
    IN OUT PANSI_STRING pString
    )
{
    RTL_FREE(&pString->Buffer);
    pString->Length = pString->MaximumLength = 0;
}
