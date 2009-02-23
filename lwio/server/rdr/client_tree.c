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

#include "rdr.h"

static
NTSTATUS
SMBSrvClientTreeCreate(
    IN PSMB_SESSION pSession,
    IN PCSTR pszPath,
    OUT PSMB_TREE* ppTree
    );

NTSTATUS
SMBSrvClientTreeOpen(
    PCSTR pszHostname,
    PCSTR pszPrincipal,
    PCSTR pszSharename,
    PSMB_TREE* ppTree
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_SOCKET pSocket = NULL;
    PSMB_SESSION pSession = NULL;
    PSMB_TREE pTree = NULL;

    ntStatus = SMBSrvClientSocketCreate(
                    pszHostname,
                    &pSocket);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBSrvClientSessionCreate(
                    pSocket,
                    pszPrincipal,
                    &pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBSrvClientTreeCreate(
                    pSession,
                    pszSharename,
                    &pTree);
    BAIL_ON_NT_STATUS(ntStatus);

    SMBTreeAddReference(pTree);

    *ppTree = pTree;

cleanup:

    return ntStatus;

error:

    if (pTree)
    {
        SMBTreeRelease(pTree);
    }

    if (pSession)
    {
        SMBSrvClientSessionRelease(pSession);
    }

    if (pSocket)
    {
        SMBSocketRelease(pSocket);
    }

    goto cleanup;
}

static
NTSTATUS
SMBSrvClientTreeCreate(
    IN PSMB_SESSION pSession,
    IN PCSTR pszPath,
    OUT PSMB_TREE* ppTree
    )
{
    DWORD     ntStatus = 0;
    PSMB_TREE pTree = NULL;
    PWSTR     pwszPath = NULL;
    BOOLEAN   bAddedTreeByPath = FALSE;

    ntStatus = SMBSessionFindTreeByPath(
                    pSession,
                    pszPath,
                    &pTree);
    if (!ntStatus)
    {
        goto done;
    }

    ntStatus = SMBTreeCreate(&pTree);
    BAIL_ON_NT_STATUS(ntStatus);

    pTree->pSession = pSession;

    SMB_SAFE_FREE_MEMORY(pTree->pszPath);

    /* Path is trusted */
    ntStatus = SMBStrndup(
                    pszPath,
                    strlen(pszPath) + 1,
                    &pTree->pszPath);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBSrvClientSessionAddTreeByPath(pSession, pTree);
    BAIL_ON_NT_STATUS(ntStatus);

    bAddedTreeByPath = TRUE;

    /* @todo: once we can hash Unicode case-insensitively, we can remove this
       hack and go fully Unicode-native */
    ntStatus = SMBMbsToWc16s((char *) pszPath, &pwszPath);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = TreeConnect(pSession, pwszPath, &pTree->tid);
    BAIL_ON_NT_STATUS(ntStatus);

    /* Set state and awake any waiting threads */
    /* @todo: move into TreeConenct */
    SMBTreeSetState(pTree, SMB_RESOURCE_STATE_VALID);

    ntStatus = SMBSrvClientSessionAddTreeById(pSession, pTree);
    BAIL_ON_NT_STATUS(ntStatus);

done:

    *ppTree = pTree;

cleanup:

    SMB_SAFE_FREE_MEMORY(pwszPath);

    return ntStatus;

error:

    *ppTree = NULL;

    if (pTree)
    {
        if (bAddedTreeByPath)
        {
            SMBSrvClientSessionRemoveTreeByPath(pSession, pTree);
        }

        SMBTreeInvalidate(pTree, ERROR_SMB, ntStatus);

        SMBTreeRelease(pTree);
    }

    goto cleanup;
}

NTSTATUS
SMBSrvClientTreeAddResponse(
    PSMB_TREE     pTree,
    PSMB_RESPONSE pResponse
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;

    SMB_LOCK_MUTEX(bInLock, &pTree->mutex);

    /* @todo: if we allocate the MID outside of this function, we need to
       check for a conflict here */
    ntStatus = SMBHashSetValue(
                    pTree->pResponseHash,
                    &pResponse->mid,
                    pResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pResponse->pTree)
    {
        SMBTreeRelease(pResponse->pTree);
    }
    SMBTreeAddReference(pTree);

    pResponse->pTree = pTree;

cleanup:

    SMB_UNLOCK_MUTEX(bInLock, &pTree->mutex);

    return ntStatus;

error:

    goto cleanup;
}

/* Must be called with the tree mutex held */
NTSTATUS
SMBSrvClientTreeIsStale_inlock(
    PSMB_TREE pTree,
    PBOOLEAN  pbIsStale
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bIsStale = FALSE;
    SMB_HASH_ITERATOR iterator;

    if (pTree->refCount > 2)
    {
        goto done;
    }

    ntStatus = SMBHashGetIterator(
                    pTree->pResponseHash,
                    &iterator);
    BAIL_ON_NT_STATUS(ntStatus);

    if (SMBHashNext(&iterator))
    {
        goto done;
    }

    /* @todo: find a tick function which won't jump backward */
    /* @todo: make idle time configurable */
    if (difftime(time(NULL), pTree->lastActiveTime) < 60*15)
    {
        goto done;
    }

    bIsStale = TRUE;

done:

    *pbIsStale = bIsStale;

cleanup:

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
SMBSrvClientTreeClose(
    PSMB_TREE pTree
    )
{
    if (pTree)
    {
        SMBTreeRelease(pTree);
    }

    return 0;
}
