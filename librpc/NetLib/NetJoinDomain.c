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
#include <config.h>

#include <sys/utsname.h>

#include "includes.h"

#if HAVE_SYS_VARARGS_H
#include <sys/varargs.h>
#endif

#include <random.h>
#include <lwrpc/mpr.h>
#include <lwps/lwps.h>
#include <keytab.h>
#include <lwio/lwio.h>

#include "NetUtil.h"
#include "NetGetDcName.h"

#define MACHPASS_LEN  (16)

static
NET_API_STATUS
NetJoinDomainLocalInternal(
    const wchar16_t *machine,
    const wchar16_t *domain,
    const wchar16_t *account_ou,
    const wchar16_t *account,
    const wchar16_t *password,
    uint32 options,
    const wchar16_t *osname,
    const wchar16_t *osver,
    const wchar16_t *ospack,
    BOOLEAN is_retry
    )
{
    const uint32 lsa_access = LSA_ACCESS_LOOKUP_NAMES_SIDS |
                              LSA_ACCESS_VIEW_POLICY_INFO;
    const uint32 domain_access  = DOMAIN_ACCESS_ENUM_ACCOUNTS |
                                  DOMAIN_ACCESS_OPEN_ACCOUNT |
                                  DOMAIN_ACCESS_LOOKUP_INFO_2 |
                                  DOMAIN_ACCESS_CREATE_USER;
    const uint32 user_access = USER_ACCESS_GET_ATTRIBUTES |
                               USER_ACCESS_SET_ATTRIBUTES |
                               USER_ACCESS_SET_PASSWORD;

    NTSTATUS status = STATUS_SUCCESS;
    NTSTATUS close_status = STATUS_SUCCESS;
    int err = ERROR_SUCCESS;
    NETRESOURCE nr = {0};
    size_t domain_controller_len;
    handle_t lsa_b, samr_b;
    LsaPolicyInformation *lsa_policy_info = NULL;
    PolicyHandle account_handle;
    wchar16_t *machname = NULL;
    wchar16_t machine_pass[MACHPASS_LEN+1];
    wchar16_t *account_name = NULL;
    wchar16_t *dns_domain_name = NULL;
    wchar16_t *domain_controller_name = NULL;
    uint32 rid, newacct;
    NetConn *conn = NULL;
    NetConn *lsa_conn = NULL;
    LDAP *ld = NULL;
    wchar16_t *machname_lc = NULL;    /* machine name lower cased */
    wchar16_t *base_dn = NULL;
    wchar16_t *dn = NULL;
    wchar16_t *dns_attr_name = NULL;
    wchar16_t *dns_attr_val[2] = {0};
    wchar16_t *spn_attr_name = NULL;
    wchar16_t *spn_attr_val[3] = {0};
    wchar16_t *osname_attr_name = NULL;
    wchar16_t *osname_attr_val[2] = {0};
    wchar16_t *osver_attr_name = NULL;
    wchar16_t *osver_attr_val[2] = {0};
    wchar16_t *ospack_attr_name = NULL;
    wchar16_t *ospack_attr_val[2] = {0};
    wchar16_t *sid_str = NULL;

    machname = wc16sdup(machine);
    goto_if_no_memory_winerr(machname, done);
    wc16supper(machname);

    status = NetpGetDcName(domain, is_retry, &domain_controller_name);
    goto_if_ntstatus_not_success(status, done);

    domain_controller_len = wc16slen(domain_controller_name);
    nr.RemoteName = (wchar16_t*) malloc((domain_controller_len + 8) * sizeof(wchar16_t));
    goto_if_no_memory_winerr(nr.RemoteName, done);

    /* specify credentials for domain controller connection */
    sw16printf(nr.RemoteName, "\\\\%S\\IPC$", domain_controller_name);

    if (account && password)
    {
        /* Set up access token */
        LW_PIO_ACCESS_TOKEN hAccessToken = NULL;

        status = LwIoCreatePlainAccessTokenW(account, password, &hAccessToken);
        goto_if_ntstatus_not_success(status, done);

        status = LwIoSetThreadAccessToken(hAccessToken);
        goto_if_ntstatus_not_success(status, done);

        LwIoDeleteAccessToken(hAccessToken);
    }

    status = NetConnectLsa(&lsa_conn, domain_controller_name, lsa_access);
    goto_if_ntstatus_not_success(status, close);

    lsa_b = lsa_conn->lsa.bind;

    status = LsaQueryInfoPolicy2(lsa_b, &lsa_conn->lsa.policy_handle,
                                 LSA_POLICY_INFO_DNS, &lsa_policy_info);
    goto_if_ntstatus_not_success(status, disconn_lsa);

    dns_domain_name = GetFromUnicodeStringEx(&lsa_policy_info->dns.dns_domain);
    goto_if_no_memory_winerr(dns_domain_name, disconn_lsa);

    /* If account_ou is specified pre-create disabled machine
       account object in given branch of directory. It will
       be reset afterwards by means of rpc calls */
    if (account_ou) {
        err = DirectoryConnect(domain_controller_name, &ld, &base_dn);
        goto_if_winerr_not_success(err, disconn_lsa);

        err = MachAcctCreate(ld, machname, account_ou,
                             (options & NETSETUP_DOMAIN_JOIN_IF_JOINED));
        goto_if_winerr_not_success(err, disconn_lsa);

        err = DirectoryDisconnect(ld);
        goto_if_winerr_not_success(err, disconn_lsa);
    }

    status = NetConnectSamr(&conn, domain_controller_name, domain_access, 0);
    goto_if_ntstatus_not_success(status, disconn_lsa);

    samr_b = conn->samr.bind;

    get_random_string_w16(machine_pass, sizeof(machine_pass)/sizeof(wchar16_t));

    /* create account$ name */
    account_name = (wchar16_t*) malloc(sizeof(wchar16_t) *
                                       (wc16slen(machname) + 2));
    goto_if_no_memory_winerr(account_name, disconn_samr);

    sw16printf(account_name, "%S$", machname);

    /* for start, let's assume the account already exists */
    newacct = false;

    status = NetOpenUser(conn, account_name, user_access, &account_handle, &rid);
    if (status == STATUS_NONE_MAPPED) {
        if (!(options & NETSETUP_ACCT_CREATE)) goto disconn_samr;

        status = CreateWksAccount(conn, machname, &account_handle);
        goto_if_ntstatus_not_success(status, disconn_samr);

        if (machine_pass[0] == '\0') {
            status = STATUS_INTERNAL_ERROR;
            goto_if_ntstatus_not_success(status, disconn_samr);
        }

        newacct = true;

    } else if (status == STATUS_SUCCESS &&
               !(options & NETSETUP_DOMAIN_JOIN_IF_JOINED)) {
        err = NERR_SetupAlreadyJoined;
        goto disconn_samr;
    }
    else
    {
        goto_if_ntstatus_not_success(status, disconn_samr);
    }

    status = ResetWksAccount(conn, machname, &account_handle);
    goto_if_ntstatus_not_success(status, disconn_samr);

    status = SetMachinePassword(conn, &account_handle, newacct, machname,
                                machine_pass);
    goto_if_ntstatus_not_success(status, disconn_samr);

    status = SamrClose(samr_b, &account_handle);
    goto_if_ntstatus_not_success(status, disconn_samr);

    status = SidToStringW(conn->samr.dom_sid, &sid_str);
    if (status != STATUS_SUCCESS) {
        err = NtStatusToWin32Error(status);

        close_status = DisableWksAccount(conn, machname, &account_handle);
        goto_if_ntstatus_not_success(close_status, close);

        goto disconn_samr;
    }

    err = SaveMachinePassword(machname, conn->samr.dom_name, dns_domain_name,
                              domain_controller_name, sid_str,
                              machine_pass);
    if (err != ERROR_SUCCESS) {
        close_status = DisableWksAccount(conn, machname, &account_handle);
        goto_if_ntstatus_not_success(close_status, close);

        goto disconn_samr;
    }

    /* 
     * Open connection to directory server if it's going to be needed
     */
    if (!(options & NETSETUP_DEFER_SPN_SET) ||
        osname || osver) {

        err = DirectoryConnect(domain_controller_name, &ld, &base_dn);
        goto_if_winerr_not_success(err, disconn_samr);

        err = MachAcctSearch(ld, machname, base_dn, &dn);
        goto_if_winerr_not_success(err, disconn_samr);

        /*
         * Set SPN and DnsHostName attributes unless this part is to be deferred
         */
        if (!(options & NETSETUP_DEFER_SPN_SET)) {
            wchar16_t *dnshostname;

            machname_lc = wc16sdup(machname);
            goto_if_no_memory_winerr(machname_lc, disconn_samr);
            wc16slower(machname_lc);

            dns_attr_name   = ambstowc16s("dNSHostName");
            dns_attr_val[0] = LdapAttrValDnsHostName(machname_lc,
                                                     dns_domain_name);
            dns_attr_val[1] = NULL;
            dnshostname = dns_attr_val[0];

            err = MachAcctSetAttribute(ld, dn, dns_attr_name,
	                               (const wchar16_t**)dns_attr_val, 0);
            goto_if_winerr_not_success(err, disconn_samr);

            spn_attr_name   = ambstowc16s("servicePrincipalName");
            spn_attr_val[0] = LdapAttrValSvcPrincipalName(dnshostname);
            spn_attr_val[1] = LdapAttrValSvcPrincipalName(machine);
            spn_attr_val[2] = NULL;

            err = MachAcctSetAttribute(ld, dn, spn_attr_name,
				       (const wchar16_t**)spn_attr_val, 0);
            goto_if_winerr_not_success(err, disconn_samr);
        }

        /*
         * Set operating system name and version attributes if specified
         */
        if (osname || osver) {
            osname_attr_name = ambstowc16s("operatingSystem");
            osname_attr_val[0] = wc16sdup(osname);
            osname_attr_val[1] = NULL;

            err = MachAcctSetAttribute(ld, dn,
                                       osname_attr_name,
				       (const wchar16_t**)osname_attr_val,
                                       0);
            if (err == ERROR_ACCESS_DENIED)
            {
                /* The user must be a non-admin. In this case, we cannot
                 * set the attribute.
                 */
                err = ERROR_SUCCESS;
            }
            goto_if_winerr_not_success(err, disconn_samr);

            osver_attr_name = ambstowc16s("operatingSystemVersion");
            osver_attr_val[0] = wc16sdup(osver);
            osver_attr_val[1] = NULL;

            err = MachAcctSetAttribute(ld, dn,
                                       osver_attr_name,
				       (const wchar16_t**)osver_attr_val,
                                       0);
            if (err == ERROR_ACCESS_DENIED)
            {
                err = ERROR_SUCCESS;
            }
            goto_if_winerr_not_success(err, disconn_samr);

            ospack_attr_name = ambstowc16s("operatingSystemServicePack");
            ospack_attr_val[0] = wc16sdup(ospack);
            ospack_attr_val[1] = NULL;

            err = MachAcctSetAttribute(ld, dn,
                                       ospack_attr_name,
				       (const wchar16_t**)ospack_attr_val,
                                       0);
            if (err == ERROR_ACCESS_DENIED)
            {
                err = ERROR_SUCCESS;
            }
            goto_if_winerr_not_success(err, disconn_samr);
        }

        err = DirectoryDisconnect(ld);
        goto_if_winerr_not_success(err, disconn_samr);
    }

disconn_samr:

    close_status = NetDisconnectSamr(conn);
    if (status == STATUS_SUCCESS &&
        close_status != STATUS_SUCCESS) {
        return NtStatusToWin32Error(close_status);
    }

disconn_lsa:

    close_status = NetDisconnectLsa(lsa_conn);
    if (status == STATUS_SUCCESS &&
        close_status != STATUS_SUCCESS) {
        return NtStatusToWin32Error(close_status);
    }

close:

    /* release domain controller connection creds */
    if (account && password)
    {
        status = LwIoSetThreadAccessToken(NULL);
        goto_if_ntstatus_not_success(status, done);
    }

done:
    if (lsa_policy_info) {
        LsaRpcFreeMemory((void*)lsa_policy_info);
    }

    if (sid_str) {
        SidStrFreeW(sid_str);
    }

    SAFE_FREE(nr.RemoteName);
    SAFE_FREE(account_name);
    SAFE_FREE(dns_domain_name);
    SAFE_FREE(machname);
    SAFE_FREE(machname_lc);
    SAFE_FREE(base_dn);
    SAFE_FREE(dn);
    SAFE_FREE(dns_attr_name);
    SAFE_FREE(dns_attr_val[0]);
    SAFE_FREE(spn_attr_name);
    SAFE_FREE(spn_attr_val[0]);
    SAFE_FREE(spn_attr_val[1]);
    SAFE_FREE(osname_attr_name);
    SAFE_FREE(osname_attr_val[0]);
    SAFE_FREE(osver_attr_name);
    SAFE_FREE(osver_attr_val[0]);
    SAFE_FREE(ospack_attr_name);
    SAFE_FREE(ospack_attr_val[0]);
    SAFE_FREE(domain_controller_name);

    if (err && !is_retry)
    {
        err = NetJoinDomainLocalInternal(machine, domain, account_ou,
                                         account, password, options,
                                         osname, osver, ospack, TRUE);
    }

    return err;
}

