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
    USHORT Length;
    USHORT MaximumLength;
    wchar16_t* Buffer;
} LW_UNICODE_STRING, *LW_PUNICODE_STRING;

typedef struct _LW_ANSI_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PCHAR Buffer;
} LW_ANSI_STRING, *LW_PANSI_STRING;

#ifndef LW_STRICT_NAMESPACE

typedef LW_UNICODE_STRING UNICODE_STRING;
typedef LW_PUNICODE_STRING PUNICODE_STRING;

typedef LW_ANSI_STRING ANSI_STRING;
typedef LW_PANSI_STRING PANSI_STRING;

#endif /* LW_STRICT_NAMESPAE */

VOID
LwRtlUnicodeStringInit(
    OUT LW_PUNICODE_STRING pString,
    IN LW_PWSTR pszString
    );

VOID
LwRtlAnsiStringInit(
    OUT LW_PANSI_STRING pString,
    IN LW_PSTR pszString
    );

NTSTATUS
LwRtlUnicodeStringCreateFromCString(
    OUT LW_PUNICODE_STRING pString,
    IN LW_PCSTR pszString
    );

NTSTATUS
LwRtlWC16StringCreateFromCString(
    OUT LW_PWSTR* ppszNewString,
    IN LW_PCSTR pszOriginalString
    );

VOID
LwRtlUnicodeStringFree(
    IN OUT LW_PUNICODE_STRING pString
    );

VOID
LwRtlAnsiStringFree(
    IN OUT LW_PANSI_STRING pString
    );

NTSTATUS
LwRtlUnicodeStringDuplicate(
    OUT LW_PUNICODE_STRING pNewString,
    IN LW_PUNICODE_STRING pOriginalString
    );

NTSTATUS
LwRtlAnsiStringDuplicate(
    OUT LW_PANSI_STRING pNewString,
    IN LW_PANSI_STRING pOriginalString
    );

NTSTATUS
LwRtlWC16StringDuplicate(
    OUT LW_PWSTR* ppszNewString,
    IN LW_PCWSTR pszOriginalString
    );

NTSTATUS
LwRtlCStringDuplicate(
    OUT LW_PSTR* ppszNewString,
    IN LW_PCSTR pszOriginalString
    );

BOOLEAN
LwRtlUnicodeStringIsEqual(
    IN LW_PUNICODE_STRING pString1,
    IN LW_PUNICODE_STRING pString2,
    IN LW_BOOLEAN bIsCaseSensitive
    );


#ifndef LW_STRICT_NAMESPACE
#define RtlUnicodeStringInit LwRtlUnicodeStringInit
#define RtlAnsiStringInit LwRtlAnsiStringInit
#define RtlUnicodeStringCreateFromCString LwRtlUnicodeStringCreateFromCString
#define RtlWC16StringCreateFromCString LwRtlWC16StringCreateFromCString
#define RtlUnicodeStringFree LwRtlUnicodeStringFree
#define RtlAnsiStringFree LwRtlAnsiStringFree
#define RtlUnicodeStringDuplicate LwRtlUnicodeStringDuplicate
#define RtlAnsiStringDuplicate LwRtlAnsiStringDuplicate
#define RtlWC16StringDuplicate LwRtlWC16StringDuplicate
#define RtlCStringDuplicate LwRtlCStringDuplicate
#define RtlUnicodeStringIsEqual LwRtlUnicodeStringIsEqual
#endif /* LW_STRICT_NAMESPAE */

#define USE_RTL_STRING_LOG_HACK 1

#ifdef USE_RTL_STRING_LOG_HACK
PCSTR
LwRtlUnicodeStringToLog(
    IN LW_PUNICODE_STRING pString
    );

PCSTR
LwRtlAnsiStringToLog(
    IN LW_PANSI_STRING pString
    );

PCSTR
LwRtlWC16StringToLog(
    IN LW_PCWSTR pszString
    );

#ifndef LW_STRICT_NAMESPACE
#define RtlUnicodeStringToLog LwRtlUnicodeStringToLog
#define RtlAnsiStringToLog LwRtlAnsiStringToLog
#define RtlWC16StringToLog LwRtlWC16StringToLog
#endif /* LW_STRICT_NAMESPAE */

#endif /* USE_RTL_STRING_LOG_HACK */

#endif /* __RTL_STRING_H__ */
