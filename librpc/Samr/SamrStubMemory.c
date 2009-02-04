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
    int i = 0;

    for (i = 0; i < r->count; i++) {
        RidName *rn = &(r->entries[i]);
        FreeUnicodeString(&rn->name);
    }

    free(r->entries);
}


void SamrFreeStubRidNameArray(RidNameArray *ptr)
{
    SamrCleanStubRidNameArray(ptr);
    free(ptr);
}


void SamrCleanStubIds(Ids *r)
{
    SAFE_FREE(r->ids);
}


void SamrCleanStubUnicodeStringArray(UnicodeStringArray *r)
{
    int i = 0;

    for (i = 0; i < r->count; i++) {
        UnicodeString *s = &(r->names[i]);
        FreeUnicodeString(s);
    }

    free(r->names);
}


void SamrCleanStubEntryArray(EntryArray *r)
{
    int i = 0;

    for (i = 0; i < r->count; i++) {
        Entry *e = &(r->entries[i]);
        FreeUnicodeString(&e->name);
    }

    free(r->entries);
}


void SamrFreeStubEntryArray(EntryArray *ptr)
{
    SamrCleanStubEntryArray(ptr);
    free(ptr);
}


void SamrFreeStubDomSid(DomSid *ptr)
{
    SAFE_FREE(ptr);
}


void SamrCleanStubSidArray(SidArray *r)
{
    int i = 0;

    for (i = 0; i < r->num_sids; i++) {
        DomSid *s = r->sids[i].sid;
        SAFE_FREE(s);
    }

    free(r->sids);
}


void SamrCleanStubRidWithAttributeArray(RidWithAttributeArray *r)
{
    free(r->rids);
}


void SamrFreeStubRidWithAttributeArray(RidWithAttributeArray *ptr)
{
    SamrCleanStubRidWithAttributeArray(ptr);
    free(ptr);
}


void SamrCleanStubAliasInfo(AliasInfo *r, uint16 level)
{
    switch (level) {
    case ALIAS_INFO_ALL:
        FreeUnicodeString(&r->all.name);
        FreeUnicodeString(&r->all.description);
        break;

    case ALIAS_INFO_NAME:
        FreeUnicodeString(&r->name);
        break;

    case ALIAS_INFO_DESCRIPTION:
        FreeUnicodeString(&r->description);
        break;
    }
}


void SamrFreeStubAliasInfo(AliasInfo *ptr, uint16 level)
{
    SamrCleanStubAliasInfo(ptr, level);
    free(ptr);
}


void SamrCleanStubDomainInfo(DomainInfo *r, uint16 level)
{
    switch (level) {
    case 2:
        FreeUnicodeString(&r->info2.comment);
        FreeUnicodeString(&r->info2.domain_name);
        FreeUnicodeString(&r->info2.primary);
        break;

    case 4:
        FreeUnicodeString(&r->info4.comment);
        break;

    case 5:
        FreeUnicodeString(&r->info5.domain_name);
        break;

    case 6:
        FreeUnicodeString(&r->info6.primary);
        break;

    case 11:
        SamrCleanStubDomainInfo(r, 2);
    }
}


void SamrFreeStubDomainInfo(DomainInfo *ptr, uint16 level)
{
    SamrCleanStubDomainInfo(ptr, level);
    free(ptr);
}


