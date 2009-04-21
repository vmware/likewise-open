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

#include "includes.h"


void SamrCleanStubRidNameArray(RidNameArray *r)
{
    RPCSTATUS st = 0;
    int i = 0;

    for (i = 0; i < r->count; i++) {
        RidName *rn = &(r->entries[i]);
        rpc_sm_client_free(rn->name.string, &st);
    }

    rpc_sm_client_free(r->entries, &st);
}


void SamrFreeStubRidNameArray(RidNameArray *ptr)
{
    RPCSTATUS st = 0;

    SamrCleanStubRidNameArray(ptr);
    rpc_sm_client_free(ptr, &st);
}


void SamrCleanStubIds(Ids *r)
{
    RPCSTATUS st = 0;

    if (r->count) {
        rpc_sm_client_free(r->ids, &st);
    }
}


void SamrCleanStubUnicodeStringArray(UnicodeStringArray *r)
{
    RPCSTATUS st = 0;
    int i = 0;

    for (i = 0; i < r->count; i++) {
        UnicodeString *s = &(r->names[i]);
        rpc_sm_client_free(s->string, &st);
    }

    rpc_sm_client_free(r->names, &st);
}


void SamrCleanStubEntryArray(EntryArray *r)
{
    RPCSTATUS st = 0;
    int i = 0;

    for (i = 0; i < r->count; i++) {
        Entry *e = &(r->entries[i]);
        rpc_sm_client_free(e->name.string, &st);
    }

    rpc_sm_client_free(r->entries, &st);
}


void SamrFreeStubEntryArray(EntryArray *ptr)
{
    RPCSTATUS st = 0;

    SamrCleanStubEntryArray(ptr);
    rpc_sm_client_free(ptr, &st);
}


void SamrFreeStubDomSid(PSID ptr)
{
    RPCSTATUS st = 0;

    rpc_sm_client_free(ptr, &st);
}


void SamrCleanStubSidArray(SidArray *r)
{
    RPCSTATUS st = 0;
    int i = 0;

    for (i = 0; i < r->num_sids; i++) {
        PSID s = r->sids[i].sid;
        rpc_sm_client_free(s, &st);
    }

    rpc_sm_client_free(r->sids, &st);
}


void SamrCleanStubRidWithAttributeArray(RidWithAttributeArray *r)
{
    RPCSTATUS st = 0;

    rpc_sm_client_free(r->rids, &st);
}


void SamrFreeStubRidWithAttributeArray(RidWithAttributeArray *ptr)
{
    SamrCleanStubRidWithAttributeArray(ptr);
    free(ptr);
}


void SamrCleanStubAliasInfo(AliasInfo *r, uint16 level)
{
    RPCSTATUS st = 0;

    switch (level) {
    case ALIAS_INFO_ALL:
        rpc_sm_client_free(r->all.name.string, &st);
        rpc_sm_client_free(r->all.description.string, &st);
        break;

    case ALIAS_INFO_NAME:
        rpc_sm_client_free(r->name.string, &st);
        break;

    case ALIAS_INFO_DESCRIPTION:
        rpc_sm_client_free(r->description.string, &st);
        break;
    }
}


void SamrFreeStubAliasInfo(AliasInfo *ptr, uint16 level)
{
    RPCSTATUS st = 0;

    SamrCleanStubAliasInfo(ptr, level);
    rpc_sm_client_free(ptr, &st);
}


void SamrCleanStubDomainInfo(DomainInfo *r, uint16 level)
{
    RPCSTATUS st = 0;

    switch (level) {
    case 2:
        rpc_sm_client_free(r->info2.comment.string, &st);
        rpc_sm_client_free(r->info2.domain_name.string, &st);
        rpc_sm_client_free(r->info2.primary.string, &st);
        break;

    case 4:
        rpc_sm_client_free(r->info4.comment.string, &st);
        break;

    case 5:
        rpc_sm_client_free(r->info5.domain_name.string, &st);
        break;

    case 6:
        rpc_sm_client_free(r->info6.primary.string, &st);
        break;

    case 11:
        SamrCleanStubDomainInfo(r, 2);
    }
}


void SamrFreeStubDomainInfo(DomainInfo *ptr, uint16 level)
{
    RPCSTATUS st = 0;

    SamrCleanStubDomainInfo(ptr, level);
    rpc_sm_client_free(ptr, &st);
}


