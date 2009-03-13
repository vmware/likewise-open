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
 * Abstract: Lsa memory (de)allocation routines (rpc client library)
 *
 * Authors: Rafal Szczesniak (rafal@likewisesoftware.com)
 */

#include "includes.h"


NTSTATUS
LsaRpcInitMemory()
{
    NTSTATUS status = STATUS_SUCCESS;
    int locked = 0;

    GLOBAL_DATA_LOCK(locked);

    if (!bLsaInitialised) {
        status = MemPtrListInit((PtrList**)&lsa_ptr_list);
        goto_if_ntstatus_not_success(status, done);

        bLsaInitialised = 1;
    }
done:
    GLOBAL_DATA_UNLOCK(locked);

    return status;
}


NTSTATUS
LsaRpcDestroyMemory()
{
    NTSTATUS status = STATUS_SUCCESS;
    int locked = 0;

    GLOBAL_DATA_LOCK(locked);

    if (bLsaInitialised && lsa_ptr_list) {
        status = MemPtrListDestroy((PtrList**)&lsa_ptr_list);
        goto_if_ntstatus_not_success(status, done);

        bLsaInitialised = 0;
    }

done:
    GLOBAL_DATA_UNLOCK(locked);

    return status;
}


NTSTATUS
LsaRpcAllocateMemory(
    void **out,
    size_t size,
    void *dep
    )
{
    return MemPtrAllocate((PtrList*)lsa_ptr_list, out, size, dep);
}


NTSTATUS
LsaRpcFreeMemory(
    void *ptr
    )
{
    return MemPtrFree((PtrList*)lsa_ptr_list, ptr);
}


NTSTATUS
LsaRpcAddDepMemory(
    void *ptr,
    void *dep
    )
{
    return MemPtrAddDependant((PtrList*)lsa_ptr_list, ptr, dep);
}


NTSTATUS
LsaAllocateTranslatedSids(
    TranslatedSid **out,
    TranslatedSidArray *in
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    TranslatedSid *ptr = NULL;
    int i = 0;
    int count = 0;

    goto_if_invalid_param_ntstatus(out, cleanup);

    count = (in == NULL) ? 1 : in->count;

    status = LsaRpcAllocateMemory((void**)&ptr,
                                  sizeof(TranslatedSid) * count,
                                  NULL);
    goto_if_ntstatus_not_success(status, error);

    if (in != NULL) {
        for (i = 0; i < count; i++) {
            ptr[i].type  = in->sids[i].type;
            ptr[i].rid   = in->sids[i].rid;
            ptr[i].index = in->sids[i].index;
        }
    }

    *out = ptr;

cleanup:
    return status;

error:
    if (ptr) {
        LsaRpcFreeMemory((void*)ptr);
    }

    *out = NULL;
    goto cleanup;
}


NTSTATUS
LsaAllocateTranslatedSids2(
    TranslatedSid2 **out,
    TranslatedSidArray2 *in
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    TranslatedSid2 *ptr = NULL;
    int i = 0;
    int count = 0;

    goto_if_invalid_param_ntstatus(out, cleanup);

    count = (in == NULL) ? 1 : in->count;

    status = LsaRpcAllocateMemory((void**)&ptr,
                                  sizeof(TranslatedSid2) * count,
                                  NULL);
    goto_if_ntstatus_not_success(status, error);

    if (in != NULL) {
        for (i = 0; i < count; i++) {
            ptr[i].type     = in->sids[i].type;
            ptr[i].rid      = in->sids[i].rid;
            ptr[i].index    = in->sids[i].index;
            ptr[i].unknown1 = in->sids[i].unknown1;
        }
    }

    *out = ptr;

cleanup:
    return status;

error:
    if (ptr) {
        LsaRpcFreeMemory((void*)ptr);
    }

    *out = NULL;
    goto cleanup;
}


NTSTATUS
LsaAllocateRefDomainList(
    RefDomainList **out,
    RefDomainList *in
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    RefDomainList *ptr = NULL;
    int i = 0;

    goto_if_invalid_param_ntstatus(out, cleanup);

    status = LsaRpcAllocateMemory((void**)&ptr, sizeof(RefDomainList),
                                  NULL);
    goto_if_ntstatus_not_success(status, error);

    if (in == NULL) goto cleanup;

    ptr->count    = in->count;
    ptr->max_size = in->max_size;

    status = LsaRpcAllocateMemory((void**)&ptr->domains,
                                  sizeof(LsaDomainInfo) * ptr->count,
                                  (void*)ptr);
    goto_if_ntstatus_not_success(status, error);

    for (i = 0; i < ptr->count; i++) {
        LsaDomainInfo *info = &(ptr->domains[i]);

        status = CopyUnicodeStringEx(&info->name, &in->domains[i].name);
        goto_if_ntstatus_not_success(status, error);

        if (info->name.string) {
            status = LsaRpcAddDepMemory((void*)info->name.string, (void*)ptr);
            goto_if_ntstatus_not_success(status, error);
        }

        MsRpcDuplicateSid(&info->sid, in->domains[i].sid);
        goto_if_no_memory_ntstatus(info->sid, error);

        if (info->sid) {
            status = LsaRpcAddDepMemory((void*)info->sid, (void*)ptr);
            goto_if_ntstatus_not_success(status, error);
        }
    }

    *out = ptr;

cleanup:

    return status;

error:
    if (ptr) {
        LsaRpcFreeMemory(ptr);
    }

    *out = NULL;
    goto cleanup;
}


