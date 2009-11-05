/*
 * Copyright (c) Likewise Software
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */
#include "includes.h"


DWORD
LwiTdbOpen(
    PCSTR pszPath,
    PTDB_CONTEXT *ppTdb
    )
{
    DWORD dwError = 0;
    PTDB_CONTEXT pTdb = NULL;

    pTdb = tdb_open(pszPath, 0, TDB_DEFAULT, O_RDONLY, 0400);
    if (!pTdb) 
    {
        // Doesn't look like tdb_open has anyway to return an error.
        dwError = LwMapErrnoToLwError(EACCES);
    }
    BAIL_ON_UP_ERROR(dwError);

    *ppTdb = pTdb;

cleanup:

    return dwError;

error:
    goto cleanup;
}

VOID
LwiTdbClose(
    PTDB_CONTEXT pTdb
    )
{
    if (pTdb) 
    {
        tdb_close(pTdb);
        pTdb = NULL;
    }
}

DWORD
LwiTdbAllocateFetch(
    PTDB_CONTEXT pTdb,
    PCSTR pszKey,
    PVOID *ppResult,
    PDWORD pdwSize)
{
    DWORD dwError = 0;
    PVOID pResult = NULL;
    TDB_DATA result;
    TDB_DATA key;

    key.dptr = (PSTR)pszKey;
    key.dsize = strlen(pszKey);

    result = tdb_fetch(pTdb, key);
    if (result.dsize)
    {
        dwError = LwAllocateMemory(result.dsize, &pResult);
        BAIL_ON_UP_ERROR(dwError);

        memcpy(pResult, result.dptr, result.dsize);
    }

    *ppResult = pResult;
    if (pdwSize)
    {
        *pdwSize = (DWORD) result.dsize;
    }

cleanup:
    free(result.dptr);

    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pResult);
    goto cleanup;
}

