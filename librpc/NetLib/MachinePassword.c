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

#include <sys/utsname.h>

#include "includes.h"

#include <lwps/lwps.h>
#include <keytab.h>

#include "NetUtil.h"


static WINERR SavePrincipalKey(const wchar16_t *name, const wchar16_t *pass,
                               uint32 pass_len, const wchar16_t *realm,
                               const wchar16_t *salt, const wchar16_t *dc_name,
                               uint32 kvno)
{
    uint32 ktstatus = 0;
    WINERR err = ERROR_SUCCESS;
    wchar16_t *principal = NULL;

    goto_if_invalid_param_winerr(name, cleanup);
    goto_if_invalid_param_winerr(pass, cleanup);
    goto_if_invalid_param_winerr(dc_name, cleanup);

    ktstatus = KtKrb5FormatPrincipalW(name, realm, &principal);
    if (ktstatus != 0) {
        err = NtStatusToWin32Error(STATUS_UNSUCCESSFUL);
        goto cleanup;
    }

    ktstatus = KtKrb5AddKeyW(principal, (void*)pass, pass_len,
                             NULL, salt, dc_name, kvno);
    if (ktstatus != 0) {
        err = NtStatusToWin32Error(STATUS_UNSUCCESSFUL);
        goto cleanup;
    }

cleanup:
    if (principal)
    {
        KtFreeMemory(principal);
    }

    return err;
}


