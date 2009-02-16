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
 *        file_std_info.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        FileStandardInformation Handler
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "pvfs.h"

/* Forward declarations */

static NTSTATUS
PvfsQueryFileStandardInfo(
    PPVFS_IRP_CONTEXT pIrpContext
    );


/* File Globals */



/* Code */


NTSTATUS
PvfsFileStandardInfo(
    PVFS_INFO_TYPE Type,
    PPVFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;

    switch(Type)
    {
    case PVFS_SET:
        ntError = STATUS_NOT_SUPPORTED;
        break;
        
    case PVFS_QUERY:
        ntError = PvfsQueryFileStandardInfo(pIrpContext);
        break;

    default:
        ntError = STATUS_INVALID_PARAMETER;
        break;        
    }
    BAIL_ON_NT_STATUS(ntError);
    
cleanup:
    return ntError;

error:
    goto cleanup;    
}

static NTSTATUS
PvfsQueryFileStandardInfo(
    PPVFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;    
    PIRP pIrp = pIrpContext->pIrp;
    PPVFS_CCB pCcb = NULL;
    PFILE_STANDARD_INFORMATION pFileInfo = NULL;    
    IRP_ARGS_QUERY_SET_INFORMATION Args = pIrpContext->pIrp->Args.QuerySetInformation;
    PVFS_STAT Stat = {0};    

    /* Sanity checks */

    pCcb = (PPVFS_CCB)IoFileGetContext(pIrp->FileHandle);
    PVFS_BAIL_ON_INVALID_CCB(pCcb, ntError);

    /* No access checked needed for this call */

    BAIL_ON_INVALID_PTR(Args.FileInformation, ntError);

    if (Args.Length < sizeof(*pFileInfo))
    {
        ntError = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(ntError);        
    }

    pFileInfo = (PFILE_STANDARD_INFORMATION)Args.FileInformation;    

    /* Real work starts here */

    ntError = PvfsSysFstat(pCcb->fd, &Stat);
    BAIL_ON_NT_STATUS(ntError);    

    pFileInfo->AllocationSize = Stat.s_alloc;
    pFileInfo->EndOfFile      = Stat.s_size;
    pFileInfo->NumberOfLinks  = Stat.s_nlink;
    pFileInfo->DeletePending  = FALSE;          /* Ignore Delete-on-Close */
    pFileInfo->Directory      = S_ISDIR(Stat.s_mode) ? TRUE : FALSE;

    pIrp->IoStatusBlock.BytesTransferred = sizeof(*pFileInfo);
    ntError = STATUS_SUCCESS;

cleanup:
    return ntError;
    
error: 
    goto cleanup;    
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/

