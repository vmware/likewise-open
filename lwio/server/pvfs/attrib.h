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
 *        attrib.h
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        Supporting DOS Attribute routines
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#ifndef _PVFS_ATTRIB_H_
#define _PVFS_ATTRIB_H_

NTSTATUS
PvfsGetFileAttributes(
    IN PPVFS_CCB pCcb,
    OUT PFILE_ATTRIBUTES pAttributes
    );

NTSTATUS
PvfsGetFilenameAttributes(
    IN PSTR pszPath,
    OUT PFILE_ATTRIBUTES pAttributes
    );

/* Used for SetFileBasicInformation(), et. al. */

NTSTATUS
PvfsSetFileAttributes(
    IN PPVFS_CCB pCcb,
    IN FILE_ATTRIBUTES Attributes
    );

/* Use by DeviceIoControl() */

NTSTATUS
PvfsSetFileAttributesEx(
    IN PPVFS_CCB pCcb,
    IN FILE_ATTRIBUTES Attributes
    );

#ifdef HAVE_EA_SUPPORT

/* From  attrib_xattr.c */

NTSTATUS
PvfsGetFileAttributesXattr(
    IN PPVFS_CCB pCcb,
    OUT PFILE_ATTRIBUTES pAttributes
    );

NTSTATUS
PvfsGetFilenameAttributesXattr(
    IN PSTR pszPath,
    OUT PFILE_ATTRIBUTES pAttributes
    );

NTSTATUS
PvfsSetFileAttributesXattr(
    IN PPVFS_CCB pCcb,
    IN FILE_ATTRIBUTES Attributes
    );

#endif   /* HAVE_EA_SUPPORT */

#endif     /* _PVFS_ATTRIB_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/