NTSTATUS
LsaAllocateTranslatedNames(
    TranslatedName **out,
    TranslatedNameArray *in
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    TranslatedName *ptr = NULL;
    int i = 0;
    int count = 0;

    goto_if_invalid_param_ntstatus(out, cleanup);

    count = (in) ? in->count : 1;

    /* Allocate given number of names or only one
       if "in" parameter is NULL */
    status = LsaRpcAllocateMemory((void**)&ptr, 
                                  sizeof(TranslatedName) * count,
                                  NULL);
    goto_if_ntstatus_not_success(status, error);

    if (in != NULL) {
        for (i = 0; i < in->count; i++) {
            TranslatedName *nout = &ptr[i];
            TranslatedName *nin  = &in->names[i];

            nout->type      = nin->type;
            nout->sid_index = nin->sid_index;

            status = CopyUnicodeString(&nout->name, &nin->name);
            goto_if_ntstatus_not_success(status, error);

            if (nout->name.string) {
                status = LsaRpcAddDepMemory((void*)nout->name.string, (void*)ptr);
                goto_if_ntstatus_not_success(status, error);
            }
        }
    }

    *out = ptr;

cleanup:
    return status;

error:
    if (ptr) {
        LsaRpcFreeMemory((void*)ptr);
    }

    *out = NULL;
    goto cleanup;
}


static
NTSTATUS
LsaCopyPolInfoField(
    void *out,
    void *in,
    size_t size)
{
    NTSTATUS status = STATUS_SUCCESS;

    goto_if_invalid_param_ntstatus(out, cleanup);
    goto_if_invalid_param_ntstatus(in, cleanup);

    memcpy(out, in, size);

cleanup:
    return status;
}


static
NTSTATUS
LsaCopyPolInfoAuditEvents(
    AuditEventsInfo *out,
    AuditEventsInfo *in,
    void *dep
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    goto_if_invalid_param_ntstatus(out, cleanup);
    goto_if_invalid_param_ntstatus(in, cleanup);

    out->auditing_mode = in->auditing_mode;
    out->count         = in->count;

    status = LsaRpcAllocateMemory((void**)&out->settings,
                                  (size_t)out->count,
                                  dep);
    goto_if_ntstatus_not_success(status, cleanup);

    memcpy((void*)&out->settings, (void*)&in->settings,
           out->count);

cleanup:
    return status;
}


static
NTSTATUS
LsaCopyPolInfoLsaDomain(
    LsaDomainInfo *out,
    LsaDomainInfo *in,
    void *dep
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    goto_if_invalid_param_ntstatus(out, cleanup);
    goto_if_invalid_param_ntstatus(in, cleanup);

    status = CopyUnicodeStringEx(&out->name, &in->name);
    goto_if_ntstatus_not_success(status, cleanup);

    if (out->name.string) {
        status = LsaRpcAddDepMemory((void*)out->name.string, dep);
        goto_if_ntstatus_not_success(status, cleanup);
    }

    MsRpcDuplicateSid(&out->sid, in->sid);
    goto_if_no_memory_ntstatus(out->sid, cleanup);

    if (out->sid) {
        status = LsaRpcAddDepMemory((void*)out->sid, dep);
        goto_if_ntstatus_not_success(status, cleanup);
    }

cleanup:
    return status;
}


static
NTSTATUS
LsaCopyPolInfoPDAccount(
    PDAccountInfo *out,
    PDAccountInfo *in,
    void *dep
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    goto_if_invalid_param_ntstatus(out, cleanup);
    goto_if_invalid_param_ntstatus(in, cleanup);

    status = CopyUnicodeString(&out->name, &in->name);
    goto_if_ntstatus_not_success(status, cleanup);

    if (out->name.string) {
        status = LsaRpcAddDepMemory((void*)out->name.string, dep);
        goto_if_ntstatus_not_success(status, cleanup);
    }

cleanup:
    return status;
}


static
NTSTATUS
LsaCopyPolInfoReplicaSource(
    ReplicaSourceInfo *in,
    ReplicaSourceInfo *out,
    void *dep
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    goto_if_invalid_param_ntstatus(out, cleanup);
    goto_if_invalid_param_ntstatus(in, cleanup);

    status = CopyUnicodeString(&out->source, &in->source);
    goto_if_ntstatus_not_success(status, cleanup);

    if (out->source.string) {
        status = LsaRpcAddDepMemory((void*)out->source.string, dep);
        goto_if_ntstatus_not_success(status, cleanup);
    }

    status = CopyUnicodeString(&out->account, &in->account);
    goto_if_ntstatus_not_success(status, cleanup);

    if (out->account.string) {
        status = LsaRpcAddDepMemory((void*)out->account.string, dep);
        goto_if_ntstatus_not_success(status, cleanup);
    }

cleanup:
    return status;
}


