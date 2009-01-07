#ifndef __IOSTRING_H__
#define __IOSTRING_H__

#include "io-types.h"

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
    IN PSTR pszString
    );

NTSTATUS
IoWC16StringCreateFromCString(
    OUT PWSTR* ppszNewString,
    IN PSTR pszOriginalString
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
