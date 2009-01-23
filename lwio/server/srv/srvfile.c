#include "includes.h"

static
VOID
SrvFileFree(
    PSMB_SRV_FILE pFile
    );

NTSTATUS
SrvFileCreate(
    USHORT         fid,
    PSMB_SRV_FILE* ppFile
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_SRV_FILE pFile = NULL;

    pthread_mutex_init(&pFile->mutex, NULL);
    pFile->pMutex = &pFile->mutex;

    ntStatus = SMBAllocateMemory(
                    sizeof(PSMB_SRV_FILE),
                    (PVOID*)&pFile);
    BAIL_ON_NT_STATUS(ntStatus);

    pFile->fid = fid;
    pFile->refcount = 1;

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
        pthread_mutex_destroy(&pFile->mutex);
        pFile->pMutex = NULL;
    }

    SMBFreeMemory(pFile);
}
