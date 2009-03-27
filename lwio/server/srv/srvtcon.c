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

#include "includes.h"

static
NTSTATUS
SrvGetShareName(
    PCSTR  pszHostname,
    PCSTR  pszDomain,
    PWSTR  pwszPath,
    PWSTR* ppwszSharename
    );

static
NTSTATUS
SrvGetShareNameCheckHostname(
    PCSTR  pszHostname,
    PCSTR  pszDomain,
    PWSTR  pwszPath,
    PWSTR* ppwszSharename
    );

static
NTSTATUS
SrvGetShareNameCheckFQDN(
    PCSTR  pszHostname,
    PCSTR  pszDomain,
    PWSTR  pwszPath,
    PWSTR* ppwszSharename
    );

static
NTSTATUS
SrvBuildTreeConnectResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    PSMB_SRV_TREE       pTree,
    PSMB_PACKET*        ppSmbResponse
    );

static
NTSTATUS
SrvGetMaximalShareAccessMask(
    PSHARE_DB_INFO pShareInfo,
    ACCESS_MASK*   pMask
    );

static
NTSTATUS
SrvGetGuestShareAccessMask(
    PSHARE_DB_INFO pShareInfo,
    ACCESS_MASK*   pMask
    );

static
NTSTATUS
SrvGetServiceName(
    PSHARE_DB_INFO pShareInfo,
    PSTR* ppszService
    );

static
NTSTATUS
SrvGetNativeFilesystem(
    PSHARE_DB_INFO pShareInfo,
    PWSTR* ppwszNativeFilesystem
    );

