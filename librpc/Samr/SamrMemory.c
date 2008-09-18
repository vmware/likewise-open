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


NTSTATUS SamrInitMemory()
{
    NTSTATUS status = STATUS_SUCCESS;
    int locked = 0;

    GLOBAL_DATA_LOCK(locked);

    if (!bSamrInitialised) {
        status = MemPtrListInit((PtrList**)&samr_ptr_list);
        goto_if_ntstatus_not_success(status, done);

        bSamrInitialised = 1;
    }
done:
    GLOBAL_DATA_UNLOCK(locked);

    return status;
}


NTSTATUS SamrDestroyMemory()
{
    NTSTATUS status = STATUS_SUCCESS;
    int locked = 0;

    GLOBAL_DATA_LOCK(locked);

    if (bSamrInitialised && samr_ptr_list) {
        status = MemPtrListDestroy((PtrList**)&samr_ptr_list);
        goto_if_ntstatus_not_success(status, done);

        bSamrInitialised = 0;
    }

done:
    GLOBAL_DATA_UNLOCK(locked);

    return status;
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

    goto_if_invalid_param_ntstatus(outn, cleanup);
    goto_if_invalid_param_ntstatus(outr, cleanup);
    goto_if_invalid_param_ntstatus(in, cleanup);

    status = SamrAllocateMemory((void**)&ptrn,
                                sizeof(wchar16_t*) * in->count,
                                NULL);
    goto_if_ntstatus_not_success(status, error);

    status = SamrAllocateMemory((void**)&ptrr,
                                sizeof(uint32) * in->count,
                                NULL);
    goto_if_ntstatus_not_success(status, error);

    for (i = 0; i < in->count; i++) {
        RidName *rn = &(in->entries[i]);
        
        ptrr[i]  = rn->rid;
        ptrn[i] = GetFromUnicodeString(&rn->name);
        goto_if_no_memory_ntstatus(ptrn[i], error);
        
        status = SamrAddDepMemory((void*)ptrn[i], (void*)ptrn);
        goto_if_ntstatus_not_success(status, error);
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

    goto_if_invalid_param_ntstatus(out, cleanup);
    goto_if_invalid_param_ntstatus(in, cleanup);

    status = SamrAllocateMemory((void**)&ptr,
                                sizeof(wchar16_t*) * in->count,
                                NULL);
    goto_if_ntstatus_not_success(status, error);

    for (i = 0; i < in->count; i++) {
        Entry *e = &(in->entries[i]);

        ptr[i] = GetFromUnicodeString(&e->name);
        goto_if_no_memory_ntstatus(ptr[i], error);

        status = SamrAddDepMemory((void*)ptr[i], (void*)ptr);
        goto_if_ntstatus_not_success(status, error);
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

    goto_if_invalid_param_ntstatus(out, cleanup);
    goto_if_invalid_param_ntstatus(in, cleanup);

    status = SamrAllocateMemory((void**)&ptr,
                                sizeof(uint32) * in->count,
                                NULL);
    goto_if_ntstatus_not_success(status, error);

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


NTSTATUS SamrAllocateDomSid(DomSid **out, DomSid *in, void *dep)
{
    NTSTATUS status = STATUS_SUCCESS;
    DomSid *ptr = NULL;

    goto_if_invalid_param_ntstatus(out, cleanup);

    SidCopyAlloc(&ptr, in);
    goto_if_no_memory_ntstatus(ptr, error);

    status = SamrAddDepMemory((void*)ptr, (void*)dep);
    goto_if_ntstatus_not_success(status, error);

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


NTSTATUS SamrAllocateSids(DomSid ***out, SidArray *in)
{
    NTSTATUS status = STATUS_SUCCESS;
    DomSid **ptr = NULL;
    int i = 0;

    goto_if_invalid_param_ntstatus(out, cleanup);
    goto_if_invalid_param_ntstatus(in, cleanup);

    status = SamrAllocateMemory((void**)&ptr,
                                sizeof(DomSid*) * in->num_sids,
                                NULL);
    goto_if_ntstatus_not_success(status, error);

    for (i = 0; i < in->num_sids; i++) {
        DomSid *sid = in->sids[i].sid;

        status = SamrAllocateDomSid(&(ptr[i]), sid, (void*)ptr);
        goto_if_ntstatus_not_success(status, error);
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

    goto_if_invalid_param_ntstatus(out_rids, cleanup);
    goto_if_invalid_param_ntstatus(out_attrs, cleanup);
    goto_if_invalid_param_ntstatus(in, cleanup);

    status = SamrAllocateMemory((void**)&ptr_rids,
                                sizeof(uint32) * in->count,
                                NULL);
    goto_if_ntstatus_not_success(status, error);

    status = SamrAllocateMemory((void**)&ptr_attrs,
                                sizeof(uint32) * in->count,
                                NULL);
    goto_if_ntstatus_not_success(status, error);

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
NTSTATUS SamrCopyUnicodeString(UnicodeString *out, UnicodeString *in,
                               void *dep)
{
    NTSTATUS status = STATUS_SUCCESS;

    goto_if_invalid_param_ntstatus(out, cleanup);
    goto_if_invalid_param_ntstatus(in, cleanup);

    status = CopyUnicodeString(out, in);
    goto_if_ntstatus_not_success(status, cleanup);

    if (out->string) {
        status = SamrAddDepMemory((void*)out->string, dep);
        goto_if_ntstatus_not_success(status, cleanup);
    }

cleanup:
    return status;
}


NTSTATUS SamrAllocateAliasInfo(AliasInfo **out, AliasInfo *in, uint16 level)
{
    NTSTATUS status = STATUS_SUCCESS;
    AliasInfo *ptr = NULL;

    goto_if_invalid_param_ntstatus(out, cleanup);
    goto_if_invalid_param_ntstatus(in, cleanup);

    status = SamrAllocateMemory((void*)&ptr, sizeof(AliasInfo), NULL);
    goto_if_ntstatus_not_success(status, error);

    switch (level) {
    case ALIAS_INFO_ALL:
        status = SamrCopyUnicodeString(&ptr->all.name, &in->all.name, (void*)ptr);
        goto_if_ntstatus_not_success(status, error);

        status = SamrCopyUnicodeString(&ptr->all.description,
                                       &in->all.description,
                                       (void*)ptr);
        goto_if_ntstatus_not_success(status, error);

        ptr->all.num_members = in->all.num_members;
        break;

    case ALIAS_INFO_NAME:
        status = SamrCopyUnicodeString(&ptr->name, &in->name, (void*)ptr);
        goto_if_ntstatus_not_success(status, error);
        break;

    case ALIAS_INFO_DESCRIPTION:
        status = SamrCopyUnicodeString(&ptr->all.description,
                                       &in->all.description,
                                       (void*)ptr);
        goto_if_ntstatus_not_success(status, error);
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
NTSTATUS SamrCopyDomainInfo1(DomainInfo1 *out, DomainInfo1 *in, void *dep)
{
    NTSTATUS status = STATUS_SUCCESS;

    goto_if_invalid_param_ntstatus(out, cleanup);
    goto_if_invalid_param_ntstatus(in, cleanup);

    memcpy((void*)out, (void*)in, sizeof(DomainInfo1));

cleanup:
    return status;
}


static
NTSTATUS SamrCopyDomainInfo2(DomainInfo2 *out, DomainInfo2 *in, void *dep)
{
    NTSTATUS status = STATUS_SUCCESS;

    goto_if_invalid_param_ntstatus(out, cleanup);
    goto_if_invalid_param_ntstatus(in, cleanup);

    status = CopyUnicodeString(&out->comment, &in->comment);
    goto_if_ntstatus_not_success(status, cleanup);

    if (out->comment.string) {
        status = SamrAddDepMemory((void*)out->comment.string, dep);
        goto_if_ntstatus_not_success(status, cleanup);
    }

    status = CopyUnicodeString(&out->domain_name, &in->domain_name);
    goto_if_ntstatus_not_success(status, cleanup);

    if (out->domain_name.string) {
        status = SamrAddDepMemory((void*)out->domain_name.string, dep);
        goto_if_ntstatus_not_success(status, cleanup);
    }

    status = CopyUnicodeString(&out->primary, &in->primary);
    goto_if_ntstatus_not_success(status, cleanup);

    if (out->primary.string) {
        status = SamrAddDepMemory((void*)out->primary.string, dep);
        goto_if_ntstatus_not_success(status, cleanup);
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
}


static
NTSTATUS SamrCopyDomainInfo3(DomainInfo3 *out, DomainInfo3 *in, void *dep)
{
    NTSTATUS status = STATUS_SUCCESS;

    goto_if_invalid_param_ntstatus(out, cleanup);
    goto_if_invalid_param_ntstatus(in, cleanup);

    out->force_logoff_time = in->force_logoff_time;

cleanup:
    return status;
}


static
NTSTATUS SamrCopyDomainInfo4(DomainInfo4 *out, DomainInfo4 *in, void *dep)
{
    return SamrCopyUnicodeString(&out->comment, &in->comment, dep);
}


static
NTSTATUS SamrCopyDomainInfo5(DomainInfo5 *out, DomainInfo5 *in, void *dep)
{
    return SamrCopyUnicodeString(&out->domain_name, &in->domain_name, dep);
}


static
NTSTATUS SamrCopyDomainInfo6(DomainInfo6 *out, DomainInfo6 *in, void *dep)
{
    return SamrCopyUnicodeString(&out->primary, &in->primary, dep);
}


static
NTSTATUS SamrCopyDomainInfo7(DomainInfo7 *out, DomainInfo7 *in, void *dep)
{
    NTSTATUS status = STATUS_SUCCESS;

    goto_if_invalid_param_ntstatus(out, cleanup);
    goto_if_invalid_param_ntstatus(in, cleanup);

    out->role = in->role;

cleanup:
    return status;
}


static
NTSTATUS SamrCopyDomainInfo8(DomainInfo8 *out, DomainInfo8 *in, void *dep)
{
    NTSTATUS status = STATUS_SUCCESS;

    goto_if_invalid_param_ntstatus(out, cleanup);
    goto_if_invalid_param_ntstatus(in, cleanup);

    out->sequence_number    = in->sequence_number;
    out->domain_create_time = in->domain_create_time;

cleanup:
    return status;
}


static
NTSTATUS SamrCopyDomainInfo9(DomainInfo9 *out, DomainInfo9 *in, void *dep)
{
    NTSTATUS status = STATUS_SUCCESS;

    goto_if_invalid_param_ntstatus(out, cleanup);
    goto_if_invalid_param_ntstatus(in, cleanup);

    out->unknown = in->unknown;

cleanup:
    return status;
}


static
NTSTATUS SamrCopyDomainInfo11(DomainInfo11 *out, DomainInfo11 *in, void *dep)
{
    NTSTATUS status = STATUS_SUCCESS;

    goto_if_invalid_param_ntstatus(out, cleanup);
    goto_if_invalid_param_ntstatus(in, cleanup);

    status = SamrCopyDomainInfo2(&out->info2, &in->info2, dep);
    goto_if_ntstatus_not_success(status, cleanup);

    out->lockout_duration  = in->lockout_duration;
    out->lockout_window    = in->lockout_window;
    out->lockout_threshold = in->lockout_threshold;

cleanup:
    return status;
}


static
NTSTATUS SamrCopyDomainInfo12(DomainInfo12 *out, DomainInfo12 *in, void *dep)
{
    NTSTATUS status = STATUS_SUCCESS;

    goto_if_invalid_param_ntstatus(out, cleanup);
    goto_if_invalid_param_ntstatus(in, cleanup);

    out->lockout_duration  = in->lockout_duration;
    out->lockout_window    = in->lockout_window;
    out->lockout_threshold = in->lockout_threshold;

cleanup:
    return status;
}


static
NTSTATUS SamrCopyDomainInfo13(DomainInfo13 *out, DomainInfo13 *in, void *dep)
{
    NTSTATUS status = STATUS_SUCCESS;

    goto_if_invalid_param_ntstatus(out, cleanup);
    goto_if_invalid_param_ntstatus(in, cleanup);

    out->sequence_number    = in->sequence_number;
    out->domain_create_time = in->domain_create_time;
    out->unknown1           = in->unknown1;
    out->unknown2           = in->unknown2;

cleanup:
    return status;
}


NTSTATUS SamrAllocateDomainInfo(DomainInfo **out, DomainInfo *in, uint16 level)
{
    NTSTATUS status = STATUS_SUCCESS;
    DomainInfo *ptr = NULL;

    goto_if_invalid_param_ntstatus(out, cleanup);
    goto_if_invalid_param_ntstatus(in, cleanup);

    status = SamrAllocateMemory((void*)&ptr, sizeof(DomainInfo), NULL);
    goto_if_ntstatus_not_success(status, error);

    if (in != NULL) {
        switch (level) {
        case 1:
            status = SamrCopyDomainInfo1(&ptr->info1, &in->info1, (void*)ptr);
            goto_if_ntstatus_not_success(status, error);
            break;

        case 2:
            status = SamrCopyDomainInfo2(&ptr->info2, &in->info2, (void*)ptr);
            goto_if_ntstatus_not_success(status, error);
            break;

        case 3:
            status = SamrCopyDomainInfo3(&ptr->info3, &in->info3, (void*)ptr);
            goto_if_ntstatus_not_success(status, error);
            break;

        case 4:
            status = SamrCopyDomainInfo4(&ptr->info4, &in->info4, (void*)ptr);
            goto_if_ntstatus_not_success(status, error);
            break;

        case 5:
            status = SamrCopyDomainInfo5(&ptr->info5, &in->info5, (void*)ptr);
            goto_if_ntstatus_not_success(status, error);
            break;

        case 6:
            status = SamrCopyDomainInfo6(&ptr->info6, &in->info6, (void*)ptr);
            goto_if_ntstatus_not_success(status, error);
            break;

        case 7:
            status = SamrCopyDomainInfo7(&ptr->info7, &in->info7, (void*)ptr);
            goto_if_ntstatus_not_success(status, error);
            break;

        case 8:
            status = SamrCopyDomainInfo8(&ptr->info8, &in->info8, (void*)ptr);
            goto_if_ntstatus_not_success(status, error);
            break;

        case 9:
            status = SamrCopyDomainInfo9(&ptr->info9, &in->info9, (void*)ptr);
            goto_if_ntstatus_not_success(status, error);
            break;

        case 11:
            status = SamrCopyDomainInfo11(&ptr->info11, &in->info11,
                                          (void*)ptr);
            goto_if_ntstatus_not_success(status, error);
            break;

        case 12:
            status = SamrCopyDomainInfo12(&ptr->info12, &in->info12,
                                          (void*)ptr);
            goto_if_ntstatus_not_success(status, error);
            break;

        case 13:
            status = SamrCopyDomainInfo13(&ptr->info13, &in->info13,
                                          (void*)ptr);
            goto_if_ntstatus_not_success(status, error);
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
NTSTATUS SamrCopyUserInfo1(UserInfo1 *out, UserInfo1 *in, void *dep)
{
    NTSTATUS status = STATUS_SUCCESS;

    goto_if_invalid_param_ntstatus(out, cleanup);
    goto_if_invalid_param_ntstatus(in, cleanup);

    status = SamrCopyUnicodeString(&out->account_name, &in->account_name, dep);
    goto_if_ntstatus_not_success(status, cleanup);

    status = SamrCopyUnicodeString(&out->full_name, &in->full_name, dep);
    goto_if_ntstatus_not_success(status, cleanup);

    status = SamrCopyUnicodeString(&out->description, &in->description, dep);
    goto_if_ntstatus_not_success(status, cleanup);

    status = SamrCopyUnicodeString(&out->comment, &in->comment, dep);
    goto_if_ntstatus_not_success(status, cleanup);

    out->primary_gid = in->primary_gid;

cleanup:
    return status;
}


static
NTSTATUS SamrCopyUserInfo2(UserInfo2 *out, UserInfo2 *in, void *dep)
{
    NTSTATUS status = STATUS_SUCCESS;

    goto_if_invalid_param_ntstatus(out, cleanup);
    goto_if_invalid_param_ntstatus(in, cleanup);

    status = SamrCopyUnicodeString(&out->comment, &in->comment, dep);
    goto_if_ntstatus_not_success(status, cleanup);

    status = SamrCopyUnicodeString(&out->unknown1, &in->unknown1, dep);
    goto_if_ntstatus_not_success(status, cleanup);

    out->country_code = in->country_code;
    out->code_page    = in->code_page;

cleanup:
    return status;
}


static
NTSTATUS SamrCopyLogonHours(LogonHours *out, LogonHours *in, void *dep)
{
    NTSTATUS status = STATUS_SUCCESS;

    goto_if_invalid_param_ntstatus(out, cleanup);
    goto_if_invalid_param_ntstatus(in, cleanup);

    /* Allocate 1260 bytes and copy units_per_week/8 bytes
       according to samr.idl */

    status = SamrAllocateMemory((void**)&out->units, sizeof(uint8) * 1260, NULL);
    goto_if_ntstatus_not_success(status, cleanup);

    memcpy((void*)out->units, (void*)in->units, in->units_per_week/8);

cleanup:
    return status;
}


static
NTSTATUS SamrCopyUserInfo3(UserInfo3 *out, UserInfo3 *in, void *dep)
{
    NTSTATUS status = STATUS_SUCCESS;

    goto_if_invalid_param_ntstatus(out, cleanup);
    goto_if_invalid_param_ntstatus(in, cleanup);

    status = SamrCopyUnicodeString(&out->account_name, &in->account_name, dep);
    goto_if_ntstatus_not_success(status, cleanup);

    status = SamrCopyUnicodeString(&out->full_name, &in->full_name, dep);
    goto_if_ntstatus_not_success(status, cleanup);

    out->rid         = in->rid;
    out->primary_gid = in->primary_gid;

    status = SamrCopyUnicodeString(&out->home_directory, &in->home_directory, dep);
    goto_if_ntstatus_not_success(status, cleanup);

    status = SamrCopyUnicodeString(&out->home_drive, &in->home_drive, dep);
    goto_if_ntstatus_not_success(status, cleanup);

    status = SamrCopyUnicodeString(&out->logon_script, &in->logon_script, dep);
    goto_if_ntstatus_not_success(status, cleanup);

    status = SamrCopyUnicodeString(&out->profile_path, &in->profile_path, dep);
    goto_if_ntstatus_not_success(status, cleanup);

    status = SamrCopyUnicodeString(&out->workstations, &in->workstations, dep);
    goto_if_ntstatus_not_success(status, cleanup);

    out->last_logon             = in->last_logon;
    out->last_logoff            = in->last_logoff;
    out->last_password_change   = in->last_password_change;
    out->allow_password_change  = in->allow_password_change;
    out->force_password_change  = in->force_password_change;

    status = SamrCopyLogonHours(&out->logon_hours, &in->logon_hours, dep);
    goto_if_ntstatus_not_success(status, cleanup);

    out->bad_password_count = in->bad_password_count;
    out->logon_count        = in->logon_count;
    out->account_flags      = in->account_flags;

cleanup:
    return status;
}


static
NTSTATUS SamrCopyUserInfo4(UserInfo4 *out, UserInfo4 *in, void *dep)
{
    NTSTATUS status = STATUS_SUCCESS;

    goto_if_invalid_param_ntstatus(out, cleanup);
    goto_if_invalid_param_ntstatus(in, cleanup);

    status = SamrCopyLogonHours(&out->logon_hours, &in->logon_hours, dep);
    goto_if_ntstatus_not_success(status, cleanup);

cleanup:
    return status;
}


static
NTSTATUS SamrCopyUserInfo5(UserInfo5 *out, UserInfo5 *in, void *dep)
{
    NTSTATUS status = STATUS_SUCCESS;

    goto_if_invalid_param_ntstatus(out, cleanup);
    goto_if_invalid_param_ntstatus(in, cleanup);

    status = SamrCopyUnicodeString(&out->account_name, &in->account_name, dep);
    goto_if_ntstatus_not_success(status, cleanup);

    status = SamrCopyUnicodeString(&out->full_name, &in->full_name, dep);
    goto_if_ntstatus_not_success(status, cleanup);

    out->rid         = in->rid;
    out->primary_gid = in->primary_gid;

    status = SamrCopyUnicodeString(&out->home_directory, &in->home_directory, dep);
    goto_if_ntstatus_not_success(status, cleanup);

    status = SamrCopyUnicodeString(&out->home_drive, &in->home_drive, dep);
    goto_if_ntstatus_not_success(status, cleanup);

    status = SamrCopyUnicodeString(&out->logon_script, &in->logon_script, dep);
    goto_if_ntstatus_not_success(status, cleanup);

    status = SamrCopyUnicodeString(&out->profile_path, &in->profile_path, dep);
    goto_if_ntstatus_not_success(status, cleanup);

    status = SamrCopyUnicodeString(&out->description, &in->description, dep);
    goto_if_ntstatus_not_success(status, cleanup);

    status = SamrCopyUnicodeString(&out->workstations, &in->workstations, dep);
    goto_if_ntstatus_not_success(status, cleanup);

    out->last_logon   = in->last_logon;
    out->last_logoff  = in->last_logoff;

    status = SamrCopyLogonHours(&out->logon_hours, &in->logon_hours, dep);
    goto_if_ntstatus_not_success(status, cleanup);

    out->bad_password_count   = in->bad_password_count;
    out->logon_count          = in->logon_count;
    out->last_password_change = in->last_password_change;
    out->account_expiry       = in->account_expiry;
    out->account_flags        = in->account_flags;

cleanup:
    return status;
}


static
NTSTATUS SamrCopyUserInfo6(UserInfo6 *out, UserInfo6 *in, void *dep)
{
    NTSTATUS status = STATUS_SUCCESS;

    goto_if_invalid_param_ntstatus(out, cleanup);
    goto_if_invalid_param_ntstatus(in, cleanup);

    status = SamrCopyUnicodeString(&out->account_name, &in->account_name, dep);
    goto_if_ntstatus_not_success(status, cleanup);

    status = SamrCopyUnicodeString(&out->full_name, &in->full_name, dep);
    goto_if_ntstatus_not_success(status, cleanup);

cleanup:
    return status;
}


static
NTSTATUS SamrCopyUserInfo7(UserInfo7 *out, UserInfo7 *in, void *dep)
{
    NTSTATUS status = STATUS_SUCCESS;

    goto_if_invalid_param_ntstatus(out, cleanup);
    goto_if_invalid_param_ntstatus(in, cleanup);

    status = SamrCopyUnicodeString(&out->account_name, &in->account_name, dep);
    goto_if_ntstatus_not_success(status, cleanup);

cleanup:
    return status;
}


static
NTSTATUS SamrCopyUserInfo8(UserInfo8 *out, UserInfo8 *in, void *dep)
{
    NTSTATUS status = STATUS_SUCCESS;

    goto_if_invalid_param_ntstatus(out, cleanup);
    goto_if_invalid_param_ntstatus(in, cleanup);

    status = SamrCopyUnicodeString(&out->full_name, &in->full_name, dep);
    goto_if_ntstatus_not_success(status, cleanup);

cleanup:
    return status;
}


static
NTSTATUS SamrCopyUserInfo9(UserInfo9 *out, UserInfo9 *in, void *dep)
{
    NTSTATUS status = STATUS_SUCCESS;

    goto_if_invalid_param_ntstatus(out, cleanup);
    goto_if_invalid_param_ntstatus(in, cleanup);

    out->primary_gid = in->primary_gid;

cleanup:
    return status;
}


static
NTSTATUS SamrCopyUserInfo10(UserInfo10 *out, UserInfo10 *in, void *dep)
{
    NTSTATUS status = STATUS_SUCCESS;

    goto_if_invalid_param_ntstatus(out, cleanup);
    goto_if_invalid_param_ntstatus(in, cleanup);

    status = SamrCopyUnicodeString(&out->home_directory, &in->home_directory,
                                   dep);
    goto_if_ntstatus_not_success(status, cleanup);

    status = SamrCopyUnicodeString(&out->home_drive, &in->home_drive, dep);
    goto_if_ntstatus_not_success(status, cleanup);

cleanup:
    return status;
}


static
NTSTATUS SamrCopyUserInfo11(UserInfo11 *out, UserInfo11 *in, void *dep)
{
    NTSTATUS status = STATUS_SUCCESS;

    goto_if_invalid_param_ntstatus(out, cleanup);
    goto_if_invalid_param_ntstatus(in, cleanup);

    status = SamrCopyUnicodeString(&out->logon_script, &in->logon_script, dep);
    goto_if_ntstatus_not_success(status, cleanup);

cleanup:
    return status;
}


static
NTSTATUS SamrCopyUserInfo12(UserInfo12 *out, UserInfo12 *in, void *dep)
{
    NTSTATUS status = STATUS_SUCCESS;

    goto_if_invalid_param_ntstatus(out, cleanup);
    goto_if_invalid_param_ntstatus(in, cleanup);

    status = SamrCopyUnicodeString(&out->profile_path, &in->profile_path, dep);
    goto_if_ntstatus_not_success(status, cleanup);

cleanup:
    return status;
}


static
NTSTATUS SamrCopyUserInfo13(UserInfo13 *out, UserInfo13 *in, void *dep)
{
    NTSTATUS status = STATUS_SUCCESS;

    goto_if_invalid_param_ntstatus(out, cleanup);
    goto_if_invalid_param_ntstatus(in, cleanup);

    status = SamrCopyUnicodeString(&out->description, &in->description, dep);
    goto_if_ntstatus_not_success(status, cleanup);

cleanup:
    return status;
}


static
NTSTATUS SamrCopyUserInfo14(UserInfo14 *out, UserInfo14 *in, void *dep)
{
    NTSTATUS status = STATUS_SUCCESS;

    goto_if_invalid_param_ntstatus(out, cleanup);
    goto_if_invalid_param_ntstatus(in, cleanup);

    status = SamrCopyUnicodeString(&out->workstations, &in->workstations, dep);
    goto_if_ntstatus_not_success(status, cleanup);

cleanup:
    return status;
}


static
NTSTATUS SamrCopyUserInfo16(UserInfo16 *out, UserInfo16 *in, void *dep)
{
    NTSTATUS status = STATUS_SUCCESS;

    goto_if_invalid_param_ntstatus(out, cleanup);
    goto_if_invalid_param_ntstatus(in, cleanup);

    out->account_flags = in->account_flags;

cleanup:
    return status;
}


static
NTSTATUS SamrCopyUserInfo17(UserInfo17 *out, UserInfo17 *in, void *dep)
{
    NTSTATUS status = STATUS_SUCCESS;

    goto_if_invalid_param_ntstatus(out, cleanup);
    goto_if_invalid_param_ntstatus(in, cleanup);

    out->account_expiry = in->account_expiry;

cleanup:
    return status;
}


static
NTSTATUS SamrCopyUserInfo20(UserInfo20 *out, UserInfo20 *in, void *dep)
{
    NTSTATUS status = STATUS_SUCCESS;

    goto_if_invalid_param_ntstatus(out, cleanup);
    goto_if_invalid_param_ntstatus(in, cleanup);

    status = SamrCopyUnicodeString(&out->parameters, &in->parameters, dep);
    goto_if_ntstatus_not_success(status, cleanup);

cleanup:
    return status;
}


static
NTSTATUS SamrCopyUserInfo21(UserInfo21 *out, UserInfo21 *in, void *dep)
{
    NTSTATUS status = STATUS_SUCCESS;

    goto_if_invalid_param_ntstatus(out, cleanup);
    goto_if_invalid_param_ntstatus(in, cleanup);

    out->last_logon            = in->last_logon;
    out->last_logoff           = in->last_logoff;
    out->last_password_change  = in->last_password_change;
    out->account_expiry        = in->account_expiry;
    out->allow_password_change = in->allow_password_change;
    out->force_password_change = in->force_password_change;

    status = SamrCopyUnicodeString(&out->account_name, &in->account_name, dep);
    goto_if_ntstatus_not_success(status, cleanup);

    status = SamrCopyUnicodeString(&out->full_name, &in->full_name, dep);
    goto_if_ntstatus_not_success(status, cleanup);

    status = SamrCopyUnicodeString(&out->home_directory, &in->home_directory, dep);
    goto_if_ntstatus_not_success(status, cleanup);

    status = SamrCopyUnicodeString(&out->home_drive, &in->home_drive, dep);
    goto_if_ntstatus_not_success(status, cleanup);

    status = SamrCopyUnicodeString(&out->logon_script, &in->logon_script, dep);
    goto_if_ntstatus_not_success(status, cleanup);

    status = SamrCopyUnicodeString(&out->profile_path, &in->profile_path, dep);
    goto_if_ntstatus_not_success(status, cleanup);

    status = SamrCopyUnicodeString(&out->description, &in->description, dep);
    goto_if_ntstatus_not_success(status, cleanup);

    status = SamrCopyUnicodeString(&out->workstations, &in->workstations, dep);
    goto_if_ntstatus_not_success(status, cleanup);

    status = SamrCopyUnicodeString(&out->comment, &in->comment, dep);
    goto_if_ntstatus_not_success(status, cleanup);

    status = SamrCopyUnicodeString(&out->parameters, &in->parameters, dep);
    goto_if_ntstatus_not_success(status, cleanup);

    status = SamrCopyUnicodeString(&out->unknown1, &in->unknown1, dep);
    goto_if_ntstatus_not_success(status, cleanup);

    status = SamrCopyUnicodeString(&out->unknown2, &in->unknown2, dep);
    goto_if_ntstatus_not_success(status, cleanup);

    status = SamrCopyUnicodeString(&out->unknown3, &in->unknown2, dep);
    goto_if_ntstatus_not_success(status, cleanup);

    out->buf_count = in->buf_count;

    status = SamrAllocateMemory((void**)&out->buffer, out->buf_count, dep);
    goto_if_ntstatus_not_success(status, cleanup);

    memcpy((void*)out->buffer, (void*)in->buffer, out->buf_count);

    out->rid            = in->rid;
    out->primary_gid    = in->primary_gid;
    out->account_flags  = in->account_flags;
    out->fields_present = in->fields_present;

    status = SamrCopyLogonHours(&out->logon_hours, &in->logon_hours, dep);
    goto_if_ntstatus_not_success(status, cleanup);

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
}


static
NTSTATUS SamrCopyCryptPassword(CryptPassword *out, CryptPassword *in, void *dep)
{
    NTSTATUS status = STATUS_SUCCESS;

    goto_if_invalid_param_ntstatus(out, cleanup);
    goto_if_invalid_param_ntstatus(in, cleanup);

    memcpy((void*)out->data, (void*)in->data, sizeof(out->data));

cleanup:
    return status;
}


static
NTSTATUS SamrCopyUserInfo23(UserInfo23 *out, UserInfo23 *in, void *dep)
{
    NTSTATUS status = STATUS_SUCCESS;

    goto_if_invalid_param_ntstatus(out, cleanup);
    goto_if_invalid_param_ntstatus(in, cleanup);

    status = SamrCopyUserInfo21(&out->info, &in->info, dep);
    goto_if_ntstatus_not_success(status, cleanup);

    status = SamrCopyCryptPassword(&out->password, &in->password, dep);
    goto_if_ntstatus_not_success(status, cleanup);

cleanup:
    return status;
}


static
NTSTATUS SamrCopyUserInfo24(UserInfo24 *out, UserInfo24 *in, void *dep)
{
    NTSTATUS status = STATUS_SUCCESS;

    goto_if_invalid_param_ntstatus(out, cleanup);
    goto_if_invalid_param_ntstatus(in, cleanup);

    status = SamrCopyCryptPassword(&out->password, &in->password, dep);
    goto_if_ntstatus_not_success(status, cleanup);

    out->password_len = in->password_len;

cleanup:
    return status;
}


static
NTSTATUS SamrCopyCryptPasswordEx(CryptPasswordEx *out, CryptPasswordEx *in,
                                 void *dep)
{
    NTSTATUS status = STATUS_SUCCESS;

    goto_if_invalid_param_ntstatus(out, cleanup);
    goto_if_invalid_param_ntstatus(in, cleanup);

    memcpy((void*)&out->data, (void*)&in->data, sizeof(out->data));

cleanup:
    return status;
}


static
NTSTATUS SamrCopyUserInfo25(UserInfo25 *out, UserInfo25 *in, void *dep)
{
    NTSTATUS status = STATUS_SUCCESS;

    goto_if_invalid_param_ntstatus(out, cleanup);
    goto_if_invalid_param_ntstatus(in, cleanup);

    status = SamrCopyUserInfo21(&out->info, &in->info, dep);
    goto_if_ntstatus_not_success(status, cleanup);

    status = SamrCopyCryptPasswordEx(&out->password, &in->password, dep);
    goto_if_ntstatus_not_success(status, cleanup);

cleanup:
    return status;
}


static
NTSTATUS SamrCopyUserInfo26(UserInfo26 *out, UserInfo26 *in, void *dep)
{
    NTSTATUS status = STATUS_SUCCESS;

    goto_if_invalid_param_ntstatus(out, cleanup);
    goto_if_invalid_param_ntstatus(in, cleanup);

    status = SamrCopyCryptPasswordEx(&out->password, &in->password, dep);
    goto_if_ntstatus_not_success(status, cleanup);

    out->password_len = in->password_len;

cleanup:
    return status;
}


NTSTATUS SamrAllocateUserInfo(UserInfo **out, UserInfo *in, uint16 level)
{
    NTSTATUS status = STATUS_SUCCESS;
    UserInfo *ptr = NULL;

    goto_if_invalid_param_ntstatus(out, cleanup);
    goto_if_invalid_param_ntstatus(in, cleanup);

    status = SamrAllocateMemory((void*)&ptr, sizeof(UserInfo), NULL);
    goto_if_ntstatus_not_success(status, error);


    if (in != NULL) {
        switch (level) {
        case 1:
            status = SamrCopyUserInfo1(&ptr->info1, &in->info1, (void*)ptr);
            goto_if_ntstatus_not_success(status, error);
            break;
            
        case 2:
            status = SamrCopyUserInfo2(&ptr->info2, &in->info2, (void*)ptr);
            goto_if_ntstatus_not_success(status, error);
            break;

        case 3:
            status = SamrCopyUserInfo3(&ptr->info3, &in->info3, (void*)ptr);
            goto_if_ntstatus_not_success(status, error);
            break;

        case 4:
            status = SamrCopyUserInfo4(&ptr->info4, &in->info4, (void*)ptr);
            goto_if_ntstatus_not_success(status, error);
            break;

        case 5:
            status = SamrCopyUserInfo5(&ptr->info5, &in->info5, (void*)ptr);
            goto_if_ntstatus_not_success(status, error);
            break;

        case 6:
            status = SamrCopyUserInfo6(&ptr->info6, &in->info6, (void*)ptr);
            goto_if_ntstatus_not_success(status, error);
            break;

        case 7:
            status = SamrCopyUserInfo7(&ptr->info7, &in->info7, (void*)ptr);
            goto_if_ntstatus_not_success(status, error);
            break;

        case 8:
            status = SamrCopyUserInfo8(&ptr->info8, &in->info8, (void*)ptr);
            goto_if_ntstatus_not_success(status, error);
            break;

        case 9:
            status = SamrCopyUserInfo9(&ptr->info9, &in->info9, (void*)ptr);
            goto_if_ntstatus_not_success(status, error);
            break;

        case 11:
            status = SamrCopyUserInfo11(&ptr->info11, &in->info11,
                                        (void*)ptr);
            goto_if_ntstatus_not_success(status, error);
            break;

        case 12:
            status = SamrCopyUserInfo12(&ptr->info12, &in->info12,
                                          (void*)ptr);
            goto_if_ntstatus_not_success(status, error);
            break;

        case 13:
            status = SamrCopyUserInfo13(&ptr->info13, &in->info13,
                                          (void*)ptr);
            goto_if_ntstatus_not_success(status, error);
            break;

        case 14:
            status = SamrCopyUserInfo14(&ptr->info14, &in->info14,
                                          (void*)ptr);
            goto_if_ntstatus_not_success(status, error);
            break;

        case 16:
            status = SamrCopyUserInfo16(&ptr->info16, &in->info16,
                                          (void*)ptr);
            goto_if_ntstatus_not_success(status, error);
            break;

        case 17:
            status = SamrCopyUserInfo17(&ptr->info17, &in->info17,
                                          (void*)ptr);
            goto_if_ntstatus_not_success(status, error);
            break;

        case 20:
            status = SamrCopyUserInfo20(&ptr->info20, &in->info20,
                                          (void*)ptr);
            goto_if_ntstatus_not_success(status, error);
            break;

        case 21:
            status = SamrCopyUserInfo21(&ptr->info21, &in->info21,
                                          (void*)ptr);
            goto_if_ntstatus_not_success(status, error);
            break;

        case 23:
            status = SamrCopyUserInfo23(&ptr->info23, &in->info23,
                                          (void*)ptr);
            goto_if_ntstatus_not_success(status, error);
            break;

        case 24:
            status = SamrCopyUserInfo24(&ptr->info24, &in->info24,
                                          (void*)ptr);
            goto_if_ntstatus_not_success(status, error);
            break;

        case 25:
            status = SamrCopyUserInfo25(&ptr->info25, &in->info25,
                                          (void*)ptr);
            goto_if_ntstatus_not_success(status, error);
            break;

        case 26:
            status = SamrCopyUserInfo26(&ptr->info26, &in->info26,
                                          (void*)ptr);
            goto_if_ntstatus_not_success(status, error);
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


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
