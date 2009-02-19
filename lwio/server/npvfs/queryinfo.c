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
 *        queryinfo.c
 *
 * Abstract:
 *
 *        Likewise Named Pipe File System Driver (PVFS)
 *
 *        Query Info Dispatch Routines
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "npfs.h"

/* File Globals */

struct _InfoLevelDispatchEntry {
    FILE_INFORMATION_CLASS Level;
    NTSTATUS (*fn)(NPFS_INFO_TYPE RequestType,
                   PNPFS_IRP_CONTEXT pIrpContext);
};

static struct _InfoLevelDispatchEntry InfoLevelDispatchTable[] = {
    { FileDirectoryInformation,         NULL },
    { FileFullDirectoryInformation,     NULL },
    { FileBothDirectoryInformation,     NULL },
    { FileBasicInformation,             &NpfsFileBasicInfo },
    { FileStandardInformation,          &NpfsFileStandardInfo },
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
    { FilePipeInformation,              &NpfsFilePipeInfo },
    { FilePipeLocalInformation,         &NpfsFilePipeLocalInfo },
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
NpfsQueryInformation(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;
    PNPFS_IRP_CONTEXT pIrpContext = NULL;

    ntStatus = NpfsAllocateIrpContext(
                        pIrp,
                        &pIrpContext
                        );
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NpfsCommonQueryInformation(
                        pIrpContext,
                        pIrp);
    BAIL_ON_NT_STATUS(ntStatus);

error:

    return ntStatus;
}
NTSTATUS
NpfsCommonQueryInformation(
    PNPFS_IRP_CONTEXT  pIrpContext,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
    FILE_INFORMATION_CLASS InfoLevel = 0;
    int i = 0;
    size_t sizeTable = sizeof(InfoLevelDispatchTable) /
                       sizeof(struct _InfoLevelDispatchEntry);

    InfoLevel = pIrpContext->pIrp->Args.QuerySetInformation.FileInformationClass;

    if (InfoLevel >= FileMaximumInformation)
    {
        ntStatus = STATUS_INVALID_INFO_CLASS;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    /* Loop through the dispatch table.  Levels included in the table
       but having a NULL handler get NOT_SUPPORTED while those not in
       the table at all get NOT_IMPLEMENTED. */

    for (i=0; i<sizeTable; i++)
    {
        if (InfoLevelDispatchTable[i].Level == InfoLevel)
        {
            if (InfoLevelDispatchTable[i].fn == NULL)
            {
                ntStatus = STATUS_NOT_SUPPORTED;
                break;
            }

            ntStatus = InfoLevelDispatchTable[i].fn(NPFS_QUERY, pIrpContext);
            break;
        }
    }
    if (i == sizeTable) {
        ntStatus = STATUS_NOT_IMPLEMENTED;
    }
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:
    return ntStatus;

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

