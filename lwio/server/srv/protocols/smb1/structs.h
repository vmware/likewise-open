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
 *        structs.h
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Structures
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#ifndef __STRUCTS_H__
#define __STRUCTS_H__

typedef enum
{
    SRV_TREE_CONNECT_STAGE_SMB_V1_INITIAL = 0,
    SRV_TREE_CONNECT_STAGE_SMB_V1_ATTEMPT_QUERY_INFO,
    SRV_TREE_CONNECT_STAGE_SMB_V1_QUERY_INFO_COMPLETED,
    SRV_TREE_CONNECT_STAGE_SMB_V1_DONE
} SRV_TREE_CONNECT_STAGE_SMB_V1;

typedef struct _SRV_TREE_CONNECT_STATE_SMB_V1
{
    LONG                           refCount;

    pthread_mutex_t                mutex;
    pthread_mutex_t*               pMutex;

    SRV_TREE_CONNECT_STAGE_SMB_V1  stage;

    PTREE_CONNECT_REQUEST_HEADER   pRequestHeader; // Do not free
    PBYTE                          pszPassword;    // Do not free
    PBYTE                          pszService;     // Do not free
    PWSTR                          pwszPath;       // Do not free

    IO_STATUS_BLOCK                ioStatusBlock;

    IO_ASYNC_CONTROL_BLOCK         acb;
    PIO_ASYNC_CONTROL_BLOCK        pAcb;

    PVOID                          pSecurityDescriptor;
    PVOID                          pSecurityQOS;
    PIO_ECP_LIST                   pEcpList;
    IO_FILE_HANDLE                 hFile;

    PSRV_SHARE_INFO                pShareInfo;
    PLWIO_SRV_SESSION              pSession;
    PLWIO_SRV_TREE                 pTree;

    IO_FILE_NAME                   fileName;
    PSTR                           pszService2;
    PBYTE                          pVolumeInfo;
    USHORT                         usBytesAllocated;
    PIO_CREATE_SECURITY_CONTEXT    pIoSecContext;
    PWSTR                          pwszNativeFileSystem;
    ULONG                          ulMaximalShareAccessMask;
    ULONG                          ulGuestMaximalShareAccessMask;

    BOOLEAN                        bRemoveTreeFromSession;

} SRV_TREE_CONNECT_STATE_SMB_V1, *PSRV_TREE_CONNECT_STATE_SMB_V1;

typedef enum
{
    SRV_CREATE_STAGE_SMB_V1_INITIAL = 0,
    SRV_CREATE_STAGE_SMB_V1_CREATE_FILE_COMPLETED,
    SRV_CREATE_STAGE_SMB_V1_ATTEMPT_QUERY_INFO,
    SRV_CREATE_STAGE_SMB_V1_QUERY_INFO_COMPLETED,
    SRV_CREATE_STAGE_SMB_V1_DONE
} SRV_CREATE_STAGE_SMB_V1;

typedef struct _SRV_CREATE_STATE_SMB_V1
{
    LONG                    refCount;

    pthread_mutex_t         mutex;
    pthread_mutex_t*        pMutex;

    SRV_CREATE_STAGE_SMB_V1 stage;

    PCREATE_REQUEST_HEADER  pRequestHeader; // Do not free
    PWSTR                   pwszFilename;   // Do not free

    IO_STATUS_BLOCK         ioStatusBlock;

    IO_ASYNC_CONTROL_BLOCK  acb;
    PIO_ASYNC_CONTROL_BLOCK pAcb;

    PVOID                   pSecurityDescriptor;
    PVOID                   pSecurityQOS;
    PIO_FILE_NAME           pFilename;
    PIO_ECP_LIST            pEcpList;
    IO_FILE_HANDLE          hFile;

    FILE_BASIC_INFORMATION       fileBasicInfo;
    PFILE_BASIC_INFORMATION      pFileBasicInfo;

    FILE_STANDARD_INFORMATION    fileStdInfo;
    PFILE_STANDARD_INFORMATION   pFileStdInfo;

    FILE_PIPE_INFORMATION        filePipeInfo;
    PFILE_PIPE_INFORMATION       pFilePipeInfo;

    FILE_PIPE_LOCAL_INFORMATION  filePipeLocalInfo;
    PFILE_PIPE_LOCAL_INFORMATION pFilePipeLocalInfo;

    PLWIO_SRV_TREE          pTree;
    PLWIO_SRV_FILE          pFile;
    BOOLEAN                 bRemoveFileFromTree;

} SRV_CREATE_STATE_SMB_V1, *PSRV_CREATE_STATE_SMB_V1;

typedef enum
{
    SRV_OPEN_STAGE_SMB_V1_INITIAL = 0,
    SRV_OPEN_STAGE_SMB_V1_OPEN_FILE_COMPLETED,
    SRV_OPEN_STAGE_SMB_V1_ATTEMPT_QUERY_INFO,
    SRV_OPEN_STAGE_SMB_V1_QUERY_INFO_COMPLETED,
    SRV_OPEN_STAGE_SMB_V1_DONE
} SRV_OPEN_STAGE_SMB_V1;

