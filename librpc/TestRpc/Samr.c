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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if HAVE_STRINGS_H
#include <strings.h>
#endif

#include <compat/rpcstatus.h>
#include <dce/dce_error.h>
#include <dce/smb.h>
#include <wc16str.h>
#include <secdesc/secdesc.h>
#include <lw/ntstatus.h>

#include <lwrpc/types.h>
#include <lwrpc/security.h>
#include <lwrpc/allocate.h>
#include <lwrpc/unicodestring.h>
#include <lwrpc/samr.h>
#include <lwrpc/lsa.h>
#include <lwrpc/mpr.h>
#include <md5.h>
#include <rc4.h>
#include <des.h>
#include <crypto.h>

#include "TestRpc.h"
#include "Params.h"

extern int verbose_mode;


handle_t CreateSamrBinding(handle_t *binding, const wchar16_t *host)
{
    RPCSTATUS status = RPC_S_OK;
    size_t hostname_size = 0;
    char *hostname = NULL;
    PIO_ACCESS_TOKEN access_token = NULL;

    if (binding == NULL || host == NULL) return NULL;

    hostname_size = wc16slen(host) + 1;
    hostname = (char*) malloc(hostname_size * sizeof(char));
    if (hostname == NULL) return NULL;

    wc16stombs(hostname, host, hostname_size);


    if (LwIoGetThreadAccessToken(&access_token) != STATUS_SUCCESS)
    {
        return NULL;
    }

    status = InitSamrBindingDefault(binding, hostname, access_token);
    if (status != RPC_S_OK) {
        int result;
        unsigned char errmsg[dce_c_error_string_len];
	
        dce_error_inq_text(status, errmsg, &result);
        if (result == 0) {
            printf("Error: %s\n", errmsg);
        } else {
            printf("Unknown error: %08lx\n", (unsigned long int)status);
        }

        SAFE_FREE(hostname);
        return NULL;
    }

    SAFE_FREE(hostname);
    return *binding;
}

static
void
GetSessionKey(handle_t binding, unsigned char** sess_key,
              unsigned16* sess_key_len, unsigned32* st)
{
    rpc_transport_info_handle_t info = NULL;

    rpc_binding_inq_transport_info(binding, &info, st);
    if (*st)
    {
        goto error;
    }

    rpc_smb_transport_info_inq_session_key(info, sess_key,
                                           sess_key_len);

cleanup:
    return;

error:
    *sess_key     = NULL;
    *sess_key_len = 0;
    goto cleanup;
}

