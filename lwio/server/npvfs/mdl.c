#include "npfs.h"

NTSTATUS
NpfsEnqueueBuffer(
    PNPFS_MDL pMdlList,
    PVOID pBuffer,
    ULONG Length
    )
{
    NTSTATUS ntStatus = 0;


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

error:

    if (pMdl) {
        NpfsFreeMdl(pMdl);
    }
    *ppMdlList = pNewMdlList;
    return(ntStatus);

}

NTSTATUS
NpfsCreateMdl(
    ULONG Length,
    PVOID pBuffer
    PNPFS_MDL * ppMdl
    )
{
    NTSTATUS ntStatus = 0;

    ntStatus = RtlAllocateMemory(
                    Length,
                    &pTargBuffer
                    );
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = RtlCopyMemory(
                    pTargBuffer,
                    pBuffer,
                    Length
                    );
    BAIL_ON_NT_STATUS(ntStatus);

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

    return(ntStatus);
}

NTSTATUS
NpfsDequeueMdl(
    PNPFS_MDL pMdlList,
    PNPFS_MDL pMdl,
    PNPFS_MDL *ppMdlList
    )
{
    NTSTATUS ntStatus = 0;

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

NTSTATUS
NpfsDeleteMdl(
    PNPFS_MDL pMdl
    )
{
    NTSTATUS ntStatus = 0;

    return(ntStatus);
}



NTSTATUS
NpfsAddMdltoInboundQueue(
    PNPFS_CCB pCCB,
    PNPFS_MDL pMdl
    )
{
    Lock the CCB


    if


}

