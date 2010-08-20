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

typedef struct
{
    USHORT               usFid;
    USHORT               usReserved;
    SECURITY_INFORMATION ulSecurityInfo;
} __attribute__((__packed__)) SMB_SECURITY_INFORMATION_HEADER,
                             *PSMB_SECURITY_INFORMATION_HEADER;

typedef struct
{
    ULONG   ulFunctionCode;
    USHORT  usFid;
    BOOLEAN bIsFsctl;
    UCHAR   ucFlags;
} __attribute__((__packed__)) SMB_IOCTL_HEADER, *PSMB_IOCTL_HEADER;

typedef struct
{
    ULONG   ulCompletionFilter;
    USHORT  usFid;
    BOOLEAN bWatchTree;
    UCHAR   ucReserved;
} __attribute__((__packed__)) SMB_NOTIFY_CHANGE_HEADER,
                             *PSMB_NOTIFY_CHANGE_HEADER;

typedef USHORT LW_OPLOCK_ACTION;

#define LW_OPLOCK_ACTION_SEND_BREAK  0x0001
#define LW_OPLOCK_ACTION_PROCESS_ACK 0x0002

typedef struct
{
    USHORT usFid;
    USHORT usAction;

} __attribute__((__packed__)) LW_OPLOCK_HEADER, *PLW_OPLOCK_HEADER;

typedef struct _SRV_OPLOCK_INFO
{
    UCHAR oplockRequest;
    UCHAR oplockLevel;
} SRV_OPLOCK_INFO, *PSRV_OPLOCK_INFO;

typedef struct _SRV_OPLOCK_STATE_SMB_V1
{
    LONG                    refCount;

    pthread_mutex_t         mutex;
    pthread_mutex_t*        pMutex;

    IO_STATUS_BLOCK         ioStatusBlock;

    IO_ASYNC_CONTROL_BLOCK  acb;
    PIO_ASYNC_CONTROL_BLOCK pAcb;

    PLWIO_SRV_CONNECTION    pConnection;

    USHORT                  usUid;
    USHORT                  usTid;
    USHORT                  usFid;

    PSRV_TIMER_REQUEST      pTimerRequest;

    IO_FSCTL_OPLOCK_REQUEST_INPUT_BUFFER   oplockBuffer_in;
    IO_FSCTL_OPLOCK_REQUEST_OUTPUT_BUFFER  oplockBuffer_out;
    IO_FSCTL_OPLOCK_BREAK_ACK_INPUT_BUFFER oplockBuffer_ack;

} SRV_OPLOCK_STATE_SMB_V1, *PSRV_OPLOCK_STATE_SMB_V1;

typedef struct _SRV_CHANGE_NOTIFY_STATE_SMB_V1
{
    LONG                    refCount;

    pthread_mutex_t         mutex;
    pthread_mutex_t*        pMutex;

    ULONG64                 ullNotifyId;

    IO_STATUS_BLOCK         ioStatusBlock;

    IO_ASYNC_CONTROL_BLOCK  acb;
    PIO_ASYNC_CONTROL_BLOCK pAcb;

    ULONG                   ulCompletionFilter;
    BOOLEAN                 bWatchTree;

    PLWIO_SRV_CONNECTION    pConnection;

    USHORT                  usUid;
    USHORT                  usTid;
    USHORT                  usFid;
    USHORT                  usMid;
    ULONG                   ulPid;

    ULONG                   ulRequestSequence;

    PBYTE                   pBuffer;
    ULONG                   ulBufferLength;
    ULONG                   ulBytesUsed;

    ULONG                   ulMaxBufferSize;

} SRV_CHANGE_NOTIFY_STATE_SMB_V1, *PSRV_CHANGE_NOTIFY_STATE_SMB_V1;

typedef enum
{
    SRV_TREE_CONNECT_STAGE_SMB_V1_INITIAL = 0,
    SRV_TREE_CONNECT_STAGE_SMB_V1_CREATE_TREE_ROOT_HANDLE,
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

    PSRV_SHARE_INFO                pShareInfo;
    PLWIO_SRV_SESSION              pSession;
    PLWIO_SRV_TREE                 pTree;

    IO_FILE_NAME                   fileName;
    PSTR                           pszService2;
    PBYTE                          pVolumeInfo;
    USHORT                         usBytesAllocated;
    PWSTR                          pwszNativeFileSystem;
    ULONG                          ulMaximalShareAccessMask;
    ULONG                          ulGuestMaximalShareAccessMask;

    struct sockaddr                clientAddress;
    ULONG                          ulClientAddressLength;

    BOOLEAN                        bRemoveTreeFromSession;

} SRV_TREE_CONNECT_STATE_SMB_V1, *PSRV_TREE_CONNECT_STATE_SMB_V1;

