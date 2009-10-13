/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software
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


BOOL
CallLsaOpenPolicy(
    handle_t hBinding,
    PWSTR pwszSysName,
    POLICY_HANDLE *phPolicy
    );


handle_t CreateLsaBinding(handle_t *binding, const wchar16_t *host)
{
    RPCSTATUS status = RPC_S_OK;
    size_t hostname_size = 0;
    char *hostname = NULL;
    PIO_CREDS creds = NULL;

    if (binding == NULL || host == NULL) return NULL;

    hostname_size = wc16slen(host) + 1;
    hostname = (char*) malloc(hostname_size * sizeof(char));
    if (hostname == NULL) return NULL;
    wc16stombs(hostname, host, hostname_size);

    if (LwIoGetThreadCreds(&creds) != STATUS_SUCCESS)
    {
        return NULL;
    }

    status = InitLsaBindingDefault(binding, hostname, creds);
    if (status != RPC_S_OK) {
        int result;
        unsigned char errmsg[dce_c_error_string_len];

        dce_error_inq_text(status, errmsg, &result);
        if (result == 0) {
            printf("Error: %s\n", errmsg);
        } else {
            printf("Unknown error: %08lx\n", (unsigned long int)status);
        }

        return NULL;
    }

    if (creds) {
        LwIoDeleteCreds(creds);
    }

    SAFE_FREE(hostname);
    return *binding;
}


int TestLsaOpenPolicy(struct test *t, const wchar16_t *hostname,
                      const wchar16_t *user, const wchar16_t *pass,
                      struct parameter *options, int optcount)
{
    const uint32 access_rights = LSA_ACCESS_LOOKUP_NAMES_SIDS;

    int ret = true;
    NTSTATUS status = STATUS_SUCCESS;
    handle_t lsa_b = NULL;
    POLICY_HANDLE hPolicy = NULL;

    TESTINFO(t, hostname, user, pass);

    SET_SESSION_CREDS(hCreds);

    lsa_b = CreateLsaBinding(&lsa_b, hostname);
    if (lsa_b == NULL) test_fail(("Test failed: couldn't create lsa binding\n"));

    INPUT_ARG_PTR(lsa_b);
    INPUT_ARG_WSTR(hostname);
    INPUT_ARG_UINT(access_rights);

    CALL_MSRPC(status = LsaOpenPolicy2(lsa_b, hostname, NULL,
                                       access_rights, &hPolicy));
    if (status != 0) rpc_fail(status);

    OUTPUT_ARG_PTR(lsa_b);
    OUTPUT_ARG_PTR(hPolicy);

    status = LsaClose(lsa_b, hPolicy);
    if (status != 0) rpc_fail(status);

    FreeLsaBinding(&lsa_b);

    RELEASE_SESSION_CREDS;

done:
    LsaRpcDestroyMemory();

    return ret;
}


int TestLsaLookupNames(struct test *t, const wchar16_t *hostname,
                       const wchar16_t *user, const wchar16_t *pass,
                       struct parameter *options, int optcount)
{
    const uint32 access_rights = LSA_ACCESS_LOOKUP_NAMES_SIDS;
    const char *def_usernames = "[BUILTIN\\Users:BUILTIN\\Administrators]";
    const uint32 def_revlookup = 0;

    int ret = true;
    NTSTATUS status = STATUS_SUCCESS;
    enum param_err perr = perr_success;
    handle_t lsa_b = NULL;
    wchar16_t *domname = NULL;
    wchar16_t **names = NULL;
    uint32 num_names = 0;
    wchar16_t **usernames = NULL;
    int usernames_count = 0;
    uint32 revlookup = 0;
    POLICY_HANDLE hPolicy = NULL;
    RefDomainList *domains = NULL;
    TranslatedSid *sids = NULL;
    SidArray sid_array = {0};
    TranslatedName *trans_names = NULL;
    uint32 level, names_count, sids_count;
    int i = 0;

    TESTINFO(t, hostname, user, pass);

    SET_SESSION_CREDS(hCreds);

