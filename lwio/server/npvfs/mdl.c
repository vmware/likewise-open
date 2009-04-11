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
    PNPFS_MDL pMdlList,
    PVOID pBuffer,
    ULONG Length,
    PULONG pulBytesTransferred,
    PNPFS_MDL * ppMdlList
    )
{
    NTSTATUS ntStatus = 0;
    PNPFS_MDL pMdl = NULL;
    PNPFS_MDL pNewMdlList = NULL;

    ntStatus = NpfsCreateMdl(
                    Length,
                    pBuffer,
                    &pMdl
                    );
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NpfsEnqueueMdl(
                    pMdlList,
                    pMdl,
                    &pNewMdlList
                    );
    BAIL_ON_NT_STATUS(ntStatus);

    *ppMdlList = pNewMdlList;
    *pulBytesTransferred = Length;
    return(ntStatus);

error:

    if (pMdl) {
        NpfsFreeMdl(pMdl);
    }
    *pulBytesTransferred = 0;
    *ppMdlList = NULL;
    return(ntStatus);

}

NTSTATUS
NpfsDequeueBuffer(
    PNPFS_MDL pMdlList,
    PVOID pBuffer,
    ULONG Length,
    PULONG pulBytesTransferred,
    PNPFS_MDL * ppMdlList
    )
{
    NTSTATUS ntStatus = 0;
    ULONG LengthRemaining = 0;
    ULONG BytesAvail = 0;
    ULONG BytesToCopy = 0;
    ULONG BytesCopied = 0;
    PNPFS_MDL pMdl = NULL;

    if (!pMdlList) {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    LengthRemaining = Length;
    while (LengthRemaining && pMdlList){

        BytesAvail = pMdlList->Length - pMdlList->Offset;
        BytesToCopy = min(BytesAvail, LengthRemaining);
        memcpy(pBuffer, pMdlList->Buffer + pMdlList->Offset, BytesToCopy);
        BytesCopied += BytesToCopy;
        pMdlList->Offset += BytesToCopy;
        LengthRemaining -= BytesToCopy;
        if (pMdlList->Length - pMdlList->Offset == 0){
            NpfsDequeueMdl(pMdlList, &pMdl, &pMdlList);
            NpfsFreeMdl(pMdl);

        }
    }
    *pulBytesTransferred = BytesCopied;
    *ppMdlList = pMdlList;
    return(ntStatus);

error:
    *pulBytesTransferred = 0;
    *ppMdlList = NULL;
    return(ntStatus);
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

NTSTATUS
NpfsEnqueueMdl(
    PNPFS_MDL pMdlList,
    PNPFS_MDL pMdl,
    PNPFS_MDL *ppMdlList
    )
{
    NTSTATUS ntStatus = 0;

    if (!pMdl) {
        return (STATUS_INVALID_PARAMETER);
    }

    pMdl->pNext = pMdlList;
    *ppMdlList = pMdl;

    return (ntStatus);
}

NTSTATUS
NpfsDequeueMdl(
    PNPFS_MDL pMdlList,
    PNPFS_MDL * ppMdl,
    PNPFS_MDL *ppMdlList
    )
{
    NTSTATUS ntStatus = 0;
    PNPFS_MDL pMdl = NULL;

    if (!pMdlList) {
        *ppMdlList = NULL;
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pMdl = pMdlList;
    pMdlList = pMdl->pNext;

    *ppMdl = pMdl;
    *ppMdlList = pMdlList;

error:
    return(ntStatus);
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

NTSTATUS
NpfsFreeMdlList(
    PNPFS_MDL pNpfsMdlList
    )
{
    NTSTATUS ntStatus = 0;

    return(ntStatus);
}

BOOLEAN
NpfsMdlListIsEmpty(
    PNPFS_MDL pNpfsMdlList
    )
{
    if (!pNpfsMdlList) {
        return TRUE;
    }else {
        return FALSE;
    }
}

