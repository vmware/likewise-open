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

/*
 * Authors: Rafal Szczesniak (rafal@likewisesoftware.com)
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <compat/rpcstatus.h>
#include <dce/rpc.h>
#include <dce/dce_error.h>
#include <dce/schannel.h>
#include <lwio/lwio.h>
#include <wc16str.h>
#include <lwnet.h>
#include <lw/ntstatus.h>

#include <lwrpc/types.h>
#include <lwrpc/allocate.h>
#include <lwrpc/lsa.h>
#include <lwrpc/netlogon.h>
#include <lwrpc/mpr.h>
#include <lwps/lwps.h>

#include <md5.h>
#include <hmac_md5.h>
#include <crypto.h>

#include "TestRpc.h"
#include "Params.h"


handle_t CreateNetlogonBinding(handle_t *binding, const wchar16_t *host)
{
    RPCSTATUS status = RPC_S_OK;
    size_t hostname_size = 0;
    char *hostname = NULL;
    PIO_ACCESS_TOKEN access_token = NULL;

    if (binding == NULL || host == NULL) return NULL;

    if (LwIoGetThreadAccessToken(&access_token) != STATUS_SUCCESS) return NULL;

    hostname_size = wc16slen(host) + 1;
    hostname = (char*) malloc(hostname_size * sizeof(char));
    if (hostname == NULL) return NULL;

    wc16stombs(hostname, host, hostname_size);

    status = InitNetlogonBindingDefault(binding, hostname, access_token, FALSE);
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

    if (access_token)
    {
        LwIoDeleteAccessToken(access_token);
    }

    return *binding;
}


handle_t TestOpenSchannel(handle_t netr_b,
                      const wchar16_t *hostname,
                      const wchar16_t *user, const wchar16_t *pass,
                      wchar16_t *server, wchar16_t *domain,
                      wchar16_t *computer, wchar16_t *machpass,
                      uint32 protection_level,
                      NetrCredentials *creds,
                      NETRESOURCE *schnr)
{
    RPCSTATUS st = rpc_s_ok;
    NTSTATUS status = STATUS_SUCCESS;
    wchar16_t *machine_acct = NULL;
    handle_t schn_b = NULL;
    PIO_ACCESS_TOKEN auth = NULL;
    uint8 srv_cred[8];
    rpc_schannel_auth_info_t schnauth_info;

    memset((void*)srv_cred, 0, sizeof(srv_cred));
    memset((void*)&schnauth_info, 0, sizeof(schnauth_info));

    machine_acct = asw16printfw(L"%ws$", computer);
    if (machine_acct == NULL) goto error;

    status = NetrOpenSchannel(netr_b, machine_acct, hostname, server, domain,
                              computer, machpass, creds, &schn_b);
    goto_if_ntstatus_not_success(status, error);

    if (!NetrCredentialsCorrect(creds, srv_cred)) {
        status = STATUS_ACCESS_DENIED;
        goto error;
    }

    memcpy(schnauth_info.session_key, creds->session_key, 16);
    schnauth_info.domain_name  = (unsigned char*) awc16stombs(domain);
    schnauth_info.machine_name = (unsigned char*) awc16stombs(computer);
    schnauth_info.sender_flags = rpc_schn_initiator_flags;

    status = LwIoCreatePlainAccessTokenW(user, pass, &auth);
    goto_if_ntstatus_not_success(status, error);

    status = LwIoSetThreadAccessToken(auth);
    goto_if_ntstatus_not_success(status, error);

    LwIoDeleteAccessToken(auth);

done:
    SAFE_FREE(machine_acct);

    return (st == rpc_s_ok &&
            status == STATUS_SUCCESS) ? schn_b : NULL;

error:
    goto done;
}


void TestCloseSchannel(handle_t schn_b, NETRESOURCE *schnr)
{
    FreeNetlogonBinding(&schn_b);

    LwIoSetThreadAccessToken(NULL);

    SAFE_FREE(schnr->RemoteName);
}



int TestNetlogonSamLogon(struct test *t, const wchar16_t *hostname,
                         const wchar16_t *user, const wchar16_t *pass,
                         struct parameter *options, int optcount)
{
    const char *def_server = "TEST";
    const char *def_domain = "TESTNET";
    const char *def_computer = "TEST";
    const char *def_machpass = "SECRET";
    const char *def_username = "user";
    const char *def_password = "pass";
    const uint32 def_logon_level = 3;
    const uint32 def_validation_level = 2;

    NTSTATUS status = STATUS_SUCCESS;
    handle_t netr_b = NULL;
    handle_t schn_b = NULL;
    NETRESOURCE schnr = {0};
    enum param_err perr = perr_success;
    wchar16_t *computer = NULL;
    wchar16_t *machacct = NULL;
    wchar16_t *machpass = NULL;
    wchar16_t *server = NULL;
    wchar16_t *domain = NULL;
    wchar16_t *username = NULL;
    wchar16_t *password = NULL;
    char *computer_name = NULL;
    char *machine_pass = NULL;
    uint32 logon_level = 0;
    uint32 validation_level = 0;
    NetrCredentials creds = {0};
    NetrValidationInfo *validation_info = NULL;
    uint8 authoritative = 0;
    HANDLE store = (HANDLE)NULL;
    LWPS_PASSWORD_INFO *pi = NULL;
    char host[128] = {0};
    NetrDomainQuery Query;
    NetrDomainQuery1 Query1;
    NetrDomainInfo *pInfo = NULL;

    TESTINFO(t, hostname, user, pass);

    perr = fetch_value(options, optcount, "computer", pt_w16string, &computer,
                       &def_computer);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "machpass", pt_w16string, &machpass,
                       &def_machpass);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "server", pt_w16string, &server,
                       &def_server);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "domain", pt_w16string, &domain,
                       &def_domain);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "username", pt_w16string, &username,
                       &def_username);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "password", pt_w16string, &password,
                       &def_password);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "logon_level", pt_uint32, &logon_level,
                       &def_logon_level);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "validation_level", pt_uint32,
                       &validation_level, &def_validation_level);
    if (!perr_is_ok(perr)) perr_fail(perr);

    PARAM_INFO("computer", pt_w16string, computer);
    PARAM_INFO("machpass", pt_w16string, machpass);
    PARAM_INFO("server", pt_w16string, server);
    PARAM_INFO("domain", pt_w16string, domain);
    PARAM_INFO("username", pt_w16string, username);
    PARAM_INFO("password", pt_w16string, password);
    PARAM_INFO("logon_level", pt_int32, &logon_level);
    PARAM_INFO("validation_level", pt_int32, &validation_level);

    SET_SESSION_CREDS(pCreds);

    computer_name = awc16stombs(computer);
    machine_pass  = awc16stombs(machpass);

    if (strcmp(computer_name, def_computer) == 0 &&
        strcmp(machine_pass, def_machpass) == 0) {

        SAFE_FREE(computer);
        SAFE_FREE(machpass);

        status = LwpsOpenPasswordStore(LWPS_PASSWORD_STORE_DEFAULT, &store);
        if (status != STATUS_SUCCESS) return false;

        gethostname(host, sizeof(host));

        status = LwpsGetPasswordByHostName(store, host, &pi);
        if (status != STATUS_SUCCESS) return false;

        machacct = wc16sdup(pi->pwszMachineAccount);
        machpass = wc16sdup(pi->pwszMachinePassword);
        computer = wc16sdup(pi->pwszHostname);

        status = LwpsClosePasswordStore(store);
        if (status != STATUS_SUCCESS) return false;
    }

    netr_b = CreateNetlogonBinding(&netr_b, hostname);
    if (netr_b == NULL) goto cleanup;

    status = NetrOpenSchannel(netr_b, machacct, hostname, server, domain,
                              computer, machpass, &creds, &schn_b);
    if (status != STATUS_SUCCESS) goto close;

    memset(&Query1, 0, sizeof(Query1));

    Query1.workstation_domain = domain;
    Query1.workstation_site   = ambstowc16s("Default-First-Site-Name");

    Query.query1 = &Query1;

    status = NetrGetDomainInfo(schn_b, &creds, server, computer,
                               1, &Query, &pInfo);
    if (status != STATUS_SUCCESS) goto close;

    NetrFreeMemory(pInfo);

    status = NetrGetDomainInfo(schn_b, &creds, server, computer,
                               1, &Query, &pInfo);
    if (status != STATUS_SUCCESS) goto close;

    NetrFreeMemory(pInfo);

    CALL_MSRPC(status = NetrSamLogonInteractive(schn_b, &creds, server, domain, computer,
                                                username, password,
                                                (uint16)logon_level,
                                                (uint16)validation_level,
                                                &validation_info, &authoritative));
    if (status != STATUS_SUCCESS) goto close;

    TestCloseSchannel(schn_b, &schnr);

close:
    FreeNetlogonBinding(&netr_b);
    RELEASE_SESSION_CREDS;

done:
cleanup:
    SAFE_FREE(computer);
    SAFE_FREE(machpass);
    SAFE_FREE(server);
    SAFE_FREE(domain);
    SAFE_FREE(username);
    SAFE_FREE(password);
    SAFE_FREE(computer_name);
    SAFE_FREE(machine_pass);

    return (status == STATUS_SUCCESS);
}


int TestNetlogonSamLogoff(struct test *t, const wchar16_t *hostname,
                          const wchar16_t *user, const wchar16_t *pass,
                          struct parameter *options, int optcount)
{
    const char *def_server = "TEST";
    const char *def_domain = "TESTNET";
    const char *def_computer = "TestWks4";
    const char *def_machpass = "secret01$";
    const char *def_username = "user";
    const char *def_password = "pass";
    const uint32 def_logon_level = 2;
    const uint32 def_validation_level = 2;

    NTSTATUS status = STATUS_SUCCESS;
    handle_t netr_b = NULL;
    handle_t schn_b = NULL;
    NETRESOURCE schnr = {0};
    enum param_err perr = perr_success;
    wchar16_t *computer = NULL;
    wchar16_t *machpass = NULL;
    wchar16_t *server = NULL;
    wchar16_t *domain = NULL;
    wchar16_t *username = NULL;
    wchar16_t *password = NULL;
    uint32 logon_level = 0;
    uint32 validation_level = 0;
    NetrCredentials creds = {0};
    int hostname_len;

    TESTINFO(t, hostname, user, pass);

    if (username && password)
    {
        /* Set up access token */
        PIO_ACCESS_TOKEN hAccessToken = NULL;

        status = LwIoCreatePlainAccessTokenW(user, pass, &hAccessToken);
        goto_if_ntstatus_not_success(status, done);

        status = LwIoSetThreadAccessToken(hAccessToken);
        goto_if_ntstatus_not_success(status, done);

        LwIoDeleteAccessToken(hAccessToken);
    }

    hostname_len = wc16slen(hostname);
    schnr.RemoteName = (wchar16_t*) malloc((hostname_len + 8) * sizeof(wchar16_t));
    if (schnr.RemoteName == NULL) goto cleanup;

    perr = fetch_value(options, optcount, "computer", pt_w16string, &computer,
                       &def_computer);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "machpass", pt_w16string, &machpass,
                       &def_machpass);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "server", pt_w16string, &server,
                       &def_server);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "domain", pt_w16string, &domain,
                       &def_domain);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "username", pt_w16string, &username,
                       &def_username);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "password", pt_w16string, &password,
                       &def_password);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "logon_level", pt_uint32, &logon_level,
                       &def_logon_level);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "validation_level", pt_uint32,
                       &validation_level, &def_validation_level);
    if (!perr_is_ok(perr)) perr_fail(perr);

    PARAM_INFO("computer", pt_w16string, computer);
    PARAM_INFO("machpass", pt_w16string, machpass);
    PARAM_INFO("server", pt_w16string, server);
    PARAM_INFO("domain", pt_w16string, domain);
    PARAM_INFO("username", pt_w16string, username);
    PARAM_INFO("password", pt_w16string, password);
    PARAM_INFO("logon_level", pt_int32, &logon_level);
    PARAM_INFO("validation_level", pt_int32, &validation_level);

    netr_b = CreateNetlogonBinding(&netr_b, hostname);
    if (netr_b == NULL) goto cleanup;

    schn_b = TestOpenSchannel(netr_b, hostname, user, pass,
                          server, domain, computer, machpass,
                          rpc_c_authn_level_pkt_privacy,
                          &creds, &schnr);

    CALL_MSRPC(status = NetrSamLogoff(schn_b, &creds, server, domain, computer,
                                      username, password, (uint16)logon_level));
    if (status != STATUS_SUCCESS) goto close;

    TestCloseSchannel(schn_b, &schnr);