void SamrCleanStubUserInfo(UserInfo *r, uint16 level)
{
    RPCSTATUS st = 0;

    switch (level) {
    case 1:
        rpc_sm_client_free(r->info1.account_name.string, &st);
        rpc_sm_client_free(r->info1.full_name.string, &st);
        rpc_sm_client_free(r->info1.description.string, &st);
        rpc_sm_client_free(r->info1.comment.string, &st);
        break;

    case 2:
        rpc_sm_client_free(r->info2.comment.string, &st);
        rpc_sm_client_free(r->info2.unknown1.string, &st);
        break;

    case 3:
        rpc_sm_client_free(r->info3.account_name.string, &st);
        rpc_sm_client_free(r->info3.full_name.string, &st);
        rpc_sm_client_free(r->info3.home_directory.string, &st);
        rpc_sm_client_free(r->info3.home_drive.string, &st);
        rpc_sm_client_free(r->info3.logon_script.string, &st);
        rpc_sm_client_free(r->info3.profile_path.string, &st);
        rpc_sm_client_free(r->info3.workstations.string, &st);
        rpc_sm_client_free(r->info3.logon_hours.units, &st);
        break;

    case 4:
        rpc_sm_client_free(r->info4.logon_hours.units, &st);
        break;

    case 5:
        rpc_sm_client_free(r->info5.account_name.string, &st);
        rpc_sm_client_free(r->info5.full_name.string, &st);
        rpc_sm_client_free(r->info5.home_directory.string, &st);
        rpc_sm_client_free(r->info5.home_drive.string, &st);
        rpc_sm_client_free(r->info5.logon_script.string, &st);
        rpc_sm_client_free(r->info5.profile_path.string, &st);
        rpc_sm_client_free(r->info5.description.string, &st);
        rpc_sm_client_free(r->info5.workstations.string, &st);
        rpc_sm_client_free(r->info5.logon_hours.units, &st);
        break;

    case 6:
        rpc_sm_client_free(r->info6.account_name.string, &st);
        rpc_sm_client_free(r->info6.full_name.string, &st);
        break;

    case 7:
        rpc_sm_client_free(r->info7.account_name.string, &st);
        break;

    case 8:
        rpc_sm_client_free(r->info8.full_name.string, &st);
        break;

    case 10:
        rpc_sm_client_free(r->info10.home_directory.string, &st);
        rpc_sm_client_free(r->info10.home_drive.string, &st);
        break;

    case 11:
        rpc_sm_client_free(r->info11.logon_script.string, &st);
        break;

    case 12:
        rpc_sm_client_free(r->info12.profile_path.string, &st);
        break;

    case 13:
        rpc_sm_client_free(r->info13.description.string, &st);
        break;

    case 14:
        rpc_sm_client_free(r->info14.workstations.string, &st);
        break;

    case 20:
        rpc_sm_client_free(r->info20.parameters.string, &st);
        break;

    case 21:
        rpc_sm_client_free(r->info21.account_name.string, &st);
        rpc_sm_client_free(r->info21.full_name.string, &st);
        rpc_sm_client_free(r->info21.home_directory.string, &st);
        rpc_sm_client_free(r->info21.home_drive.string, &st);
        rpc_sm_client_free(r->info21.logon_script.string, &st);
        rpc_sm_client_free(r->info21.profile_path.string, &st);
        rpc_sm_client_free(r->info21.description.string, &st);
        rpc_sm_client_free(r->info21.workstations.string, &st);
        rpc_sm_client_free(r->info21.comment.string, &st);
        rpc_sm_client_free(r->info21.parameters.string, &st);
        rpc_sm_client_free(r->info21.unknown1.string, &st);
        rpc_sm_client_free(r->info21.unknown2.string, &st);
        rpc_sm_client_free(r->info21.unknown3.string, &st);
        if (&r->info21.buf_count) {
            rpc_sm_client_free(r->info21.buffer, &st);
        }
        rpc_sm_client_free(r->info21.logon_hours.units, &st);
        break;

    case 23:
        rpc_sm_client_free(r->info23.info.account_name.string, &st);
        rpc_sm_client_free(r->info23.info.full_name.string, &st);
        rpc_sm_client_free(r->info23.info.home_directory.string, &st);
        rpc_sm_client_free(r->info23.info.home_drive.string, &st);
        rpc_sm_client_free(r->info23.info.logon_script.string, &st);
        rpc_sm_client_free(r->info23.info.profile_path.string, &st);
        rpc_sm_client_free(r->info23.info.description.string, &st);
        rpc_sm_client_free(r->info23.info.workstations.string, &st);
        rpc_sm_client_free(r->info23.info.comment.string, &st);
        rpc_sm_client_free(r->info23.info.parameters.string, &st);
        rpc_sm_client_free(r->info23.info.unknown1.string, &st);
        rpc_sm_client_free(r->info23.info.unknown2.string, &st);
        rpc_sm_client_free(r->info23.info.unknown3.string, &st);
        if (&r->info23.info.buf_count) {
            rpc_sm_client_free(r->info23.info.buffer, &st);
        }
        rpc_sm_client_free(r->info23.info.logon_hours.units, &st);

        break;

    case 25:
        rpc_sm_client_free(r->info25.info.account_name.string, &st);
        rpc_sm_client_free(r->info25.info.full_name.string, &st);
        rpc_sm_client_free(r->info25.info.home_directory.string, &st);
        rpc_sm_client_free(r->info25.info.home_drive.string, &st);
        rpc_sm_client_free(r->info25.info.logon_script.string, &st);
        rpc_sm_client_free(r->info25.info.profile_path.string, &st);
        rpc_sm_client_free(r->info25.info.description.string, &st);
        rpc_sm_client_free(r->info25.info.workstations.string, &st);
        rpc_sm_client_free(r->info25.info.comment.string, &st);
        rpc_sm_client_free(r->info25.info.parameters.string, &st);
        rpc_sm_client_free(r->info25.info.unknown1.string, &st);
        rpc_sm_client_free(r->info25.info.unknown2.string, &st);
        rpc_sm_client_free(r->info25.info.unknown3.string, &st);
        if (&r->info25.info.buf_count) {
            rpc_sm_client_free(r->info25.info.buffer, &st);
        }
        rpc_sm_client_free(r->info25.info.logon_hours.units, &st);

        break;
    }
}


