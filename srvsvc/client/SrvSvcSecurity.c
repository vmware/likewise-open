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

#include "includes.h"


#define GET_UINT8(buf, ofs) ((uint8)( \
    ((((const uint8 *)(buf))[(ofs)+0]) <<  0 ) | \
    0))

#define GET_UINT16(buf, ofs) ((uint16)( \
    ((((const uint8 *)(buf))[(ofs)+0]) <<  0 ) | \
    ((((const uint8 *)(buf))[(ofs)+1]) <<  8 ) | \
    0))

#define GET_UINT32(buf, ofs) ((uint32)( \
    ((((const uint8 *)(buf))[(ofs)+0]) <<  0 ) | \
    ((((const uint8 *)(buf))[(ofs)+1]) <<  8 ) | \
    ((((const uint8 *)(buf))[(ofs)+2]) << 16 ) | \
    ((((const uint8 *)(buf))[(ofs)+3]) << 24 ) | \
    0))

#define PUT_UINT8(buf, ofs, val) do { \
    ((uint8 *)(buf))[(ofs)+0] = ((uint8)((((uint8)(val)) >>  0) & 0xFF)); \
} while (0);

#define PUT_UINT16(buf, ofs, val) do { \
    ((uint8 *)(buf))[(ofs)+0] = ((uint8)((((uint16)(val)) >>  0) & 0xFF)); \
    ((uint8 *)(buf))[(ofs)+1] = ((uint8)((((uint16)(val)) >>  8) & 0xFF)); \
} while (0);

#define PUT_UINT32(buf, ofs, val) do { \
    ((uint8 *)(buf))[(ofs)+0] = ((uint8)((((uint32)(val)) >>  0) & 0xFF)); \
    ((uint8 *)(buf))[(ofs)+1] = ((uint8)((((uint32)(val)) >>  8) & 0xFF)); \
    ((uint8 *)(buf))[(ofs)+2] = ((uint8)((((uint32)(val)) >> 16) & 0xFF)); \
    ((uint8 *)(buf))[(ofs)+3] = ((uint8)((((uint32)(val)) >> 24) & 0xFF)); \
} while (0);

static NET_API_STATUS
DecodeDomSid(
    const uint8 *buf,
    uint32 buflen,
    uint32 start_ofs,
    DomSid *r,
    uint32 *r_size,
    void *(*allocfn)(void *allocpv, size_t len),
    void *allocpv
    )
{
    uint32 ofs = start_ofs;
    uint32 i;

    if (ofs >= buflen) {
        return ERROR_INVALID_SID;
    }

    if ((ofs + 8) > buflen) {
        return ERROR_INVALID_SID;
    }

    r->revision = GET_UINT8(buf, ofs);
    ofs += 1;

    if (r->revision != 1) {
        return ERROR_INVALID_SID;
    }

    r->subauth_count = GET_UINT8(buf, ofs);
    ofs += 1;
    if (r->subauth_count > 15) {
        return ERROR_INVALID_SID;
    }

    memcpy(r->authid, &buf[ofs], 6);
    ofs += 6;

    r->subauth = (uint32 *)allocfn(allocpv, sizeof(uint32) * r->subauth_count);
    if (!r->subauth) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    for (i = 0; i < r->subauth_count; i++) {
            if ((ofs + 4) > buflen) {
                return ERROR_INVALID_SUB_AUTHORITY;
            }

            r->subauth[i] = GET_UINT32(buf, ofs);
            ofs += 4;
    }

    if (r_size) {
        *r_size = ofs - start_ofs;
    }

    return ERROR_SUCCESS;
}