/*
  Utility function for getting SAM domain name given a hostname
*/
NTSTATUS GetSamDomainName(wchar16_t **domname, const wchar16_t *hostname)
{
    const uint32 conn_access = SAMR_ACCESS_OPEN_DOMAIN |
                               SAMR_ACCESS_ENUM_DOMAINS;
    const uint32 enum_size = 32;
    const char *builtin = "Builtin";

    NTSTATUS status = STATUS_SUCCESS;
    NTSTATUS ret = STATUS_SUCCESS;
    handle_t samr_binding = NULL;
    PolicyHandle conn_handle = {0};
    uint32 resume = 0;
    uint32 count = 0;
    uint32 i = 0;
    wchar16_t **dom_names = NULL;

    if (domname == NULL || hostname == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    samr_binding = CreateSamrBinding(&samr_binding, hostname);
    if (samr_binding == NULL) return STATUS_UNSUCCESSFUL;

    status = SamrConnect2(samr_binding, hostname, conn_access, &conn_handle);
    if (status != 0) rpc_fail(status);

    dom_names = NULL;

    do {
        status = SamrEnumDomains(samr_binding, &conn_handle, &resume,
                                 enum_size, &dom_names, &count);
        if (status != STATUS_SUCCESS &&
            status != STATUS_MORE_ENTRIES) rpc_fail(status);

        for (i = 0; i < count; i++) {
            char n[32] = {0};

            wc16stombs(n, dom_names[i], sizeof(n));
            if (strcasecmp(n, builtin)) {
                *domname = (wchar16_t*) wc16sdup(dom_names[i]);
                ret = STATUS_SUCCESS;

                SamrFreeMemory((void*)dom_names);
                goto found;
            }
        }

        SamrFreeMemory((void*)dom_names);

    } while (status == STATUS_MORE_ENTRIES);

    *domname = NULL;
    ret = STATUS_NOT_FOUND;

found:
    status = SamrClose(samr_binding, &conn_handle);
    if (status != 0) return status;

done:
    FreeSamrBinding(&samr_binding);
    
    return status;
}


/*
  Utility function for getting SAM domain SID given a hostname
*/
NTSTATUS GetSamDomainSid(DomSid **sid, const wchar16_t *hostname)
{
    const uint32 conn_access = SAMR_ACCESS_OPEN_DOMAIN |
                               SAMR_ACCESS_ENUM_DOMAINS;

    NTSTATUS status = STATUS_SUCCESS;
    handle_t samr_b = NULL;
    PolicyHandle conn_h = {0};
    wchar16_t *domname = NULL;
    DomSid *out_sid = NULL;

    if (sid == NULL || hostname == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    status = GetSamDomainName(&domname, hostname);
    if (status != 0) goto done;

    samr_b = CreateSamrBinding(&samr_b, hostname);
    if (samr_b == NULL) rpc_fail(STATUS_UNSUCCESSFUL);

    status = SamrConnect2(samr_b, hostname, conn_access, &conn_h);
    if (status != 0) rpc_fail(status);

    status = SamrLookupDomain(samr_b, &conn_h, domname, &out_sid);
    if (status != 0) rpc_fail(status);

    status = SamrClose(samr_b, &conn_h);
    if (status != 0) rpc_fail(status);

    /* Allocate a copy of sid so it can be freed clean by the caller */
    RtlSidCopyAlloc(sid, out_sid);

done:
    FreeSamrBinding(&samr_b);

    if (out_sid) {
        SamrFreeMemory((void*)out_sid);
    }

    SAFE_FREE(domname);

    return status;
}


NTSTATUS EnsureUserAccount(const wchar16_t *hostname, wchar16_t *username,
                           int *created)
{
    
    const uint32 account_flags = ACB_NORMAL;
    const uint32 conn_access = SAMR_ACCESS_OPEN_DOMAIN |
                               SAMR_ACCESS_ENUM_DOMAINS;
    const uint32 domain_access = DOMAIN_ACCESS_OPEN_ACCOUNT |
                                 DOMAIN_ACCESS_ENUM_ACCOUNTS |
                                 DOMAIN_ACCESS_CREATE_USER |
                                 DOMAIN_ACCESS_LOOKUP_INFO_2;

    NTSTATUS status = STATUS_SUCCESS;
    DomSid *sid = NULL;
    handle_t samr_b = NULL;
    PolicyHandle conn_h = {0};
    PolicyHandle domain_h = {0};
    PolicyHandle user_h = {0};
    uint32 user_rid = 0;

    if (created) *created = false;

    status = GetSamDomainSid(&sid, hostname);
    if (status != 0) rpc_fail(status);

    CreateSamrBinding(&samr_b, hostname);
    if (samr_b == NULL) goto done;

    status = SamrConnect2(samr_b, hostname, conn_access, &conn_h);
    if (status != 0) rpc_fail(status)
    
    status = SamrOpenDomain(samr_b, &conn_h, domain_access, sid, &domain_h);
    if (status != 0) rpc_fail(status);

    status = SamrCreateUser(samr_b, &domain_h, username, account_flags,
                            &user_h, &user_rid);
    if (status == STATUS_SUCCESS) {
        if (created) *created = true;

        status = SamrClose(samr_b, &user_h);
        if (status != 0) rpc_fail(status); 

    } else if (status != 0 &&
               status != STATUS_USER_EXISTS) rpc_fail(status);

    status = SamrClose(samr_b, &domain_h);
    if (status != 0) rpc_fail(status);

    status = SamrClose(samr_b, &conn_h);
    if (status != 0) rpc_fail(status);

done: 
    FreeSamrBinding(&samr_b);
    if (sid) SidFree(sid);

    return status;
}


NTSTATUS CleanupAccount(const wchar16_t *hostname, wchar16_t *username)
{
    const uint32 conn_access = SAMR_ACCESS_OPEN_DOMAIN |
                               SAMR_ACCESS_ENUM_DOMAINS;
    const uint32 domain_access = DOMAIN_ACCESS_OPEN_ACCOUNT;
    const uint32 user_access = SEC_STD_DELETE;

    handle_t samr_b = NULL;
    NTSTATUS status = STATUS_SUCCESS;
    PolicyHandle conn_h = {0};
    PolicyHandle domain_h = {0};
    PolicyHandle account_h = {0};
    DomSid *sid = NULL;
    wchar16_t *names[1] = {0};
    uint32 *rids = NULL;
    uint32 *types = NULL;
    uint32 rids_count = 0;

    status = GetSamDomainSid(&sid, hostname);
    if (status != 0) rpc_fail(status);

    CreateSamrBinding(&samr_b, hostname);
    if (samr_b == NULL) rpc_fail(status);

    status = SamrConnect2(samr_b, hostname, conn_access, &conn_h);
    if (status != 0) rpc_fail(status);
    
    status = SamrOpenDomain(samr_b, &conn_h, domain_access, sid, &domain_h);
    if (status != 0) rpc_fail(status);

    names[0] = username;
    status = SamrLookupNames(samr_b, &domain_h, 1, names, &rids, &types,
                             &rids_count);

    /* if no account has been found return success */
    if (status == STATUS_NONE_MAPPED) {
        status = STATUS_SUCCESS;
        goto done;

    } else if (status != 0) {
        rpc_fail(status);
    }

    status = SamrOpenUser(samr_b, &domain_h, user_access, rids[0], &account_h);
    if (status != 0) rpc_fail(status);

    status = SamrDeleteUser(samr_b, &account_h);
    if (status != 0) rpc_fail(status);
    
    status = SamrClose(samr_b, &domain_h);
    if (status != 0) rpc_fail(status);

    status = SamrClose(samr_b, &conn_h);
    if (status != 0) rpc_fail(status);

    FreeSamrBinding(&samr_b);

done:
    if (rids) SamrFreeMemory((void*)rids);
    if (types) SamrFreeMemory((void*)types);

    SAFE_FREE(sid);
    
    return status;
}


NTSTATUS EnsureAlias(const wchar16_t *hostname, wchar16_t *aliasname,
                     int *created)
{
    
    const uint32 conn_access = SAMR_ACCESS_OPEN_DOMAIN |
                               SAMR_ACCESS_ENUM_DOMAINS;
    const uint32 domain_access = DOMAIN_ACCESS_OPEN_ACCOUNT |
                                 DOMAIN_ACCESS_ENUM_ACCOUNTS |
                                 DOMAIN_ACCESS_CREATE_ALIAS |
                                 DOMAIN_ACCESS_LOOKUP_INFO_2;
    const uint32 alias_access = ALIAS_ACCESS_LOOKUP_INFO |
                                ALIAS_ACCESS_SET_INFO;

    NTSTATUS status = STATUS_SUCCESS;
    DomSid *sid = NULL;
    handle_t samr_b = NULL;
    PolicyHandle conn_h = {0};
    PolicyHandle domain_h = {0};
    PolicyHandle alias_h = {0};
    uint32 alias_rid = 0;

    if (created) *created = false;

    status = GetSamDomainSid(&sid, hostname);
    if (status != 0) rpc_fail(status);

    CreateSamrBinding(&samr_b, hostname);
    if (samr_b == NULL) goto done;

    status = SamrConnect2(samr_b, hostname, conn_access, &conn_h);
    if (status != 0) rpc_fail(status);
    
    status = SamrOpenDomain(samr_b, &conn_h, domain_access, sid, &domain_h);
    if (status != 0) return status;

    status = SamrCreateDomAlias(samr_b, &domain_h, aliasname, alias_access,
                                &alias_h, &alias_rid);
    if (status == STATUS_SUCCESS) {
        /* Let caller know that new alias have been created */
        if (created) *created = true;

        status = SamrClose(samr_b, &alias_h);
        if (status != 0) rpc_fail(status);

    } else if (status != 0 &&
               status != STATUS_ALIAS_EXISTS) rpc_fail(status);

    status = SamrClose(samr_b, &domain_h);
    if (status != 0) rpc_fail(status);

    status = SamrClose(samr_b, &conn_h);
    if (status != 0) rpc_fail(status);

done:
    FreeSamrBinding(&samr_b);
    if (sid) SidFree(sid);

    return status;
}


NTSTATUS CleanupAlias(const wchar16_t *hostname, wchar16_t *username)
{
    const uint32 conn_access = SAMR_ACCESS_OPEN_DOMAIN |
                               SAMR_ACCESS_ENUM_DOMAINS;
    const uint32 domain_access = DOMAIN_ACCESS_OPEN_ACCOUNT;
    const uint32 alias_access = SEC_STD_DELETE;

    handle_t samr_b = NULL;
    NTSTATUS status;
    PolicyHandle conn_h = {0};
    PolicyHandle domain_h = {0};
    PolicyHandle alias_h = {0};
    DomSid *sid = NULL;
    wchar16_t *names[1] = {0};
    uint32 *rids = NULL;
    uint32 *types = NULL;
    uint32 rids_count = 0;

    status = GetSamDomainSid(&sid, hostname);
    if (status != 0) rpc_fail(status);

    CreateSamrBinding(&samr_b, hostname);
    if (samr_b == NULL) goto done;

    status = SamrConnect2(samr_b, hostname, conn_access, &conn_h);
    if (status != 0) rpc_fail(status);
    
    status = SamrOpenDomain(samr_b, &conn_h, domain_access, sid, &domain_h);
    if (status != 0) rpc_fail(status);

    names[0] = username;
    status = SamrLookupNames(samr_b, &domain_h, 1, names, &rids, &types,
                             &rids_count);

    /* if no account has been found return success */
    if (status == STATUS_NONE_MAPPED) {
        status = STATUS_SUCCESS;
        goto done;

    } else if (status != 0) {
        rpc_fail(status)
    }

    status = SamrOpenAlias(samr_b, &domain_h, alias_access, rids[0], &alias_h);
    if (status != 0) rpc_fail(status);

    status = SamrDeleteDomAlias(samr_b, &alias_h);
    if (status != 0) rpc_fail(status);
    
    status = SamrClose(samr_b, &domain_h);
    if (status != 0) rpc_fail(status);

    status = SamrClose(samr_b, &conn_h);
    if (status != 0) rpc_fail(status);

    FreeSamrBinding(&samr_b);

done:
    if (rids) SamrFreeMemory((void*)rids);
    if (types) SamrFreeMemory((void*)types);

    SAFE_FREE(sid);
    
    return status;
}


int TestSamrQueryUser(struct test *t, const wchar16_t *hostname,
                      const wchar16_t *user, const wchar16_t *pass,
                      struct parameter *options, int optcount)
{
    const uint32 conn_access_mask = SAMR_ACCESS_OPEN_DOMAIN |
                                    SAMR_ACCESS_ENUM_DOMAINS |
                                    SAMR_ACCESS_CONNECT_TO_SERVER;

    const uint32 dom_access_mask = DOMAIN_ACCESS_OPEN_ACCOUNT |
                                   DOMAIN_ACCESS_ENUM_ACCOUNTS |
                                   DOMAIN_ACCESS_CREATE_USER |
                                   DOMAIN_ACCESS_CREATE_ALIAS |
                                   DOMAIN_ACCESS_LOOKUP_INFO_2;

    const uint32 usr_access_mask = USER_ACCESS_GET_NAME_ETC |
                                   USER_ACCESS_GET_LOCALE |
                                   USER_ACCESS_GET_LOGONINFO |
                                   USER_ACCESS_GET_ATTRIBUTES |
                                   USER_ACCESS_CHANGE_PASSWORD |
                                   SEC_STD_DELETE;
    const char *def_guestname = "Guest";
    const int def_level = 0;

    NTSTATUS status = STATUS_SUCCESS;
    enum param_err perr;
    handle_t samr_binding = NULL;
    NETRESOURCE nr = {0};
    int i = 0;
    PolicyHandle conn_handle = {0};
    PolicyHandle dom_handle = {0};
    PolicyHandle user_handle = {0};
    DomSid *sid = NULL;
    wchar16_t *names[1];
    wchar16_t *username = NULL;
    wchar16_t *domname = NULL;
    uint32 *rids = NULL;
    uint32 *types = NULL;
    uint32 rids_count = 0;
    UserInfo *info = NULL;
    int32 level = 0;

    TESTINFO(t, hostname, user, pass);

    SET_SESSION_CREDS(nr, hostname, user, pass);

    samr_binding = CreateSamrBinding(&samr_binding, hostname);
    if (samr_binding == NULL) return -1;

    perr = fetch_value(options, optcount, "username", pt_w16string, &username,
                       &def_guestname);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "level", pt_int32, &level,
                       &def_level);
    if (!perr_is_ok(perr)) perr_fail(perr);

    PARAM_INFO("username", pt_w16string, username);
    PARAM_INFO("level", pt_int32, &level);

    names[0] = username;

    /*
     * Simple user account querying
     */
    status = SamrConnect2(samr_binding, hostname, conn_access_mask, &conn_handle);
    if (status != 0) rpc_fail(status);

    status = GetSamDomainName(&domname, hostname);
    if (status != 0) rpc_fail(status);

    status = SamrLookupDomain(samr_binding, &conn_handle, domname, &sid);
    if (status != 0) rpc_fail(status);

    SamrFreeMemory((void*)domname);

    status = SamrOpenDomain(samr_binding, &conn_handle, dom_access_mask, sid, &dom_handle);
    if (status != 0) rpc_fail(status);

    status = SamrLookupNames(samr_binding, &dom_handle, 1, names, &rids, &types,
                             &rids_count);
    if (status != 0) rpc_fail(status);

    status = SamrOpenUser(samr_binding, &dom_handle, usr_access_mask, rids[0], &user_handle);
    if (status != 0) rpc_fail(status);

    SamrFreeMemory((void*)rids);
    SamrFreeMemory((void*)types);

    if (level == 0) {
        for (i = 1; i < 26; i++) {
            if (i == 5) continue;      /* infolevel 5 is still broken for some reason */

            INPUT_ARG_PTR(samr_binding);
            INPUT_ARG_PTR(&user_handle);
            INPUT_ARG_UINT(i);

            CALL_MSRPC(status = SamrQueryUserInfo(samr_binding, &user_handle,
                                                  (uint16)i, &info));
            if (status != STATUS_SUCCESS &&
                status != STATUS_INVALID_INFO_CLASS) rpc_fail(status);

            SamrFreeMemory((void*)info);
            info = NULL;
        }
    } else {
        INPUT_ARG_PTR(samr_binding);
        INPUT_ARG_PTR(&user_handle);
        INPUT_ARG_UINT(level);

        CALL_MSRPC(status = SamrQueryUserInfo(samr_binding, &user_handle,
                                              (uint16)level, &info));
        if (status != STATUS_SUCCESS &&
            status != STATUS_INVALID_INFO_CLASS) rpc_fail(status);

        SamrFreeMemory((void*)info);
        info = NULL;
    }

    status = SamrClose(samr_binding, &user_handle);
    if (status != 0) rpc_fail(status);

    status = SamrClose(samr_binding, &dom_handle);
    if (status != 0) rpc_fail(status);

    status = SamrClose(samr_binding, &conn_handle);
    if (status != 0) rpc_fail(status);

    RELEASE_SESSION_CREDS(nr);

done:
    SAFE_FREE(username);
    SamrDestroyMemory();

    return (status == STATUS_SUCCESS);
}


int TestSamrAlias(struct test *t, const wchar16_t *hostname,
                  const wchar16_t *user, const wchar16_t *pass,
                  struct parameter *options, int optcount)
{
    const uint32 conn_access_mask = SAMR_ACCESS_OPEN_DOMAIN |
                                    SAMR_ACCESS_ENUM_DOMAINS |
                                    SAMR_ACCESS_CONNECT_TO_SERVER;

    const uint32 dom_access_mask = DOMAIN_ACCESS_OPEN_ACCOUNT |
                                   DOMAIN_ACCESS_ENUM_ACCOUNTS |
                                   DOMAIN_ACCESS_CREATE_USER |
                                   DOMAIN_ACCESS_CREATE_ALIAS |
                                   DOMAIN_ACCESS_LOOKUP_INFO_2;

    const uint32 alias_access_mask = ALIAS_ACCESS_LOOKUP_INFO |
                                     ALIAS_ACCESS_SET_INFO |
                                     ALIAS_ACCESS_ADD_MEMBER |
                                     ALIAS_ACCESS_REMOVE_MEMBER |
                                     ALIAS_ACCESS_GET_MEMBERS |
                                     SEC_STD_DELETE;

    const char *testalias = "TestAlias";
    const char *testuser = "TestUser";
    const char *testalias_desc = "TestAlias Comment";

    NTSTATUS status = STATUS_SUCCESS;
    enum param_err perr;
    handle_t samr_binding = NULL;
    NETRESOURCE nr = {0};
    uint32 user_rid = 0;
    uint32 *rids = NULL;
    uint32 *types = NULL;
    uint32 num_rids = 0;
    wchar16_t *aliasname = NULL;
    wchar16_t *aliasdesc = NULL;
    wchar16_t *username = NULL;
    wchar16_t *domname = NULL;
    int i = 0;
    uint32 rids_count = 0;
    PolicyHandle conn_handle = {0};
    PolicyHandle dom_handle = {0};
    PolicyHandle alias_handle = {0};
    PolicyHandle user_handle = {0};
    wchar16_t *names[1] = {0};
    DomSid *sid = NULL;
    DomSid *user_sid = NULL;
    AliasInfo *aliasinfo = NULL;
    AliasInfo setaliasinfo;
    DomSid **member_sids = NULL;
    uint32 members_num = 0;
    int alias_created = 0;
    int user_created = 0;

    TESTINFO(t, hostname, user, pass);

    SET_SESSION_CREDS(nr, hostname, user, pass);

    samr_binding = CreateSamrBinding(&samr_binding, hostname);
    if (samr_binding == NULL) return false;

    perr = fetch_value(options, optcount, "aliasname", pt_w16string, &aliasname, &testalias);
    if (!perr_is_ok(perr)) perr_fail(perr);
    
    perr = fetch_value(options, optcount, "aliasdesc", pt_w16string, &aliasdesc, &testalias_desc);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "username", pt_w16string, &username, &testuser);
    if (!perr_is_ok(perr)) perr_fail(perr);

    PARAM_INFO("aliasname", pt_w16string, aliasname);
    PARAM_INFO("aliasdesc", pt_w16string, aliasdesc);
    PARAM_INFO("username", pt_w16string, username);
    

    /*
     * Creating and querying/setting alias (in the host domain)
     */
    
    status = SamrConnect2(samr_binding, hostname, conn_access_mask, &conn_handle);
    if (status != 0) rpc_fail(status);

    status = GetSamDomainName(&domname, hostname);
    if (status != 0) rpc_fail(status);

    status = SamrLookupDomain(samr_binding, &conn_handle, domname, &sid);
    if (status != 0) rpc_fail(status);

    status = SamrOpenDomain(samr_binding, &conn_handle, dom_access_mask, sid,
                            &dom_handle);
    if (status != 0) rpc_fail(status);

    /*
     * Ensure alias to perform tests on
     */
    status = EnsureAlias(hostname, aliasname, &alias_created);
    if (status != 0) rpc_fail(status);

    names[0] = aliasname;

    status = SamrLookupNames(samr_binding, &dom_handle, 1, names,
                             &rids, &types, &rids_count);
    if (status != 0) rpc_fail(status);

    if (rids_count == 0 || rids_count != 1) {
        printf("Incosistency found when looking for alias name\n");
        rpc_fail(STATUS_UNSUCCESSFUL);
    }

    status = SamrOpenAlias(samr_binding, &dom_handle, alias_access_mask,
                           rids[0], &alias_handle);
    if (status != 0) rpc_fail(status);

    if (rids) SamrFreeMemory((void*)rids);
    if (types) SamrFreeMemory((void*)types);
    rids  = NULL;
    types = NULL;

    /*
     * Ensure a user account which will soon become alias member
     */
    status = EnsureUserAccount(hostname, username, &user_created);
    if (status != 0) rpc_fail(status);

    names[0] = username;

    status = SamrLookupNames(samr_binding, &dom_handle, 1, names, &rids, &types,
                             &rids_count);
    if (status != 0) rpc_fail(status);

    if (rids_count == 0 || rids_count != 1) {
        printf("Incosistency found when looking for alias name\n");
        rpc_fail(STATUS_UNSUCCESSFUL);
    }

    user_rid = rids[0];

    if (rids) SamrFreeMemory((void*)rids);
    if (types) SamrFreeMemory((void*)types);
    rids  = NULL;
    types = NULL;

    for (i = 3; i > 1; i--) {
        INPUT_ARG_PTR(samr_binding);
        INPUT_ARG_PTR(&alias_handle);
        INPUT_ARG_UINT(i);

        CALL_MSRPC(status = SamrQueryAliasInfo(samr_binding, &alias_handle,
                                               (uint16)i, &aliasinfo));
        if (status != 0) rpc_fail(status);

        if (aliasinfo) SamrFreeMemory((void*)aliasinfo);
    }

    status = SamrQueryAliasInfo(samr_binding, &alias_handle, 1, &aliasinfo);
    if (status != 0) rpc_fail(status);

    status = InitUnicodeString(&setaliasinfo.description, aliasdesc);
    if (status != 0) rpc_fail(status);

    status = SamrSetAliasInfo(samr_binding, &alias_handle, 3, &setaliasinfo);
    if (status != 0) rpc_fail(status);

    status = RtlSidAllocateResizedCopy(&user_sid, sid->subauth_count+1, sid);
    if (status != 0) rpc_fail(status);

    user_sid->subauth[user_sid->subauth_count - 1] = user_rid;

    status = SamrGetAliasMembership(samr_binding, &dom_handle, user_sid, 1,
                                    &rids, &num_rids);
    if (status != 0) rpc_fail(status);


    /*
     * Adding, deleting and querying members in alias
     */

    status = SamrAddAliasMember(samr_binding, &alias_handle, user_sid);
    if (status != 0) rpc_fail(status);

    status = SamrGetMembersInAlias(samr_binding, &alias_handle, &member_sids,
                                   &members_num);
    if (status != 0) rpc_fail(status);

    status = SamrDeleteAliasMember(samr_binding, &alias_handle, user_sid);
    if (status != 0) rpc_fail(status);


    /*
     * Cleanup
     */

    if (alias_created) {
        status = SamrDeleteDomAlias(samr_binding, &alias_handle);
        if (status != 0) rpc_fail(status);

    } else {
        status = SamrClose(samr_binding, &alias_handle);
        if (status != 0) rpc_fail(status);
    }

    if (user_created) {
        status = SamrDeleteUser(samr_binding, &user_handle);
        if (status != 0) rpc_fail(status);
    }

    status = SamrClose(samr_binding, &dom_handle);
    if (status != 0) rpc_fail(status);

    status = SamrClose(samr_binding, &conn_handle);
    if (status != 0) rpc_fail(status);

    FreeSamrBinding(&samr_binding);
    RELEASE_SESSION_CREDS(nr);

done:
    if (aliasinfo) SamrFreeMemory((void*)aliasinfo);
    if (rids) SamrFreeMemory((void*)rids);
    if (member_sids) SamrFreeMemory((void*)member_sids);
    if (domname) SamrFreeMemory((void*)domname);
    if (rids) SamrFreeMemory((void*)rids);
    if (types) SamrFreeMemory((void*)types);

    FreeUnicodeString(&setaliasinfo.description);
    if (user_sid) SidFree(user_sid);

    SAFE_FREE(aliasname);
    SAFE_FREE(aliasdesc);
    SAFE_FREE(username);

    return (status == STATUS_SUCCESS);
}


