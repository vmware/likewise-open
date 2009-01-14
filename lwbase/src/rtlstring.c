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

#define RTL_LOG_LEAVE_ON_STATUS_EE(status, EE)

NTSTATUS
LwRtlUnicodeStringCreateFromCString(
    OUT PUNICODE_STRING pString,
    IN PCSTR pszString
    )
{
    NTSTATUS status = 0;
    PWSTR pszNewString = NULL;
    UNICODE_STRING newString = { 0 };

    status = RtlWC16StringCreateFromCString(&pszNewString, pszString);
    GOTO_CLEANUP_ON_STATUS(status);

    newString.Buffer = pszNewString;
    pszNewString = 0;
    newString.Length = wc16slen(newString.Buffer) * sizeof(newString.Buffer[0]);
    newString.MaximumLength = newString.Length + sizeof(newString.Buffer[0]);

cleanup:
    if (status)
    {
        RTL_FREE(&pszNewString);
        RtlUnicodeStringFree(&newString);
    }

    *pString = newString;

    return status;
}

NTSTATUS
LwRtlWC16StringCreateFromCString(
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
LwRtlUnicodeStringInit(
    OUT PUNICODE_STRING pString,
    IN PWSTR pszString
    )
{
    pString->Buffer = pszString;
    pString->Length = pszString ? wc16slen(pszString) * sizeof(pString->Buffer[0]): 0;
    pString->MaximumLength = pString->Length;
}

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

VOID
LwRtlUnicodeStringFree(
    IN OUT PUNICODE_STRING pString
    )
{
    RTL_FREE(&pString->Buffer);
    pString->Length = pString->MaximumLength = 0;
}

VOID
LwRtlAnsiStringFree(
    IN OUT PANSI_STRING pString
    )
{
    RTL_FREE(&pString->Buffer);
    pString->Length = pString->MaximumLength = 0;
}

NTSTATUS
LwRtlUnicodeStringDuplicate(
    OUT PUNICODE_STRING pNewString,
    IN PUNICODE_STRING pOriginalString
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    UNICODE_STRING newString = { 0 };

    if (!pOriginalString || !pNewString)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }

    if (pOriginalString->Buffer && pOriginalString->Length > 0)
    {
        // Add a NULL anyhow.

        status = RTL_ALLOCATE(&newString.Buffer, wchar16_t, pOriginalString->Length + sizeof(pOriginalString->Buffer[0]));
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);

        newString.Length = pOriginalString->Length;
        newString.MaximumLength = pOriginalString->Length + sizeof(pOriginalString->Buffer[0]);

        memcpy(newString.Buffer, pOriginalString->Buffer, pOriginalString->Length);
        newString.Buffer[newString.Length] = 0;
    }

cleanup:
    if (status)
    {
        RtlUnicodeStringFree(&newString);
    }

    *pNewString = newString;

    RTL_LOG_LEAVE_ON_STATUS_EE(status, EE);
    return status;
}

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

        status = RTL_ALLOCATE(&newString.Buffer, CHAR, pOriginalString->Length + sizeof(pOriginalString->Buffer[0]));
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);

        newString.Length = pOriginalString->Length;
        newString.MaximumLength = pOriginalString->Length + sizeof(pOriginalString->Buffer[0]);

        memcpy(newString.Buffer, pOriginalString->Buffer, pOriginalString->Length);
        newString.Buffer[newString.Length] = 0;
    }

cleanup:
    if (status)
    {
        RtlAnsiStringFree(&newString);
    }

    *pNewString = newString;

    RTL_LOG_LEAVE_ON_STATUS_EE(status, EE);
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

    size = (wc16slen(pszOriginalString) + 1) * sizeof(pszOriginalString[0]);

    status = RTL_ALLOCATE(&pszNewString, wchar16_t, size);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    memcpy(pszNewString, pszOriginalString, size);

cleanup:
    if (status)
    {
        RTL_FREE(&pszNewString);
    }

    *ppszNewString = pszNewString;

    RTL_LOG_LEAVE_ON_STATUS_EE(status, EE);
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
        RTL_FREE(&pszNewString);
    }

    *ppszNewString = pszNewString;

    RTL_LOG_LEAVE_ON_STATUS_EE(status, EE);
    return status;
}

BOOLEAN
LwRtlUnicodeStringIsEqual(
    IN PUNICODE_STRING pString1,
    IN PUNICODE_STRING pString2,
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
#define RTLP_STRING_LOG_COUNT 1000

typedef struct _RTLP_STRING_LOG_DATA {
    ULONG NextIndex;
    PSTR ppszString[RTLP_STRING_LOG_COUNT];
} RTLP_STRING_LOG_DATA, *PRTLP_STRING_LOG_DATA;

RTLP_STRING_LOG_DATA gpLwRtlStringLogData = { 0 };
#define RTLP_STRING_NULL_TEXT "<null>"

static
BOOLEAN
LwRtlpUnicodeStringIsNullTerminated(
    IN PUNICODE_STRING pString
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
LwRtlpAnsiStringIsNullTerminated(
    IN PANSI_STRING pString
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
PCSTR
LwRtlpStringLogRotate(
    IN OPTIONAL PSTR pszString
    )
{
    // TODO-Locking or interlocked increment.
    if (gpLwRtlStringLogData.ppszString[gpLwRtlStringLogData.NextIndex])
    {
       free(gpLwRtlStringLogData.ppszString[gpLwRtlStringLogData.NextIndex]);
    }
    gpLwRtlStringLogData.ppszString[gpLwRtlStringLogData.NextIndex] = pszString;
    gpLwRtlStringLogData.NextIndex++;
    if (gpLwRtlStringLogData.NextIndex >= RTLP_STRING_LOG_COUNT)
    {
        gpLwRtlStringLogData.NextIndex = 0;
    }
    return pszString ? pszString : RTLP_STRING_NULL_TEXT;
}

PCSTR
LwRtlUnicodeStringToLog(
    IN PUNICODE_STRING pString
    )
{
    PCSTR pszOutput = NULL;

    if (LwRtlpUnicodeStringIsNullTerminated(pString))
    {
        pszOutput = RtlWC16StringToLog(pString->Buffer);
    }
    else
    {
        UNICODE_STRING tempString = { 0 };
        RtlUnicodeStringDuplicate(&tempString, pString);
        pszOutput = RtlWC16StringToLog(tempString.Buffer);
        RtlUnicodeStringFree(&tempString);
    }

    return pszOutput;
}

PCSTR
LwRtlAnsiStringToLog(
    IN PANSI_STRING pString
    )
{
    PCSTR pszOutput = NULL;

    if (LwRtlpAnsiStringIsNullTerminated(pString))
    {
        pszOutput = pString->Buffer;
    }
    else
    {
        ANSI_STRING tempString = { 0 };
        RtlAnsiStringDuplicate(&tempString, pString);
        pszOutput = LwRtlpStringLogRotate(tempString.Buffer ? strdup(tempString.Buffer) : NULL);
        RtlAnsiStringFree(&tempString);
    }

    return pszOutput;
}

PCSTR
LwRtlWC16StringToLog(
    IN PCWSTR pszString
    )
{
    return LwRtlpStringLogRotate(pszString ? awc16stombs(pszString) : NULL);
}

