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

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lw/ntstatus.h>
#include <lwrpc/allocate.h>
#include <lwrpc/sidhelper.h>


#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

#define CT_PTR_ADD(Pointer, Offset) \
    ((char*)(Pointer) + Offset)

#define CT_FIELD_OFFSET(Type, Field) \
    ((uint32_t)(&(((Type*)(0))->Field)))

#define CT_FIELD_SIZE(Type, Field) \
    (sizeof(((Type*)(0))->Field))

#define CT_FIELD_RECORD(Pointer, Type, Field) \
    ((Type*)CT_PTR_ADD(Pointer, -((ssize_t)CT_FIELD_OFFSET(Type, Field))))


size_t
SidGetRequiredSize(
    IN uint8 SubAuthorityCount
    )
{
    return CT_FIELD_OFFSET(SID, subauth) + CT_FIELD_SIZE(SID, subauth[0]) * SubAuthorityCount;
}

size_t
SidGetSize(
    IN PSID Sid
    )
{
    return SidGetRequiredSize(Sid->subauth_count);
}

void
SidFree(
    IN PSID Sid
    )
{
    free(Sid);
}

void
SidCopyPartial(
    OUT PSID Sid,
    IN size_t Size,
    IN PSID SourceSid
    )
{
    size_t sourceSize;
    if (SourceSid == NULL || Sid == NULL) return;
    
    sourceSize = SidGetSize(SourceSid);
    memcpy(Sid, SourceSid, MIN(Size, sourceSize));
}


void
SidCopyAlloc(
    OUT PSID *DstSid,
    IN SID *SrcSid
    )
{
    if (DstSid == NULL || SrcSid == NULL) return;

    size_t src_size = SidGetSize(SrcSid);
    *DstSid = (SID*) malloc(src_size);
    if (*DstSid) {
        memcpy(*DstSid, SrcSid, src_size);
    }
}


void
SidCopy(
    OUT SID *DstSid,
    IN SID *SrcSid
    )
{
    if (DstSid == NULL || SrcSid == NULL) return;

    size_t src_size = SidGetSize(SrcSid);
    memcpy(DstSid, SrcSid, src_size);
}

NTSTATUS
SidAllocate(
    OUT PSID* Sid,
    IN uint8 SubAuthorityCount
    )
{
    return SidAllocateResizedCopy(Sid, SubAuthorityCount, NULL);
}

