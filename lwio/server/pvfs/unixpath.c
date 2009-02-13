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
 *        create_file.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *       Create Dispatch Routine
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Gerald Carter <gcarter@likewise.com>
 */

#include "pvfs.h"

/* Forward declarations */


/* Code */


/********************************************************
 *******************************************************/

NTSTATUS
PvfsCanonicalPathName(
    PSTR *ppszPath,
    IO_FILE_NAME IoPath
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;    
    PSTR pszPath = NULL;
    PSTR pszCursor = NULL;    

    ntError = RtlCStringAllocateFromWC16String(&pszPath,
                                               IoPath.FileName);
    BAIL_ON_NT_STATUS(ntError);

    pszCursor = pszPath;
    while (pszCursor && *pszCursor)
    {
        if (*pszCursor == '\\') {
            *pszCursor = '/';
        }
        pszCursor++;        
    }
    

    *ppszPath = pszPath;

    ntError = STATUS_SUCCESS;    

cleanup:
    return ntError;
    
error:
    goto cleanup;
}


/********************************************************
 *******************************************************/

BOOLEAN
PvfsWildcardMatch(
    IN PSTR pszPathname,
    IN PSTR pszPattern,
    IN BOOLEAN bCaseSensitive
    )
{
    /* Everything matches '*' */
    if (RtlCStringIsEqual(pszPattern, "*", FALSE)) {
        return TRUE;
    }

    /* Quick check for an exact match */
    if (!strchr(pszPattern, '?') && !strchr(pszPattern, '*'))
    {
        return RtlCStringIsEqual(pszPathname, pszPattern, bCaseSensitive);
    }

    /* Let everything else match for now */

    return TRUE;
}



/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
