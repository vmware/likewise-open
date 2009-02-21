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
 *        ccb.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        Context Control Block routineus
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 *          Krishna Ganugapati <krishnag@likewise.com>
 */

#include "pvfs.h"

/* Forward declarations */


/* Code */

/*******************************************************
 ******************************************************/

static VOID
PvfsAddRefCCB(
    PPVFS_CCB pCCB
    )
{
    PvfsInterlockedIncrement(&pCCB->cRef);

    return;
}

/***********************************************************
 **********************************************************/

NTSTATUS
PvfsAllocateCCB(
    PPVFS_CCB *ppCCB
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_CCB pCCB = NULL;

    *ppCCB = NULL;

    ntError = PvfsAllocateMemory((PVOID*)&pCCB, sizeof(PVFS_CCB));
    BAIL_ON_NT_STATUS(ntError);


    pthread_mutex_init(&pCCB->FileMutex, NULL);
    pthread_mutex_init(&pCCB->ControlMutex, NULL);

    PvfsInitializeInterlockedCounter(&pCCB->cRef);
    PvfsAddRefCCB(pCCB);

    *ppCCB = pCCB;

    ntError = STATUS_SUCCESS;

cleanup:
    return ntError;

error:
    goto cleanup;
}

/*******************************************************
 ******************************************************/

NTSTATUS
PvfsFreeCCB(
    PPVFS_CCB pCCB
    )
{
    if (pCCB->pDirContext) {
        PvfsFreeDirectoryContext(pCCB->pDirContext);
    }

    PvfsFreeMemory(pCCB->pszFilename);
    PvfsFreeMemory(pCCB);

    return STATUS_SUCCESS;
}


/*******************************************************
 ******************************************************/

VOID
PvfsReleaseCCB(
    PPVFS_CCB pCCB
    )
{
    PvfsInterlockedDecrement(&pCCB->cRef);
    if (PvfsInterlockedCounter(&pCCB->cRef) == 0)
    {
        PvfsFreeCCB(pCCB);
    }

    return;
}


/*******************************************************
 ******************************************************/

NTSTATUS
PvfsAcquireCCB(
    IO_FILE_HANDLE FileHandle,
    PPVFS_CCB * ppCCB
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PPVFS_CCB pCCB = (PPVFS_CCB)IoFileGetContext(FileHandle);

    PVFS_BAIL_ON_INVALID_CCB(pCCB, ntError);

    PvfsAddRefCCB(pCCB);

    *ppCCB = pCCB;

cleanup:
    return ntError;

error:
    *ppCCB = NULL;

    goto cleanup;

}

/*******************************************************
 ******************************************************/

NTSTATUS
PvfsStoreCCB(
    IO_FILE_HANDLE FileHandle,
    PPVFS_CCB pCCB
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;

    ntError = IoFileSetContext(FileHandle, (PVOID)pCCB);
    BAIL_ON_NT_STATUS(ntError);

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
