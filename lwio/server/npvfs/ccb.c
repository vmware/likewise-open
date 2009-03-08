#include "npfs.h"


NTSTATUS
NpfsCreateSCB(
    PNPFS_IRP_CONTEXT pIrpContext,
    PNPFS_CCB * ppSCB
    )
{
    NTSTATUS ntStatus = 0;
    PNPFS_CCB pSCB = NULL;

    ntStatus = NpfsAllocateMemory(
                    sizeof(*pSCB),
                    OUT_PPVOID(&pSCB)
                    );
    BAIL_ON_NT_STATUS(ntStatus);

    pSCB->CcbType = NPFS_CCB_SERVER;

    NpfsInitializeInterlockedCounter(&pSCB->cRef);
    NpfsAddRefCCB(pSCB);

    *ppSCB = pSCB;

    return(ntStatus);


error:

    *ppSCB = NULL;

    return(ntStatus);
}

NTSTATUS
NpfsCreateCCB(
    PNPFS_IRP_CONTEXT pIrpContext,
    PNPFS_CCB * ppCCB
    )
{
    NTSTATUS ntStatus = 0;
    PNPFS_CCB pCCB = NULL;

    ntStatus = NpfsAllocateMemory(
                    sizeof(*pCCB),
                    OUT_PPVOID(&pCCB)
                    );
    BAIL_ON_NT_STATUS(ntStatus);

    pCCB->CcbType = NPFS_CCB_CLIENT;

    NpfsInitializeInterlockedCounter(&pCCB->cRef);
    NpfsAddRefCCB(pCCB);

    *ppCCB = pCCB;

    return(ntStatus);


error:

    *ppCCB = NULL;

    return(ntStatus);
}


VOID
NpfsReleaseCCB(
    PNPFS_CCB pCCB
    )
{
    NpfsInterlockedDecrement(&pCCB->cRef);
    if (!NpfsInterlockedCounter(&pCCB->cRef))
    {
        NpfsFreeCCB(pCCB);
    }
    return;
}

VOID
NpfsAddRefCCB(
    PNPFS_CCB pCCB
    )
{
    NpfsInterlockedIncrement(&pCCB->cRef);
    return;
}


NTSTATUS
NpfsFreeCCB(
    PNPFS_CCB pCCB
    )
{

    NTSTATUS ntStatus = 0;

    NpfsReleasePipe(pCCB->pPipe);

    ntStatus = NpfsFreeMdlList(
                    pCCB->pMdlList
                    );
    BAIL_ON_NT_STATUS(ntStatus);

    NpfsFreeMemory(pCCB);

error:

    return(ntStatus);
}

NTSTATUS
NpfsGetCCB(
    IO_FILE_HANDLE FileHandle,
    PNPFS_CCB * ppCCB
    )
{
    NTSTATUS ntStatus = 0;
    PNPFS_CCB pCCB = NULL;

    pCCB = (PNPFS_CCB)IoFileGetContext(FileHandle);
    if (!pCCB)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    NpfsAddRefCCB(pCCB);

    *ppCCB = pCCB;

cleanup:

    return ntStatus;

error:
    *ppCCB = NULL;

    goto cleanup;

}

NTSTATUS
NpfsSetCCB(
    IO_FILE_HANDLE FileHandle,
    PNPFS_CCB pCCB
    )
{
    NTSTATUS ntStatus = 0;

    ntStatus = IoFileSetContext(
                        FileHandle,
                        pCCB
                        );
    BAIL_ON_NT_STATUS(ntStatus);

error:

    return (ntStatus);
}
