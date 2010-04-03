/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * */

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
 *        files.c
 *
 * Abstract:
 *
 *        Likewise File System Driver (Srv)
 *
 *        DeviceIo Dispatch Routine
 *
 *        File Management
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 */

#include "includes.h"

static
NTSTATUS
SrvProtocolValidateFileInfoLevel(
    ULONG ulInfoLevel
    );

static
NTSTATUS
SrvProtocolCountAllFiles(
    PSRV_RESOURCE pResource,
    PVOID         pUserData,
    PBOOLEAN      pbContinue
    );

static
NTSTATUS
SrvProtocolEnumAllFiles(
    PSRV_RESOURCE pResource,
    PVOID         pUserData,
    PBOOLEAN      pbContinue
    );

static
NTSTATUS
SrvProtocolFindFile(
    PSRV_RESOURCE pResource,
    PVOID         pUserData,
    PBOOLEAN      pbContinue
    );

static
NTSTATUS
SrvProtocolUpdateQueryConnection(
    PSRV_RESOURCE                 pResource,
    PSRV_PROTOCOL_FILE_ENUM_QUERY pFileEnumQuery
    );

static
NTSTATUS
SrvProtocolFindConnection(
    ULONG                 ulResourceId,
    PLWIO_SRV_CONNECTION* ppConnection
    );

static
NTSTATUS
SrvProtocolUpdateQuerySession(
    PSRV_RESOURCE                 pResource,
    PSRV_PROTOCOL_FILE_ENUM_QUERY pFileEnumQuery
    );

static
NTSTATUS
SrvProtocolUpdateQueryTree(
    PSRV_RESOURCE                 pResource,
    PSRV_PROTOCOL_FILE_ENUM_QUERY pFileEnumQuery
    );

static
NTSTATUS
SrvProtocolProcessFile(
    ULONG             ulResourceId,
    PLWIO_SRV_SESSION pSession,
    PLWIO_SRV_TREE    pTree,
    PUSHORT           pUsFid,
    ULONG             ulInfoLevel,
    PBYTE             pBuffer,
    ULONG             ulBufferSize,
    PULONG            pulBytesUsed
    );

static
NTSTATUS
SrvProtocolProcessFile2(
    ULONG               ulResourceId,
    PLWIO_SRV_SESSION_2 pSession,
    PLWIO_SRV_TREE_2    pTree,
    PSMB2_FID           pFid,
    ULONG               ulInfoLevel,
    PBYTE               pBuffer,
    ULONG               ulBufferSize,
    PULONG              pulBytesUsed
    );

static
NTSTATUS
SrvProtocolProcessFile_level_2(
    ULONG  ulResourceId,
    PBYTE  pBuffer,
    ULONG  ulBufferSize,
    PULONG pulBytesUsed
    );

static
NTSTATUS
SrvProtocolProcessFile_level_3(
    ULONG  ulResourceId,
    ULONG  ulPermissions,
    ULONG  ulNumLocks,
    PWSTR  pwszPathname,
    PWSTR  pwszUsername,
    PBYTE  pBuffer,
    ULONG  ulBufferSize,
    PULONG pulBytesUsed
    );

static
VOID
SrvProtocolClearFileQueryContents(
    PSRV_PROTOCOL_FILE_ENUM_QUERY pFileEnumQuery
    );