static NET_API_STATUS
DecodeSecAce(
    const uint8 *buf,
    uint32 buflen,
    uint32 start_ofs,
    SecAce *r,
    uint32 *r_size,
    void *(*allocfn)(void *allocpv, size_t len),
    void *allocpv
    )
{
    NET_API_STATUS ret = ERROR_SUCCESS;
    uint32 ofs = start_ofs;
    uint32 size;

    if (ofs >= buflen) {
        return ERROR_INVALID_ACL;
    }

    if ((ofs + 8) > buflen) {
        return ERROR_INVALID_ACL;
    }

    r->sec_ace_type = GET_UINT8(buf, ofs);
    ofs += 1;

    r->sec_ace_flags = GET_UINT8(buf, ofs);
    ofs += 1;

    r->size = GET_UINT16(buf, ofs);
    ofs += 2;

    r->access_mask = GET_UINT32(buf, ofs);
    ofs += 4;

    /* TODO: parse r->object for AD style ACEs*/

    ret = DecodeDomSid(buf, buflen, ofs, r->trustee, &size, allocfn, allocpv);
    if (ret != ERROR_SUCCESS) {
        return ret;
    }
    ofs += size;

    /*
     * sometimes ACEs contains padding at the end
     * we need to make sure we return the ace size
     * including this padding to the caller.
     */
    size = ofs - start_ofs;
    if (r->size < size) {
        return ERROR_INVALID_ACL;
    }
    if ((ofs + r->size) > buflen) {
        return ERROR_INVALID_ACL;
    }

    if (r_size) {
        *r_size = r->size;
    }

    return ERROR_SUCCESS;
}

static NET_API_STATUS
DecodeSecAcl(
    const uint8 *buf,
    uint32 buflen,
    uint32 start_ofs,
    SecAcl *r,
    uint32 *r_size,
    void *(*allocfn)(void *allocpv, size_t len),
    void *allocpv
    )
{
    uint32 ofs = start_ofs;
    uint32 i;

    if (ofs >= buflen) {
        return ERROR_INVALID_ACL;
    }

    if ((ofs + 8) > buflen) {
        return ERROR_INVALID_ACL;
    }

    r->revision = GET_UINT16(buf, ofs);
    ofs += 2;

    if (r->revision != 2) {
        return ERROR_INVALID_ACL;
    }

    r->size = GET_UINT16(buf, ofs);
    ofs += 2;

    r->num_aces = GET_UINT32(buf, ofs);
    ofs += 4;
    if (r->num_aces > 1000) {
        return ERROR_INVALID_ACL;
    }

    r->aces = (SecAce *)allocfn(allocpv, sizeof(SecAce) * r->num_aces);
    if (!r->aces) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    for (i = 0; i < r->num_aces; i++) {
            NET_API_STATUS ret = ERROR_SUCCESS;
            uint32 size;

            ret = DecodeSecAce(buf, buflen, ofs, &r->aces[i], &size,
                               allocfn, allocpv);
            if (ret != ERROR_SUCCESS) {
                return ret;
            }
            ofs += size;
    }

    if (r_size) {
        *r_size = ofs - start_ofs;
    }

    return ERROR_SUCCESS;
}

NET_API_STATUS SecurityDescriptorFromBuffer(
    SecDesc **security_descriptor,
    const uint8 *buf,
    uint32 buflen,
    void *(*allocfn)(void *allocfn, size_t len),
    void *allocpv
    )
{
    NET_API_STATUS ret = ERROR_SUCCESS;
    SecDesc *sd;
    uint32 sd_size;
    uint32 ofs = 0;
    uint32 owner_ofs;
    uint32 group_ofs;
    uint32 sacl_ofs;
    uint32 dacl_ofs;

    *security_descriptor = NULL;

    if (!buf) {
        return ERROR_SUCCESS;
    }

    if ((ofs + 20) > buflen) {
        return ERROR_INVALID_SECURITY_DESCR;
    }

    sd = (SecDesc *)allocfn(allocpv, sizeof(SecDesc));
    if (!sd) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    sd->revision = GET_UINT8(buf, ofs);
    ofs += 1;

    if (sd->revision != 1) {
        return ERROR_INVALID_SECURITY_DESCR;
    }

    /* padding */
    ofs += 1;

    sd->type = GET_UINT16(buf, ofs);
    ofs += 2;

    if (!(sd->type & 0x8000)) {
        return ERROR_BAD_DESCRIPTOR_FORMAT;
    }

    /* mark as non self relative */
    sd->type &= ~0x8000;

    owner_ofs = GET_UINT32(buf, ofs);
    ofs += 4;

    group_ofs = GET_UINT32(buf, ofs);
    ofs += 4;

    sacl_ofs = GET_UINT32(buf, ofs);
    ofs += 4;

    dacl_ofs = GET_UINT32(buf, ofs);
    ofs += 4;

    if (owner_ofs) {
        sd->owner = (DomSid *)allocfn(allocpv,sizeof(DomSid));
        if (!sd->owner) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        ret = DecodeDomSid(buf, buflen, owner_ofs, sd->owner, NULL,
                           allocfn, allocpv);
        if (ret != ERROR_SUCCESS) {
            return ret;
        }
    } else {
        sd->owner = NULL;
    }

    if (group_ofs) {
        sd->group = (DomSid *)allocfn(allocpv, sizeof(DomSid));
        if (!sd->group) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        ret = DecodeDomSid(buf, buflen, group_ofs, sd->group, NULL,
                           allocfn, allocpv);
        if (ret != ERROR_SUCCESS) {
            return ret;
        }
    } else {
        sd->group = NULL;
    }

    if (sacl_ofs) {
        sd->sacl = (SecAcl *)allocfn(allocpv, sizeof(SecAcl));
        if (!sd->sacl) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        ret = DecodeSecAcl(buf, buflen, sacl_ofs, sd->sacl, NULL,
                           allocfn, allocpv);
        if (ret != ERROR_SUCCESS) {
            return ret;
        }
    } else {
        sd->sacl = NULL;
    }

    if (dacl_ofs) {
        sd->dacl = (SecAcl *)allocfn(allocpv, sizeof(SecAcl));
        if (!sd->dacl) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        ret = DecodeSecAcl(buf, buflen, dacl_ofs, sd->dacl, NULL,
                           allocfn, allocpv);
        if (ret != ERROR_SUCCESS) {
            return ret;
        }
    } else {
        sd->dacl = NULL;
    }

    *security_descriptor = (SecDesc*)sd;
    return ERROR_SUCCESS;
}

