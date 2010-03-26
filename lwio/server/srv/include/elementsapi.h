/*
 * Copyright Likewise Software    2004-2009
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
 *        elementsapi.h
 *
 * Abstract:
 *
 *        Likewise Input Output (LWIO) - SRV
 *
 *        Elements API
 *
 * Authors: Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */

#ifndef __ELEMENTSAPI_H__
#define __ELEMENTSAPI_H__

#define SRV_LRU_CAPACITY             64

#define SMB_FIND_CLOSE_AFTER_REQUEST 0x1
#define SMB_FIND_CLOSE_IF_EOS        0x2
#define SMB_FIND_RETURN_RESUME_KEYS  0x4
#define SMB_FIND_CONTINUE_SEARCH     0x8
#define SMB_FIND_WITH_BACKUP_INTENT  0x10

typedef UCHAR SMB_OPLOCK_LEVEL;

#define SMB_OPLOCK_LEVEL_NONE  0x00
#define SMB_OPLOCK_LEVEL_I     0x01
#define SMB_OPLOCK_LEVEL_BATCH 0x02
#define SMB_OPLOCK_LEVEL_II    0x03

#define SMB_CN_MAX_BUFFER_SIZE 0x00010000

typedef VOID (*PFN_LWIO_SRV_FREE_OPLOCK_STATE)(HANDLE hOplockState);
typedef VOID (*PFN_LWIO_SRV_FREE_BRL_STATE)(HANDLE hByteRangeLockState);
typedef VOID (*PFN_LWIO_SRV_FREE_ASYNC_STATE)(HANDLE hAsyncState);

typedef struct __SMB2_FID
{
    ULONG64 ullPersistentId;
    ULONG64 ullVolatileId;
} __attribute__((__packed__)) SMB2_FID, *PSMB2_FID;

typedef struct _LWIO_ASYNC_STATE
{
    pthread_rwlock_t               mutex;
    pthread_rwlock_t*              pMutex;

    LONG                           refcount;

    ULONG64                        ullAsyncId;
    USHORT                         usCommand;

    HANDLE                         hAsyncState;
    PFN_LWIO_SRV_FREE_ASYNC_STATE  pfnFreeAsyncState;

} LWIO_ASYNC_STATE, *PLWIO_ASYNC_STATE;

typedef struct _LWIO_SRV_FILE
{
    pthread_rwlock_t        mutex;
    pthread_rwlock_t*       pMutex;

    LONG                    refcount;

    USHORT                  fid;

    IO_FILE_HANDLE          hFile;
    PIO_FILE_NAME           pFilename; // physical path on server
    PWSTR                   pwszFilename; // requested path
    ACCESS_MASK             desiredAccess;
    LONG64                  allocationSize;
    FILE_ATTRIBUTES         fileAttributes;
    FILE_SHARE_FLAGS        shareAccess;
    FILE_CREATE_DISPOSITION createDisposition;
    FILE_CREATE_OPTIONS     createOptions;

    UCHAR                          ucCurrentOplockLevel;

    HANDLE                         hOplockState;
    PFN_LWIO_SRV_FREE_OPLOCK_STATE pfnFreeOplockState;

    HANDLE                         hByteRangeLockState;
    PFN_LWIO_SRV_FREE_BRL_STATE    pfnFreeByteRangeLockState;
    ULONG64                        ullLastFailedLockOffset;

} LWIO_SRV_FILE, *PLWIO_SRV_FILE;

typedef struct _LWIO_SRV_SEARCH_SPACE_2
{
    UCHAR                  ucInfoClass;
    UCHAR                  ucSearchFlags;
    ULONG                  ulFileIndex;
    PWSTR                  pwszSearchPatternRaw;
    PWSTR                  pwszSearchPatternRef;
    ULONG                  ulSearchPatternLength;
    PWSTR                  pwszSearchPattern;
    PBYTE                  pFileInfo;
    PBYTE                  pFileInfoCursor;
    USHORT                 usFileInfoLen;
    BOOLEAN                bUseLongFilenames;

} LWIO_SRV_SEARCH_SPACE_2, *PLWIO_SRV_SEARCH_SPACE_2;