int TestSamrUsersInAliases(struct test *t, const wchar16_t *hostname,
                           const wchar16_t *user, const wchar16_t *pass,
                           struct parameter *options, int optcount)
{
    const uint32 conn_access_mask = SAMR_ACCESS_OPEN_DOMAIN |
                                    SAMR_ACCESS_ENUM_DOMAINS |
                                    SAMR_ACCESS_CONNECT_TO_SERVER;

    const uint32 builtin_dom_access_mask = DOMAIN_ACCESS_OPEN_ACCOUNT |
                                           DOMAIN_ACCESS_ENUM_ACCOUNTS |
                                           DOMAIN_ACCESS_LOOKUP_INFO_2;
    const char *btin_sidstr = "S-1-5-32";

    NTSTATUS status = STATUS_SUCCESS;
    enum param_err perr;
    handle_t samr_binding = NULL;
    NETRESOURCE nr = {0};
    PolicyHandle conn_handle = {0};
    PolicyHandle builtin_dom_handle = {0};
    char *sidstr = NULL;
    DomSid *sid = NULL;
    DomainInfo *dominfo = NULL;

    TESTINFO(t, hostname, user, pass);

    SET_SESSION_CREDS(nr, hostname, user, pass);

    samr_binding = CreateSamrBinding(&samr_binding, hostname);
    if (samr_binding == NULL) return false;

    /*
     * Querying user membership in aliases
     */

    perr = fetch_value(options, optcount, "sid", pt_string, &sidstr,
                       &btin_sidstr);
    if (!perr_is_ok(perr)) perr_fail(perr);
	
    ParseSidStringA(&sid, sidstr);

    status = SamrConnect2(samr_binding, hostname, conn_access_mask,
                          &conn_handle);
    if (status != 0) rpc_fail(status);

    status = SamrOpenDomain(samr_binding, &conn_handle, builtin_dom_access_mask,
                            sid, &builtin_dom_handle);
    if (status == 0) {
        uint32 acct_flags = ACB_NORMAL;
        uint32 resume = 0;
        wchar16_t **names = NULL;
        uint32 *rids = NULL;
        uint32 num_entries = 0;
        uint32 i = 0;
        DomSid *alias_sid = NULL;

        do {
            status = SamrEnumDomainAliases(samr_binding, &builtin_dom_handle,
                                           &resume, acct_flags, &names, &rids,
                                           &num_entries);
	    
            for (i = 0; i < num_entries; i++) {
                uint32 *member_rids = NULL;
                uint32 count = 0;

                status = RtlSidAllocateResizedCopy(&alias_sid,
                                                   sid->subauth_count + 1,
                                                   sid);
                alias_sid->subauth[alias_sid->subauth_count - 1] = rids[i];
					
                /* there's actually no need to check status code here */
                status = SamrGetAliasMembership(samr_binding,
                                                &builtin_dom_handle,
                                                alias_sid, 1, &member_rids,
                                                &count);
                SAFE_FREE(alias_sid);

                if (member_rids) SamrFreeMemory((void*)member_rids);
            }

            if (names) SamrFreeMemory((void*)names);
            if (rids) SamrFreeMemory((void*)rids);
            names = NULL;
            rids  = NULL;

        } while (status == STATUS_MORE_ENTRIES);

        status = SamrQueryDomainInfo(samr_binding, &builtin_dom_handle, 
                                     (uint16)2, &dominfo);

        status = SamrClose(samr_binding, &builtin_dom_handle);
    }

