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
 *        querydir.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        Query Directory Info dispatch driver
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "pvfs.h"

/* Forward declarations */



/* File Globals */

static struct _InfoLevelDispatchEntry InfoLevelDispatchTable[] = {
    { FileBothDirectoryInformation,     &PvfsFileBothDirInfo },
    { FileDirectoryInformation,         NULL },
    { FileFullDirectoryInformation,     NULL },
    { FileIdBothDirectoryInformation,   NULL },
    { FileIdFullDirectoryInformation,   NULL },
    { FileNamesInformation,             NULL },
    { FileObjectIdInformation,          NULL },
    { FileReparsePointInformation,      NULL }
};


/* Code */

NTSTATUS
PvfsQueryDirInformation(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PPVFS_IRP_CONTEXT  pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PIRP pIrp = pIrpContext->pIrp;
    FILE_INFORMATION_CLASS InfoLevel = 0;
    int i = 0;    
    size_t sizeTable = sizeof(InfoLevelDispatchTable) / 
                       sizeof(struct _InfoLevelDispatchEntry);

    InfoLevel = pIrp->Args.QueryDirectory.FileInformationClass;

    /* Loop through the dispatch table.  Levels included in the table
       but having a NULL handler get NOT_IMPLEMENTED while those not in
       the table at all get INVALID_INFO_CLASS. */

    for (i=0; i<sizeTable; i++)
    {
        if (InfoLevelDispatchTable[i].Level == InfoLevel)
        {
            if (InfoLevelDispatchTable[i].fn == NULL)
            {
                ntError = STATUS_NOT_IMPLEMENTED;
                break;                
            }
    
            ntError = InfoLevelDispatchTable[i].fn(PVFS_QUERY, pIrpContext);
            break;            
        }        
    }
    if (i == sizeTable) {
        ntError = STATUS_INVALID_INFO_CLASS;
    }    
    BAIL_ON_NT_STATUS(ntError);

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