typedef enum
{
    SRV_CREATE_STAGE_SMB_V1_INITIAL = 0,
    SRV_CREATE_STAGE_SMB_V1_CREATE_FILE_COMPLETED,
    SRV_CREATE_STAGE_SMB_V1_ATTEMPT_QUERY_INFO,
    SRV_CREATE_STAGE_SMB_V1_QUERY_INFO_COMPLETED,
    SRV_CREATE_STAGE_SMB_V1_REQUEST_OPLOCK,
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

    FILE_CREATE_RESULT           ulCreateAction;

    UCHAR                                 ucOplockLevel;

    PLWIO_SRV_TREE          pTree;
    PLWIO_SRV_FILE          pFile;
    PLWIO_SRV_FILE          pRootDirectory;
    BOOLEAN                 bRemoveFileFromTree;

} SRV_CREATE_STATE_SMB_V1, *PSRV_CREATE_STATE_SMB_V1;

typedef enum
{
    SRV_OPEN_STAGE_SMB_V1_INITIAL = 0,
    SRV_OPEN_STAGE_SMB_V1_OPEN_FILE_COMPLETED,
    SRV_OPEN_STAGE_SMB_V1_ATTEMPT_QUERY_INFO,
    SRV_OPEN_STAGE_SMB_V1_REQUEST_OPLOCK,
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

    FILE_CREATE_RESULT           ulCreateAction;

    UCHAR                        ucOplockLevel;

    PLWIO_SRV_TREE               pTree;
    PLWIO_SRV_FILE               pFile;
    BOOLEAN                      bRemoveFileFromTree;

} SRV_OPEN_STATE_SMB_V1, *PSRV_OPEN_STATE_SMB_V1;

typedef enum
{
    SRV_LOCK_STAGE_SMB_V1_INITIAL = 0,
    SRV_LOCK_STAGE_SMB_V1_ATTEMPT_LOCK,
    SRV_LOCK_STAGE_SMB_V1_DONE
} SRV_LOCK_STAGE_SMB_V1;

typedef struct _SRV_LOCK_STATE_SMB_V1
{
    LONG                             refCount;

    pthread_mutex_t                  mutex;
    pthread_mutex_t*                 pMutex;

    SRV_LOCK_STAGE_SMB_V1            stage;

    IO_ASYNC_CONTROL_BLOCK           acb;
    PIO_ASYNC_CONTROL_BLOCK          pAcb;

    IO_STATUS_BLOCK                  ioStatusBlock;

    PSMB_LOCKING_ANDX_REQUEST_HEADER pRequestHeader;     // Do not free

    PLOCKING_ANDX_RANGE_LARGE_FILE   pUnlockRangeLarge;  // Do not free
    PLOCKING_ANDX_RANGE_LARGE_FILE   pLockRangeLarge;    // Do not free

    PLOCKING_ANDX_RANGE              pUnlockRange;       // Do not free
    PLOCKING_ANDX_RANGE              pLockRange;         // Do not free

    ULONG                            ulPid;
    USHORT                           usMid;

    ULONG64                          ullAsyncId;

    USHORT                           iUnlock;
    BOOLEAN                          bUnlockPending;

    USHORT                           iLock;
    BOOLEAN                          bLockPending;

    PLWIO_SRV_FILE                   pFile;

    BOOLEAN                          bRequestExclusiveLock;
    LONG64                           llOffset;
    LONG64                           llLength;
    ULONG                            ulKey;

    BOOLEAN                          bExpired;
    BOOLEAN                          bCompleted;
    BOOLEAN                          bCancelled;

    PSRV_TIMER_REQUEST               pTimerRequest;

} SRV_LOCK_STATE_SMB_V1, *PSRV_LOCK_STATE_SMB_V1;

typedef struct _SRV_BYTE_RANGE_LOCK_STATE
{
    ULONG64 ullAsyncId;

    struct _SRV_BYTE_RANGE_LOCK_STATE* pNext;

} SRV_BYTE_RANGE_LOCK_STATE, *PSRV_BYTE_RANGE_LOCK_STATE;

typedef struct _SRV_BYTE_RANGE_LOCK_STATE_LIST
{
    pthread_mutex_t  mutex;
    pthread_mutex_t* pMutex;

    PSRV_BYTE_RANGE_LOCK_STATE pHead;
    PSRV_BYTE_RANGE_LOCK_STATE pTail;

} SRV_BYTE_RANGE_LOCK_STATE_LIST, *PSRV_BYTE_RANGE_LOCK_STATE_LIST;

typedef enum
{
    SRV_READ_STAGE_SMB_V1_INITIAL = 0,
    SRV_READ_STAGE_SMB_V1_RESPONSE_START,
    SRV_READ_STAGE_SMB_V1_ATTEMPT_READ,
    SRV_READ_STAGE_SMB_V1_RESPONSE_FINISH,
    SRV_READ_STAGE_SMB_V1_ZCT_COMPLETE,
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

    UCHAR                     ucWordCount;

    union
    {
        PREAD_ANDX_REQUEST_HEADER_WC_10 pRequestHeader_WC_10;
        PREAD_ANDX_REQUEST_HEADER_WC_12 pRequestHeader_WC_12;
    }; // Do not free

    PLWIO_SRV_FILE             pFile;

    ULONG64                    ullBytesToRead;
    LONG64                     llByteOffset;

    PREAD_ANDX_RESPONSE_HEADER pResponseHeader; // Do not free
    PBYTE                      pOutBuffer;
    ULONG                      ulBytesAvailable;
    ULONG                      ulOffset;
    ULONG                      ulPackageByteCount;
    ULONG                      ulTotalBytesUsed;
    ULONG                      ulDataOffset;
    ULONG                      ulBytesRead;
    ULONG                      ulBytesToRead;
    PBYTE                      pData;
    ULONG                      ulKey;
    BOOLEAN                    bPagedIo;
    BOOLEAN                    bStartedRead;
    PLW_ZCT_VECTOR             pZct;
    PVOID                      pZctCompletion;

} SRV_READ_STATE_SMB_V1, *PSRV_READ_STATE_SMB_V1;