typedef struct _LWIO_SRV_FILE_2
{
    pthread_rwlock_t        mutex;
    pthread_rwlock_t*       pMutex;

    LONG                    refcount;

    SMB2_FID                fid;

    IO_FILE_HANDLE          hFile;
    PIO_FILE_NAME           pFilename; // physical path on server
    PWSTR                   pwszFilename; // requested path
    ACCESS_MASK             desiredAccess;
    LONG64                  allocationSize;
    FILE_ATTRIBUTES         fileAttributes;
    FILE_SHARE_FLAGS        shareAccess;
    FILE_CREATE_DISPOSITION createDisposition;
    FILE_CREATE_OPTIONS     createOptions;

    LWIO_SRV_SEARCH_SPACE_2  searchSpace;
    PLWIO_SRV_SEARCH_SPACE_2 pSearchSpace;

    UCHAR                          ucCurrentOplockLevel;

    HANDLE                         hOplockState;
    PFN_LWIO_SRV_FREE_OPLOCK_STATE pfnFreeOplockState;

} LWIO_SRV_FILE_2, *PLWIO_SRV_FILE_2;

typedef struct _LWIO_SRV_TREE
{
    LONG              refcount;

    pthread_rwlock_t  mutex;
    pthread_rwlock_t* pMutex;

    USHORT            tid;

    PSRV_SHARE_INFO   pShareInfo;

    PLWIO_SRV_FILE    lruFile[SRV_LRU_CAPACITY];

    PLWRTL_RB_TREE    pFileCollection;

    PLWRTL_RB_TREE    pAsyncStateCollection;

    USHORT            nextAvailableFid;

} LWIO_SRV_TREE, *PLWIO_SRV_TREE;

typedef struct _LWIO_SRV_TREE_2
{
    LONG              refcount;

    pthread_rwlock_t  mutex;
    pthread_rwlock_t* pMutex;

    ULONG             ulTid;

    PSRV_SHARE_INFO   pShareInfo;

    PLWIO_SRV_FILE_2  lruFile[SRV_LRU_CAPACITY];

    PLWRTL_RB_TREE    pFileCollection;

    ULONG64           ullNextAvailableFid;

} LWIO_SRV_TREE_2, *PLWIO_SRV_TREE_2;

typedef struct _LWIO_SRV_SESSION
{
    LONG              refcount;

    pthread_rwlock_t   mutex;
    pthread_rwlock_t*  pMutex;

    USHORT            uid;

    PLWIO_SRV_TREE    lruTree[SRV_LRU_CAPACITY];

    PLWRTL_RB_TREE    pTreeCollection;

    HANDLE            hFinderRepository;

    USHORT            nextAvailableTid;

    PSTR              pszClientPrincipalName;

    PIO_CREATE_SECURITY_CONTEXT   pIoSecurityContext;

} LWIO_SRV_SESSION, *PLWIO_SRV_SESSION;

typedef struct _LWIO_SRV_SESSION_2
{
    LONG              refcount;

    pthread_rwlock_t   mutex;
    pthread_rwlock_t*  pMutex;

    ULONG64           ullUid;

    PLWIO_SRV_TREE_2  lruTree[SRV_LRU_CAPACITY];

    PLWRTL_RB_TREE    pTreeCollection;

    HANDLE            hFinderRepository;

    ULONG             ulNextAvailableTid;

    PSTR              pszClientPrincipalName;

    PIO_CREATE_SECURITY_CONTEXT   pIoSecurityContext;

} LWIO_SRV_SESSION_2, *PLWIO_SRV_SESSION_2;

typedef enum
{
    LWIO_SRV_CONN_STATE_INITIAL = 0,
    LWIO_SRV_CONN_STATE_NEGOTIATE,
    LWIO_SRV_CONN_STATE_READY,
    LWIO_SRV_CONN_STATE_INVALID
} LWIO_SRV_CONN_STATE;

typedef struct _SRV_PROPERTIES
{
    USHORT  preferredSecurityMode;
    BOOLEAN bEncryptPasswords;
    BOOLEAN bEnableSecuritySignatures;
    BOOLEAN bRequireSecuritySignatures;
    USHORT  MaxMpxCount;
    USHORT  MaxNumberVCs;
    ULONG   MaxBufferSize;
    ULONG   MaxRawSize;
    ULONG   Capabilities;
    uuid_t  GUID;

} SRV_PROPERTIES, *PSRV_PROPERTIES;