NTSTATUS
SrvProtocolEnumerateFiles(
    PWSTR  pwszBasepath,
    PWSTR  pwszUsername,
    ULONG  ulInfoLevel,
    PBYTE  pBuffer,
    ULONG  ulBufferSize,
    PULONG pulBytesUsed,
    PULONG pulEntriesRead,
    PULONG pulTotalEntries,
    PULONG pulResumeHandle
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    SRV_PROTOCOL_FILE_ENUM_QUERY fileEnumQuery =
    {
            .pwszBasepath   = pwszBasepath,
            .pwszUsername   = pwszUsername,
            .ulInfoLevel    = ulInfoLevel,
            .iEntryIndex    = 0,
            .iResumeIndex   = pulResumeHandle ? *pulResumeHandle : 0,
            .ulEntriesRead  = 0,
            .ulTotalEntries = 0,
            .pBuffer        = pBuffer,
            .ulBufferSize   = ulBufferSize,
            .ulBytesUsed    = 0
    };

    ntStatus = SrvProtocolValidateFileInfoLevel(fileEnumQuery.ulInfoLevel);
    BAIL_ON_NT_STATUS(ntStatus);

    if (!fileEnumQuery.pwszUsername && !fileEnumQuery.pwszBasepath)
    {
        ntStatus = SrvElementsEnumResources(
                        SRV_RESOURCE_TYPE_FILE,
                        &SrvProtocolCountAllFiles,
                        &fileEnumQuery);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvElementsEnumResources(
                        SRV_RESOURCE_TYPE_FILE,
                        &SrvProtocolEnumAllFiles,
                        &fileEnumQuery);
        BAIL_ON_NT_STATUS(ntStatus);
    }
    else
    {
        ntStatus = STATUS_NOT_SUPPORTED;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *pulBytesUsed    = fileEnumQuery.ulBytesUsed;
    *pulEntriesRead  = fileEnumQuery.ulEntriesRead;
    *pulTotalEntries = fileEnumQuery.ulTotalEntries;
    if (pulResumeHandle)
    {
        *pulResumeHandle =
                fileEnumQuery.iResumeIndex + fileEnumQuery.ulEntriesRead;
    }

cleanup:

    SrvProtocolClearFileQueryContents(&fileEnumQuery);

    return ntStatus;

error:

    *pulBytesUsed    = 0;
    *pulEntriesRead  = 0;
    *pulTotalEntries = 0;

    if (pBuffer && ulBufferSize)
    {
        memset(pBuffer, 0, ulBufferSize);
    }

    goto cleanup;
}

NTSTATUS
SrvProtocolGetFileInfo(
    ULONG  ulInfoLevel,       /* IN              */
    ULONG  ulFileId,          /* IN              */
    PBYTE  pBuffer,           /* IN              */
    ULONG  ulBufferSize,      /* IN              */
    PULONG pulBytesUsed       /* IN OUT          */
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    SRV_PROTOCOL_FILE_ENUM_QUERY fileEnumQuery =
    {
            .pwszBasepath   = NULL,
            .pwszUsername   = NULL,
            .ulInfoLevel    = ulInfoLevel,
            .iEntryIndex    = 0,
            .iResumeIndex   = 0,
            .ulEntriesRead  = 0,
            .ulTotalEntries = 0,
            .pBuffer        = pBuffer,
            .ulBufferSize   = ulBufferSize,
            .ulBytesUsed    = 0
    };

    ntStatus = SrvProtocolValidateFileInfoLevel(fileEnumQuery.ulInfoLevel);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvElementsFindResource(
                    ulFileId,
                    SRV_RESOURCE_TYPE_FILE,
                    &SrvProtocolFindFile,
                    &fileEnumQuery);
    BAIL_ON_NT_STATUS(ntStatus);

    *pulBytesUsed = fileEnumQuery.ulBytesUsed;

cleanup:

    SrvProtocolClearFileQueryContents(&fileEnumQuery);

    return ntStatus;

error:

    *pulBytesUsed = 0;

    if (pBuffer && ulBufferSize)
    {
        memset(pBuffer, 0, ulBufferSize);
    }

    if (ntStatus == STATUS_NOT_FOUND)
    {
        ntStatus = STATUS_NO_SUCH_FILE;
    }

    goto cleanup;
}

static
NTSTATUS
SrvProtocolValidateFileInfoLevel(
    ULONG ulInfoLevel
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    switch (ulInfoLevel)
    {
        case 2:
        case 3:

            break;

        default:

            ntStatus = STATUS_INVALID_INFO_CLASS;

            break;
    }

    return ntStatus;
}

static
NTSTATUS
SrvProtocolCountAllFiles(
    PSRV_RESOURCE pResource,
    PVOID         pUserData,
    PBOOLEAN      pbContinue
    )
{
    NTSTATUS      ntStatus  = STATUS_SUCCESS;
    PSRV_PROTOCOL_FILE_ENUM_QUERY pFileEnumQuery =
                                    (PSRV_PROTOCOL_FILE_ENUM_QUERY)pUserData;
    BOOLEAN bContinue = TRUE;

    if (pFileEnumQuery->ulTotalEntries == UINT32_MAX)
    {
        bContinue = FALSE;
    }
    else
    {
        pFileEnumQuery->ulTotalEntries++;
    }

    *pbContinue = bContinue;

    return ntStatus;
}

static
NTSTATUS
SrvProtocolEnumAllFiles(
    PSRV_RESOURCE pResource,
    PVOID         pUserData,
    PBOOLEAN      pbContinue
    )
{
    NTSTATUS      ntStatus  = STATUS_SUCCESS;
    PSRV_PROTOCOL_FILE_ENUM_QUERY pFileEnumQuery =
                                    (PSRV_PROTOCOL_FILE_ENUM_QUERY)pUserData;
    BOOLEAN bContinue = TRUE;

    if (pFileEnumQuery->iEntryIndex < pFileEnumQuery->iResumeIndex)
    {
        pFileEnumQuery->iEntryIndex++;
        pResource = NULL; // Skip
    }

    if (pFileEnumQuery->iEntryIndex == UINT32_MAX)
    {
        bContinue = FALSE;
        pResource = NULL;
    }

    if (pResource)
    {
        ULONG ulBytesUsed = 0;

        ntStatus = SrvProtocolUpdateQueryConnection(pResource, pFileEnumQuery);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvProtocolUpdateQuerySession(pResource, pFileEnumQuery);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvProtocolUpdateQueryTree(pResource, pFileEnumQuery);
        BAIL_ON_NT_STATUS(ntStatus);

        switch (pResource->pAttributes->protocolVersion)
        {
            case SMB_PROTOCOL_VERSION_1:

                ntStatus = SrvProtocolProcessFile(
                                pResource->ulResourceId,
                                pFileEnumQuery->pSession,
                                pFileEnumQuery->pTree,
                                pResource->pAttributes->fileId.pFid1,
                                pFileEnumQuery->ulInfoLevel,
                                pFileEnumQuery->pBuffer,
                                pFileEnumQuery->ulBufferSize,
                                &ulBytesUsed);

                break;

            case SMB_PROTOCOL_VERSION_2:

                ntStatus = SrvProtocolProcessFile2(
                                pResource->ulResourceId,
                                pFileEnumQuery->pSession2,
                                pFileEnumQuery->pTree2,
                                pResource->pAttributes->fileId.pFid2,
                                pFileEnumQuery->ulInfoLevel,
                                pFileEnumQuery->pBuffer,
                                pFileEnumQuery->ulBufferSize,
                                &ulBytesUsed);

                break;

            default:

                ntStatus = STATUS_INTERNAL_ERROR;

                break;
        }
        BAIL_ON_NT_STATUS(ntStatus);

        pFileEnumQuery->pBuffer      += ulBytesUsed;
        pFileEnumQuery->ulBufferSize -= ulBytesUsed;
        pFileEnumQuery->ulBytesUsed  += ulBytesUsed;

        pFileEnumQuery->iEntryIndex++;
        pFileEnumQuery->ulEntriesRead++;
    }

    *pbContinue = bContinue;

cleanup:

    return ntStatus;

error:

    *pbContinue = FALSE;

    if (ntStatus == STATUS_END_OF_FILE)
    {
        ntStatus = STATUS_SUCCESS;
    }

    goto cleanup;
}

static
NTSTATUS
SrvProtocolFindFile(
    PSRV_RESOURCE pResource,
    PVOID         pUserData,
    PBOOLEAN      pbContinue
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_PROTOCOL_FILE_ENUM_QUERY pFileEnumQuery =
                                    (PSRV_PROTOCOL_FILE_ENUM_QUERY)pUserData;
    ULONG ulBytesUsed = 0;

    ntStatus = SrvProtocolUpdateQueryConnection(pResource, pFileEnumQuery);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvProtocolUpdateQuerySession(pResource, pFileEnumQuery);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvProtocolUpdateQueryTree(pResource, pFileEnumQuery);
    BAIL_ON_NT_STATUS(ntStatus);

    switch (pResource->pAttributes->protocolVersion)
    {
        case SMB_PROTOCOL_VERSION_1:

            ntStatus = SrvProtocolProcessFile(
                            pResource->ulResourceId,
                            pFileEnumQuery->pSession,
                            pFileEnumQuery->pTree,
                            pResource->pAttributes->fileId.pFid1,
                            pFileEnumQuery->ulInfoLevel,
                            pFileEnumQuery->pBuffer,
                            pFileEnumQuery->ulBufferSize,
                            &ulBytesUsed);

            break;

        case SMB_PROTOCOL_VERSION_2:

            ntStatus = SrvProtocolProcessFile2(
                            pResource->ulResourceId,
                            pFileEnumQuery->pSession2,
                            pFileEnumQuery->pTree2,
                            pResource->pAttributes->fileId.pFid2,
                            pFileEnumQuery->ulInfoLevel,
                            pFileEnumQuery->pBuffer,
                            pFileEnumQuery->ulBufferSize,
                            &ulBytesUsed);

            break;

        default:

            ntStatus = STATUS_INTERNAL_ERROR;

            break;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    pFileEnumQuery->pBuffer      += ulBytesUsed;
    pFileEnumQuery->ulBufferSize -= ulBytesUsed;
    pFileEnumQuery->ulBytesUsed  += ulBytesUsed;

    pFileEnumQuery->iEntryIndex++;
    pFileEnumQuery->ulEntriesRead++;

cleanup:

    *pbContinue = FALSE;

    return ntStatus;

error:

    if (ntStatus == STATUS_END_OF_FILE)
    {
        ntStatus = STATUS_BUFFER_TOO_SMALL;
    }

    goto cleanup;
}

static
NTSTATUS
SrvProtocolUpdateQueryConnection(
    PSRV_RESOURCE                 pResource,
    PSRV_PROTOCOL_FILE_ENUM_QUERY pFileEnumQuery
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (pFileEnumQuery->pConnection &&
        (pFileEnumQuery->pConnection->resource.ulResourceId !=
                        pResource->pAttributes->ulConnectionResourceId))
    {
        SrvConnectionRelease(pFileEnumQuery->pConnection);
        pFileEnumQuery->pConnection = NULL;
    }

    if (!pFileEnumQuery->pConnection)
    {
        ntStatus = SrvProtocolFindConnection(
                        pResource->pAttributes->ulConnectionResourceId,
                        &pFileEnumQuery->pConnection);
        BAIL_ON_NT_STATUS(ntStatus);
    }

error:

    return ntStatus;
}

static
NTSTATUS
SrvProtocolFindConnection(
    ULONG                 ulResourceId,
    PLWIO_SRV_CONNECTION* ppConnection
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN  bInLock  = FALSE;
    PLWIO_SRV_CONNECTION pConnection = NULL;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &gProtocolApiGlobals.mutex);

    ntStatus = LwRtlRBTreeFind(
                    gProtocolApiGlobals.pConnections,
                    &ulResourceId,
                    (PVOID*)&pConnection);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppConnection = SrvConnectionAcquire(pConnection);

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &gProtocolApiGlobals.mutex);

    return ntStatus;

error:

    *ppConnection = NULL;

    goto cleanup;
}

