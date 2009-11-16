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
 *        connection.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Protocols API - SMBV1
 *
 *        Connection
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

NTSTATUS
SrvConnectionFindSession_SMB_V1(
    PSRV_EXEC_CONTEXT_SMB_V1 pSmb1Context,
    PLWIO_SRV_CONNECTION     pConnection,
    USHORT                   usUid,
    PLWIO_SRV_SESSION*       ppSession
    )
{
    NTSTATUS          ntStatus = STATUS_SUCCESS;
    PLWIO_SRV_SESSION pSession = NULL;

    if (usUid)
    {
        if (pSmb1Context->pSession)
        {
            if (pSmb1Context->pSession->uid != usUid)
            {
                ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
                BAIL_ON_NT_STATUS(ntStatus);
            }
            else
            {
                pSession = SrvSessionAcquire(pSmb1Context->pSession);
            }
        }
        else
        {
            ntStatus = SrvConnectionFindSession(
                            pConnection,
                            usUid,
                            &pSession);
            BAIL_ON_NT_STATUS(ntStatus);

            pSmb1Context->pSession = SrvSessionAcquire(pSession);
        }
    }
    else if (pSmb1Context->pSession)
    {
        pSession = SrvSessionAcquire(pSmb1Context->pSession);
    }
    else
    {
        ntStatus = STATUS_NO_SUCH_LOGON_SESSION;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppSession = pSession;

cleanup:

    return ntStatus;

error:

    *ppSession = NULL;

    if (pSession)
    {
        SrvSessionRelease(pSession);
    }

    goto cleanup;
}



NTSTATUS
SrvConnectionFindTree_SMB_V1(
    PSRV_EXEC_CONTEXT_SMB_V1 pSmb1Context,
    PLWIO_SRV_CONNECTION     pConnection,
    USHORT                   usTid,
    PLWIO_SRV_TREE*          ppTree
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLWIO_SRV_TREE pTree = NULL;

    if (usTid)
    {
        if (pSmb1Context->pTree)
        {
            if (pSmb1Context->pTree->tid != usTid)
            {
                ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
                BAIL_ON_NT_STATUS(ntStatus);
            }
            else
            {
                pTree = SrvTreeAcquire(pSmb1Context->pTree);
            }
        }
        else
        {
            ntStatus = SrvConnectionFindTree(
                            pConnection,
                            usTid,
                            &pTree);
            BAIL_ON_NT_STATUS(ntStatus);

            pSmb1Context->pTree = SrvTreeAcquire(pTree);
        }
    }
    else if (pSmb1Context->pTree)
    {
        pTree = SrvTreeAcquire(pSmb1Context->pTree);
    }
    else
    {
        ntStatus = STATUS_BAD_NETWORK_NAME;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppTree = pTree;

cleanup:

    return ntStatus;

error:

    *ppTree = NULL;

    if (pTree)
    {
        SrvTreeRelease(pTree);
    }

    goto cleanup;
}
