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

typedef struct _LW_UNICODE_STRING {
    LW_USHORT Length;
    LW_USHORT MaximumLength;
    LW_PWCHAR Buffer;
} LW_UNICODE_STRING, *LW_PUNICODE_STRING;

typedef struct _LW_ANSI_STRING {
    LW_USHORT Length;
    LW_USHORT MaximumLength;
    LW_PCHAR Buffer;
} LW_ANSI_STRING, *LW_PANSI_STRING;

#ifndef LW_STRICT_NAMESPACE

typedef LW_UNICODE_STRING UNICODE_STRING;
typedef LW_PUNICODE_STRING PUNICODE_STRING;

typedef LW_ANSI_STRING ANSI_STRING;
typedef LW_PANSI_STRING PANSI_STRING;

#endif /* LW_STRICT_NAMESPAE */

// c-style (null-terminated) strings

size_t
LwRtlCStringNumChars(
    LW_IN LW_PCSTR pszString
    );

LW_NTSTATUS
LwRtlCStringAllocateFromWC16String(
    LW_OUT LW_PSTR* ppszNewString,
    LW_IN LW_PCWSTR pszOriginalString
    );

LW_NTSTATUS
LwRtlCStringDuplicate(
    LW_OUT LW_PSTR* ppszNewString,
    LW_IN LW_PCSTR pszOriginalString
    );

LW_VOID
LwRtlCStringFree(
    LW_IN LW_OUT LW_PSTR* ppszString
    );

LW_NTSTATUS
LwRtlCStringAllocatePrintf(
    LW_OUT PSTR* ppszString,
    LW_IN PCSTR pszFormat,
    LW_IN ...
    );

LW_NTSTATUS
LwRtlCStringAllocateAppendPrintf(
    LW_IN LW_OUT LW_PSTR* ppszString,
    LW_IN LW_PCSTR pszFormat,
    ...
    );

#define LwRtlCStringIsNullOrEmpty(String) (!(String) || !(*(String)))

// wc16-style (null-terminated) strings

size_t
LwRtlWC16StringNumChars(
    LW_IN LW_PCWSTR pszString
    );

LW_NTSTATUS
LwRtlWC16StringAllocateFromCString(
    LW_OUT LW_PWSTR* ppszNewString,
    LW_IN LW_PCSTR pszOriginalString
    );

LW_NTSTATUS
LwRtlWC16StringDuplicate(
    LW_OUT LW_PWSTR* ppszNewString,
    LW_IN LW_PCWSTR pszOriginalString
    );

LW_VOID
LwRtlWC16StringFree(
    LW_OUT LW_PWSTR* ppszString
    );

#define LwRtlWC16StringIsNullOrEmpty(String) LwRtlCStringIsNullOrEmpty(String)

// UNICODE_STRING strings

LW_VOID
LwRtlUnicodeStringInit(
    LW_OUT LW_PUNICODE_STRING pString,
    LW_IN LW_PWSTR pszString
    );

LW_NTSTATUS
LwRtlUnicodeStringAllocateFromWC16String(
    LW_OUT LW_PUNICODE_STRING pString,
    LW_IN LW_PCWSTR pszString
    );

LW_NTSTATUS
LwRtlUnicodeStringAllocateFromCString(
    LW_OUT LW_PUNICODE_STRING pString,
    LW_IN LW_PCSTR pszString
    );

LW_NTSTATUS
LwRtlUnicodeStringDuplicate(
    LW_OUT LW_PUNICODE_STRING pNewString,
    LW_IN LW_PUNICODE_STRING pOriginalString
    );

LW_VOID
LwRtlUnicodeStringFree(
    LW_IN LW_OUT LW_PUNICODE_STRING pString
    );

LW_BOOLEAN
LwRtlUnicodeStringIsEqual(
    LW_IN LW_PUNICODE_STRING pString1,
    LW_IN LW_PUNICODE_STRING pString2,
    LW_IN LW_BOOLEAN bIsCaseSensitive
    );

// ANSI strings

LW_VOID
LwRtlAnsiStringInit(
    LW_OUT LW_PANSI_STRING pString,
    LW_IN LW_PSTR pszString
    );

LW_NTSTATUS
LwRtlAnsiStringAllocateFromCString(
    LW_OUT LW_PANSI_STRING pNewString,
    LW_IN LW_PCSTR pszString
    );

LW_NTSTATUS
LwRtlAnsiStringDuplicate(
    LW_OUT LW_PANSI_STRING pNewString,
    LW_IN LW_PANSI_STRING pOriginalString
    );

LW_VOID
LwRtlAnsiStringFree(
    LW_IN LW_OUT LW_PANSI_STRING pString
    );

#ifndef LW_STRICT_NAMESPACE

#define RtlCStringNumChars LwRtlCStringNumChars
#define RtlCStringAllocateFromWC16String LwRtlCStringAllocateFromWC16String
#define RtlCStringDuplicate LwRtlCStringDuplicate
#define RtlCStringFree LwRtlCStringFree
#define RtlCStringAllocatePrintf LwRtlCStringAllocatePrintf
#define RtlCStringAllocateAppendPrintf LwRtlCStringAllocateAppendPrintf

#define RtlWC16StringNumChars LwRtlWC16StringNumChars
#define RtlWC16StringAllocateFromCString LwRtlWC16StringAllocateFromCString
#define RtlWC16StringDuplicate LwRtlWC16StringDuplicate
#define RtlWC16StringFree LwRtlWC16StringFree

#define RtlUnicodeStringInit LwRtlUnicodeStringInit
#define RtlUnicodeStringAllocateFromWC16String LwRtlUnicodeStringAllocateFromWC16String
#define RtlUnicodeStringAllocateFromCString LwRtlUnicodeStringAllocateFromCString
#define RtlUnicodeStringDuplicate LwRtlUnicodeStringDuplicate
#define RtlUnicodeStringFree LwRtlUnicodeStringFree
#define RtlUnicodeStringIsEqual LwRtlUnicodeStringIsEqual

#define RtlAnsiStringInit LwRtlAnsiStringInit
#define RtlAnsiStringAllocateFromCString LwRtlAnsiStringAllocateFromCString
#define RtlAnsiStringDuplicate LwRtlAnsiStringDuplicate
#define RtlAnsiStringFree LwRtlAnsiStringFree

#endif /* LW_STRICT_NAMESPAE */

#define USE_RTL_STRING_LOG_HACK 1

#ifdef USE_RTL_STRING_LOG_HACK
LW_PCSTR
LwRtlUnicodeStringToLog(
    LW_IN LW_PUNICODE_STRING pString
    );

LW_PCSTR
LwRtlAnsiStringToLog(
    LW_IN LW_PANSI_STRING pString
    );

LW_PCSTR
LwRtlWC16StringToLog(
    LW_IN LW_PCWSTR pszString
    );

#ifndef LW_STRICT_NAMESPACE
#define RtlUnicodeStringToLog LwRtlUnicodeStringToLog
#define RtlAnsiStringToLog LwRtlAnsiStringToLog
#define RtlWC16StringToLog LwRtlWC16StringToLog
#endif /* LW_STRICT_NAMESPAE */

#endif /* USE_RTL_STRING_LOG_HACK */

#endif /* __RTL_STRING_H__ */
