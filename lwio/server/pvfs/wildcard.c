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
 *        wildcard.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        FindFirst Wildcard matching algorithm
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "pvfs.h"

typedef enum _PVFS_WILDCARD_TYPE {
    PVFS_WILDCARD_TYPE_NONE = 0,
    PVFS_WILDCARD_TYPE_SPLAT,
	PVFS_WILDCARD_TYPE_SINGLE,
    PVFS_WILDCARD_TYPE_DOT,
    PVFS_WILDCARD_TYPE_SINGLE_DOT,
    PVFS_WILDCARD_TYPE_SPLAT_DOT
} PVFS_WILDCARD_TYPE;

typedef struct _PVFS_WILDCARD_STATE_ENTRY
{
    LW_LIST_LINKS StackLinks;
    PSTR pszPattern;
    PSTR pszInputString;

} PVFS_WILDCARD_STATE_ENTRY, *PPVFS_WILDCARD_STATE_ENTRY;

/* Forward declarations */

static
BOOLEAN
PvfsCStringOnlyContainsChars(
    IN PCSTR pszString,
    IN PCSTR pszAllowedCharacters
    );

static
PSTR
PvfsWildcardEatString(
    PSTR pszString,
    PSTR pszMatch
    );

static
PVFS_WILDCARD_TYPE
NextMatchState(
    PSTR *ppszPattern,
    PDWORD pdwCount
    );

static
NTSTATUS
PvfsWildcardStackPush(
    IN OUT PLW_LIST_LINKS pStack,
    IN PSTR pszInputString,
    IN PSTR pszPattern
    );

static
NTSTATUS
PvfsWildcardStackPop(
    IN OUT PLW_LIST_LINKS pStack,
    OUT PSTR *ppszInputString,
    OUT PSTR *ppszPattern
    );

static
VOID
PvfsWildcardStackDestroy(
    PLW_LIST_LINKS pStack
    );


/* Code */

/********************************************************
 Useful post to the Win32.programmer.kernel group
 http://groups.google.com/group/microsoft.public.win32.programmer.kernel/browse_thread/thread/0049c8dc7ce65149/e13654b08ffb5f01?hl=en&#e13654b08ffb5f01
 *******************************************************/