    perr = fetch_value(options, optcount, "usernames", pt_w16string_list,
                       &usernames, &def_usernames);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "revlookup", pt_uint32, &revlookup,
                       &def_revlookup);
    if (!perr_is_ok(perr)) perr_fail(perr);

    lsa_b = CreateLsaBinding(&lsa_b, hostname);
    if (lsa_b == NULL) test_fail(("Test failed: couldn't create lsa binding\n"));

    status = LsaOpenPolicy2(lsa_b, hostname, NULL, access_rights,
                            &hPolicy);
    if (status != 0) rpc_fail(status);

    while (usernames[usernames_count++]);
    names = (wchar16_t**) malloc(sizeof(wchar16_t*) * usernames_count);
    test_fail_if_no_memory(names);

    memset(names, 0, sizeof(wchar16_t*) * usernames_count);
    num_names = usernames_count - 1;

    for (i = 0; i < (int)num_names; i++) {
        names[i] = (wchar16_t*) wc16sdup(usernames[i]);
        test_fail_if_no_memory(names[i]);
    }

    /* Lookup name to sid */
    level = 1;

    INPUT_ARG_PTR(lsa_b);
    INPUT_ARG_PTR(hPolicy);
    INPUT_ARG_UINT(num_names);

    for (i = 0; i < num_names; i++) {
        INPUT_ARG_WSTR(names[i]);
    }

    INPUT_ARG_PTR(domains);
    INPUT_ARG_PTR(sids);
    INPUT_ARG_UINT(level);

    CALL_MSRPC(status = LsaLookupNames(lsa_b, hPolicy, num_names, names,
                                       &domains, &sids, level, &sids_count));

    OUTPUT_ARG_UINT(sids_count);

    if (!revlookup) goto done;

    /* Reverse lookup sid to name */
    sid_array.num_sids = sids_count;
    sid_array.sids = (SidPtr*) malloc(sid_array.num_sids * sizeof(SidPtr));

    for (i = 0; i < sid_array.num_sids; i++) {
        PSID usr_sid;
        PSID dom_sid;
        uint32 sid_index;

        dom_sid = NULL;
        sid_index = sids[i].index;

        if (sid_index < domains->count) {
            dom_sid = domains->domains[sid_index].sid;
            MsRpcAllocateSidAppendRid(&usr_sid,
                                      dom_sid,
                                      sids[i].rid);
            sid_array.sids[i].sid = usr_sid;
        }
    }

    if (domains) {
        LsaRpcFreeMemory((void*)domains);
        domains = NULL;
    }

    level = 6;

    INPUT_ARG_PTR(lsa_b);
    INPUT_ARG_PTR(hPolicy);
    INPUT_ARG_PTR(&sid_array);
    INPUT_ARG_PTR(domains);
    INPUT_ARG_PTR(trans_names);
    INPUT_ARG_UINT(level);

    CALL_MSRPC(status = LsaLookupSids(lsa_b, hPolicy, &sid_array,
                                      &domains, &trans_names, level,
                                      &names_count));

    OUTPUT_ARG_UINT(names_count);

    if (domains) {
        LsaRpcFreeMemory((void*)domains);
        domains = NULL;
    }

    status = LsaClose(lsa_b, hPolicy);

    FreeLsaBinding(&lsa_b);

    RELEASE_SESSION_CREDS;

    for (i = 0; i < sid_array.num_sids; i++) {
        SAFE_FREE(sid_array.sids[i].sid);
    }

done:
    if (sids) {
        LsaRpcFreeMemory((void*)sids);
    }

    if (trans_names) {
        LsaRpcFreeMemory((void*)trans_names);
    }

    LsaRpcDestroyMemory();

    SAFE_FREE(sid_array.sids);

    if (names) {
        i = 0;
        while (names[i]) {
            SAFE_FREE(names[i]);
            i++;
        }
        SAFE_FREE(names);
    }

    if (usernames) {
        i = 0;
        while (usernames[i]) {
            SAFE_FREE(usernames[i]);
            i++;
        }
        SAFE_FREE(usernames);
    }

    SAFE_FREE(domname);

    return ret;
}




