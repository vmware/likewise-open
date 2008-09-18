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
#if 0 /* disable temporarily because schannel is not in trunk yet */
#include <dce/schannel.h>
#endif

#include <lwrpc/types.h>
#include <lwrpc/security.h>
#include <wc16str.h>
#include <lwrpc/ntstatus.h>
#include <md5.h>
#include <hmac_md5.h>
#include <lwrpc/allocate.h>
#include <lwrpc/lsa.h>
#include <lwrpc/lsabinding.h>
#include <lwrpc/netlogon.h>
#include <lwrpc/netlogonbinding.h>
#include <lwrpc/mpr.h>
#include <npc.h>

#include "TestRpc.h"
#include "Params.h"


handle_t CreateNetlogonBinding(handle_t *binding, const wchar16_t *host)
{
    RPCSTATUS status;
    size_t hostname_size;
    unsigned char *hostname;

    if (binding == NULL || host == NULL) return NULL;

    hostname_size = wc16slen(host) + 1;
    hostname = (unsigned char*) malloc(hostname_size * sizeof(char));
    if (hostname == NULL) return NULL;
    wc16stombs(hostname, host, hostname_size);

    status = InitNetlogonBindingDefault(binding, hostname);
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


#if 0
int TestNetlogonDomainAuth(struct test *t, const wchar16_t *hostname,
                           const wchar16_t *user, const wchar16_t *pass,
                           struct parameter *options, int optcount)
{
    const char *def_server = "TEST";
    const char *def_computer = "TestWks4";
    const char *def_machpass = "secret01$";

    RPCSTATUS st = rpc_s_ok;
    NTSTATUS status = STATUS_SUCCESS;
    int err = ERROR_SUCCESS;
    handle_t netr_b, schn_b;
    rpc_schannel_auth_info_t schnauth_info = {0};
    NETRESOURCE nr = {0};
    NETRESOURCE schnr = {0};
    NPC_TOKEN_HANDLE npc_token = NULL;
    enum param_err perr;
    wchar16_t *computer, *machine_acct, *machpass, *server;
    size_t hostname_len;
    uint8 cli_chal[8], srv_chal[8], srv_cred[8];
    uint8 pass_hash[16];
    NetrCredentials creds = {0};
    NetrDomainTrustList trusts = {0};

    TESTINFO(t, hostname, user, pass);

    SET_SESSION_CREDS(nr, hostname, user, pass);

    perr = fetch_value(options, optcount, "computer", pt_w16string, &computer,
                       &def_computer);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "machpass", pt_w16string, &machpass,
                       &def_machpass);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "server", pt_w16string, &server,
                       &def_server);
    if (!perr_is_ok(perr)) perr_fail(perr);

    PARAM_INFO("computer", pt_w16string, computer);
    PARAM_INFO("machpass", pt_w16string, machpass);
    PARAM_INFO("server", pt_w16string, server);

    netr_b = CreateNetlogonBinding(&netr_b, hostname);
    if (netr_b == NULL) return false;

    md4hash(pass_hash, machpass);

    get_random_buffer((uint8*)cli_chal, sizeof(cli_chal));
    status = NetrServerReqChallenge(netr_b, hostname, computer, cli_chal,
                                    srv_chal);
    if (status != STATUS_SUCCESS) goto cleanup;

    NetrCredentialsInit(&creds, cli_chal, srv_chal, pass_hash,
                        NETLOGON_NET_ADS_FLAGS);

    machine_acct = (wchar16_t*) malloc((wc16slen(computer) + 2) *
                                       sizeof(wchar16_t));
    if (machine_acct == NULL) return false;

    sw16printf(machine_acct, "%S$", computer);

    status = NetrServerAuthenticate2(netr_b, hostname, machine_acct,
                                     creds.channel_type, computer,
                                     creds.cli_cred.data, srv_cred,
                                     &creds.negotiate_flags);
    if (status != STATUS_SUCCESS) goto cleanup;

    if (!NetrCredentialsCorrect(&creds, srv_cred)) return false;

    memcpy(schnauth_info.session_key, creds.session_key, 16);
    schnauth_info.domain_name  = awc16stombs(server);
    schnauth_info.machine_name = awc16stombs(computer);

    err = ErrnoToWin32Error(NpcCreateImpersonationToken(
                &npc_token));
    if (err != ERROR_SUCCESS) goto cleanup;

    err = ErrnoToWin32Error(NpcImpersonate(
                npc_token));
    if (err != ERROR_SUCCESS) goto cleanup;

    hostname_len = wc16slen(hostname);
    schnr.RemoteName = (wchar16_t*) malloc((hostname_len + 8) * sizeof(wchar16_t));
    if (schnr.RemoteName == NULL) goto cleanup;

    /* specify credentials for domain controller connection */
    sw16printf(schnr.RemoteName, "\\\\%S\\IPC$", hostname);
    err = WNetAddConnection2(&schnr, pass, user);
    if (err != ERROR_SUCCESS) goto cleanup;

    schn_b = CreateNetlogonBinding(&schn_b, hostname);
    if (schn_b == NULL) goto cleanup;

    rpc_binding_set_auth_info(schn_b,
                              NULL,
                              rpc_c_authn_level_pkt_integrity,
                              rpc_c_authn_schannel,
                              (rpc_auth_identity_handle_t)&schnauth_info,
                              rpc_c_authz_name, /* authz_protocol */
                              &st);

    status = NetrEnumerateTrustedDomainsEx(schn_b, server, &trusts);
    if (status != STATUS_SUCCESS) goto cleanup;

    FreeNetlogonBinding(&schn_b);

    err = WNetCancelConnection2(schnr.RemoteName, 0, 0);
    if (err != ERROR_SUCCESS) goto cleanup;

    err = ErrnoToWin32Error(NpcRevertToSelf());
    if (status != STATUS_SUCCESS) goto cleanup;

    err = ErrnoToWin32Error(NpcCloseImpersonationToken(npc_token));
    npc_token = NULL;
    if (status != STATUS_SUCCESS) goto cleanup;