    status = SamrClose(samr_binding, &conn_handle);
    if (status != 0) rpc_fail(status);

    FreeSamrBinding(&samr_binding);
    RELEASE_SESSION_CREDS(nr);

done:
    SAFE_FREE(sidstr);

    return (status == STATUS_SUCCESS);
}


int TestSamrQueryDomain(struct test *t, const wchar16_t *hostname,
                        const wchar16_t *user, const wchar16_t *pass,
                        struct parameter *options, int optcount)
{
    const uint32 conn_access_mask = SAMR_ACCESS_OPEN_DOMAIN |
                                    SAMR_ACCESS_ENUM_DOMAINS |
                                    SAMR_ACCESS_CONNECT_TO_SERVER;

    const uint32 dom_access_mask = DOMAIN_ACCESS_OPEN_ACCOUNT |
                                   DOMAIN_ACCESS_ENUM_ACCOUNTS |
                                   DOMAIN_ACCESS_CREATE_USER |
                                   DOMAIN_ACCESS_CREATE_ALIAS |
                                   DOMAIN_ACCESS_LOOKUP_INFO_2 |
                                   DOMAIN_ACCESS_LOOKUP_INFO_1;

    NTSTATUS status = STATUS_SUCCESS;
    handle_t samr_binding = NULL;
    NETRESOURCE nr = {0};
    int i = 0;
    PolicyHandle conn_handle = {0};
    PolicyHandle dom_handle = {0};
    DomSid *sid = NULL;
    DomainInfo *dominfo = NULL;
    wchar16_t *domname = NULL;

    SET_SESSION_CREDS(nr, hostname, user, pass);

    samr_binding = CreateSamrBinding(&samr_binding, hostname);
    if (samr_binding == NULL) return false;

    status = SamrConnect2(samr_binding, hostname, conn_access_mask,
                          &conn_handle);
    if (status != 0) rpc_fail(status);

    status = GetSamDomainName(&domname, hostname);
    if (status != 0) rpc_fail(status);

    status = SamrLookupDomain(samr_binding, &conn_handle, domname, &sid);
    if (status != 0) rpc_fail(status);

    status = SamrOpenDomain(samr_binding, &conn_handle, dom_access_mask,
                            sid, &dom_handle);
    if (status != 0) rpc_fail(status);

    for (i = 1; i < 13; i++) {
        if (i == 10) continue;

        INPUT_ARG_PTR(samr_binding);
        INPUT_ARG_PTR(&dom_handle);
        INPUT_ARG_INT(i);
        INPUT_ARG_PTR(dominfo);
        
        CALL_MSRPC(status = SamrQueryDomainInfo(samr_binding, &dom_handle,
                                                (uint16)i, &dominfo));

        OUTPUT_ARG_PTR(dominfo);

        if (dominfo) {
            SamrFreeMemory((void*)dominfo);
        }
    }
	
    status = SamrClose(samr_binding, &dom_handle);
    if (status != 0) rpc_fail(status);
	
    status = SamrClose(samr_binding, &conn_handle);
    if (status != 0) rpc_fail(status);

    FreeSamrBinding(&samr_binding);
    RELEASE_SESSION_CREDS(nr);

done:
    if (sid) SamrFreeMemory((void*)sid);
    SAFE_FREE(domname);

    return (status == STATUS_SUCCESS);
}


#define DISPLAY_ACCOUNTS(type, names, rids, num)                \
    {                                                           \
        uint32 i;                                               \
        for (i = 0; i < num; i++) {                             \
            wchar16_t *name = enum_names[i];                    \
            uint32 rid = enum_rids[i];                          \
                                                                \
            printfw16("%s: %S (rid=0x%x)\n", type, name, rid);  \
        }                                                       \
    }

int TestSamrEnumUsers(struct test *t, const wchar16_t *hostname,
                      const wchar16_t *user, const wchar16_t *pass,
                      struct parameter *options, int optcount)
{
    const uint32 conn_access_mask = SAMR_ACCESS_OPEN_DOMAIN |
                                    SAMR_ACCESS_ENUM_DOMAINS |
                                    SAMR_ACCESS_CONNECT_TO_SERVER;

    const uint32 dom_access_mask = DOMAIN_ACCESS_OPEN_ACCOUNT |
                                   DOMAIN_ACCESS_ENUM_ACCOUNTS |
                                   DOMAIN_ACCESS_CREATE_USER |
                                   DOMAIN_ACCESS_CREATE_ALIAS |
                                   DOMAIN_ACCESS_LOOKUP_INFO_2;

    const int def_specifydomain = 0;
    const char *def_domainname = "BUILTIN";

    NTSTATUS status = STATUS_SUCCESS;
    handle_t samr_binding = NULL;
    NETRESOURCE nr = {0};
    enum param_err perr;
    uint32 resume = 0;
    uint32 num_entries = 0;
    uint32 max_size = 0;
    uint32 account_flags = 0;
    wchar16_t **enum_names = NULL;
    wchar16_t *domname = NULL;
    wchar16_t *domainname = NULL;
    uint32 *enum_rids = NULL;
    PolicyHandle conn_handle = {0};
    PolicyHandle dom_handle = {0};
    DomSid *sid = NULL;
    int specifydomain = 0;

    perr = fetch_value(options, optcount, "specifydomain", pt_int32,
                       &specifydomain, &def_specifydomain);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "domainname", pt_w16string,
                       &domainname, &def_domainname);
    if (!perr_is_ok(perr)) perr_fail(perr);

    SET_SESSION_CREDS(nr, hostname, user, pass);

    samr_binding = CreateSamrBinding(&samr_binding, hostname);
    if (samr_binding == NULL) return false;

    status = SamrConnect2(samr_binding, hostname, conn_access_mask,
                          &conn_handle);
    if (status != 0) rpc_fail(status);

    if (specifydomain) {
        domname = wc16sdup(domainname);

    } else {
        status = GetSamDomainName(&domname, hostname);
        if (status != 0) rpc_fail(status);
    }

    status = SamrLookupDomain(samr_binding, &conn_handle, domname, &sid);
    if (status != 0) rpc_fail(status);

    status = SamrOpenDomain(samr_binding, &conn_handle, dom_access_mask,
                            sid, &dom_handle);
    if (status != 0) rpc_fail(status);

    /*
     * Enumerating domain users
     */
	
    max_size = 128;
    resume = 0;
    account_flags = ACB_NORMAL;
    do {
        status = SamrEnumDomainUsers(samr_binding, &dom_handle,&resume,
                                     account_flags, max_size, &enum_names,
                                     &enum_rids, &num_entries);

        if (status == STATUS_SUCCESS ||
            status == STATUS_MORE_ENTRIES)
            DISPLAY_ACCOUNTS("User", enum_names, enum_rids, num_entries);

        if (enum_names) SamrFreeMemory((void*)enum_names);
        if (enum_rids) SamrFreeMemory((void*)enum_rids);

    } while (status == STATUS_MORE_ENTRIES);

    resume = 0;
    account_flags = ACB_DOMTRUST;
    do {
        status = SamrEnumDomainUsers(samr_binding, &dom_handle,&resume,
                                     account_flags, max_size, &enum_names,
                                     &enum_rids, &num_entries);
        if (status == STATUS_SUCCESS ||
            status == STATUS_MORE_ENTRIES)
            DISPLAY_ACCOUNTS("Domain trust", enum_names, enum_rids, num_entries);

        if (enum_names) SamrFreeMemory((void*)enum_names);
        if (enum_rids) SamrFreeMemory((void*)enum_rids);

    } while (status == STATUS_MORE_ENTRIES);

    resume = 0;
    account_flags = ACB_WSTRUST;
    do {
        status = SamrEnumDomainUsers(samr_binding, &dom_handle,&resume,
                                     account_flags, max_size, &enum_names,
                                     &enum_rids, &num_entries);
        if (status == STATUS_SUCCESS ||
            status == STATUS_MORE_ENTRIES)
            DISPLAY_ACCOUNTS("Workstation", enum_names, enum_rids, num_entries);

        if (enum_names) SamrFreeMemory((void*)enum_names);
        if (enum_rids) SamrFreeMemory((void*)enum_rids);

    } while (status == STATUS_MORE_ENTRIES);

    resume = 0;
    account_flags = ACB_DISABLED | ACB_NORMAL;
    do {
        status = SamrEnumDomainUsers(samr_binding, &dom_handle, &resume,
                                     account_flags, max_size, &enum_names,
                                     &enum_rids, &num_entries);
        if (status == STATUS_SUCCESS ||
            status == STATUS_MORE_ENTRIES)
            DISPLAY_ACCOUNTS("Disabled", enum_names, enum_rids, num_entries);

        if (enum_names) SamrFreeMemory((void*)enum_names);
        if (enum_rids) SamrFreeMemory((void*)enum_rids);

    } while (status == STATUS_MORE_ENTRIES);


    SamrClose(samr_binding, &dom_handle);
    if (status != 0) rpc_fail(status);
	
    SamrClose(samr_binding, &conn_handle);
    if (status != 0) rpc_fail(status);

    FreeSamrBinding(&samr_binding);
    RELEASE_SESSION_CREDS(nr);

done:
    if (sid) {
        SamrFreeMemory((void*)sid);
    }

    SAFE_FREE(domname);
    SAFE_FREE(domainname);

    return (status == STATUS_SUCCESS);
}


#define DISPLAY_DOMAINS(names, num)                             \
    {                                                           \
        uint32 i;                                               \
        for (i = 0; i < num; i++) {                             \
            wchar16_t *name = names[i];                         \
            printfw16("Domain name: [%S]\n", name);             \
        }                                                       \
    }

int TestSamrEnumDomains(struct test *t, const wchar16_t *hostname,
                        const wchar16_t *user, const wchar16_t *pass,
                        struct parameter *options, int optcount)
{
    const uint32 conn_access_mask = SAMR_ACCESS_OPEN_DOMAIN |
                                    SAMR_ACCESS_ENUM_DOMAINS |
                                    SAMR_ACCESS_CONNECT_TO_SERVER;

