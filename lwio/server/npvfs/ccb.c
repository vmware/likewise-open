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
                    sizeof(NPFS_CCB),
                    &pSCB
                    );
    BAIL_ON_NT_STATUS(ntStatus);

    pSCB->CcbType = NPFS_CCB_SERVER;

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
                    sizeof(NPFS_CCB),
                    &pCCB
                    );
    BAIL_ON_NT_STATUS(ntStatus);

    pCCB->CcbType = NPFS_CCB_CLIENT;

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
    if (!NpfsInterlockedCounter(&pCCB->cRef)) {

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
    if (!pCCB) {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }
    ENTER_READER_RW_LOCK(&gCCBLock);

    ntStatus = NpfsFindCCB(pCCB);
    BAIL_ON_NT_STATUS(ntStatus);
    NpfsAddRefCCB(pCCB);

    *ppCCB = pCCB;

cleanup:

    LEAVE_READER_RW_LOCK(&gCCBLock);

    return(ntStatus);

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

    ntStatus = NpfsAddCCB(pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = IoFileSetContext(
                        FileHandle,
                        pCCB
                        );
    BAIL_ON_NT_STATUS(ntStatus);

error:

    return (ntStatus);
}

NTSTATUS
NpfsAddCCB(
    PNPFS_CCB pCCB
    )
{
    NTSTATUS ntStatus = 0;

    ENTER_WRITER_RW_LOCK(&gCCBLock);

    ntStatus = NpfsFindCCB(pCCB);
    if (ntStatus == 0) {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = STATUS_SUCCESS;

    pCCB->pNext = gpCCBList;
    gpCCBList = pCCB;

error:

    LEAVE_WRITER_RW_LOCK(&gCCBLock);

    return(ntStatus);
}

NTSTATUS
NpfsFindCCB(
    PNPFS_CCB pCCB
    )
{
    PNPFS_CCB pTemp = NULL;

    pTemp = gpCCBList;

    while (pTemp) {
        if (pTemp == pCCB) {
            return STATUS_SUCCESS;
        }
        pTemp = pTemp->pNext;
    }
    return STATUS_NOT_FOUND;
}
