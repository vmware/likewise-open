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
 * Abstract: Netlogon memory (de)allocation routines (rpc client library)
 *
 * Authors: Rafal Szczesniak (rafal@likewisesoftware.com)
 */

#include "includes.h"

extern void *netr_ptr_list;


NTSTATUS NetrInitMemory()
{
    NTSTATUS status = STATUS_SUCCESS;
    int locked = 0;

    GLOBAL_DATA_LOCK(locked);

    if (!bInitialised) {
        status = MemPtrListInit((PtrList**)&netr_ptr_list);
        goto_if_ntstatus_not_success(status, done);

        bInitialised = 1;
    }

done:
    GLOBAL_DATA_UNLOCK(locked);

    return status;
}


NTSTATUS NetrDestroyMemory()
{
    NTSTATUS status = STATUS_SUCCESS;
    int locked = 0;

    GLOBAL_DATA_LOCK(locked);

    if (bInitialised && netr_ptr_list) {
        status = MemPtrListDestroy((PtrList**)&netr_ptr_list);
        goto_if_ntstatus_not_success(status, done);

        bInitialised = 0;
    }

done:
    GLOBAL_DATA_UNLOCK(locked);

    return status;
}


NTSTATUS NetrAllocateMemory(void **out, size_t size, void *dep)
{
    return MemPtrAllocate((PtrList*)netr_ptr_list, out, size, dep);
}


NTSTATUS NetrFreeMemory(void *ptr)
{
    return MemPtrFree((PtrList*)netr_ptr_list, ptr);
}


NTSTATUS NetrAddDepMemory(void *ptr, void *dep)
{
    return MemPtrAddDependant((PtrList*)netr_ptr_list, ptr, dep);
}


/*
 * Type specific functions
 */

NTSTATUS
NetrAllocateUniString(
    wchar16_t **out,
    const wchar16_t *in,
    void *dep
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    size_t len = 0;
    wchar16_t *ptr = NULL;

    goto_if_invalid_param_ntstatus(out, cleanup);
    goto_if_invalid_param_ntstatus(in, cleanup);

    len = wc16slen(in);

    status = NetrAllocateMemory((void**)&ptr, sizeof(wchar16_t) * (len + 1),
                                dep);
    goto_if_ntstatus_not_success(status, error);

    wc16sncpy(ptr, in, len);

    *out = ptr;

cleanup:
    return status;

error:
    if (ptr) {
        NetrFreeMemory((void*)ptr);
    }

    *out = NULL;

    goto cleanup;
}


NTSTATUS NetrAllocateDomainTrusts(NetrDomainTrust **out,
                                  NetrDomainTrustList *in)
{
    NTSTATUS status = STATUS_SUCCESS;
    NetrDomainTrust *ptr = NULL;
    int i = 0;

    goto_if_invalid_param_ntstatus(out, cleanup);
    goto_if_invalid_param_ntstatus(in, cleanup);

    status = NetrAllocateMemory((void**)&ptr,
                                sizeof(NetrDomainTrust) * in->count,
                                NULL);
    goto_if_ntstatus_not_success(status, cleanup);

    for (i = 0; i < in->count; i++) {
        NetrDomainTrust *tout = &ptr[i];
        NetrDomainTrust *tin  = &in->array[i];

        tout->trust_flags  = tin->trust_flags;
        tout->parent_index = tin->parent_index;
        tout->trust_type   = tin->trust_type;
        tout->trust_attrs  = tin->trust_attrs;

        if (tin->netbios_name) {
            tout->netbios_name = wc16sdup(tin->netbios_name);
            goto_if_no_memory_ntstatus(tout->netbios_name, error);

            status = NetrAddDepMemory((void*)tout->netbios_name, (void*)ptr);
            goto_if_ntstatus_not_success(status, error);
        }

        if (tin->dns_name) {
            tout->dns_name = wc16sdup(tin->dns_name);
            goto_if_no_memory_ntstatus(tout->dns_name, error);

            status = NetrAddDepMemory((void*)tout->dns_name, (void*)ptr);
            goto_if_ntstatus_not_success(status, error);
        }

        if (tin->sid)
        {
            MsRpcDuplicateSid(&tout->sid, tin->sid);
            goto_if_no_memory_ntstatus(tout->sid, error);

            status = NetrAddDepMemory((void*)tout->sid, (void*)ptr);
            goto_if_ntstatus_not_success(status, error);
        }

        tout->guid = tin->guid;
    }

cleanup:
    *out = ptr;

    return status;

error:
    NetrFreeMemory((void*)ptr);
    ptr = NULL;
    goto cleanup;
}