typedef struct _SRV_CLIENT_PROPERITES
{

    USHORT MaxBufferSize;
    USHORT MaxMpxCount;
    USHORT VcNumber;
    ULONG  SessionKey;
    ULONG  Capabilities;
    PWSTR  pwszNativeOS;
    PWSTR  pwszNativeLanMan;
    PWSTR  pwszNativeDomain;

} SRV_CLIENT_PROPERTIES, *PSRV_CLIENT_PROPERTIES;

typedef VOID (*PFN_LWIO_SRV_FREE_SOCKET_HANDLE)(HANDLE hSocket);

typedef struct _LWIO_SRV_CONNECTION
{
    LONG                refCount;

    pthread_rwlock_t     mutex;
    pthread_rwlock_t*    pMutex;

    LWIO_SRV_CONN_STATE  state;

    HANDLE                          hSocket;
    PFN_LWIO_SRV_FREE_SOCKET_HANDLE pfnSocketFree;

    SRV_PROPERTIES        serverProperties;
    SRV_CLIENT_PROPERTIES clientProperties;

    SMB_PROTOCOL_VERSION protocolVer;

    ULONG           ulSequence;

    union
    {
        USHORT          usNextAvailableUid;
        ULONG64         ullNextAvailableUid;
    };

    // Invariant
    // Not owned
    HANDLE              hPacketAllocator;

    struct
    {
        BOOLEAN         bReadHeader;
        size_t          sNumBytesToRead;
        size_t          sOffset;
        PSMB_PACKET     pRequestPacket;

    } readerState;

    PBYTE               pSessionKey;
    ULONG               ulSessionKeyLength;

    PSRV_HOST_INFO             pHostinfo;
    PLWIO_SRV_SHARE_ENTRY_LIST pShareList;

    HANDLE              hGssContext;

    pthread_mutex_t     mutexGssNegotiate;
    pthread_mutex_t*    pMutexGssNegotiate;

    HANDLE              hGssNegotiate;

    union
    {
        PLWIO_SRV_SESSION   lruSession[SRV_LRU_CAPACITY];
        PLWIO_SRV_SESSION_2 lruSession2[SRV_LRU_CAPACITY];
    };

    PLWRTL_RB_TREE      pSessionCollection;

    ULONG64             ullNextAvailableAsyncId;

    PLWRTL_RB_TREE      pAsyncStateCollection;

} LWIO_SRV_CONNECTION, *PLWIO_SRV_CONNECTION;

typedef struct _SRV_FINDER_REPOSITORY
{
    LONG             refCount;

    pthread_mutex_t  mutex;
    pthread_mutex_t* pMutex;

    PLWRTL_RB_TREE   pSearchSpaceCollection;

    USHORT           usNextSearchId;

} SRV_FINDER_REPOSITORY, *PSRV_FINDER_REPOSITORY;

typedef struct _SRV_SEARCH_SPACE
{
    LONG             refCount;

    pthread_mutex_t  mutex;
    pthread_mutex_t* pMutex;

    USHORT           usSearchId;

    IO_FILE_HANDLE   hFile;
    PWSTR            pwszSearchPattern;
    USHORT           usSearchAttrs;
    ULONG            ulSearchStorageType;
    PBYTE            pFileInfo;
    PBYTE            pFileInfoCursor;
    USHORT           usFileInfoLen;
    SMB_INFO_LEVEL   infoLevel;
    BOOLEAN          bUseLongFilenames;

} SRV_SEARCH_SPACE, *PSRV_SEARCH_SPACE;

typedef struct _SRV_TIMER_REQUEST* PSRV_TIMER_REQUEST;

typedef VOID (*PFN_SRV_TIMER_CALLBACK)(
                    PSRV_TIMER_REQUEST pTimerRequest,
                    PVOID              pUserData
                    );

typedef struct _SMB_FS_VOLUME_INFO_HEADER
{
    LONG64  llVolumeCreationTime;
    ULONG   ulVolumeSerialNumber;
    ULONG   ulVolumeLabelLength;
    BOOLEAN bSupportsObjects;
    UCHAR   pad;
} __attribute__((__packed__)) SMB_FS_VOLUME_INFO_HEADER,
                             *PSMB_FS_VOLUME_INFO_HEADER;

