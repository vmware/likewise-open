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
 *        driver.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        Driver Entry Function
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "pvfs.h"

struct _InfoLevelDispatchEntry {
    FILE_INFORMATION_CLASS Level;
    NTSTATUS (*fn)(PVFS_INFO_TYPE RequestType,
                   PPVFS_IRP_CONTEXT pIrpContext);
};

static struct _InfoLevelDispatchEntry InfoLevelDispatchTable[] = {
    { FileDirectoryInformation,         NULL },
    { FileFullDirectoryInformation,     NULL },
    { FileBothDirectoryInformation,     NULL },
    { FileBasicInformation,             NULL },
    { FileStandardInformation,          NULL },
    { FileInternalInformation,          NULL },
    { FileEaInformation,                NULL },
    { FileAccessInformation,            NULL },
    { FileNameInformation,              NULL },
    { FileRenameInformation,            NULL },
    { FileLinkInformation,              NULL },
    { FileNamesInformation,             NULL },
    { FileDispositionInformation,       NULL },
    { FilePositionInformation,          NULL },
    { FileFullEaInformation,            NULL },
    { FileModeInformation,              NULL },
    { FileAlignmentInformation,         NULL },
    { FileAllInformation,               NULL },
    { FileAllocationInformation,        NULL },
    { FileEndOfFileInformation,         NULL },
    { FileAlternateNameInformation,     NULL },
    { FileStreamInformation,            NULL },
    { FilePipeInformation,              NULL },
    { FilePipeLocalInformation,         NULL },
    { FilePipeRemoteInformation,        NULL },
    { FileMailslotQueryInformation,     NULL },
    { FileMailslotSetInformation,       NULL },
    { FileCompressionInformation,       NULL },
    { FileObjectIdInformation,          NULL },
    { FileCompletionInformation,        NULL },
    { FileMoveClusterInformation,       NULL },
    { FileQuotaInformation,             NULL },
    { FileReparsePointInformation,      NULL },
    { FileNetworkOpenInformation,       NULL },
    { FileAttributeTagInformation,      NULL },
    { FileTrackingInformation,          NULL },
    { FileIdBothDirectoryInformation,   NULL },
    { FileIdFullDirectoryInformation,   NULL },
    { FileValidDataLengthInformation,   NULL },
    { FileShortNameInformation,         NULL }
};


NTSTATUS
PvfsQuerySetInformation(
    PVFS_INFO_TYPE RequestType,
    IO_DEVICE_HANDLE IoDeviceHandle,
    PPVFS_IRP_CONTEXT  pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PIRP pIrp = pIrpContext->pIrp;
    FILE_INFORMATION_CLASS InfoLevel = 0;
    InfoLevel = pIrp->Args.QuerySetInformation.FileInformationClass;
    int i = 0;
    size_t sizeTable = sizeof(InfoLevelDispatchTable) /
                       sizeof(struct _InfoLevelDispatchEntry);


    if (InfoLevel >= FileMaximumInformation)
    {
        ntError = STATUS_INVALID_INFO_CLASS;
        BAIL_ON_NT_STATUS(ntError);
    }

    /* Loop through the dispatch table.  Levels included in the table
       but having a NULL handler get NOT_SUPPORTED while those not in
       the table at all get NOT_IMPLEMENTED. */

    for (i=0; i<sizeTable; i++)
    {
        if (InfoLevelDispatchTable[i].Level == InfoLevel)
        {
            if (InfoLevelDispatchTable[InfoLevel].fn == NULL)
            {
                ntError = STATUS_NOT_SUPPORTED;
                break;
            }

            ntError = InfoLevelDispatchTable[i].fn(RequestType, pIrpContext);
            break;
        }
    }
    if (i == sizeTable) {
        ntError = STATUS_NOT_IMPLEMENTED;
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

