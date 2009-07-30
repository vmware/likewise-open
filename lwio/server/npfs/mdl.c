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

#define min(a,b) (a <= b)? a:b

NTSTATUS
NpfsEnqueueBuffer(
    PLW_LIST_LINKS pMdlList,
    PVOID pBuffer,
    ULONG Length,
    PULONG pulBytesTransferred
    )
{
    NTSTATUS ntStatus = 0;
    PNPFS_MDL pMdl = NULL;

    ntStatus = NpfsCreateMdl(Length, pBuffer, &pMdl);
    BAIL_ON_NT_STATUS(ntStatus);

    NpfsEnqueueMdl(pMdlList, pMdl);

    *pulBytesTransferred = Length;

cleanup:

    return ntStatus;

error:

    if (pMdl)
    {
        NpfsFreeMdl(pMdl);
    }

    *pulBytesTransferred = 0;

    goto cleanup;
}

NTSTATUS
NpfsDequeueBuffer(
    PLW_LIST_LINKS pMdlList,
    PVOID pBuffer,
    ULONG Length,
    PULONG pulBytesTransferred
    )
{
    NTSTATUS ntStatus = 0;
    ULONG LengthRemaining = 0;
    ULONG BytesAvail = 0;
    ULONG BytesToCopy = 0;
    ULONG BytesCopied = 0;
    PNPFS_MDL pMdl = NULL;

    if (!pMdlList)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    LengthRemaining = Length;
    while (LengthRemaining && !NpfsMdlListIsEmpty(pMdlList))
    {
        pMdl = LW_STRUCT_FROM_FIELD(pMdlList->Next, NPFS_MDL, link);
        BytesAvail = pMdl->Length - pMdl->Offset;
        BytesToCopy = min(BytesAvail, LengthRemaining);
        memcpy(pBuffer, pMdl->Buffer + pMdl->Offset, BytesToCopy);
        BytesCopied += BytesToCopy;
        pMdl->Offset += BytesToCopy;
        LengthRemaining -= BytesToCopy;
        if (pMdl->Length - pMdl->Offset == 0)
        {
            NpfsDequeueMdl(pMdlList, &pMdl);
            NpfsFreeMdl(pMdl);
        }
    }

    *pulBytesTransferred = BytesCopied;

cleanup:

    return ntStatus;

error:

    *pulBytesTransferred = 0;

    goto cleanup;
}

NTSTATUS
NpfsCreateMdl(
    ULONG Length,
    PVOID pBuffer,
    PNPFS_MDL * ppMdl
    )
{
    NTSTATUS ntStatus = 0;
    PVOID pTargBuffer = NULL;
    PNPFS_MDL pMdl = NULL;

    ntStatus = NpfsAllocateMemory(
                    Length,
                    &pTargBuffer
                    );
    BAIL_ON_NT_STATUS(ntStatus);

    memcpy(pTargBuffer, pBuffer, Length);

    ntStatus = NpfsAllocateMemory(
                    sizeof(*pMdl),
                    OUT_PPVOID(&pMdl)
                    );
    BAIL_ON_NT_STATUS(ntStatus);

    pMdl->Buffer = pTargBuffer;
    pMdl->Length = Length;

    *ppMdl = pMdl;

    return(ntStatus);

error:

    if(pTargBuffer) {
        NpfsFreeMemory(pTargBuffer);
    }

    if (pMdl) {
        NpfsFreeMemory(pMdl);
    }

    *ppMdl = NULL;
    return(ntStatus);
}

VOID
NpfsEnqueueMdl(
    PLW_LIST_LINKS pMdlList,
    PNPFS_MDL pMdl
    )
{
    LwListInsertBefore(pMdlList, &pMdl->link);
}

VOID
NpfsDequeueMdl(
    PLW_LIST_LINKS pMdlList,
    PNPFS_MDL* ppMdl
    )
{
    PNPFS_MDL pMdl = NULL;
    PLW_LIST_LINKS pLink = NULL;

    pLink = LwListRemoveAfter(pMdlList);
    pMdl = LW_STRUCT_FROM_FIELD(pLink, NPFS_MDL, link);

    *ppMdl = pMdl;
}

NTSTATUS
NpfsCopyMdl(
    PNPFS_MDL pMdl,
    PVOID pBuffer,
    ULONG Length,
    ULONG *ppLengthCopied
    )
{
    NTSTATUS ntStatus = 0;

    return ntStatus;

}

VOID
NpfsFreeMdl(
    PNPFS_MDL pMdl
    )
{
    if (pMdl->Buffer)
    {
        NpfsFreeMemory(pMdl->Buffer);
    }

    NpfsFreeMemory(pMdl);

    return;
}



NTSTATUS
NpfsAddMdltoInboundQueue(
    PNPFS_CCB pCCB,
    PNPFS_MDL pMdl
    )
{
    NTSTATUS ntStatus = 0;

    return(ntStatus);
}

VOID
NpfsFreeMdlList(
    PLW_LIST_LINKS pMdlList
    )
{
    PLW_LIST_LINKS pLink = NULL;
    PLW_LIST_LINKS pNext = NULL;
    PNPFS_MDL pMdl = NULL;

    for (pLink = pMdlList->Next; pLink != pMdlList; pLink = pNext)
    {
        pNext = pLink->Next;
        pMdl = LW_STRUCT_FROM_FIELD(pLink, NPFS_MDL, link);

        NpfsFreeMdl(pMdl);
    }
}

BOOLEAN
NpfsMdlListIsEmpty(
    PLW_LIST_LINKS pMdlList
    )
{
    return LwListIsEmpty(pMdlList);
}
