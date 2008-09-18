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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <compat/rpcstatus.h>
#include <dce/dce_error.h>

#include <lwrpc/types.h>
#include <lwrpc/security.h>
#include <wc16str.h>
#include <lwrpc/ntstatus.h>
#include <lwrpc/allocate.h>
#include <lwrpc/lsa.h>
#include <lwrpc/lsabinding.h>
#include <lwrpc/mpr.h>

#include "TestRpc.h"
#include "Params.h"


handle_t CreateLsaBinding(handle_t *binding, const wchar16_t *host)
{
    RPCSTATUS status;
    size_t hostname_size;
    unsigned char *hostname;

    if (binding == NULL || host == NULL) return NULL;

    hostname_size = wc16slen(host) + 1;
    hostname = (unsigned char*) malloc(hostname_size * sizeof(char));
    if (hostname == NULL) return NULL;
    wc16stombs(hostname, host, hostname_size);

    status = InitLsaBindingDefault(binding, hostname);
    if (status != RPC_S_OK) {
        int result;
        CHAR_T errmsg[dce_c_error_string_len];
	
        dce_error_inq_text(status, errmsg, &result);
        if (result == 0) {
            printf("Error: %s\n", errmsg);
        } else {
            printf("Unknown error: %08x\n", status);
        }

        return NULL;
    }

    SAFE_FREE(hostname);
    return *binding;
}


int TestLsaOpenPolicy(struct test *t, const wchar16_t *hostname,
                      const wchar16_t *user, const wchar16_t *pass,
                      struct parameter *options, int optcount)
{
    const uint32 access_rights = LSA_ACCESS_LOOKUP_NAMES_SIDS;

    NTSTATUS status;
    handle_t lsa_binding;
    NETRESOURCE nr = {0};
    PolicyHandle lsa_policy = {0};

    TESTINFO(t, hostname, user, pass);

    SET_SESSION_CREDS(nr, hostname, user, pass);

    lsa_binding = CreateLsaBinding(&lsa_binding, hostname);
    if (lsa_binding == NULL) goto done;

    INPUT_ARG_PTR(lsa_binding);
    INPUT_ARG_WSTR(hostname);
    INPUT_ARG_UINT(access_rights);

    CALL_MSRPC(status = LsaOpenPolicy2(lsa_binding, hostname, NULL,
                                       access_rights, &lsa_policy));
    if (status != 0) rpc_fail(status);

    OUTPUT_ARG_PTR(lsa_binding);
    OUTPUT_ARG_PTR(&lsa_policy);

    status = LsaClose(lsa_binding, &lsa_policy);

    FreeLsaBinding(&lsa_binding);

    RELEASE_SESSION_CREDS(nr);

done:
    LsaRpcDestroyMemory();

    return true;
}


int TestLsaLookupNames(struct test *t, const wchar16_t *hostname,
                       const wchar16_t *user, const wchar16_t *pass,
                       struct parameter *options, int optcount)
{
    const uint32 access_rights = LSA_ACCESS_LOOKUP_NAMES_SIDS;
    const uint32 num_names = 1;
    const char *def_username = "BUILTIN\\Guest";
    const uint32 def_revlookup = 0;

    NTSTATUS status = STATUS_SUCCESS;
    enum param_err perr;
    handle_t lsa_b;
    NETRESOURCE nr = {0};
    wchar16_t *domname, buffer[512], *names[2];
    wchar16_t *username;
    uint32 revlookup;
    PolicyHandle lsa_policy = {0};
    RefDomainList *domains = NULL;
    TranslatedSid *sids = NULL;
    SidArray sid_array = {0};
    TranslatedName *trans_names = NULL;
    uint32 level, names_count, sids_count;
    int i;

    TESTINFO(t, hostname, user, pass);

    SET_SESSION_CREDS(nr, hostname, user, pass);