typedef struct _SMB_FS_ATTRIBUTE_INFO_HEADER
{
    ULONG ulFSAttributes;
    LONG  lMaxFilenameLen;
    ULONG ulFileSystemNameLen;
} __attribute__((__packed__)) SMB_FS_ATTRIBUTE_INFO_HEADER,
                             *PSMB_FS_ATTRIBUTE_INFO_HEADER;

typedef struct _SMB_FS_INFO_VOLUME_HEADER
{
    ULONG   ulVolumeSerialNumber;
    UCHAR   ucNumLabelChars;

    /* PWSTR pwszLabel; */

} __attribute__((__packed__)) SMB_FS_INFO_VOLUME_HEADER,
                             *PSMB_FS_INFO_VOLUME_HEADER;

typedef struct _SMB_FS_FULL_INFO_HEADER
{
    ULONG64 ullTotalAllocationUnits;
    ULONG64 ullCallerAvailableAllocationUnits;
    ULONG64 ullAvailableAllocationUnits;
    ULONG   ulSectorsPerAllocationUnit;
    ULONG   ulBytesPerSector;

} __attribute__((__packed__)) SMB_FS_FULL_INFO_HEADER,
                             *PSMB_FS_FULL_INFO_HEADER;

typedef struct _SMB_FILE_INTERNAL_INFO_HEADER
{
    ULONG64 ullIndex;

} __attribute__((__packed__)) SMB_FILE_INTERNAL_INFO_HEADER,
                             *PSMB_FILE_INTERNAL_INFO_HEADER;

typedef struct _SMB_FILE_EA_INFO_HEADER
{
    ULONG ulEaSize;

} __attribute__((__packed__)) SMB_FILE_EA_INFO_HEADER,
                             *PSMB_FILE_EA_INFO_HEADER;

typedef struct _SRV_PROTOCOL_EXEC_CONTEXT* PSRV_PROTOCOL_EXEC_CONTEXT;

typedef VOID (*PFN_SRV_PROTOCOL_FREE_EXEC_CONTEXT)(
                        PSRV_PROTOCOL_EXEC_CONTEXT pContext
                        );

typedef struct _SRV_EXEC_CONTEXT
{
    LONG                               refCount;

    pthread_mutex_t                    mutex;
    pthread_mutex_t*                   pMutex;

    PLWIO_SRV_CONNECTION               pConnection;
    PSMB_PACKET                        pSmbRequest;

    PSRV_PROTOCOL_EXEC_CONTEXT         pProtocolContext;
    PFN_SRV_PROTOCOL_FREE_EXEC_CONTEXT pfnFreeContext;

    PSMB_PACKET                        pSmbResponse;
    ULONG                              ulNumDuplicates;

    PSMB_PACKET                        pInterimResponse;

    BOOLEAN                            bInternal;

    ULONG64                            ullAsyncId;

} SRV_EXEC_CONTEXT, *PSRV_EXEC_CONTEXT;

typedef struct _SRV_ELEMENTS_STATISTICS
{
    LONG64 llNumConnections;
    LONG64 llMaxNumConnections;

    LONG64 llNumSessions;
    LONG64 llMaxNumSessions;

    LONG64 llNumTreeConnects;
    LONG64 llMaxNumTreeConnects;

    LONG64 llNumOpenFiles;
    LONG64 llMaxNumOpenFiles;

} SRV_ELEMENTS_STATISTICS, *PSRV_ELEMENTS_STATISTICS;

NTSTATUS
SrvElementsInit(
    VOID
    );

NTSTATUS
SrvTimerPostRequest(
    IN  LONG64                 llExpiry,
    IN  PVOID                  pUserData,
    IN  PFN_SRV_TIMER_CALLBACK pfnTimerExpiredCB,
    OUT PSRV_TIMER_REQUEST*    ppTimerRequest
    );

NTSTATUS
SrvTimerCancelRequest(
    IN  PSRV_TIMER_REQUEST pTimerRequest,
    OUT PVOID*             ppUserData
    );

NTSTATUS
SrvTimerRelease(
    IN  PSRV_TIMER_REQUEST pTimerRequest
    );

NTSTATUS
SrvGssAcquireContext(
    PSRV_HOST_INFO pHostinfo,
    HANDLE         hGssOrig,
    PHANDLE        phGssNew
    );

BOOLEAN
SrvGssNegotiateIsComplete(
    HANDLE hGss,
    HANDLE hGssNegotiate
    );

