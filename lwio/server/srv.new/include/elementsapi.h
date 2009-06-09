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

#define SMB_FIND_CLOSE_AFTER_REQUEST 0x1
#define SMB_FIND_CLOSE_IF_EOS        0x2
#define SMB_FIND_RETURN_RESUME_KEYS  0x4
#define SMB_FIND_CONTINUE_SEARCH     0x8
#define SMB_FIND_WITH_BACKUP_INTENT  0x10

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

} LWIO_SRV_FILE, *PLWIO_SRV_FILE;

typedef struct _LWIO_SRV_TREE
{
    LONG                   refcount;

    pthread_rwlock_t  mutex;
    pthread_rwlock_t* pMutex;

    USHORT            tid;

    PSRV_SHARE_INFO   pShareInfo;

    PLWRTL_RB_TREE    pFileCollection;

    USHORT            nextAvailableFid;

} LWIO_SRV_TREE, *PLWIO_SRV_TREE;

typedef struct _LWIO_SRV_SESSION
{
    LONG              refcount;

    pthread_rwlock_t   mutex;
    pthread_rwlock_t*  pMutex;

    USHORT            uid;

    PLWRTL_RB_TREE    pTreeCollection;

    HANDLE            hFinderRepository;

    USHORT            nextAvailableTid;

    PSTR              pszClientPrincipalName;

    PIO_CREATE_SECURITY_CONTEXT   pIoSecurityContext;

} LWIO_SRV_SESSION, *PLWIO_SRV_SESSION;

typedef enum
{
    LWIO_SRV_CONN_STATE_INITIAL = 0,
    LWIO_SRV_CONN_STATE_NEGOTIATE,
    LWIO_SRV_CONN_STATE_READY,
    LWIO_SRV_CONN_STATE_INVALID
} LWIO_SRV_CONN_STATE;

typedef struct LWIO_SRV_SOCKET* PLWIO_SRV_SOCKET;

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

typedef struct _LWIO_SRV_CONNECTION
{
    LONG                refCount;

    pthread_rwlock_t     mutex;
    pthread_rwlock_t*    pMutex;

    LWIO_SRV_CONN_STATE  state;

    PLWIO_SRV_SOCKET     pSocket;

    SRV_PROPERTIES        serverProperties;
    SRV_CLIENT_PROPERTIES clientProperties;

    ULONG               ulSequence;

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
    HANDLE              hGssNegotiate;

    PLWRTL_RB_TREE      pSessionCollection;

    USHORT              nextAvailableUid;

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

NTSTATUS
SrvElementsInit(
	VOID
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
    PSTR* ppszClientPrincipalName
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
SrvConnectionCreate(
    PLWIO_SRV_SOCKET           pSocket,
    HANDLE                    hPacketAllocator,
    HANDLE                    hGssContext,
    PLWIO_SRV_SHARE_ENTRY_LIST pShareList,
    PSRV_PROPERTIES           pServerProperties,
    PSRV_HOST_INFO            pHostinfo,
    PLWIO_SRV_CONNECTION*      ppConnection
    );

NTSTATUS
SrvConnectionCreateSession(
    PLWIO_SRV_CONNECTION pConnection,
    PLWIO_SRV_SESSION* ppSession
    );

NTSTATUS
SrvConnectionFindSession(
    PLWIO_SRV_CONNECTION pConnection,
    USHORT uid,
    PLWIO_SRV_SESSION* ppSession
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

VOID
SrvConnectionRelease(
    PLWIO_SRV_CONNECTION pConnection
    );

NTSTATUS
SrvConnectionRemoveSession(
    PLWIO_SRV_CONNECTION pConnection,
    USHORT              uid
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

NTSTATUS
SrvSessionGetNamedPipeClientPrincipal(
    PLWIO_SRV_SESSION pSession,
    PIO_ECP_LIST     pEcpList
    );

VOID
SrvSessionRelease(
    PLWIO_SRV_SESSION pSession
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

BOOLEAN
SrvTreeIsNamedPipe(
    PLWIO_SRV_TREE pTree
    );

VOID
SrvTreeRelease(
    PLWIO_SRV_TREE pTree
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

VOID
SrvFileRelease(
    PLWIO_SRV_FILE pFile
    );

NTSTATUS
SrvFinderCreateRepository(
    OUT PHANDLE phFinderRepository
    );

NTSTATUS
SrvFinderCreateSearchSpace(
    IN  PIO_CREATE_SECURITY_CONTEXT pIoSecurityContext,
    IN  HANDLE         hFinderRepository,
    IN  PWSTR          pwszFilesystemPath,
    IN  PWSTR          pwszSearchPattern,
    IN  USHORT         usSearchAttrs,
    IN  ULONG          ulSearchStorageType,
    IN  SMB_INFO_LEVEL infoLevel,
    IN  BOOLEAN        bUseLongFilenames,
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
SrvElementsShutdown(
    VOID
    );

#endif /* __ELEMENTSAPI_H__ */