    perr = fetch_value(options, optcount, "username", pt_w16string, &username,
                       &def_username);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "revlookup", pt_uint32, &revlookup,
                       &def_revlookup);
    if (!perr_is_ok(perr)) perr_fail(perr);

    lsa_b = CreateLsaBinding(&lsa_b, hostname);
    if (lsa_b == NULL) return false;
    
    status = GetSamDomainName(&domname, hostname);
    if (status != 0) rpc_fail(status);

    names[0] = (wchar16_t*) wc16sdup(username);

    status = LsaOpenPolicy2(lsa_b, hostname, NULL, access_rights,
                            &lsa_policy);
    if (status != 0) rpc_fail(status);

    /* Lookup name to sid */
    level = 1;

    INPUT_ARG_PTR(lsa_b);
    INPUT_ARG_PTR(&lsa_policy);
    INPUT_ARG_UINT(num_names);
    INPUT_ARG_WSTR(names[0]);
    INPUT_ARG_WSTR(names[1]);
    INPUT_ARG_PTR(domains);
    INPUT_ARG_PTR(sids);
    INPUT_ARG_UINT(level);

    CALL_MSRPC(status = LsaLookupNames(lsa_b, &lsa_policy, num_names, names,
                                       &domains, &sids, level, &sids_count));

    OUTPUT_ARG_UINT(sids_count);

    if (!revlookup) goto done;

    /* Reverse lookup sid to name */
    sid_array.num_sids = sids_count;
    sid_array.sids = (SidPtr*) malloc(sid_array.num_sids * sizeof(SidPtr));

    for (i = 0; i < sid_array.num_sids; i++) {
        DomSid *usr_sid, *dom_sid;
        uint32 sid_index;

        dom_sid = NULL;
        sid_index = sids[i].index;
		
        if (sid_index < domains->count) {
            dom_sid = domains->domains[sid_index].sid;
            SidAllocateResizedCopy(&usr_sid,
                                   dom_sid->subauth_count + 1,
                                   dom_sid);
            usr_sid->subauth[usr_sid->subauth_count - 1] = sids[i].rid;
            sid_array.sids[i].sid = usr_sid;
        }
    }

    if (domains) {
        LsaRpcFreeMemory((void*)domains);
        domains = NULL;
    }

    level = 6;

    INPUT_ARG_PTR(lsa_b);
    INPUT_ARG_PTR(&lsa_policy);
    INPUT_ARG_PTR(sid_array);
    INPUT_ARG_PTR(domains);
    INPUT_ARG_PTR(trans_names);
    INPUT_ARG_UINT(level);

    CALL_MSRPC(status = LsaLookupSids(lsa_b, &lsa_policy, &sid_array,
                                      &domains, &trans_names, level,
                                      &names_count));

    OUTPUT_ARG_UINT(names_count);

    if (domains) {
        LsaRpcFreeMemory((void*)domains);
        domains = NULL;
    }

    status = LsaClose(lsa_b, &lsa_policy);

    FreeLsaBinding(&lsa_b);

    RELEASE_SESSION_CREDS(nr);

    for (i = 0; i < sid_array.num_sids; i++) {
        SAFE_FREE(sid_array.sids[i].sid);
    }

done:
    if (sids) LsaRpcFreeMemory((void*)sids);
    if (trans_names) LsaRpcFreeMemory((void*)trans_names);

    LsaRpcDestroyMemory();

    SAFE_FREE(sid_array.sids);
    SAFE_FREE(names[0]);
    SAFE_FREE(username);
    SAFE_FREE(domname);

    return true;
}




int TestLsaLookupNames2(struct test *t, const wchar16_t *hostname,
                        const wchar16_t *user, const wchar16_t *pass,
                        struct parameter *options, int optcount)
{
    const uint32 access_rights = LSA_ACCESS_LOOKUP_NAMES_SIDS;
    const uint32 num_names = 1;
    const char *def_username = "BUILTIN\\Guest";
    const uint32 def_revlookup = 0;