NTSTATUS
SrvGssGetSessionDetails(
    HANDLE hGss,
    HANDLE hGssNegotiate,
    PBYTE* ppSessionKey,
    PULONG pulSessionKeyLength,
    PSTR* ppszClientPrincipalName,
    LW_MAP_SECURITY_GSS_CONTEXT* pContextHandle
    );

NTSTATUS
SrvGssBeginNegotiate(
    HANDLE  hGss,
    PHANDLE phGssNegotiate
    );

NTSTATUS
SrvGssNegotiate(
    HANDLE  hGss,
    HANDLE  hGssResume,
    PBYTE   pSecurityInputBlob,
    ULONG   ulSecurityInputBlobLen,
    PBYTE*  ppSecurityOutputBlob,
    ULONG*  pulSecurityOutputBloblen
    );

VOID
SrvGssEndNegotiate(
    HANDLE hGss,
    HANDLE hGssNegotiate
    );

VOID
SrvGssReleaseContext(
    HANDLE hGss
    );

NTSTATUS
SrvGssNegHints(
    HANDLE hGssContext,
    PBYTE *ppNegHints,
    ULONG *pulNegHintsLength
    );

ULONG64
SrvAsyncStateBuildId(
    ULONG  ulPid,
    USHORT usMid
    );

NTSTATUS
SrvAsyncStateCreate(
    ULONG64                       ullAsyncId,
    USHORT                        usCommand,
    HANDLE                        hAsyncState,
    PFN_LWIO_SRV_FREE_ASYNC_STATE pfnFreeAsyncState,
    PLWIO_ASYNC_STATE*            ppAsyncState
    );

PLWIO_ASYNC_STATE
SrvAsyncStateAcquire(
    PLWIO_ASYNC_STATE pAsyncState
    );

VOID
SrvAsyncStateRelease(
    PLWIO_ASYNC_STATE pAsyncState
    );

NTSTATUS
SrvConnectionCreate(
    HANDLE                          hSocket,
    HANDLE                          hPacketAllocator,
    HANDLE                          hGssContext,
    PLWIO_SRV_SHARE_ENTRY_LIST      pShareList,
    PSRV_PROPERTIES                 pServerProperties,
    PSRV_HOST_INFO                  pHostinfo,
    PFN_LWIO_SRV_FREE_SOCKET_HANDLE pfnSocketFree,
    PLWIO_SRV_CONNECTION*           ppConnection
    );

SMB_PROTOCOL_VERSION
SrvConnectionGetProtocolVersion(
    PLWIO_SRV_CONNECTION pConnection
    );

NTSTATUS
SrvConnectionSetProtocolVersion(
    PLWIO_SRV_CONNECTION pConnection,
    SMB_PROTOCOL_VERSION protoVer
    );

NTSTATUS
SrvConnectionSetProtocolVersion_inlock(
    PLWIO_SRV_CONNECTION pConnection,
    SMB_PROTOCOL_VERSION protoVer
    );

NTSTATUS
SrvConnectionCreateSession(
    PLWIO_SRV_CONNECTION pConnection,
    PLWIO_SRV_SESSION* ppSession
    );

NTSTATUS
SrvConnection2CreateSession(
    PLWIO_SRV_CONNECTION pConnection,
    PLWIO_SRV_SESSION_2* ppSession
    );

NTSTATUS
SrvConnectionFindSession(
    PLWIO_SRV_CONNECTION pConnection,
    USHORT               uid,
    PLWIO_SRV_SESSION*   ppSession
    );

NTSTATUS
SrvConnection2FindSession(
    PLWIO_SRV_CONNECTION pConnection,
    ULONG64              ullUid,
    PLWIO_SRV_SESSION_2* ppSession
    );

NTSTATUS
SrvConnection2CreateAsyncState(
    PLWIO_SRV_CONNECTION          pConnection,
    USHORT                        usCommand,
    PFN_LWIO_SRV_FREE_ASYNC_STATE pfnFreeAsyncState,
    PLWIO_ASYNC_STATE*            ppAsyncState
    );

NTSTATUS
SrvConnection2FindAsyncState(
    PLWIO_SRV_CONNECTION pConnection,
    ULONG64              ullAsyncId,
    PLWIO_ASYNC_STATE*   ppAsyncState
    );

NTSTATUS
SrvConnection2RemoveAsyncState(
    PLWIO_SRV_CONNECTION pConnection,
    ULONG64              ullAsyncId
    );

