#include "includes.h"

static
NTSTATUS
SrvTreeAcquireFileId_inlock(
   PSMB_SRV_TREE pTree,
   PUSHORT       pFid
   );

static
int
SrvTreeFileCompare(
    PVOID pKey1,
    PVOID pKey2
    );

static
VOID
SrvTreeFileRelease(
    PVOID pFile
    );

static
VOID
SrvTreeFree(
    PSMB_SRV_TREE pTree
    );

NTSTATUS
SrvTreeCreate(
    USHORT            tid,
    PSHARE_DB_INFO    pShareInfo,
    PSMB_SRV_TREE*    ppTree
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_SRV_TREE pTree = NULL;

    SMB_LOG_DEBUG("Creating Tree [tid: %u]", tid);

    ntStatus = SMBAllocateMemory(
                    sizeof(SMB_SRV_TREE),
                    (PVOID*)&pTree);
    BAIL_ON_NT_STATUS(ntStatus);

    pTree->refcount = 1;

    pthread_rwlock_init(&pTree->mutex, NULL);
    pTree->pMutex = &pTree->mutex;

    pTree->tid = tid;

    SMB_LOG_DEBUG("Associating Tree [object:0x%x][tid:%u]",
                    pTree,
                    tid);

    pTree->pShareInfo = pShareInfo;
    InterlockedIncrement(&pShareInfo->refcount);

    ntStatus = SMBRBTreeCreate(
                    &SrvTreeFileCompare,
                    NULL,
                    &SrvTreeFileRelease,
                    &pTree->pFileCollection);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppTree = pTree;

cleanup:

    return ntStatus;

error:

    *ppTree = NULL;

    if (pTree)
    {
        SrvTreeRelease(pTree);
    }

    goto cleanup;
}

NTSTATUS
SrvTreeFindFile(
    PSMB_SRV_TREE  pTree,
    USHORT         fid,
    PSMB_SRV_FILE* ppFile
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_SRV_FILE pFile = NULL;
    BOOLEAN bInLock = FALSE;

    SMB_LOCK_RWMUTEX_SHARED(bInLock, &pTree->mutex);

    ntStatus = SMBRBTreeFind(
                    pTree->pFileCollection,
                    &fid,
                    (PVOID*)&pFile);
    BAIL_ON_NT_STATUS(ntStatus);

    InterlockedIncrement(&pFile->refcount);

    *ppFile = pFile;

cleanup:

    SMB_UNLOCK_RWMUTEX(bInLock, &pTree->mutex);

    return ntStatus;

error:

    *ppFile = NULL;

    goto cleanup;
}

NTSTATUS
SrvTreeCreateFile(
    PSMB_SRV_TREE           pTree,
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
    BOOLEAN bInLock = FALSE;
    PSMB_SRV_FILE pFile = NULL;
    USHORT  fid = 0;

    SMB_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pTree->mutex);

    ntStatus = SrvTreeAcquireFileId_inlock(
                    pTree,
                    &fid);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvFileCreate(
                    fid,
                    phFile,
                    ppFilename,
                    desiredAccess,
                    allocationSize,
                    fileAttributes,
                    shareAccess,
                    createDisposition,
                    createOptions,
                    &pFile);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBRBTreeAdd(
                    pTree->pFileCollection,
                    &pFile->fid,
                    pFile);
    BAIL_ON_NT_STATUS(ntStatus);

    InterlockedIncrement(&pFile->refcount);

    *ppFile = pFile;

cleanup:

    SMB_UNLOCK_RWMUTEX(bInLock, &pTree->mutex);

error:

    *ppFile = NULL;

    if (pFile)
    {
        SrvFileRelease(pFile);
    }

    goto cleanup;
}

VOID
SrvTreeRelease(
    PSMB_SRV_TREE pTree
    )
{
    SMB_LOG_DEBUG("Releasing tree [tid:%u]", pTree->tid);

    if (InterlockedDecrement(&pTree->refcount) == 0)
    {
        SrvTreeFree(pTree);
    }
}

static
NTSTATUS
SrvTreeAcquireFileId_inlock(
   PSMB_SRV_TREE pTree,
   PUSHORT       pFid
   )
{
    NTSTATUS ntStatus = 0;
    USHORT   candidateFid = pTree->nextAvailableFid;
    BOOLEAN  bFound = FALSE;

    do
    {
        PSMB_SRV_FILE pFile = NULL;

        if (!candidateFid || (candidateFid == UINT16_MAX))
        {
            candidateFid++;
        }

        ntStatus = SMBRBTreeFind(
                        pTree->pFileCollection,
                        &candidateFid,
                        (PVOID*)&pFile);
        if (ntStatus == STATUS_NOT_FOUND)
        {
            ntStatus = 0;
            bFound = TRUE;
        }
        BAIL_ON_NT_STATUS(ntStatus);

    } while ((candidateFid != pTree->nextAvailableFid) && !bFound);

    if (!bFound)
    {
        ntStatus = STATUS_TOO_MANY_OPENED_FILES;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pTree->nextAvailableFid = candidateFid + 1;
    *pFid = candidateFid;

cleanup:

    return ntStatus;

error:

    *pFid = 0;

    goto cleanup;
}

static
int
SrvTreeFileCompare(
    PVOID pKey1,
    PVOID pKey2
    )
{
    PUSHORT pFid1 = (PUSHORT)pKey1;
    PUSHORT pFid2 = (PUSHORT)pKey2;

    if (*pFid1 > *pFid2)
    {
        return 1;
    }
    else if (*pFid1 < *pFid2)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

static
VOID
SrvTreeFileRelease(
    PVOID pTree
    )
{
    SrvFileRelease((PSMB_SRV_FILE)pTree);
}

static
VOID
SrvTreeFree(
    PSMB_SRV_TREE pTree
    )
{
    SMB_LOG_DEBUG("Freeing tree [object:0x%x][tid:%u]",
                    pTree,
                    pTree->tid);

    if (pTree->pMutex)
    {
        pthread_rwlock_destroy(&pTree->mutex);
        pTree->pMutex = NULL;
    }

    if (pTree->pFileCollection)
    {
        SMBRBTreeFree(pTree->pFileCollection);
    }

    if (pTree->pShareInfo)
    {
        SrvShareDbReleaseInfo(pTree->pShareInfo);
    }

    SMBFreeMemory(pTree);
}