NET_API_STATUS NetJoinDomainLocal(const wchar16_t *machine,
                                  const wchar16_t *domain,
                                  const wchar16_t *account_ou,
                                  const wchar16_t *account,
                                  const wchar16_t *password,
                                  uint32 options,
                                  const wchar16_t *osname,
                                  const wchar16_t *osver,
                                  const wchar16_t *ospack)
{
    return NetJoinDomainLocalInternal(machine, domain, account_ou,
                                      account, password, options,
                                      osname, osver, ospack, FALSE);
}


#if !defined(MAXHOSTNAMELEN)
#define MAXHOSTNAMELEN (256)
#endif

NET_API_STATUS NetJoinDomain(const wchar16_t *hostname,
                             const wchar16_t *domain,
                             const wchar16_t *account_ou,
                             const wchar16_t *account,
                             const wchar16_t *password,
                             uint32 options)
{
    NET_API_STATUS status;
    char localname[MAXHOSTNAMELEN];
    wchar16_t host[MAXHOSTNAMELEN];
    wchar16_t *osName = NULL;
    wchar16_t *osVersion = NULL;
    wchar16_t *osSvcPack = NULL;
    struct utsname osname;

    /* at the moment we support only locally triggered join */
    if (hostname) {
        status = ERROR_INVALID_PARAMETER;
        goto done;
    }

	if (!(options & NETSETUP_JOIN_DOMAIN)) return ERROR_INVALID_FUNCTION;

    /*
      Get local host name to pass for local join
    */
	if (gethostname((char*)localname, sizeof(localname)) < 0) {
        status = ERROR_INTERNAL_ERROR;
	    goto done;
	}
	mbstowc16s(host, localname, sizeof(wchar16_t)*MAXHOSTNAMELEN);

    /*
      Get operating system name&version information
    */
    if (uname(&osname) < 0) {
        osName    = NULL;
        osVersion = NULL;
        osSvcPack = NULL;

    } else {
        osName    = ambstowc16s(osname.sysname);
        osVersion = ambstowc16s(osname.release);
        osSvcPack = ambstowc16s(" ");
    }

    status = NetJoinDomainLocal(host, domain, account_ou, account,
                                password, options, osName, osVersion,
                                osSvcPack);

done:
    SAFE_FREE(osName);
    SAFE_FREE(osVersion);
    SAFE_FREE(osSvcPack);

    return status;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