close:
    FreeNetlogonBinding(&netr_b);
    RELEASE_SESSION_CREDS;

    if (username && password)
    {
        status = LwIoSetThreadAccessToken(NULL);
        goto_if_ntstatus_not_success(status, cleanup);
    }

done:
cleanup:
    SAFE_FREE(computer);
    SAFE_FREE(server);

    return (status == STATUS_SUCCESS);
}



int TestNetlogonSamLogonEx(struct test *t, const wchar16_t *hostname,
                           const wchar16_t *user, const wchar16_t *pass,
                           struct parameter *options, int optcount)
{
    const char *def_server = "TEST";
    const char *def_domain = "TESTNET";
    const char *def_computer = "TestWks4";
    const char *def_machpass = "secret01$";
    const char *def_username = "user";
    const char *def_password = "pass";
    const uint32 def_logon_level = 2;
    const uint32 def_validation_level = 2;

    NTSTATUS status = STATUS_SUCCESS;
    handle_t netr_b = NULL;
    handle_t schn_b = NULL;
    rpc_schannel_auth_info_t schnauth_info = {0};
    NETRESOURCE schnr = {0};
    enum param_err perr = perr_success;
    wchar16_t *computer = NULL;
    wchar16_t *machine_acct = NULL;
    wchar16_t *machpass = NULL;
    wchar16_t *server = NULL;
    wchar16_t *domain = NULL;
    wchar16_t *username = NULL;
    wchar16_t *password = NULL;
    uint32 logon_level = 0;
    uint32 validation_level = 0;
    NetrCredentials creds = {0};
    NetrValidationInfo *validation_info = NULL;
    uint8 authoritative = 0;