#if 0
static void DumpDomSid(
    const DomSid *r,
    const char *prefix
    )
{
    uint32 i;

    /* we only support NT4 ACLs yet */
    if (r->revision != 1) {
        printf("%srevision = 0x%02X\n",
               prefix, r->revision);
        return;
    }

    i = r->authid[2] >> 24;
    i = r->authid[3] >> 16;
    i = r->authid[4] >> 8;
    i = r->authid[5] >> 0;
    printf("%sS-%u-%lu",
           prefix, r->revision, i);

    for (i=0; i < r->subauth_count; i++) {
        printf("-%lu", r->subauth[i]);
    }

    printf("\n");
}

static void DumpSecAce(
    const SecAce *r,
    const char *prefix
    )
{
    const char *subprefix = "\t\t";

    printf("%ssec_ace_type = 0x%02X\n",
           prefix, r->sec_ace_type);

    printf("%ssec_ace_flags = 0x%02X\n",
           prefix, r->sec_ace_flags);

    printf("%ssize = 0x%04X\n",
           prefix, r->size);

    printf("%saccess_mask = 0x%08X\n",
           prefix, r->access_mask);

    printf("%strustee = %p\n",
           prefix, r->trustee);

    DumpDomSid(&r->trustee, subprefix);
}

static void DumpSecAcl(
    const SecAcl *r,
    const char *prefix
    )
{
    const char *subprefix = "\t\t";
    uint32 i;

    printf("%srevision = 0x%04X\n",
           prefix, r->revision);

    /* we only support NT4 ACLs yet */
    if (r->revision != 2) {
        return;
    }

    printf("%ssize = 0x%04X\n",
           prefix, r->size);

    printf("%snum_aces = 0x%08X\n",
           prefix, r->num_aces);

    for (i=0; i < r->num_aces; i++) {
        DumpSecAce(&r->aces[i], subprefix);
    }
}

void SecurityDescriptorDump(
    const PSECURITY_DESCRIPTOR security_descriptor
    )
{
    const SecDesc *sd = (const SecDesc *)security_descriptor;

    if (sd == NULL) {
        printf("NULL sd\n");
        return;
    }

    printf("\trevision = 0x%02X\n", sd->revision);

    if (sd->revision != 1) {
        return;
    }

    printf("\ttype = 0x%04X\n", sd->type);

    printf("\towner = %p\n", sd->owner);
    if (sd->owner) {
        DumpDomSid(sd->owner, "\t");
    }

    printf("\tgroup = %p\n", sd->group);
    if (sd->group) {
        DumpDomSid(sd->group, "\t");
    }

    printf("\tsacl = %p\n", sd->sacl);
    if (sd->sacl) {
        DumpSecAcl(sd->sacl, "\t");
    }

    printf("\tdacl = %p\n", sd->dacl);
    if (sd->dacl) {
        DumpSecAcl(sd->dacl, "\t");
    }
}
#endif

