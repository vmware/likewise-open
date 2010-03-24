#include "includes.h"

static
VOID
SrvCCBFree(
    PSRV_CCB pCCB
    );

NTSTATUS
SrvCCBCreate(
    PSRV_IRP_CONTEXT pIrpContext,
    PSRV_CCB*        ppCCB
    )
{
    NTSTATUS ntStatus = 0;
    PSRV_CCB pCCB = NULL;

    ntStatus = SrvAllocateMemory(sizeof(SRV_CCB), (PVOID*)&pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    pCCB->CcbType = SRV_CCB_DEVICE;
    pCCB->refCount = 1;

    *ppCCB = pCCB;

cleanup:

    return ntStatus;

error:

    *ppCCB = NULL;

    goto cleanup;
}

NTSTATUS
SrvCCBGet(
    IO_FILE_HANDLE FileHandle,
    PSRV_CCB*      ppCCB
    )
{
    NTSTATUS ntStatus = 0;
    PSRV_CCB pCCB = NULL;

    pCCB = (PSRV_CCB)IoFileGetContext(FileHandle);
    if (!pCCB)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppCCB = pCCB;

cleanup:

    return ntStatus;

error:

    *ppCCB = NULL;

    goto cleanup;
}

NTSTATUS
SrvCCBSet(
    IO_FILE_HANDLE FileHandle,
    PSRV_CCB       pCCB
    )
{
    return IoFileSetContext(FileHandle, pCCB);
}

VOID
SrvCCBRelease(
    PSRV_CCB pCCB
    )
{
    if (InterlockedDecrement(&pCCB->refCount) == 0)
    {
        SrvCCBFree(pCCB);
    }
}

static
VOID
SrvCCBFree(
    PSRV_CCB pCCB
    )
{
    SrvFreeMemory(pCCB);
}