    NTSTATUS status = STATUS_SUCCESS;
    enum param_err perr;
    handle_t lsa_b;
    NETRESOURCE nr = {0};
    wchar16_t *domname, buffer[512], *names[2];
    wchar16_t *username;
    uint32 revlookup;
    PolicyHandle lsa_policy = {0};
    RefDomainList *domains = NULL;
    TranslatedSid2 *sids = NULL;
    SidArray sid_array = {0};
    TranslatedName *trans_names = NULL;
    uint32 level, names_count, sids_count;
    int i;

    TESTINFO(t, hostname, user, pass);

    SET_SESSION_CREDS(nr, hostname, user, pass);

    perr = fetch_value(options, optcount, "username", pt_w16string, &username,
                       &def_username);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "revlookup", pt_uint32, &revlookup,
                       &def_revlookup);
    if (!perr_is_ok(perr)) perr_fail(perr);

    lsa_b = CreateLsaBinding(&lsa_b, hostname);
    if (lsa_b == NULL) return false;
    
    status = GetSamDomainName(&domname, hostname);
    if (status != 0) rpc_fail(status);

    names[0] = (wchar16_t*) wc16sdup(username);

    status = LsaOpenPolicy2(lsa_b, hostname, NULL, access_rights,
                            &lsa_policy);
    if (status != 0) rpc_fail(status);

    /* Lookup name to sid */
    level = 1;

    INPUT_ARG_PTR(lsa_b);
    INPUT_ARG_PTR(&lsa_policy);
    INPUT_ARG_UINT(num_names);
    INPUT_ARG_WSTR(names[0]);
    INPUT_ARG_WSTR(names[1]);
    INPUT_ARG_PTR(domains);
    INPUT_ARG_PTR(sids);
    INPUT_ARG_UINT(level);

    CALL_MSRPC(status = LsaLookupNames2(lsa_b, &lsa_policy, num_names, names,
                                        &domains, &sids, level, &sids_count));

    OUTPUT_ARG_UINT(sids_count);

    if (!revlookup) goto done;

    /* Reverse lookup sid to name */
    sid_array.num_sids = sids_count;
    sid_array.sids = (SidPtr*) malloc(sid_array.num_sids * sizeof(SidPtr));

    for (i = 0; i < sid_array.num_sids; i++) {
        DomSid *usr_sid, *dom_sid;
        uint32 sid_index;

        dom_sid = NULL;
        sid_index = sids[i].index;
		
        if (sid_index < domains->count) {
            dom_sid = domains->domains[sid_index].sid;
            SidAllocateResizedCopy(&usr_sid,
                                   dom_sid->subauth_count + 1,
                                   dom_sid);
            usr_sid->subauth[usr_sid->subauth_count - 1] = sids[i].rid;
            sid_array.sids[i].sid = usr_sid;
        }
    }

    if (domains) {
        LsaRpcFreeMemory((void*)domains);
        domains = NULL;
    }

    level = 6;

    INPUT_ARG_PTR(lsa_b);
    INPUT_ARG_PTR(&lsa_policy);
    INPUT_ARG_PTR(sid_array);
    INPUT_ARG_PTR(domains);
    INPUT_ARG_PTR(trans_names);
    INPUT_ARG_UINT(level);

    CALL_MSRPC(status = LsaLookupSids(lsa_b, &lsa_policy, &sid_array,
                                      &domains, &trans_names, level,
                                      &names_count));

    OUTPUT_ARG_UINT(names_count);

    if (domains) {
        LsaRpcFreeMemory((void*)domains);
        domains = NULL;
    }

    status = LsaClose(lsa_b, &lsa_policy);

    FreeLsaBinding(&lsa_b);

    RELEASE_SESSION_CREDS(nr);

    for (i = 0; i < sid_array.num_sids; i++) {
        SAFE_FREE(sid_array.sids[i].sid);
    }

