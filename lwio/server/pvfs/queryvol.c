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
 *        queryvol.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        Query Volume Info dispatch driver
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "pvfs.h"

/* Forward declarations */


/* File Globals */

static struct _InfoLevelDispatchEntry InfoLevelDispatchTable[] = {
    { (FILE_INFORMATION_CLASS)FileFsAttributeInformation,  PvfsFileFsAttribInfo },
    { (FILE_INFORMATION_CLASS)FileFsSizeInformation,       PvfsFileFsSizeInfo },
    { (FILE_INFORMATION_CLASS)FileFsVolumeInformation,     PvfsFileFsVolInfo },
    { (FILE_INFORMATION_CLASS)FileFsDeviceInformation,     PvfsFileFsDeviceInfo },
    { (FILE_INFORMATION_CLASS)FileFsControlInformation,    PvfsFileFsControlInfo }
};


/* Code */

static
NTSTATUS
PvfsQuerySetVolumeInformation(
    PPVFS_IRP_CONTEXT  pIrpContext,
    PVFS_INFO_TYPE infoType
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PIRP pIrp = pIrpContext->pIrp;
    FS_INFORMATION_CLASS InfoLevel = 0;
    int i = 0;
    size_t sizeTable = sizeof(InfoLevelDispatchTable) /
                       sizeof(struct _InfoLevelDispatchEntry);

    InfoLevel = pIrp->Args.QuerySetVolumeInformation.FsInformationClass;

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

            ntError = InfoLevelDispatchTable[i].fn(infoType, pIrpContext);
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


NTSTATUS
PvfsQueryVolumeInformation(
    PPVFS_IRP_CONTEXT  pIrpContext
    )
{
    return PvfsQuerySetVolumeInformation(pIrpContext, PVFS_QUERY);
}


NTSTATUS
PvfsSetVolumeInformation(
    PPVFS_IRP_CONTEXT  pIrpContext
    )
{
    return PvfsQuerySetVolumeInformation(pIrpContext, PVFS_SET);
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/