BOOLEAN
PvfsWildcardMatch(
    IN PSTR pszPathname,
    IN PSTR pszPattern,
    IN BOOLEAN bCaseSensitive
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PSTR pszString = NULL;
    PSTR pszMatch  = NULL;
    PSTR pszPathUpper = NULL;
    PSTR pszPatternUpper = NULL;
    BOOLEAN bMatched = FALSE;
    LW_LIST_LINKS Stack;

    // Quick check for an exact match

    if (!strchr(pszPattern, '?') && !strchr(pszPattern, '*'))
    {
        return RtlCStringIsEqual(pszPathname, pszPattern, bCaseSensitive);
    }

    // If we have a case insensitive search, upper case the Pathname
    // and Pattern for easier comparison

    pszString = pszPathname;
    pszMatch = pszPattern;

    if (!bCaseSensitive) {
        ntError = RtlCStringDuplicate(&pszPathUpper, pszPathname);
        BAIL_ON_NT_STATUS(ntError);

        ntError = RtlCStringDuplicate(&pszPatternUpper, pszPattern);
        BAIL_ON_NT_STATUS(ntError);

        PvfsCStringUpper(pszPathUpper);
        PvfsCStringUpper(pszPatternUpper);

        pszString = pszPathUpper;
        pszMatch  = pszPatternUpper;
    }


    // Enter state machine

    LwListInit(&Stack);

reset_state:
    while (PVFS_CSTRING_NON_NULL(pszString) && PVFS_CSTRING_NON_NULL(pszMatch))
    {
        PVFS_WILDCARD_TYPE eState = 0;
        CHAR cSrc = '\0';
        CHAR cMatch = '\0';
        DWORD dwCount = 0;

        // Save the current CHAR

        cSrc = *pszString;
        cMatch = *pszMatch;

        // Consumes the pattern from pszMatch

        eState = NextMatchState(&pszMatch, &dwCount);

        switch (eState)
        {
        case PVFS_WILDCARD_TYPE_NONE:
            if (cSrc != cMatch)
            {
                ntError = STATUS_NO_MATCH;
                pszString = NULL;
            }
            else
            {
                pszString++;
            }
            break;

        case PVFS_WILDCARD_TYPE_SPLAT:
        {
            // We are done if this is the last character in the pattern
            if (!PVFS_CSTRING_NON_NULL(pszMatch))
            {
                pszString = NULL;
            }
            else
            {
                // Be greedy - Consume as much of the string using the '*'
                // as possible.  This will require a stack in order to backtrack
                // on a failure.
                pszString = PvfsWildcardEatString(pszString, pszMatch);
                if (!pszString)
                {
                    ntError = STATUS_NO_MATCH;
                }
                else
                {
                    // Add next character past "String" match and the previously
                    // match "Pattern" state (so we pick up the '*' when we pop
                    // the stack
                    ntError = PvfsWildcardStackPush(
                                  &Stack,
                                  pszString+1,
                                  pszMatch-1);
                    BAIL_ON_NT_STATUS(ntError);
                }
            }
            break;
        }

        case PVFS_WILDCARD_TYPE_SINGLE:
        {
            DWORD i = 0;

            // Consume dwCount characters
            for (i=0;
                 (i<dwCount) && PVFS_CSTRING_NON_NULL(pszString);
                 i++, pszString++)
            {
                // no loop body
                ;
            }
            break;
        }

        case PVFS_WILDCARD_TYPE_DOT:
            // For now deal with the '.' as just another character
            if (cSrc != cMatch)
            {
                ntError = STATUS_NO_MATCH;
                pszString = NULL;
            }
            else
            {
                pszString++;
            }

            break;

        case PVFS_WILDCARD_TYPE_SPLAT_DOT:
        {
            PSTR pszCursor = NULL;

            // Similar to "A*B" except we search for the '.' from the end

            if ((pszCursor = strrchr(pszString, '.')) == NULL)
            {
                ntError = STATUS_NO_MATCH;
                pszString = NULL;
            }
            else
            {
                pszString = pszCursor + 1;
            }
            break;
        }

        case PVFS_WILDCARD_TYPE_SINGLE_DOT:
        {
            DWORD i = 0;

            // We can match 0 - dwCount characters up to the last '.'
            // This is really a hold over from DOS 8.3 filenames

            for (i=0;
                 i<dwCount && PVFS_CSTRING_NON_NULL(pszString) && (*pszString != '.');
                 i++, pszString++)
            {
                // no loop body
                ;
            }

            // If we any path left, it better be on '.' for a match

            if (*pszString == '.')
            {
                pszString++;
            }
            else
            {
                ntError = STATUS_NO_MATCH;
                pszString = NULL;
            }

            break;
        }

        }    // end of switch {...}
    }        // end of for {...}

    // We matched if pszString is empty AND either pszMatch is empty
    // OR only contains wildcard characters

    if ((ntError == STATUS_SUCCESS) &&
        !PVFS_CSTRING_NON_NULL(pszString) &&
        (!PVFS_CSTRING_NON_NULL(pszMatch) ||
         PvfsCStringOnlyContainsChars(pszMatch, "?*")))
    {
        bMatched = TRUE;
    }
    else if (!LwListIsEmpty(&Stack))
    {
        // Pop back to the earlier state, consume one character (*from '*')
        // and try another path
        ntError = PvfsWildcardStackPop(&Stack, &pszString, &pszMatch);
        BAIL_ON_NT_STATUS(ntError);

        pszString++;
        goto reset_state;
    }


cleanup:

    PvfsWildcardStackDestroy(&Stack);

    if (!bCaseSensitive)
    {
        PVFS_FREE(&pszPathUpper);
        PVFS_FREE(&pszPatternUpper);
    }

    // If we have any string left to parse, we don't have a match

    return bMatched;

error:
    goto cleanup;
}

/***********************************************************************
************************************************************************/

static
BOOLEAN
PvfsCStringOnlyContainsChars(
    IN PCSTR pszString,
    IN PCSTR pszAllowedCharacters
    )
{
    BOOLEAN bResult = TRUE;
    PCSTR pszInputCursor = pszString;
    PCSTR pszMatchCursor = pszAllowedCharacters;

    for (pszInputCursor = pszString;
         *pszInputCursor && bResult;
         pszInputCursor++)
    {
        // Guilty unilt proven innocent
        bResult = FALSE;

        // Must check all allowed characters until we get a
        // single match or have compared them all
        for (pszMatchCursor = pszAllowedCharacters;
             *pszMatchCursor;
             pszMatchCursor++)
        {
            if (*pszInputCursor ==*pszMatchCursor)
            {
                bResult = TRUE;
                break;
            }
        }
    }

    return bResult;
}


/***********************************************************************
 Five transitions in state machine are the value wildcard patterns ('*'
 '?' '*.' '?.' '.' and 'none).  Consume the next token from the Pattern.
************************************************************************/

