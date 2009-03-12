/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
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
 *        includes
 *
 * Abstract:
 *
 *        Likewise Posix File System (SMBSS)
 *
 *        Service Entry API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Gerald Carter <gcarter@likewise.com>
 */

#ifndef __PVFS_H__
#define __PVFS_H__

#include "config.h"

#if defined(HAVE_ATTR_XATTR_H) && defined(HAVE_FSETXATTR) && defined(HAVE_FGETXATTR)
#  ifdef __LWI_LINUX__
#    define HAVE_EA_SUPPORT
#  endif
#endif


#include "lwiosys.h"

#include <lw/base.h>
#include <lw/security-types.h>

#include "iodriver.h"
#include "lwioutils.h"

#include "structs.h"
#include "macros.h"
#include "fileinfo_p.h"
#include "security_p.h"
#include "create_p.h"
#include "alloc_p.h"
#include "time_p.h"
#include "syswrap_p.h"
#include "synchronize_p.h"
#include "ccb_p.h"
#include "acl.h"

/* Unix (POSIX) APIs */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <utime.h>

#ifdef HAVE_SYS_VFS_H
#  include <sys/vfs.h>
#endif

#ifdef HAVE_ATTR_XATTR_H
#  include <attr/xattr.h>
#endif

/* Top level APi functions */

NTSTATUS
PvfsCreate(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PPVFS_IRP_CONTEXT  pIrpContext
    );

NTSTATUS
PvfsDeviceIo(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PPVFS_IRP_CONTEXT  pIrpContext
    );

NTSTATUS
PvfsFsCtrl(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PPVFS_IRP_CONTEXT  pIrpContext
    );

NTSTATUS
PvfsWrite(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PPVFS_IRP_CONTEXT  pIrpContext
    );

NTSTATUS
PvfsRead(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PPVFS_IRP_CONTEXT  pIrpContext
    );

NTSTATUS
PvfsClose(
    IO_DEVICE_HANDLE DeviceHandle,
    PPVFS_IRP_CONTEXT  pIrpContext
    );

NTSTATUS
PvfsQuerySetInformation(
    PVFS_INFO_TYPE RequestType,
    IO_DEVICE_HANDLE IoDeviceHandle,
    PPVFS_IRP_CONTEXT  pIrpContext
    );

NTSTATUS
PvfsQueryDirInformation(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PPVFS_IRP_CONTEXT  pIrpContext
    );

NTSTATUS
PvfsQueryVolumeInformation(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PPVFS_IRP_CONTEXT  pIrpContext
    );

NTSTATUS
PvfsLockControl(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PPVFS_IRP_CONTEXT pIrpContext
    );

NTSTATUS
PvfsFlushBuffers(
    IO_DEVICE_HANDLE DeviceHandle,
    PPVFS_IRP_CONTEXT  pIrpContext
    );

NTSTATUS
PvfsQuerySetSecurityFile(
    PVFS_INFO_TYPE RequestType,
    IO_DEVICE_HANDLE IoDeviceHandle,
    PPVFS_IRP_CONTEXT pIrpContext
    );


/* From errno.c */

NTSTATUS
PvfsMapUnixErrnoToNtStatus(
    int err
    );


/* From unixpath.c */

NTSTATUS
PvfsCanonicalPathName(
    PSTR *ppszPath,
    IO_FILE_NAME IoPath
    );

NTSTATUS
PvfsWC16CanonicalPathName(
    PSTR *ppszPath,
    PWSTR pwszPathname
    );

NTSTATUS
PvfsValidatePath(
    PPVFS_CCB pCcb
    );


/* From wildcard.c */

BOOLEAN
PvfsWildcardMatch(
    IN PSTR pszPathname,
    IN PSTR pszPattern,
    IN BOOLEAN bCaseSensitive
    );


/* From util_open.c */

NTSTATUS
MapPosixOpenFlags(
    int *unixFlags,
    ACCESS_MASK Access,
    IRP_ARGS_CREATE CreateArgs
    );


/* From string.c */

VOID
PvfsCStringUpper(
	PSTR pszString
	);

#endif /* __PVFS_H__ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
