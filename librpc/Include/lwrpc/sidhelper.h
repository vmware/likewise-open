/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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

#ifndef __SIDHELPER_H__
#define __SIDHELPER_H__

#include <lwrpc/types.h>
#include <lwrpc/security.h>
#include <sys/types.h>

#define IN
#define OUT

#if 0
typedef struct dom_sid {
        uint8 revision;
        [range(0,15)] uint8 subauth_count;
        uint8 authid[6];
        [size_is(subauth_count)] uint32 subauth[];
} DomSid;
#endif

typedef struct _SID_IDENTIFIER_AUTHORITY {
    uint8 Octet[6];
} SID_IDENTIFIER_AUTHORITY;

size_t
SidGetRequiredSize(
    IN uint8 SubAuthorityCount
    );

size_t
SidGetSize(
    IN PSID Sid
    );

void
SidFree(
    IN PSID Sid
    );

NTSTATUS
SidAllocate(
    OUT PSID* Sid,
    IN uint8 SubAuthityCount
    );

uint8
SidGetSubAuthorityCount(
    IN PSID Sid
    );

uint32
SidGetSubAuthority(
    IN PSID Sid,
    IN uint8 SubAuthorityIndex
    );

NTSTATUS
SidAllocateAndInitialze(
    OUT PSID* Sid,
    IN SID_IDENTIFIER_AUTHORITY Authority,
    IN uint8 SubAuthorityCount,
    ...
    );

/**
 * Allocate and copy a SID
 *
 * @param[out] Sid - Returns allocated SID.
 * @param[in] SubAuthorityCount - sub auth count to use in new SID.
 * @param[in] SourceSid - source SID to copy.
 */
NTSTATUS
SidAllocateResizedCopy(
    OUT PSID* Sid,
    IN uint8 SubAuthorityCount,
    IN PSID SourceSid
    );

NTSTATUS
SidCopyEx(
    OUT PSID Target,
    IN size_t Size,
    IN PSID Source
    );

void
SidCopy(
    OUT SID *DstSid,
    IN SID *SrcSid
    );

void
SidCopyAlloc(
    OUT PSID *DstSid,
    IN SID *SrcSid
    );

#if 0
typedef struct _STATIC_SID {
    SID Header;
    uint32 SubAuth[MAXIMUM_SUBAUTHTORITY_COUNT];
} STATIC_SID;
#endif

/**
 * Parse SID given string representation
 * @param[out] Sid - allocated sid
 * @param[in] SidStr - string sid representation
 */
NTSTATUS
ParseSidString(
    OUT PSID* Sid,
    IN const char *SidStr
    );

/**
 * Get string representation of SID
 * @param[in] Sid - sid
 * @param[out] SidStr - string sid representation
 */
NTSTATUS
SidToString(
    IN PSID Sid,
    OUT wchar16_t** SidStr
    );

/**
 * Free string returned from SidToString function
 * @param[in] SidStr - string sid representation
 */
void
SidFreeString(
    IN wchar16_t* SidStr
    );


#define SID_BUILTIN_DOMAIN    "S-1-5-32"


#endif /* __SIDHELPER_H__ */