void SamrFreeStubUserInfo(UserInfo *ptr, uint16 level)
{
    RPCSTATUS st = 0;

    SamrCleanStubUserInfo(ptr, level);
    rpc_sm_client_free(ptr, &st);
}


static
void
SamrCleanStubDisplayInfoFull(
    SamrDisplayInfoFull *ptr
    )
{
    RPCSTATUS st = 0;
    uint32 i = 0;

    for (i = 0; i < ptr->count; i++) {
        SamrDisplayEntryFull *e = &(ptr->entries[i]);

        rpc_sm_client_free(e->account_name.string, &st);
        rpc_sm_client_free(e->description.string, &st);
        rpc_sm_client_free(e->full_name.string, &st);
    }
}


static
void
SamrCleanStubDisplayInfoGeneral(
    SamrDisplayInfoGeneral *ptr
    )
{
    RPCSTATUS st = 0;
    uint32 i = 0;

    for (i = 0; i < ptr->count; i++) {
        SamrDisplayEntryGeneral *e = &(ptr->entries[i]);

        rpc_sm_client_free(e->account_name.string, &st);
        rpc_sm_client_free(e->description.string, &st);
    }
}


static
void
SamrCleanStubDisplayInfoGeneralGroups(
    SamrDisplayInfoGeneralGroups *ptr
    )
{
    RPCSTATUS st = 0;
    uint32 i = 0;

    for (i = 0; i < ptr->count; i++) {
        SamrDisplayEntryGeneralGroup *e = &(ptr->entries[i]);

        rpc_sm_client_free(e->account_name.string, &st);
        rpc_sm_client_free(e->description.string, &st);
    }
}


static
void
SamrCleanStubDisplayInfoAscii(
    SamrDisplayInfoAscii *ptr
    )
{
    RPCSTATUS st = 0;
    uint32 i = 0;

    for (i = 0; i < ptr->count; i++) {
        SamrDisplayEntryAscii *e = &(ptr->entries[i]);

        if (e->account_name.Buffer) {
            rpc_sm_client_free(e->account_name.Buffer, &st);
        }
    }
}


void
SamrCleanStubDisplayInfo(
    SamrDisplayInfo *ptr,
    uint16 level
    )
{
    switch (level) {
    case 1:
        SamrCleanStubDisplayInfoFull(&ptr->info1);
        break;

    case 2:
        SamrCleanStubDisplayInfoGeneral(&ptr->info2);
        break;

    case 3:
        SamrCleanStubDisplayInfoGeneralGroups(&ptr->info3);
        break;

    case 4:
        SamrCleanStubDisplayInfoAscii(&ptr->info4);
        break;

    case 5:
        SamrCleanStubDisplayInfoAscii(&ptr->info5);
        break;
    }
}



/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
