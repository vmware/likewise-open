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
 *        regshare.c
 *
 * Abstract:
 *
 *        Likewise Server Message Block (LSMB)
 *
 *        Server sub-system
 *
 *        Server share database interface
 *
 * Authors: Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */
#include "includes.h"

NTSTATUS
SrvShareRegInit(
    VOID
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
SrvShareRegOpen(
    OUT PHANDLE phRepository
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
SrvShareRegFindByName(
    HANDLE           hRepository,
    PWSTR            pwszShareName,
    PSRV_SHARE_INFO* ppShareInfo
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
SrvShareRegAdd(
    IN HANDLE hRepository,
    IN PWSTR  pwszShareName,
    IN PWSTR  pwszPath,
    IN PWSTR  pwszComment,
    IN PBYTE  pSecDesc,
    IN ULONG  ulSecDescLen,
    IN PWSTR  pwszService
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
SrvShareRegBeginEnum(
    HANDLE  hRepository,
    ULONG   ulLimit,
    PHANDLE phResume
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
SrvShareRegEnum(
    HANDLE            hRepository,
    HANDLE            hResume,
    PSRV_SHARE_INFO** pppShareInfoList,
    PULONG            pulNumSharesFound
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
SrvShareRegEndEnum(
    IN HANDLE hRepository,
    IN HANDLE hResume
    )
{
    // TODO

    return STATUS_SUCCESS;
}

NTSTATUS
SrvShareRegDelete(
    IN HANDLE hRepository,
    IN PWSTR  pwszShareName
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
SrvShareRegGetCount(
    IN     HANDLE  hRepository,
    IN OUT PULONG  pulNumShares
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

VOID
SrvShareRegClose(
    IN HANDLE hRepository
    )
{
    // TODO
}

VOID
SrvShareRegShutdown(
    VOID
    )
{
    // TODO
}