NTSTATUS NetrInitIdentityInfo(NetrIdentityInfo *ptr, void *dep,
                              const wchar16_t *domain,
                              const wchar16_t *workstation,
                              const wchar16_t *account, uint32 param_control,
                              uint32 logon_id_low, uint32 logon_id_high)
{
    NTSTATUS status = STATUS_SUCCESS;
    wchar16_t *nbt_workstation = NULL;
    size_t nbt_workstation_len = 0;

    goto_if_invalid_param_ntstatus(ptr, cleanup);
    goto_if_invalid_param_ntstatus(domain, cleanup);
    goto_if_invalid_param_ntstatus(account, cleanup);
    goto_if_invalid_param_ntstatus(workstation, cleanup);

    nbt_workstation_len = wc16slen(workstation);
    status = NetrAllocateMemory((void**)&nbt_workstation,
                                (nbt_workstation_len + 3) * sizeof(wchar16_t),
                                dep);
    goto_if_ntstatus_not_success(status, error);

    if (sw16printfw(
            nbt_workstation,
            nbt_workstation_len + 3,
            L"\\\\%ws",
            workstation) < 0)
    {
        status = ErrnoToNtStatus(errno);
        goto_if_ntstatus_not_success(status, error);
    }

    status = InitUnicodeString(&ptr->domain_name, domain);
    goto_if_ntstatus_not_success(status, error);

    if (ptr->domain_name.string) {
        status = NetrAddDepMemory((void*)ptr->domain_name.string, (void*)dep);
        goto_if_ntstatus_not_success(status, error);
    }

    status = InitUnicodeString(&ptr->account_name, account);
    goto_if_ntstatus_not_success(status, error);

    if (ptr->account_name.string) {
        status = NetrAddDepMemory((void*)ptr->account_name.string, (void*)dep);
        goto_if_ntstatus_not_success(status, error);
    }

    status = InitUnicodeString(&ptr->workstation, nbt_workstation);
    goto_if_ntstatus_not_success(status, error);

    if (ptr->workstation.string) {
        status = NetrAddDepMemory((void*)ptr->workstation.string, (void*)dep);
        goto_if_ntstatus_not_success(status, error);
    }

    ptr->param_control = param_control;
    ptr->logon_id_low  = logon_id_low;
    ptr->logon_id_high = logon_id_high;

    if (nbt_workstation) {
        NetrFreeMemory((void*)nbt_workstation);
    }

cleanup:
    return status;

error:
    FreeUnicodeString(&ptr->domain_name);
    FreeUnicodeString(&ptr->account_name);
    FreeUnicodeString(&ptr->workstation);

    goto cleanup;
}

/*
 * Compatibility wrapper
 */
NTSTATUS NetrAllocateLogonInfo(
    NetrLogonInfo **out, uint16 level,
    const wchar16_t *domain,
    const wchar16_t *workstation,
    const wchar16_t *account,
    const wchar16_t *password
    )
{
    return NetrAllocateLogonInfoHash(out, level, domain, workstation, account, password);
}


