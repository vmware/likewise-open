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

/* Forward declarations */

static PVFS_WILDCARD_TYPE
NextMatchState(
    PCSTR pszPattern
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
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PSTR pszString = NULL;
    PSTR pszMatch  = NULL;
    PSTR pszPathUpper = NULL;
    PSTR pszPatternUpper = NULL;
    BOOLEAN bMatched = FALSE;

    /* Quick check for an exact match */

    if (!strchr(pszPattern, '?') && !strchr(pszPattern, '*'))
    {
        return RtlCStringIsEqual(pszPathname, pszPattern, bCaseSensitive);
    }

    /* If we have a case insensitive search, upper case the
       Pathname and Pattern for easier comparison */

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


    /* Enter state machine */


    for (/* already performed init */;
         PVFS_CSTRING_NON_NULL(pszString) && PVFS_CSTRING_NON_NULL(pszMatch);
         pszString++, pszMatch++)
    {
        PVFS_WILDCARD_TYPE eState = 0;
        CHAR cSrc = *pszString;
        CHAR cMatch = *pszMatch;

        eState = NextMatchState(pszMatch);

        switch (eState) {
        case PVFS_WILDCARD_TYPE_NONE:
            if (cSrc != cMatch) {
                ntError = STATUS_NO_MATCH;
                BAIL_ON_NT_STATUS(ntError);
            }
            break;

        case PVFS_WILDCARD_TYPE_SPLAT:
        {
            PSTR pszCursor = NULL;
            CHAR cNext = *(pszMatch+1);

            /* We are done if this is the last character
               in the pattern */
            if (cNext == '\0') {
                pszString = NULL;
                goto cleanup;
            }

            /* If we don't find a match for the next character
               in the pattern, then fail */
            if ((pszCursor = strchr(pszString, cNext)) == NULL) {
                ntError = STATUS_NO_MATCH;
                BAIL_ON_NT_STATUS(ntError);
            }

            pszString = pszCursor;

            break;
        }


        case PVFS_WILDCARD_TYPE_SINGLE:
            /* automatic single character match */
            break;

        case PVFS_WILDCARD_TYPE_DOT:
            /* For now deal with the '.' as just another character */
            if (cSrc != cMatch) {
                ntError = STATUS_NO_MATCH;
                BAIL_ON_NT_STATUS(ntError);
            }
            break;

        case PVFS_WILDCARD_TYPE_SPLAT_DOT:
            break;

        case PVFS_WILDCARD_TYPE_SINGLE_DOT:
            break;

        }
    }

    /* Save return result */

    bMatched = pszString && *pszString ? FALSE : TRUE;

cleanup:
    if (!bCaseSensitive)
    {
        PVFS_SAFE_FREE_MEMORY(pszPathUpper);
        PVFS_SAFE_FREE_MEMORY(pszPatternUpper);
    }


    /* If we have any string left to parse, we don't
       have a match */

    return bMatched;

error:
    goto cleanup;
}

/********************************************************
 Five trnsitions in state machine are the valie
 wildcard patterns ('*' '?' '*.' '?.' '.' and 'none).
 *******************************************************/

static PVFS_WILDCARD_TYPE
NextMatchState(
    PCSTR pszPattern
    )
{
    CHAR c1,c2;

    c1 = *pszPattern;
    c2 = *(pszPattern+1);

    if (c1 == '.') {
        return PVFS_WILDCARD_TYPE_DOT;
    }

    if (c1 == '*') {
        if (c2 == '.') {
            return PVFS_WILDCARD_TYPE_SPLAT_DOT;
        }

        return PVFS_WILDCARD_TYPE_SPLAT;
    }

    if (c1 == '?') {
        if (c2 == '.') {
            return PVFS_WILDCARD_TYPE_SINGLE_DOT;
        }

        return PVFS_WILDCARD_TYPE_SINGLE;
    }

    return PVFS_WILDCARD_TYPE_NONE;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
