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
 *        Likewise IO (Srv) (LWIO-SRV)
 *
 *        Local Authentication Provider
 *
 *        Share Database Interface
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __SRVDB_H__
#define __SRVDB_H__

NTSTATUS
SrvShareDbInit(
    PLWIO_SRV_SHARE_DB_CONTEXT pShareDBContext
    );

NTSTATUS
SrvShareDbOpen(
    PLWIO_SRV_SHARE_DB_CONTEXT pShareDBContext,
    PHANDLE phDb
    );

NTSTATUS
SrvShareDbFindByName(
    PLWIO_SRV_SHARE_DB_CONTEXT pShareDBContext,
    HANDLE hDb,
    PCSTR  pszShareName,
    PSHARE_DB_INFO* ppShareInfo
    );

NTSTATUS
SrvShareDbAdd(
    PLWIO_SRV_SHARE_DB_CONTEXT pShareDBContext,
    HANDLE hDb,
    PCSTR  pszShareName,
    PCSTR  pszPath,
    PCSTR  pszComment,
    PCSTR  pszSid,
    PCSTR  pszService
    );

NTSTATUS
SrvShareDbAdd_inlock(
    PLWIO_SRV_SHARE_DB_CONTEXT pShareDBContext,
    HANDLE hDb,
    PCSTR  pszShareName,
    PCSTR  pszPath,
    PCSTR  pszComment,
    PCSTR  pszSid,
    PCSTR  pszService
    );

NTSTATUS
SrvShareMapFromWindowsPath(
    PLWIO_SRV_SHARE_DB_CONTEXT pShareDBContext,
    PWSTR  pwszInputPath,
    PWSTR* ppwszPath
    );

NTSTATUS
SrvShareMapToWindowsPath(
    PLWIO_SRV_SHARE_DB_CONTEXT pShareDBContext,
    PWSTR  pwszInputPath,
    PWSTR* ppwszPath
    );

NTSTATUS
SrvShareDbEnum(
    PLWIO_SRV_SHARE_DB_CONTEXT pShareDBContext,
    HANDLE           hDb,
    ULONG            ulOffset,
    ULONG            ulLimit,
    PSHARE_DB_INFO** pppShareInfoArray,
    PULONG           pulNumSharesFound
    );

NTSTATUS
SrvShareDbEnum_inlock(
    PLWIO_SRV_SHARE_DB_CONTEXT pShareDBContext,
    HANDLE           hDb,
    ULONG            ulOffset,
    ULONG            ulLimit,
    PSHARE_DB_INFO** pppShareInfoArray,
    PULONG           pulNumSharesFound
    );

NTSTATUS
SrvShareDbDelete(
    PLWIO_SRV_SHARE_DB_CONTEXT pShareDBContext,
    HANDLE hDb,
    PCSTR  pszShareName
    );

NTSTATUS
SrvShareDbGetCount(
    PLWIO_SRV_SHARE_DB_CONTEXT pShareDBContext,
    HANDLE hDb,
    PULONG pulNumShares
    );

VOID
SrvShareDbFreeInfoList(
    PSHARE_DB_INFO* ppShareInfoList,
    ULONG           ulNumShares
    );

VOID
SrvShareDbReleaseInfo(
    PSHARE_DB_INFO pShareInfo
    );

VOID
SrvShareDbClose(
    PLWIO_SRV_SHARE_DB_CONTEXT pShareDBContext,
    HANDLE hDb
    );

VOID
SrvShareDbShutdown(
    PLWIO_SRV_SHARE_DB_CONTEXT pShareDBContext
    );

#endif /* __LSASSDB_H__ */