NTSTATUS NetrAllocateLogonInfoHash(
    NetrLogonInfo **out, uint16 level,
    const wchar16_t *domain,
    const wchar16_t *workstation,
    const wchar16_t *account,
    const wchar16_t *password
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    NetrLogonInfo *ptr = NULL;
    NetrPasswordInfo *pass = NULL;
    uint8 lm_hash[16] = {0};
    uint8 nt_hash[16] = {0};

    goto_if_invalid_param_ntstatus(out, cleanup);
    goto_if_invalid_param_ntstatus(domain, cleanup);
    goto_if_invalid_param_ntstatus(account, cleanup);
    goto_if_invalid_param_ntstatus(workstation, cleanup);
    goto_if_invalid_param_ntstatus(password, cleanup);

    status = NetrAllocateMemory((void**)&ptr, sizeof(NetrLogonInfo), NULL);
    goto_if_ntstatus_not_success(status, cleanup);


    switch (level)
    {
    case 1:
    case 3:
    case 5:
        /* Create password hashes (NT and LM) */
        deshash(lm_hash, password);
        md4hash(nt_hash, password);

        status = NetrAllocateMemory((void**)&pass, sizeof(NetrPasswordInfo),
                                    (void*)ptr);
        goto_if_ntstatus_not_success(status, error);

        status = NetrInitIdentityInfo(&pass->identity, (void*)pass,
                                      domain, workstation, account, 0, 0, 0);
        goto_if_ntstatus_not_success(status, error);

        /* Copy the password hashes */
        memcpy((void*)pass->lmpassword.data, (void*)lm_hash,
               sizeof(pass->lmpassword.data));
        memcpy((void*)pass->ntpassword.data, (void*)nt_hash,
               sizeof(pass->ntpassword.data));
        break;

    default:
        status = STATUS_INVALID_LEVEL;
        goto error;
    }

    switch (level) {
    case 1:
        ptr->password1 = pass;
        break;
    case 3:
        ptr->password3 = pass;
        break;
    case 5:
        ptr->password5 = pass;
        break;
    }

cleanup:
    *out = ptr;

    return status;

error:
    if (ptr) {
        NetrFreeMemory((void*)ptr);
    }

    ptr = NULL;
    goto cleanup;
}

NTSTATUS NetrAllocateLogonInfoNet(
    NetrLogonInfo **out,
    uint16 level,
    const wchar16_t *domain,
    const wchar16_t *workstation,
    const wchar16_t *account,
    const uint8_t *challenge,
    const uint8_t *lm_resp,
    const uint8_t *nt_resp
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    NetrLogonInfo *ptr = NULL;
    NetrNetworkInfo *net = NULL;

    goto_if_invalid_param_ntstatus(out, cleanup);
    goto_if_invalid_param_ntstatus(domain, cleanup);
    goto_if_invalid_param_ntstatus(account, cleanup);
    goto_if_invalid_param_ntstatus(workstation, cleanup);
    goto_if_invalid_param_ntstatus(challenge, cleanup);
    /* LanMan Response can be NULL */
    goto_if_invalid_param_ntstatus(nt_resp, cleanup);

    status = NetrAllocateMemory((void**)&ptr, sizeof(NetrLogonInfo), NULL);
    goto_if_ntstatus_not_success(status, cleanup);

    switch (level)
    {
    case 2:
    case 6:
        status = NetrAllocateMemory((void**)&net, sizeof(NetrNetworkInfo),
                                    (void*)ptr);
        goto_if_ntstatus_not_success(status, error);

        status = NetrInitIdentityInfo(&net->identity, (void*)net,
                                      domain, workstation, account, 0, 0, 0);
        goto_if_ntstatus_not_success(status, error);

        memcpy(net->challenge, challenge, sizeof(net->challenge));

        /* Allocate challenge structures */
        if (lm_resp)
        {
            status = NetrAllocateMemory((void**)&net->lm.data, 24, (void*)net);
            goto_if_ntstatus_not_success(status, error);

            net->lm.length = 24;
            net->lm.size   = 24;
            memcpy(net->lm.data, lm_resp, net->lm.size);
        }

        /* Always have NT Response */

        status = NetrAllocateMemory((void**)&net->nt.data, 24, (void*)net);
        goto_if_ntstatus_not_success(status, error);

        net->nt.length = 24;
        net->nt.size   = 24;
        memcpy(net->nt.data, nt_resp, net->nt.size);

        break;

    default:
        status = STATUS_INVALID_LEVEL;
        goto error;
    }

    switch (level) {
    case 2:
        ptr->network2  = net;
        break;
    case 6:
        ptr->network6  = net;
        break;
    }

cleanup:
    *out = ptr;

    return status;

error:
    if (ptr) {
        NetrFreeMemory((void*)ptr);
    }

    ptr = NULL;
    goto cleanup;
}


