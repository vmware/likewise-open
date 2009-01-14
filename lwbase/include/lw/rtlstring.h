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

#ifndef __RTL_STRING_H__
#define __RTL_STRING_H__

#include <lw/types.h>
#include <lw/attrs.h>
#include <lw/ntstatus.h>

typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    wchar16_t* Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef struct _ANSI_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PCHAR Buffer;
} ANSI_STRING, *PANSI_STRING;

VOID
RtlUnicodeStringInit(
    OUT PUNICODE_STRING pString,
    IN PWSTR pszString
    );

VOID
RtlAnsiStringInit(
    OUT PANSI_STRING pString,
    IN PSTR pszString
    );

NTSTATUS
RtlUnicodeStringCreateFromCString(
    OUT PUNICODE_STRING pString,
    IN PCSTR pszString
    );

NTSTATUS
RtlWC16StringCreateFromCString(
    OUT PWSTR* ppszNewString,
    IN PCSTR pszOriginalString
    );

VOID
RtlUnicodeStringFree(
    IN OUT PUNICODE_STRING pString
    );

VOID
RtlAnsiStringFree(
    IN OUT PANSI_STRING pString
    );

NTSTATUS
RtlUnicodeStringDuplicate(
    OUT PUNICODE_STRING pNewString,
    IN PUNICODE_STRING pOriginalString
    );

NTSTATUS
RtlAnsiStringDuplicate(
    OUT PANSI_STRING pNewString,
    IN PANSI_STRING pOriginalString
    );

NTSTATUS
RtlWC16StringDuplicate(
    OUT PWSTR* ppszNewString,
    IN PCWSTR pszOriginalString
    );

NTSTATUS
RtlCStringDuplicate(
    OUT PSTR* ppszNewString,
    IN PCSTR pszOriginalString
    );

BOOLEAN
RtlUnicodeStringIsEqual(
    IN PUNICODE_STRING pString1,
    IN PUNICODE_STRING pString2,
    IN BOOLEAN bIsCaseSensitive
    );

#define USE_RTL_STRING_LOG_HACK 1

#ifdef USE_RTL_STRING_LOG_HACK
PCSTR
RtlUnicodeStringToLog(
    IN PUNICODE_STRING pString
    );

PCSTR
RtlAnsiStringToLog(
    IN PANSI_STRING pString
    );

PCSTR
RtlWC16StringToLog(
    IN PCWSTR pszString
    );
#endif

#endif /* __RTL_STRING_H__ */