    NTSTATUS status = STATUS_SUCCESS;
    handle_t samr_binding = NULL;
    NETRESOURCE nr = {0};
    uint32 resume = 0;
    uint32 num_entries = 0;
    uint32 max_size = 0;
    wchar16_t **enum_domains = NULL;
    PolicyHandle conn_handle = {0};

    SET_SESSION_CREDS(nr, hostname, user, pass);

    samr_binding = CreateSamrBinding(&samr_binding, hostname);
    if (samr_binding == NULL) return false;

    status = SamrConnect2(samr_binding, hostname, conn_access_mask,
                          &conn_handle);
    if (status != 0) rpc_fail(status);

    resume = 0;
    max_size = 64;

    do {
        status = SamrEnumDomains(samr_binding, &conn_handle, &resume, max_size,
                                 &enum_domains, &num_entries);

        DISPLAY_DOMAINS(enum_domains, num_entries);

        if (enum_domains) SamrFreeMemory((void*)enum_domains);

    } while (status == STATUS_MORE_ENTRIES);

    status = SamrClose(samr_binding, &conn_handle);
    if (status != 0) rpc_fail(status);

    FreeSamrBinding(&samr_binding);
    RELEASE_SESSION_CREDS(nr);

done:
    return (status == STATUS_SUCCESS);
}


int TestSamrCreateUserAccount(struct test *t, const wchar16_t *hostname,
                              const wchar16_t *user, const wchar16_t *pass,
                              struct parameter *options, int optcount)
{
    const uint32 conn_access_mask = SAMR_ACCESS_OPEN_DOMAIN |
                                    SAMR_ACCESS_ENUM_DOMAINS |
                                    SAMR_ACCESS_CONNECT_TO_SERVER;

    const uint32 dom_access_mask = DOMAIN_ACCESS_OPEN_ACCOUNT |
                                   DOMAIN_ACCESS_ENUM_ACCOUNTS |
                                   DOMAIN_ACCESS_CREATE_USER |
                                   DOMAIN_ACCESS_CREATE_ALIAS |
                                   DOMAIN_ACCESS_LOOKUP_INFO_2;

    const uint32 usr_access_mask = USER_ACCESS_GET_NAME_ETC |
                                   USER_ACCESS_GET_LOCALE |
                                   USER_ACCESS_GET_LOGONINFO |
                                   USER_ACCESS_GET_ATTRIBUTES |
                                   USER_ACCESS_CHANGE_PASSWORD |
                                   SEC_STD_DELETE;
	
    const uint32 account_flags = ACB_NORMAL;
    const char *newuser = "Testuser";

    NTSTATUS status = STATUS_SUCCESS;
    enum param_err perr;
    handle_t samr_binding = NULL;
    NETRESOURCE nr = {0};
    wchar16_t *newusername = NULL;
    wchar16_t *domname = NULL;
    uint32 rid = 0;
    PolicyHandle conn_handle = {0};
    PolicyHandle dom_handle = {0};
    PolicyHandle account_handle = {0};
    DomSid *sid = NULL;

    perr = fetch_value(options, optcount, "username", pt_w16string,
                       &newusername, &newuser);
    if (!perr_is_ok(perr)) perr_fail(perr);


    SET_SESSION_CREDS(nr, hostname, user, pass);

    /*
     * Make sure there's no account with the same name already
     */
    status = CleanupAccount(hostname, newusername);
    if (status != 0) goto done;

    samr_binding = CreateSamrBinding(&samr_binding, hostname);
    if (samr_binding == NULL) return false;

    /*
     * Creating and deleting user account
     */
	
    status = SamrConnect2(samr_binding, hostname, conn_access_mask,
                          &conn_handle);
    if (status != 0) rpc_fail(status);

    status = GetSamDomainName(&domname, hostname);
    if (status != 0) rpc_fail(status);

    status = SamrLookupDomain(samr_binding, &conn_handle, domname, &sid);
    if (status != 0) rpc_fail(status);

    status = SamrOpenDomain(samr_binding, &conn_handle, dom_access_mask,
                            sid, &dom_handle);
    if (status != 0) rpc_fail(status);

    status = SamrCreateUser(samr_binding, &dom_handle, newusername,
                            account_flags, &account_handle, &rid);
    if (status != 0) rpc_fail(status);

    status = SamrClose(samr_binding, &account_handle);
    if (status != 0) rpc_fail(status);

    status = SamrOpenUser(samr_binding, &dom_handle, usr_access_mask,
                          rid, &account_handle);
    if (status != 0) rpc_fail(status);

    status = SamrDeleteUser(samr_binding, &account_handle);
    if (status != 0) rpc_fail(status);

    status = SamrClose(samr_binding, &dom_handle);
    if (status != 0) rpc_fail(status);

    status = SamrClose(samr_binding, &conn_handle);
    if (status != 0) rpc_fail(status);

    FreeSamrBinding(&samr_binding);
    RELEASE_SESSION_CREDS(nr);

done:
    if (sid) SamrFreeMemory((void*)sid);

    SAFE_FREE(newusername);
    SAFE_FREE(domname);

    return (status == STATUS_SUCCESS);
}


int TestSamrCreateAlias(struct test *t, const wchar16_t *hostname,
                        const wchar16_t *user, const wchar16_t *pass,
                        struct parameter *options, int optcount)
{
    const uint32 conn_access_mask = SAMR_ACCESS_OPEN_DOMAIN |
                                    SAMR_ACCESS_ENUM_DOMAINS |
                                    SAMR_ACCESS_CONNECT_TO_SERVER;

    const uint32 dom_access_mask = DOMAIN_ACCESS_OPEN_ACCOUNT |
                                   DOMAIN_ACCESS_ENUM_ACCOUNTS |
                                   DOMAIN_ACCESS_CREATE_USER |
                                   DOMAIN_ACCESS_CREATE_ALIAS |
                                   DOMAIN_ACCESS_LOOKUP_INFO_2;

    const uint32 alias_access = ALIAS_ACCESS_LOOKUP_INFO |
                                ALIAS_ACCESS_SET_INFO |
                                SEC_STD_DELETE;

    const char *def_aliasname = "Testalias";

    NTSTATUS status = STATUS_SUCCESS;
    enum param_err perr;
    handle_t samr_binding = NULL;
    NETRESOURCE nr = {0};
    wchar16_t *newaliasname = NULL;
    wchar16_t *domname = NULL;
    uint32 rid = 0;
    PolicyHandle conn_handle = {0};
    PolicyHandle dom_handle = {0};
    PolicyHandle account_handle = {0};
    DomSid *sid = NULL;

    perr = fetch_value(options, optcount, "aliasname", pt_w16string,
                       &newaliasname, &def_aliasname);
    if (!perr_is_ok(perr)) perr_fail(perr);

    SET_SESSION_CREDS(nr, hostname, user, pass);

    status = CleanupAlias(hostname, newaliasname);
    if (status != 0) rpc_fail(status);

    samr_binding = CreateSamrBinding(&samr_binding, hostname);
    if (samr_binding == NULL) return false;

    status = SamrConnect2(samr_binding, hostname, conn_access_mask,
                          &conn_handle);
    if (status != 0) rpc_fail(status);

    status = GetSamDomainName(&domname, hostname);
    if (status != 0) rpc_fail(status);

    status = SamrLookupDomain(samr_binding, &conn_handle, domname, &sid);
    if (status != 0) rpc_fail(status);

    status = SamrOpenDomain(samr_binding, &conn_handle, dom_access_mask,
                            sid, &dom_handle);
    if (status != 0) rpc_fail(status);

    status = SamrCreateDomAlias(samr_binding, &dom_handle, newaliasname,
                                alias_access, &account_handle, &rid);
    if (status != 0) rpc_fail(status);

    status = SamrClose(samr_binding, &account_handle);
    if (status != 0) rpc_fail(status);

    status = SamrOpenAlias(samr_binding, &dom_handle, alias_access, rid,
                           &account_handle);
    if (status != 0) rpc_fail(status);

    status = SamrDeleteDomAlias(samr_binding, &account_handle);
    if (status != 0) rpc_fail(status);

    status = SamrClose(samr_binding, &dom_handle);
    if (status != 0) rpc_fail(status);

    status = SamrClose(samr_binding, &conn_handle);
    if (status != 0) rpc_fail(status);

    FreeSamrBinding(&samr_binding);
    RELEASE_SESSION_CREDS(nr);

done:
    if (sid) SamrFreeMemory((void*)sid);

    SAFE_FREE(domname);
    SAFE_FREE(newaliasname);

    return (status == STATUS_SUCCESS);
}


int TestSamrSetUserPassword(struct test *t, const wchar16_t *hostname,
                            const wchar16_t *user, const wchar16_t *pass,
                            struct parameter *options, int optcount)
{
    const uint32 conn_access_mask = SAMR_ACCESS_OPEN_DOMAIN |
                                    SAMR_ACCESS_ENUM_DOMAINS |
                                    SAMR_ACCESS_CONNECT_TO_SERVER;
    const uint32 dom_access_mask = DOMAIN_ACCESS_OPEN_ACCOUNT |
                                   DOMAIN_ACCESS_ENUM_ACCOUNTS |
                                   DOMAIN_ACCESS_CREATE_USER |
                                   DOMAIN_ACCESS_CREATE_ALIAS |
                                   DOMAIN_ACCESS_LOOKUP_INFO_1;

    const uint32 usr_access_mask = USER_ACCESS_GET_NAME_ETC |
                                   USER_ACCESS_GET_LOCALE |
                                   USER_ACCESS_GET_LOGONINFO |
                                   USER_ACCESS_GET_ATTRIBUTES |
                                   USER_ACCESS_CHANGE_PASSWORD |
                                   USER_ACCESS_SET_PASSWORD |
                                   USER_ACCESS_SET_ATTRIBUTES |
	                           SEC_STD_DELETE;

    const char *newuser = "Testuser";
    const char *testpass = "JustTesting30$";

    NTSTATUS status = STATUS_SUCCESS;
    RPCSTATUS rpcstatus;
    enum param_err perr;
    handle_t samr_binding;
    NETRESOURCE nr = {0};
    int newacct;
    wchar16_t *newusername, *domname;
    uint32 rid;
    PolicyHandle conn_handle = {0};
    PolicyHandle dom_handle = {0};
    PolicyHandle account_handle = {0};
    DomSid *sid = NULL;
    uint32 *rids, *types, rids_count;
    wchar16_t *names[1];
    UserInfo userinfo;
    UserInfo26 *info26 = NULL;
    unsigned char initval[16] = {0};
    wchar16_t *password;
    unsigned char *sess_key;
    unsigned16 sess_key_len;
    unsigned char digested_sess_key[16] = {0};
    struct md5context ctx;