typedef struct _SRV_ZCT_WRITE_STATE
{
    ULONG                ulDataBytesMissing;
    ULONG                ulDataBytesResident;
    ULONG                ulSkipBytes;
    PLW_ZCT_VECTOR       pZct;
    PVOID                pZctCompletion;
    PVOID                pPadding;
    ULONG                ulPaddingSize;
    PLWIO_SRV_CONNECTION pPausedConnection;
} SRV_ZCT_WRITE_STATE, *PSRV_ZCT_WRITE_STATE;

typedef enum
{
    SRV_WRITEX_STAGE_SMB_V1_INITIAL = 0,
    SRV_WRITEX_STAGE_SMB_V1_ATTEMPT_WRITE,
    SRV_WRITEX_STAGE_SMB_V1_ZCT_IO,
    SRV_WRITEX_STAGE_SMB_V1_ZCT_COMPLETE,
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

    UCHAR                      ucWordCount;
    PWRITE_ANDX_REQUEST_HEADER_WC_12 pRequestHeader_WC_12; // Do not free
    PWRITE_ANDX_REQUEST_HEADER_WC_14 pRequestHeader_WC_14; // Do not free

    PBYTE                      pData;          // Do not free
    LONG64                     llOffset;
    ULONG                      ulLength;
    ULONG                      ulKey;
    ULONG                      ulBytesWritten;

    BOOLEAN                    bStartedIo;

    SRV_ZCT_WRITE_STATE        zct;

} SRV_WRITEX_STATE_SMB_V1, *PSRV_WRITEX_STATE_SMB_V1;

typedef enum
{
    SRV_WRITE_STAGE_SMB_V1_INITIAL = 0,
    SRV_WRITE_STAGE_SMB_V1_ATTEMPT_WRITE,
    SRV_WRITE_STAGE_SMB_V1_ZCT_IO,
    SRV_WRITE_STAGE_SMB_V1_ZCT_COMPLETE,
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
    LONG64                  llOffset;
    ULONG                   ulLength;
    ULONG                   ulKey;
    ULONG                   ulBytesWritten;

    BOOLEAN bStartedIo;

    // ZCT information
    SRV_ZCT_WRITE_STATE Zct;

    FILE_END_OF_FILE_INFORMATION  fileEofInfo;
    PFILE_END_OF_FILE_INFORMATION pFileEofInfo;

} SRV_WRITE_STATE_SMB_V1, *PSRV_WRITE_STATE_SMB_V1;

typedef enum
{
    SRV_SET_INFO_STAGE_SMB_V1_INITIAL        = 0,
    SRV_SET_INFO_STAGE_SMB_V1_ATTEMPT_SET,
    SRV_SET_INFO_STAGE_SMB_V1_BUILD_RESPONSE,
    SRV_SET_INFO_STAGE_SMB_V1_DONE
} SRV_SET_INFO_STAGE_SMB_V1;

typedef struct _SRV_SET_INFO_STATE_SMB_V1
{
    LONG                       refCount;

    pthread_mutex_t            mutex;
    pthread_mutex_t*           pMutex;

    SRV_SET_INFO_STAGE_SMB_V1     stage;

    IO_STATUS_BLOCK            ioStatusBlock;

    IO_ASYNC_CONTROL_BLOCK     acb;
    PIO_ASYNC_CONTROL_BLOCK    pAcb;

    PIO_ECP_LIST               pEcpList;

    PLWIO_SRV_SESSION          pSession;
    PLWIO_SRV_TREE             pTree;

    IO_FILE_HANDLE             hFile;
    IO_FILE_NAME               fileName;

    PSET_INFO_REQUEST_HEADER   pRequestHeader; // Do not free
    PWSTR                      pwszFilename;   // Do not free

    FILE_BASIC_INFORMATION     fileBasicInfo;

} SRV_SET_INFO_STATE_SMB_V1, *PSRV_SET_INFO_STATE_SMB_V1;

typedef enum
{
    SRV_SET_INFO2_STAGE_SMB_V1_INITIAL        = 0,
    SRV_SET_INFO2_STAGE_SMB_V1_ATTEMPT_SET,
    SRV_SET_INFO2_STAGE_SMB_V1_BUILD_RESPONSE,
    SRV_SET_INFO2_STAGE_SMB_V1_DONE
} SRV_SET_INFO2_STAGE_SMB_V1;

