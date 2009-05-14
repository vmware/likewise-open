#include "includes.h"

static
VOID
SrvFileFree(
    PLWIO_SRV_FILE pFile
    );

NTSTATUS
SrvFileCreate(
    USHORT                  fid,
    PWSTR                   pwszFilename,
    PIO_FILE_HANDLE         phFile,
    PIO_FILE_NAME*          ppFilename,
    ACCESS_MASK             desiredAccess,
    LONG64                  allocationSize,
    FILE_ATTRIBUTES         fileAttributes,
    FILE_SHARE_FLAGS        shareAccess,
    FILE_CREATE_DISPOSITION createDisposition,
    FILE_CREATE_OPTIONS     createOptions,
    PLWIO_SRV_FILE*          ppFile
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_FILE pFile = NULL;

    LWIO_LOG_DEBUG("Creating file [fid:%u]", fid);

    ntStatus = LW_RTL_ALLOCATE(
                    &pFile,
                    LWIO_SRV_FILE,
                    sizeof(LWIO_SRV_FILE));
    BAIL_ON_NT_STATUS(ntStatus);

    pFile->refcount = 1;

    pthread_rwlock_init(&pFile->mutex, NULL);
    pFile->pMutex = &pFile->mutex;

    ntStatus = SMBAllocateStringW(
                    pwszFilename,
                    &pFile->pwszFilename);
    BAIL_ON_NT_STATUS(ntStatus);

    pFile->fid = fid;
    pFile->hFile = *phFile;
    *phFile = NULL;
    pFile->pFilename = *ppFilename;
    *ppFilename = NULL;
    pFile->desiredAccess = desiredAccess;
    pFile->allocationSize = allocationSize;
    pFile->fileAttributes = fileAttributes;
    pFile->shareAccess = shareAccess;
    pFile->createDisposition = createDisposition;
    pFile->createOptions = createOptions;

    LWIO_LOG_DEBUG("Associating file [object:0x%x][fid:%u]",
                    pFile,
                    fid);

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
    PLWIO_SRV_FILE pFile
    )
{
    LWIO_LOG_DEBUG("Releasing file [fid:%u]", pFile->fid);

    if (InterlockedDecrement(&pFile->refcount) == 0)
    {
        SrvFileFree(pFile);
    }
}

static
VOID
SrvFileFree(
    PLWIO_SRV_FILE pFile
    )
{
    LWIO_LOG_DEBUG("Freeing file [object:0x%x][fid:%u]",
                    pFile,
                    pFile->fid);

    if (pFile->pMutex)
    {
        pthread_rwlock_destroy(&pFile->mutex);
        pFile->pMutex = NULL;
    }

    if (pFile->pFilename)
    {
        if (pFile->pFilename->FileName)
        {
            LwRtlMemoryFree (pFile->pFilename->FileName);
        }

        LwRtlMemoryFree(pFile->pFilename);
    }

    if (pFile->hFile)
    {
        IoCloseFile(pFile->hFile);
    }

    if (pFile->pwszFilename)
    {
        LwRtlMemoryFree(pFile->pwszFilename);
    }

    LwRtlMemoryFree(pFile);
}
