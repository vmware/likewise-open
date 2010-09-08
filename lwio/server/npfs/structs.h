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
 *        structs.h
 *
 * Abstract:
 *
 *        Likewise Named Pipe File System (NPFS)
 *
 *        Structures
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

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
    LW_LIST_LINKS ReadIrpList;

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
    BYTE ClientAddress[16];
    USHORT usClientAddressLen;
    PACCESS_TOKEN pClientAccessToken;

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

typedef enum _NPFS_INFO_TYPE
{
    NPFS_QUERY = 1,
    NPFS_SET
} NPFS_INFO_TYPE, *PNPFS_INFO_TYPE;