NTSTATUS
SrvConnectionGetNamedPipeClientAddress(
    PLWIO_SRV_CONNECTION pConnection,
    PIO_ECP_LIST        pEcpList
    );

NTSTATUS
SrvConnectionGetNamedPipeSessionKey(
    PLWIO_SRV_CONNECTION pConnection,
    PIO_ECP_LIST        pEcpList
    );

LWIO_SRV_CONN_STATE
SrvConnectionGetState(
    PLWIO_SRV_CONNECTION pConnection
    );

BOOLEAN
SrvConnectionIsInvalid(
    PLWIO_SRV_CONNECTION pConnection
    );

PLWIO_SRV_CONNECTION
SrvConnectionAcquire(
    PLWIO_SRV_CONNECTION pConnection
    );

VOID
SrvConnectionRelease(
    PLWIO_SRV_CONNECTION pConnection
    );

NTSTATUS
SrvConnectionRemoveSession(
    PLWIO_SRV_CONNECTION pConnection,
    USHORT              uid
    );

NTSTATUS
SrvConnection2RemoveSession(
    PLWIO_SRV_CONNECTION pConnection,
    ULONG64              ullUid
    );

VOID
SrvConnectionSetInvalid(
    PLWIO_SRV_CONNECTION pConnection
    );

VOID
SrvConnectionSetState(
    PLWIO_SRV_CONNECTION pConnection,
    LWIO_SRV_CONN_STATE  connState
    );

NTSTATUS
SrvSessionCreate(
    USHORT            uid,
    PLWIO_SRV_SESSION* ppSession
    );

NTSTATUS
SrvSessionFindTree(
    PLWIO_SRV_SESSION pSession,
    USHORT           tid,
    PLWIO_SRV_TREE*   ppTree
    );

NTSTATUS
SrvSessionRemoveTree(
    PLWIO_SRV_SESSION pSession,
    USHORT           tid
    );

NTSTATUS
SrvSessionCreateTree(
    PLWIO_SRV_SESSION pSession,
    PSRV_SHARE_INFO   pShareInfo,
    PLWIO_SRV_TREE*   ppTree
    );

PLWIO_SRV_SESSION
SrvSessionAcquire(
    PLWIO_SRV_SESSION pSession
    );

VOID
SrvSessionRelease(
    PLWIO_SRV_SESSION pSession
    );

VOID
SrvSessionRundown(
    PLWIO_SRV_SESSION pSession
    );

NTSTATUS
SrvSession2Create(
    ULONG64              ullUid,
    PLWIO_SRV_SESSION_2* ppSession
    );

NTSTATUS
SrvSession2FindTree(
    PLWIO_SRV_SESSION_2 pSession,
    ULONG               ulTid,
    PLWIO_SRV_TREE_2*   ppTree
    );

NTSTATUS
SrvSession2RemoveTree(
    PLWIO_SRV_SESSION_2 pSession,
    ULONG               ulTid
    );

NTSTATUS
SrvSession2CreateTree(
    PLWIO_SRV_SESSION_2 pSession,
    PSRV_SHARE_INFO     pShareInfo,
    PLWIO_SRV_TREE_2*   ppTree
    );

PLWIO_SRV_SESSION_2
SrvSession2Acquire(
    PLWIO_SRV_SESSION_2 pSession
    );

VOID
SrvSession2Release(
    PLWIO_SRV_SESSION_2 pSession
    );

VOID
SrvSession2Rundown(
    PLWIO_SRV_SESSION_2 pSession
    );

NTSTATUS
SrvTreeCreate(
    USHORT            tid,
    PSRV_SHARE_INFO   pShareInfo,
    PLWIO_SRV_TREE*    ppTree
    );

NTSTATUS
SrvTreeFindFile(
    PLWIO_SRV_TREE  pTree,
    USHORT         fid,
    PLWIO_SRV_FILE* ppFile
    );

NTSTATUS
SrvTreeCreateFile(
    PLWIO_SRV_TREE           pTree,
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
    );

NTSTATUS
SrvTreeRemoveFile(
    PLWIO_SRV_TREE pTree,
    USHORT        fid
    );

NTSTATUS
SrvTreeAddAsyncState(
    PLWIO_SRV_TREE    pTree,
    PLWIO_ASYNC_STATE pAsyncState
    );