static
NTSTATUS
SrvProtocolUpdateQuerySession(
    PSRV_RESOURCE                 pResource,
    PSRV_PROTOCOL_FILE_ENUM_QUERY pFileEnumQuery
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    switch (pResource->pAttributes->protocolVersion)
    {
        case SMB_PROTOCOL_VERSION_1:

            if (pFileEnumQuery->pSession &&
                (pFileEnumQuery->pSession->uid !=
                                pResource->pAttributes->sessionId.usUid))
            {
                SrvSessionRelease(pFileEnumQuery->pSession);
                pFileEnumQuery->pSession = NULL;
            }

            if (!pFileEnumQuery->pSession)
            {
                ntStatus = SrvConnectionFindSession(
                                pFileEnumQuery->pConnection,
                                pResource->pAttributes->sessionId.usUid,
                                &pFileEnumQuery->pSession);
                BAIL_ON_NT_STATUS(ntStatus);
            }

            break;

        case SMB_PROTOCOL_VERSION_2:

            if (pFileEnumQuery->pSession2 &&
                (pFileEnumQuery->pSession2->ullUid !=
                                pResource->pAttributes->sessionId.ullUid))
            {
                SrvSession2Release(pFileEnumQuery->pSession2);
                pFileEnumQuery->pSession2 = NULL;
            }

            if (!pFileEnumQuery->pSession2)
            {
                ntStatus = SrvConnection2FindSession(
                                pFileEnumQuery->pConnection,
                                pResource->pAttributes->sessionId.ullUid,
                                &pFileEnumQuery->pSession2);
                BAIL_ON_NT_STATUS(ntStatus);
            }

            break;

        default:

            ntStatus = STATUS_INTERNAL_ERROR;

            break;

    }

error:

    return ntStatus;
}