static NTSTATUS NetrInitSamBaseInfo(NetrSamBaseInfo *ptr,
                                    NetrSamBaseInfo *in, void *dep)
{
    NTSTATUS status = STATUS_SUCCESS;
    int i = 0;

    goto_if_invalid_param_ntstatus(ptr, cleanup);
    goto_if_invalid_param_ntstatus(in, cleanup);

    ptr->last_logon            = in->last_logon;
    ptr->last_logoff           = in->last_logoff;
    ptr->acct_expiry           = in->acct_expiry;
    ptr->last_password_change  = in->last_password_change;
    ptr->allow_password_change = in->allow_password_change;
    ptr->force_password_change = in->force_password_change;

    status = CopyUnicodeStringEx(&ptr->account_name, &in->account_name);
    goto_if_ntstatus_not_success(status, error);

    if (ptr->account_name.string) {
        status = NetrAddDepMemory((void*)ptr->account_name.string, (void*)dep);
        goto_if_ntstatus_not_success(status, error);
    }

    status = CopyUnicodeStringEx(&ptr->full_name, &in->full_name);
    goto_if_ntstatus_not_success(status, error);

    if (ptr->full_name.string) {
        status = NetrAddDepMemory((void*)ptr->full_name.string, (void*)dep);
        goto_if_ntstatus_not_success(status, error);
    }

    status = CopyUnicodeStringEx(&ptr->logon_script, &in->logon_script);
    goto_if_ntstatus_not_success(status, error);

    if (ptr->logon_script.string) {
        status = NetrAddDepMemory((void*)ptr->logon_script.string, (void*)dep);
        goto_if_ntstatus_not_success(status, error);
    }

    status = CopyUnicodeStringEx(&ptr->profile_path, &in->profile_path);
    goto_if_ntstatus_not_success(status, error);

    if (ptr->profile_path.string) {
        status = NetrAddDepMemory((void*)ptr->profile_path.string, (void*)dep);
        goto_if_ntstatus_not_success(status, error);
    }

    status = CopyUnicodeStringEx(&ptr->home_directory, &in->home_directory);
    goto_if_ntstatus_not_success(status, error);

    if (ptr->home_directory.string) {
        status = NetrAddDepMemory((void*)ptr->home_directory.string, (void*)dep);
        goto_if_ntstatus_not_success(status, error);
    }

    status = CopyUnicodeStringEx(&ptr->home_drive, &in->home_drive);
    goto_if_ntstatus_not_success(status, error);

    if (ptr->home_drive.string) {
        status = NetrAddDepMemory((void*)ptr->home_drive.string, (void*)dep);
        goto_if_ntstatus_not_success(status, error);
    }

    ptr->logon_count = in->logon_count;
    ptr->bad_password_count = in->bad_password_count;
    ptr->rid = in->rid;
    ptr->primary_gid = in->primary_gid;

    ptr->groups.count = in->groups.count;
    status = NetrAllocateMemory((void*)&ptr->groups.rids,
                                sizeof(RidWithAttribute) * ptr->groups.count,
                                (void*)dep);
    goto_if_ntstatus_not_success(status, error);

    for (i = 0; i < ptr->groups.count; i++) {
        RidWithAttribute *ptr_ra = &(ptr->groups.rids[i]);
        RidWithAttribute *in_ra = &(in->groups.rids[i]);

        ptr_ra->rid        = in_ra->rid;
        ptr_ra->attributes = in_ra->attributes;
    }

    ptr->user_flags = in->user_flags;

    memcpy((void*)ptr->key.key, (void*)in->key.key, sizeof(ptr->key.key));

    status = CopyUnicodeStringEx(&ptr->logon_server, &in->logon_server);
    goto_if_ntstatus_not_success(status, error);

    if (ptr->logon_server.string) {
        status = NetrAddDepMemory((void*)ptr->logon_server.string, (void*)dep);
        goto_if_ntstatus_not_success(status, error);
    }

    status = CopyUnicodeStringEx(&ptr->domain, &in->domain);
    goto_if_ntstatus_not_success(status, error);

    if (ptr->domain.string) {
        status = NetrAddDepMemory((void*)ptr->domain.string, (void*)dep);
        goto_if_ntstatus_not_success(status, error);
    }

    if (in->domain_sid) {
        MsRpcDuplicateSid(&ptr->domain_sid, in->domain_sid);
        goto_if_no_memory_ntstatus(ptr->domain_sid, error);

        status = NetrAddDepMemory((void*)ptr->domain_sid, (void*)dep);
        goto_if_ntstatus_not_success(status, error);

    } else {
        ptr->domain_sid = NULL;
    }

    memcpy((void*)ptr->lmkey.key, (void*)in->lmkey.key,
           sizeof(ptr->lmkey.key));

    ptr->acct_flags = in->acct_flags;

cleanup:
    return status;

error:
    FreeUnicodeStringEx(&ptr->account_name);
    FreeUnicodeStringEx(&ptr->full_name);
    FreeUnicodeStringEx(&ptr->logon_script);
    FreeUnicodeStringEx(&ptr->profile_path);
    FreeUnicodeStringEx(&ptr->home_directory);
    FreeUnicodeStringEx(&ptr->home_drive);
    FreeUnicodeStringEx(&ptr->logon_server);
    FreeUnicodeStringEx(&ptr->domain);

    if (ptr->groups.rids) {
        NetrFreeMemory((void*)ptr->groups.rids);
    }

    if (ptr->domain_sid) {
        MsRpcFreeSid(ptr->domain_sid);
    }

    goto cleanup;
}


