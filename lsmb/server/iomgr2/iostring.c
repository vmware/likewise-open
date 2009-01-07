#include "iop.h"

static
PCSTR
IopStringLogRotate(
    IN OPTIONAL PSTR pszString
    );

NTSTATUS
IoUnicodeStringCreateFromCString(
    OUT PIO_UNICODE_STRING pString,
    IN PCSTR pszString
    )
{
    NTSTATUS status = 0;
    PWSTR pszNewString = NULL;
    IO_UNICODE_STRING newString = { 0 };

    status = IoWC16StringCreateFromCString(&pszNewString, pszString);
    GOTO_CLEANUP_ON_STATUS(status);

    newString.Buffer = pszNewString;
    pszNewString = 0;
    newString.Length = wc16slen(newString.Buffer) * sizeof(newString.Buffer[0]);
    newString.MaximumLength = newString.Length + sizeof(newString.Buffer[0]);

cleanup:
    if (status)
    {
        IO_FREE(&pszNewString);
        IoUnicodeStringFree(&newString);
    }

    *pString = newString;

    return status;
}

NTSTATUS
IoWC16StringCreateFromCString(
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

VOID
IoUnicodeStringInit(
    OUT PIO_UNICODE_STRING pString,
    IN PWSTR pszString
    )
{
    pString->Buffer = pszString;
    pString->Length = pszString ? wc16slen(pszString) * sizeof(pString->Buffer[0]): 0;
    pString->MaximumLength = pString->Length;
}

VOID
IoAnsiStringInit(
    OUT PIO_ANSI_STRING pString,
    IN PSTR pszString
    )
{
    pString->Buffer = pszString;
    pString->Length = pszString ? strlen(pszString) * sizeof(pString->Buffer[0]): 0;
    pString->MaximumLength = pString->Length;
}

VOID
IoUnicodeStringFree(
    IN OUT PIO_UNICODE_STRING pString
    )
{
    IO_FREE(&pString->Buffer);
    pString->Length = pString->MaximumLength = 0;
}

VOID
IoAnsiStringFree(
    IN OUT PIO_ANSI_STRING pString
    )
{
    IO_FREE(&pString->Buffer);
    pString->Length = pString->MaximumLength = 0;
}

NTSTATUS
IoUnicodeStringDuplicate(
    OUT PIO_UNICODE_STRING pNewString,
    IN PIO_UNICODE_STRING pOriginalString
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    IO_UNICODE_STRING newString = { 0 };

    if (!pOriginalString || !pNewString)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }

    if (pOriginalString->Buffer && pOriginalString->Length > 0)
    {
        // Add a NULL anyhow.

        status = IO_ALLOCATE(&newString.Buffer, wchar16_t, pOriginalString->Length + sizeof(pOriginalString->Buffer[0]));
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);

        newString.Length = pOriginalString->Length;
        newString.MaximumLength = pOriginalString->Length + sizeof(pOriginalString->Buffer[0]);

        memcpy(newString.Buffer, pOriginalString->Buffer, pOriginalString->Length);
        newString.Buffer[newString.Length] = 0;
    }

cleanup:
    if (status)
    {
        IoUnicodeStringFree(&newString);
    }

    *pNewString = newString;

    IO_LOG_LEAVE_ON_STATUS_EE(status, EE);
    return status;
}

NTSTATUS
IoAnsiStringDuplicate(
    OUT PIO_ANSI_STRING pNewString,
    IN PIO_ANSI_STRING pOriginalString
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    IO_ANSI_STRING newString = { 0 };

    if (!pOriginalString || !pNewString)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }

    if (pOriginalString->Buffer && pOriginalString->Length > 0)
    {
        // Add a NULL anyhow.

        status = IO_ALLOCATE(&newString.Buffer, CHAR, pOriginalString->Length + sizeof(pOriginalString->Buffer[0]));
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);

        newString.Length = pOriginalString->Length;
        newString.MaximumLength = pOriginalString->Length + sizeof(pOriginalString->Buffer[0]);

        memcpy(newString.Buffer, pOriginalString->Buffer, pOriginalString->Length);
        newString.Buffer[newString.Length] = 0;
    }

cleanup:
    if (status)
    {
        IoAnsiStringFree(&newString);
    }

    *pNewString = newString;

    IO_LOG_LEAVE_ON_STATUS_EE(status, EE);
    return status;
}

NTSTATUS
IoWC16StringDuplicate(
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

    size = wc16slen(pszOriginalString) * sizeof(pszOriginalString[0]);

    status = IO_ALLOCATE(&pszNewString, wchar16_t, size);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    memcpy(pszNewString, pszOriginalString, size);

cleanup:
    if (status)
    {
        IO_FREE(&pszNewString);
    }

    *ppszNewString = pszNewString;

    IO_LOG_LEAVE_ON_STATUS_EE(status, EE);
    return status;
}

