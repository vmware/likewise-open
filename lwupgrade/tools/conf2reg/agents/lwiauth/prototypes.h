/*
 * Copyright (c) Likewise Software.  All rights reserved.
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
 * license@likewise.com
 */

DWORD
LwiTdbOpen(
    PCSTR pszPath,
    PTDB_CONTEXT *ppTdb
    );

VOID
LwiTdbClose(
    PTDB_CONTEXT pTdb
    );

DWORD
LwiTdbAllocateFetch(
    PTDB_CONTEXT pTdb,
    PCSTR pszKey,
    PVOID *ppResult,
    PDWORD pdwSize
    );

DWORD
LwiGetMachineInformationA(
    PLWISERVERINFO pConfig,
    PCSTR pszSecretsPath,
    PLWPS_PASSWORD_INFOA pPasswordInfo
    );

VOID
LwiFreeMachineInformationContentsA(
    PLWPS_PASSWORD_INFOA pInfo
    );

DWORD
LwiAllocateMachineInformationContentsW(
    PLWPS_PASSWORD_INFOA pInfo,
    PLWPS_PASSWORD_INFO pPasswordInfo
    );

VOID
LwiFreeMachineInformationContentsW(
    PLWPS_PASSWORD_INFO pInfo
    );

DWORD
MovePassword(
    PSTR pszDomainName
    );

