/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        samr_memory.h
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        Samr rpc memory management functions
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#ifndef _SAMR_MEMORY_H_
#define _SAMR_MEMORY_H_


NTSTATUS
SamrAllocateMemory(
    OUT PVOID  *ppOut,
    IN  size_t  Size,
    IN  PVOID   pDep
    );


NTSTATUS
SamrAddDepMemory(
    IN  PVOID pPtr,
    IN  PVOID pDep
    );


NTSTATUS
SamrAllocateNamesAndRids(
    OUT PWSTR        **pppNames,
    OUT UINT32       **ppRids,
    IN  RidNameArray  *pIn
    );


NTSTATUS
SamrAllocateNames(
    OUT PWSTR **pppwszNames,
    IN  EntryArray *pNamesArray
    );


NTSTATUS
SamrAllocateIds(
    OUT UINT32 **ppOutIds,
    IN  Ids *pInIds
    );


NTSTATUS
SamrAllocateDomSid(
    OUT PSID *ppOut,
    IN  PSID  pIn,
    IN  PVOID pDep
    );


NTSTATUS
SamrAllocateSids(
    OUT PSID     **pppSids,
    IN  SidArray  *pSidArray
    );


NTSTATUS
SamrAllocateRidsAndAttributes(
    OUT UINT32                **ppRids,
    OUT UINT32                **ppAttributes,
    IN  RidWithAttributeArray  *pIn
    );


NTSTATUS
SamrAllocateAliasInfo(
    OUT AliasInfo **ppOut,
    IN  AliasInfo  *pIn,
    IN  UINT16      Level
    );


NTSTATUS
SamrAllocateDomainInfo(
    OUT DomainInfo **ppOut,
    IN  DomainInfo *pIn,
    IN  UINT16 Level
    );


NTSTATUS
SamrAllocateUserInfo(
    OUT UserInfo **ppOut,
    IN  UserInfo *pIn,
    IN  UINT16    Level
    );


NTSTATUS
SamrAllocateDisplayInfo(
    OUT SamrDisplayInfo **ppOut,
    IN  SamrDisplayInfo  *pIn,
    IN  UINT16            Level
    );


#endif /* _SAMR_MEMORY_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
