/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */



/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        srvfile2.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Elements
 *
 *        File Object (Version 2)
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 */

#include "includes.h"

static
BOOLEAN
SrvFile2IsInParent_inlock(
    PLWIO_SRV_FILE_2 pFile
    );

static
VOID
SrvFile2Free(
    PLWIO_SRV_FILE_2 pFile
    );

static
BOOLEAN
SrvFile2IsRundown_inlock(
    PLWIO_SRV_FILE_2 pFile
    );

static
VOID
SrvFile2SetRundown_inlock(
    PLWIO_SRV_FILE_2 pFile
    );

NTSTATUS
SrvFile2Create(
    PLWIO_SRV_TREE_2        pTree,
    PSMB2_FID               pFid,
    PWSTR                   pwszFilename,
    PIO_FILE_HANDLE         phFile,
    PIO_FILE_NAME*          ppFilename,
    ACCESS_MASK             desiredAccess,
    LONG64                  allocationSize,
    FILE_ATTRIBUTES         fileAttributes,
    FILE_SHARE_FLAGS        shareAccess,
    FILE_CREATE_DISPOSITION createDisposition,
    FILE_CREATE_OPTIONS     createOptions,
    PLWIO_SRV_FILE_2*       ppFile
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_FILE_2 pFile = NULL;

    LWIO_LOG_DEBUG("Creating file [fid: (persistent:%08X)(volatile:%08X)]",
                   pFid->ullPersistentId,
                   pFid->ullVolatileId);

    ntStatus = SrvAllocateMemory(
                    sizeof(LWIO_SRV_FILE_2),
                    (PVOID*)&pFile);
    BAIL_ON_NT_STATUS(ntStatus);

    pFile->refcount = 1;

    pthread_rwlock_init(&pFile->mutex, NULL);
    pFile->pMutex = &pFile->mutex;

    pFile->pTree = pTree;
    SrvTree2Acquire(pTree);

    ntStatus = SrvAllocateStringW(pwszFilename, &pFile->pwszFilename);
    BAIL_ON_NT_STATUS(ntStatus);

    pFile->fid = *pFid;
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

    pFile->resource.resourceType                 = SRV_RESOURCE_TYPE_FILE;
    pFile->resource.pAttributes                  = &pFile->resourceAttrs;
    pFile->resource.pAttributes->protocolVersion = SMB_PROTOCOL_VERSION_2;
    pFile->resource.pAttributes->fileId.pFid2    = &pFile->fid;
    pFile->resource.pAttributes->treeId.usTid     = pTree->ulTid;
    pFile->resource.pAttributes->sessionId.ullUid = pTree->ullUid;
    pFile->resource.pAttributes->ulConnectionResourceId =
                                                pTree->ulConnectionResourceId;

    LWIO_LOG_DEBUG( "Associating file [object:0x%x]"
                    "[fid: (persistent:%08X)(volatile:%08X)]",
                    pFile,
                    pFid->ullPersistentId,
                    pFid->ullVolatileId);

    SRV_ELEMENTS_INCREMENT_OPEN_FILES;

    *ppFile = pFile;

cleanup:

    return ntStatus;

error:

    *ppFile = NULL;

    if (pFile)
    {
        SrvFile2Release(pFile);
    }

    goto cleanup;
}

BOOLEAN
SrvFile2IsInParent(
    PLWIO_SRV_FILE_2 pFile
    )
{
    BOOLEAN bIsInParent = FALSE;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pFile->mutex);
    bIsInParent = SrvFile2IsInParent_inlock(pFile);
    LWIO_UNLOCK_RWMUTEX(bInLock, &pFile->mutex);

    return bIsInParent;
}

static
BOOLEAN
SrvFile2IsInParent_inlock(
    PLWIO_SRV_FILE_2 pFile
    )
{
    return IsSetFlag(pFile->objectFlags, SRV_OBJECT_FLAG_IN_PARENT);
}

VOID
SrvFile2SetInParent(
    PLWIO_SRV_FILE_2 pFile
    )
{
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pFile->mutex);
    LWIO_ASSERT(!IsSetFlag(pFile->objectFlags, SRV_OBJECT_FLAG_IN_PARENT));
    SetFlag(pFile->objectFlags, SRV_OBJECT_FLAG_IN_PARENT);
    LWIO_UNLOCK_RWMUTEX(bInLock, &pFile->mutex);
}