static
NTSTATUS
SrvProtocolUpdateQueryTree(
    PSRV_RESOURCE                 pResource,
    PSRV_PROTOCOL_FILE_ENUM_QUERY pFileEnumQuery
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    switch (pResource->pAttributes->protocolVersion)
    {
        case SMB_PROTOCOL_VERSION_1:

            if (pFileEnumQuery->pTree &&
                (pFileEnumQuery->pTree->tid !=
                                pResource->pAttributes->treeId.usTid))
            {
                SrvTreeRelease(pFileEnumQuery->pTree);
                pFileEnumQuery->pTree = NULL;
            }

            if (!pFileEnumQuery->pTree)
            {
                ntStatus = SrvSessionFindTree(
                                pFileEnumQuery->pSession,
                                pResource->pAttributes->treeId.usTid,
                                &pFileEnumQuery->pTree);
                BAIL_ON_NT_STATUS(ntStatus);
            }

            break;

        case SMB_PROTOCOL_VERSION_2:

            if (pFileEnumQuery->pTree2 &&
                (pFileEnumQuery->pTree2->ullUid !=
                                pResource->pAttributes->treeId.ulTid))
            {
                SrvTree2Release(pFileEnumQuery->pTree2);
                pFileEnumQuery->pTree2 = NULL;
            }

            if (!pFileEnumQuery->pTree2)
            {
                ntStatus = SrvSession2FindTree(
                                pFileEnumQuery->pSession2,
                                pResource->pAttributes->treeId.ulTid,
                                &pFileEnumQuery->pTree2);
                BAIL_ON_NT_STATUS(ntStatus);
            }

            break;

        default:

            ntStatus = STATUS_INTERNAL_ERROR;

            break;

    }

error:

    return ntStatus;
}