static NTSTATUS NetrAllocateSamInfo2(NetrSamInfo2 **out, NetrSamInfo2 *in,
                                     void *dep)
{
    NTSTATUS status = STATUS_SUCCESS;
    NetrSamInfo2 *ptr = NULL;

    goto_if_invalid_param_ntstatus(out, cleanup);

    status = NetrAllocateMemory((void*)&ptr, sizeof(NetrSamInfo2), dep);
    goto_if_ntstatus_not_success(status, error);

    if (in) {
        status = NetrInitSamBaseInfo(&ptr->base, &in->base, (void*)ptr);
        goto_if_ntstatus_not_success(status, error);
    }

    *out = ptr;

cleanup:
    return status;

error:
    if (ptr) {
        NetrFreeMemory((void*)ptr);
    }

    *out = NULL;

    goto cleanup;
}


static NTSTATUS NetrAllocateSamInfo3(NetrSamInfo3 **out, NetrSamInfo3 *in,
                                      void *dep)
{
    NTSTATUS status = STATUS_SUCCESS;
    NetrSamInfo3 *ptr = NULL;
    int i = 0;

    goto_if_invalid_param_ntstatus(out, cleanup);

    status = NetrAllocateMemory((void*)&ptr, sizeof(NetrSamInfo3), dep);
    goto_if_ntstatus_not_success(status, error);

    if (in) {
        status = NetrInitSamBaseInfo(&ptr->base, &in->base, (void*)ptr);
        goto_if_ntstatus_not_success(status, error);

        ptr->sidcount = in->sidcount;

        status = NetrAllocateMemory((void*)&ptr->sids,
                                    sizeof(NetrSidAttr) * ptr->sidcount,
                                    (void*)ptr);
        goto_if_ntstatus_not_success(status, error);

        for (i = 0; ptr->sidcount; i++) {
            NetrSidAttr *ptr_sa = &(ptr->sids[i]);
            NetrSidAttr *in_sa = &(in->sids[i]);

            if (in_sa->sid) {
                MsRpcDuplicateSid(&ptr_sa->sid, in_sa->sid);
                goto_if_no_memory_ntstatus(ptr_sa->sid, error);

                status = NetrAddDepMemory((void*)ptr_sa->sid, (void*)ptr);
                goto_if_ntstatus_not_success(status, error);

            } else {
                ptr_sa->sid = NULL;
            }

            ptr_sa->attribute = in_sa->attribute;
        }
    }

    *out = ptr;

cleanup:
    return status;

error:
    if (ptr) {
        NetrFreeMemory((void*)ptr);
    }

    *out = NULL;

    goto cleanup;
}


static NTSTATUS NetrAllocatePacInfo(NetrPacInfo **out, NetrPacInfo *in,
                                    void *dep)
{
    return STATUS_NOT_IMPLEMENTED;
}