static uint32 DomSidGetSize(
    const DomSid *sid
    )
{
    uint32 size = 0;

    if (!sid) {
        return size;
    }

    size += 8;
    size += (sid->subauth_count * 4);

    return size;
}

static SecAceGetSize(
    const SecAce *ace
    )
{
    int size = 0;

    size += 8;
    size += 0;/*TODO: AD style ACE */
    size += DomSidGetSize(ace->trustee);

    return size;
}

static SecAclGetSize(
    const SecAcl *acl
    )
{
    uint32 size = 0;
    uint32 i;

    if (!acl) {
        return size;
    }

    size += 8;

    for (i=0; i < acl->num_aces; i++) {
        size += SecAceGetSize(&acl->aces[i]);
    }

    return size;
}

uint32 SecurityDescriptorGetSize(
    const SecDesc *security_descriptor
    )
{
    const SecDesc *sd = (const SecDesc *)security_descriptor;
    uint32 size = 0;

    if (!sd) {
        return size;
    }

    size += 20;
    size += DomSidGetSize(sd->owner);
    size += DomSidGetSize(sd->group);
    size += SecAclGetSize(sd->sacl);
    size += SecAclGetSize(sd->dacl);

    return size;
}

static NET_API_STATUS PushDomSid(
    const DomSid *sid,
    uint8 *buf,
    uint32 len,
    uint32 *size
    )
{
    uint32 sid_size;
    uint32 ofs = 0;
    uint32 i;

    if (!size) {
        return ERROR_INVALID_PARAMETER;
    }

    *size = 0;

    if (!sid) {
        return ERROR_SUCCESS;
    }

    if (sid->revision != 1) {
        return ERROR_INVALID_SID;
    }

    sid_size = DomSidGetSize(sid);

    if (len < sid_size) {
         return ERROR_INTERNAL_ERROR;
    }

    PUT_UINT8(buf, ofs, sid->revision);
    ofs += 1;

    PUT_UINT8(buf, ofs, sid->subauth_count);
    ofs += 1;

    memcpy(&buf[ofs], sid->authid, 6);
    ofs += 6;

    for (i=0; i < sid->subauth_count; i++) {
        PUT_UINT32(buf, ofs, sid->subauth[i]);
        ofs += 4;
    }

    if (ofs != sid_size) {
       return ERROR_INTERNAL_ERROR;
    }

    *size = sid_size;
    return ERROR_SUCCESS;
}

static NET_API_STATUS PushSecAce(
    const SecAce *ace,
    uint8 *buf,
    uint32 len,
    uint32 *size
    )
{
    NET_API_STATUS ret = ERROR_SUCCESS;
    uint32 ace_size;
    uint32 ofs = 0;
    uint32 trustee_size;

    if (!size) {
        return ERROR_INVALID_PARAMETER;
    }

    *size = 0;

    if (!ace) {
        return ERROR_SUCCESS;
    }

    ace_size = SecAceGetSize(ace);

    if (len < ace_size) {
         return ERROR_INTERNAL_ERROR;
    }

    PUT_UINT8(buf, ofs, ace->sec_ace_type);
    ofs += 1;

    PUT_UINT8(buf, ofs, ace->sec_ace_flags);
    ofs += 1;

    /* set the calculated size here */
    PUT_UINT16(buf, ofs, ace_size);
    ofs += 2;

    PUT_UINT32(buf, ofs, ace->access_mask);
    ofs += 4;

    /*TODO: AD style ACEs */

    ret = PushDomSid(ace->trustee, &buf[ofs],
                     ace_size - ofs, &trustee_size);
    if (ret != ERROR_SUCCESS) {
        return ret;
    }

    ofs += trustee_size;

    if (ofs != ace_size) {
       return ERROR_INTERNAL_ERROR;
    }

    *size = ace_size;
    return ERROR_SUCCESS;
}