NTSTATUS
SidAllocateResizedCopy(
    OUT PSID* Sid,
    IN uint8 SubAuthorityCount,
    IN PSID SourceSid
    )
{
    size_t size = SidGetRequiredSize(SubAuthorityCount);
    PSID sid = malloc(size);

    if (!sid)
    {
        *Sid = NULL;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    SidCopyPartial(sid, size, SourceSid);
    sid->subauth_count = SubAuthorityCount;

    *Sid = sid;

    return STATUS_SUCCESS;
}


#define FORWARD(position, str, len, sep) \
    while ((position - str < len) && \
	   *(position++) != sep)

NTSTATUS
ParseSidString(
    OUT PSID* Sid,
    IN const char *SidStr
    )
{
    const char *sid_start = "S-";
    const char separator = '-';
    size_t sidstr_len;
    char *start, *pos;
    uint8 count, fields, i;
    unsigned int authid;
    SID *sid;
    NTSTATUS status = STATUS_SUCCESS;

    if (SidStr == NULL || Sid == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    /* start of the actual sid string */
    sidstr_len = strlen(SidStr);
    if (sidstr_len == 0) return STATUS_INVALID_PARAMETER;
    
    start = strstr(SidStr, sid_start);
    if (start == NULL) return STATUS_INVALID_SID;

    /* count the number of fields */
    pos    = start;
    fields = 0;
    while ((pos - start) < sidstr_len) {
	if ((*pos) == separator) fields++;
	pos++;
    }

    /* first two dashes separate fixed fields
       (S-1-5-21-123456-123456-123456-500) */
    fields -= 2;

    /* revision number */
    FORWARD(start, SidStr, sidstr_len, separator);

    sid = (SID*) malloc(SidGetRequiredSize(fields));
    if (sid == NULL) return STATUS_NO_MEMORY;
    
    sid->revision = (uint8) atol(start);
    if (sid->revision != 1) {
	status = STATUS_INVALID_SID;
	goto error;
    }

    /* auth id */
    FORWARD(start, SidStr, sidstr_len, separator);

    authid = strtoul(start, &start, 10);
    memset(&sid->authid, 0, sizeof(sid->authid));

    /* decimal representation of authid is apparently 32-bit number */
    for (i = 0; i < sizeof(uint32); i++) {
	    unsigned long mask = 0xff << (i * 8);
	    sid->authid[sizeof(sid->authid) - (i + 1)] = (authid & mask) >> (i * 8);
    }

    FORWARD(start, SidStr, sidstr_len, separator);
    count = 0;

    while ((start - SidStr < sidstr_len) &&
	   *start != 0) {

	sid->subauth[count++] = (uint32) strtoul(start, NULL, 10);
	FORWARD(start, SidStr, sidstr_len, separator);
    }

    sid->subauth_count = count;
    *Sid = sid;
    return status;

error:
    SAFE_FREE(sid);
    *Sid = NULL;
    return status;
}

NTSTATUS
SidToString(
    IN PSID sid,
    OUT wchar16_t** sid_str
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    wchar16_t* result = NULL;
    char  sidPrefix[64];
    char* sidSuffix = NULL;
    size_t mem_allocated = 0; 
    size_t mem_available = 0; 
    size_t cur_offset = 0;
    int   iSubAuth = 0;

    if (sid->authid[0] || sid->authid[1])
    {
        sprintf(sidPrefix, "S-%u-0x%.2X%.2X%.2X%.2X%.2X%.2X",
                sid->revision,
                sid->authid[0],
                sid->authid[1],
                sid->authid[2],
                sid->authid[3],
                sid->authid[4],
                sid->authid[5]);
    }
    else
    {
        uint32 authid = 
            ((uint32)sid->authid[2] << 24) | 
            ((uint32)sid->authid[3] << 16) |
            ((uint32)sid->authid[4] << 8)  |
            ((uint32)sid->authid[5] << 0);
        
        sprintf(sidPrefix, "S-%u-%lu", sid->revision, (unsigned long) authid);
    }

    for (iSubAuth = 0; iSubAuth < sid->subauth_count; iSubAuth++)
    {
        char  sidPart[64];
        size_t len = 0;

        sprintf(sidPart, "-%u", sid->subauth[iSubAuth]); 

        len = strlen(sidPart);

        if (mem_available < (len+1)) {
           size_t mem_increment = 64;
           char* newSuffix = NULL;

           newSuffix = realloc(sidSuffix, mem_allocated + mem_increment);

           if (newSuffix == NULL) {
               status = STATUS_NO_MEMORY;
               goto error;
           }

           sidSuffix = newSuffix;
           mem_allocated += mem_increment;
           mem_available += mem_increment;

           memset(sidSuffix + cur_offset, 0, mem_available);
        }

        memcpy(sidSuffix + cur_offset, sidPart, len);
        cur_offset += len;
        mem_available -= len;
    }

    result = (wchar16_t*)malloc(
                     sizeof(wchar16_t) * (strlen(sidPrefix) +
                            (sidSuffix ? strlen(sidSuffix) : 0) + 1));
    if (result == NULL) {
        status = STATUS_NO_MEMORY;
        goto error;
    }

    sw16printf(result, "%s%s", sidPrefix, (sidSuffix ? sidSuffix : ""));

    *sid_str = result;

cleanup:

    SAFE_FREE(sidSuffix);

    return status;

error:

    if (result) {
       SAFE_FREE(result);
    }

    if (sid_str) {
        *sid_str = NULL;
    }

    goto cleanup;    
}


void
SidFreeString(
    IN wchar16_t* sid_str
    )
{
    free(sid_str);
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
