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

    ntStatus = RtlUnicodeStringDuplicate(
                    &pFCB->PipeName,
                    pUnicodeString
                    );
    BAIL_ON_NT_STATUS(ntStatus);

    pthread_rwlock_init(&pFCB->PipeListRWLock, NULL);
    NpfsInitializeInterlockedCounter(&pFCB->cRef);

    pFCB->MaxNumberOfInstances = 0xFF;
    // Number of currently available instances
    pFCB->CurrentNumberOfInstances = 0xFF;
    // TODO: This should be the default type
    pFCB->NamedPipeType = FILE_PIPE_MESSAGE_TYPE;

    pFCB->pNext = gpFCB;
    gpFCB = pFCB;

    *ppFCB = pFCB;

    return(ntStatus);

error:

    if (pFCB) {
        NpfsFreeMemory(pFCB);
    }

    *ppFCB = NULL;

    return(ntStatus);
}

NTSTATUS
NpfsFindFCB(
    PUNICODE_STRING pUnicodeString,
    PNPFS_FCB * ppFCB
    )
{
    NTSTATUS ntStatus = 0;
    PNPFS_FCB pFCB = NULL;
    BOOLEAN bEqual = FALSE;

    pFCB = gpFCB;
    while (pFCB) {
         bEqual = RtlUnicodeStringIsEqual(
                            pUnicodeString,
                            &pFCB->PipeName,
                            FALSE
                            );
        if (bEqual) {
            NpfsAddRefFCB(pFCB);
            *ppFCB = pFCB;
            return (ntStatus);
        }
        pFCB = pFCB->pNext;
    }
    ntStatus = STATUS_OBJECT_NAME_NOT_FOUND;
    *ppFCB = NULL;
    return(ntStatus);
}

VOID
NpfsReleaseFCB(
    PNPFS_FCB pFCB
    )
{
    NpfsInterlockedDecrement(&pFCB->cRef);
    if (!NpfsInterlockedCounter(&pFCB->cRef)) {

        NpfsFreeFCB(pFCB);
    }
    return;
}

VOID
NpfsAddRefFCB(
    PNPFS_FCB pFCB
    )
{
    NpfsInterlockedIncrement(&pFCB->cRef);
    return;
}


NTSTATUS
NpfsFreeFCB(
    PNPFS_FCB pFCB
    )
{
    NTSTATUS ntStatus = 0;


    NpfsFreeMemory(pFCB);


    return(ntStatus);
}