    TESTINFO(t, hostname, user, pass);

    SET_SESSION_CREDS(pCreds);

    perr = fetch_value(options, optcount, "computer", pt_w16string, &computer,
                       &def_computer);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "machpass", pt_w16string, &machpass,
                       &def_machpass);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "server", pt_w16string, &server,
                       &def_server);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "domain", pt_w16string, &domain,
                       &def_domain);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "username", pt_w16string, &username,
                       &def_username);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "password", pt_w16string, &password,
                       &def_password);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "logon_level", pt_uint32, &logon_level,
                       &def_logon_level);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "validation_level", pt_uint32,
                       &validation_level, &def_validation_level);
    if (!perr_is_ok(perr)) perr_fail(perr);

    PARAM_INFO("computer", pt_w16string, computer);
    PARAM_INFO("machpass", pt_w16string, machpass);
    PARAM_INFO("server", pt_w16string, server);
    PARAM_INFO("domain", pt_w16string, domain);
    PARAM_INFO("username", pt_w16string, username);
    PARAM_INFO("password", pt_w16string, password);
    PARAM_INFO("logon_level", pt_int32, &logon_level);
    PARAM_INFO("validation_level", pt_int32, &validation_level);

    netr_b = CreateNetlogonBinding(&netr_b, hostname);
    if (netr_b == NULL) goto cleanup;

    schn_b = TestOpenSchannel(netr_b, hostname, user, pass,
                          server, domain, computer, machpass,
                          rpc_c_authn_level_pkt_integrity,
                          &creds, &schnr);
    if (schn_b == NULL) goto cleanup;

    CALL_MSRPC(status = NetrSamLogonEx(schn_b, server, domain, computer,
                                       username, password,
                                       (uint16)logon_level,
                                       (uint16)validation_level,
                                       &validation_info, &authoritative));
    if (status != STATUS_SUCCESS) goto close;

    TestCloseSchannel(schn_b, &schnr);