    memset((void*)&userinfo, 0, sizeof(userinfo));

    perr = fetch_value(options, optcount, "username", pt_w16string,
                       &newusername, &newuser);
    if (!perr_is_ok(perr)) perr_fail(perr)

    perr = fetch_value(options, optcount, "password", pt_w16string,
                       &password, &testpass);
    if (!perr_is_ok(perr)) perr_fail(perr);

    SET_SESSION_CREDS(nr, hostname, user, pass);

    samr_binding = CreateSamrBinding(&samr_binding, hostname);
    if (samr_binding == NULL) return false;

    status = EnsureUserAccount(hostname, newusername, &newacct);
    if (status != 0) rpc_fail(status);

    /*
     * Creating and deleting user account
     */
	
    status = SamrConnect2(samr_binding, hostname, conn_access_mask,
                          &conn_handle);
    if (status != 0) rpc_fail(status);

    GetSessionKey(samr_binding, &sess_key, &sess_key_len, &rpcstatus);
    if (rpcstatus != 0) return false;

    status = GetSamDomainName(&domname, hostname);
    if (status != 0) rpc_fail(status);

    status = SamrLookupDomain(samr_binding, &conn_handle, domname, &sid);
    if (status != 0) rpc_fail(status);

    status = SamrOpenDomain(samr_binding, &conn_handle, dom_access_mask,
                            sid, &dom_handle);
    if (status != 0) rpc_fail(status);

    names[0] = newusername;

    status = SamrLookupNames(samr_binding, &dom_handle, 1, names, &rids, &types,
                             &rids_count);
    if (status != 0) rpc_fail(status);

    rid = rids[0];

    status = SamrOpenUser(samr_binding, &dom_handle, usr_access_mask,
                          rid, &account_handle);
    if (status != 0) rpc_fail(status);

    info26 = &userinfo.info26;
    EncodePassBufferW16(info26->password.data, password);
    info26->password_len = strlen(testpass);

    memset(initval, 0, sizeof(initval));
    
    md5init(&ctx);
    md5update(&ctx, initval, 16);
    md5update(&ctx, sess_key, (unsigned int)sess_key_len);
    md5final(&ctx, digested_sess_key);

    rc4(info26->password.data, 516, digested_sess_key, 16);
    memcpy((void*)&info26->password.data, initval, 16);
    
    status = SamrSetUserInfo(samr_binding, &account_handle, 26, &userinfo);
    if (status != 0) rpc_fail(status);

    if (newacct) {
        status = SamrDeleteUser(samr_binding, &account_handle);
        if (status != 0) rpc_fail(status);
    }

    status = SamrClose(samr_binding, &dom_handle);
    if (status != 0) rpc_fail(status);

    status = SamrClose(samr_binding, &conn_handle);
    if (status != 0) rpc_fail(status);

    FreeSamrBinding(&samr_binding);
    RELEASE_SESSION_CREDS(nr);

done:
    if (sid) SamrFreeMemory((void*)sid);
    if (rids) SamrFreeMemory((void*)rids);
    if (types) SamrFreeMemory((void*)types);

    SAFE_FREE(newusername);
    SAFE_FREE(password);
    SAFE_FREE(domname);
    SAFE_FREE(sess_key);

    return (status == STATUS_SUCCESS);
}


int TestSamrMultipleConnections(struct test *t, const wchar16_t *hostname,
                                const wchar16_t *user, const wchar16_t *pass,
                                struct parameter *options, int optcount)
{
    const uint32 conn_access = SAMR_ACCESS_OPEN_DOMAIN |
                               SAMR_ACCESS_ENUM_DOMAINS |
                               SAMR_ACCESS_CONNECT_TO_SERVER;
    NTSTATUS status = STATUS_SUCCESS;
    NETRESOURCE nr = {0};
    handle_t samr_binding1 = NULL;
    handle_t samr_binding2 = NULL;
    PolicyHandle conn_handle1 = {0};
    PolicyHandle conn_handle2 = {0};
    unsigned char *key1 = NULL;
    unsigned char *key2 = NULL;
    unsigned16 key_len1, key_len2;
    RPCSTATUS st = 0;

    samr_binding1 = NULL;
    samr_binding2 = NULL;

    SET_SESSION_CREDS(nr, hostname, user, pass);

    samr_binding1 = CreateSamrBinding(&samr_binding1, hostname);
    if (samr_binding1 == NULL) return false;

    status = SamrConnect2(samr_binding1, hostname, conn_access, &conn_handle1);
    if (status != 0) rpc_fail(status);

    GetSessionKey(samr_binding1, &key1, &key_len1, &st);
    if (st != 0) return false;

    samr_binding2 = CreateSamrBinding(&samr_binding2, hostname);
    if (samr_binding2 == NULL) return false;

    status = SamrConnect2(samr_binding2, hostname, conn_access, &conn_handle2);
    if (status != 0) rpc_fail(status);

    GetSessionKey(samr_binding2, &key2, &key_len2, &st);
    if (st != 0) return false;

    status = SamrClose(samr_binding1, &conn_handle1);
    if (status != 0) rpc_fail(status);

    status = SamrClose(samr_binding2, &conn_handle2);
    if (status != 0) rpc_fail(status);

    FreeSamrBinding(&samr_binding1);
    FreeSamrBinding(&samr_binding2);

    RELEASE_SESSION_CREDS(nr);

done:
    SAFE_FREE(key1);
    SAFE_FREE(key2);

    return (status == STATUS_SUCCESS);
}


int TestSamrChangeUserPassword(struct test *t, const wchar16_t *hostname,
                               const wchar16_t *user, const wchar16_t *pass,
                               struct parameter *options, int optcount)
{
    const char *defusername = "TestUser";
    const char *defoldpass = "";
    const char *defnewpass = "secret";

    NTSTATUS status = STATUS_SUCCESS;
    enum param_err perr;
    handle_t samr_b = NULL;
    NETRESOURCE nr = {0};
    wchar16_t *username = NULL;
    wchar16_t *oldpassword = NULL;
    wchar16_t *newpassword = NULL;
    uint8 old_nthash[16] = {0};
    uint8 new_nthash[16] = {0};
    uint8 old_lmhash[16] = {0};
    size_t oldlen, newlen;
    uint8 ntpassbuf[516] = {0};
    uint8 ntverhash[16] = {0};

    perr = fetch_value(options, optcount, "username", pt_w16string,
                       &username, &defusername);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "oldpass", pt_w16string,
                       &oldpassword, &defoldpass);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "newpass", pt_w16string,
                       &newpassword, &defnewpass);
    if (!perr_is_ok(perr)) perr_fail(perr);

    SET_SESSION_CREDS(nr, hostname, user, pass);

    samr_b = CreateSamrBinding(&samr_b, hostname);
    if (samr_b == NULL) return STATUS_UNSUCCESSFUL;

    oldlen = wc16slen(oldpassword);
    newlen = wc16slen(newpassword);

    /* prepare NT password hashes */
    md4hash(old_nthash, oldpassword);
    md4hash(new_nthash, newpassword);

    /* prepare LM password hash */
    deshash(old_lmhash, oldpassword);

    /* encode password buffer */
    EncodePassBufferW16(ntpassbuf, newpassword);
    rc4(ntpassbuf, 516, old_nthash, 16);

    /* encode NT verifier */
    des56(ntverhash, old_nthash, 8, new_nthash);
    des56(&ntverhash[8], &old_nthash[8], 8, &new_nthash[7]);

    status = SamrChangePasswordUser2(samr_b, hostname, username, ntpassbuf,
                                     ntverhash, 0, NULL, NULL);
    if (status != 0) rpc_fail(status);

    FreeSamrBinding(&samr_b);
    RELEASE_SESSION_CREDS(nr);

done:

    SAFE_FREE(username);
    SAFE_FREE(oldpassword);
    SAFE_FREE(newpassword);

    return (status == STATUS_SUCCESS);
}


int TestSamrEnumAliases(struct test *t, const wchar16_t *hostname,
                        const wchar16_t *user, const wchar16_t *pass,
                        struct parameter *options, int optcount)
{
    const uint32 conn_access_mask = SAMR_ACCESS_OPEN_DOMAIN |
                                    SAMR_ACCESS_ENUM_DOMAINS |
                                    SAMR_ACCESS_CONNECT_TO_SERVER;

    const uint32 dom_access_mask = DOMAIN_ACCESS_OPEN_ACCOUNT |
                                   DOMAIN_ACCESS_ENUM_ACCOUNTS |
                                   DOMAIN_ACCESS_CREATE_USER |
                                   DOMAIN_ACCESS_CREATE_ALIAS |
                                   DOMAIN_ACCESS_LOOKUP_INFO_2;

    const int def_specifydomain = 0;
    const char *def_domainname = "BUILTIN";

    NTSTATUS status = STATUS_SUCCESS;
    handle_t samr_binding;
    NETRESOURCE nr = {0};
    enum param_err perr;
    uint32 resume, num_entries, max_size;
    uint32 account_flags;
    wchar16_t **enum_names, *domname, *domainname;
    uint32 *enum_rids;
    PolicyHandle conn_handle = {0};
    PolicyHandle dom_handle = {0};
    DomSid *sid = NULL;
    int specifydomain;