typedef struct _SRV_SET_INFO2_STATE_SMB_V1
{
    LONG                       refCount;

    pthread_mutex_t            mutex;
    pthread_mutex_t*           pMutex;

    SRV_SET_INFO2_STAGE_SMB_V1     stage;

    IO_STATUS_BLOCK            ioStatusBlock;

    IO_ASYNC_CONTROL_BLOCK     acb;
    PIO_ASYNC_CONTROL_BLOCK    pAcb;

    PLWIO_SRV_FILE             pFile;

    PSET_INFO2_REQUEST_HEADER  pRequestHeader; // Do not free

    FILE_BASIC_INFORMATION     fileBasicInfo;

} SRV_SET_INFO2_STATE_SMB_V1, *PSRV_SET_INFO2_STATE_SMB_V1;

typedef enum
{
    SRV_QUERY_INFO2_STAGE_SMB_V1_INITIAL = 0,
    SRV_QUERY_INFO2_STAGE_SMB_V1_ATTEMPT_IO,
    SRV_QUERY_INFO2_STAGE_SMB_V1_BUILD_RESPONSE,
    SRV_QUERY_INFO2_STAGE_SMB_V1_DONE
} SRV_QUERY_INFO2_STAGE_SMB_V1;

typedef struct _SRV_QUERY_INFO2_STATE_SMB_V1
{
    LONG                         refCount;

    pthread_mutex_t              mutex;
    pthread_mutex_t*             pMutex;

    SRV_QUERY_INFO2_STAGE_SMB_V1 stage;

    IO_STATUS_BLOCK              ioStatusBlock;

    IO_ASYNC_CONTROL_BLOCK       acb;
    PIO_ASYNC_CONTROL_BLOCK      pAcb;

    PLWIO_SRV_FILE               pFile;

    PQUERY_INFO2_REQUEST_HEADER  pRequestHeader; // Do not free

    FILE_BASIC_INFORMATION       fileBasicInfo;
    PFILE_BASIC_INFORMATION      pFileBasicInfo;

    FILE_STANDARD_INFORMATION    fileStandardInfo;
    PFILE_STANDARD_INFORMATION   pFileStandardInfo;

} SRV_QUERY_INFO2_STATE_SMB_V1, *PSRV_QUERY_INFO2_STATE_SMB_V1;

typedef enum
{
    SRV_TRANS_STAGE_SMB_V1_INITIAL = 0,
    SRV_TRANS_STAGE_SMB_V1_CREATE_FILE_COMPLETED,
    SRV_TRANS_STAGE_SMB_V1_ATTEMPT_IO,
    SRV_TRANS_STAGE_SMB_V1_IO_COMPLETE,
    SRV_TRANS_STAGE_SMB_V1_ATTEMPT_WRITE,
    SRV_TRANS_STAGE_SMB_V1_WRITE_COMPLETE,
    SRV_TRANS_STAGE_SMB_V1_ATTEMPT_READ,
    SRV_TRANS_STAGE_SMB_V1_READ_COMPLETE,
    SRV_TRANS_STAGE_SMB_V1_BUILD_RESPONSE,
    SRV_TRANS_STAGE_SMB_V1_DONE
} SRV_TRANS_STAGE_SMB_V1;

typedef struct _SRV_TRANS_STATE_SMB_V1
{
    LONG                         refCount;

    pthread_mutex_t              mutex;
    pthread_mutex_t*             pMutex;

    SRV_TRANS_STAGE_SMB_V1       stage;

    IO_STATUS_BLOCK              ioStatusBlock;

    IO_ASYNC_CONTROL_BLOCK       acb;
    PIO_ASYNC_CONTROL_BLOCK      pAcb;

    PVOID                        pSecurityDescriptor;
    PVOID                        pSecurityQOS;

    PTRANSACTION_REQUEST_HEADER  pRequestHeader; // Do not free
    PUSHORT                      pBytecount;     // Do not free
    PWSTR                        pwszName;       // Do not free
    PUSHORT                      pSetup;         // Do not free
    PBYTE                        pParameters;    // Do not free
    PBYTE                        pData;          // Do not free

    PLWIO_SRV_SESSION            pSession;
    PLWIO_SRV_TREE               pTree;
    PLWIO_SRV_FILE               pFile;

    IO_FILE_HANDLE               hFile;
    IO_FILE_NAME                 fileName;
    LONG64                       llOffset;

    FILE_PIPE_INFORMATION        pipeInfo;
    PFILE_PIPE_INFORMATION       pPipeInfo;

    FILE_PIPE_LOCAL_INFORMATION  pipeLocalInfo;
    PFILE_PIPE_LOCAL_INFORMATION pPipeLocalInfo;

    PBYTE                        pData2;
    USHORT                       usBytesRead;

} SRV_TRANS_STATE_SMB_V1, *PSRV_TRANS_STATE_SMB_V1;