static
NTSTATUS
LsaCopyPolInfoDnsDomain(
    DnsDomainInfo *out,
    DnsDomainInfo *in,
    void *dep
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    goto_if_invalid_param_ntstatus(out, cleanup);
    goto_if_invalid_param_ntstatus(in, cleanup);

    status = CopyUnicodeStringEx(&out->name, &in->name);
    goto_if_ntstatus_not_success(status, cleanup);

    if (out->name.string) {
        status = LsaRpcAddDepMemory((void*)out->name.string, dep);
        goto_if_ntstatus_not_success(status, cleanup);
    }

    status = CopyUnicodeStringEx(&out->dns_domain, &in->dns_domain);
    goto_if_ntstatus_not_success(status, cleanup);

    if (out->dns_domain.string) {
        status = LsaRpcAddDepMemory((void*)out->dns_domain.string, dep);
        goto_if_ntstatus_not_success(status, cleanup);
    }

    status = CopyUnicodeStringEx(&out->dns_forest, &in->dns_forest);
    goto_if_ntstatus_not_success(status, cleanup);

    if (out->dns_forest.string) {
        status = LsaRpcAddDepMemory((void*)out->dns_forest.string, dep);
        goto_if_ntstatus_not_success(status, cleanup);
    }

    memcpy((void*)&out->domain_guid, (void*)&in->domain_guid,
           sizeof(Guid));

    MsRpcDuplicateSid(&out->sid, in->sid);
    goto_if_no_memory_ntstatus(out->sid, cleanup);

    if (out->sid) {
        status = LsaRpcAddDepMemory((void*)out->sid, dep);
        goto_if_ntstatus_not_success(status, cleanup);
    }

cleanup:
    return status;
}


NTSTATUS
LsaAllocatePolicyInformation(
   LsaPolicyInformation **out,
   LsaPolicyInformation *in,
   uint32 level
   )
{
    NTSTATUS status = STATUS_SUCCESS;
    LsaPolicyInformation *ptr = NULL;

    goto_if_invalid_param_ntstatus(out, cleanup);

    status = LsaRpcAllocateMemory((void**)&ptr,
                                  sizeof(LsaPolicyInformation),
                                  NULL);
    goto_if_ntstatus_not_success(status, error);

    if (in != NULL) {

        switch (level) {
        case LSA_POLICY_INFO_AUDIT_LOG:
            status = LsaCopyPolInfoField((void*)&ptr->audit_log,
                                         (void*)&in->audit_log,
                                         sizeof(AuditLogInfo));
            break;

        case LSA_POLICY_INFO_AUDIT_EVENTS:
            status = LsaCopyPolInfoAuditEvents(&ptr->audit_events,
                                               &in->audit_events,
                                               (void*)ptr);
            break;

        case LSA_POLICY_INFO_DOMAIN:
            status = LsaCopyPolInfoLsaDomain(&ptr->domain,
                                             &in->domain,
                                             (void*)ptr);
            break;

        case LSA_POLICY_INFO_PD:
            status = LsaCopyPolInfoPDAccount(&ptr->pd, &in->pd, (void*)ptr);
            break;

        case LSA_POLICY_INFO_ACCOUNT_DOMAIN:
            status = LsaCopyPolInfoLsaDomain(&ptr->account_domain,
                                             &in->account_domain,
                                             (void*)ptr);
            break;

        case LSA_POLICY_INFO_ROLE:
            status = LsaCopyPolInfoField((void*)&ptr->role, (void*)&in->role,
                                         sizeof(ServerRole));
            break;

        case LSA_POLICY_INFO_REPLICA:
            status = LsaCopyPolInfoReplicaSource(&ptr->replica, &in->replica,
                                                 (void*)ptr);
            break;

        case LSA_POLICY_INFO_QUOTA:
            status = LsaCopyPolInfoField((void*)&ptr->quota, (void*)&in->quota,
                                         sizeof(DefaultQuotaInfo));
            break;

        case LSA_POLICY_INFO_DB:
            status = LsaCopyPolInfoField((void*)&ptr->db, (void*)&in->db,
                                         sizeof(ModificationInfo));
            break;

        case LSA_POLICY_INFO_AUDIT_FULL_SET:
            status = LsaCopyPolInfoField((void*)&ptr->audit_set,
                                         (void*)&in->audit_set,
                                         sizeof(AuditFullSetInfo));
            break;
        
        case LSA_POLICY_INFO_AUDIT_FULL_QUERY:
            status = LsaCopyPolInfoField((void*)&ptr->audit_query,
                                         (void*)&in->audit_query,
                                         sizeof(AuditFullQueryInfo));
            break;

        case LSA_POLICY_INFO_DNS:
            status = LsaCopyPolInfoDnsDomain(&ptr->dns, &in->dns, (void*)ptr);
            break;

        default:
            status = STATUS_INVALID_LEVEL;
        }

        goto_if_ntstatus_not_success(status, error);

    }

    *out = ptr;

cleanup:
    return status;

error:
    if (ptr) {
        LsaRpcFreeMemory((void*)ptr);
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