static NTSTATUS NetrAllocateSamInfo6(NetrSamInfo6 **out, NetrSamInfo6 *in,
                                    void *dep)
{
    return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS NetrAllocateValidationInfo(NetrValidationInfo **out,
                                    NetrValidationInfo *in, uint16 level)
{
    NTSTATUS status = STATUS_SUCCESS;
    NetrValidationInfo *ptr = NULL;

    goto_if_invalid_param_ntstatus(out, cleanup);

    status = NetrAllocateMemory((void**)&ptr, sizeof(NetrValidationInfo), NULL);
    goto_if_ntstatus_not_success(status, error);

    switch (level) {
    case 2:
        status = NetrAllocateSamInfo2(&ptr->sam2, in->sam2, (void*)ptr);
        break;
    case 3:
        status = NetrAllocateSamInfo3(&ptr->sam3, in->sam3, (void*)ptr);
        break;
    case 4:
        status = NetrAllocatePacInfo(&ptr->pac4, in->pac4, (void*)ptr);
        break;
    case 5:
        status = NetrAllocatePacInfo(&ptr->pac5, in->pac5, (void*)ptr);
        break;
    case 6:
        status = NetrAllocateSamInfo6(&ptr->sam6, in->sam6, (void*)ptr);
        break;

    default:
        status = STATUS_INVALID_LEVEL;
    }
    goto_if_ntstatus_not_success(status, error);

    *out = ptr;

cleanup:
    return status;

error:
    if (ptr) {
        NetrFreeMemory((void*)ptr);
    }

    *out = NULL;

    goto cleanup;
}


static
NTSTATUS
NetrCopyDomainTrustInfo(
    NetrDomainTrustInfo *out,
    NetrDomainTrustInfo *in,
    void *dep
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    goto_if_invalid_param_ntstatus(out, cleanup);
    goto_if_invalid_param_ntstatus(in, cleanup);

    status = CopyUnicodeString(&out->domain_name, &in->domain_name);
    goto_if_ntstatus_not_success(status, error);

    if (out->domain_name.string) {
        status = NetrAddDepMemory((void*)out->domain_name.string,
                                  (void*)dep);
        goto_if_ntstatus_not_success(status, error);
    }

    status = CopyUnicodeString(&out->full_domain_name,
                               &in->full_domain_name);
    goto_if_ntstatus_not_success(status, error);

    if (out->full_domain_name.string) {
        status = NetrAddDepMemory((void*)out->full_domain_name.string,
                                  (void*)dep);
        goto_if_ntstatus_not_success(status, error);
    }

    status = CopyUnicodeString(&out->forest, &in->forest);
    goto_if_ntstatus_not_success(status, error);

    if (out->forest.string) {
        status = NetrAddDepMemory((void*)out->forest.string,
                                  (void*)dep);
        goto_if_ntstatus_not_success(status, error);
    }

    memcpy(&out->guid, &in->guid, sizeof(out->guid));

    if (in->sid) {
        MsRpcDuplicateSid(&out->sid, in->sid);
        goto_if_no_memory_ntstatus(out->sid, error);

        status = NetrAddDepMemory((void*)out->sid, dep);
        goto_if_ntstatus_not_success(status, error);
    }

cleanup:
    return status;

error:
    memset(out, 0, sizeof(*out));
    goto cleanup;
}


static
NTSTATUS
NetrAllocateDomainInfo1(
    NetrDomainInfo1 **out,
    NetrDomainInfo1 *in,
    void *dep
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    NetrDomainInfo1 *ptr = NULL;
    int i = 0;

    status = NetrAllocateMemory((void**)&ptr, sizeof(NetrDomainInfo1), dep);
    goto_if_ntstatus_not_success(status, error);

    if (in == NULL) goto cleanup;

    status = NetrCopyDomainTrustInfo(&ptr->domain_info, &in->domain_info,
                                     dep);
    goto_if_ntstatus_not_success(status, error);

    ptr->num_trusts = in->num_trusts;

    status = NetrAllocateMemory((void**)&ptr->trusts,
                                sizeof(NetrDomainTrustInfo) * ptr->num_trusts,
                                dep);
    goto_if_ntstatus_not_success(status, error);

    for (i = 0; i < ptr->num_trusts; i++) {
        status = NetrCopyDomainTrustInfo(&ptr->trusts[i], &in->trusts[i],
                                         ptr->trusts);
        goto_if_ntstatus_not_success(status, error);
    }

    *out = ptr;

cleanup:
    return status;

error:
    if (ptr) {
        NetrFreeMemory(ptr);
    }

    goto error;
}


NTSTATUS
NetrAllocateDomainInfo(
    NetrDomainInfo **out,
    NetrDomainInfo *in,
    uint32 level
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    NetrDomainInfo *ptr = NULL;

    goto_if_invalid_param_ntstatus(out, cleanup);

    status = NetrAllocateMemory((void**)&ptr, sizeof(NetrDomainInfo), NULL);
    goto_if_ntstatus_not_success(status, error);

    if (in == NULL) goto cleanup;

    switch (level) {
    case 1:
        status = NetrAllocateDomainInfo1(&ptr->info1, in->info1, (void*)ptr);
        break;

    case 2:
        status = NetrAllocateDomainInfo1(&ptr->info2, in->info2, (void*)ptr);
        break;

    default:
        status = STATUS_INVALID_LEVEL;
    }
    goto_if_ntstatus_not_success(status, error);

    *out = ptr;

cleanup:
    return status;

error:
    if (ptr) {
        NetrFreeMemory((void*)ptr);
    }

    goto cleanup;
}


NTSTATUS
NetrAllocateDcNameInfo(
    DsrDcNameInfo **out,
    DsrDcNameInfo *in
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    DsrDcNameInfo *ptr = NULL;

    goto_if_invalid_param_ntstatus(out, cleanup);

    status = NetrAllocateMemory((void**)&ptr, sizeof(DsrDcNameInfo), NULL);
    goto_if_ntstatus_not_success(status, error);

    if (in == NULL) goto cleanup;

    if (in->dc_name) {
        ptr->dc_name = wc16sdup(in->dc_name);
        goto_if_no_memory_ntstatus(ptr->dc_name, error);

        status = NetrAddDepMemory(ptr->dc_name, ptr);
        goto_if_ntstatus_not_success(status, error);
    }

    if (in->dc_address) {
        ptr->dc_address = wc16sdup(in->dc_address);
        goto_if_no_memory_ntstatus(ptr->dc_address, error);

        status = NetrAddDepMemory(ptr->dc_address, ptr);
        goto_if_ntstatus_not_success(status, error);
    }

    ptr->address_type = in->address_type;

    ptr->flags = in->flags;

    memcpy(&ptr->domain_guid, &in->domain_guid, sizeof(ptr->domain_guid));

    if (in->domain_name) {
        ptr->domain_name = wc16sdup(in->domain_name);
        goto_if_no_memory_ntstatus(ptr->domain_name, error);

        status = NetrAddDepMemory(ptr->domain_name, ptr);
        goto_if_ntstatus_not_success(status, error);
    }

    if (in->forest_name) {
        ptr->forest_name = wc16sdup(in->forest_name);
        goto_if_no_memory_ntstatus(ptr->forest_name, error);

        status = NetrAddDepMemory(ptr->forest_name, ptr);
        goto_if_ntstatus_not_success(status, error);
    }

    if (in->dc_site_name) {
        ptr->dc_site_name = wc16sdup(in->dc_site_name);
        goto_if_no_memory_ntstatus(ptr->dc_site_name, error);

        status = NetrAddDepMemory(ptr->dc_site_name, ptr);
        goto_if_ntstatus_not_success(status, error);
    }

    if (in->cli_site_name) {
        ptr->cli_site_name = wc16sdup(in->cli_site_name);
        goto_if_no_memory_ntstatus(ptr->cli_site_name, error);

        status = NetrAddDepMemory(ptr->cli_site_name, ptr);
        goto_if_ntstatus_not_success(status, error);
    }

    *out = ptr;

cleanup:
    return status;

error:
    if (ptr) {
        NetrFreeMemory((void*)ptr);
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