int TestLsaLookupNames2(struct test *t, const wchar16_t *hostname,
                        const wchar16_t *user, const wchar16_t *pass,
                        struct parameter *options, int optcount)
{
    const uint32 access_rights = LSA_ACCESS_LOOKUP_NAMES_SIDS;
    const char *def_username = "[BUILTIN\\Users:BUILTIN\\Administrators]";
    const uint32 def_revlookup = 0;

    int ret = true;
    NTSTATUS status = STATUS_SUCCESS;
    enum param_err perr = perr_success;
    handle_t lsa_b = NULL;
    wchar16_t *domname = NULL;
    wchar16_t **names = NULL;
    uint32 num_names = 0;
    wchar16_t **usernames = NULL;
    int usernames_count = 0;
    uint32 revlookup;
    POLICY_HANDLE hPolicy = NULL;
    RefDomainList *domains = NULL;
    TranslatedSid2 *sids = NULL;
    SidArray sid_array = {0};
    TranslatedName *trans_names = NULL;
    uint32 level, names_count, sids_count;
    int i = 0;

    TESTINFO(t, hostname, user, pass);

    SET_SESSION_CREDS(hCreds);

    perr = fetch_value(options, optcount, "usernames", pt_w16string_list,
                       &usernames, &def_username);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "revlookup", pt_uint32, &revlookup,
                       &def_revlookup);
    if (!perr_is_ok(perr)) perr_fail(perr);

    lsa_b = CreateLsaBinding(&lsa_b, hostname);
    if (lsa_b == NULL) test_fail(("Test failed: couldn't create lsa binding\n"));

    status = GetSamDomainName(&domname, hostname);
    if (status != 0) rpc_fail(status);

    status = LsaOpenPolicy2(lsa_b, hostname, NULL, access_rights,
                            &hPolicy);
    if (status != 0) rpc_fail(status);

    while (usernames[usernames_count++]);
    names = (wchar16_t**) malloc(sizeof(wchar16_t*) * usernames_count);
    test_fail_if_no_memory(names);

    memset(names, 0, sizeof(wchar16_t*) * usernames_count);
    num_names = usernames_count - 1;

    for (i = 0; i < (int)num_names; i++) {
        names[i] = (wchar16_t*) wc16sdup(usernames[i]);
        test_fail_if_no_memory(names[i]);
    }

    /* Lookup name to sid */
    level = 1;

    INPUT_ARG_PTR(lsa_b);
    INPUT_ARG_PTR(hPolicy);
    INPUT_ARG_UINT(num_names);

    for (i = 0; i < num_names; i++) {
        INPUT_ARG_WSTR(names[i]);
    }

    INPUT_ARG_PTR(domains);
    INPUT_ARG_PTR(sids);
    INPUT_ARG_UINT(level);

    CALL_MSRPC(status = LsaLookupNames2(lsa_b, hPolicy, num_names, names,
                                        &domains, &sids, level, &sids_count));

    OUTPUT_ARG_UINT(sids_count);

    if (!revlookup) goto done;

    /* Reverse lookup sid to name */
    sid_array.num_sids = sids_count;
    sid_array.sids = (SidPtr*) malloc(sid_array.num_sids * sizeof(SidPtr));

    for (i = 0; i < sid_array.num_sids; i++) {
        PSID usr_sid;
        PSID dom_sid;
        uint32 sid_index;
        wchar16_t *sidstr = NULL;

        dom_sid = NULL;
        sid_index = sids[i].index;

        if (sid_index < domains->count) {
            dom_sid = domains->domains[sid_index].sid;
            MsRpcAllocateSidAppendRid(&usr_sid,
                                      dom_sid,
                                      sids[i].rid);
            sid_array.sids[i].sid = usr_sid;

            RtlAllocateWC16StringFromSid(&sidstr, usr_sid);
            DUMP_WSTR(" ", sidstr);

            RTL_FREE(&sidstr);
        }
    }

    if (domains) {
        LsaRpcFreeMemory((void*)domains);
        domains = NULL;
    }

    level = 6;

    INPUT_ARG_PTR(lsa_b);
    INPUT_ARG_PTR(hPolicy);
    INPUT_ARG_PTR(&sid_array);
    INPUT_ARG_PTR(domains);
    INPUT_ARG_PTR(trans_names);
    INPUT_ARG_UINT(level);

    CALL_MSRPC(status = LsaLookupSids(lsa_b, hPolicy, &sid_array,
                                      &domains, &trans_names, level,
                                      &names_count));

    OUTPUT_ARG_UINT(names_count);

    if (domains) {
        LsaRpcFreeMemory((void*)domains);
        domains = NULL;
    }

    status = LsaClose(lsa_b, hPolicy);

    FreeLsaBinding(&lsa_b);

    RELEASE_SESSION_CREDS;

    for (i = 0; i < sid_array.num_sids; i++) {
        SAFE_FREE(sid_array.sids[i].sid);
    }

