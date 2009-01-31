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
                    sizeof(NPFS_MDL),
                    &pMdl
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
