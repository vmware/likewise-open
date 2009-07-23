/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        context.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Protocols API - SMBV2
 *
 *        Session
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

NTSTATUS
SrvSession2FindTree_SMB_V2(
    PSMB2_CONTEXT       pContext,
    PLWIO_SRV_SESSION_2 pSession,
    ULONG               ulTid,
    PLWIO_SRV_TREE_2*   ppTree
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLWIO_SRV_TREE_2 pTree = NULL;

    if (ulTid)
    {
        ntStatus = SrvSession2FindTree(
                        pSession,
                        ulTid,
                        &pTree);
    }
    else if (pContext->pTree)
    {
        pTree = pContext->pTree;
        InterlockedIncrement(&pTree->refcount);
    }
    else
    {
        ntStatus = STATUS_BAD_NETWORK_NAME;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    *ppTree = pTree;

cleanup:

    return ntStatus;

error:

    *ppTree = NULL;

    goto cleanup;
}
