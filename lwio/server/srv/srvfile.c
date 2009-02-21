#include "includes.h"

static
VOID
SrvFileFree(
    PSMB_SRV_FILE pFile
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
    PSMB_SRV_FILE*          ppFile
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_SRV_FILE pFile = NULL;

    SMB_LOG_DEBUG("Creating file [fid:%u]", fid);

    ntStatus = LW_RTL_ALLOCATE(
                    &pFile,
                    SMB_SRV_FILE,
                    sizeof(SMB_SRV_FILE));
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

    SMB_LOG_DEBUG("Associating file [object:0x%x][fid:%u]",
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
    PSMB_SRV_FILE pFile
    )
{
    SMB_LOG_DEBUG("Releasing file [fid:%u]", pFile->fid);

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
    SMB_LOG_DEBUG("Freeing file [object:0x%x][fid:%u]",
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