    perr = fetch_value(options, optcount, "specifydomain", pt_int32,
                       &specifydomain, &def_specifydomain);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "domainname", pt_w16string,
                       &domainname, &def_domainname);
    if (!perr_is_ok(perr)) perr_fail(perr);


    SET_SESSION_CREDS(nr, hostname, user, pass);

    samr_binding = CreateSamrBinding(&samr_binding, hostname);
    if (samr_binding == NULL) return false;

    status = SamrConnect2(samr_binding, hostname, conn_access_mask,
                          &conn_handle);
    if (status != 0) rpc_fail(status);

    if (specifydomain) {
        domname = wc16sdup(domainname);

    } else {
        status = GetSamDomainName(&domname, hostname);
        if (status != 0) rpc_fail(status);
    }

    status = SamrLookupDomain(samr_binding, &conn_handle, domname, &sid);
    if (status != 0) rpc_fail(status);

    status = SamrOpenDomain(samr_binding, &conn_handle, dom_access_mask,
                            sid, &dom_handle);
    if (status != 0) rpc_fail(status);

    /*
     * Enumerating domain aliases
     */
	
    max_size = 128;
    resume = 0;
    account_flags = ACB_NORMAL;
    do {
        enum_names = NULL;
        enum_rids  = NULL;

        status = SamrEnumDomainAliases(samr_binding, &dom_handle, &resume,
                                       account_flags, &enum_names, &enum_rids,
                                       &num_entries);

        if (status == STATUS_SUCCESS ||
            status == STATUS_MORE_ENTRIES)
            DISPLAY_ACCOUNTS("Alias", enum_names, enum_rids, num_entries);

        if (enum_names) SamrFreeMemory((void*)enum_names);
        if (enum_rids) SamrFreeMemory((void*)enum_rids);

    } while (status == STATUS_MORE_ENTRIES);

    SamrClose(samr_binding, &dom_handle);
    if (status != 0) rpc_fail(status);
	
    SamrClose(samr_binding, &conn_handle);
    if (status != 0) rpc_fail(status);

    RELEASE_SESSION_CREDS(nr);

done:
    if (sid) SamrFreeMemory((void*)sid);

    SAFE_FREE(domname);
    SAFE_FREE(domainname);
    
    return true;
}


int TestSamrGetUserGroups(struct test *t, const wchar16_t *hostname,
                          const wchar16_t *user, const wchar16_t *pass,
                          struct parameter *options, int optcount)
{
    const uint32 conn_access = SAMR_ACCESS_OPEN_DOMAIN |
                               SAMR_ACCESS_ENUM_DOMAINS;
    const uint32 domain_access = DOMAIN_ACCESS_OPEN_ACCOUNT |
                                 DOMAIN_ACCESS_ENUM_ACCOUNTS |
                                 DOMAIN_ACCESS_CREATE_USER |
                                 DOMAIN_ACCESS_LOOKUP_INFO_2;

    const uint32 user_access = USER_ACCESS_GET_NAME_ETC |
                               USER_ACCESS_GET_LOCALE |
                               USER_ACCESS_GET_LOGONINFO |
                               USER_ACCESS_GET_ATTRIBUTES |
                               USER_ACCESS_GET_GROUPS |
                               USER_ACCESS_GET_GROUP_MEMBERSHIP;

    const uint32 lsa_access = LSA_ACCESS_LOOKUP_NAMES_SIDS;

    const char *def_username = "Guest";
    const int def_resolvesids = 0;
    const uint32 def_resolvelevel = 6;

    NTSTATUS status = STATUS_SUCCESS;
    enum param_err perr;
    handle_t samr_b = NULL;
    handle_t lsa_b = NULL;
    NETRESOURCE nr = {0};
    DomSid *domsid = NULL;
    PolicyHandle conn_h = {0};
    PolicyHandle domain_h = {0};
    PolicyHandle user_h = {0};
    PolicyHandle lsa_h = {0};
    wchar16_t *username = NULL;
    int resolvesids = 0;
    uint32 resolve_level = 0;
    wchar16_t *names[1] = {0};
    uint32 *rids = NULL;
    uint32 *types = NULL;
    uint32 rids_count = 0;
    uint32 *grp_rids = NULL;
    uint32 *grp_attrs = NULL;
    uint32 grp_count = 0;
    int i = 0;
    DomSid **grp_sids = NULL;
    wchar16_t **grp_sidstrs = NULL;
    SidArray sid_array = {0};
    RefDomainList *domains = NULL;
    TranslatedName *trans_names = NULL;
    uint32 names_count = 0;
    wchar16_t *grp_name = NULL;
    wchar16_t *dom_name = NULL;

    perr = fetch_value(options, optcount, "username", pt_w16string,
                       &username, &def_username);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "resolvesids", pt_int32,
                       &resolvesids, &def_resolvesids);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "resolvelevel", pt_uint32,
                       &resolve_level, &def_resolvelevel);
    if (!perr_is_ok(perr)) perr_fail(perr);

    PARAM_INFO("username", pt_w16string, username);
    PARAM_INFO("resolvesids", pt_int32, &resolvesids);
    PARAM_INFO("resolvelevel", pt_uint32, &resolve_level);

    SET_SESSION_CREDS(nr, hostname, user, pass);

    status = GetSamDomainSid(&domsid, hostname);
    if (status != 0) rpc_fail(status);

    samr_b = CreateSamrBinding(&samr_b, hostname);
    if (samr_b == NULL) return false;

    status = SamrConnect2(samr_b, hostname, conn_access, &conn_h);
    if (status != 0) rpc_fail(status);

    status = SamrOpenDomain(samr_b, &conn_h, domain_access, domsid, &domain_h);
    if (status != 0) rpc_fail(status);

    names[0] = username;
    status = SamrLookupNames(samr_b, &domain_h, 1, names, &rids, &types,
                             &rids_count);
    if (status != 0) rpc_fail(status);

    status = SamrOpenUser(samr_b, &domain_h, user_access, rids[0], &user_h);
    if (status != 0) rpc_fail(status);

    INPUT_ARG_PTR(samr_b);
    INPUT_ARG_PTR(&user_h);
    INPUT_ARG_PTR(grp_rids);
    INPUT_ARG_PTR(grp_attrs);

    CALL_MSRPC(status = SamrGetUserGroups(samr_b, &user_h, &grp_rids,
                                          &grp_attrs, &grp_count));

    OUTPUT_ARG_PTR(grp_rids);
    OUTPUT_ARG_PTR(grp_attrs);
    OUTPUT_ARG_PTR(grp_count);

    grp_sids = (DomSid**) malloc(sizeof(DomSid*) * grp_count);
    if (grp_sids == NULL) rpc_fail(STATUS_NO_MEMORY);

    grp_sidstrs = (wchar16_t**) malloc(sizeof(wchar16_t*) * grp_count);
    if (grp_sidstrs == NULL) rpc_fail(STATUS_NO_MEMORY);

    if (resolvesids) {
        sid_array.num_sids = grp_count;
        sid_array.sids = (SidPtr*) malloc(sizeof(SidPtr) * grp_count);
        if (sid_array.sids == NULL) rpc_fail(STATUS_NO_MEMORY);
    }

    for (i = 0; i < grp_count; i++) {
        status = RtlSidAllocateResizedCopy(&(grp_sids[i]),
                                           domsid->subauth_count + 1,
                                           domsid);
        if (status != 0) rpc_fail(status);

        grp_sids[i]->subauth[grp_sids[i]->subauth_count - 1] = grp_rids[i];

        status = SidToStringW(grp_sids[i], &(grp_sidstrs[i]));
        if (status != 0) rpc_fail(status);

        if (resolvesids) {
            sid_array.sids[i].sid = grp_sids[i];
        }
    }

    if (resolvesids) {
        lsa_b = CreateLsaBinding(&lsa_b, hostname);
        if (lsa_b == NULL) rpc_fail(STATUS_UNSUCCESSFUL);

        status = LsaOpenPolicy2(lsa_b, hostname, NULL, lsa_access, &lsa_h);
        if (status != 0) rpc_fail(status);

        status = LsaLookupSids(lsa_b, &lsa_h, &sid_array, &domains,
                               &trans_names, resolve_level, &names_count);
        if (status != 0) rpc_fail(status);
    }

    for (i = 0; i < grp_count; i++) {
        printfw16("%S", grp_sidstrs[i]);

        if (resolvesids && i < names_count) {
            LsaDomainInfo *di = NULL;
            TranslatedName *tn = &(trans_names[i]);

            grp_name = GetFromUnicodeString(&tn->name);
            if (grp_name == NULL) rpc_fail(STATUS_NO_MEMORY);

            di = &(domains->domains[tn->sid_index]);
            dom_name = GetFromUnicodeStringEx(&di->name);
            if (dom_name == NULL) rpc_fail(STATUS_NO_MEMORY);

            printfw16(" [%S\\%S]", dom_name, grp_name);

            SAFE_FREE(grp_name);
            SAFE_FREE(dom_name);
        }

        printf("\n");
    }

    status = SamrClose(samr_b, &user_h);
    if (status != 0) rpc_fail(status);

    status = SamrClose(samr_b, &domain_h);
    if (status != 0) rpc_fail(status);

    status = SamrClose(samr_b, &conn_h);
    if (status != 0) rpc_fail(status);

    if (resolvesids) {
        status = LsaClose(lsa_b, &lsa_h);
        if (status != 0) rpc_fail(status);

        FreeLsaBinding(&lsa_b);
    }

    FreeSamrBinding(&samr_b);
    RELEASE_SESSION_CREDS(nr);

done:
    SAFE_FREE(username);
    if (domsid) SidFree(domsid);
    if (rids) SamrFreeMemory((void*)rids);
    if (types) SamrFreeMemory((void*)types);
    if (grp_rids) SamrFreeMemory((void*)grp_rids);
    if (grp_attrs) SamrFreeMemory((void*)grp_attrs);
    
    if (trans_names) LsaRpcFreeMemory((void*)trans_names);
    if (domains) LsaRpcFreeMemory((void*)domains);

    for (i = 0; i < grp_count; i++) {
        SidFree(grp_sids[i]);
        SAFE_FREE(grp_sidstrs[i]);
    }
    SAFE_FREE(grp_sids);
    SAFE_FREE(grp_sidstrs);
    SAFE_FREE(sid_array.sids);

    SAFE_FREE(grp_name);
    SAFE_FREE(dom_name);

    SamrDestroyMemory();
    LsaRpcDestroyMemory();

    return (status == STATUS_SUCCESS);
}


int TestSamrGetUserAliases(struct test *t, const wchar16_t *hostname,
                           const wchar16_t *user, const wchar16_t *pass,
                           struct parameter *options, int optcount)
{
    const uint32 conn_access = SAMR_ACCESS_OPEN_DOMAIN |
                               SAMR_ACCESS_ENUM_DOMAINS;
    const uint32 domain_access = DOMAIN_ACCESS_OPEN_ACCOUNT |
                                 DOMAIN_ACCESS_ENUM_ACCOUNTS |
                                 DOMAIN_ACCESS_LOOKUP_INFO_2;

    const uint32 btin_access = DOMAIN_ACCESS_OPEN_ACCOUNT |
                               DOMAIN_ACCESS_ENUM_ACCOUNTS |
                               DOMAIN_ACCESS_LOOKUP_INFO_2;

    const uint32 lsa_access = LSA_ACCESS_LOOKUP_NAMES_SIDS;