static
NTSTATUS
SrvProtocolProcessFile(
    ULONG             ulResourceId,
    PLWIO_SRV_SESSION pSession,
    PLWIO_SRV_TREE    pTree,
    PUSHORT           pUsFid,
    ULONG             ulInfoLevel,
    PBYTE             pBuffer,
    ULONG             ulBufferSize,
    PULONG            pulBytesUsed
    )
{
    NTSTATUS       ntStatus       = STATUS_SUCCESS;
    PLWIO_SRV_FILE pFile          = NULL;
    ULONG          ulBytesUsed    = 0;
    BOOLEAN        bSessionInLock = FALSE;
    BOOLEAN        bFileInLock    = FALSE;

    ntStatus = SrvTreeFindFile(pTree, *pUsFid, &pFile);
    BAIL_ON_NT_STATUS(ntStatus);

    switch (ulInfoLevel)
    {
        case 2:

            ntStatus = SrvProtocolProcessFile_level_2(
                            ulResourceId,
                            pBuffer,
                            ulBufferSize,
                            &ulBytesUsed);

            break;

        case 3:

            LWIO_LOCK_RWMUTEX_SHARED(bSessionInLock, &pSession->mutex);
            LWIO_LOCK_RWMUTEX_SHARED(bFileInLock, &pFile->mutex);

            ntStatus = SrvProtocolProcessFile_level_3(
                            ulResourceId,
                            pFile->ulPermissions,
                            pFile->ulNumLocks,
                            pFile->pwszFilename,
                            pSession->pwszClientPrincipalName,
                            pBuffer,
                            ulBufferSize,
                            &ulBytesUsed);

            break;

        default:

            ntStatus = STATUS_INVALID_INFO_CLASS;

            break;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    *pulBytesUsed = ulBytesUsed;

cleanup:

    LWIO_UNLOCK_RWMUTEX(bFileInLock, &pFile->mutex);
    LWIO_UNLOCK_RWMUTEX(bSessionInLock, &pSession->mutex);

    if (pFile)
    {
        SrvFileRelease(pFile);
    }

    return ntStatus;

error:

    *pulBytesUsed = 0;

    goto cleanup;
}

static
NTSTATUS
SrvProtocolProcessFile2(
    ULONG               ulResourceId,
    PLWIO_SRV_SESSION_2 pSession,
    PLWIO_SRV_TREE_2    pTree,
    PSMB2_FID           pFid,
    ULONG               ulInfoLevel,
    PBYTE               pBuffer,
    ULONG               ulBufferSize,
    PULONG              pulBytesUsed
    )
{
    NTSTATUS         ntStatus       = STATUS_SUCCESS;
    PLWIO_SRV_FILE_2 pFile          = NULL;
    ULONG            ulBytesUsed    = 0;
    BOOLEAN          bSessionInLock = FALSE;
    BOOLEAN          bFileInLock    = FALSE;

    ntStatus = SrvTree2FindFile(pTree, pFid, &pFile);
    BAIL_ON_NT_STATUS(ntStatus);

    switch (ulInfoLevel)
    {
        case 2:

            ntStatus = SrvProtocolProcessFile_level_2(
                            ulResourceId,
                            pBuffer,
                            ulBufferSize,
                            &ulBytesUsed);

            break;

        case 3:

            LWIO_LOCK_RWMUTEX_SHARED(bSessionInLock, &pSession->mutex);
            LWIO_LOCK_RWMUTEX_SHARED(bFileInLock, &pFile->mutex);

            ntStatus = SrvProtocolProcessFile_level_3(
                            ulResourceId,
                            pFile->ulPermissions,
                            pFile->ulNumLocks,
                            pFile->pwszFilename,
                            pSession->pwszClientPrincipalName,
                            pBuffer,
                            ulBufferSize,
                            &ulBytesUsed);

        default:

            ntStatus = STATUS_INVALID_INFO_CLASS;

            break;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    *pulBytesUsed = ulBytesUsed;

cleanup:

    LWIO_UNLOCK_RWMUTEX(bFileInLock, &pFile->mutex);
    LWIO_UNLOCK_RWMUTEX(bSessionInLock, &pSession->mutex);

    if (pFile)
    {
        SrvFile2Release(pFile);
    }

    return ntStatus;

error:

    *pulBytesUsed = 0;

    goto cleanup;
}

static
NTSTATUS
SrvProtocolProcessFile_level_2(
    ULONG  ulResourceId,
    PBYTE  pBuffer,
    ULONG  ulBufferSize,
    PULONG pulBytesUsed
    )
{
    NTSTATUS    ntStatus    = STATUS_SUCCESS;
    ULONG       ulBytesUsed = 0;
    FILE_INFO_2 fileInfo    = {0};

    fileInfo.fi2_id = ulResourceId;

    ntStatus = LwFileInfoMarshalEnumOutputInfo_level_2(
                    &fileInfo,
                    pBuffer,
                    ulBufferSize,
                    &ulBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    *pulBytesUsed = ulBytesUsed;

cleanup:

    return ntStatus;

error:

    *pulBytesUsed = 0;

    goto cleanup;
}

static
NTSTATUS
SrvProtocolProcessFile_level_3(
    ULONG  ulResourceId,
    ULONG  ulPermissions,
    ULONG  ulNumLocks,
    PWSTR  pwszPathname,
    PWSTR  pwszUsername,
    PBYTE  pBuffer,
    ULONG  ulBufferSize,
    PULONG pulBytesUsed
    )
{
    NTSTATUS    ntStatus    = STATUS_SUCCESS;
    ULONG       ulBytesUsed = 0;
    FILE_INFO_3 fileInfo    =
    {
        .fi3_idd         = ulResourceId,
        .fi3_permissions = ulPermissions,
        .fi3_num_locks   = ulNumLocks,
        .fi3_path_name   = pwszPathname,
        .fi3_username    = pwszUsername
    };

    ntStatus = LwFileInfoMarshalEnumOutputInfo_level_3(
                    &fileInfo,
                    pBuffer,
                    ulBufferSize,
                    &ulBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    *pulBytesUsed = ulBytesUsed;

cleanup:

    return ntStatus;

error:

    *pulBytesUsed = 0;

    goto cleanup;
}

static
VOID
SrvProtocolClearFileQueryContents(
    PSRV_PROTOCOL_FILE_ENUM_QUERY pFileEnumQuery
    )
{
    if (pFileEnumQuery->pTree)
    {
        SrvTreeRelease(pFileEnumQuery->pTree);
    }
    if (pFileEnumQuery->pTree2)
    {
        SrvTree2Release(pFileEnumQuery->pTree2);
    }
    if (pFileEnumQuery->pSession)
    {
        SrvSessionRelease(pFileEnumQuery->pSession);
    }
    if (pFileEnumQuery->pSession2)
    {
        SrvSession2Release(pFileEnumQuery->pSession2);
    }
    if (pFileEnumQuery->pConnection)
    {
        SrvConnectionRelease(pFileEnumQuery->pConnection);
    }
}