NTSTATUS
SrvTreeFindAsyncState(
    PLWIO_SRV_TREE     pTree,
    ULONG64            ullAsyncId,
    PLWIO_ASYNC_STATE* ppAsyncState
    );

NTSTATUS
SrvTreeRemoveAsyncState(
    PLWIO_SRV_TREE pTree,
    ULONG64        ullAsyncId
    );

BOOLEAN
SrvTreeIsNamedPipe(
    PLWIO_SRV_TREE pTree
    );

NTSTATUS
SrvGetTreeRelativePath(
    PWSTR  pwszOriginalPath,
    PWSTR* ppwszSpecificPath
    );

PLWIO_SRV_TREE
SrvTreeAcquire(
    PLWIO_SRV_TREE pTree
    );

VOID
SrvTreeRelease(
    PLWIO_SRV_TREE pTree
    );

VOID
SrvTreeRundown(
    PLWIO_SRV_TREE pTree
    );

NTSTATUS
SrvTree2Create(
    ULONG             ulTid,
    PSRV_SHARE_INFO   pShareInfo,
    PLWIO_SRV_TREE_2* ppTree
    );

NTSTATUS
SrvTree2FindFile(
    PLWIO_SRV_TREE_2  pTree,
    PSMB2_FID         pFid,
    PLWIO_SRV_FILE_2* ppFile
    );

NTSTATUS
SrvTree2CreateFile(
    PLWIO_SRV_TREE_2        pTree,
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
    );

NTSTATUS
SrvTree2RemoveFile(
    PLWIO_SRV_TREE_2 pTree,
    PSMB2_FID        pFid
    );

BOOLEAN
SrvTree2IsNamedPipe(
    PLWIO_SRV_TREE_2 pTree
    );

PLWIO_SRV_TREE_2
SrvTree2Acquire(
    PLWIO_SRV_TREE_2 pTree
    );

VOID
SrvTree2Release(
    PLWIO_SRV_TREE_2 pTree
    );

VOID
SrvTree2Rundown(
    PLWIO_SRV_TREE_2 pTree
    );

NTSTATUS
SrvIoCreateFile(
    IN PSRV_SHARE_INFO pShareInfo,
    OUT PIO_FILE_HANDLE pFileHandle,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK pAsyncControlBlock,
    OUT PIO_STATUS_BLOCK pIoStatusBlock,
    IN PIO_CREATE_SECURITY_CONTEXT pSecurityContext,
    IN PIO_FILE_NAME pFileName,
    IN OPTIONAL PSECURITY_DESCRIPTOR_RELATIVE pSecurityDescriptor,
    IN OPTIONAL PVOID pSecurityQualityOfService,
    IN ACCESS_MASK DesiredAccess,
    IN OPTIONAL LONG64 AllocationSize,
    IN FILE_ATTRIBUTES FileAttributes,
    IN FILE_SHARE_FLAGS ShareAccess,
    IN FILE_CREATE_DISPOSITION CreateDisposition,
    IN FILE_CREATE_OPTIONS CreateOptions,
    IN OPTIONAL PVOID pEaBuffer,
    IN ULONG EaLength,
    IN OUT PIO_ECP_LIST* ppEcpList
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
    );

NTSTATUS
SrvFileSetOplockState(
    PLWIO_SRV_FILE                 pFile,
    HANDLE                         hOplockState,
    PFN_LWIO_SRV_FREE_OPLOCK_STATE pfnReleaseOplockState
    );

HANDLE
SrvFileRemoveOplockState(
    PLWIO_SRV_FILE pFile
    );

VOID
SrvFileResetOplockState(
    PLWIO_SRV_FILE pFile
    );

VOID
SrvFileSetOplockLevel(
    PLWIO_SRV_FILE pFile,
    UCHAR          ucOplockLevel
    );

UCHAR
SrvFileGetOplockLevel(
    PLWIO_SRV_FILE pFile
    );

VOID
SrvFileSetLastFailedLockOffset(
    PLWIO_SRV_FILE pFile,
    ULONG64        ullLastFailedLockOffset
    );

ULONG64
SrvFileGetLastFailedLockOffset(
    PLWIO_SRV_FILE pFile
    );

PLWIO_SRV_FILE
SrvFileAcquire(
    PLWIO_SRV_FILE pFile
    );

