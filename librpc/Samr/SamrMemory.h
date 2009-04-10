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

#ifndef _SAMR_MEMORY_H_
#define _SAMR_MEMORY_H_


NTSTATUS
SamrAllocateMemory(
    void **out,
    size_t len,
    void *dep
    );


NTSTATUS
SamrAddDepMemory(
    void *ptr,
    void *dep
    );


NTSTATUS
SamrAllocateNamesAndRids(
    wchar16_t ***outn,
    uint32 **outr,
    RidNameArray *in
    );


NTSTATUS
SamrAllocateNames(
    wchar16_t ***out,
    EntryArray *in
    );


NTSTATUS
SamrAllocateIds(
    uint32 **out,
    Ids *in
    );


NTSTATUS
SamrAllocateDomSid(
    PSID* out,
    PSID in,
    void *dep
    );


NTSTATUS
SamrAllocateSids(
    PSID** out,
    SidArray *in
    );


NTSTATUS
SamrAllocateRidsAndAttributes(
    uint32 **out_rids,
    uint32 **out_attrs,
    RidWithAttributeArray *in
    );


NTSTATUS
SamrAllocateAliasInfo(
    AliasInfo **out,
    AliasInfo *in,
    uint16 level
    );


NTSTATUS
SamrAllocateDomainInfo(
    DomainInfo **out,
    DomainInfo *in,
    uint16 level
    );


NTSTATUS
SamrAllocateUserInfo(
    UserInfo **out,
    UserInfo *in,
    uint16 level
    );


NTSTATUS
SamrAllocateDisplayInfo(
    SamrDisplayInfo **out,
    SamrDisplayInfo *in,
    uint16 level
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
