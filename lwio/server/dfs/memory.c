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
 *        memory.c
 *
 * Abstract:
 *
 *        Likewise Distributed File System Driver (DFS)
 *
 *        Memory allocation routines
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "dfs.h"


/***********************************************************************
 **********************************************************************/

NTSTATUS
DfsAllocateMemory(
    OUT PVOID *ppBuffer,
    IN DWORD dwSize
    )
{
    /* No op */

    if (dwSize == 0)
    {
        *ppBuffer = NULL;
        return STATUS_SUCCESS;
    }

    /* Real work */

    return LW_RTL_ALLOCATE(ppBuffer, VOID, dwSize);
}


/***********************************************************************
 **********************************************************************/

NTSTATUS
DfsReallocateMemory(
    IN OUT PVOID *ppBuffer,
    IN DWORD dwNewSize
    )
{
    NTSTATUS ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    PVOID pBuffer = *ppBuffer;
    PVOID pNewBuffer;

    if (dwNewSize <= 0)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Check for a simple malloc() */

    if (pBuffer == NULL)
    {
        return DfsAllocateMemory(ppBuffer, dwNewSize);
    }


    if ((pNewBuffer = LwRtlMemoryRealloc(pBuffer, dwNewSize)) != NULL)
    {
        *ppBuffer = pNewBuffer;
        ntStatus = STATUS_SUCCESS;
    }

    return ntStatus;
}



/***********************************************************************
 **********************************************************************/

VOID
DfsFreeMemory(
    IN OUT PVOID *ppBuffer
    )
{
    LW_RTL_FREE(ppBuffer);
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
