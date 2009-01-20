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


typedef struct _SID_IDENTIFIER_AUTHORITY {
    uint8 Octet[6];
} SID_IDENTIFIER_AUTHORITY;

#define SECURITY_NT_AUTHORITY          ((SID_IDENTIFIER_AUTHORITY) {{ 0x0, 0x0, 0x0, 0x0, 0x0, 0x5 }})
#define SECURITY_CREATOR_SID_AUTHORITY ((SID_IDENTIFIER_AUTHORITY) {{ 0x0, 0x0, 0x0, 0x0, 0x0, 0x3 }})

#define SID_BUILTIN_DOMAIN    "S-1-5-32"


NTSTATUS
RtlSidInitialize(
    IN OUT SID *pSid,
    IN SID_IDENTIFIER_AUTHORITY *pAuthority,
    IN UINT8 SubAuthCount
    );


DWORD
GetSidLengthRequired(
    IN UINT8 SubAuthCount
    );


size_t
SidGetSize(
    IN const SID *pSid
    );


size_t
SidGetRequiredSize(
    IN UINT8 SubAuthorityCount
    );


UINT8
SidGetSubAuthorityCount(
    IN const SID *pSid
    );


DWORD
SidGetSubAuthority(
    IN SID *pSid,
    IN UINT8 SubAuthorityIndex
    );


NTSTATUS
RtlSidAllocate(
    OUT PSID* ppSid,
    IN UINT8 SubAuthCount
    );


void
SidFree(
    IN SID *pSid
    );


void*
FreeSid(
    IN SID *pSid
    );


UINT8*
GetSidSubAuthorityCount(
    IN SID *pSid
    );


DWORD*
GetSidSubAuthority(
    IN SID *pSid,
    IN DWORD SubAuthority
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
    IN const SID *pSourceSid
    );


void
SidCopy(
    OUT SID *pDstSid,
    IN const SID *pSrcSid
    );


NTSTATUS
RtlSidCopyPartial(
    SID *pSid,
    DWORD Size,
    const SID *pSourceSid
    );


NTSTATUS
RtlSidCopyAlloc(
    OUT PSID *ppDstSid,
    IN const SID *pSrcSid
    );


NTSTATUS
RtlSidAppendRid(
    OUT PSID *ppDstSid,
    IN DWORD dwRid,
    IN const SID *pSrcSid
    );


BOOL
IsEqualSid(
    SID *pS1,
    SID *pS2
    );


NTSTATUS
ParseSidStringA(
    OUT PSID *ppSid,
    IN PCSTR pszSidStr
    );

NTSTATUS
ParseSidStringW(
    OUT PSID *ppSid,
    IN PCWSTR pwszSidStr
    );


NTSTATUS
SidToStringA(
    IN SID *pSid,
    OUT PSTR *ppszSidStr
    );


void
SidStrFreeA(
    IN PSTR pszSidStr
    );


NTSTATUS
SidToStringW(
    IN SID *pSid,
    OUT PWSTR *ppwszSidStr
    );


void
SidStrFreeW(
    IN PWSTR pwszSidStr
    );


/*
  MS compatibility functions
*/

BOOL
InitializeSid(
    IN OUT SID *pSid,
    IN SID_IDENTIFIER_AUTHORITY *pAuthority,
    IN UINT8 SubAuthCount
    );


DWORD
GetLengthSid(
    IN SID *pSid
    );


BOOL
IsValidSid(
    IN SID *pSid
    );


BOOL
AllocateAndInitializeSid(
    IN SID_IDENTIFIER_AUTHORITY *Authority,
    IN UINT8 SubAuthorityCound,
    IN DWORD dwSubAuthority0,
    IN DWORD dwSubAuthority1,
    IN DWORD dwSubAuthority2,
    IN DWORD dwSubAuthority3,
    IN DWORD dwSubAuthority4,
    IN DWORD dwSubAuthority5,
    IN DWORD dwSubAuthority6,
    IN DWORD dwSubAuthority7,
    OUT PSID *ppSid
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