done:
    if (trans_names) LsaRpcFreeMemory((void*)trans_names);

    LsaRpcDestroyMemory();

    SAFE_FREE(sid_array.sids);
    SAFE_FREE(names[0]);
    SAFE_FREE(username);
    SAFE_FREE(domname);

    return true;
}


int TestLsaLookupSids(struct test *t, const wchar16_t *hostname,
                      const wchar16_t *user, const wchar16_t *pass,
                      struct parameter *options, int optcount)
{
    const uint32 access_rights = LSA_ACCESS_LOOKUP_NAMES_SIDS;

    const char *def_input_sid = "S-1-5-21-500";

    NTSTATUS status = STATUS_SUCCESS;
    enum param_err perr;
    handle_t lsa_b = NULL;
    NETRESOURCE nr = {0};
    DomSid *input_sid = NULL;
    wchar16_t *domname = NULL;
    wchar16_t buffer[512] = {0};
    wchar16_t *names[2] = {0};
    wchar16_t *adminname = NULL;
    wchar16_t *guestname = NULL;
    PolicyHandle lsa_policy = {0};
    RefDomainList *domains = NULL;
    SidArray sid_array = {0};
    TranslatedName *trans_names = NULL;
    uint32 level = 0;
    uint32 count = 0;
    int i = 0;

    TESTINFO(t, hostname, user, pass);

    SET_SESSION_CREDS(nr, hostname, user, pass);

    perr = fetch_value(options, optcount, "sid", pt_sid, &input_sid,
                       &def_input_sid);
    if (!perr_is_ok(perr)) perr_fail(perr);

    lsa_b = CreateLsaBinding(&lsa_b, hostname);
    if (lsa_b == NULL) return false;
    
    status = GetSamDomainName(&domname, hostname);
    if (status != 0) rpc_fail(status);

    status = LsaOpenPolicy2(lsa_b, hostname, NULL, access_rights,
                            &lsa_policy);
    if (status != 0) rpc_fail(status);

    /* Prepare SID to name lookup */
    sid_array.num_sids = 1;
    sid_array.sids = (SidPtr*) malloc(sizeof(SidPtr));

    sid_array.sids[0].sid = input_sid;

    level = 1;

    wchar16_t *sidstr = NULL;
    SidToString(input_sid, &sidstr);
    INPUT_ARG_WSTR(sidstr);

    INPUT_ARG_PTR(lsa_b);
    INPUT_ARG_PTR(&lsa_policy);
    INPUT_ARG_PTR(sid_array);
    INPUT_ARG_PTR(domains);
    INPUT_ARG_PTR(names);
    INPUT_ARG_UINT(level);

    CALL_MSRPC(status = LsaLookupSids(lsa_b, &lsa_policy, &sid_array,
                                      &domains, &trans_names, level, &count));

    OUTPUT_ARG_UINT(count);

    if (trans_names) {
        status = LsaRpcFreeMemory((void*)trans_names);
        if (status != 0) rpc_fail(status);
    }

done:
    status = LsaClose(lsa_b, &lsa_policy);

    FreeLsaBinding(&lsa_b);

    RELEASE_SESSION_CREDS(nr);

    LsaRpcDestroyMemory();

    SAFE_FREE(input_sid);
    SAFE_FREE(sid_array.sids);

    SAFE_FREE(domname);

    return true;
}


int TestLsaQueryInfoPolicy(struct test *t, const wchar16_t *hostname,
                           const wchar16_t *user, const wchar16_t *pass,
                           struct parameter *options, int optcount)
{
    const uint32 access_rights = LSA_ACCESS_LOOKUP_NAMES_SIDS |
                                 LSA_ACCESS_ENABLE_LSA |
                                 LSA_ACCESS_ADMIN_AUDIT_LOG_ATTRS |
                                 LSA_ACCESS_CHANGE_SYS_AUDIT_REQS |
                                 LSA_ACCESS_SET_DEFAULT_QUOTA |
                                 LSA_ACCESS_CREATE_PRIVILEGE |
                                 LSA_ACCESS_CREATE_SECRET_OBJECT |
                                 LSA_ACCESS_CREATE_SPECIAL_ACCOUNTS |
                                 LSA_ACCESS_CHANGE_DOMTRUST_RELATION |
                                 LSA_ACCESS_GET_SENSITIVE_POLICY_INFO |
                                 LSA_ACCESS_VIEW_SYS_AUDIT_REQS |
                                 LSA_ACCESS_VIEW_POLICY_INFO;

