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
 * Authors: Danilo Almeida (dalmeida@likewisesoftware.com)
 *          Rafal Szczesniak (rafal@likewisesoftware.com)
 */


#ifndef _SID_H_
#define _SID_H_

#include "sdsys.h"
#include "siddef.h"
#include <lw/attrs.h>

#define SID_BUILTIN_DOMAIN    "S-1-5-32"


NTSTATUS
RtlSidInitialize(
    IN OUT PSID pSid,
    IN PSID_IDENTIFIER_AUTHORITY pAuthority,
    IN UINT8 SubAuthorityCount
    );

DWORD
GetSidLengthRequired(
    IN UINT8 SubAuthorityCount
    );

size_t
SidGetSize(
    IN PSID pSid
    );

size_t
SidGetRequiredSize(
    IN UINT8 SubAuthorityCount
    );

UINT8
SidGetSubAuthorityCount(
    IN PSID pSid
    );

DWORD
SidGetSubAuthority(
    IN PSID pSid,
    IN UINT8 SubAuthorityIndex
    );

NTSTATUS
RtlSidAllocate(
    OUT PSID* ppSid,
    IN UINT8 SubAuthorityCount
    );

void
SidFree(
    IN OUT PSID pSid
    );

UINT8*
GetSidSubAuthorityCount(
    IN PSID pSid
    );

NTSTATUS
RtlSidAllocateAndInitialize(
    OUT PSID* ppSid,
    IN SID_IDENTIFIER_AUTHORITY Authority,
    IN UINT8 SubAuthorityCount,
    ...
    );

NTSTATUS
RtlSidAllocateResizedCopy(
    OUT PSID* ppSid,
    IN UINT8 SubAuthorityCount,
    IN PSID pSourceSid
    );

void
SidCopy(
    OUT PSID pDstSid,
    IN PSID pSrcSid
    );

NTSTATUS
RtlSidCopyPartial(
    OUT PSID pSid,
    IN DWORD Size,
    IN PSID pSourceSid
    );

NTSTATUS
RtlSidCopyAlloc(
    OUT PSID* ppDstSid,
    IN PSID pSrcSid
    );

NTSTATUS
RtlSidAppendRid(
    OUT PSID* ppDstSid,
    IN DWORD dwRid,
    IN PSID pSrcSid
    );

BOOL
IsEqualSid(
    IN PSID pS1,
    IN PSID pS2
    );

NTSTATUS
ParseSidStringA(
    OUT PSID* ppSid,
    IN PCSTR pszSidStr
    );

NTSTATUS
ParseSidStringW(
    OUT PSID* ppSid,
    IN PCWSTR pwszSidStr
    );

NTSTATUS
SidToStringA(
    IN PSID pSid,
    OUT PSTR* ppszSidStr
    );

void
SidStrFreeA(
    IN OUT PSTR pszSidStr
    );

NTSTATUS
SidToStringW(
    IN PSID pSid,
    OUT PWSTR* ppwszSidStr
    );

void
SidStrFreeW(
    IN OUT PWSTR pwszSidStr
    );


/*
  MS compatibility functions
*/ 

BOOL
InitializeSid(
    IN OUT PSID pSid,
    IN PSID_IDENTIFIER_AUTHORITY pAuthority,
    IN UINT8 SubAuthorityCount
    );

DWORD
GetLengthSid(
    IN PSID pSid
    );

BOOL
IsValidSid(
    IN PSID pSid
    );

BOOL
AllocateAndInitializeSid(
    IN PSID_IDENTIFIER_AUTHORITY pAuthority,
    IN UINT8 SubAuthorityCount,
    IN DWORD dwSubAuthority0,
    IN DWORD dwSubAuthority1,
    IN DWORD dwSubAuthority2,
    IN DWORD dwSubAuthority3,
    IN DWORD dwSubAuthority4,
    IN DWORD dwSubAuthority5,
    IN DWORD dwSubAuthority6,
    IN DWORD dwSubAuthority7,
    OUT PSID* ppSid
    );


#if defined(UNICODE)
#define RtlParseSidString    RtlParseSidStringW
#define RtlSidToString       RtlSidToStringW
#define SidStrFree           SidStrFreeW
#else
#define RtlParseSidString    RtlParseSidStringA
#define RtlSidToString       RtlSidToStringA
#define SidStrFree           SidStrFreeA
#endif /* defined(UNICODE) */


#endif /* _SID_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
