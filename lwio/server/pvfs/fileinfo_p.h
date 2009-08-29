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
 *        fileinfo_p.h
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        FileInformation Handlers
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */


/* Query/Set FileInformation */

NTSTATUS
PvfsFileBasicInfo(
    PVFS_INFO_TYPE Type,
    PPVFS_IRP_CONTEXT pIrpContext
    );

NTSTATUS
PvfsFileStandardInfo(
    PVFS_INFO_TYPE Type,
    PPVFS_IRP_CONTEXT pIrpContext
    );

NTSTATUS
PvfsFileInternalInfo(
    PVFS_INFO_TYPE Type,
    PPVFS_IRP_CONTEXT pIrpContext
    );

NTSTATUS
PvfsFileEaInfo(
    PVFS_INFO_TYPE Type,
    PPVFS_IRP_CONTEXT pIrpContext
    );

NTSTATUS
PvfsFileStreamInfo(
    PVFS_INFO_TYPE Type,
    PPVFS_IRP_CONTEXT pIrpContext
    );

NTSTATUS
PvfsFileEndOfFileInfo(
    PVFS_INFO_TYPE Type,
    PPVFS_IRP_CONTEXT pIrpContext
    );

NTSTATUS
PvfsFileDispositionInfo(
    PVFS_INFO_TYPE Type,
    PPVFS_IRP_CONTEXT pIrpContext
    );

NTSTATUS
PvfsFileRenameInfo(
    PVFS_INFO_TYPE Type,
    PPVFS_IRP_CONTEXT pIrpContext
    );

NTSTATUS
PvfsFileAllocationInfo(
    PVFS_INFO_TYPE Type,
    PPVFS_IRP_CONTEXT pIrpContext
    );

NTSTATUS
PvfsFileAllInfo(
    PVFS_INFO_TYPE Type,
    PPVFS_IRP_CONTEXT pIrpContext
    );


/* QueryDirectoryInformation */

NTSTATUS
PvfsFileFullDirInfo(
    PVFS_INFO_TYPE Type,
    PPVFS_IRP_CONTEXT pIrpContext
    );

NTSTATUS
PvfsFileBothDirInfo(
    PVFS_INFO_TYPE Type,
    PPVFS_IRP_CONTEXT pIrpContext
    );

NTSTATUS
PvfsFileDirInfo(
    PVFS_INFO_TYPE Type,
    PPVFS_IRP_CONTEXT pIrpContext
    );



/* QueryVolumeInformation */

NTSTATUS
PvfsFileFsAttribInfo(
    PVFS_INFO_TYPE Type,
    PPVFS_IRP_CONTEXT pIrpContext
    );

NTSTATUS
PvfsFileFsVolInfo(
    PVFS_INFO_TYPE Type,
    PPVFS_IRP_CONTEXT pIrpContext
    );

NTSTATUS
PvfsFileFsSizeInfo(
    PVFS_INFO_TYPE Type,
    PPVFS_IRP_CONTEXT pIrpContext
    );


/* utility functions */

/* From util_dir.c */

NTSTATUS
PvfsEnumerateDirectory(
    PPVFS_CCB pCcb,
    PIRP_ARGS_QUERY_DIRECTORY pQueryDirArgs
    );

VOID
PvfsFreeDirectoryContext(
    PPVFS_DIRECTORY_CONTEXT pDirCtx
    );


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/