    const uint32 def_level = 0;

    NTSTATUS status = STATUS_SUCCESS;
    enum param_err perr;
    handle_t lsa_b = NULL;
    NETRESOURCE nr = {0};
    DomSid *input_sid = NULL;
    wchar16_t *domname = NULL;
    PolicyHandle lsa_policy = {0};
    LsaPolicyInformation *info = NULL;
    uint32 level = 0;

    TESTINFO(t, hostname, user, pass);

    SET_SESSION_CREDS(nr, hostname, user, pass);

    perr = fetch_value(options, optcount, "level", pt_uint32, &level,
                       &def_level);
    if (!perr_is_ok(perr)) perr_fail(perr);

    lsa_b = CreateLsaBinding(&lsa_b, hostname);
    if (lsa_b == NULL) return false;
    
    status = GetSamDomainName(&domname, hostname);
    if (status != 0) rpc_fail(status);

    status = LsaOpenPolicy2(lsa_b, hostname, NULL, access_rights,
                            &lsa_policy);
    if (status != 0) rpc_fail(status);

    /*
     * level = 1 doesn't work yet for some reason (unmarshalling error probably)
     */

    if (level) {
        if (level == 1) {
            printf("Level %d unsupported. Exiting...\n", level);
            goto close;
        }

        INPUT_ARG_PTR(lsa_b);
        INPUT_ARG_PTR(&lsa_policy);
        INPUT_ARG_UINT(level);
        INPUT_ARG_PTR(&info);

        CALL_MSRPC(status = LsaQueryInfoPolicy(lsa_b, &lsa_policy,
                                               level, &info));
        OUTPUT_ARG_PTR(&info);

        if (info) {
            status = LsaRpcFreeMemory((void*)info);
            if (status != 0) rpc_fail(status);
        }

    } else {
        for (level = 1; level <= 12; level++) {
            if (level == 1) continue;

            INPUT_ARG_PTR(lsa_b);
            INPUT_ARG_PTR(&lsa_policy);
            INPUT_ARG_UINT(level);
            INPUT_ARG_PTR(&info);

            CALL_MSRPC(status = LsaQueryInfoPolicy(lsa_b, &lsa_policy,
                                                  level, &info));
            OUTPUT_ARG_PTR(&info);

            if (info) {
                status = LsaRpcFreeMemory((void*)info);
                if (status != 0) rpc_fail(status);
            }

            info = NULL;
        }
    }

close:
    status = LsaClose(lsa_b, &lsa_policy);
    FreeLsaBinding(&lsa_b);

    RELEASE_SESSION_CREDS(nr);

done:
    SAFE_FREE(domname);

    status = LsaRpcDestroyMemory();

    return (status == STATUS_SUCCESS);
}


int TestLsaQueryInfoPolicy2(struct test *t, const wchar16_t *hostname,
                            const wchar16_t *user, const wchar16_t *pass,
                            struct parameter *options, int optcount)
{
    const uint32 access_rights = LSA_ACCESS_LOOKUP_NAMES_SIDS |
                                 LSA_ACCESS_ENABLE_LSA |
                                 LSA_ACCESS_ADMIN_AUDIT_LOG_ATTRS |
                                 LSA_ACCESS_CHANGE_SYS_AUDIT_REQS |
                                 LSA_ACCESS_SET_DEFAULT_QUOTA |
                                 LSA_ACCESS_CREATE_PRIVILEGE |
                                 LSA_ACCESS_CREATE_SECRET_OBJECT |
                                 LSA_ACCESS_CREATE_SPECIAL_ACCOUNTS |
                                 LSA_ACCESS_CHANGE_DOMTRUST_RELATION |
                                 LSA_ACCESS_GET_SENSITIVE_POLICY_INFO |
                                 LSA_ACCESS_VIEW_SYS_AUDIT_REQS |
                                 LSA_ACCESS_VIEW_POLICY_INFO;