void SamrCleanStubUserInfo(UserInfo *r, uint16 level)
{
    switch (level) {
    case 1:
        FreeUnicodeString(&r->info1.account_name);
        FreeUnicodeString(&r->info1.full_name);
        FreeUnicodeString(&r->info1.description);
        FreeUnicodeString(&r->info1.comment);
        break;

    case 2:
        FreeUnicodeString(&r->info2.comment);
        FreeUnicodeString(&r->info2.unknown1);
        break;

    case 3:
        FreeUnicodeString(&r->info3.account_name);
        FreeUnicodeString(&r->info3.full_name);
        FreeUnicodeString(&r->info3.home_directory);
        FreeUnicodeString(&r->info3.home_drive);
        FreeUnicodeString(&r->info3.logon_script);
        FreeUnicodeString(&r->info3.profile_path);
        FreeUnicodeString(&r->info3.workstations);
        SAFE_FREE(r->info3.logon_hours.units);
        break;

    case 4:
        SAFE_FREE(r->info4.logon_hours.units);
        break;

    case 5:
        FreeUnicodeString(&r->info5.account_name);
        FreeUnicodeString(&r->info5.full_name);
        FreeUnicodeString(&r->info5.home_directory);
        FreeUnicodeString(&r->info5.home_drive);
        FreeUnicodeString(&r->info5.logon_script);
        FreeUnicodeString(&r->info5.profile_path);
        FreeUnicodeString(&r->info5.description);
        FreeUnicodeString(&r->info5.workstations);
        SAFE_FREE(r->info5.logon_hours.units);
        break;

    case 6:
        FreeUnicodeString(&r->info6.account_name);
        FreeUnicodeString(&r->info6.full_name);
        break;

    case 7:
        FreeUnicodeString(&r->info7.account_name);
        break;

    case 8:
        FreeUnicodeString(&r->info8.full_name);
        break;

    case 10:
        FreeUnicodeString(&r->info10.home_directory);
        FreeUnicodeString(&r->info10.home_drive);
        break;

    case 11:
        FreeUnicodeString(&r->info11.logon_script);
        break;

    case 12:
        FreeUnicodeString(&r->info12.profile_path);
        break;

    case 13:
        FreeUnicodeString(&r->info13.description);
        break;

    case 14:
        FreeUnicodeString(&r->info14.workstations);
        break;

    case 20:
        FreeUnicodeString(&r->info20.parameters);
        break;

    case 21:
        FreeUnicodeString(&r->info21.account_name);
        FreeUnicodeString(&r->info21.full_name);
        FreeUnicodeString(&r->info21.home_directory);
        FreeUnicodeString(&r->info21.home_drive);
        FreeUnicodeString(&r->info21.logon_script);
        FreeUnicodeString(&r->info21.profile_path);
        FreeUnicodeString(&r->info21.description);
        FreeUnicodeString(&r->info21.workstations);
        FreeUnicodeString(&r->info21.comment);
        FreeUnicodeString(&r->info21.parameters);
        FreeUnicodeString(&r->info21.unknown1);
        FreeUnicodeString(&r->info21.unknown2);
        FreeUnicodeString(&r->info21.unknown3);
        if (&r->info21.buf_count) SAFE_FREE(r->info21.buffer);
        SAFE_FREE(r->info21.logon_hours.units);
        break;

    case 23:
        FreeUnicodeString(&r->info23.info.account_name);
        FreeUnicodeString(&r->info23.info.full_name);
        FreeUnicodeString(&r->info23.info.home_directory);
        FreeUnicodeString(&r->info23.info.home_drive);
        FreeUnicodeString(&r->info23.info.logon_script);
        FreeUnicodeString(&r->info23.info.profile_path);
        FreeUnicodeString(&r->info23.info.description);
        FreeUnicodeString(&r->info23.info.workstations);
        FreeUnicodeString(&r->info23.info.comment);
        FreeUnicodeString(&r->info23.info.parameters);
        FreeUnicodeString(&r->info23.info.unknown1);
        FreeUnicodeString(&r->info23.info.unknown2);
        FreeUnicodeString(&r->info23.info.unknown3);
        if (&r->info23.info.buf_count) {
            SAFE_FREE(r->info23.info.buffer);
        }
        SAFE_FREE(r->info23.info.logon_hours.units);

        break;

    case 25:
        FreeUnicodeString(&r->info25.info.account_name);
        FreeUnicodeString(&r->info25.info.full_name);
        FreeUnicodeString(&r->info25.info.home_directory);
        FreeUnicodeString(&r->info25.info.home_drive);
        FreeUnicodeString(&r->info25.info.logon_script);
        FreeUnicodeString(&r->info25.info.profile_path);
        FreeUnicodeString(&r->info25.info.description);
        FreeUnicodeString(&r->info25.info.workstations);
        FreeUnicodeString(&r->info25.info.comment);
        FreeUnicodeString(&r->info25.info.parameters);
        FreeUnicodeString(&r->info25.info.unknown1);
        FreeUnicodeString(&r->info25.info.unknown2);
        FreeUnicodeString(&r->info25.info.unknown3);
        if (&r->info25.info.buf_count) {
            SAFE_FREE(r->info25.info.buffer);
        }
        SAFE_FREE(r->info25.info.logon_hours.units);

        break;
    }
}


void SamrFreeStubUserInfo(UserInfo *ptr, uint16 level)
{
    SamrCleanStubUserInfo(ptr, level);
    free(ptr);
}



/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
