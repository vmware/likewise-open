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
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __PVFS_H__
#define __PVFS_H__

#include "config.h"
#include "lwiosys.h"

#include <lw/rtlstring.h>
#include <lw/rtlgoto.h>

#include "iodriver.h"

#include "lwioutils.h"
#include "structs.h"

NTSTATUS
NpfsCreate(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    );

NTSTATUS
NpfsCreateNamedPipe(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    );


NTSTATUS
NpfsDeviceIo(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    );

NTSTATUS
NpfsFsCtl(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    );

NTSTATUS
NpfsWrite(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    );


NTSTATUS
NpfsRead(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    );

NTSTATUS
NpfsClose(
    IO_DEVICE_HANDLE DeviceHandle,
    PIRP pIrp
    );

NTSTATUS
NpfsQueryInformation(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    );

NTSTATUS
NpfsSetInformation(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    );

#include "create.h"
#include "close.h"
#include "externs.h"

#define ENTER_READER_RW_LOCK(pMutex)

#define LEAVE_READER_RW_LOCK(pMutex)

#define ENTER_WRITER_RW_LOCK(pMutex)

#define LEAVE_WRITER_RW_LOCK(pMutex)


#define ENTER_MUTEX(pMutex)  pthread_mutex_lock(pMutex)

#define LEAVE_MUTEX(pMutex)  pthread_mutex_unlock(pMutex)

#define SERVER_CCB          1
#define CLIENT_CCB          2

#define NPFS_CCB_SERVER     1
#define NPFS_CCB_CLIENT     2

#define PIPE_SERVER_INIT_STATE  0
#define PIPE_SERVER_CONNECTED   1
#define PIPE_SERVER_DISCONNECTED 2
#define PIPE_SERVER_CREATED     3
#define PIPE_SERVER_WAITING_FOR_CONNECTION  4
#define PIPE_SERVER_CLOSED      5

#define PIPE_CLIENT_INIT_STATE  0
#define PIPE_CLIENT_CONNECTED   1
#define PIPE_CLIENT_CLOSED      2

#include "prototypes.h"

#endif /* __PVFS_H__ */



