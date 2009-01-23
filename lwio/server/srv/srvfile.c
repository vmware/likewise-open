#include "includes.h"

static
VOID
SrvFileFree(
    PSMB_SRV_FILE pFile
    );

NTSTATUS
SrvFileCreate(
    PSRV_ID_ALLOCATOR pIdAllocator,
    PSMB_SRV_FILE*    ppFile
    )
{
    NTSTATUS ntStatus = 0;
    USHORT fid = 0;
    PSMB_SRV_FILE pFile = NULL;

    pthread_rwlock_init(&pFile->mutex, NULL);
    pFile->pMutex = &pFile->mutex;

    ntStatus = SMBAllocateMemory(
                    sizeof(PSMB_SRV_FILE),
                    (PVOID*)&pFile);
    BAIL_ON_NT_STATUS(ntStatus);

    pFile->refcount = 1;

    ntStatus = SrvIdAllocatorAcquireId(
                    pIdAllocator,
                    &fid);
    BAIL_ON_NT_STATUS(ntStatus);

    pFile->fid = fid;

    pFile->pFileIdAllocator = pIdAllocator;
    InterlockedIncrement(&pIdAllocator->refcount);

    *ppFile = pFile;

cleanup:

    return ntStatus;

error:

    *ppFile = NULL;

    if (pFile)
    {
        SrvFileRelease(pFile);
    }

    goto cleanup;
}

VOID
SrvFileRelease(
    PSMB_SRV_FILE pFile
    )
{
    if (InterlockedDecrement(&pFile->refcount) == 0)
    {
        SrvFileFree(pFile);
    }
}

static
VOID
SrvFileFree(
    PSMB_SRV_FILE pFile
    )
{
    if (pFile->pMutex)
    {
        pthread_rwlock_destroy(&pFile->mutex);
        pFile->pMutex = NULL;
    }

    if (pFile->pFileIdAllocator)
    {
        SrvIdAllocatorReleaseId(
                pFile->pFileIdAllocator,
                pFile->fid);

        SrvIdAllocatorRelease(pFile->pFileIdAllocator);
    }

    SMBFreeMemory(pFile);
}