done:
    if (trans_names) {
        LsaRpcFreeMemory((void*)trans_names);
    }

    LsaRpcDestroyMemory();

    SAFE_FREE(sid_array.sids);

    if (names) {
        i = 0;
        while (names[i]) {
            SAFE_FREE(names[i]);
            i++;
        }
        SAFE_FREE(names);
    }

    if (usernames) {
        i = 0;
        while (usernames[i]) {
            SAFE_FREE(usernames[i]);
            i++;
        }
        SAFE_FREE(usernames);
    }

    SAFE_FREE(domname);

    return ret;
}


int TestLsaLookupNames3(struct test *t, const wchar16_t *hostname,
                        const wchar16_t *user, const wchar16_t *pass,
                        struct parameter *options, int optcount)
{
    const uint32 access_rights = LSA_ACCESS_LOOKUP_NAMES_SIDS;
    const char *def_username = "[BUILTIN\\Users:BUILTIN\\Administrators]";
    const uint32 def_revlookup = 0;

    int ret = true;
    NTSTATUS status = STATUS_SUCCESS;
    enum param_err perr = perr_success;
    handle_t lsa_b = NULL;
    wchar16_t *domname = NULL;
    wchar16_t **names = NULL;
    uint32 num_names = 0;
    wchar16_t **usernames = NULL;
    int usernames_count = 0;
    uint32 revlookup;
    POLICY_HANDLE hPolicy = NULL;
    RefDomainList *domains = NULL;
    TranslatedSid3 *sids = NULL;
    SidArray sid_array = {0};
    TranslatedName *trans_names = NULL;
    uint32 level, names_count, sids_count;
    int i = 0;

    TESTINFO(t, hostname, user, pass);

    SET_SESSION_CREDS(hCreds);