VOID
SrvFile2ClearInParent(
    PLWIO_SRV_FILE_2 pFile
    )
{
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pFile->mutex);
    LWIO_ASSERT(IsSetFlag(pFile->objectFlags, SRV_OBJECT_FLAG_IN_PARENT));
    ClearFlag(pFile->objectFlags, SRV_OBJECT_FLAG_IN_PARENT);
    LWIO_UNLOCK_RWMUTEX(bInLock, &pFile->mutex);
}

NTSTATUS
SrvFile2SetOplockState(
    PLWIO_SRV_FILE_2               pFile,
    HANDLE                         hOplockState,
    PFN_LWIO_SRV_FREE_OPLOCK_STATE pfnFreeOplockState
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN  bInLock  = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pFile->mutex);

    if (pFile->hOplockState && pFile->pfnFreeOplockState)
    {
        pFile->pfnFreeOplockState(pFile->hOplockState);
        pFile->hOplockState       = NULL;
        pFile->pfnFreeOplockState = NULL;
    }

    if (SrvFile2IsRundown_inlock(pFile))
    {
        ntStatus = STATUS_INVALID_HANDLE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pFile->hOplockState       = hOplockState;
    pFile->pfnFreeOplockState = pfnFreeOplockState;

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pFile->mutex);

    return ntStatus;

error:

    goto cleanup;
}

HANDLE
SrvFile2RemoveOplockState(
    PLWIO_SRV_FILE_2 pFile
    )
{
    HANDLE  hOplockState = NULL;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pFile->mutex);

    hOplockState = pFile->hOplockState;

    pFile->hOplockState       = NULL;
    pFile->pfnFreeOplockState = NULL;

    LWIO_UNLOCK_RWMUTEX(bInLock, &pFile->mutex);

    return hOplockState;
}

VOID
SrvFile2ResetOplockState(
    PLWIO_SRV_FILE_2 pFile
    )
{
    BOOLEAN  bInLock  = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pFile->mutex);

    if (pFile->hOplockState && pFile->pfnFreeOplockState)
    {
        pFile->pfnFreeOplockState(pFile->hOplockState);
        pFile->hOplockState       = NULL;
        pFile->pfnFreeOplockState = NULL;
    }

    LWIO_UNLOCK_RWMUTEX(bInLock, &pFile->mutex);
}

VOID
SrvFile2SetOplockLevel(
    PLWIO_SRV_FILE_2 pFile,
    UCHAR            ucOplockLevel
    )
{
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pFile->mutex);

    pFile->ucCurrentOplockLevel = ucOplockLevel;

    LWIO_UNLOCK_RWMUTEX(bInLock, &pFile->mutex);
}

UCHAR
SrvFile2GetOplockLevel(
    PLWIO_SRV_FILE_2 pFile
    )
{
    UCHAR ucOplockLevel = SMB_OPLOCK_LEVEL_NONE;

    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pFile->mutex);

    ucOplockLevel = pFile->ucCurrentOplockLevel;

    LWIO_UNLOCK_RWMUTEX(bInLock, &pFile->mutex);

    return ucOplockLevel;
}

VOID
SrvFile2SetClosing(
    PLWIO_SRV_FILE_2 pFile
    )
{
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pFile->mutex);

    pFile->bIsClosing = TRUE;

    LWIO_UNLOCK_RWMUTEX(bInLock, &pFile->mutex);
}


BOOLEAN
SrvFile2IsFileClosing(
    PLWIO_SRV_FILE_2 pFile
    )
{
    BOOLEAN bClosing = FALSE;

    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pFile->mutex);

    bClosing = pFile->bIsClosing;

    LWIO_UNLOCK_RWMUTEX(bInLock, &pFile->mutex);

    return bClosing;
}

VOID
SrvFile2RegisterLock(
    PLWIO_SRV_FILE_2 pFile
    )
{
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pFile->mutex);

    pFile->ulNumLocks++;

    LWIO_UNLOCK_RWMUTEX(bInLock, &pFile->mutex);
}

VOID
SrvFile2RegisterUnlock(
    PLWIO_SRV_FILE_2 pFile
    )
{
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pFile->mutex);

    pFile->ulNumLocks--;

    LWIO_UNLOCK_RWMUTEX(bInLock, &pFile->mutex);
}

PLWIO_SRV_FILE_2
SrvFile2Acquire(
    PLWIO_SRV_FILE_2 pFile
    )
{
    LWIO_LOG_DEBUG("Acquiring file [fid: (persistent:%08X)(volatile:%08X)]",
                    pFile->fid.ullPersistentId,
                    pFile->fid.ullVolatileId);

    InterlockedIncrement(&pFile->refcount);

    return pFile;
}

