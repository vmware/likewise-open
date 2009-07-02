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

#include "includes.h"

#define MACHPASS_LEN  (16)

static
uint32
NetWc16sHash(
    const wchar16_t *str
    )
{
    DWORD  dwLen = 0;
    DWORD  dwPos = 0;
    uint32 result = 0;
    char   *data = (char *)str;

    dwLen = wc16slen(str) * 2;

    for (dwPos = 0 ; dwPos < dwLen ; dwPos++)
    {
        if ( data[dwPos] )
        {
            // rotate result to the left 3 bits with wrap around
            result = (result << 3) | (result >> (sizeof(uint32_t)*8 - 3));
            result += data[dwPos];
        }
    }

    return result;
}

static
wchar16_t *
NetHashToWc16s(
    uint32    hash
    )
{
    char *pszValidChars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    DWORD dwNumValid = strlen(pszValidChars);
    char pszHashStr[16];
    uint32 hash_local = hash;
    DWORD dwPos = 0;
    DWORD new_char = 0;
    wchar16_t *result = NULL;

    memset(pszHashStr, 0, sizeof(pszHashStr));

    pszHashStr[dwPos++] = '-';

    while( hash_local )
    {
        new_char = hash_local % dwNumValid;
        pszHashStr[dwPos++] = pszValidChars[new_char];
        hash_local /= dwNumValid;
    }

    result = ambstowc16s(pszHashStr);

    return result;
}

static
NET_API_STATUS
NetGetAccountName(
    const wchar16_t *machname,
    const wchar16_t *domain_controller_name,
    const wchar16_t *dns_domain_name,
    wchar16_t       **account_name
    )
{
    int err = ERROR_SUCCESS;
    LDAP *ld = NULL;
    wchar16_t *base_dn = NULL;
    wchar16_t *dn = NULL;
    wchar16_t *machname_lc = NULL;
    wchar16_t *samacct = NULL;
    wchar16_t *hashstr = NULL;
    uint32    hash = 0;
    uint32    offset = 0;
    wchar16_t newname[16];
    size_t    hashstrlen = 0;

    memset( newname, 0, sizeof(newname));

    /* the host name is short enough to use as is */
    if ( wc16slen(machname) < 16 )
    {
        samacct = wc16sdup(machname);
        BAIL_ON_NO_MEMORY(samacct);
    }

    /* look for an existing account using the dns_host_name attribute */
    if ( !samacct )
    {
        machname_lc = wc16sdup(machname);
        BAIL_ON_NO_MEMORY(machname_lc);
        wc16slower(machname_lc);

        err = DirectoryConnect(domain_controller_name, &ld, &base_dn);
        BAIL_ON_WINERR_ERROR(err);

        err = MachDnsNameSearch(ld, machname_lc, base_dn, dns_domain_name, &samacct);
        if ( err == ERROR_SUCCESS )
        {
            samacct[wc16slen(samacct) - 1] = 0;
        }
        else
        {
            err = ERROR_SUCCESS;
        }
    }

     /*
      * No account was found and the name is too long so hash the host
      * name and combine with as much of the existing name as will fit
      * in the available space.  Search for an existing account with
      * that name and if a collision is detected, increment the hash
      * and try again.
      */
    if ( !samacct )
    {
        hash = NetWc16sHash(machname_lc);

        for (offset = 0 ; offset < 100 ; offset++)
        {
            hashstr = NetHashToWc16s(hash + offset);
            BAIL_ON_NO_MEMORY(hashstr);
            hashstrlen = wc16slen(hashstr);

            wc16sncpy( newname, machname, 15 - hashstrlen);
            wc16sncpy( newname + 15 - hashstrlen, hashstr, hashstrlen);

            SAFE_FREE(hashstr);

            err = MachAcctSearch( ld, newname, base_dn, &dn );
            if ( err != ERROR_SUCCESS )
            {
                err = ERROR_SUCCESS;

                samacct = wc16sdup(newname);
                BAIL_ON_NO_MEMORY(samacct);

                break;
            }
        }
        if (offset == 10)
        {
            err = ERROR_DUPLICATE_NAME;
            goto error;
        }
    }

    *account_name = samacct;

cleanup:

    if ( ld )
    {
        DirectoryDisconnect(ld);
    }

    SAFE_FREE(machname_lc);
    SAFE_FREE(hashstr);
    SAFE_FREE(dn);

    return err;

error:

    *account_name = NULL;

    SAFE_FREE(samacct);

    goto cleanup;
}


