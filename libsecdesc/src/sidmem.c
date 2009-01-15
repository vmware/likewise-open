/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2008
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

/*
 * Authors: Rafal Szczesniak (rafal@likewisesoftware.com)
 */

#include "includes.h"


NTSTATUS
SdAllocateMemory(
    void** ppOut,
    DWORD dwSize
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    void *pOut = NULL;

    BAIL_ON_NULL_PTR_PARAM(ppOut);

    pOut = malloc((size_t)dwSize);
    BAIL_ON_NULL_PTR(pOut);

    memset(pOut, 0, (size_t)dwSize);

    *ppOut = pOut;

cleanup:
    return status;

error:
    *ppOut = NULL;
    goto cleanup;
}


NTSTATUS
SdReallocMemory(
    void **ppOut,
    DWORD dwSize,
    void *pIn
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    void *pOut = NULL;

    BAIL_ON_NULL_PTR_PARAM(ppOut);

    pOut = realloc(pIn, (size_t)dwSize);
    BAIL_ON_NULL_PTR(pOut);

    *ppOut = pOut;

cleanup:
    return status;

error:
    *ppOut = NULL;
    goto error;
}


void
SdFreeMemory(
    void *pIn
    )
{
    free(pIn);
}


void
SdFreeString(
    wchar16_t* sid_str
    )
{
    free(sid_str);
}




/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
