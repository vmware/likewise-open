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
 *        Tree
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

NTSTATUS
SrvTree2FindFile_SMB_V2(
    PSRV_EXEC_CONTEXT_SMB_V2 pSmb2Context,
    PLWIO_SRV_TREE_2         pTree,
    PSMB2_FID                pFid,
    PLWIO_SRV_FILE_2*        ppFile
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLWIO_SRV_FILE_2 pFile = NULL;

    if (!pFid)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if ((pFid->ullPersistentId == 0xFFFFFFFFFFFFFFFFLL) &&
            (pFid->ullVolatileId == 0xFFFFFFFFFFFFFFFFLL))
    {
        if (!pSmb2Context->pFile)
        {
            ntStatus = STATUS_INVALID_PARAMETER;
        }
        else
        {
            pFile = SrvFile2Acquire(pSmb2Context->pFile);
        }
    }
    else if (pSmb2Context->pFile) // explicit id specified must match
    {
        if ((pSmb2Context->pFile->fid.ullPersistentId == pFid->ullPersistentId) &&
            (pSmb2Context->pFile->fid.ullVolatileId == pFid->ullVolatileId))
        {
            pFile = SrvFile2Acquire(pSmb2Context->pFile);
        }
        else
        {
            ntStatus = STATUS_INVALID_PARAMETER;
        }
    }
    else
    {
        ntStatus = SrvTree2FindFile(pTree, pFid, &pFile);
        BAIL_ON_NT_STATUS(ntStatus);

        pSmb2Context->pFile = SrvFile2Acquire(pFile);
    }
    BAIL_ON_NT_STATUS(ntStatus);

    if (!pFile)
    {
        ntStatus = STATUS_INVALID_HANDLE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppFile = pFile;

cleanup:

    return ntStatus;

error:

    *ppFile = NULL;

    goto cleanup;
}