WINERR SaveMachinePassword(const wchar16_t *machine,
                           const wchar16_t *domain_name,
                           const wchar16_t *dns_domain_name,
                           const wchar16_t *dc_name,
                           const wchar16_t *sid_str,
                           const wchar16_t *password)
{
    WINERR err = ERROR_SUCCESS;
    NTSTATUS status = STATUS_SUCCESS;
    uint32 ktstatus = 0;
    wchar16_t *account = NULL;
    wchar16_t *dom_name = NULL;
    wchar16_t *dns_dom_name = NULL;
    wchar16_t *sid = NULL;
    wchar16_t *hostname = NULL;
    wchar16_t *pass = NULL;
    LWPS_PASSWORD_INFO pi = {0};
    size_t pass_len = 0;
    uint32 kvno = 0;
    wchar16_t *base_dn = NULL;
    wchar16_t *salt = NULL;
    /* various forms of principal name for keytab */
    wchar16_t *host_machine_uc = NULL;
    wchar16_t *host_machine_lc = NULL;
    wchar16_t *host_machine_fqdn_lc = NULL;
    wchar16_t *principal = NULL;

    /* create account$ name */
    account = (wchar16_t*) malloc(sizeof(wchar16_t) * (wc16slen(machine) + 2));
    goto_if_no_memory_winerr(account, done);

    sw16printf(account, "%S$", machine);

    dom_name = wc16sdup(domain_name);
    goto_if_no_memory_winerr(dom_name, done);

    dns_dom_name = wc16sdup(dns_domain_name);
    goto_if_no_memory_winerr(dns_dom_name, done);

    sid = wc16sdup(sid_str);
    goto_if_no_memory_winerr(sid, done);

    hostname = wc16sdup(machine);
    goto_if_no_memory_winerr(hostname, done);

    pass = wc16sdup(password);
    goto_if_no_memory_winerr(pass, done);

    /*
     * Store the machine password first
     */
    
    pi.pwszDomainName      = dom_name;
    pi.pwszDnsDomainName   = dns_dom_name;
    pi.pwszSID             = sid;
    pi.pwszHostname        = hostname;
    pi.pwszMachineAccount  = account;
    pi.pwszMachinePassword = pass;
    pi.last_change_time    = time(NULL);
    pi.dwSchannelType      = SCHANNEL_WKSTA;

    status = LwpsWritePasswordToAllStores(&pi);
    if (status != STATUS_SUCCESS) {
        err = NtStatusToWin32Error(status);
        goto done;
    }

    pass_len = wc16slen(pass);

    /* TODO: sort out error code propagation from libkeytab functions */


    /*
     * Find the current key version number for machine account
     */

    ktstatus = KtKrb5FormatPrincipalW(account, NULL, &principal);
    if (ktstatus != 0) {
        err = NtStatusToWin32Error(STATUS_UNSUCCESSFUL);
        goto done;
    }

    /* Get the directory base naming context first */
    ktstatus = KtLdapGetBaseDnW(dc_name, &base_dn);
    if (ktstatus != 0) {
        err = NtStatusToWin32Error(STATUS_UNSUCCESSFUL);
        goto done;
    }

    ktstatus = KtLdapGetKeyVersionW(dc_name, base_dn, principal, &kvno);
    if (ktstatus == KT_STATUS_LDAP_NO_KVNO_FOUND) {
        /* This is probably win2k DC we're talking to, because it doesn't
           store kvno in directory. In such case return default key version */
        kvno = 0;

    } else if (ktstatus != 0) {
        err = NtStatusToWin32Error(STATUS_UNSUCCESSFUL);
        goto done;
    }

    ktstatus = KtGetSaltingPrincipalW(hostname, dns_domain_name, NULL,
                                      dc_name, base_dn, &salt);
    if (ktstatus != 0) {
        err = NtStatusToWin32Error(STATUS_UNSUCCESSFUL);
        goto done;
        
    } else if (ktstatus == 0 && salt == NULL) {
        salt = wc16sdup(principal);
    }

    /*
     * Update keytab records with various forms of machine principal
     */

    /* MACHINE$@DOMAIN.NET */
    err = SavePrincipalKey(account, pass, pass_len, NULL, salt, dc_name, kvno);
    goto_if_winerr_not_success(err, done);

    /* host/MACHINE@DOMAIN.NET */
    host_machine_uc = (wchar16_t*) malloc(sizeof(wchar16_t) *
                                          (wc16slen(hostname) + 6));
    goto_if_no_memory_winerr(host_machine_uc, done);

    wc16supper(hostname);
    sw16printf(host_machine_uc, "host/%S", hostname);

    err = SavePrincipalKey(host_machine_uc, pass, pass_len, NULL, salt,
                           dc_name, kvno);
    goto_if_winerr_not_success(err, done);

    /* host/machine.domain.net@DOMAIN.NET */
    host_machine_fqdn_lc = (wchar16_t*) malloc(sizeof(wchar16_t) *
                                               (wc16slen(hostname) +
                                                wc16slen(dns_domain_name) + 8));
    goto_if_no_memory_winerr(host_machine_fqdn_lc, done);

    sw16printf(host_machine_fqdn_lc, "host/%S.%S", hostname, dns_domain_name);
    wc16slower(host_machine_fqdn_lc);

    err = SavePrincipalKey(host_machine_fqdn_lc, pass, pass_len, NULL, salt,
                           dc_name, kvno);
    goto_if_winerr_not_success(err, done);

    /* host/machine@DOMAIN.NET */
    host_machine_lc = (wchar16_t*) malloc(sizeof(wchar16_t) *
                                          (wc16slen(hostname) + 6));
    goto_if_no_memory_winerr(host_machine_lc, done);

    sw16printf(host_machine_lc, "host/%S", hostname);
    wc16slower(host_machine_lc);

    err = SavePrincipalKey(host_machine_lc, pass, pass_len, NULL, salt,
                           dc_name, kvno);
    goto_if_winerr_not_success(err, done);

    /* cifs/machine.domain.net@DOMAIN.NET */
    cifs_machine_fqdn_lc = (wchar16_t*) malloc(sizeof(wchar16_t) *
                                               (wc16slen(hostname) +
                                                wc16slen(dns_domain_name) + 8));
    goto_if_no_memory_winerr(cifs_machine_fqdn_lc, done);

    sw16printf(cifs_machine_fqdn_lc, "cifs/%S.%S", hostname, dns_domain_name);
    wc16slower(cifs_machine_fqdn_lc);

    err = SavePrincipalKey(cifs_machine_fqdn_lc, pass, pass_len, NULL, salt,
                           dc_name, kvno);
    goto_if_winerr_not_success(err, done);

done:
    if (base_dn) KtFreeMemory(base_dn);
    if (salt) KtFreeMemory(salt);

    SAFE_FREE(dom_name);
    SAFE_FREE(dns_dom_name);
    SAFE_FREE(sid);
    SAFE_FREE(hostname);
    SAFE_FREE(pass);
    SAFE_FREE(account);
    SAFE_FREE(host_machine_uc);
    SAFE_FREE(host_machine_lc);
    SAFE_FREE(host_machine_fqdn_lc);
    SAFE_FREE(principal);

    return err;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
