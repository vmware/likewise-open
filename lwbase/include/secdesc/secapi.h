/* Editor Settings: expandtabs and use 4 spaces for indentation
* ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
*/

/*
 * Copyright Likewise Software    2004-2009
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
 * Authors: Rafal Szczesniak (rafal@likewisesoftware.com)
 */

#ifndef __SECAPI_H_
#define __SECAPI_H_

#include <secdesc/sectypes.h>
#include <lw/attrs.h>
#include <lw/ntstatus.h>

//
// Policy Handle API
//

NTSTATUS
RtlPolHndCreate(
    OUT PolicyHandle* pH,
    IN ULONG HandleType
    );

NTSTATUS
RtlPolHndCopy(
    OUT PolicyHandle* pHout,
    IN PolicyHandle* pHin
    );

NTSTATUS
RtlPolHndAllocate(
    OUT PolicyHandle** ppH,
    IN ULONG HandleType
    );

VOID
RtlPolHndFree(
    IN OUT PolicyHandle* pH
    );

BOOLEAN
RtlPolHndIsEqual(
    IN PolicyHandle* pH1,
    IN PolicyHandle* pH2
    );

BOOLEAN
RtlPolHndIsEmpty(
    IN PolicyHandle* pH
    );

//
// SID-Related API
//

// TODO-Perhpas use non-string version instead?
#define SID_BUILTIN_DOMAIN  "S-1-5-32"

#if 0
NTSTATUS
RtlSidInitialize(
    IN OUT PSID pSid,
    IN PSID_IDENTIFIER_AUTHORITY pAuthority,
    IN UINT8 SubAuthorityCount
    );

ULONG
GetSidLengthRequired(
    IN UINT8 SubAuthorityCount
    );
#endif

size_t
SidGetSize(
    IN PSID pSid
    );

size_t
SidGetRequiredSize(
    IN UINT8 SubAuthorityCount
    );

#if 0
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
#endif

void
SidFree(
    IN OUT PSID pSid
    );

#if 0
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
#endif

NTSTATUS
RtlSidAllocateResizedCopy(
    OUT PSID* ppSid,
    IN UINT8 SubAuthorityCount,
    IN PSID pSourceSid
    );

#if 0
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
#endif

NTSTATUS
RtlSidCopyAlloc(
    OUT PSID* ppDstSid,
    IN PSID pSrcSid
    );

#if 0
NTSTATUS
RtlSidAppendRid(
    OUT PSID* ppDstSid,
    IN DWORD dwRid,
    IN PSID pSrcSid
    );

BOOLEAN
IsEqualSid(
    IN PSID pS1,
    IN PSID pS2
    );
#endif

NTSTATUS
ParseSidStringA(
    OUT PSID* ppSid,
    IN PCSTR pszSidStr
    );

#if 0
NTSTATUS
ParseSidStringW(
    OUT PSID* ppSid,
    IN PCWSTR pwszSidStr
    );
#endif

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

#if 0
//
// MS compatibility functions
//

BOOL
InitializeSid(
    IN OUT PSID pSid,
    IN PSID_IDENTIFIER_AUTHORITY pAuthority,
    IN UINT8 SubAuthorityCount
    );

ULONG
GetLengthSid(
    IN PSID pSid
    );

BOOLEAN
IsValidSid(
    IN PSID pSid
    );

NTSTATUS
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
#endif

#endif

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