static
NET_API_STATUS
NetJoinDomainLocalInternal(
    const wchar16_t *machine,
    const wchar16_t *machine_dns_domain,
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
    handle_t lsa_b = NULL;
    handle_t samr_b = NULL;
    LsaPolicyInformation *lsa_policy_info = NULL;
    PolicyHandle account_handle;
    wchar16_t *machname = NULL;
    wchar16_t *machacct_name = NULL;
    wchar16_t machine_pass[MACHPASS_LEN+1];
    wchar16_t *account_name = NULL;
    size_t account_name_cch = 0;
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
    wchar16_t *desc_attr_name = NULL;
    wchar16_t *desc_attr_val[2] = {0};
    wchar16_t *osname_attr_name = NULL;
    wchar16_t *osname_attr_val[2] = {0};
    wchar16_t *osver_attr_name = NULL;
    wchar16_t *osver_attr_val[2] = {0};
    wchar16_t *ospack_attr_name = NULL;
    wchar16_t *ospack_attr_val[2] = {0};
    wchar16_t *sid_str = NULL;
    LW_PIO_ACCESS_TOKEN access_token = NULL;

    machname = wc16sdup(machine);
    BAIL_ON_NO_MEMORY(machname);
    wc16supper(machname);

    status = NetpGetDcName(domain, is_retry, &domain_controller_name);
    BAIL_ON_NTSTATUS_ERROR(status);

    if (account && password)
    {
        status = LwIoCreatePlainAccessTokenW(account, password, &access_token);
        BAIL_ON_NTSTATUS_ERROR(status);
    }
    else
    {
        status = LwIoGetThreadAccessToken(&access_token);
        BAIL_ON_NTSTATUS_ERROR(status);
    }

    status = NetConnectLsa(&lsa_conn,
                           domain_controller_name,
                           lsa_access,
                           access_token);
    BAIL_ON_NTSTATUS_ERROR(status);

    lsa_b = lsa_conn->lsa.bind;

    status = LsaQueryInfoPolicy2(lsa_b, &lsa_conn->lsa.policy_handle,
                                 LSA_POLICY_INFO_DNS, &lsa_policy_info);
    BAIL_ON_NTSTATUS_ERROR(status);

    dns_domain_name = GetFromUnicodeStringEx(&lsa_policy_info->dns.dns_domain);
    BAIL_ON_NO_MEMORY(dns_domain_name);

    err = NetGetAccountName(
        machname,
        domain_controller_name,
        machine_dns_domain ? machine_dns_domain : dns_domain_name,
        &machacct_name);
    BAIL_ON_WINERR_ERROR(err);

    /* If account_ou is specified pre-create disabled machine
       account object in given branch of directory. It will
       be reset afterwards by means of rpc calls */
    if (account_ou) {
        err = DirectoryConnect(domain_controller_name, &ld, &base_dn);
        BAIL_ON_WINERR_ERROR(err);

        err = MachAcctCreate(ld, machacct_name, account_ou,
                             (options & NETSETUP_DOMAIN_JOIN_IF_JOINED));
        BAIL_ON_WINERR_ERROR(err);

        err = DirectoryDisconnect(ld);
        BAIL_ON_WINERR_ERROR(err);
    }

    status = NetConnectSamr(&conn,
                            domain_controller_name,
                            domain_access,
                            0,
                            access_token);
    BAIL_ON_NTSTATUS_ERROR(status);

    samr_b = conn->samr.bind;

    get_random_string_w16(machine_pass, sizeof(machine_pass)/sizeof(wchar16_t));

    /* create account$ name */
    account_name_cch = wc16slen(machacct_name) + 2;
    account_name = (wchar16_t*) malloc(sizeof(wchar16_t) * account_name_cch);
    BAIL_ON_NO_MEMORY(account_name);

    if (sw16printfw(
                account_name,
                account_name_cch,
                L"%ws$",
                machacct_name) < 0)
    {
        status = ErrnoToNtStatus(errno);
        BAIL_ON_NTSTATUS_ERROR(status);
    }

    /* for start, let's assume the account already exists */
    newacct = false;

    status = NetOpenUser(conn, account_name, user_access, &account_handle, &rid);
    if (status == STATUS_NONE_MAPPED) {
        if (!(options & NETSETUP_ACCT_CREATE)) goto error;

        status = CreateWksAccount(conn, machacct_name, &account_handle);
        BAIL_ON_NTSTATUS_ERROR(status);

        if (machine_pass[0] == '\0') {
            BAIL_ON_NTSTATUS_ERROR(STATUS_INTERNAL_ERROR);
        }

        newacct = true;

    } else if (status == STATUS_SUCCESS &&
               !(options & NETSETUP_DOMAIN_JOIN_IF_JOINED)) {
        BAIL_ON_WINERR_ERROR(NERR_SetupAlreadyJoined);
    }
    else
    {
        BAIL_ON_NTSTATUS_ERROR(status);
    }

    status = ResetWksAccount(conn, machacct_name, &account_handle);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SetMachinePassword(conn, &account_handle, newacct, machname,
                                machine_pass);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrClose(samr_b, &account_handle);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = RtlAllocateWC16StringFromSid(&sid_str, conn->samr.dom_sid);
    if (status != STATUS_SUCCESS) {
        err = NtStatusToWin32Error(status);

        close_status = DisableWksAccount(conn, machname, &account_handle);
        BAIL_ON_NTSTATUS_ERROR(close_status);
    }

    err = SaveMachinePassword(
              machname,
              account_name,
              machine_dns_domain ? machine_dns_domain : dns_domain_name,
              conn->samr.dom_name,
              dns_domain_name,
              domain_controller_name,
              sid_str,
              machine_pass);
    if (err != ERROR_SUCCESS) {
        close_status = DisableWksAccount(conn, machacct_name, &account_handle);
        BAIL_ON_NTSTATUS_ERROR(close_status);
    }

    /* 
     * Open connection to directory server if it's going to be needed
     */
    if (!(options & NETSETUP_DEFER_SPN_SET) ||
        osname || osver) {

        err = DirectoryConnect(domain_controller_name, &ld, &base_dn);
        BAIL_ON_WINERR_ERROR(err);

        err = MachAcctSearch(ld, machacct_name, base_dn, &dn);
        BAIL_ON_WINERR_ERROR(err);

        /*
         * Set SPN and DnsHostName attributes unless this part is to be deferred
         */
        if (!(options & NETSETUP_DEFER_SPN_SET)) {
            wchar16_t *dnshostname;

            machname_lc = wc16sdup(machname);
            BAIL_ON_NO_MEMORY(machname_lc);
            wc16slower(machname_lc);

            dns_attr_name   = ambstowc16s("dNSHostName");
            dns_attr_val[0] = LdapAttrValDnsHostName(
                                  machname_lc,
                                  machine_dns_domain ?
                                  machine_dns_domain :
                                  dns_domain_name);
            dns_attr_val[1] = NULL;
            dnshostname = dns_attr_val[0];

            err = MachAcctSetAttribute(ld, dn,
                                       dns_attr_name,
                                       (const wchar16_t**)dns_attr_val, 0);
            BAIL_ON_WINERR_ERROR(err);

            spn_attr_name   = ambstowc16s("servicePrincipalName");
            spn_attr_val[0] = LdapAttrValSvcPrincipalName(dnshostname);
            spn_attr_val[1] = LdapAttrValSvcPrincipalName(machine);
            spn_attr_val[2] = NULL;

            err = MachAcctSetAttribute(ld, dn, spn_attr_name,
                                       (const wchar16_t**)spn_attr_val, 0);
            BAIL_ON_WINERR_ERROR(err);
        }

        if ( wc16scmp(machname, machacct_name) )
        {
            desc_attr_name = ambstowc16s("description");
            desc_attr_val[0] = wc16sdup(machname);
            desc_attr_val[1] = NULL;

            err = MachAcctSetAttribute(
                      ld,
                      dn,
                      desc_attr_name,
                      (const wchar16_t**)desc_attr_val,
                      0);
            BAIL_ON_WINERR_ERROR(err);
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
            else
            {
                BAIL_ON_WINERR_ERROR(err);
            }

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
            else
            {
                BAIL_ON_WINERR_ERROR(err);
            }

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
            else
            {
                BAIL_ON_WINERR_ERROR(err);
            }
        }

        err = DirectoryDisconnect(ld);
        BAIL_ON_WINERR_ERROR(err);
    }

cleanup:
    if (conn) {
        close_status = NetDisconnectSamr(conn);
        if (status == STATUS_SUCCESS &&
            close_status != STATUS_SUCCESS)
        {
            status = close_status;
        }
    }

    if (lsa_conn) {
        close_status = NetDisconnectLsa(lsa_conn);
        if (status == STATUS_SUCCESS &&
            close_status != STATUS_SUCCESS)
        {
            status = close_status;
        }
    }

    if (lsa_policy_info)
    {
        LsaRpcFreeMemory((void*)lsa_policy_info);
    }

    if (access_token)
    {
        LwIoDeleteAccessToken(access_token);
    }

    RTL_FREE(&sid_str);
    SAFE_FREE(account_name);
    SAFE_FREE(dns_domain_name);
    SAFE_FREE(machname);
    SAFE_FREE(machacct_name);
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
        err = NetJoinDomainLocalInternal(machine, machine_dns_domain,
                                         domain, account_ou,
                                         account, password, options,
                                         osname, osver, ospack, TRUE);
    }

    if (err == ERROR_SUCCESS &&
        status != STATUS_SUCCESS) {
        err = NtStatusToWin32Error(status);
    }

    return err;

error:
    goto cleanup;
}


NET_API_STATUS NetJoinDomainLocal(const wchar16_t *machine,
                                  const wchar16_t *machine_dns_domain,
                                  const wchar16_t *domain,
                                  const wchar16_t *account_ou,
                                  const wchar16_t *account,
                                  const wchar16_t *password,
                                  uint32 options,
                                  const wchar16_t *osname,
                                  const wchar16_t *osver,
                                  const wchar16_t *ospack)
{
    return NetJoinDomainLocalInternal(machine, machine_dns_domain,
                                      domain, account_ou,
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
    NET_API_STATUS status = ERROR_SUCCESS;
    char *localname = NULL;
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
    status = NetGetHostInfo(&localname);
    if (status != ERROR_SUCCESS)
    {
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

    status = NetJoinDomainLocal(host, NULL, domain, account_ou, account,
                                password, options, osName, osVersion,
                                osSvcPack);

done:
    if (localname)
    {
        NetFreeMemory(localname);
    }

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