close:
    FreeNetlogonBinding(&netr_b);
    RELEASE_SESSION_CREDS;

done:
cleanup:
    SAFE_FREE(machine_acct);
    SAFE_FREE(computer);
    SAFE_FREE(server);
    SAFE_FREE(schnr.RemoteName);
    SAFE_FREE(schnauth_info.domain_name);
    SAFE_FREE(schnauth_info.machine_name);

    return (status == STATUS_SUCCESS);
}



int TestNetlogonCredentials(struct test *t, const wchar16_t *hostname,
                            const wchar16_t *user, const wchar16_t *pass,
                            struct parameter *options, int optcount)
{
    const char *def_computer = "TEST";
    const char *def_machpass = "secret01$";
    const char *def_clichal = "0123";
    const char *def_srvchal = "4567";

    enum param_err perr = perr_success;
    wchar16_t *machpass, *computer;
    char *clichal, *srvchal;
    size_t clichal_len, srvchal_len;
    uint8 cli_chal[8], srv_chal[8];
    uint8 pass_hash[16];
    NetrCredentials creds;

    TESTINFO(t, hostname, user, pass);

    perr = fetch_value(options, optcount, "computer", pt_w16string, &computer,
                       &def_computer);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "machpass", pt_w16string, &machpass,
                       &def_machpass);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "clichal", pt_string, &clichal,
                       &def_clichal);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "srvchal", pt_string, &srvchal,
                       &def_srvchal);
    if (!perr_is_ok(perr)) perr_fail(perr);

    PARAM_INFO("machpass", pt_w16string, machpass);
    PARAM_INFO("clichal", pt_string, clichal);
    PARAM_INFO("srvchal", pt_string, srvchal);

    md4hash(pass_hash, machpass);

    clichal_len = strlen(clichal);
    clichal_len = (clichal_len > sizeof(cli_chal)) ? sizeof(cli_chal) : clichal_len;
    srvchal_len = strlen(srvchal);
    srvchal_len = (srvchal_len > sizeof(srv_chal)) ? sizeof(srv_chal) : srvchal_len;

    memset(cli_chal, 0, sizeof(cli_chal));
    memset(srv_chal, 0, sizeof(srv_chal));
    memcpy(cli_chal, clichal, clichal_len);
    memcpy(srv_chal, srvchal, srvchal_len);

    NetrCredentialsInit(&creds, cli_chal, srv_chal, pass_hash,
                        NETLOGON_NET_ADS_FLAGS);

