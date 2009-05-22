/*
 * Copyright Likewise Software    2004-2009
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
 *        shareapi.h
 *
 * Abstract:
 *
 *        Likewise Input Output (LWIO) - SRV
 *
 *        Share API
 *
 * Authors: Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */

#ifndef __SHAREAPI_H__
#define __SHAREAPI_H__

#include <shares/defs.h>
#include <shares/structs.h>

NTSTATUS
SrvShareInit(
	VOID
	);

NTSTATUS
SrvShareInitList(
    IN OUT PLWIO_SRV_SHARE_ENTRY_LIST pShareList
    );

NTSTATUS
SrvShareFindByName(
    IN  PLWIO_SRV_SHARE_ENTRY_LIST pShareList,
    IN  PWSTR                      pszShareName,
    OUT PSHARE_DB_INFO*            ppShareInfo
    );

NTSTATUS
SrvShareAdd(
    IN OUT PLWIO_SRV_SHARE_ENTRY_LIST pShareList,
    IN     PWSTR                      pwszShareName,
    IN     PWSTR                      pwszPath,
    IN     PWSTR                      pwszComment,
    IN     PBYTE                      pSecDesc,
    IN     ULONG                      ulSecDescLen,
    IN     ULONG                      ulShareType
    );

NTSTATUS
SrvShareDeleteShare(
    IN OUT PLWIO_SRV_SHARE_ENTRY_LIST pShareList,
    IN     PWSTR                      pwszShareName
    );

NTSTATUS
SrvShareSetInfo(
	IN OUT PLWIO_SRV_SHARE_ENTRY_LIST pShareList,
    IN     PWSTR                      pwszShareName,
    IN     PSHARE_DB_INFO             pShareInfo
    );

NTSTATUS
SrvShareGetInfo(
	IN  PLWIO_SRV_SHARE_ENTRY_LIST pShareList,
    IN  PWSTR                      pwszShareName,
    OUT PSHARE_DB_INFO*            ppShareInfo
    );

NTSTATUS
SrvShareEnumShares(
	IN  PLWIO_SRV_SHARE_ENTRY_LIST pShareList,
    IN  ULONG                      ulLevel,
    OUT PSHARE_DB_INFO**           pppShareInfo,
    OUT PDWORD                     pdwNumEntries
    );

VOID
SrvShareFreeListContents(
    IN OUT PLWIO_SRV_SHARE_ENTRY_LIST pShareList
    );

VOID
SrvShareShutdown(
    VOID
    );

#endif /* __SHAREAPI_H__ */
