/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
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
 *        includes
 *
 * Abstract:
 *
 *        Likewise Posix File System (SMBSS)
 *
 *        Service Entry API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __PVFS_H__
#define __PVFS_H__

#include "config.h"
#include "lwiosys.h"
#include "lwiofsctl.h"

#include <lw/rtlstring.h>
#include <lw/rtlgoto.h>

#include "iodriver.h"

#include "lwioutils.h"
#include "lwlist.h"

typedef struct _NPFS_IRP_CONTEXT
{
    LW_LIST_LINKS Link;
    PIRP pIrp;
} NPFS_IRP_CONTEXT, *PNPFS_IRP_CONTEXT;

typedef enum _NpfsCcbType
{
    NPFS_CCB_SERVER,
    NPFS_CCB_CLIENT
} NpfsCcbType;

typedef enum _NpfsServerState
{
    PIPE_SERVER_INIT_STATE,
    PIPE_SERVER_CONNECTED,
    PIPE_SERVER_DISCONNECTED,
    PIPE_SERVER_CREATED,
    PIPE_SERVER_WAITING_FOR_CONNECTION,
    PIPE_SERVER_CLOSED
} NpfsServerState;

typedef enum _NpfsClientState
{
    PIPE_CLIENT_INIT_STATE,
    PIPE_CLIENT_CONNECTED,
    PIPE_CLIENT_CLOSED
} NpfsClientState;

struct _NPFS_PIPE;
struct _NPFS_FCB;

typedef struct _NPFS_MDL
{
    ULONG Length;
    ULONG Offset;
    PVOID Buffer;

    LW_LIST_LINKS link;
} NPFS_MDL, *PNPFS_MDL;

typedef struct _NPFS_CCB
{
    LONG lRefCount;

    NpfsCcbType CcbType;

    ULONG CompletionState;
    ULONG ReadMode;

    struct _NPFS_PIPE * pPipe;

    LW_LIST_LINKS mdlList;

    LW_LIST_LINKS link;
} NPFS_CCB, *PNPFS_CCB;


typedef struct _NPFS_PIPE
{
    LONG lRefCount;

    struct _NPFS_FCB *pFCB;
    pthread_mutex_t PipeMutex;
    pthread_cond_t PipeCondition;
    NpfsClientState PipeClientState;
    NpfsServerState PipeServerState;
    PNPFS_CCB pSCB;
    PNPFS_CCB pCCB;
    PBYTE pSessionKey;
    ULONG ulSessionKeyLength;
    PSTR pszClientPrincipalName;
    ULONG ulClientAddress;

    PNPFS_IRP_CONTEXT pPendingServerConnect;

    LW_LIST_LINKS link;
} NPFS_PIPE, *PNPFS_PIPE;

typedef struct _NPFS_FCB
{
    LONG lRefCount;

    pthread_rwlock_t       PipeListRWLock;
    UNICODE_STRING         PipeName;
    ULONG                  NamedPipeConfiguration;
    FILE_PIPE_TYPE_MASK    NamedPipeType;
    ULONG                  MaxNumberOfInstances;
    ULONG                  CurrentNumberOfInstances;
    ULONG                  Max;
    LW_LIST_LINKS          pipeList;
    LW_LIST_LINKS          link;
}NPFS_FCB, *PNPFS_FCB;

typedef enum _PVFS_INFO_TYPE
{
    NPFS_QUERY = 1,
    NPFS_SET
} NPFS_INFO_TYPE, *PNPFS_INFO_TYPE;

NTSTATUS
NpfsCreate(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    );

NTSTATUS
NpfsCreateNamedPipe(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    );

NTSTATUS
NpfsDeviceIo(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    );

NTSTATUS
NpfsFsCtl(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    );

NTSTATUS
NpfsWrite(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    );


NTSTATUS
NpfsRead(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    );

NTSTATUS
NpfsClose(
    IO_DEVICE_HANDLE DeviceHandle,
    PIRP pIrp
    );

NTSTATUS
NpfsQueryInformation(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    );

NTSTATUS
NpfsSetInformation(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    );

NTSTATUS
NpfsCommonCreate(
    PNPFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

NTSTATUS
NpfsValidateCreate(
    PNPFS_IRP_CONTEXT pIrpContext,
    PUNICODE_STRING pPipeName
    );

NTSTATUS
NpfsAllocateIrpContext(
    PIRP pIrp,
    PNPFS_IRP_CONTEXT * ppIrpContext
    );


NTSTATUS
NpfsFreeIrpContext(
    PNPFS_IRP_CONTEXT pIrpContext
    );

NTSTATUS
NpfsClose(
    IO_DEVICE_HANDLE DeviceHandle,
    PIRP pIrp
    );


NTSTATUS
NpfsCommonClose(
    PNPFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

NTSTATUS
NpfsCloseHandle(
    PNPFS_CCB pCCB
    );

NTSTATUS
NpfsServerCloseHandle(
    PNPFS_CCB pSCB
    );

NTSTATUS
NpfsClientCloseHandle(
    PNPFS_CCB pCCB
    );

NTSTATUS
NpfsServerFreeCCB(
    PNPFS_CCB pSCB
    );

NTSTATUS
NpfsClientFreeCCB(
    PNPFS_CCB pCCB
    );

extern LW_LIST_LINKS gFCBList;
extern pthread_rwlock_t gServerLock;

#define ENTER_READER_RW_LOCK(pMutex) pthread_rwlock_rdlock(pMutex)

#define LEAVE_READER_RW_LOCK(pMutex) pthread_rwlock_unlock(pMutex)

#define ENTER_WRITER_RW_LOCK(pMutex) pthread_rwlock_wrlock(pMutex)

#define LEAVE_WRITER_RW_LOCK(pMutex) pthread_rwlock_unlock(pMutex)


#define ENTER_MUTEX(pMutex)  pthread_mutex_lock(pMutex)

#define LEAVE_MUTEX(pMutex)  pthread_mutex_unlock(pMutex)

#include "prototypes.h"

#endif /* __PVFS_H__ */