typedef enum
{
    SRV_TRANS2_STAGE_SMB_V1_INITIAL = 0,
    SRV_TRANS2_STAGE_SMB_V1_CREATE_FILE_COMPLETED,
    SRV_TRANS2_STAGE_SMB_V1_ATTEMPT_IO,
    SRV_TRANS2_STAGE_SMB_V1_IO_COMPLETE,
    SRV_TRANS2_STAGE_SMB_V1_BUILD_RESPONSE,
    SRV_TRANS2_STAGE_SMB_V1_DONE
} SRV_TRANS2_STAGE_SMB_V1;

typedef struct _SRV_TRANS2_STATE_SMB_V1
{
    LONG                       refCount;

    pthread_mutex_t            mutex;
    pthread_mutex_t*           pMutex;

    SRV_TRANS2_STAGE_SMB_V1    stage;

    IO_STATUS_BLOCK            ioStatusBlock;

    IO_ASYNC_CONTROL_BLOCK     acb;
    PIO_ASYNC_CONTROL_BLOCK    pAcb;

    PVOID                      pSecurityDescriptor;
    PVOID                      pSecurityQOS;
    PIO_ECP_LIST               pEcpList;

    PTRANSACTION_REQUEST_HEADER pRequestHeader; // Do not free
    PUSHORT                     pBytecount;     // Do not free
    PUSHORT                     pSetup;         // Do not free
    PBYTE                       pParameters;    // Do not free
    PBYTE                       pData;          // Do not free
    PSMB_INFO_LEVEL             pSmbInfoLevel;  // Do not free
    PUSHORT                     pMaxDfsReferralLevel;  // Do not free
    USHORT                      usFid;          // From request
    PWSTR                       pwszFilename;   // Do not free

    PLWIO_SRV_SESSION           pSession;
    PLWIO_SRV_TREE              pTree;
    PLWIO_SRV_FILE              pFile;
    PLWIO_SRV_FILE              pRootDir;

    IO_FILE_HANDLE              hFile;
    IO_FILE_NAME                fileName;

    IO_FILE_NAME                dirPath;
    IO_FILE_HANDLE              hDir;

    PBYTE                       pData2;
    USHORT                      usBytesAllocated;
    USHORT                      usBytesUsed;

    BOOLEAN                     bSetInfoAttempted;

} SRV_TRANS2_STATE_SMB_V1, *PSRV_TRANS2_STATE_SMB_V1;

typedef enum
{
    SRV_CLOSE_STAGE_SMB_V1_INITIAL = 0,
    SRV_CLOSE_STAGE_SMB_V1_SET_INFO_COMPLETED,
    SRV_CLOSE_STAGE_SMB_V1_ATTEMPT_CLOSE,
    SRV_CLOSE_STAGE_SMB_V1_BUILD_RESPONSE,
    SRV_CLOSE_STAGE_SMB_V1_DONE
} SRV_CLOSE_STAGE_SMB_V1;

typedef struct _SRV_CLOSE_STATE_SMB_V1
{
    LONG                       refCount;

    pthread_mutex_t            mutex;
    pthread_mutex_t*           pMutex;

    SRV_CLOSE_STAGE_SMB_V1     stage;

    IO_STATUS_BLOCK            ioStatusBlock;

    IO_ASYNC_CONTROL_BLOCK     acb;
    PIO_ASYNC_CONTROL_BLOCK    pAcb;

    PVOID                      pSecurityDescriptor;
    PVOID                      pSecurityQOS;

    PCLOSE_REQUEST_HEADER      pRequestHeader; // Do not free

    PLWIO_SRV_FILE             pFile;
    FILE_BASIC_INFORMATION     fileBasicInfo;

} SRV_CLOSE_STATE_SMB_V1, *PSRV_CLOSE_STATE_SMB_V1;

typedef enum
{
    SRV_CREATEDIR_STAGE_SMB_V1_INITIAL = 0,
    SRV_CREATEDIR_STAGE_SMB_V1_COMPLETED,
    SRV_CREATEDIR_STAGE_SMB_V1_BUILD_RESPONSE,
    SRV_CREATEDIR_STAGE_SMB_V1_DONE
} SRV_CREATEDIR_STAGE_SMB_V1;

typedef struct _SRV_CREATEDIR_STATE_SMB_V1
{
    LONG                       refCount;

    pthread_mutex_t            mutex;
    pthread_mutex_t*           pMutex;

    SRV_CREATEDIR_STAGE_SMB_V1 stage;

    IO_STATUS_BLOCK            ioStatusBlock;

    IO_ASYNC_CONTROL_BLOCK     acb;
    PIO_ASYNC_CONTROL_BLOCK    pAcb;

    PSMB_CREATE_DIRECTORY_REQUEST_HEADER pRequestHeader;   // Do not free
    PWSTR                                pwszPathFragment; // Do not free

    PVOID                      pSecurityDescriptor;
    PVOID                      pSecurityQOS;
    PIO_ECP_LIST               pEcpList;

    IO_FILE_HANDLE             hFile;
    IO_FILE_NAME               fileName;

} SRV_CREATEDIR_STATE_SMB_V1, *PSRV_CREATEDIR_STATE_SMB_V1;

