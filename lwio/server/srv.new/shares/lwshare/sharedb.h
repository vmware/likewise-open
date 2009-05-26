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
 *        sharedb.h
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Share Repository based on sqlite
 *
 *        Share Management
 *
 * Authors: Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */

#ifndef __SRV_SHAREDB_H__
#define __SRV_SHAREDB_H__

NTSTATUS
SrvShareDbInit(
    VOID
    );

NTSTATUS
SrvShareDbOpen(
    OUT PHANDLE phRepository
    );

NTSTATUS
SrvShareDbFindByName(
	IN  HANDLE       hRepository,
	IN  PWSTR        pwszShareName,
	OUT PSHARE_INFO* ppShareInfo
	);

NTSTATUS
SrvShareDbAdd(
	IN  HANDLE hRepository,
	IN  PWSTR  pwszShareName,
	IN  PWSTR  pwszPath,
	IN  PWSTR  pwszComment,
	IN  PBYTE  pSecDesc,
	IN  ULONG  ulSecDescLen,
	IN  PWSTR  pwszService
	);

NTSTATUS
SrvShareDbBeginEnum(
	IN  HANDLE  hRepository,
	IN  ULONG   ulLimit,
	OUT PHANDLE phResume
	);

NTSTATUS
SrvShareDbEnum(
	IN     HANDLE           hRepository,
	IN     HANDLE           hResume,
	OUT    PSHARE_DB_INFO** pppShareInfoList,
	IN OUT PULONG           pulNumSharesFound
	);

NTSTATUS
SrvShareDbEndEnum(
	IN HANDLE           hRepository,
	IN HANDLE           hResume
	);

NTSTATUS
SrvShareDbDelete(
	IN HANDLE hRepository,
	IN PWSTR  pwszShareName
	);

NTSTATUS
SrvShareDbGetCount(
	IN     HANDLE  hRepository,
    IN OUT PULONG  pulNumShares
    );

VOID
SrvShareDbClose(
	IN HANDLE hRepository
	);

VOID
SrvShareDbShutdown(
    VOID
    );

#endif /* __SRV_SHAREDB_H__ */

