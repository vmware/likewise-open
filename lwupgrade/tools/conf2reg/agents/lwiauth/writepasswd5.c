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
 * Abstract: Machine trust password handling (rpc client library)
 *
 * Authors: Rafal Szczesniak (rafal@likewisesoftware.com)
 */

#include "includes.h"

typedef DWORD WINERR;

static
DWORD
SavePrincipalKey(
    const wchar16_t *name,
    const wchar16_t *pass,
    DWORD pass_len,
    const wchar16_t *realm,
    const wchar16_t *salt,
    const wchar16_t *dc_name,
    DWORD kvno)
{
    DWORD ktstatus = 0;
    NTSTATUS status = STATUS_SUCCESS;
    WINERR err = ERROR_SUCCESS;
    wchar16_t *principal = NULL;

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

error:
    goto cleanup;
}


DWORD
SaveMachinePassword(
    const wchar16_t *machine,
    const wchar16_t *machacct_name,
    const wchar16_t *mach_dns_domain,
    const wchar16_t *domain_name,
    const wchar16_t *dns_domain_name,
    const wchar16_t *dc_name,
    const wchar16_t *sid_str,
    const wchar16_t *password,
    time_t LastChangeTime,
    DWORD dwSchannelType
    )
{
    DWORD dwError = 0;
    WINERR err = ERROR_SUCCESS;
    NTSTATUS status = STATUS_SUCCESS;
    DWORD ktstatus = 0;
    wchar16_t *account = NULL;
    wchar16_t *dom_name = NULL;
    wchar16_t *ad_dns_dom_name_lc = NULL;
    wchar16_t *dns_dom_name_uc = NULL;
    wchar16_t *dns_dom_name_lc = NULL;
    wchar16_t *sid = NULL;
    wchar16_t *hostname_uc = NULL;
    wchar16_t *hostname_lc = NULL;
    wchar16_t *pass = NULL;
    LWPS_PASSWORD_INFO pi = {0};
    size_t pass_len = 0;
    DWORD kvno = 0;
    wchar16_t *base_dn = NULL;
    wchar16_t *salt = NULL;
    /* various forms of principal name for keytab */
    wchar16_t *host_machine_uc = NULL;
    wchar16_t *host_machine_lc = NULL;
    wchar16_t *host_machine_fqdn = NULL;
    size_t host_machine_fqdn_size = 0;
    wchar16_t *cifs_machine_fqdn = NULL;
    size_t cifs_machine_fqdn_size = 0;
    wchar16_t *principal = NULL;

    dwError = LwAllocateWc16String(&account, machacct_name);
    BAIL_ON_UP_ERROR(dwError);

    dwError = LwAllocateWc16String(&dom_name, domain_name);
    BAIL_ON_UP_ERROR(dwError);

    dwError = LwAllocateWc16String(&ad_dns_dom_name_lc, dns_domain_name);
    BAIL_ON_UP_ERROR(dwError);

    wc16slower(ad_dns_dom_name_lc);

    dwError = LwAllocateWc16String(&dns_dom_name_uc, mach_dns_domain);
    BAIL_ON_UP_ERROR(dwError);

    wc16supper(dns_dom_name_uc);

    dwError = LwAllocateWc16String(&dns_dom_name_lc, mach_dns_domain);
    BAIL_ON_UP_ERROR(dwError);

    wc16slower(dns_dom_name_lc);

    dwError = LwAllocateWc16String(&sid, sid_str);
    BAIL_ON_UP_ERROR(dwError);

    dwError = LwAllocateWc16String(&hostname_uc, machine);
    BAIL_ON_UP_ERROR(dwError);

    wc16supper(hostname_uc);

    dwError = LwAllocateWc16String(&hostname_lc, machine);
    BAIL_ON_UP_ERROR(dwError);

    wc16slower(hostname_lc);

    dwError = LwAllocateWc16String(&pass, password);
    BAIL_ON_UP_ERROR(dwError);

    /*
     * Store the machine password first
     */

    pi.pwszDomainName      = dom_name;
    pi.pwszDnsDomainName   = ad_dns_dom_name_lc;
    pi.pwszSID             = sid;
    pi.pwszHostname        = hostname_uc;
    pi.pwszHostDnsDomain   = dns_dom_name_lc;
    pi.pwszMachineAccount  = account;
    pi.pwszMachinePassword = pass;
    pi.last_change_time    = LastChangeTime;
    pi.dwSchannelType      = dwSchannelType;

    status = LwpsWritePasswordToAllStores(&pi);
    if (status != STATUS_SUCCESS) {
        err = NtStatusToWin32Error(status);
        goto error;
    }

    pass_len = wc16slen(pass);

    /* TODO: sort out error code propagation from libkeytab functions */


    /*
     * Find the current key version number for machine account
     */

    ktstatus = KtKrb5FormatPrincipalW(account, dns_dom_name_uc, &principal);
    if (ktstatus != 0) {
        err = NtStatusToWin32Error(STATUS_UNSUCCESSFUL);
        goto error;
    }

    /* Get the directory base naming context first */
    ktstatus = KtLdapGetBaseDnW(dc_name, &base_dn);
    if (ktstatus != 0) {
        err = NtStatusToWin32Error(STATUS_UNSUCCESSFUL);
        goto error;
    }

    ktstatus = KtLdapGetKeyVersionW(dc_name, base_dn, principal, &kvno);
    if (ktstatus == KT_STATUS_LDAP_NO_KVNO_FOUND) {
        /* This is probably win2k DC we're talking to, because it doesn't
           store kvno in directory. In such case return default key version */
        kvno = 0;

    } else if (ktstatus != 0) {
        err = NtStatusToWin32Error(STATUS_UNSUCCESSFUL);
        goto error;
    }

    ktstatus = KtGetSaltingPrincipalW(machine, account, mach_dns_domain, dns_dom_name_uc,
                                      dc_name, base_dn, &salt);
    if (ktstatus != 0) {
        err = NtStatusToWin32Error(STATUS_UNSUCCESSFUL);
        goto error;

    } else if (ktstatus == 0 && salt == NULL) {
        salt = wc16sdup(principal);
    }

    /*
     * Update keytab records with various forms of machine principal
     */

    /* MACHINE$@DOMAIN.NET */
    err = SavePrincipalKey(account, pass, pass_len, dns_dom_name_uc, salt, dc_name, kvno);
    BAIL_ON_WINERR_ERROR(err);

    /* host/MACHINE@DOMAIN.NET */

    host_machine_uc = asw16printfw(L"host/%ws", hostname_uc);
    if (host_machine_uc == NULL)
    {
        err = ErrnoToWin32Error(errno);
        BAIL_ON_WINERR_ERROR(err);
    }

    err = SavePrincipalKey(host_machine_uc, pass, pass_len, dns_dom_name_uc, salt,
                           dc_name, kvno);
    BAIL_ON_WINERR_ERROR(err);

    /* host/machine.domain.net@DOMAIN.NET */
    host_machine_fqdn_size = wc16slen(hostname_lc) +
                             wc16slen(dns_dom_name_lc) +
                             8;
    host_machine_fqdn = (wchar16_t*) malloc(sizeof(wchar16_t) *
                                            host_machine_fqdn_size);
    BAIL_ON_NO_MEMORY(host_machine_fqdn);

    if (sw16printfw(
                host_machine_fqdn,
                host_machine_fqdn_size,
                L"host/%ws.%ws",
                hostname_lc,
                dns_dom_name_lc) < 0)
    {
        err = ErrnoToWin32Error(errno);
        BAIL_ON_WINERR_ERROR(err);
    }

    err = SavePrincipalKey(host_machine_fqdn, pass, pass_len, dns_dom_name_uc, salt,
                           dc_name, kvno);
    BAIL_ON_WINERR_ERROR(err);

    if (sw16printfw(
                host_machine_fqdn,
                host_machine_fqdn_size,
                L"host/%ws.%ws",
                hostname_uc,
                dns_dom_name_uc) < 0)
    {
        err = ErrnoToWin32Error(errno);
        BAIL_ON_WINERR_ERROR(err);
    }

    err = SavePrincipalKey(host_machine_fqdn, pass, pass_len, dns_dom_name_uc, salt,
                           dc_name, kvno);
    BAIL_ON_WINERR_ERROR(err);

    if (sw16printfw(
                host_machine_fqdn,
                host_machine_fqdn_size,
                L"host/%ws.%ws",
                hostname_uc,
                dns_dom_name_lc) < 0)
    {
        err = ErrnoToWin32Error(errno);
        BAIL_ON_WINERR_ERROR(err);
    }

    err = SavePrincipalKey(host_machine_fqdn, pass, pass_len, dns_dom_name_uc, salt,
                           dc_name, kvno);
    BAIL_ON_WINERR_ERROR(err);

    if (sw16printfw(
                host_machine_fqdn,
                host_machine_fqdn_size,
                L"host/%ws.%ws",
                hostname_lc,
                dns_dom_name_uc) < 0)
    {
        err = ErrnoToWin32Error(errno);
        BAIL_ON_WINERR_ERROR(err);
    }

    err = SavePrincipalKey(host_machine_fqdn, pass, pass_len, dns_dom_name_uc, salt,
                           dc_name, kvno);
    BAIL_ON_WINERR_ERROR(err);

    /* host/machine@DOMAIN.NET */
    host_machine_lc = asw16printfw(L"host/%ws", hostname_lc);
    if (host_machine_lc == NULL)
    {
        err = ErrnoToWin32Error(errno);
        BAIL_ON_WINERR_ERROR(err);
    }

    err = SavePrincipalKey(host_machine_lc, pass, pass_len, dns_dom_name_uc, salt,
                           dc_name, kvno);
    BAIL_ON_WINERR_ERROR(err);

    /* cifs/machine.domain.net@DOMAIN.NET */
    cifs_machine_fqdn_size = wc16slen(hostname_lc) +
                             wc16slen(dns_dom_name_lc) +
                             8;
    cifs_machine_fqdn = (wchar16_t*) malloc(sizeof(wchar16_t) *
                                            cifs_machine_fqdn_size);
    BAIL_ON_NO_MEMORY(cifs_machine_fqdn);

    if (sw16printfw(
                cifs_machine_fqdn,
                cifs_machine_fqdn_size,
                L"cifs/%ws.%ws",
                hostname_lc,
                dns_dom_name_lc) < 0)
    {
        err = ErrnoToWin32Error(errno);
        BAIL_ON_WINERR_ERROR(err);
    }

    err = SavePrincipalKey(cifs_machine_fqdn, pass, pass_len, dns_dom_name_uc, salt,
                           dc_name, kvno);
    BAIL_ON_WINERR_ERROR(err);

    if (sw16printfw(
                cifs_machine_fqdn,
                cifs_machine_fqdn_size,
                L"cifs/%ws.%ws",
                hostname_uc,
                dns_dom_name_uc) < 0)
    {
        err = ErrnoToWin32Error(errno);
        BAIL_ON_WINERR_ERROR(err);
    }

    err = SavePrincipalKey(cifs_machine_fqdn, pass, pass_len, dns_dom_name_uc, salt,
                           dc_name, kvno);
    BAIL_ON_WINERR_ERROR(err);

    if (sw16printfw(
                cifs_machine_fqdn,
                cifs_machine_fqdn_size,
                L"cifs/%ws.%ws",
                hostname_uc,
                dns_dom_name_lc) < 0)
    {
        err = ErrnoToWin32Error(errno);
        BAIL_ON_WINERR_ERROR(err);
    }

    err = SavePrincipalKey(cifs_machine_fqdn, pass, pass_len, dns_dom_name_uc, salt,
                               dc_name, kvno);
    BAIL_ON_WINERR_ERROR(err);

    if (sw16printfw(
                cifs_machine_fqdn,
                cifs_machine_fqdn_size,
                L"cifs/%ws.%ws",
                hostname_lc,
                dns_dom_name_uc) < 0)
    {
        err = ErrnoToWin32Error(errno);
        BAIL_ON_WINERR_ERROR(err);
    }

    err = SavePrincipalKey(cifs_machine_fqdn, pass, pass_len, dns_dom_name_uc, salt,
                               dc_name, kvno);
    BAIL_ON_WINERR_ERROR(err);

cleanup:
    if (base_dn) KtFreeMemory(base_dn);
    if (salt) KtFreeMemory(salt);

    LW_SAFE_FREE_MEMORY(dom_name);
    LW_SAFE_FREE_MEMORY(ad_dns_dom_name_lc);
    LW_SAFE_FREE_MEMORY(dns_dom_name_lc);
    LW_SAFE_FREE_MEMORY(dns_dom_name_uc);
    LW_SAFE_FREE_MEMORY(sid);
    LW_SAFE_FREE_MEMORY(hostname_lc);
    LW_SAFE_FREE_MEMORY(hostname_uc);
    LW_SAFE_FREE_MEMORY(pass);
    LW_SAFE_FREE_MEMORY(account);
    LW_SAFE_FREE_MEMORY(host_machine_uc);
    LW_SAFE_FREE_MEMORY(host_machine_lc);
    LW_SAFE_FREE_MEMORY(host_machine_fqdn);
    LW_SAFE_FREE_MEMORY(cifs_machine_fqdn);
    LW_SAFE_FREE_MEMORY(principal);

    return dwError;

error:
    goto cleanup;
}

DWORD
MovePassword(
    VOID
    )
{
    DWORD dwError = 0;
    PLWPS_PASSWORD_INFO* pInfo = NULL;

    dwError = LwpsOpenPasswordStore(LWPS_PASSWORD_STORE_TDB, &hStore);
    BAIL_ON_UP_ERROR(dwError);

    dwError = LwpsGetPasswordByCurrentHostName(hStore, &pInfo);
    BAIL_ON_UP_ERROR(dwError);

    dwError = LwpsClosePasswordStore(hStore);
    BAIL_ON_UP_ERROR(dwError);
    hStore = NULL;

    dwError = LwpsWritePasswordToAllStores(pInfo);
    BAIL_ON_UP_ERROR(dwError);

cleanup:
    return dwError;
error:
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
