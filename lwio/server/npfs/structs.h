/*
 * Copyright (c) Likewise Software.  All rights reserved.
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
 * license@likewise.com
 */

#ifndef __NPFS_STRUCTS_H__
#define __NPFS_STRUCTS_H__

struct _NPFS_PIPE;
struct _NPFS_FCB;

typedef struct _NPFS_INTERLOCKED_ULONG
{
    ULONG           ulCounter;
    pthread_mutex_t CounterMutex;

} NPFS_INTERLOCKED_ULONG, *PNPFS_INTERLOCKED_ULONG;

typedef struct _NPFS_MDL
{
    ULONG Length;
    ULONG Offset;
    PVOID Buffer;

    struct _NPFS_MDL *pNext;

} NPFS_MDL, *PNPFS_MDL;

typedef struct _NPFS_CCB
{
    NPFS_INTERLOCKED_ULONG cRef;

    ULONG CcbType;

    ULONG CompletionState;
    ULONG ReadMode;

    struct _NPFS_PIPE * pPipe;
    PNPFS_MDL pMdlList;

    struct _NPFS_CCB * pNext;

} NPFS_CCB, *PNPFS_CCB;


typedef struct _NPFS_PIPE
{
    NPFS_INTERLOCKED_ULONG cRef;

    struct _NPFS_FCB *pFCB;
    pthread_mutex_t PipeMutex;
    pthread_cond_t PipeCondition;
    ULONG PipeClientState;
    ULONG PipeServerState;
    PNPFS_CCB pSCB;
    PNPFS_CCB pCCB;
    PBYTE pSessionKey;
    ULONG ulSessionKeyLength;
    PSTR pszClientPrincipalName;
    ULONG ulClientAddress;

    struct _NPFS_PIPE *pNext;

} NPFS_PIPE, *PNPFS_PIPE;

typedef struct _NPFS_FCB
{
    NPFS_INTERLOCKED_ULONG cRef;

    pthread_rwlock_t       PipeListRWLock;
    UNICODE_STRING         PipeName;
    ULONG                  NamedPipeConfiguration;
    FILE_PIPE_TYPE_MASK    NamedPipeType;
    ULONG                  MaxNumberOfInstances;
    ULONG                  CurrentNumberOfInstances;
    ULONG                  Max;
    PNPFS_PIPE             pPipes;

    struct _NPFS_FCB *pNext;

}NPFS_FCB, *PNPFS_FCB;

typedef struct _NPFS_IRP_CONTEXT
{
    PIRP pIrp;
} NPFS_IRP_CONTEXT, *PNPFS_IRP_CONTEXT;

typedef enum _PVFS_INFO_TYPE
{
    NPFS_QUERY = 1,
    NPFS_SET
} NPFS_INFO_TYPE, *PNPFS_INFO_TYPE;

#endif /* __NPFS_STRUCTS_H__ */
