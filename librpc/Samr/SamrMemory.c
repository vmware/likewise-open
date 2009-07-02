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
 * Abstract: Samr memory (de)allocation routines (rpc client library)
 *
 * Authors: Rafal Szczesniak (rafal@likewisesoftware.com)
 */

#include "includes.h"


NTSTATUS SamrInitMemory()
{
    NTSTATUS status = STATUS_SUCCESS;
    int locked = 0;

    GLOBAL_DATA_LOCK(locked);

    if (!bSamrInitialised) {
        status = MemPtrListInit((PtrList**)&samr_ptr_list);
        BAIL_ON_NTSTATUS_ERROR(status);

        bSamrInitialised = 1;
    }

cleanup:
    GLOBAL_DATA_UNLOCK(locked);

    return status;

error:
    goto cleanup;
}


NTSTATUS SamrDestroyMemory()
{
    NTSTATUS status = STATUS_SUCCESS;
    int locked = 0;

    GLOBAL_DATA_LOCK(locked);

    if (bSamrInitialised && samr_ptr_list) {
        status = MemPtrListDestroy((PtrList**)&samr_ptr_list);
        BAIL_ON_NTSTATUS_ERROR(status);

        bSamrInitialised = 0;
    }

cleanup:
    GLOBAL_DATA_UNLOCK(locked);

    return status;

error:
    goto cleanup;
}


NTSTATUS SamrAllocateMemory(void **out, size_t size, void *dep)
{
    return MemPtrAllocate((PtrList*)samr_ptr_list, out, size, dep);
}


NTSTATUS SamrFreeMemory(void *ptr)
{
    return MemPtrFree((PtrList*)samr_ptr_list, ptr);
}


NTSTATUS SamrAddDepMemory(void *ptr, void *dep)
{
    return MemPtrAddDependant((PtrList*)samr_ptr_list, ptr, dep);
}


NTSTATUS SamrAllocateNamesAndRids(wchar16_t ***outn, uint32 **outr,
                                  RidNameArray *in)
{
    NTSTATUS status = STATUS_SUCCESS;
    wchar16_t **ptrn = NULL;
    uint32 *ptrr = NULL;
    int i = 0;

    BAIL_ON_INVALID_PTR(outn);
    BAIL_ON_INVALID_PTR(outr);
    BAIL_ON_INVALID_PTR(in);

    status = SamrAllocateMemory((void**)&ptrn,
                                sizeof(wchar16_t*) * in->count,
                                NULL);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrAllocateMemory((void**)&ptrr,
                                sizeof(uint32) * in->count,
                                NULL);
    BAIL_ON_NTSTATUS_ERROR(status);

    for (i = 0; i < in->count; i++) {
        RidName *rn = &(in->entries[i]);
        
        ptrr[i]  = rn->rid;
        ptrn[i] = GetFromUnicodeString(&rn->name);
        BAIL_ON_NO_MEMORY(ptrn[i]);
        
        status = SamrAddDepMemory((void*)ptrn[i], (void*)ptrn);
        BAIL_ON_NTSTATUS_ERROR(status);
    }

    *outn = ptrn;
    *outr = ptrr;

cleanup:
    return status;

error:
    if (ptrn) {
        SamrFreeMemory((void*)ptrn);
    }

    if (ptrr) {
        SamrFreeMemory((void*)ptrr);
    }

    *outn = NULL;
    *outr = NULL;
    goto cleanup;
}


NTSTATUS SamrAllocateNames(wchar16_t ***out, EntryArray *in)
{
    NTSTATUS status = STATUS_SUCCESS;
    wchar16_t **ptr = NULL;
    int i = 0;

    BAIL_ON_INVALID_PTR(out);
    BAIL_ON_INVALID_PTR(in);

    status = SamrAllocateMemory((void**)&ptr,
                                sizeof(wchar16_t*) * in->count,
                                NULL);
    BAIL_ON_NTSTATUS_ERROR(status);

    for (i = 0; i < in->count; i++) {
        Entry *e = &(in->entries[i]);

        ptr[i] = GetFromUnicodeString(&e->name);
        BAIL_ON_NO_MEMORY(ptr[i]);

        status = SamrAddDepMemory((void*)ptr[i], (void*)ptr);
        BAIL_ON_NTSTATUS_ERROR(status);
    }

    *out = ptr;

cleanup:
    return status;

error:
    if (ptr) {
        SamrFreeMemory((void*)ptr);
    }

    *out = NULL;
    goto cleanup;
}


NTSTATUS SamrAllocateIds(uint32 **out, Ids *in)
{
    NTSTATUS status = STATUS_SUCCESS;
    uint32 *ptr = NULL;
    int i = 0;

    BAIL_ON_INVALID_PTR(out);
    BAIL_ON_INVALID_PTR(in);

    status = SamrAllocateMemory((void**)&ptr,
                                sizeof(uint32) * in->count,
                                NULL);
    BAIL_ON_NTSTATUS_ERROR(status);

    for (i = 0; i < in->count; i++) {
        ptr[i] = in->ids[i];
    }

    *out = ptr;

cleanup:
    return status;

error:
    if (ptr) {
        SamrFreeMemory((void*)ptr);
    }

    *out = NULL;
    goto cleanup;
}


