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
 *        alloc.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        Pvfs Memory allocation routines
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "pvfs.h"

/***********************************************************
 **********************************************************/

NTSTATUS
PvfsAllocateMemory(
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
    return RTL_ALLOCATE(ppBuffer, VOID, dwSize);
}


/***********************************************************
 **********************************************************/

NTSTATUS
PvfsReallocateMemory(
    IN OUT PVOID *ppBuffer,
    IN DWORD dwNewSize
    )
{
    NTSTATUS ntError = STATUS_INSUFFICIENT_RESOURCES;
    PVOID pBuffer = *ppBuffer;
    PVOID pNewBuffer;

    if (dwNewSize <= 0) {
        return STATUS_INVALID_PARAMETER;
    }

    /* Check for a simple malloc() */

    if (pBuffer == NULL)
    {
        return PvfsAllocateMemory(ppBuffer, dwNewSize);
    }


    if ((pNewBuffer = RtlMemoryRealloc(pBuffer, dwNewSize)) != NULL)
    {
        *ppBuffer = pNewBuffer;
        ntError = STATUS_SUCCESS;
    }

    return ntError;
}

/***********************************************************
 **********************************************************/
VOID
PvfsFreeMemory(
    IN OUT PVOID *ppBuffer
    )
{
    RTL_FREE(ppBuffer);
}

/***********************************************************
 **********************************************************/

NTSTATUS
PvfsFreeIrpContext(
	PPVFS_IRP_CONTEXT *ppIrpContext
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;

    if (ppIrpContext && *ppIrpContext)
    {
        pthread_mutex_destroy(&(*ppIrpContext)->Mutex);
        pthread_cond_destroy(&(*ppIrpContext)->Event);

        PVFS_FREE(ppIrpContext);
    }

    return ntError;
}

/***********************************************************
 **********************************************************/

NTSTATUS
PvfsAllocateIrpContext(
	PPVFS_IRP_CONTEXT *ppIrpContext,
    PIRP pIrp
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_IRP_CONTEXT pIrpContext = NULL;

    *ppIrpContext = NULL;

    ntError = PvfsAllocateMemory((PVOID*)&pIrpContext,
                                 sizeof(PVFS_IRP_CONTEXT));
    BAIL_ON_NT_STATUS(ntError);

    pthread_mutex_init(&pIrpContext->Mutex, NULL);
    pthread_cond_init(&pIrpContext->Event, NULL);

    pIrpContext->ntError = STATUS_PENDING;
    pIrpContext->bFinished = FALSE;

    pIrpContext->pIrp = pIrp;

    *ppIrpContext = pIrpContext;

cleanup:
    return ntError;

error:
    goto cleanup;
}



/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