cleanup:
    FreeNetlogonBinding(&netr_b);
    SAFE_FREE(machine_acct);
    SAFE_FREE(computer);
    SAFE_FREE(server);
    SAFE_FREE(schnr.RemoteName);
    SAFE_FREE(schnauth_info.domain_name);
    SAFE_FREE(schnauth_info.machine_name);
    RELEASE_SESSION_CREDS(nr);

    return true;
}
#endif


int TestNetlogonCredentials(struct test *t, const wchar16_t *hostname,
                            const wchar16_t *user, const wchar16_t *pass,
                            struct parameter *options, int optcount)
{
    const char *def_server = "TEST";
    const char *def_computer = "TEST";
    const char *def_machpass = "secret01$";
    const char *def_clichal = "0123";
    const char *def_srvchal = "4567";

    NTSTATUS status;
    handle_t netr_b;
    NETRESOURCE nr = {0};
    enum param_err perr;
    wchar16_t *machpass, *computer, *machine_acct;
    char *clichal, *srvchal;
    size_t clichal_len, srvchal_len;
    uint8 cli_chal[8], srv_chal[8];
    uint8 pass_hash[16];
    NetrCredentials creds;

    TESTINFO(t, hostname, user, pass);

    SET_SESSION_CREDS(nr, hostname, user, pass);

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
cleanup:
    RELEASE_SESSION_CREDS(nr);

    return true;
}


int TestNetlogonEnumTrustedDomains(struct test *t, const wchar16_t *hostname,
                                   const wchar16_t *user, const wchar16_t *pass,
                                   struct parameter *options, int optcount)
{
    const char *def_server = "TEST";
    const char *def_machpass = "secret01$";

    NTSTATUS status = STATUS_SUCCESS;
    handle_t netr_b;
    NETRESOURCE nr = {0};
    enum param_err perr;
    wchar16_t *server = NULL;
    uint32 count = 0;
    NetrDomainTrust *trusts = NULL;

    TESTINFO(t, hostname, user, pass);

    SET_SESSION_CREDS(nr, hostname, user, pass);

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
    RELEASE_SESSION_CREDS(nr);

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
    NETRESOURCE nr = {0};
    enum param_err perr;
    uint32 trustflags = 0;
    uint32 count = 0;
    NetrDomainTrust *trusts = NULL;

    TESTINFO(t, hostname, user, pass);

    SET_SESSION_CREDS(nr, hostname, user, pass);

    perr = fetch_value(options, optcount, "trustflags", pt_uint32, &trustflags,
                       &def_trustflags);
    if (!perr_is_ok(perr)) perr_fail(perr);

    PARAM_INFO("trustflags", pt_uint32, trustflags);

    netr_b = CreateNetlogonBinding(&netr_b, hostname);
    if (netr_b == NULL) return false;

    CALL_NETAPI(err = DsrEnumerateDomainTrusts(netr_b, hostname, trustflags,
                                               &trusts, &count));
    if (err != ERROR_SUCCESS) goto done;

    status = NetrFreeMemory((void*)trusts);

    FreeNetlogonBinding(&netr_b);
    RELEASE_SESSION_CREDS(nr);

done:
    NetrDestroyMemory();

    return (status == STATUS_SUCCESS &&
            err == ERROR_SUCCESS);
}


void SetupNetlogonTests(struct test *t)
{
    NetrInitMemory();

    AddTest(t, "NETLOGON-CREDS-TEST", TestNetlogonCredentials);
    AddTest(t, "NETLOGON-ENUM-TRUSTED-DOM" , TestNetlogonEnumTrustedDomains);
    AddTest(t, "NETLOGON-DSR-ENUM-DOMTRUSTS", TestNetlogonEnumDomainTrusts);
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