NTSTATUS SamrAllocateDomSid(PSID* out, PSID in, void *dep)
{
    NTSTATUS status = STATUS_SUCCESS;
    PSID ptr = NULL;

    BAIL_ON_INVALID_PTR(out);

    MsRpcDuplicateSid(&ptr, in);
    BAIL_ON_NO_MEMORY(ptr);

    status = SamrAddDepMemory((void*)ptr, (void*)dep);
    BAIL_ON_NTSTATUS_ERROR(status);

    *out = ptr;

cleanup:
    return status;

error:
    if (ptr) {
        SamrFreeMemory((void*)ptr);
    }

    *out = NULL;
    goto cleanup;
}


NTSTATUS SamrAllocateSids(PSID** out, SidArray *in)
{
    NTSTATUS status = STATUS_SUCCESS;
    PSID* ptr = NULL;
    int i = 0;

    BAIL_ON_INVALID_PTR(out);
    BAIL_ON_INVALID_PTR(in);

    status = SamrAllocateMemory((void**)&ptr,
                                sizeof(PSID) * in->num_sids,
                                NULL);
    BAIL_ON_NTSTATUS_ERROR(status);

    for (i = 0; i < in->num_sids; i++) {
        PSID sid = in->sids[i].sid;

        status = SamrAllocateDomSid(&(ptr[i]), sid, (void*)ptr);
        BAIL_ON_NTSTATUS_ERROR(status);
    }

    *out = ptr;

cleanup:
    return status;

error:
    if (ptr) {
        SamrFreeMemory((void*)ptr);
    }

    *out = NULL;
    goto cleanup;
}


NTSTATUS SamrAllocateRidsAndAttributes(uint32 **out_rids, uint32 **out_attrs,
                                       RidWithAttributeArray *in)
{
    NTSTATUS status = STATUS_SUCCESS;
    uint32 *ptr_rids = NULL;
    uint32 *ptr_attrs = NULL;
    int i = 0;

    BAIL_ON_INVALID_PTR(out_rids);
    BAIL_ON_INVALID_PTR(out_attrs);
    BAIL_ON_INVALID_PTR(in);

    status = SamrAllocateMemory((void**)&ptr_rids,
                                sizeof(uint32) * in->count,
                                NULL);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrAllocateMemory((void**)&ptr_attrs,
                                sizeof(uint32) * in->count,
                                NULL);
    BAIL_ON_NTSTATUS_ERROR(status);

    for (i = 0; i < in->count; i++) {
        RidWithAttribute *ra = &(in->rids[i]);

        ptr_rids[i]  = ra->rid;
        ptr_attrs[i] = ra->attributes;
    }

    *out_rids  = ptr_rids;
    *out_attrs = ptr_attrs;

cleanup:
    return status;

error:
    if (ptr_rids) {
        SamrFreeMemory((void*)ptr_rids);
    }

    if (ptr_attrs) {
        SamrFreeMemory((void*)ptr_attrs);
    }

    *out_rids  = NULL;
    *out_attrs = NULL;
    goto cleanup;
}


static
NTSTATUS
SamrCopyUnicodeString(
    UnicodeString *out,
    UnicodeString *in,
    void *dep
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(out);
    BAIL_ON_INVALID_PTR(in);

    status = CopyUnicodeString(out, in);
    BAIL_ON_NTSTATUS_ERROR(status);

    if (out->string) {
        status = SamrAddDepMemory((void*)out->string, dep);
        BAIL_ON_NTSTATUS_ERROR(status);
    }

cleanup:
    return status;

error:
    goto cleanup;
}


NTSTATUS SamrAllocateAliasInfo(AliasInfo **out, AliasInfo *in, uint16 level)
{
    NTSTATUS status = STATUS_SUCCESS;
    AliasInfo *ptr = NULL;

    BAIL_ON_INVALID_PTR(out);
    BAIL_ON_INVALID_PTR(in);

    status = SamrAllocateMemory((void*)&ptr, sizeof(AliasInfo), NULL);
    BAIL_ON_NTSTATUS_ERROR(status);

    switch (level) {
    case ALIAS_INFO_ALL:
        status = SamrCopyUnicodeString(&ptr->all.name, &in->all.name, (void*)ptr);
        BAIL_ON_NTSTATUS_ERROR(status);

        status = SamrCopyUnicodeString(&ptr->all.description,
                                       &in->all.description,
                                       (void*)ptr);
        BAIL_ON_NTSTATUS_ERROR(status);

        ptr->all.num_members = in->all.num_members;
        break;

    case ALIAS_INFO_NAME:
        status = SamrCopyUnicodeString(&ptr->name, &in->name, (void*)ptr);
        BAIL_ON_NTSTATUS_ERROR(status);
        break;

    case ALIAS_INFO_DESCRIPTION:
        status = SamrCopyUnicodeString(&ptr->description,
                                       &in->description,
                                       (void*)ptr);
        BAIL_ON_NTSTATUS_ERROR(status);
        break;

    default:
        status = STATUS_INVALID_LEVEL;
        goto error;
    }

    *out = ptr;

cleanup:
    return status;

error:
    if (ptr) {
        SamrFreeMemory((void*)ptr);
    }

    *out = NULL;
    goto cleanup;
}