done:
    RELEASE_SESSION_CREDS;

    return true;
}


int TestNetlogonEnumTrustedDomains(struct test *t, const wchar16_t *hostname,
                                   const wchar16_t *user, const wchar16_t *pass,
                                   struct parameter *options, int optcount)
{
    const char *def_server = "TEST";

    NTSTATUS status = STATUS_SUCCESS;
    handle_t netr_b;
    enum param_err perr = perr_success;
    wchar16_t *server = NULL;
    uint32 count = 0;
    NetrDomainTrust *trusts = NULL;

    TESTINFO(t, hostname, user, pass);

    SET_SESSION_CREDS(pCreds);

    perr = fetch_value(options, optcount, "server", pt_w16string, &server,
                       &def_server);
    if (!perr_is_ok(perr)) perr_fail(perr);

    PARAM_INFO("server", pt_w16string, server);

    netr_b = CreateNetlogonBinding(&netr_b, hostname);
    if (netr_b == NULL) return false;

    CALL_MSRPC(status = NetrEnumerateTrustedDomainsEx(netr_b, server,
                                                      &trusts, &count));
    if (status != STATUS_SUCCESS) goto done;

    status = NetrFreeMemory((void*)trusts);

    FreeNetlogonBinding(&netr_b);
    RELEASE_SESSION_CREDS;

done:
    SAFE_FREE(server);

    NetrDestroyMemory();

    return (status == STATUS_SUCCESS);
}


int TestNetlogonEnumDomainTrusts(struct test *t, const wchar16_t *hostname,
                                 const wchar16_t *user, const wchar16_t *pass,
                                 struct parameter *options, int optcount)
{
    const uint32 def_trustflags = NETR_TRUST_FLAG_IN_FOREST |
                                  NETR_TRUST_FLAG_OUTBOUND |
                                  NETR_TRUST_FLAG_TREEROOT |
                                  NETR_TRUST_FLAG_PRIMARY |
                                  NETR_TRUST_FLAG_NATIVE |
                                  NETR_TRUST_FLAG_INBOUND;
    
