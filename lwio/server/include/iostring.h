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

#ifndef __IOSTRING_H__
#define __IOSTRING_H__

#include <lwio/io-types.h>

typedef struct _IO_UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    wchar16_t* Buffer;
} IO_UNICODE_STRING, *PIO_UNICODE_STRING;

typedef struct _IO_ANSI_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PCHAR Buffer;
} IO_ANSI_STRING, *PIO_ANSI_STRING;

VOID
IoUnicodeStringInit(
    OUT PIO_UNICODE_STRING pString,
    IN PWSTR pszString
    );

VOID
IoAnsiStringInit(
    OUT PIO_ANSI_STRING pString,
    IN PSTR pszString
    );

NTSTATUS
IoUnicodeStringCreateFromCString(
    OUT PIO_UNICODE_STRING pString,
    IN PCSTR pszString
    );

NTSTATUS
IoWC16StringCreateFromCString(
    OUT PWSTR* ppszNewString,
    IN PCSTR pszOriginalString
    );

VOID
IoUnicodeStringFree(
    IN OUT PIO_UNICODE_STRING pString
    );

VOID
IoAnsiStringFree(
    IN OUT PIO_ANSI_STRING pString
    );

NTSTATUS
IoUnicodeStringDuplicate(
    OUT PIO_UNICODE_STRING pNewString,
    IN PIO_UNICODE_STRING pOriginalString
    );

NTSTATUS
IoAnsiStringDuplicate(
    OUT PIO_ANSI_STRING pNewString,
    IN PIO_ANSI_STRING pOriginalString
    );

NTSTATUS
IoWC16StringDuplicate(
    OUT PWSTR* ppszNewString,
    IN PCWSTR pszOriginalString
    );

NTSTATUS
IoCStringDuplicate(
    OUT PSTR* ppszNewString,
    IN PCSTR pszOriginalString
    );

BOOLEAN
IoUnicodeStringIsEqual(
    IN PIO_UNICODE_STRING pString1,
    IN PIO_UNICODE_STRING pString2,
    IN BOOLEAN bIsCaseSensitive
    );

PCSTR
IoUnicodeStringToLog(
    IN PIO_UNICODE_STRING pString
    );

PCSTR
IoAnsiStringToLog(
    IN PIO_ANSI_STRING pString
    );

PCSTR
IoWC16StringToLog(
    IN PCWSTR pszString
    );

#endif /* __IOSTRING_H__ */