    perr = fetch_value(options, optcount, "usernames", pt_w16string_list,
                       &usernames, &def_username);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "revlookup", pt_uint32, &revlookup,
                       &def_revlookup);
    if (!perr_is_ok(perr)) perr_fail(perr);

    lsa_b = CreateLsaBinding(&lsa_b, hostname);
    if (lsa_b == NULL) test_fail(("Test failed: couldn't create lsa binding\n"));

    status = GetSamDomainName(&domname, hostname);
    if (status != 0) rpc_fail(status);

    status = LsaOpenPolicy2(lsa_b, hostname, NULL, access_rights,
                            &hPolicy);
    if (status != 0) rpc_fail(status);

    while (usernames[usernames_count++]);
    names = (wchar16_t**) malloc(sizeof(wchar16_t*) * usernames_count);
    test_fail_if_no_memory(names);

    memset(names, 0, sizeof(wchar16_t*) * usernames_count);
    num_names = usernames_count - 1;

    for (i = 0; i < (int)num_names; i++) {
        names[i] = (wchar16_t*) wc16sdup(usernames[i]);
        test_fail_if_no_memory(names[i]);
    }

    /* Lookup name to sid */
    level = 1;

    INPUT_ARG_PTR(lsa_b);
    INPUT_ARG_PTR(hPolicy);
    INPUT_ARG_UINT(num_names);

    for (i = 0; i < num_names; i++) {
        INPUT_ARG_WSTR(names[i]);
    }

    INPUT_ARG_PTR(domains);
    INPUT_ARG_PTR(sids);
    INPUT_ARG_UINT(level);

    CALL_MSRPC(status = LsaLookupNames3(lsa_b, hPolicy, num_names, names,
                                        &domains, &sids, level, &sids_count));

    OUTPUT_ARG_UINT(sids_count);

    if (!revlookup) goto done;

    /* Reverse lookup sid to name */
    sid_array.num_sids = sids_count;
    sid_array.sids = (SidPtr*) malloc(sid_array.num_sids * sizeof(SidPtr));

    for (i = 0; i < sid_array.num_sids; i++) {
        uint32 sid_index;
        wchar16_t *sidstr = NULL;

        sid_index = sids[i].index;

        if (sid_index < domains->count) {
            sid_array.sids[i].sid = sids[i].sid;

            RtlAllocateWC16StringFromSid(&sidstr, sids[i].sid);
            DUMP_WSTR(" ", sidstr);

            RTL_FREE(&sidstr);
        }
    }

    if (domains) {
        LsaRpcFreeMemory((void*)domains);
        domains = NULL;
    }

    level = 6;

    INPUT_ARG_PTR(lsa_b);
    INPUT_ARG_PTR(hPolicy);
    INPUT_ARG_PTR(&sid_array);
    INPUT_ARG_PTR(domains);
    INPUT_ARG_PTR(trans_names);
    INPUT_ARG_UINT(level);

    CALL_MSRPC(status = LsaLookupSids(lsa_b, hPolicy, &sid_array,
                                      &domains, &trans_names, level,
                                      &names_count));

    OUTPUT_ARG_UINT(names_count);

    if (domains) {
        LsaRpcFreeMemory((void*)domains);
        domains = NULL;
    }

    status = LsaClose(lsa_b, hPolicy);

    FreeLsaBinding(&lsa_b);

    RELEASE_SESSION_CREDS;

done:
    if (trans_names) {
        LsaRpcFreeMemory((void*)trans_names);
    }

    if (sids) {
        LsaRpcFreeMemory((void*)sids);
    }

    LsaRpcDestroyMemory();

    SAFE_FREE(sid_array.sids);

    if (names) {
        i = 0;
        while (names[i]) {
            SAFE_FREE(names[i]);
            i++;
        }
        SAFE_FREE(names);
    }

    if (usernames) {
        i = 0;
        while (usernames[i]) {
            SAFE_FREE(usernames[i]);
            i++;
        }
        SAFE_FREE(usernames);
    }

    SAFE_FREE(domname);

    return ret;
}


int TestLsaLookupSids(struct test *t, const wchar16_t *hostname,
                      const wchar16_t *user, const wchar16_t *pass,
                      struct parameter *options, int optcount)
{
    const uint32 access_rights = LSA_ACCESS_LOOKUP_NAMES_SIDS;

    const char *def_input_sids = "[S-1-5-32-544]";

    int ret = true;
    NTSTATUS status = STATUS_SUCCESS;
    enum param_err perr = perr_success;
    handle_t lsa_b = NULL;
    PSID* input_sids = NULL;
    int input_sid_count = 0;
    wchar16_t *domname = NULL;
    wchar16_t *names[2] = {0};
    POLICY_HANDLE hPolicy = NULL;
    RefDomainList *domains = NULL;
    SidArray sid_array = {0};
    TranslatedName *trans_names = NULL;
    wchar16_t *sidstr = NULL;
    uint32 level = 0;
    uint32 count = 0;
    int i = 0;

    TESTINFO(t, hostname, user, pass);

    SET_SESSION_CREDS(hCreds);

    perr = fetch_value(options, optcount, "sids", pt_sid_list, &input_sids,
                       &def_input_sids);
    if (!perr_is_ok(perr)) perr_fail(perr);