VOID
SrvFileRelease(
    PLWIO_SRV_FILE pFile
    );

VOID
SrvFileRundown(
    PLWIO_SRV_FILE pFile
    );

NTSTATUS
SrvFile2Create(
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
    );

NTSTATUS
SrvFile2SetOplockState(
    PLWIO_SRV_FILE_2               pFile,
    HANDLE                         hOplockState,
    PFN_LWIO_SRV_FREE_OPLOCK_STATE pfnReleaseOplockState
    );

HANDLE
SrvFile2RemoveOplockState(
    PLWIO_SRV_FILE_2 pFile
    );

VOID
SrvFile2ResetOplockState(
    PLWIO_SRV_FILE_2 pFile
    );

VOID
SrvFile2SetOplockLevel(
    PLWIO_SRV_FILE_2 pFile,
    UCHAR            ucOplockLevel
    );

UCHAR
SrvFile2GetOplockLevel(
    PLWIO_SRV_FILE_2 pFile
    );

PLWIO_SRV_FILE_2
SrvFile2Acquire(
    PLWIO_SRV_FILE_2 pFile
    );

VOID
SrvFile2Release(
    PLWIO_SRV_FILE_2 pFile
    );

VOID
SrvFile2Rundown(
    PLWIO_SRV_FILE_2 pFile
    );

NTSTATUS
SrvFinderCreateRepository(
    OUT PHANDLE phFinderRepository
    );

NTSTATUS
SrvFinderBuildSearchPath(
    IN              PWSTR    pwszPath,
    IN              PWSTR    pwszSearchPattern,
       OUT          PWSTR*   ppwszFilesystemPath,
       OUT          PWSTR*   ppwszSearchPattern,
    IN OUT OPTIONAL PBOOLEAN pbPathHasWildCards
    );

NTSTATUS
SrvFinderCreateSearchSpace(
    IN  PSRV_SHARE_INFO pShareInfo,
    IN  PIO_CREATE_SECURITY_CONTEXT pIoSecurityContext,
    IN  HANDLE         hFinderRepository,
    IN  PWSTR          pwszFilesystemPath,
    IN  PWSTR          pwszSearchPattern,
    IN  USHORT         usSearchAttrs,
    IN  ULONG          ulSearchStorageType,
    IN  SMB_INFO_LEVEL infoLevel,
    IN  BOOLEAN        bUseLongFilenames,
    IN  ACCESS_MASK    accessMask,
    OUT PHANDLE        phFinder,
    OUT PUSHORT        pusSearchId
    );

NTSTATUS
SrvFinderGetSearchSpace(
    IN  HANDLE  hFinderRepository,
    IN  USHORT  usSearchId,
    OUT PHANDLE phFinder
    );

VOID
SrvFinderReleaseSearchSpace(
    IN HANDLE hFinder
    );

NTSTATUS
SrvFinderCloseSearchSpace(
    IN HANDLE hFinderRepository,
    IN USHORT usSearchId
    );

VOID
SrvFinderCloseRepository(
    IN HANDLE hFinderRepository
    );

NTSTATUS
SrvBuildExecContext(
   IN  PLWIO_SRV_CONNECTION pConnection,
   IN  PSMB_PACKET          pSmbRequest,
   IN  BOOLEAN              bInternal,
   OUT PSRV_EXEC_CONTEXT*   ppContext
   );

NTSTATUS
SrvBuildEmptyExecContext(
   OUT PSRV_EXEC_CONTEXT* ppContext
   );

BOOLEAN
SrvIsValidExecContext(
   IN PSRV_EXEC_CONTEXT pExecContext
   );

VOID
SrvReleaseExecContextHandle(
   IN HANDLE hExecContext
   );

VOID
SrvReleaseExecContext(
   IN PSRV_EXEC_CONTEXT pContext
   );

NTSTATUS
SrvElementsGetBootTime(
    PULONG64 pullBootTime
    );

BOOLEAN
SrvElementsGetShareNameEcpEnabled(
    VOID
    );

NTSTATUS
SrvElementsGetStats(
    PSRV_ELEMENTS_STATISTICS pStats
    );

NTSTATUS
SrvElementsResetStats(
    VOID
    );

NTSTATUS
SrvElementsShutdown(
    VOID
    );

#endif /* __ELEMENTSAPI_H__ */