typedef struct _SRV_OPEN_STATE_SMB_V1
{
    LONG                         refCount;

    pthread_mutex_t              mutex;
    pthread_mutex_t*             pMutex;

    SRV_OPEN_STAGE_SMB_V1        stage;

    POPEN_REQUEST_HEADER         pRequestHeader; // Do not free
    PWSTR                        pwszFilename;   // Do not free

    IO_STATUS_BLOCK              ioStatusBlock;

    IO_ASYNC_CONTROL_BLOCK       acb;
    PIO_ASYNC_CONTROL_BLOCK      pAcb;

    PVOID                        pSecurityDescriptor;
    PVOID                        pSecurityQOS;
    PIO_FILE_NAME                pFilename;
    PIO_ECP_LIST                 pEcpList;
    IO_FILE_HANDLE               hFile;

    USHORT                       usShareAccess;
    ULONG                        ulCreateDisposition;
    ACCESS_MASK                  ulDesiredAccessMask;
    USHORT                       usCreateOptions;

    FILE_BASIC_INFORMATION       fileBasicInfo;
    PFILE_BASIC_INFORMATION      pFileBasicInfo;

    FILE_STANDARD_INFORMATION    fileStdInfo;
    PFILE_STANDARD_INFORMATION   pFileStdInfo;

    FILE_PIPE_INFORMATION        filePipeInfo;
    PFILE_PIPE_INFORMATION       pFilePipeInfo;

    FILE_PIPE_LOCAL_INFORMATION  filePipeLocalInfo;
    PFILE_PIPE_LOCAL_INFORMATION pFilePipeLocalInfo;

    PLWIO_SRV_TREE               pTree;
    PLWIO_SRV_FILE               pFile;
    BOOLEAN                      bRemoveFileFromTree;

} SRV_OPEN_STATE_SMB_V1, *PSRV_OPEN_STATE_SMB_V1;

typedef enum
{
    SRV_SMB_UNLOCK = 0,
    SRV_SMB_LOCK
} SRV_SMB_LOCK_OPERATION_TYPE;

typedef struct _SRV_SMB_LOCK_REQUEST* PSRV_SMB_LOCK_REQUEST;

typedef struct _SRV_SMB_LOCK_CONTEXT
{
    SRV_SMB_LOCK_OPERATION_TYPE operation;

    ULONG   ulKey;
    BOOLEAN bExclusiveLock;

    IO_ASYNC_CONTROL_BLOCK  acb;
    PIO_ASYNC_CONTROL_BLOCK pAcb;

    IO_STATUS_BLOCK ioStatusBlock;

    SRV_SMB_LOCK_TYPE lockType;

    union
    {
        LOCKING_ANDX_RANGE_LARGE_FILE largeFileRange;
        LOCKING_ANDX_RANGE            regularRange;

    } lockInfo;

    PSRV_SMB_LOCK_REQUEST pLockRequest;

} SRV_SMB_LOCK_CONTEXT, *PSRV_SMB_LOCK_CONTEXT;

typedef struct _SRV_SMB_LOCK_REQUEST
{
    LONG                  refCount;

    pthread_mutex_t       mutex;
    pthread_mutex_t*      pMutex;

    IO_STATUS_BLOCK       ioStatusBlock;

    ULONG                 ulTimeout;

    USHORT                usNumUnlocks;
    USHORT                usNumLocks;

    PSRV_SMB_LOCK_CONTEXT pLockContexts; /* unlocks and locks */

    LONG                  lPendingContexts;

    BOOLEAN               bExpired;
    BOOLEAN               bCompleted;
    BOOLEAN               bResponseSent;

    PSRV_TIMER_REQUEST    pTimerRequest;

    PSRV_EXEC_CONTEXT     pExecContext_timer;
    PSRV_EXEC_CONTEXT     pExecContext_async;

} SRV_SMB_LOCK_REQUEST;

typedef enum
{
    SRV_READ_STAGE_SMB_V1_INITIAL = 0,
    SRV_READ_STAGE_SMB_V1_ATTEMPT_READ,
    SRV_READ_STAGE_SMB_V1_DONE
} SRV_READ_STAGE_SMB_V1;

typedef struct _SRV_READ_STATE_SMB_V1
{
    LONG                      refCount;

    pthread_mutex_t           mutex;
    pthread_mutex_t*          pMutex;

    SRV_READ_STAGE_SMB_V1     stage;

    IO_STATUS_BLOCK           ioStatusBlock;

    IO_ASYNC_CONTROL_BLOCK    acb;
    PIO_ASYNC_CONTROL_BLOCK   pAcb;

    PREAD_ANDX_REQUEST_HEADER pRequestHeader; // Do not free

    PLWIO_SRV_FILE            pFile;

    ULONG64                   ullBytesToRead;
    LONG64                    llByteOffset;

    PREAD_ANDX_RESPONSE_HEADER pResponseHeader; // Do not free
    PBYTE pOutBuffer;
    ULONG ulBytesAvailable;
    ULONG ulOffset;
    ULONG ulPackageByteCount;
    ULONG ulTotalBytesUsed;
    ULONG ulDataOffset;
    ULONG ulBytesRead;
    ULONG ulBytesToRead;
    PBYTE pData;
    ULONG ulKey;

} SRV_READ_STATE_SMB_V1, *PSRV_READ_STATE_SMB_V1;