static
PVFS_WILDCARD_TYPE
NextMatchState(
    PSTR *ppszPattern,
    PDWORD pdwCount
    )
{
    PVFS_WILDCARD_TYPE Type = PVFS_WILDCARD_TYPE_NONE;
    PVFS_WILDCARD_TYPE InterimType = PVFS_WILDCARD_TYPE_NONE;
    CHAR c1 = '\0';
    PSTR pszPattern = *ppszPattern;
    PSTR pszCursor = NULL;
    DWORD dwInterimCount = 0;
    DWORD dwCount = 0;

    c1 = *pszPattern;

    /* We always consume at least one CHAR */

    pszPattern++;

    switch (c1)
    {
    case '.':
        Type = PVFS_WILDCARD_TYPE_DOT;
        break;

    case '*':
        /* We have to consume up to the next non-wildcard character
           or the end of string */

        pszCursor = pszPattern;
        InterimType = NextMatchState(&pszCursor, &dwInterimCount);

        switch (InterimType) {
        case PVFS_WILDCARD_TYPE_NONE:
            Type = PVFS_WILDCARD_TYPE_SPLAT;
            break;
        case PVFS_WILDCARD_TYPE_SPLAT:
        case PVFS_WILDCARD_TYPE_SINGLE:
            pszPattern = pszCursor;
            Type = PVFS_WILDCARD_TYPE_SPLAT;
            break;
        case PVFS_WILDCARD_TYPE_DOT:
        case PVFS_WILDCARD_TYPE_SPLAT_DOT:
        case PVFS_WILDCARD_TYPE_SINGLE_DOT:
            /* "**." and "*?." just becomes "*." */
            pszPattern = pszCursor;
            Type = PVFS_WILDCARD_TYPE_SPLAT_DOT;
            break;
        }
        break;

    case '?':
        /* Same as "*".  Consume up to the next non-wildcard character
           or the end of string */

        pszCursor = pszPattern;
        InterimType = NextMatchState(&pszCursor, &dwInterimCount);

        switch (InterimType) {
        case PVFS_WILDCARD_TYPE_SPLAT_DOT:
        case PVFS_WILDCARD_TYPE_SPLAT:
        case PVFS_WILDCARD_TYPE_NONE:
            dwCount = 1;
            Type = PVFS_WILDCARD_TYPE_SINGLE;
            break;

        case PVFS_WILDCARD_TYPE_DOT:
            dwCount = 1;
            pszPattern = pszCursor;
            Type = PVFS_WILDCARD_TYPE_SINGLE_DOT;
            break;

        case PVFS_WILDCARD_TYPE_SINGLE:
            dwCount = dwInterimCount + 1;
            pszPattern = pszCursor;
            Type = PVFS_WILDCARD_TYPE_SINGLE;
            break;

        case PVFS_WILDCARD_TYPE_SINGLE_DOT:
            dwCount = dwInterimCount + 1;
            pszPattern = pszCursor;
            Type = PVFS_WILDCARD_TYPE_SINGLE_DOT;
            break;
        }
        break;

    default:
        Type = PVFS_WILDCARD_TYPE_NONE;
        break;
    }

    *pdwCount = dwCount;
    *ppszPattern = pszPattern;

    return Type;
}


/***********************************************************************
 **********************************************************************/

static
PSTR
PvfsWildcardEatString(
    PSTR pszString,
    PSTR pszMatch
    )
{
    PSTR pszMatchCursor = NULL;
    PSTR pszStringCursor = pszString;
    CHAR cSaved = '\0';

    // Find the first non-wildcard character in the patternbt

    pszMatchCursor = pszMatch;
    while (*pszMatchCursor)
    {
        switch (*pszMatchCursor)
        {
        case '*':
        case '?':
            cSaved = *pszMatchCursor;
            *pszMatchCursor = '\0';
            break;
        default:
            pszMatchCursor++;
            break;
        }
    }


    if (*pszMatch != '\0')
    {
        // We have any non-wildcard pattern to match, find the needle
        // in the haystack
#if 0
        while (pszString)
        {
            pszString = strstr(pszString, pszMatch);
            if (pszString && *pszString)
            {
                pszStringCursor = pszString;
                pszString++;
            }
        }
#else
        pszStringCursor = strstr(pszString, pszMatch);
#endif
    }

    *pszMatchCursor = cSaved;

    return pszStringCursor;
}

/***********************************************************************
 **********************************************************************/

static
NTSTATUS
PvfsWildcardStackPush(
    IN OUT PLW_LIST_LINKS pStack,
    IN PSTR pszInputString,
    IN PSTR pszPattern
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PPVFS_WILDCARD_STATE_ENTRY pState = NULL;

    ntError = PvfsAllocateMemory((PVOID*)&pState, sizeof(*pState), TRUE);
    BAIL_ON_NT_STATUS(ntError);

    pState->pszInputString = pszInputString;
    pState->pszPattern = pszPattern;

    LwListInsertAfter(pStack, &pState->StackLinks);

cleanup:
    return ntError;

error:
    goto cleanup;
}

/***********************************************************************
 **********************************************************************/

static
NTSTATUS
PvfsWildcardStackPop(
    IN OUT PLW_LIST_LINKS pStack,
    OUT PSTR *ppszInputString,
    OUT PSTR *ppszPattern
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PLW_LIST_LINKS pStateLink = NULL;
    PPVFS_WILDCARD_STATE_ENTRY pState = NULL;

    pStateLink = LwListRemoveAfter(pStack);

    pState = LW_STRUCT_FROM_FIELD(
                 pStateLink,
                 PVFS_WILDCARD_STATE_ENTRY,
                 StackLinks);

    *ppszInputString = pState->pszInputString;
    *ppszPattern     = pState->pszPattern;

    PvfsFreeMemory((PVOID*)&pState);

    return ntError;
}

/***********************************************************************
 **********************************************************************/

static
VOID
PvfsWildcardStackDestroy(
    PLW_LIST_LINKS pStack
    )
{
    PSTR a = NULL;
    PSTR b = NULL;

    while (!LwListIsEmpty(pStack))
    {
        PvfsWildcardStackPop(pStack, &a, &b);
    }
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