    lsa_b = CreateLsaBinding(&lsa_b, hostname);
    if (lsa_b == NULL) test_fail(("Test failed: couldn't create lsa binding\n"));

    status = GetSamDomainName(&domname, hostname);
    if (status != 0) rpc_fail(status);

    status = LsaOpenPolicy2(lsa_b, hostname, NULL, access_rights,
                            &hPolicy);
    if (status != 0) rpc_fail(status);

    /* Count SIDs to resolve */
    while (input_sids[input_sid_count++]);

    /* Prepare SID to name lookup */
    sid_array.num_sids = input_sid_count - 1;
    sid_array.sids = (SidPtr*) malloc(sizeof(SidPtr) * sid_array.num_sids);
    test_fail_if_no_memory(sid_array.sids);

    for (i = 0; i < sid_array.num_sids; i++) {
        sid_array.sids[i].sid = input_sids[i];

        RtlAllocateWC16StringFromSid(&sidstr, input_sids[i]);
        test_fail_if_no_memory(sidstr);

        DUMP_WSTR(" ", sidstr);

        RTL_FREE(&sidstr);
    }

    level = 1;

    INPUT_ARG_PTR(lsa_b);
    INPUT_ARG_PTR(hPolicy);
    INPUT_ARG_PTR(&sid_array);
    INPUT_ARG_PTR(domains);
    INPUT_ARG_PTR(names);
    INPUT_ARG_UINT(level);

    CALL_MSRPC(status = LsaLookupSids(lsa_b, hPolicy, &sid_array,
                                      &domains, &trans_names, level, &count));

    OUTPUT_ARG_UINT(count);

    if (trans_names) {
        status = LsaRpcFreeMemory((void*)trans_names);
        if (status != 0) rpc_fail(status);
    }

    status = LsaClose(lsa_b, hPolicy);

    FreeLsaBinding(&lsa_b);

    RELEASE_SESSION_CREDS;

done:
    LsaRpcDestroyMemory();

    for (i = 0; i < input_sid_count; i++)
    {
        RTL_FREE(&input_sids[i]);
    }

    SAFE_FREE(sid_array.sids);

    SAFE_FREE(input_sids);
    SAFE_FREE(domname);

    return ret;
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

    int ret = true;
    NTSTATUS status = STATUS_SUCCESS;
    enum param_err perr = perr_success;
    handle_t lsa_b = NULL;
    wchar16_t *domname = NULL;
    POLICY_HANDLE hPolicy = NULL;
    LsaPolicyInformation *info = NULL;
    uint32 level = 0;

    TESTINFO(t, hostname, user, pass);

    SET_SESSION_CREDS(hCreds);

    perr = fetch_value(options, optcount, "level", pt_uint32, &level,
                       &def_level);
    if (!perr_is_ok(perr)) perr_fail(perr);

    lsa_b = CreateLsaBinding(&lsa_b, hostname);
    if (lsa_b == NULL) test_fail(("Test failed: couldn't create lsa binding\n"));

    status = GetSamDomainName(&domname, hostname);
    if (status != 0) rpc_fail(status);

    status = LsaOpenPolicy2(lsa_b, hostname, NULL, access_rights,
                            &hPolicy);
    if (status != 0) rpc_fail(status);

    /*
     * level = 1 doesn't work yet for some reason (unmarshalling error probably)
     */

