#include "includes.h"

static
NTSTATUS
SrvCCBAdd(
    PSRV_CCB pCCB
    );

static
NTSTATUS
SrvCCBFind(
    PSRV_CCB pCCB
    );

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

    ntStatus = LW_RTL_ALLOCATE(
                    &pCCB,
                    SRV_CCB,
                    sizeof(SRV_CCB));
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
    BOOLEAN  bInLock = FALSE;
    PSRV_CCB pCCB = NULL;

    pCCB = (PSRV_CCB)IoFileGetContext(FileHandle);
    if (!pCCB)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    SMB_LOCK_MUTEX(bInLock, &gSMBSrvGlobals.mutex);

    ntStatus = SrvCCBFind(pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    InterlockedIncrement(&pCCB->refCount);

    *ppCCB = pCCB;

cleanup:

    SMB_UNLOCK_MUTEX(bInLock, &gSMBSrvGlobals.mutex);

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
    NTSTATUS ntStatus = 0;

    ntStatus = SrvCCBAdd(pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = IoFileSetContext(
                        FileHandle,
                        pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

error:

    return ntStatus;
}

static
NTSTATUS
SrvCCBAdd(
    PSRV_CCB pCCB
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;

    SMB_LOCK_MUTEX(bInLock, &gSMBSrvGlobals.mutex);

    ntStatus = SrvCCBFind(pCCB);
    if (ntStatus == 0)
    {
        ntStatus = STATUS_OBJECTID_EXISTS;
        BAIL_ON_NT_STATUS(ntStatus);
    }
    ntStatus = STATUS_SUCCESS;

    pCCB->pNext = gSMBSrvGlobals.pCCBList;
    gSMBSrvGlobals.pCCBList = pCCB;

error:

    SMB_UNLOCK_MUTEX(bInLock, &gSMBSrvGlobals.mutex);

    return ntStatus;
}

static
NTSTATUS
SrvCCBFind(
    PSRV_CCB pCCB
    )
{
    NTSTATUS ntStatus = STATUS_NOT_FOUND;
    PSRV_CCB pCandidate = NULL;

    for (pCandidate = gSMBSrvGlobals.pCCBList; pCandidate; pCandidate = pCandidate->pNext)
    {
        if (pCandidate == pCCB)
        {
            ntStatus = STATUS_SUCCESS;
            break;
        }
    }

    return ntStatus;
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
    LwRtlMemoryFree(pCCB);
}

