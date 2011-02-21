/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

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
    PIO_SECURITY_CONTEXT_PROCESS_INFORMATION pProcInfo = NULL;

    pProcInfo = IoSecurityGetProcessInfo(pIrpContext->pIrp->Args.Create.SecurityContext);

    ntStatus = SrvAllocateMemory(sizeof(SRV_CCB), (PVOID*)&pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    pCCB->CcbType = SRV_CCB_DEVICE;
    pCCB->refCount = 1;
    pCCB->UnixUid = pProcInfo ? pProcInfo->Uid : -1;

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