    const char *btin_sidstr = "S-1-5-32";

    const char *def_username = "Guest";
    const int def_resolvesids = 0;
    const uint32 def_resolvelevel = 6;

    NTSTATUS status = STATUS_SUCCESS;
    enum param_err perr;
    handle_t samr_b = NULL;
    handle_t lsa_b = NULL;
    NETRESOURCE nr = {0};
    DomSid *btinsid = NULL;
    DomSid *domsid = NULL;
    PolicyHandle lsa_h = {0};
    PolicyHandle conn_h = {0};
    PolicyHandle btin_h = {0};
    PolicyHandle domain_h = {0};
    wchar16_t *username = NULL;
    int resolvesids = 0;
    wchar16_t *names[1];
    RefDomainList *usr_domains = NULL;
    TranslatedSid2 *trans_sids = NULL;
    DomSid *usr_sid = NULL;
    uint32 sids_count = 0;
    uint32 level = 0;
    uint32 resolve_level = 0;
    uint32 *btin_rids = NULL;
    uint32 *dom_rids = NULL;
    uint32 btin_rids_count = 0;
    uint32 dom_rids_count = 0;
    SidArray sid_array = {0};
    uint32 alias_count = 0;
    int i = 0;
    wchar16_t **alias_sidstrs = NULL;
    RefDomainList *alias_domains = NULL;
    TranslatedName *trans_names = NULL;
    uint32 names_count = 0;
    wchar16_t *alias_name = NULL;
    wchar16_t *dom_name = NULL;

    perr = fetch_value(options, optcount, "username", pt_w16string,
                       &username, &def_username);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "resolvesids", pt_int32,
                       &resolvesids, &def_resolvesids);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "resolvelevel", pt_uint32,
                       &resolve_level, &def_resolvelevel);
    if (!perr_is_ok(perr)) perr_fail(perr);

    PARAM_INFO("username", pt_w16string, username);
    PARAM_INFO("resolvesids", pt_int32, &resolvesids);
    PARAM_INFO("resolvelevel", pt_uint32, &resolve_level);

    SET_SESSION_CREDS(nr, hostname, user, pass);

    /*
     * Resolve username to sid first
     */
    lsa_b = CreateLsaBinding(&lsa_b, hostname);
    if (lsa_b == NULL) rpc_fail(STATUS_UNSUCCESSFUL);

    status = LsaOpenPolicy2(lsa_b, hostname, NULL, lsa_access, &lsa_h);
    if (status != 0) rpc_fail(status);

    names[0] = username;
    level = LSA_LOOKUP_NAMES_ALL;

    status = LsaLookupNames2(lsa_b, &lsa_h, 1, names, &usr_domains, &trans_sids,
                             level, &sids_count);
    if (status != 0) rpc_fail(status);

    if (sids_count == 0) rpc_fail(STATUS_UNSUCCESSFUL);

    /* Create user account sid */
    RtlSidCopyAlloc(&domsid, usr_domains->domains[trans_sids[0].index].sid);
    if (domsid == NULL) rpc_fail(STATUS_NO_MEMORY);

    status = RtlSidAllocateResizedCopy(&usr_sid, domsid->subauth_count + 1,
                                       domsid);
    if (status != 0) rpc_fail(status);
    usr_sid->subauth[usr_sid->subauth_count - 1] = trans_sids[0].rid;

    samr_b = CreateSamrBinding(&samr_b, hostname);
    if (samr_b == NULL) return false;

    status = SamrConnect2(samr_b, hostname, conn_access, &conn_h);
    if (status != 0) rpc_fail(status);

    ParseSidStringA(&btinsid, btin_sidstr);

    status = SamrOpenDomain(samr_b, &conn_h, btin_access, btinsid, &btin_h);
    if (status != 0) rpc_fail(status);

    INPUT_ARG_PTR(samr_b);
    INPUT_ARG_PTR(&btin_h);
    INPUT_ARG_PTR(usr_sid);
    INPUT_ARG_UINT(sids_count);

    CALL_MSRPC(status = SamrGetAliasMembership(samr_b, &btin_h, usr_sid, sids_count,
                                               &btin_rids, &btin_rids_count));

    OUTPUT_ARG_PTR(&btin_rids);
    OUTPUT_ARG_UINT(btin_rids_count);

    status = SamrOpenDomain(samr_b, &conn_h, domain_access, domsid, &domain_h);
    if (status != 0) rpc_fail(status);

    INPUT_ARG_PTR(samr_b);
    INPUT_ARG_PTR(&domain_h);
    INPUT_ARG_PTR(usr_sid);
    INPUT_ARG_UINT(sids_count);

    CALL_MSRPC(status = SamrGetAliasMembership(samr_b, &domain_h, usr_sid,
                                               sids_count, &dom_rids,
                                               &dom_rids_count));

    OUTPUT_ARG_PTR(&dom_rids);
    OUTPUT_ARG_UINT(dom_rids_count);

    alias_count = btin_rids_count + dom_rids_count;
    alias_sidstrs = (wchar16_t**) malloc(sizeof(wchar16_t*) * alias_count);
    if (alias_sidstrs == NULL) rpc_fail(STATUS_NO_MEMORY);

    if (resolvesids) {
        sid_array.num_sids = alias_count;
        sid_array.sids = (SidPtr*) malloc(sizeof(SidPtr) * alias_count);
        if (sid_array.sids == NULL) rpc_fail(STATUS_NO_MEMORY);
    }

    for (i = 0; i < alias_count; i++) {
        DomSid *alias_sid = NULL;
        DomSid *dom_sid = (i < btin_rids_count) ? btinsid : domsid;
        uint32 rid = 0;

        if (i < btin_rids_count) {
            rid = btin_rids[i];
        } else {
            rid = dom_rids[i - btin_rids_count];
        }

        status = RtlSidAllocateResizedCopy(&alias_sid, dom_sid->subauth_count + 1,
                                           dom_sid);
        if (status != 0) rpc_fail(status);

        alias_sid->subauth[alias_sid->subauth_count - 1] = rid;
        status = SidToStringW(alias_sid, &(alias_sidstrs[i]));
        if (status != 0) rpc_fail(status);

        if (resolvesids) {
            sid_array.sids[i].sid = alias_sid;
        }
    };

    if (resolvesids) {
        lsa_b = CreateLsaBinding(&lsa_b, hostname);
        if (lsa_b == NULL) rpc_fail(STATUS_UNSUCCESSFUL);

        status = LsaOpenPolicy2(lsa_b, hostname, NULL, lsa_access, &lsa_h);
        if (status != 0) rpc_fail(status);

        status = LsaLookupSids(lsa_b, &lsa_h, &sid_array, &alias_domains,
                               &trans_names, resolve_level, &names_count);
        if (status != 0) rpc_fail(status);
    }

    for (i = 0; i < alias_count; i++) {
        printfw16("%S", alias_sidstrs[i]);

        if (resolvesids && i < names_count) {
            LsaDomainInfo *di = NULL;
            TranslatedName *tn = &(trans_names[i]);

            alias_name = GetFromUnicodeString(&tn->name);
            if (alias_name == NULL) rpc_fail(STATUS_NO_MEMORY);

            di = &(alias_domains->domains[tn->sid_index]);
            dom_name = GetFromUnicodeStringEx(&di->name);
            if (dom_name == NULL) rpc_fail(STATUS_NO_MEMORY);

            printfw16(" [%S\\%S]", dom_name, alias_name);

            SAFE_FREE(alias_name);
            SAFE_FREE(dom_name);
        }

        printf("\n");
    }

    status = LsaClose(lsa_b, &lsa_h);
    if (status != 0) rpc_fail(status);

    status = SamrClose(samr_b, &btin_h);
    if (status != 0) rpc_fail(status);

    status = SamrClose(samr_b, &domain_h);
    if (status != 0) rpc_fail(status);

    status = SamrClose(samr_b, &conn_h);
    if (status != 0) rpc_fail(status);

    FreeLsaBinding(&lsa_b);
    FreeSamrBinding(&samr_b);

    RELEASE_SESSION_CREDS(nr);

done:
    SAFE_FREE(username);
    if (btinsid) SidFree(btinsid);
    if (domsid) SidFree(domsid);
    if (usr_sid) SidFree(usr_sid); 
    if (usr_domains) LsaRpcFreeMemory((void*)usr_domains);
    if (trans_sids) LsaRpcFreeMemory((void*)trans_sids);
    if (alias_domains) LsaRpcFreeMemory((void*)alias_domains);
    if (trans_names) LsaRpcFreeMemory((void*)trans_names);
    if (btin_rids) SamrFreeMemory((void*)btin_rids);
    if (dom_rids) SamrFreeMemory((void*)dom_rids);

    for (i = 0; i < alias_count; i++) {
        if (resolvesids) {
            SidFree(sid_array.sids[i].sid);
        }
        SAFE_FREE(alias_sidstrs[i]);
    }

    SAFE_FREE(sid_array.sids);
    SAFE_FREE(alias_sidstrs);

    return (status == STATUS_SUCCESS);
}


void SetupSamrTests(struct test *t)
{
    SamrInitMemory();

    AddTest(t, "SAMR-QUERY-USER", TestSamrQueryUser);
    AddTest(t, "SAMR-ALIAS", TestSamrAlias);
    AddTest(t, "SAMR-ALIAS-MEMBERS", TestSamrUsersInAliases);
    AddTest(t, "SAMR-QUERY-DOMAIN", TestSamrQueryDomain);
    AddTest(t, "SAMR-ENUM-USERS", TestSamrEnumUsers);
    AddTest(t, "SAMR-ENUM-DOMAINS", TestSamrEnumDomains);
    AddTest(t, "SAMR-CREATE-USER", TestSamrCreateUserAccount);
    AddTest(t, "SAMR-CREATE-ALIAS", TestSamrCreateAlias);
    AddTest(t, "SAMR-USER-PASSWORD", TestSamrSetUserPassword);
    AddTest(t, "SAMR-MULTIPLE-CONNECTION", TestSamrMultipleConnections);
    AddTest(t, "SAMR-USER-PASSWORD-CHANGE", TestSamrChangeUserPassword);
    AddTest(t, "SAMR-ENUM-ALIASES", TestSamrEnumAliases);
    AddTest(t, "SAMR-GET-USER-GROUPS", TestSamrGetUserGroups);
    AddTest(t, "SAMR-GET-USER-ALIASES", TestSamrGetUserAliases);
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