NTSTATUS
IoCStringDuplicate(
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

    status = IO_ALLOCATE(&pszNewString, CHAR, size);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    memcpy(pszNewString, pszOriginalString, size);

cleanup:
    if (status)
    {
        IO_FREE(&pszNewString);
    }

    *ppszNewString = pszNewString;

    IO_LOG_LEAVE_ON_STATUS_EE(status, EE);
    return status;
}

BOOLEAN
IoUnicodeStringIsEqual(
    IN PIO_UNICODE_STRING pString1,
    IN PIO_UNICODE_STRING pString2,
    IN BOOLEAN bIsCaseSensitive
    )
{
    BOOLEAN bIsEqual = FALSE;

    // TODO--comparison -- need fix in libunistr...

    if (pString1->Length != pString2->Length)
    {
        GOTO_CLEANUP();
    }
    else if (bIsCaseSensitive)
    {
        ULONG i;
        for (i = 0; i < pString1->Length / sizeof(pString1->Buffer[0]); i++)
        {
            if (pString1->Buffer[i] != pString2->Buffer[i])
            {
                GOTO_CLEANUP();
            }
        }
    }
    else
    {
        ULONG i;
        for (i = 0; i < pString1->Length / sizeof(pString1->Buffer[0]); i++)
        {
            wchar16_t c1[] = { pString1->Buffer[i], 0 };
            wchar16_t c2[] = { pString2->Buffer[i], 0 };
            wc16supper(c1);
            wc16supper(c2);
            if (c1[0] != c2[0])
            {
                GOTO_CLEANUP();
            }
        }
    }

    bIsEqual = TRUE;

cleanup:
    return bIsEqual;
}

// XXX - HACK!!!
#define IOP_STRING_LOG_COUNT 1000

typedef struct _IOP_STRING_LOG_DATA {
    ULONG NextIndex;
    PSTR ppszString[IOP_STRING_LOG_COUNT];
} IOP_STRING_LOG_DATA, *PIOP_STRING_LOG_DATA;

IOP_STRING_LOG_DATA gpIoStringLogData = { 0 };
#define IOP_STRING_NULL_TEXT "<null>"

static
BOOLEAN
IopUnicodeStringIsNullTerminated(
    IN PIO_UNICODE_STRING pString
    )
{
    BOOLEAN bIsNullTermianted = FALSE;

    if (pString &&
        pString->Buffer &&
        (pString->MaximumLength > pString->Length) &&
        !pString->Buffer[pString->Length])
    {
        bIsNullTermianted = TRUE;
    }

    return bIsNullTermianted;
}

static
BOOLEAN
IopAnsiStringIsNullTerminated(
    IN PIO_ANSI_STRING pString
    )
{
    BOOLEAN bIsNullTermianted = FALSE;

    if (pString &&
        pString->Buffer &&
        (pString->MaximumLength > pString->Length) &&
        !pString->Buffer[pString->Length])
    {
        bIsNullTermianted = TRUE;
    }

    return bIsNullTermianted;
}

PCSTR
IoUnicodeStringToLog(
    IN PIO_UNICODE_STRING pString
    )
{
    PCSTR pszOutput = NULL;

    if (IopUnicodeStringIsNullTerminated(pString))
    {
        pszOutput = IoWC16StringToLog(pString->Buffer);
    }
    else
    {
        IO_UNICODE_STRING tempString = { 0 };
        IoUnicodeStringDuplicate(&tempString, pString);
        pszOutput = IoWC16StringToLog(tempString.Buffer);
        IoUnicodeStringFree(&tempString);
    }

    return pszOutput;
}

PCSTR
IoAnsiStringToLog(
    IN PIO_ANSI_STRING pString
    )
{
    PCSTR pszOutput = NULL;

    if (IopAnsiStringIsNullTerminated(pString))
    {
        pszOutput = pString->Buffer;
    }
    else
    {
        IO_ANSI_STRING tempString = { 0 };
        IoAnsiStringDuplicate(&tempString, pString);
        pszOutput = IopStringLogRotate(tempString.Buffer ? strdup(tempString.Buffer) : NULL);
        IoAnsiStringFree(&tempString);
    }

    return pszOutput;
}

PCSTR
IoWC16StringToLog(
    IN PCWSTR pszString
    )
{
    return IopStringLogRotate(pszString ? awc16stombs(pszString) : NULL);
}

static
PCSTR
IopStringLogRotate(
    IN OPTIONAL PSTR pszString
    )
{
    // TODO-Locking or interlocked increment.
    if (gpIoStringLogData.ppszString[gpIoStringLogData.NextIndex])
    {
       free(gpIoStringLogData.ppszString[gpIoStringLogData.NextIndex]);
    }
    gpIoStringLogData.ppszString[gpIoStringLogData.NextIndex] = pszString;
    gpIoStringLogData.NextIndex++;
    if (gpIoStringLogData.NextIndex >= IOP_STRING_LOG_COUNT)
    {
        gpIoStringLogData.NextIndex = 0;
    }
    return pszString ? pszString : IOP_STRING_NULL_TEXT;
}