    const uint32 def_level = 0;

    NTSTATUS status = STATUS_SUCCESS;
    enum param_err perr;
    handle_t lsa_b = NULL;
    NETRESOURCE nr = {0};
    DomSid *input_sid = NULL;
    wchar16_t *domname = NULL;
    PolicyHandle lsa_policy = {0};
    LsaPolicyInformation *info = NULL;
    uint32 level = 0;

    TESTINFO(t, hostname, user, pass);

    SET_SESSION_CREDS(nr, hostname, user, pass);

    perr = fetch_value(options, optcount, "level", pt_uint32, &level,
                       &def_level);
    if (!perr_is_ok(perr)) perr_fail(perr);

    lsa_b = CreateLsaBinding(&lsa_b, hostname);
    if (lsa_b == NULL) return false;
    
    status = GetSamDomainName(&domname, hostname);
    if (status != 0) rpc_fail(status);

    status = LsaOpenPolicy2(lsa_b, hostname, NULL, access_rights,
                            &lsa_policy);
    if (status != 0) rpc_fail(status);

    /*
     * level = 1 doesn't work yet for some reason (unmarshalling error probably)
     */

    if (level) {
        if (level == 1) {
            printf("Level %d unsupported. Exiting...\n", level);
            goto close;
        }

        INPUT_ARG_PTR(lsa_b);
        INPUT_ARG_PTR(&lsa_policy);
        INPUT_ARG_UINT(level);
        INPUT_ARG_PTR(&info);

        CALL_MSRPC(status = LsaQueryInfoPolicy2(lsa_b, &lsa_policy,
                                                level, &info));
        OUTPUT_ARG_PTR(&info);

        if (info) {
            status = LsaRpcFreeMemory((void*)info);
            if (status != 0) rpc_fail(status);
        }

    } else {
        for (level = 1; level <= 12; level++) {
            if (level == 1) continue;

            INPUT_ARG_PTR(lsa_b);
            INPUT_ARG_PTR(&lsa_policy);
            INPUT_ARG_UINT(level);
            INPUT_ARG_PTR(info);

            CALL_MSRPC(status = LsaQueryInfoPolicy2(lsa_b, &lsa_policy,
                                                    level, &info));
            OUTPUT_ARG_PTR(info);

            if (info) {
                status = LsaRpcFreeMemory((void*)info);
                if (status != 0) rpc_fail(status);
            }

            info = NULL;
        }
    }

close:
    status = LsaClose(lsa_b, &lsa_policy);
    FreeLsaBinding(&lsa_b);

    RELEASE_SESSION_CREDS(nr);

done:
    SAFE_FREE(domname);

    status = LsaRpcDestroyMemory();

    return (status == STATUS_SUCCESS);
}


void SetupLsaTests(struct test *t)
{
    LsaRpcInitMemory();


    AddTest(t, "LSA-OPEN-POLICY", TestLsaOpenPolicy);
    AddTest(t, "LSA-LOOKUP-NAMES", TestLsaLookupNames);
    AddTest(t, "LSA-LOOKUP-NAMES2", TestLsaLookupNames2);
    AddTest(t, "LSA-LOOKUP-SIDS", TestLsaLookupSids);
    AddTest(t, "LSA-QUERY-INFO-POL", TestLsaQueryInfoPolicy);
    AddTest(t, "LSA-QUERY-INFO-POL2", TestLsaQueryInfoPolicy2);
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
