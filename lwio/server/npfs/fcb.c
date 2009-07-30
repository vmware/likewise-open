/*
 * Copyright (c) Likewise Software.  All rights reserved.
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
 * license@likewise.com
 */

#include "npfs.h"


NTSTATUS
NpfsCreateFCB(
    PUNICODE_STRING pUnicodeString,
    PNPFS_FCB * ppFCB
    )
{
    NTSTATUS ntStatus = 0;
    PNPFS_FCB pFCB = NULL;

    ntStatus = NpfsAllocateMemory(
                        sizeof(*pFCB),
                        OUT_PPVOID(&pFCB)
                        );
    BAIL_ON_NT_STATUS(ntStatus);

    LwListInit(&pFCB->link);
    LwListInit(&pFCB->pipeList);

    ntStatus = RtlUnicodeStringDuplicate(
                    &pFCB->PipeName,
                    pUnicodeString
                    );
    BAIL_ON_NT_STATUS(ntStatus);

    pthread_rwlock_init(&pFCB->PipeListRWLock, NULL);

    pFCB->lRefCount = 1;

    pFCB->MaxNumberOfInstances = 0xFF;
    // Number of currently available instances
    pFCB->CurrentNumberOfInstances = 0xFF;
    // TODO: This should be the default type
    pFCB->NamedPipeType = FILE_PIPE_MESSAGE_TYPE;

    LwListInsertBefore(&gFCBList, &pFCB->link);

    *ppFCB = pFCB;

    return ntStatus;

error:

    if (pFCB)
    {
        NpfsFreeMemory(pFCB);
    }

    *ppFCB = NULL;

    return ntStatus;
}

NTSTATUS
NpfsFindFCB(
    PUNICODE_STRING pUnicodeString,
    PNPFS_FCB * ppFCB
    )
{
    NTSTATUS ntStatus = 0;
    PNPFS_FCB pFCB = NULL;
    PLW_LIST_LINKS pLink = NULL;

    for (pLink = gFCBList.Next; pLink != &gFCBList; pLink = pLink->Next)
    {
        pFCB = LW_STRUCT_FROM_FIELD(pLink, NPFS_FCB, link);

        if (RtlUnicodeStringIsEqual(pUnicodeString, &pFCB->PipeName, FALSE))
        {
            NpfsAddRefFCB(pFCB);
            *ppFCB = pFCB;
            goto cleanup;
        }
    }

    ntStatus = STATUS_OBJECT_NAME_NOT_FOUND;
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    return(ntStatus);

error:

    *ppFCB = NULL;

    goto cleanup;
}

VOID
NpfsReleaseFCB(
    PNPFS_FCB pFCB
    )
{
    BOOLEAN bFreeFCB = FALSE;

    ENTER_WRITER_RW_LOCK(&gServerLock);

    if (InterlockedDecrement(&pFCB->lRefCount) == 0)
    {
        LwListRemove(&pFCB->link);
        bFreeFCB = TRUE;
    }

    LEAVE_WRITER_RW_LOCK(&gServerLock);

    if (bFreeFCB)
    {
        NpfsFreeFCB(pFCB);
    }

    return;
}

VOID
NpfsAddRefFCB(
    PNPFS_FCB pFCB
    )
{
    InterlockedIncrement(&pFCB->lRefCount);
}


NTSTATUS
NpfsFreeFCB(
    PNPFS_FCB pFCB
    )
{
    NTSTATUS ntStatus = 0;

    pthread_rwlock_destroy(&pFCB->PipeListRWLock);
    LwRtlUnicodeStringFree(&pFCB->PipeName);

    NpfsFreeMemory(pFCB);

    return(ntStatus);
}