    NTSTATUS status = STATUS_SUCCESS;
    WINERR err = ERROR_SUCCESS;
    handle_t netr_b = NULL;
    enum param_err perr = perr_success;
    uint32 trustflags = 0;
    uint32 count = 0;
    NetrDomainTrust *trusts = NULL;

    TESTINFO(t, hostname, user, pass);

    SET_SESSION_CREDS(pCreds);

    perr = fetch_value(options, optcount, "trustflags", pt_uint32, &trustflags,
                       &def_trustflags);
    if (!perr_is_ok(perr)) perr_fail(perr);

    PARAM_INFO("trustflags", pt_uint32, &trustflags);

    netr_b = CreateNetlogonBinding(&netr_b, hostname);
    if (netr_b == NULL) return false;

    CALL_NETAPI(err = DsrEnumerateDomainTrusts(netr_b, hostname, trustflags,
                                               &trusts, &count));
    if (err != ERROR_SUCCESS) goto done;

    status = NetrFreeMemory((void*)trusts);

    FreeNetlogonBinding(&netr_b);
    RELEASE_SESSION_CREDS;

done:
    NetrDestroyMemory();

    return (status == STATUS_SUCCESS &&
            err == ERROR_SUCCESS);
}


int TestNetlogonGetDcName(struct test *t, const wchar16_t *hostname,
                          const wchar16_t *user, const wchar16_t *pass,
                          struct parameter *options, int optcount)
{
    const uint32 def_getdcflags = DS_FORCE_REDISCOVERY;
    const char *def_domain_name = "DOMAIN";

    NTSTATUS status = STATUS_SUCCESS;
    WINERR err = ERROR_SUCCESS;
    handle_t netr_b = NULL;
    enum param_err perr = perr_success;
    uint32 getdcflags = 0;
    wchar16_t *domain_name = NULL;
    DsrDcNameInfo *info = NULL;

    TESTINFO(t, hostname, user, pass);

    SET_SESSION_CREDS(pCreds);

    perr = fetch_value(options, optcount, "getdcflags", pt_uint32, &getdcflags,
                       &def_getdcflags);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "domainname", pt_w16string, &domain_name,
                       &def_domain_name);
    if (!perr_is_ok(perr)) perr_fail(perr);

    PARAM_INFO("getdcflags", pt_uint32, &getdcflags);
    PARAM_INFO("domainname", pt_w16string, domain_name);

    netr_b = CreateNetlogonBinding(&netr_b, hostname);
    if (netr_b == NULL) return false;

    CALL_NETAPI(err = DsrGetDcName(netr_b, hostname, domain_name,
                                   NULL, NULL, getdcflags, &info));
    if (err != ERROR_SUCCESS) goto done;

    NetrFreeMemory((void*)info);

    FreeNetlogonBinding(&netr_b);
    RELEASE_SESSION_CREDS;

done:
    NetrDestroyMemory();

    return (status == STATUS_SUCCESS &&
            err == ERROR_SUCCESS);
}


void SetupNetlogonTests(struct test *t)
{
    NTSTATUS status = STATUS_SUCCESS;

    status = NetrInitMemory();
    if (status) return;

    AddTest(t, "NETR-CREDS-TEST", TestNetlogonCredentials);
    AddTest(t, "NETR-ENUM-TRUSTED-DOM" , TestNetlogonEnumTrustedDomains);
    AddTest(t, "NETR-DSR-ENUM-DOMTRUSTS", TestNetlogonEnumDomainTrusts);
    AddTest(t, "NETR-SAM-LOGON", TestNetlogonSamLogon);
    AddTest(t, "NETR-SAM-LOGOFF", TestNetlogonSamLogoff);
    AddTest(t, "NETR-SAM-LOGON-EX", TestNetlogonSamLogonEx);
    AddTest(t, "NETR-DSR-GET-DC-NAME", TestNetlogonGetDcName);
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