typedef enum
{
    SRV_WRITEX_STAGE_SMB_V1_INITIAL = 0,
    SRV_WRITEX_STAGE_SMB_V1_ATTEMPT_WRITE,
    SRV_WRITEX_STAGE_SMB_V1_BUILD_RESPONSE,
    SRV_WRITEX_STAGE_SMB_V1_DONE
} SRV_WRITEX_STAGE_SMB_V1;

typedef struct _SRV_WRITEX_STATE_SMB_V1
{
    LONG                       refCount;

    pthread_mutex_t            mutex;
    pthread_mutex_t*           pMutex;

    SRV_WRITEX_STAGE_SMB_V1    stage;

    IO_STATUS_BLOCK            ioStatusBlock;

    IO_ASYNC_CONTROL_BLOCK     acb;
    PIO_ASYNC_CONTROL_BLOCK    pAcb;

    PLWIO_SRV_FILE             pFile;

    PWRITE_ANDX_REQUEST_HEADER pRequestHeader; // Do not free
    PBYTE                      pData;          // Do not free
    LONG64                     llDataOffset;
    LONG64                     llDataLength;
    ULONG64                    ullBytesWritten;
    ULONG                      ulKey;

} SRV_WRITEX_STATE_SMB_V1, *PSRV_WRITEX_STATE_SMB_V1;

typedef enum
{
    SRV_WRITE_STAGE_SMB_V1_INITIAL = 0,
    SRV_WRITE_STAGE_SMB_V1_ATTEMPT_WRITE,
    SRV_WRITE_STAGE_SMB_V1_BUILD_RESPONSE,
    SRV_WRITE_STAGE_SMB_V1_DONE
} SRV_WRITE_STAGE_SMB_V1;

typedef struct _SRV_WRITE_STATE_SMB_V1
{
    LONG                    refCount;

    pthread_mutex_t         mutex;
    pthread_mutex_t*        pMutex;

    SRV_WRITE_STAGE_SMB_V1  stage;

    IO_STATUS_BLOCK         ioStatusBlock;

    IO_ASYNC_CONTROL_BLOCK  acb;
    PIO_ASYNC_CONTROL_BLOCK pAcb;

    PLWIO_SRV_FILE          pFile;

    PWRITE_REQUEST_HEADER   pRequestHeader; // Do not free
    PBYTE                   pData;          // Do not free
    ULONG                   ulDataOffset;
    LONG64                  llDataOffset;
    USHORT                  usDataLength;
    USHORT                  usBytesWritten;
    ULONG                   ulKey;

    FILE_END_OF_FILE_INFORMATION  fileEofInfo;
    PFILE_END_OF_FILE_INFORMATION pFileEofInfo;

} SRV_WRITE_STATE_SMB_V1, *PSRV_WRITE_STATE_SMB_V1;

typedef VOID (*PFN_SRV_MESSAGE_STATE_RELEASE_SMB_V1)(HANDLE hState);

typedef struct __SRV_MESSAGE_SMB_V1
{
    ULONG        ulSerialNum;       // Sequence # in packet; starts from 0

    PBYTE        pBuffer;           // Raw packet buffer

    UCHAR        ucCommand;         // andx or SMB command
    PSMB_HEADER  pHeader;           // header corresponding to serial 0 message
    PBYTE        pWordCount;        // points to word count field in message
                                    // (a) for the serial 0 message, this points
                                    //     to the word count field in the header
                                    // (b) andx messages share the serial 0
                                    //     smb header; but, they have their own
                                    //     word count referenced by this field.
    PANDX_HEADER pAndXHeader;
    USHORT       usHeaderSize;
    ULONG        ulMessageSize;

    ULONG        ulBytesAvailable;

} SRV_MESSAGE_SMB_V1, *PSRV_MESSAGE_SMB_V1;

typedef struct _SRV_EXEC_CONTEXT_SMB_V1
{
    PSRV_MESSAGE_SMB_V1                  pRequests;
    ULONG                                ulNumRequests;
    ULONG                                iMsg;

    PLWIO_SRV_SESSION                    pSession;
    PLWIO_SRV_TREE                       pTree;
    PLWIO_SRV_FILE                       pFile;

    HANDLE                               hState;
    PFN_SRV_MESSAGE_STATE_RELEASE_SMB_V1 pfnStateRelease;

    ULONG                                ulNumResponses;
    PSRV_MESSAGE_SMB_V1                  pResponses;

} SRV_EXEC_CONTEXT_SMB_V1;

typedef struct _SRV_RUNTIME_GLOBALS_SMB_V1
{
    pthread_mutex_t         mutex;

    PSMB_PROD_CONS_QUEUE    pWorkQueue;

} SRV_RUNTIME_GLOBALS_SMB_V1, *PSRV_RUNTIME_GLOBALS_SMB_V1;

#endif /* __STRUCTS_H__ */