    if (level) {
        if (level == 1) {
            test_fail(("Level %d unsupported. Exiting...\n", level));
        }

        INPUT_ARG_PTR(lsa_b);
        INPUT_ARG_PTR(hPolicy);
        INPUT_ARG_UINT(level);
        INPUT_ARG_PTR(&info);

        CALL_MSRPC(status = LsaQueryInfoPolicy(lsa_b, hPolicy,
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
            INPUT_ARG_PTR(hPolicy);
            INPUT_ARG_UINT(level);
            INPUT_ARG_PTR(&info);

            CALL_MSRPC(status = LsaQueryInfoPolicy(lsa_b, hPolicy,
                                                  level, &info));
            OUTPUT_ARG_PTR(&info);

            if (info) {
                status = LsaRpcFreeMemory((void*)info);
                if (status != 0) rpc_fail(status);
            }

            info = NULL;
        }
    }

    status = LsaClose(lsa_b, hPolicy);
    FreeLsaBinding(&lsa_b);

    RELEASE_SESSION_CREDS;

done:
    SAFE_FREE(domname);

    LsaRpcDestroyMemory();

    return ret;
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

    int ret = true;
    NTSTATUS status = STATUS_SUCCESS;
    enum param_err perr = perr_success;
    handle_t lsa_b = NULL;
    wchar16_t *domname = NULL;
    POLICY_HANDLE hPolicy = NULL;
    LsaPolicyInformation *info = NULL;
    uint32 level = 0;

    TESTINFO(t, hostname, user, pass);

    SET_SESSION_CREDS(hCreds);

    perr = fetch_value(options, optcount, "level", pt_uint32, &level,
                       &def_level);
    if (!perr_is_ok(perr)) perr_fail(perr);

    lsa_b = CreateLsaBinding(&lsa_b, hostname);
    if (lsa_b == NULL) test_fail(("Test failed: couldn't create lsa binding\n"));

    status = GetSamDomainName(&domname, hostname);
    if (status != 0) rpc_fail(status);

    status = LsaOpenPolicy2(lsa_b, hostname, NULL, access_rights,
                            &hPolicy);
    if (status != 0) rpc_fail(status);

    /*
     * level = 1 doesn't work yet for some reason (unmarshalling error probably)
     */

    if (level) {
        if (level == 1) {
            test_fail(("Level %d unsupported. Exiting...\n", level));
        }

        INPUT_ARG_PTR(lsa_b);
        INPUT_ARG_PTR(hPolicy);
        INPUT_ARG_UINT(level);
        INPUT_ARG_PTR(&info);

        CALL_MSRPC(status = LsaQueryInfoPolicy2(lsa_b, hPolicy,
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
            INPUT_ARG_PTR(hPolicy);
            INPUT_ARG_UINT(level);
            INPUT_ARG_PTR(info);

            CALL_MSRPC(status = LsaQueryInfoPolicy2(lsa_b, hPolicy,
                                                    level, &info));
            OUTPUT_ARG_PTR(info);

            if (info) {
                status = LsaRpcFreeMemory((void*)info);
                if (status != 0) rpc_fail(status);
            }

            info = NULL;
        }
    }

    status = LsaClose(lsa_b, hPolicy);
    FreeLsaBinding(&lsa_b);

    RELEASE_SESSION_CREDS;

done:
    SAFE_FREE(domname);

    LsaRpcDestroyMemory();

    return ret;
}


BOOL
CallLsaOpenPolicy(
    handle_t hBinding,
    PWSTR pwszSysName,
    POLICY_HANDLE *phPolicy
    )
{
    BOOL ret = TRUE;
    NTSTATUS status = STATUS_SUCCESS;
    uint32 access_rights = LSA_ACCESS_LOOKUP_NAMES_SIDS |
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
    POLICY_HANDLE hPolicy = NULL;
    LsaPolicyInformation *pPolicyInfo = NULL;
    uint32 i = 0;

    DISPLAY_COMMENT(("Testing LsaOpenPolicy\n"));

    status = LsaOpenPolicy2(hBinding, pwszSysName, NULL,
                            access_rights, &hPolicy);
    if (status) {
        DISPLAY_ERROR(("LsaOpenPolicy error %s\n", NtStatusToName(status)));
        ret = FALSE;
    } else {
        for (i = 1; i <= 12; i++)
        {
            if (i == 1) continue;

            DISPLAY_COMMENT(("Testing LsaQueryInfoPolicy (level = %d)\n", i));

            status = LsaQueryInfoPolicy(hBinding, hPolicy, i, &pPolicyInfo);
            if (status) {
                DISPLAY_ERROR(("LsaQueryInfoPolicy error %s\n",
                               NtStatusToName(status)));
                ret = FALSE;
            }
        }
    }

    DISPLAY_COMMENT(("Testing LsaClose\n"));
    status = LsaClose(hBinding, hPolicy);
    if (status != 0) {
        DISPLAY_ERROR(("LsaClose error %s\n",
                       NtStatusToName(status)));
        ret = FALSE;
        goto done;
    }

    hPolicy = NULL;

    DISPLAY_COMMENT(("Testing LsaOpenPolicy2\n"));

    status = LsaOpenPolicy2(hBinding, pwszSysName, NULL,
                            access_rights, &hPolicy);
    if (status) {
        DISPLAY_ERROR(("LsaOpenPolicy2 error %s\n", NtStatusToName(status)));
        ret = FALSE;
    } else {
        for (i = 1; i <= 12; i++)
        {
            if (i == 1) continue;

            DISPLAY_COMMENT(("Testing LsaQueryInfoPolicy2 (level = %d)\n", i));

            status = LsaQueryInfoPolicy2(hBinding, hPolicy, i, &pPolicyInfo);
            if (status) {
                DISPLAY_ERROR(("LsaQueryInfoPolicy2 error %s\n",
                               NtStatusToName(status)));
                ret = FALSE;
            }
        }
    }

    *phPolicy = hPolicy;

done:
    return ret;
}


BOOL
CallLsaClosePolicy(
    handle_t hBinding,
    POLICY_HANDLE *phPolicy
    )
{
    BOOL ret = TRUE;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    POLICY_HANDLE hPolicy = NULL;

    DISPLAY_COMMENT(("Testing LsaClose\n"));

    hPolicy = *phPolicy;
    ntStatus = LsaClose(hBinding, hPolicy);
    if (ntStatus)
    {
        DISPLAY_COMMENT(("LsaClose error %s\n", NtStatusToName(ntStatus)));
        ret = FALSE;
    }

    return ret;
}


int
TestLsaInfoPolicy(struct test *t, const wchar16_t *hostname,
                      const wchar16_t *user, const wchar16_t *pass,
                      struct parameter *options, int optcount)
{
    PCSTR pszDefSysName = "";

    BOOL ret = TRUE;
    enum param_err perr = perr_success;
    handle_t hBinding = NULL;
    POLICY_HANDLE hPolicy = NULL;
    PWSTR pwszSysName = NULL;

    perr = fetch_value(options, optcount, "systemname", pt_w16string,
                       &pwszSysName, &pszDefSysName);
    if (!perr_is_ok(perr)) perr_fail(perr);

    TESTINFO(t, hostname, user, pass);

    SET_SESSION_CREDS(hCreds);

    CreateLsaBinding(&hBinding, hostname);

    ret &= CallLsaOpenPolicy(hBinding, pwszSysName, &hPolicy);

    ret &= CallLsaClosePolicy(hBinding, &hPolicy);

    FreeLsaBinding(&hBinding);
    RELEASE_SESSION_CREDS;

done:
    SAFE_FREE(pwszSysName);

    return (int)ret;
}


void SetupLsaTests(struct test *t)
{
    NTSTATUS status = STATUS_SUCCESS;

    status = LsaRpcInitMemory();
    if (status) return;

    AddTest(t, "LSA-OPEN-POLICY", TestLsaOpenPolicy);
    AddTest(t, "LSA-LOOKUP-NAMES", TestLsaLookupNames);
    AddTest(t, "LSA-LOOKUP-NAMES2", TestLsaLookupNames2);
    AddTest(t, "LSA-LOOKUP-NAMES3", TestLsaLookupNames3);
    AddTest(t, "LSA-LOOKUP-SIDS", TestLsaLookupSids);
    AddTest(t, "LSA-QUERY-INFO-POL", TestLsaQueryInfoPolicy);
    AddTest(t, "LSA-QUERY-INFO-POL2", TestLsaQueryInfoPolicy2);
    AddTest(t, "LSA-INFO-POLICY", TestLsaInfoPolicy);
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