static
NTSTATUS
SamrCopyDomainInfo1(
    DomainInfo1 *out,
    DomainInfo1 *in,
    void *dep
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(out);
    BAIL_ON_INVALID_PTR(in);

    memcpy((void*)out, (void*)in, sizeof(DomainInfo1));

cleanup:
    return status;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyDomainInfo2(
    DomainInfo2 *out,
    DomainInfo2 *in,
    void *dep
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(out);
    BAIL_ON_INVALID_PTR(in);

    status = CopyUnicodeString(&out->comment, &in->comment);
    BAIL_ON_NTSTATUS_ERROR(status);

    if (out->comment.string) {
        status = SamrAddDepMemory((void*)out->comment.string, dep);
        BAIL_ON_NTSTATUS_ERROR(status);
    }

    status = CopyUnicodeString(&out->domain_name, &in->domain_name);
    BAIL_ON_NTSTATUS_ERROR(status);

    if (out->domain_name.string) {
        status = SamrAddDepMemory((void*)out->domain_name.string, dep);
        BAIL_ON_NTSTATUS_ERROR(status);
    }

    status = CopyUnicodeString(&out->primary, &in->primary);
    BAIL_ON_NTSTATUS_ERROR(status);

    if (out->primary.string) {
        status = SamrAddDepMemory((void*)out->primary.string, dep);
        BAIL_ON_NTSTATUS_ERROR(status);
    }

    out->force_logoff_time = in->force_logoff_time;
    out->sequence_num      = in->sequence_num;
    out->unknown1          = in->unknown1;
    out->role              = in->role;
    out->unknown2          = in->unknown2;
    out->num_users         = in->num_users;
    out->num_groups        = in->num_groups;
    out->num_aliases       = in->num_aliases;

cleanup:
    return status;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyDomainInfo3(
    DomainInfo3 *out,
    DomainInfo3 *in,
    void *dep
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(out);
    BAIL_ON_INVALID_PTR(in);

    out->force_logoff_time = in->force_logoff_time;

cleanup:
    return status;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyDomainInfo4(
    DomainInfo4 *out,
    DomainInfo4 *in,
    void *dep
    )
{
    return SamrCopyUnicodeString(&out->comment, &in->comment, dep);
}


static
NTSTATUS
SamrCopyDomainInfo5(
    DomainInfo5 *out,
    DomainInfo5 *in,
    void *dep
    )
{
    return SamrCopyUnicodeString(&out->domain_name, &in->domain_name, dep);
}


static
NTSTATUS
SamrCopyDomainInfo6(
    DomainInfo6 *out,
    DomainInfo6 *in,
    void *dep
    )
{
    return SamrCopyUnicodeString(&out->primary, &in->primary, dep);
}


static
NTSTATUS
SamrCopyDomainInfo7(
    DomainInfo7 *out,
    DomainInfo7 *in,
    void *dep
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(out);
    BAIL_ON_INVALID_PTR(in);

    out->role = in->role;

cleanup:
    return status;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyDomainInfo8(
    DomainInfo8 *out,
    DomainInfo8 *in,
    void *dep
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(out);
    BAIL_ON_INVALID_PTR(in);

    out->sequence_number    = in->sequence_number;
    out->domain_create_time = in->domain_create_time;

cleanup:
    return status;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyDomainInfo9(
    DomainInfo9 *out,
    DomainInfo9 *in,
    void *dep
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(out);
    BAIL_ON_INVALID_PTR(in);

    out->unknown = in->unknown;

cleanup:
    return status;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyDomainInfo11(
    DomainInfo11 *out,
    DomainInfo11 *in,
    void *dep
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(out);
    BAIL_ON_INVALID_PTR(in);

    status = SamrCopyDomainInfo2(&out->info2, &in->info2, dep);
    BAIL_ON_NTSTATUS_ERROR(status);

    out->lockout_duration  = in->lockout_duration;
    out->lockout_window    = in->lockout_window;
    out->lockout_threshold = in->lockout_threshold;

cleanup:
    return status;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyDomainInfo12(
    DomainInfo12 *out,
    DomainInfo12 *in,
    void *dep
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(out);
    BAIL_ON_INVALID_PTR(in);

    out->lockout_duration  = in->lockout_duration;
    out->lockout_window    = in->lockout_window;
    out->lockout_threshold = in->lockout_threshold;

cleanup:
    return status;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyDomainInfo13(
    DomainInfo13 *out,
    DomainInfo13 *in,
    void *dep
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(out);
    BAIL_ON_INVALID_PTR(in);

    out->sequence_number    = in->sequence_number;
    out->domain_create_time = in->domain_create_time;
    out->unknown1           = in->unknown1;
    out->unknown2           = in->unknown2;

cleanup:
    return status;

error:
    goto cleanup;
}


NTSTATUS
SamrAllocateDomainInfo(
    DomainInfo **out,
    DomainInfo *in,
    uint16 level
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    DomainInfo *ptr = NULL;

    BAIL_ON_INVALID_PTR(out);
    BAIL_ON_INVALID_PTR(in);

    status = SamrAllocateMemory((void*)&ptr, sizeof(DomainInfo), NULL);
    BAIL_ON_NTSTATUS_ERROR(status);

    if (in != NULL) {
        switch (level) {
        case 1:
            status = SamrCopyDomainInfo1(&ptr->info1, &in->info1, (void*)ptr);
            BAIL_ON_NTSTATUS_ERROR(status);
            break;

        case 2:
            status = SamrCopyDomainInfo2(&ptr->info2, &in->info2, (void*)ptr);
            BAIL_ON_NTSTATUS_ERROR(status);
            break;

        case 3:
            status = SamrCopyDomainInfo3(&ptr->info3, &in->info3, (void*)ptr);
            BAIL_ON_NTSTATUS_ERROR(status);
            break;

        case 4:
            status = SamrCopyDomainInfo4(&ptr->info4, &in->info4, (void*)ptr);
            BAIL_ON_NTSTATUS_ERROR(status);
            break;

        case 5:
            status = SamrCopyDomainInfo5(&ptr->info5, &in->info5, (void*)ptr);
            BAIL_ON_NTSTATUS_ERROR(status);
            break;

        case 6:
            status = SamrCopyDomainInfo6(&ptr->info6, &in->info6, (void*)ptr);
            BAIL_ON_NTSTATUS_ERROR(status);
            break;

        case 7:
            status = SamrCopyDomainInfo7(&ptr->info7, &in->info7, (void*)ptr);
            BAIL_ON_NTSTATUS_ERROR(status);
            break;

        case 8:
            status = SamrCopyDomainInfo8(&ptr->info8, &in->info8, (void*)ptr);
            BAIL_ON_NTSTATUS_ERROR(status);
            break;

        case 9:
            status = SamrCopyDomainInfo9(&ptr->info9, &in->info9, (void*)ptr);
            BAIL_ON_NTSTATUS_ERROR(status);
            break;

        case 11:
            status = SamrCopyDomainInfo11(&ptr->info11, &in->info11,
                                          (void*)ptr);
            BAIL_ON_NTSTATUS_ERROR(status);
            break;

        case 12:
            status = SamrCopyDomainInfo12(&ptr->info12, &in->info12,
                                          (void*)ptr);
            BAIL_ON_NTSTATUS_ERROR(status);
            break;

        case 13:
            status = SamrCopyDomainInfo13(&ptr->info13, &in->info13,
                                          (void*)ptr);
            BAIL_ON_NTSTATUS_ERROR(status);
            break;

        default:
            status = STATUS_INVALID_LEVEL;
            goto error;
        }
    }

    *out = ptr;

cleanup:
    return status;

error:
    if (ptr) {
        SamrFreeMemory((void*)ptr);
    }

    *out = NULL;
    goto cleanup;
}


static
NTSTATUS
SamrCopyUserInfo1(
    UserInfo1 *out,
    UserInfo1 *in,
    void *dep
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(out);
    BAIL_ON_INVALID_PTR(in);

    status = SamrCopyUnicodeString(&out->account_name, &in->account_name, dep);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrCopyUnicodeString(&out->full_name, &in->full_name, dep);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrCopyUnicodeString(&out->description, &in->description, dep);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrCopyUnicodeString(&out->comment, &in->comment, dep);
    BAIL_ON_NTSTATUS_ERROR(status);

    out->primary_gid = in->primary_gid;

cleanup:
    return status;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyUserInfo2(
    UserInfo2 *out,
    UserInfo2 *in,
    void *dep
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(out);
    BAIL_ON_INVALID_PTR(in);

    status = SamrCopyUnicodeString(&out->comment, &in->comment, dep);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrCopyUnicodeString(&out->unknown1, &in->unknown1, dep);
    BAIL_ON_NTSTATUS_ERROR(status);

    out->country_code = in->country_code;
    out->code_page    = in->code_page;

cleanup:
    return status;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyLogonHours(
    LogonHours *out,
    LogonHours *in,
    void *dep
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(out);
    BAIL_ON_INVALID_PTR(in);

    /* Allocate 1260 bytes and copy units_per_week/8 bytes
       according to samr.idl */

    status = SamrAllocateMemory((void**)&out->units, sizeof(uint8) * 1260, NULL);
    BAIL_ON_NTSTATUS_ERROR(status);

    memcpy((void*)out->units, (void*)in->units, in->units_per_week/8);

cleanup:
    return status;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyUserInfo3(
    UserInfo3 *out,
    UserInfo3 *in,
    void *dep
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(out);
    BAIL_ON_INVALID_PTR(in);

    status = SamrCopyUnicodeString(&out->account_name, &in->account_name, dep);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrCopyUnicodeString(&out->full_name, &in->full_name, dep);
    BAIL_ON_NTSTATUS_ERROR(status);

    out->rid         = in->rid;
    out->primary_gid = in->primary_gid;

    status = SamrCopyUnicodeString(&out->home_directory, &in->home_directory, dep);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrCopyUnicodeString(&out->home_drive, &in->home_drive, dep);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrCopyUnicodeString(&out->logon_script, &in->logon_script, dep);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrCopyUnicodeString(&out->profile_path, &in->profile_path, dep);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrCopyUnicodeString(&out->workstations, &in->workstations, dep);
    BAIL_ON_NTSTATUS_ERROR(status);

    out->last_logon             = in->last_logon;
    out->last_logoff            = in->last_logoff;
    out->last_password_change   = in->last_password_change;
    out->allow_password_change  = in->allow_password_change;
    out->force_password_change  = in->force_password_change;

    status = SamrCopyLogonHours(&out->logon_hours, &in->logon_hours, dep);
    BAIL_ON_NTSTATUS_ERROR(status);

    out->bad_password_count = in->bad_password_count;
    out->logon_count        = in->logon_count;
    out->account_flags      = in->account_flags;

cleanup:
    return status;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyUserInfo4(
    UserInfo4 *out,
    UserInfo4 *in,
    void *dep
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(out);
    BAIL_ON_INVALID_PTR(in);

    status = SamrCopyLogonHours(&out->logon_hours, &in->logon_hours, dep);
    BAIL_ON_NTSTATUS_ERROR(status);

cleanup:
    return status;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyUserInfo5(
    UserInfo5 *out,
    UserInfo5 *in,
    void *dep
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(out);
    BAIL_ON_INVALID_PTR(in);

    status = SamrCopyUnicodeString(&out->account_name, &in->account_name, dep);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrCopyUnicodeString(&out->full_name, &in->full_name, dep);
    BAIL_ON_NTSTATUS_ERROR(status);

    out->rid         = in->rid;
    out->primary_gid = in->primary_gid;

    status = SamrCopyUnicodeString(&out->home_directory, &in->home_directory, dep);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrCopyUnicodeString(&out->home_drive, &in->home_drive, dep);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrCopyUnicodeString(&out->logon_script, &in->logon_script, dep);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrCopyUnicodeString(&out->profile_path, &in->profile_path, dep);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrCopyUnicodeString(&out->description, &in->description, dep);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrCopyUnicodeString(&out->workstations, &in->workstations, dep);
    BAIL_ON_NTSTATUS_ERROR(status);

    out->last_logon   = in->last_logon;
    out->last_logoff  = in->last_logoff;

    status = SamrCopyLogonHours(&out->logon_hours, &in->logon_hours, dep);
    BAIL_ON_NTSTATUS_ERROR(status);

    out->bad_password_count   = in->bad_password_count;
    out->logon_count          = in->logon_count;
    out->last_password_change = in->last_password_change;
    out->account_expiry       = in->account_expiry;
    out->account_flags        = in->account_flags;

cleanup:
    return status;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyUserInfo6(
    UserInfo6 *out,
    UserInfo6 *in,
    void *dep
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(out);
    BAIL_ON_INVALID_PTR(in);

    status = SamrCopyUnicodeString(&out->account_name, &in->account_name, dep);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrCopyUnicodeString(&out->full_name, &in->full_name, dep);
    BAIL_ON_NTSTATUS_ERROR(status);

cleanup:
    return status;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyUserInfo7(
    UserInfo7 *out,
    UserInfo7 *in,
    void *dep
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(out);
    BAIL_ON_INVALID_PTR(in);

    status = SamrCopyUnicodeString(&out->account_name, &in->account_name, dep);
    BAIL_ON_NTSTATUS_ERROR(status);

cleanup:
    return status;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyUserInfo8(
    UserInfo8 *out,
    UserInfo8 *in,
    void *dep
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(out);
    BAIL_ON_INVALID_PTR(in);

    status = SamrCopyUnicodeString(&out->full_name, &in->full_name, dep);
    BAIL_ON_NTSTATUS_ERROR(status);

cleanup:
    return status;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyUserInfo9(
    UserInfo9 *out,
    UserInfo9 *in,
    void *dep
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(out);
    BAIL_ON_INVALID_PTR(in);

    out->primary_gid = in->primary_gid;

cleanup:
    return status;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyUserInfo10(
    UserInfo10 *out,
    UserInfo10 *in,
    void *dep
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(out);
    BAIL_ON_INVALID_PTR(in);

    status = SamrCopyUnicodeString(&out->home_directory, &in->home_directory,
                                   dep);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrCopyUnicodeString(&out->home_drive, &in->home_drive, dep);
    BAIL_ON_NTSTATUS_ERROR(status);

cleanup:
    return status;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyUserInfo11(
    UserInfo11 *out,
    UserInfo11 *in,
    void *dep
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(out);
    BAIL_ON_INVALID_PTR(in);

    status = SamrCopyUnicodeString(&out->logon_script, &in->logon_script, dep);
    BAIL_ON_NTSTATUS_ERROR(status);

cleanup:
    return status;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyUserInfo12(
    UserInfo12 *out,
    UserInfo12 *in,
    void *dep
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(out);
    BAIL_ON_INVALID_PTR(in);

    status = SamrCopyUnicodeString(&out->profile_path, &in->profile_path, dep);
    BAIL_ON_NTSTATUS_ERROR(status);

cleanup:
    return status;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyUserInfo13(
    UserInfo13 *out,
    UserInfo13 *in,
    void *dep
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(out);
    BAIL_ON_INVALID_PTR(in);

    status = SamrCopyUnicodeString(&out->description, &in->description, dep);
    BAIL_ON_NTSTATUS_ERROR(status);

cleanup:
    return status;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyUserInfo14(
    UserInfo14 *out,
    UserInfo14 *in,
    void *dep
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(out);
    BAIL_ON_INVALID_PTR(in);

    status = SamrCopyUnicodeString(&out->workstations, &in->workstations, dep);
    BAIL_ON_NTSTATUS_ERROR(status);

cleanup:
    return status;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyUserInfo16(
    UserInfo16 *out,
    UserInfo16 *in,
    void *dep
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(out);
    BAIL_ON_INVALID_PTR(in);

    out->account_flags = in->account_flags;

cleanup:
    return status;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyUserInfo17(
    UserInfo17 *out,
    UserInfo17 *in,
    void *dep
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(out);
    BAIL_ON_INVALID_PTR(in);

    out->account_expiry = in->account_expiry;

cleanup:
    return status;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyUserInfo20(
    UserInfo20 *out,
    UserInfo20 *in,
    void *dep
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(out);
    BAIL_ON_INVALID_PTR(in);

    status = SamrCopyUnicodeString(&out->parameters, &in->parameters, dep);
    BAIL_ON_NTSTATUS_ERROR(status);

cleanup:
    return status;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyUserInfo21(
    UserInfo21 *out,
    UserInfo21 *in,
    void *dep
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(out);
    BAIL_ON_INVALID_PTR(in);

    out->last_logon            = in->last_logon;
    out->last_logoff           = in->last_logoff;
    out->last_password_change  = in->last_password_change;
    out->account_expiry        = in->account_expiry;
    out->allow_password_change = in->allow_password_change;
    out->force_password_change = in->force_password_change;

    status = SamrCopyUnicodeString(&out->account_name, &in->account_name, dep);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrCopyUnicodeString(&out->full_name, &in->full_name, dep);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrCopyUnicodeString(&out->home_directory, &in->home_directory, dep);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrCopyUnicodeString(&out->home_drive, &in->home_drive, dep);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrCopyUnicodeString(&out->logon_script, &in->logon_script, dep);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrCopyUnicodeString(&out->profile_path, &in->profile_path, dep);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrCopyUnicodeString(&out->description, &in->description, dep);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrCopyUnicodeString(&out->workstations, &in->workstations, dep);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrCopyUnicodeString(&out->comment, &in->comment, dep);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrCopyUnicodeString(&out->parameters, &in->parameters, dep);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrCopyUnicodeString(&out->unknown1, &in->unknown1, dep);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrCopyUnicodeString(&out->unknown2, &in->unknown2, dep);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrCopyUnicodeString(&out->unknown3, &in->unknown2, dep);
    BAIL_ON_NTSTATUS_ERROR(status);

    out->buf_count = in->buf_count;

    status = SamrAllocateMemory((void**)&out->buffer, out->buf_count, dep);
    BAIL_ON_NTSTATUS_ERROR(status);

    memcpy((void*)out->buffer, (void*)in->buffer, out->buf_count);

    out->rid            = in->rid;
    out->primary_gid    = in->primary_gid;
    out->account_flags  = in->account_flags;
    out->fields_present = in->fields_present;

    status = SamrCopyLogonHours(&out->logon_hours, &in->logon_hours, dep);
    BAIL_ON_NTSTATUS_ERROR(status);

    out->bad_password_count = in->bad_password_count;
    out->logon_count        = in->logon_count;
    out->country_code       = in->country_code;
    out->code_page          = in->code_page;
    out->nt_password_set    = in->nt_password_set;
    out->lm_password_set    = in->lm_password_set;
    out->password_expired   = in->password_expired;
    out->unknown4           = in->unknown4;

cleanup:
    return status;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyCryptPassword(
    CryptPassword *out,
    CryptPassword *in,
    void *dep
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(out);
    BAIL_ON_INVALID_PTR(in);

    memcpy((void*)out->data, (void*)in->data, sizeof(out->data));

cleanup:
    return status;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyUserInfo23(
    UserInfo23 *out,
    UserInfo23 *in,
    void *dep
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(out);
    BAIL_ON_INVALID_PTR(in);

    status = SamrCopyUserInfo21(&out->info, &in->info, dep);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrCopyCryptPassword(&out->password, &in->password, dep);
    BAIL_ON_NTSTATUS_ERROR(status);

cleanup:
    return status;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyUserInfo24(
    UserInfo24 *out,
    UserInfo24 *in,
    void *dep
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(out);
    BAIL_ON_INVALID_PTR(in);

    status = SamrCopyCryptPassword(&out->password, &in->password, dep);
    BAIL_ON_NTSTATUS_ERROR(status);

    out->password_len = in->password_len;

cleanup:
    return status;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyCryptPasswordEx(
    CryptPasswordEx *out,
    CryptPasswordEx *in,
    void *dep
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(out);
    BAIL_ON_INVALID_PTR(in);

    memcpy((void*)&out->data, (void*)&in->data, sizeof(out->data));

cleanup:
    return status;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyUserInfo25(
    UserInfo25 *out,
    UserInfo25 *in,
    void *dep
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(out);
    BAIL_ON_INVALID_PTR(in);

    status = SamrCopyUserInfo21(&out->info, &in->info, dep);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrCopyCryptPasswordEx(&out->password, &in->password, dep);
    BAIL_ON_NTSTATUS_ERROR(status);

cleanup:
    return status;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyUserInfo26(
    UserInfo26 *out,
    UserInfo26 *in,
    void *dep
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(out);
    BAIL_ON_INVALID_PTR(in);

    status = SamrCopyCryptPasswordEx(&out->password, &in->password, dep);
    BAIL_ON_NTSTATUS_ERROR(status);

    out->password_len = in->password_len;

cleanup:
    return status;

error:
    goto cleanup;
}


NTSTATUS
SamrAllocateUserInfo(
    UserInfo **out,
    UserInfo *in,
    uint16 level
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    UserInfo *ptr = NULL;

    BAIL_ON_INVALID_PTR(out);
    BAIL_ON_INVALID_PTR(in);

    status = SamrAllocateMemory((void*)&ptr, sizeof(UserInfo), NULL);
    BAIL_ON_NTSTATUS_ERROR(status);


    if (in != NULL) {
        switch (level) {
        case 1:
            status = SamrCopyUserInfo1(&ptr->info1, &in->info1, (void*)ptr);
            BAIL_ON_NTSTATUS_ERROR(status);
            break;
            
        case 2:
            status = SamrCopyUserInfo2(&ptr->info2, &in->info2, (void*)ptr);
            BAIL_ON_NTSTATUS_ERROR(status);
            break;

        case 3:
            status = SamrCopyUserInfo3(&ptr->info3, &in->info3, (void*)ptr);
            BAIL_ON_NTSTATUS_ERROR(status);
            break;

        case 4:
            status = SamrCopyUserInfo4(&ptr->info4, &in->info4, (void*)ptr);
            BAIL_ON_NTSTATUS_ERROR(status);
            break;

        case 5:
            status = SamrCopyUserInfo5(&ptr->info5, &in->info5, (void*)ptr);
            BAIL_ON_NTSTATUS_ERROR(status);
            break;

        case 6:
            status = SamrCopyUserInfo6(&ptr->info6, &in->info6, (void*)ptr);
            BAIL_ON_NTSTATUS_ERROR(status);
            break;

        case 7:
            status = SamrCopyUserInfo7(&ptr->info7, &in->info7, (void*)ptr);
            BAIL_ON_NTSTATUS_ERROR(status);
            break;

        case 8:
            status = SamrCopyUserInfo8(&ptr->info8, &in->info8, (void*)ptr);
            BAIL_ON_NTSTATUS_ERROR(status);
            break;

        case 9:
            status = SamrCopyUserInfo9(&ptr->info9, &in->info9, (void*)ptr);
            BAIL_ON_NTSTATUS_ERROR(status);
            break;

        case 10:
            status = SamrCopyUserInfo10(&ptr->info10, &in->info10, (void*)ptr);
            BAIL_ON_NTSTATUS_ERROR(status);
            break;

        case 11:
            status = SamrCopyUserInfo11(&ptr->info11, &in->info11,
                                        (void*)ptr);
            BAIL_ON_NTSTATUS_ERROR(status);
            break;

        case 12:
            status = SamrCopyUserInfo12(&ptr->info12, &in->info12,
                                          (void*)ptr);
            BAIL_ON_NTSTATUS_ERROR(status);
            break;

        case 13:
            status = SamrCopyUserInfo13(&ptr->info13, &in->info13,
                                          (void*)ptr);
            BAIL_ON_NTSTATUS_ERROR(status);
            break;

        case 14:
            status = SamrCopyUserInfo14(&ptr->info14, &in->info14,
                                          (void*)ptr);
            BAIL_ON_NTSTATUS_ERROR(status);
            break;

        case 16:
            status = SamrCopyUserInfo16(&ptr->info16, &in->info16,
                                          (void*)ptr);
            BAIL_ON_NTSTATUS_ERROR(status);
            break;

        case 17:
            status = SamrCopyUserInfo17(&ptr->info17, &in->info17,
                                          (void*)ptr);
            BAIL_ON_NTSTATUS_ERROR(status);
            break;

        case 20:
            status = SamrCopyUserInfo20(&ptr->info20, &in->info20,
                                          (void*)ptr);
            BAIL_ON_NTSTATUS_ERROR(status);
            break;

        case 21:
            status = SamrCopyUserInfo21(&ptr->info21, &in->info21,
                                          (void*)ptr);
            BAIL_ON_NTSTATUS_ERROR(status);
            break;

        case 23:
            status = SamrCopyUserInfo23(&ptr->info23, &in->info23,
                                          (void*)ptr);
            BAIL_ON_NTSTATUS_ERROR(status);
            break;

        case 24:
            status = SamrCopyUserInfo24(&ptr->info24, &in->info24,
                                          (void*)ptr);
            BAIL_ON_NTSTATUS_ERROR(status);
            break;

        case 25:
            status = SamrCopyUserInfo25(&ptr->info25, &in->info25,
                                          (void*)ptr);
            BAIL_ON_NTSTATUS_ERROR(status);
            break;

        case 26:
            status = SamrCopyUserInfo26(&ptr->info26, &in->info26,
                                          (void*)ptr);
            BAIL_ON_NTSTATUS_ERROR(status);
            break;

        default:
            status = STATUS_INVALID_LEVEL;
            goto error;
        }
    }

    *out = ptr;

cleanup:
    return status;

error:
    if (ptr) {
        SamrFreeMemory((void*)ptr);
    }

    *out = NULL;
    goto cleanup;
}


static
NTSTATUS
SamrCopyDisplayEntryFull(
    SamrDisplayEntryFull *out,
    SamrDisplayEntryFull *in,
    void *dep
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(out);
    BAIL_ON_INVALID_PTR(in);

    out->idx           = in->idx;
    out->rid           = in->rid;
    out->account_flags = in->account_flags;

    status = SamrCopyUnicodeString(&out->account_name, &in->account_name, dep);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrCopyUnicodeString(&out->description, &in->description, dep);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrCopyUnicodeString(&out->full_name, &in->full_name, dep);
    BAIL_ON_NTSTATUS_ERROR(status);

cleanup:
    return status;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyDisplayInfoFull(
    SamrDisplayInfoFull *out,
    SamrDisplayInfoFull *in,
    void *dep
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    uint32 i = 0;

    BAIL_ON_INVALID_PTR(out);
    BAIL_ON_INVALID_PTR(in);

    out->count = in->count;

    status = SamrAllocateMemory((void**)&out->entries,
                                sizeof(out->entries[0]) * out->count,
                                dep);
    BAIL_ON_NTSTATUS_ERROR(status);

    for (i = 0; i < out->count; i++) {
        status = SamrCopyDisplayEntryFull(&(out->entries[i]),
                                          &(in->entries[i]),
                                          out->entries);
        BAIL_ON_NTSTATUS_ERROR(status);
    }

cleanup:
    return status;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyDisplayEntryGeneral(
    SamrDisplayEntryGeneral *out,
    SamrDisplayEntryGeneral *in,
    void *dep
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(out);
    BAIL_ON_INVALID_PTR(in);

    out->idx           = in->idx;
    out->rid           = in->rid;
    out->account_flags = in->account_flags;

    status = SamrCopyUnicodeString(&out->account_name, &in->account_name, dep);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrCopyUnicodeString(&out->description, &in->description, dep);
    BAIL_ON_NTSTATUS_ERROR(status);

cleanup:
    return status;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyDisplayInfoGeneral(
    SamrDisplayInfoGeneral *out,
    SamrDisplayInfoGeneral *in,
    void *dep
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    uint32 i = 0;

    BAIL_ON_INVALID_PTR(out);
    BAIL_ON_INVALID_PTR(in);

    out->count = in->count;

    status = SamrAllocateMemory((void**)&out->entries,
                                sizeof(out->entries[0]) * out->count,
                                dep);
    BAIL_ON_NTSTATUS_ERROR(status);

    for (i = 0; i < out->count; i++) {
        status = SamrCopyDisplayEntryGeneral(&(out->entries[i]),
                                             &(in->entries[i]),
                                             out->entries);
        BAIL_ON_NTSTATUS_ERROR(status);
    }

cleanup:
    return status;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyDisplayEntryGeneralGroup(
    SamrDisplayEntryGeneralGroup *out,
    SamrDisplayEntryGeneralGroup *in,
    void *dep
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(out);
    BAIL_ON_INVALID_PTR(in);

    out->idx           = in->idx;
    out->rid           = in->rid;
    out->account_flags = in->account_flags;

    status = SamrCopyUnicodeString(&out->account_name, &in->account_name, dep);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrCopyUnicodeString(&out->description, &in->description, dep);
    BAIL_ON_NTSTATUS_ERROR(status);

cleanup:
    return status;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyDisplayInfoGeneralGroups(
    SamrDisplayInfoGeneralGroups *out,
    SamrDisplayInfoGeneralGroups *in,
    void *dep
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    uint32 i = 0;

    BAIL_ON_INVALID_PTR(out);
    BAIL_ON_INVALID_PTR(in);

    out->count = in->count;

    status = SamrAllocateMemory((void**)&out->entries,
                                sizeof(out->entries[0]) * out->count,
                                dep);
    BAIL_ON_NTSTATUS_ERROR(status);

    for (i = 0; i < out->count; i++) {
        status = SamrCopyDisplayEntryGeneralGroup(&(out->entries[i]),
                                                  &(in->entries[i]),
                                                  out->entries);
        BAIL_ON_NTSTATUS_ERROR(status);
    }

cleanup:
    return status;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyDisplayEntryAscii(
    SamrDisplayEntryAscii *out,
    SamrDisplayEntryAscii *in,
    void *dep
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(out);
    BAIL_ON_INVALID_PTR(in);

    out->idx                        = in->idx;
    out->account_name.Length        = in->account_name.Length;
    out->account_name.MaximumLength = in->account_name.MaximumLength;

    status = SamrAllocateMemory((void**)&out->account_name.Buffer,
                                sizeof(CHAR) * out->account_name.MaximumLength,
                                dep);
    BAIL_ON_NTSTATUS_ERROR(status);

    memcpy(out->account_name.Buffer,
           in->account_name.Buffer,
           out->account_name.Length);

cleanup:
    return status;

error:
    goto cleanup;
}


static
NTSTATUS
SamrCopyDisplayInfoAscii(
    SamrDisplayInfoAscii *out,
    SamrDisplayInfoAscii *in,
    void *dep
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    uint32 i = 0;

    BAIL_ON_INVALID_PTR(out);
    BAIL_ON_INVALID_PTR(in);

    out->count = in->count;

    status = SamrAllocateMemory((void**)&out->entries,
                                sizeof(out->entries[0]) * out->count,
                                dep);
    BAIL_ON_NTSTATUS_ERROR(status);

    for (i = 0; i < out->count; i++) {
        status = SamrCopyDisplayEntryAscii(&(out->entries[i]),
                                           &(in->entries[i]),
                                           out->entries);
        BAIL_ON_NTSTATUS_ERROR(status);
    }

cleanup:
    return status;

error:
    goto cleanup;
}


NTSTATUS
SamrAllocateDisplayInfo(
    SamrDisplayInfo **out,
    SamrDisplayInfo *in,
    uint16 level
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    SamrDisplayInfo *ptr = NULL;

    BAIL_ON_INVALID_PTR(out);

    status = SamrAllocateMemory((void**)&ptr, sizeof(*ptr),
                                NULL);
    BAIL_ON_NTSTATUS_ERROR(status);

    if (in == NULL) goto cleanup;

    switch (level) {
    case 1:
        status = SamrCopyDisplayInfoFull(&ptr->info1,
                                         &in->info1,
                                         ptr);
        break;

    case 2:
        status = SamrCopyDisplayInfoGeneral(&ptr->info2,
                                            &in->info2,
                                            ptr);
        break;

    case 3:
        status = SamrCopyDisplayInfoGeneralGroups(&ptr->info3,
                                                  &in->info3,
                                                  ptr);
        break;

    case 4:
        status = SamrCopyDisplayInfoAscii(&ptr->info4,
                                          &in->info4,
                                          ptr);
        break;

    case 5:
        status = SamrCopyDisplayInfoAscii(&ptr->info5,
                                          &in->info5,
                                          ptr);

    default:
        status = STATUS_INVALID_INFO_CLASS;
        goto error;
    }

    *out = ptr;

cleanup:
    return status;

error:
    if (ptr) {
        SamrFreeMemory((void*)ptr);
    }

    *out = NULL;
    goto cleanup;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
