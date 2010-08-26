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
 *        rdr.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        API (Client)
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#ifndef __RDR_H__
#define __RDR_H__

#include "config.h"
#include "lwiosys.h"

#include <openssl/md5.h>

#include <lw/base.h>
#include <lwio/lwio.h>

#include <lw/ntstatus.h>
#include <lw/rtlstring.h>
#include <lw/rtlgoto.h>

#include "lwiodef.h"
#include "lwioutils.h"
#include "lwiofsctl.h"
#include "smbkrb5.h"

#include "marshal.h"
#include "smbwire.h"
#include <lwio/io-types.h>
#include "iodriver.h"

#include "structs.h"
#include "socket.h"
#include "tree.h"
#include "session.h"
#include "connect.h"
#include "externs.h"

#define RDR_IDLE_TIMEOUT 10

NTSTATUS
RdrCreateContext(
    PIRP pIrp,
    PRDR_OP_CONTEXT* ppContext
    );

VOID
RdrFreeContext(
    PRDR_OP_CONTEXT pContext
    );


BOOLEAN
RdrContinueContext(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    );

VOID
RdrContinueContextList(
    PLW_LIST_LINKS pList,
    NTSTATUS status,
    PVOID pParam
    );

VOID
RdrNotifyContextList(
    PLW_LIST_LINKS pList,
    BOOLEAN bLocked,
    pthread_mutex_t* pMutex,
    NTSTATUS status,
    PVOID pParam
    );

NTSTATUS
RdrAllocatePacketBuffer(
    PSMB_PACKET pPacket,
    ULONG ulSize
    );

NTSTATUS
RdrAllocatePacket(
    ULONG ulSize,
    PSMB_PACKET* ppPacket
    );

VOID
RdrFreePacket(
    PSMB_PACKET pPacket
    );

NTSTATUS
RdrAllocateContextPacket(
    PRDR_OP_CONTEXT pContext,
    ULONG ulSize
    );

NTSTATUS
RdrCreate(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    );

NTSTATUS
RdrDeviceIo(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    );

NTSTATUS
RdrFsctl(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    );

NTSTATUS
RdrWrite(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    );


NTSTATUS
RdrRead(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    );

NTSTATUS
RdrClose(
    IO_DEVICE_HANDLE DeviceHandle,
    PIRP pIrp
    );

NTSTATUS
RdrQueryInformation(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    );

NTSTATUS
RdrQuerySecurity(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    );

NTSTATUS
RdrQueryDirectory(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    );

NTSTATUS
RdrQueryVolumeInformation(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    );

NTSTATUS
RdrSetInformation(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    );

NTSTATUS
RdrWriteFileEx(
    HANDLE hFile,
    DWORD  dwNumBytesToWrite,
    PVOID  pBuffer,
    PDWORD pdwNumBytesWritten
    );

NTSTATUS
RdrCreateFileEx(
    PIO_CREDS pSecurityToken,
    PIO_SECURITY_CONTEXT_PROCESS_INFORMATION pProcessInfo,
    PCWSTR pwszPath,
    ACCESS_MASK desiredAccess,
    LONG64 llAllocationSize,
    FILE_ATTRIBUTES fileAttributes,
    FILE_SHARE_FLAGS shareAccess,
    FILE_CREATE_DISPOSITION createDisposition,
    FILE_CREATE_OPTIONS createOptions,
    PHANDLE phFile
    );

NTSTATUS
RdrGetSessionKey(
    HANDLE hFile,
    PDWORD pdwSessionKeyLength,
    PBYTE* ppSessionKey
    );


NTSTATUS
RdrCommonFsctl(
    PRDR_OP_CONTEXT pIrpContext,
    PIRP pIrp
    );

NTSTATUS
RdrCallQueryInformationFile(
    HANDLE hFile,
    PVOID fileInformation,
    ULONG ulLength,
    FILE_INFORMATION_CLASS fileInformationClass,
    PULONG pulInfoLengthUsed
    );

NTSTATUS
RdrTransactSetInfoFile(
    PRDR_TREE pTree,
    USHORT usFid,
    SMB_INFO_LEVEL infoLevel,
    PVOID pInfo,
    ULONG ulInfoLength
    );

NTSTATUS
RdrTransactRenameFile(
    PRDR_TREE pTree,
    USHORT usSearchAttributes,
    PCWSTR pwszSourceFile,
    PCWSTR pwszDestFile
    );

NTSTATUS
RdrTransactNTRenameFile(
    PRDR_TREE pTree,
    USHORT usSearchAttributes,
    USHORT usInfoLevel,
    ULONG ulClusterCount,
    PCWSTR pwszSourceFile,
    PCWSTR pwszDestFile
    );

NTSTATUS
RdrTransactTrans2RenameFile(
    PRDR_TREE pTree,
    USHORT usFid,
    USHORT usFlags,
    PCWSTR pwszPath
    );

void
RdrReleaseFile(
    PRDR_CCB pFile
    );

#endif /* __RDR_H__ */