static NET_API_STATUS PushSecAcl(
    const SecAcl *acl,
    uint8 *buf,
    uint32 len,
    uint32 *size
    )
{
    uint32 acl_size;
    uint32 ofs = 0;
    uint32 i;

    if (!size) {
        return ERROR_INVALID_PARAMETER;
    }

    *size = 0;

    if (!acl) {
        return ERROR_SUCCESS;
    }

    /* we only support NT4 acls yet */
    if (acl->revision != 2) {
        return ERROR_INVALID_ACL;
    }

    acl_size = SecAclGetSize(acl);

    if (len < acl_size) {
         return ERROR_INTERNAL_ERROR;
    }

    PUT_UINT16(buf, ofs, acl->revision);
    ofs += 2;

    /* set the calculated size here */
    PUT_UINT16(buf, ofs, acl_size);
    ofs += 2;

    PUT_UINT32(buf, ofs, acl->num_aces);
    ofs += 4;

    for (i=0; i < acl->num_aces; i++) {
        NET_API_STATUS ret = ERROR_SUCCESS;
        uint32 ace_size;

        ret = PushSecAce(&acl->aces[i], &buf[ofs],
                         acl_size - ofs, &ace_size);
        if (ret != ERROR_SUCCESS) {
            return ret;
        }
        ofs += ace_size;
    }

    if (ofs != acl_size) {
       return ERROR_INTERNAL_ERROR;
    }

    *size = acl_size;
    return ERROR_SUCCESS;
}

NET_API_STATUS SecurityDescriptorToBuffer(
    const PSECURITY_DESCRIPTOR security_descriptor,
    uint8 **bufptr,
    uint32 *buflen
    )
{
    NET_API_STATUS ret = ERROR_SUCCESS;
    const SecDesc *sd = (const SecDesc *)security_descriptor;
    uint32 sd_size;
    uint8 *buf = NULL;
    uint32 ofs = 0;
    uint32 owner_ofs = 0;
    uint32 group_ofs = 0;
    uint32 sacl_ofs = 0;
    uint32 dacl_ofs = 0;
    uint32 len;

    if (!bufptr || !buflen) {
        return ERROR_INVALID_PARAMETER;
    }

    *bufptr = NULL;
    *buflen = 0;

    if (!sd) {
        return ERROR_SUCCESS;
    }

    if (sd->revision != 1) {
        return ERROR_INVALID_SECURITY_DESCR;
    }

    if (sd->type & 0x8000) {
        return ERROR_BAD_DESCRIPTOR_FORMAT;
    }

    sd_size = SecurityDescriptorGetSize(security_descriptor);

    buf = malloc(sd_size);
    if (!buf) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    PUT_UINT8(buf, ofs, sd->revision);
    ofs += 1;

    PUT_UINT8(buf, ofs, 0);/* padding */
    ofs += 1;

    /* mark it as selfrelative */
    PUT_UINT16(buf, ofs, sd->type | 0x8000);
    ofs += 2;

    PUT_UINT32(buf, ofs, 0);/* owner */
    ofs += 4;

    PUT_UINT32(buf, ofs, 0);/* group */
    ofs += 4;

    PUT_UINT32(buf, ofs, 0);/* sacl */
    ofs += 4;

    PUT_UINT32(buf, ofs, 0);/* dacl */
    ofs += 4;

    ret = PushDomSid(sd->owner, &buf[ofs],
                     sd_size - ofs, &len);
    if (ret != ERROR_SUCCESS) {
        free(buf);
        return ret;
    }
    if (len) {
        owner_ofs = ofs;
    }
    ofs += len;

    ret = PushDomSid(sd->group, &buf[ofs],
                     sd_size - ofs, &len);
    if (ret != ERROR_SUCCESS) {
        free(buf);
        return ret;
    }
    if (len) {
        group_ofs = ofs;
    }
    ofs += len;

    ret = PushSecAcl(sd->sacl, &buf[ofs],
                     sd_size -ofs, &len);
    if (ret != ERROR_SUCCESS) {
        free(buf);
        return ret;
    }
    if (len) {
        sacl_ofs = ofs;
    }
    ofs += len;

    ret = PushSecAcl(sd->dacl, &buf[ofs],
                     sd_size - ofs, &len);
    if (ret != ERROR_SUCCESS) {
        free(buf);
        return ret;
    }
    if (len) {
        dacl_ofs = ofs;
    }
    ofs += len;

    PUT_UINT32(buf,  4, owner_ofs);
    PUT_UINT32(buf,  8, group_ofs);
    PUT_UINT32(buf, 12, sacl_ofs);
    PUT_UINT32(buf, 16, dacl_ofs);

    if (ofs != sd_size) {
        free(buf);
        return ERROR_INTERNAL_ERROR;
    }

    *bufptr = buf;
    *buflen = ofs;
    return ERROR_SUCCESS;
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