typedef enum
{
    SRV_DELETEDIR_STAGE_SMB_V1_INITIAL = 0,
    SRV_DELETEDIR_STAGE_SMB_V1_ATTEMPT_SET_INFO,
    SRV_DELETEDIR_STAGE_SMB_V1_COMPLETED,
    SRV_DELETEDIR_STAGE_SMB_V1_BUILD_RESPONSE,
    SRV_DELETEDIR_STAGE_SMB_V1_DONE
} SRV_DELETEDIR_STAGE_SMB_V1;

typedef struct _SRV_DELETEDIR_STATE_SMB_V1
{
    LONG                       refCount;

    pthread_mutex_t            mutex;
    pthread_mutex_t*           pMutex;

    SRV_DELETEDIR_STAGE_SMB_V1 stage;

    IO_STATUS_BLOCK            ioStatusBlock;

    IO_ASYNC_CONTROL_BLOCK     acb;
    PIO_ASYNC_CONTROL_BLOCK    pAcb;

    PDELETE_DIRECTORY_REQUEST_HEADER pRequestHeader;   // Do not free
    PWSTR                            pwszPathFragment; // Do not free

    PVOID                      pSecurityDescriptor;
    PVOID                      pSecurityQOS;
    PIO_ECP_LIST               pEcpList;

    FILE_DISPOSITION_INFORMATION  fileDispositionInfo;
    PFILE_DISPOSITION_INFORMATION pFileDispositionInfo;

    IO_FILE_HANDLE             hFile;
    IO_FILE_NAME               fileName;

} SRV_DELETEDIR_STATE_SMB_V1, *PSRV_DELETEDIR_STATE_SMB_V1;

typedef enum
{
    SRV_DELETE_STAGE_SMB_V1_INITIAL = 0,
    SRV_DELETE_STAGE_SMB_V1_DELETE_FILES,
    SRV_DELETE_STAGE_SMB_V1_BUILD_RESPONSE,
    SRV_DELETE_STAGE_SMB_V1_DONE
} SRV_DELETE_STAGE_SMB_V1;

typedef struct _SRV_DELETE_STATE_SMB_V1
{
    LONG                        refCount;

    pthread_mutex_t             mutex;
    pthread_mutex_t*            pMutex;

    SRV_DELETE_STAGE_SMB_V1     stage;

    IO_ASYNC_CONTROL_BLOCK      acb;
    PIO_ASYNC_CONTROL_BLOCK     pAcb;

    IO_STATUS_BLOCK             ioStatusBlock;

    PSMB_DELETE_REQUEST_HEADER  pRequestHeader;    // Do not free
    PWSTR                       pwszSearchPattern; // Do not free
    BOOLEAN                     bUseLongFilenames;

    PLWIO_SRV_SESSION           pSession;
    PLWIO_SRV_TREE              pTree;

    BOOLEAN                     bPathHasWildCards;

    PWSTR                       pwszFilesystemPath;
    PWSTR                       pwszSearchPattern2;
    HANDLE                      hSearchSpace;
    USHORT                      usSearchId;
    ULONG                       ulSearchStorageType;

    BOOLEAN                     bEndOfSearch;
    USHORT                      usSearchResultCount;
    USHORT                      iResult;
    PBYTE                       pData;
    USHORT                      usDataLen;
    USHORT                      usDataOffset;

    IO_FILE_HANDLE                            hFile;
    IO_FILE_NAME                              fileName;
    PVOID                                     pSecurityDescriptor;
    PVOID                                     pSecurityQOS;
    PIO_ECP_LIST                              pEcpList;
    FILE_CREATE_OPTIONS                       ulCreateOptions;
    BOOLEAN                                   bPendingCreate;
    PSMB_FIND_FILE_BOTH_DIRECTORY_INFO_HEADER pResult; // Do not free

} SRV_DELETE_STATE_SMB_V1, *PSRV_DELETE_STATE_SMB_V1;

typedef enum
{
    SRV_RENAME_STAGE_SMB_V1_INITIAL = 0,
    SRV_RENAME_STAGE_SMB_V1_ATTEMPT_RENAME,
    SRV_RENAME_STAGE_SMB_V1_BUILD_RESPONSE,
    SRV_RENAME_STAGE_SMB_V1_DONE
} SRV_RENAME_STAGE_SMB_V1;

typedef struct _SRV_RENAME_STATE_SMB_V1
{
    LONG                       refCount;

    pthread_mutex_t            mutex;
    pthread_mutex_t*           pMutex;

    SRV_RENAME_STAGE_SMB_V1    stage;

    IO_STATUS_BLOCK            ioStatusBlock;

    IO_ASYNC_CONTROL_BLOCK     acb;
    PIO_ASYNC_CONTROL_BLOCK    pAcb;

    PSMB_RENAME_REQUEST_HEADER pRequestHeader;  // Do not free
    PWSTR                      pwszOldName;     // Do not free
    PWSTR                      pwszNewName;     // Do not free

    PVOID                      pSecurityDescriptor;
    PVOID                      pSecurityQOS;

    PIO_ECP_LIST               pDirEcpList;
    PIO_ECP_LIST               pFileEcpList;

    IO_FILE_NAME               oldName;
    IO_FILE_NAME               newName;         // Do not free these contents
    IO_FILE_NAME               dirPath;
    IO_FILE_HANDLE             hFile;
    IO_FILE_HANDLE             hDir;

    PFILE_RENAME_INFORMATION   pFileRenameInfo; // Do not free
    PBYTE                      pData;
    ULONG                      ulDataLen;

} SRV_RENAME_STATE_SMB_V1, *PSRV_RENAME_STATE_SMB_V1;