VOID
SrvFile2Release(
    PLWIO_SRV_FILE_2 pFile
    )
{
    LWIO_LOG_DEBUG("Releasing file [fid: (persistent:%08X)(volatile:%08X)]",
                    pFile->fid.ullPersistentId,
                    pFile->fid.ullVolatileId);

    if (InterlockedDecrement(&pFile->refcount) == 0)
    {
        SRV_ELEMENTS_DECREMENT_OPEN_FILES;

        SrvFile2Free(pFile);
    }
}

VOID
SrvFile2Rundown(
    PLWIO_SRV_FILE_2 pFile
    )
{
    BOOLEAN bInLock = FALSE;
    BOOLEAN bDoRundown = FALSE;
    BOOLEAN bIsInParent = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pFile->mutex);

    if (!SrvFile2IsRundown_inlock(pFile))
    {
        SrvFile2SetRundown_inlock(pFile);

        bDoRundown = TRUE;
        bIsInParent = SrvFile2IsInParent_inlock(pFile);
    }

    LWIO_UNLOCK_RWMUTEX(bInLock, &pFile->mutex);

    if (bIsInParent)
    {
        SrvTree2RemoveFile(pFile->pTree, pFile);
    }

    if (bDoRundown)
    {
        if (pFile->resource.ulResourceId)
        {
            PSRV_RESOURCE pResource = NULL;

            SrvElementsUnregisterResource(pFile->resource.ulResourceId, &pResource);
            pFile->resource.ulResourceId = 0;
        }

        if (pFile->hFile)
        {
            IoCancelForRundownFile(pFile->hFile);
        }

        SrvFile2ResetOplockState(pFile);
    }
}

static
VOID
SrvFile2Free(
    PLWIO_SRV_FILE_2 pFile
    )
{
    LWIO_LOG_DEBUG( "Freeing file [object:0x%x]"
                    "[fid: (persistent:%08X)(volatile:%08X)]",
                    pFile,
                    pFile->fid.ullPersistentId,
                    pFile->fid.ullVolatileId);

    // Cannot be in the parent since parent would have a reference.
    LWIO_ASSERT(!SrvFile2IsInParent_inlock(pFile));

    if (pFile->pMutex)
    {
        pthread_rwlock_destroy(&pFile->mutex);
        pFile->pMutex = NULL;
    }

    if (pFile->pFilename)
    {
        if (pFile->pFilename->FileName)
        {
            SrvFreeMemory (pFile->pFilename->FileName);
        }

        SrvFreeMemory(pFile->pFilename);
    }

    if (pFile->hFile)
    {
        // TODO: Use IoAsyncCloseFile here and other Srv*Free()
        // and SrvFile*() functions -- with callback that just
        // frees IOSB -- but make sure to pre-allocate IOSB
        // when SRV_FILE_2 is created.  Or add IoNoWaitCloseFile()
        // to iomgr.
        IoCloseFile(pFile->hFile);
    }

    if (pFile->pwszFilename)
    {
        SrvFreeMemory(pFile->pwszFilename);
    }

    if (pFile->searchSpace.pwszSearchPattern)
    {
        SrvFreeMemory(pFile->searchSpace.pwszSearchPattern);
    }

    if (pFile->searchSpace.pwszSearchPatternRaw)
    {
        SrvFreeMemory(pFile->searchSpace.pwszSearchPatternRaw);
    }

    if (pFile->searchSpace.pFileInfo)
    {
        SrvFreeMemory(pFile->searchSpace.pFileInfo);
    }

    if (pFile->resource.ulResourceId)
    {
        PSRV_RESOURCE pResource = NULL;

        SrvElementsUnregisterResource(pFile->resource.ulResourceId, &pResource);
        pFile->resource.ulResourceId = 0;
    }

    // Release parent at the end
    if (pFile->pTree)
    {
        SrvTree2Release(pFile->pTree);
    }

    SrvFreeMemory(pFile);
}

static
BOOLEAN
SrvFile2IsRundown_inlock(
    PLWIO_SRV_FILE_2 pFile
    )
{
    return IsSetFlag(pFile->objectFlags, SRV_OBJECT_FLAG_RUNDOWN);
}

static
VOID
SrvFile2SetRundown_inlock(
    PLWIO_SRV_FILE_2 pFile
    )
{
    LWIO_ASSERT(!IsSetFlag(pFile->objectFlags, SRV_OBJECT_FLAG_RUNDOWN));
    SetFlag(pFile->objectFlags, SRV_OBJECT_FLAG_RUNDOWN);
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