NTSTATUS
SrvProcessTreeConnectAndX(
    PLWIO_SRV_CONTEXT pContext,
    PSMB_PACKET*      ppSmbResponse
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_SRV_CONNECTION pConnection = pContext->pConnection;
    PSMB_PACKET pSmbRequest = pContext->pRequest;
    PSMB_PACKET pSmbResponse = NULL;
    PSMB_SRV_SESSION pSession = NULL;
    PSMB_SRV_TREE pTree = NULL;
    BOOLEAN       bRemoveTreeFromSession = FALSE;
    PSHARE_DB_INFO pShareInfo = NULL;
    ULONG ulOffset = 0;
    TREE_CONNECT_REQUEST_HEADER* pRequestHeader = NULL; // Do not free
    uint8_t* pszPassword = NULL; // Do not free
    uint8_t* pszService = NULL; // Do not free
    PWSTR    pwszPath = NULL; // Do not free
    PWSTR    pwszSharename = NULL;
    BOOLEAN  bInLock = FALSE;

    ntStatus = SrvConnectionFindSession(
                    pConnection,
                    pSmbRequest->pSMBHeader->uid,
                    &pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    ulOffset = (PBYTE)pSmbRequest->pParams - (PBYTE)pSmbRequest->pSMBHeader;

    ntStatus = UnmarshallTreeConnectRequest(
                    pSmbRequest->pParams,
                    pSmbRequest->pNetBIOSHeader->len - ulOffset,
                    ulOffset,
                    &pRequestHeader,
                    &pszPassword,
                    &pwszPath,
                    &pszService);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pRequestHeader->flags & 0x1)
    {
        NTSTATUS ntStatus2 = 0;

        ntStatus2 = SrvSessionRemoveTree(
                        pSession,
                        pSmbRequest->pSMBHeader->tid);
        if (ntStatus2)
        {
            SMB_LOG_ERROR("Failed to remove tid [%u] from session [uid=%u]. [code:%d]",
                            pSmbRequest->pSMBHeader->tid,
                            pSmbRequest->pSMBHeader->uid,
                            ntStatus2);
        }
    }

    SMB_LOCK_RWMUTEX_SHARED(bInLock, &pConnection->pHostinfo->mutex);

    ntStatus = SrvGetShareName(
                    pConnection->pHostinfo->pszHostname,
                    pConnection->pHostinfo->pszDomain,
                    pwszPath,
                    &pwszSharename);
    BAIL_ON_NT_STATUS(ntStatus);

    SMB_UNLOCK_RWMUTEX(bInLock, &pConnection->pHostinfo->mutex);

    ntStatus = SrvFindShareByName(
                    pConnection->pShareDbContext,
                    pwszSharename,
                    &pShareInfo);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvSessionCreateTree(
                    pSession,
                    pShareInfo,
                    &pTree);
    BAIL_ON_NT_STATUS(ntStatus);

    bRemoveTreeFromSession = TRUE;

    ntStatus = SrvBuildTreeConnectResponse(
                    pConnection,
                    pSmbRequest,
                    pTree,
                    &pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppSmbResponse = pSmbResponse;

cleanup:

    SMB_UNLOCK_RWMUTEX(bInLock, &pConnection->pHostinfo->mutex);

    if (pSession)
    {
        SrvSessionRelease(pSession);
    }

    if (pTree)
    {
        SrvTreeRelease(pTree);
    }

    if (pShareInfo)
    {
        SrvShareDbReleaseInfo(pShareInfo);
    }

    if (pwszSharename)
    {
        LwRtlMemoryFree(pwszSharename);
    }

    return (ntStatus);

error:

    *ppSmbResponse = NULL;

    if (bRemoveTreeFromSession)
    {
        NTSTATUS ntStatus2 = 0;

        ntStatus2 = SrvSessionRemoveTree(
                        pSession,
                        pSmbRequest->pSMBHeader->tid);
        if (ntStatus2)
        {
            SMB_LOG_ERROR("Failed to remove tid [%u] from session [uid=%u][code:%d]",
                            pSmbRequest->pSMBHeader->tid,
                            pSmbRequest->pSMBHeader->uid,
                            ntStatus2);
        }
    }

    if (pSmbResponse)
    {
        SMBPacketFree(
            pConnection->hPacketAllocator,
            pSmbResponse);
    }

    goto cleanup;
}

static
NTSTATUS
SrvGetShareName(
    PCSTR pszHostname,
    PCSTR pszDomain,
    PWSTR pwszPath,
    PWSTR* ppwszSharename
    )
{
    NTSTATUS ntStatus = 0;
    PWSTR    pwszSharename = NULL;

    ntStatus = SrvGetShareNameCheckHostname(
                    pszHostname,
                    pszDomain,
                    pwszPath,
                    &pwszSharename);
    if (ntStatus)
    {
        ntStatus = SrvGetShareNameCheckFQDN(
                        pszHostname,
                        pszDomain,
                        pwszPath,
                        &pwszSharename);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppwszSharename = pwszSharename;

cleanup:

    return ntStatus;

error:

    *ppwszSharename = NULL;

    goto cleanup;
}

static
NTSTATUS
SrvGetShareNameCheckHostname(
    PCSTR  pszHostname,
    PCSTR  pszDomain,
    PWSTR  pwszPath,
    PWSTR* ppwszSharename
    )
{
    NTSTATUS  ntStatus = 0;
    PSTR      pszHostPrefix = NULL;
    PWSTR     pwszHostPrefix = NULL;
    PWSTR     pwszPath_copy = NULL;
    PWSTR     pwszSharename = NULL;
    size_t    len = 0, len_prefix = 0, len_sharename = 0;

    len = wc16slen(pwszPath);
    if (!len)
    {
        ntStatus = STATUS_OBJECT_PATH_INVALID;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = LW_RTL_ALLOCATE(
                    &pwszPath_copy,
                    WCHAR,
                    (len + 1) * sizeof(wchar16_t));
    BAIL_ON_NT_STATUS(ntStatus);

    memcpy(pwszPath_copy, pwszPath, len * sizeof(wchar16_t));

    wc16supper(pwszPath_copy);

    ntStatus = SMBAllocateStringPrintf(
                    &pszHostPrefix,
                    "\\\\%s\\",
                    pszHostname,
                    pszDomain);
    BAIL_ON_NT_STATUS(ntStatus);

    SMBStrToUpper(pszHostPrefix);

    ntStatus = SMBMbsToWc16s(
                    pszHostPrefix,
                    &pwszHostPrefix);
    BAIL_ON_NT_STATUS(ntStatus);

    len_prefix = wc16slen(pwszHostPrefix);
    if (len <= len_prefix)
    {
        ntStatus = STATUS_OBJECT_PATH_INVALID;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (memcmp((PBYTE)pwszPath_copy, (PBYTE)pwszHostPrefix, len_prefix * sizeof(wchar16_t)) != 0)
    {
        ntStatus = STATUS_OBJECT_PATH_INVALID;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    len_sharename = wc16slen(pwszPath_copy + len_prefix);
    if (!len_sharename)
    {
        ntStatus = STATUS_OBJECT_PATH_INVALID;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = LW_RTL_ALLOCATE(
                    &pwszSharename,
                    WCHAR,
                    (len_sharename + 1) * sizeof(wchar16_t));
    BAIL_ON_NT_STATUS(ntStatus);

    // copy from original path
    memcpy((PBYTE)pwszSharename, (PBYTE)pwszPath + len_prefix * sizeof(wchar16_t), len_sharename * sizeof(wchar16_t));

    *ppwszSharename = pwszSharename;

cleanup:

    if (pszHostPrefix)
    {
        LwRtlMemoryFree(pszHostPrefix);
    }
    if (pwszHostPrefix)
    {
        LwRtlMemoryFree(pwszHostPrefix);
    }
    if (pwszPath_copy)
    {
        LwRtlMemoryFree(pwszPath_copy);
    }

    return ntStatus;

error:

    *ppwszSharename = NULL;

    if (pwszSharename)
    {
        LwRtlMemoryFree(pwszSharename);
    }

    goto cleanup;
}

static
NTSTATUS
SrvGetShareNameCheckFQDN(
    PCSTR  pszHostname,
    PCSTR  pszDomain,
    PWSTR  pwszPath,
    PWSTR* ppwszSharename
    )
{
    NTSTATUS  ntStatus = 0;
    PSTR      pszHostPrefix = NULL;
    PWSTR     pwszHostPrefix = NULL;
    PWSTR     pwszPath_copy = NULL;
    PWSTR     pwszSharename = NULL;
    size_t    len = 0, len_prefix = 0, len_sharename = 0;

    len = wc16slen(pwszPath);
    if (!len)
    {
        ntStatus = STATUS_OBJECT_PATH_INVALID;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = LW_RTL_ALLOCATE(
                    &pwszPath_copy,
                    WCHAR,
                    (len + 1) * sizeof(wchar16_t));
    BAIL_ON_NT_STATUS(ntStatus);

    memcpy(pwszPath_copy, pwszPath, len * sizeof(wchar16_t));

    wc16supper(pwszPath_copy);

    ntStatus = SMBAllocateStringPrintf(
                    &pszHostPrefix,
                    "\\\\%s.%s\\",
                    pszHostname,
                    pszDomain);
    BAIL_ON_NT_STATUS(ntStatus);

    SMBStrToUpper(pszHostPrefix);

    ntStatus = SMBMbsToWc16s(
                    pszHostPrefix,
                    &pwszHostPrefix);
    BAIL_ON_NT_STATUS(ntStatus);

    len_prefix = wc16slen(pwszHostPrefix);
    if (len <= len_prefix)
    {
        ntStatus = STATUS_OBJECT_PATH_INVALID;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (memcmp((PBYTE)pwszPath_copy, (PBYTE)pwszHostPrefix, len_prefix * sizeof(wchar16_t)) != 0)
    {
        ntStatus = STATUS_OBJECT_PATH_INVALID;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    len_sharename = wc16slen(pwszPath_copy + len_prefix);
    if (!len_sharename)
    {
        ntStatus = STATUS_OBJECT_PATH_INVALID;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = LW_RTL_ALLOCATE(
                    &pwszSharename,
                    WCHAR,
                    (len_sharename + 1) * sizeof(wchar16_t));
    BAIL_ON_NT_STATUS(ntStatus);

    // copy from original path
    memcpy((PBYTE)pwszSharename, (PBYTE)pwszPath + len_prefix * sizeof(wchar16_t), len_sharename * sizeof(wchar16_t));

    *ppwszSharename = pwszSharename;

cleanup:

    if (pszHostPrefix)
    {
        LwRtlMemoryFree(pszHostPrefix);
    }
    if (pwszHostPrefix)
    {
        LwRtlMemoryFree(pwszHostPrefix);
    }
    if (pwszPath_copy)
    {
        LwRtlMemoryFree(pwszPath_copy);
    }

    return ntStatus;

error:

    *ppwszSharename = NULL;

    if (pwszSharename)
    {
        LwRtlMemoryFree(pwszSharename);
    }

    goto cleanup;
}

static
NTSTATUS
SrvBuildTreeConnectResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    PSMB_SRV_TREE       pTree,
    PSMB_PACKET*        ppSmbResponse
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_PACKET pSmbResponse = NULL;
    PTREE_CONNECT_RESPONSE_HEADER pResponseHeader = NULL;
    ULONG  packetByteCount = 0;
    PSTR   pszService = NULL;
    PWSTR  pwszNativeFileSystem = NULL;

    ntStatus = SMBPacketAllocate(
                    pConnection->hPacketAllocator,
                    &pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketBufferAllocate(
                    pConnection->hPacketAllocator,
                    64 * 1024,
                    &pSmbResponse->pRawBuffer,
                    &pSmbResponse->bufferLen);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketMarshallHeader(
                pSmbResponse->pRawBuffer,
                pSmbResponse->bufferLen,
                COM_TREE_CONNECT_ANDX,
                0,
                TRUE,
                pTree->tid,
                pSmbRequest->pSMBHeader->pid,
                pSmbRequest->pSMBHeader->uid,
                pSmbRequest->pSMBHeader->mid,
                pConnection->serverProperties.bRequireSecuritySignatures,
                pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    pSmbResponse->pSMBHeader->wordCount = 7;

    pResponseHeader = (PTREE_CONNECT_RESPONSE_HEADER)pSmbResponse->pParams;
    pSmbResponse->pData = pSmbResponse->pParams + sizeof(TREE_CONNECT_RESPONSE_HEADER);
    pSmbResponse->bufferUsed += sizeof(TREE_CONNECT_RESPONSE_HEADER);

    ntStatus = SrvGetMaximalShareAccessMask(
                    pTree->pShareInfo,
                    &pResponseHeader->maximalShareAccessMask);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvGetGuestShareAccessMask(
                    pTree->pShareInfo,
                    &pResponseHeader->guestMaximalShareAccessMask);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvGetServiceName(
                    pTree->pShareInfo,
                    &pszService);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pTree->pShareInfo->service == SHARE_SERVICE_DISK_SHARE)
    {
        ntStatus = SrvGetNativeFilesystem(
                        pTree->pShareInfo,
                        &pwszNativeFileSystem);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = MarshallTreeConnectResponseData(
                    pSmbResponse->pData,
                    pSmbResponse->bufferLen - pSmbResponse->bufferUsed,
                    pSmbResponse->bufferUsed,
                    &packetByteCount,
                    (const uint8_t*)pszService,
                    pwszNativeFileSystem);
    BAIL_ON_NT_STATUS(ntStatus);

    assert(packetByteCount <= UINT16_MAX);
    pResponseHeader->byteCount = (USHORT)packetByteCount;

    pSmbResponse->bufferUsed += packetByteCount;

    ntStatus = SMBPacketUpdateAndXOffset(pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketMarshallFooter(pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppSmbResponse = pSmbResponse;

cleanup:

    if (pszService)
    {
        LwRtlMemoryFree(pszService);
    }
    if (pwszNativeFileSystem)
    {
        LwRtlMemoryFree(pwszNativeFileSystem);
    }

    return ntStatus;

error:

    if (pSmbResponse)
    {
        SMBPacketFree(
            pConnection->hPacketAllocator,
            pSmbResponse);
    }

    goto cleanup;
}

static
NTSTATUS
SrvGetMaximalShareAccessMask(
    PSHARE_DB_INFO pShareInfo,
    ACCESS_MASK*   pMask
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN  bInLock = FALSE;
    ACCESS_MASK mask = 0;

    SMB_LOCK_RWMUTEX_SHARED(bInLock, &pShareInfo->mutex);

    switch (pShareInfo->service)
    {
        case SHARE_SERVICE_NAMED_PIPE:

            mask = (FILE_READ_DATA |
                    FILE_WRITE_DATA |
                    FILE_APPEND_DATA |
                    FILE_READ_EA |
                    FILE_WRITE_EA |
                    FILE_EXECUTE |
                    FILE_DELETE_CHILD |
                    FILE_READ_ATTRIBUTES |
                    FILE_WRITE_ATTRIBUTES);

            break;

        case SHARE_SERVICE_DISK_SHARE:

            mask = 0x1FF;

            break;

        case SHARE_SERVICE_PRINTER:
        case SHARE_SERVICE_COMM_DEVICE:
        case SHARE_SERVICE_ANY:

            mask = GENERIC_READ;

            break;

        default:

            mask = 0;

            break;
    }

    *pMask = mask;

    SMB_UNLOCK_RWMUTEX(bInLock, &pShareInfo->mutex);

    return ntStatus;
}

static
NTSTATUS
SrvGetGuestShareAccessMask(
    PSHARE_DB_INFO pShareInfo,
    ACCESS_MASK*   pMask
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN  bInLock = FALSE;
    ACCESS_MASK mask = 0;

    SMB_LOCK_RWMUTEX_SHARED(bInLock, &pShareInfo->mutex);

    switch (pShareInfo->service)
    {
        case SHARE_SERVICE_NAMED_PIPE:

            mask = (FILE_READ_DATA |
                    FILE_WRITE_DATA |
                    FILE_APPEND_DATA |
                    FILE_READ_EA |
                    FILE_WRITE_EA |
                    FILE_EXECUTE |
                    FILE_DELETE_CHILD |
                    FILE_READ_ATTRIBUTES |
                    FILE_WRITE_ATTRIBUTES);

            break;

        case SHARE_SERVICE_DISK_SHARE:

            mask = 0x1FF;

            break;

        case SHARE_SERVICE_PRINTER:
        case SHARE_SERVICE_COMM_DEVICE:
        case SHARE_SERVICE_ANY:

            mask = 0;

            break;

        default:

            mask = 0;

            break;
    }

    *pMask = mask;

    SMB_UNLOCK_RWMUTEX(bInLock, &pShareInfo->mutex);

    return ntStatus;
}

static
NTSTATUS
SrvGetServiceName(
    PSHARE_DB_INFO pShareInfo,
    PSTR* ppszService
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;
    PSTR pszService = NULL;

    SMB_LOCK_RWMUTEX_SHARED(bInLock, &pShareInfo->mutex);

    ntStatus = SrvShareGetServiceStringId(
                    pShareInfo->service,
                    &pszService);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppszService = pszService;

cleanup:

    SMB_UNLOCK_RWMUTEX(bInLock, &pShareInfo->mutex);

    return ntStatus;

error:

    *ppszService = NULL;

    goto cleanup;
}

static
NTSTATUS
SrvGetNativeFilesystem(
    PSHARE_DB_INFO pShareInfo,
    PWSTR* ppwszNativeFilesystem
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN  bInLock = FALSE;
    IO_FILE_HANDLE hFile = NULL;
    IO_FILE_NAME   fileName = {0};
    PIO_ASYNC_CONTROL_BLOCK pAsyncControlBlock = NULL;
    PIO_CREATE_SECURITY_CONTEXT pSecurityContext = NULL;
    PVOID               pSecurityDescriptor = NULL;
    PVOID               pSecurityQOS = NULL;
    IO_STATUS_BLOCK     ioStatusBlock = {0};
    PWSTR    pwszNativeFilesystem = NULL;
    PBYTE    pVolumeInfo = NULL;
    USHORT   usBytesAllocated = 0;
    PFILE_FS_ATTRIBUTE_INFORMATION pFsAttrInfo = NULL;

    SMB_LOCK_RWMUTEX_SHARED(bInLock, &pShareInfo->mutex);

    fileName.FileName = pShareInfo->pwszPath;

    ntStatus = IoCreateFile(
                    &hFile,
                    pAsyncControlBlock,
                    &ioStatusBlock,
                    pSecurityContext,
                    &fileName,
                    pSecurityDescriptor,
                    pSecurityQOS,
                    GENERIC_READ,
                    0,
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ,
                    FILE_OPEN,
                    0,
                    NULL, /* EA Buffer */
                    0,    /* EA Length */
                    NULL  /* ECP List  */
                    );
    BAIL_ON_NT_STATUS(ntStatus);

    SMB_UNLOCK_RWMUTEX(bInLock, &pShareInfo->mutex);

    usBytesAllocated = sizeof(FILE_FS_ATTRIBUTE_INFORMATION) + 256 * sizeof(wchar16_t);

    ntStatus = LW_RTL_ALLOCATE(&pVolumeInfo, BYTE, usBytesAllocated);
    BAIL_ON_NT_STATUS(ntStatus);

    do
    {
        ntStatus = IoQueryVolumeInformationFile(
                        hFile,
                        NULL,
                        &ioStatusBlock,
                        pVolumeInfo,
                        usBytesAllocated,
                        FileFsAttributeInformation);
        if (ntStatus == STATUS_SUCCESS)
        {
            break;
        }
        else if (ntStatus == STATUS_BUFFER_TOO_SMALL)
        {
            USHORT usNewSize = usBytesAllocated + 256 * sizeof(wchar16_t);

            ntStatus = SMBReallocMemory(
                            pVolumeInfo,
                            (PVOID*)&pVolumeInfo,
                            usNewSize);
            BAIL_ON_NT_STATUS(ntStatus);

            usBytesAllocated = usNewSize;

            continue;
        }
        BAIL_ON_NT_STATUS(ntStatus);

    } while (TRUE);

    pFsAttrInfo = (PFILE_FS_ATTRIBUTE_INFORMATION)pVolumeInfo;

    if (!pFsAttrInfo->FileSystemNameLength)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SMBAllocateStringW(
                    pFsAttrInfo->FileSystemName,
                    &pwszNativeFilesystem);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppwszNativeFilesystem = pwszNativeFilesystem;

cleanup:

    SMB_UNLOCK_RWMUTEX(bInLock, &pShareInfo->mutex);

    if (pVolumeInfo)
    {
        LwRtlMemoryFree(pVolumeInfo);
    }

    if (hFile)
    {
        IoCloseFile(hFile);
    }

    return ntStatus;

error:

    *ppwszNativeFilesystem = NULL;

    goto cleanup;
}


