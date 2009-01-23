#include "includes.h"

static
VOID
SrvIdAllocatorFree(
    PSRV_ID_ALLOCATOR pIdAllocator
    );

NTSTATUS
SrvIdAllocatorCreate(
    USHORT usMaxId,
    PSRV_ID_ALLOCATOR* ppIdAllocator
    )
{
    NTSTATUS ntStatus = 0;
    PSRV_ID_ALLOCATOR pIdAllocator = NULL;

    ntStatus = SMBAllocateMemory(
                    sizeof(SRV_ID_ALLOCATOR),
                    (PVOID*)&pIdAllocator);
    BAIL_ON_NT_STATUS(ntStatus);

    pthread_mutex_init(&pIdAllocator->mutex, NULL);
    pIdAllocator->pMutex = &pIdAllocator->mutex;

    pIdAllocator->refcount = 1;

    ntStatus = SMBBitVectorCreate(
                    usMaxId,
                    &pIdAllocator->map);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppIdAllocator = pIdAllocator;

cleanup:

    return ntStatus;

error:

    *ppIdAllocator = NULL;

    if (pIdAllocator)
    {
        SrvIdAllocatorRelease(pIdAllocator);
    }

    goto cleanup;
}

NTSTATUS
SrvIdAllocatorAcquireId(
    PSRV_ID_ALLOCATOR pIdAllocator,
    PUSHORT pusId
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;
    ULONG ulBit = 0;

    SMB_LOCK_MUTEX(bInLock, &pIdAllocator->mutex);

    ntStatus = SMBBitVectorFirstUnsetBit(
                    pIdAllocator->map,
                    &ulBit);
    BAIL_ON_NT_STATUS(ntStatus);

    *pusId = (USHORT)ulBit;

cleanup:

    SMB_UNLOCK_MUTEX(bInLock, &pIdAllocator->mutex);

    return ntStatus;

error:

    *pusId = 0;

    goto cleanup;
}

VOID
SrvIdAllocatorReleaseId(
    PSRV_ID_ALLOCATOR pIdAllocator,
    USHORT usId
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;

    SMB_LOCK_MUTEX(bInLock, &pIdAllocator->mutex);

    ntStatus = SMBBitVectorUnsetBit(
                    pIdAllocator->map,
                    usId);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    SMB_UNLOCK_MUTEX(bInLock, &pIdAllocator->mutex);

    return;

error:

    SMB_LOG_ERROR("Id Allocator failed to release id [%u][code: %d]", usId, ntStatus);

    goto cleanup;
}

VOID
SrvIdAllocatorRelease(
    PSRV_ID_ALLOCATOR pIdAllocator
    )
{
    if (InterlockedDecrement(&pIdAllocator->refcount) == 0)
    {
        SrvIdAllocatorFree(pIdAllocator);
    }
}

static
VOID
SrvIdAllocatorFree(
    PSRV_ID_ALLOCATOR pIdAllocator
    )
{
    if (pIdAllocator->pMutex)
    {
        pthread_mutex_destroy(&pIdAllocator->mutex);
    }

    if (pIdAllocator->map)
    {
        SMBBitVectorFree(pIdAllocator->map);
    }
}