typedef enum
{
    SRV_NT_RENAME_STAGE_SMB_V1_INITIAL = 0,
    SRV_NT_RENAME_STAGE_SMB_V1_ATTEMPT_RENAME,
    SRV_NT_RENAME_STAGE_SMB_V1_BUILD_RESPONSE,
    SRV_NT_RENAME_STAGE_SMB_V1_DONE
} SRV_NT_RENAME_STAGE_SMB_V1;

typedef struct _SRV_NT_RENAME_STATE_SMB_V1
{
    LONG                       refCount;

    pthread_mutex_t            mutex;
    pthread_mutex_t*           pMutex;

    SRV_NT_RENAME_STAGE_SMB_V1 stage;

    IO_STATUS_BLOCK            ioStatusBlock;

    IO_ASYNC_CONTROL_BLOCK     acb;
    PIO_ASYNC_CONTROL_BLOCK    pAcb;

    PSMB_NT_RENAME_REQUEST_HEADER pRequestHeader;  // Do not free
    PWSTR                         pwszOldName;     // Do not free
    PWSTR                         pwszNewName;     // Do not free

    PVOID                      pSecurityDescriptor;
    PVOID                      pSecurityQOS;

    PIO_ECP_LIST               pDirEcpList;
    PIO_ECP_LIST               pFileEcpList;

    IO_FILE_NAME               oldName;
    IO_FILE_NAME               newName;         // Do not free these contents
    IO_FILE_NAME               dirPath;
    IO_FILE_HANDLE             hFile;
    IO_FILE_HANDLE             hDir;

    PFILE_RENAME_INFORMATION   pFileRenameInfo; // Do not free
    PBYTE                      pData;
    ULONG                      ulDataLen;

} SRV_NT_RENAME_STATE_SMB_V1, *PSRV_NT_RENAME_STATE_SMB_V1;

typedef enum
{
    SRV_NTTRANSACT_STAGE_SMB_V1_INITIAL = 0,
    SRV_NTTRANSACT_STAGE_SMB_V1_CREATE_COMPLETED,
    SRV_NTTRANSACT_STAGE_SMB_V1_QUERY_INFO,
    SRV_NTTRANSACT_STAGE_SMB_V1_GET_OPLOCKS,
    SRV_NTTRANSACT_STAGE_SMB_V1_ATTEMPT_IO,
    SRV_NTTRANSACT_STAGE_SMB_V1_BUILD_RESPONSE,
    SRV_NTTRANSACT_STAGE_SMB_V1_DONE
} SRV_NTTRANSACT_STAGE_SMB_V1;

typedef struct _SRV_NTTRANSACT_STATE_SMB_V1
{
    LONG                           refCount;

    pthread_mutex_t                mutex;
    pthread_mutex_t*               pMutex;

    SRV_NTTRANSACT_STAGE_SMB_V1    stage;

    IO_STATUS_BLOCK                ioStatusBlock;

    IO_ASYNC_CONTROL_BLOCK         acb;
    PIO_ASYNC_CONTROL_BLOCK        pAcb;

    PVOID                          pSecurityQOS;

    PNT_TRANSACTION_REQUEST_HEADER pRequestHeader; // Do not free
    PUSHORT                        pusBytecount;   // Do not free
    PUSHORT                        pSetup;         // Do not free
    PBYTE                          pParameters;    // Do not free
    PBYTE                          pData;          // Do not free
    PWSTR                          pwszFilename;   // Do not free
    PBYTE                          pSecDesc;       // Do not free
    PBYTE                          pEA;            // Do not free

    PLWIO_SRV_SESSION              pSession;
    PLWIO_SRV_TREE                 pTree;
    PLWIO_SRV_FILE                 pFile;

    IO_FILE_HANDLE                 hFile;
    PIO_FILE_NAME                  pFilename;
    PLWIO_SRV_FILE                 pRootDirectory;
    PIO_ECP_LIST                   pEcpList;
    ULONG                          ulCreateAction;
    UCHAR                          ucOplockLevel;
    BOOLEAN                        bRemoveFileFromTree;

    FILE_BASIC_INFORMATION         fileBasicInfo;
    PFILE_BASIC_INFORMATION        pFileBasicInfo;

    FILE_STANDARD_INFORMATION      fileStdInfo;
    PFILE_STANDARD_INFORMATION     pFileStdInfo;

    FILE_PIPE_INFORMATION          filePipeInfo;
    PFILE_PIPE_INFORMATION         pFilePipeInfo;

    FILE_PIPE_LOCAL_INFORMATION    filePipeLocalInfo;
    PFILE_PIPE_LOCAL_INFORMATION   pFilePipeLocalInfo;

    PBYTE                          pSecurityDescriptor2;
    ULONG                          ulSecurityDescAllocLen;
    ULONG                          ulSecurityDescActualLen;

    PSMB_IOCTL_HEADER              pIoctlRequest;
    PBYTE                          pResponseBuffer;
    USHORT                         usResponseBufferLen;
    USHORT                         usActualResponseLen;

    PSMB_SECURITY_INFORMATION_HEADER pSecurityRequestHeader;
    PSMB_NOTIFY_CHANGE_HEADER        pNotifyChangeHeader;
    PSMB_NT_TRANSACT_CREATE_REQUEST_HEADER   pNtTransactCreateHeader;

} SRV_NTTRANSACT_STATE_SMB_V1, *PSRV_NTTRANSACT_STATE_SMB_V1;

typedef enum
{
    SRV_FLUSH_STAGE_SMB_V1_INITIAL = 0,
    SRV_FLUSH_STAGE_SMB_V1_FLUSH_COMPLETED,
    SRV_FLUSH_STAGE_SMB_V1_BUILD_RESPONSE,
    SRV_FLUSH_STAGE_SMB_V1_DONE
} SRV_FLUSH_STAGE_SMB_V1;

typedef struct _SRV_FLUSH_STATE_SMB_V1
{
    LONG                      refCount;

    pthread_mutex_t           mutex;
    pthread_mutex_t*          pMutex;

    SRV_FLUSH_STAGE_SMB_V1    stage;

    IO_STATUS_BLOCK           ioStatusBlock;

    IO_ASYNC_CONTROL_BLOCK    acb;
    PIO_ASYNC_CONTROL_BLOCK   pAcb;

    PFLUSH_REQUEST_HEADER     pRequestHeader; // Do not free

    PLWIO_SRV_FILE            pFile;

} SRV_FLUSH_STATE_SMB_V1, *PSRV_FLUSH_STATE_SMB_V1;

typedef enum
{
    SRV_CHECKDIR_STAGE_SMB_V1_INITIAL = 0,
    SRV_CHECKDIR_STAGE_SMB_V1_ATTEMPT_CHECK,
    SRV_CHECKDIR_STAGE_SMB_V1_BUILD_RESPONSE,
    SRV_CHECKDIR_STAGE_SMB_V1_DONE
} SRV_CHECKDIR_STAGE_SMB_V1;

typedef struct _SRV_CHECKDIR_STATE_SMB_V1
{
    LONG                      refCount;

    pthread_mutex_t           mutex;
    pthread_mutex_t*          pMutex;

    SRV_CHECKDIR_STAGE_SMB_V1 stage;

    IO_STATUS_BLOCK           ioStatusBlock;

    IO_ASYNC_CONTROL_BLOCK    acb;
    PIO_ASYNC_CONTROL_BLOCK   pAcb;

    PWSTR                               pwszPathFragment; // Do not free
    PSMB_CHECK_DIRECTORY_REQUEST_HEADER pRequestHeader;   // Do not free

    IO_FILE_NAME              fileName;
    IO_FILE_HANDLE            hFile;
    PVOID                     pSecurityDescriptor;
    PVOID                     pSecurityQOS;
    PIO_ECP_LIST              pEcpList;

} SRV_CHECKDIR_STATE_SMB_V1, *PSRV_CHECKDIR_STATE_SMB_V1;

typedef struct _SRV_TREE_NOTIFY_STATE_REPOSITORY
{
    LONG              refCount;

    pthread_mutex_t   mutex;
    pthread_mutex_t*  pMutex;

    USHORT            usTid;

    PLWRTL_RB_TREE    pNotifyStateCollection;

} SRV_TREE_NOTIFY_STATE_REPOSITORY, *PSRV_TREE_NOTIFY_STATE_REPOSITORY;

typedef VOID (*PFN_SRV_MESSAGE_STATE_RELEASE_SMB_V1)(HANDLE hState);

typedef struct __SRV_MESSAGE_SMB_V1
{
    ULONG        ulSerialNum;       // Sequence # in packet; starts from 0

    PBYTE        pBuffer;           // Raw packet buffer
    ULONG        ulOffset;          // Offset of packet buffer within message

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
    ULONG        ulZctMessageSize;

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

/* Add new config parameters in alphabetic order. */
typedef struct _SRV_CONFIG_SMB_V1
{
    DWORD dwOplockTimeoutMillisecs;

} SRV_CONFIG_SMB_V1, *PSRV_CONFIG_SMB_V1;

typedef struct _SRV_RUNTIME_GLOBALS_SMB_V1
{
    pthread_mutex_t       mutex;

    PSMB_PROD_CONS_QUEUE  pWorkQueue;

    pthread_rwlock_t      configLock;
    pthread_rwlock_t      *pConfigLock;
    SRV_CONFIG_SMB_V1     config;

} SRV_RUNTIME_GLOBALS_SMB_V1, *PSRV_RUNTIME_GLOBALS_SMB_V1;

#endif /* __STRUCTS_H__ */



/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
