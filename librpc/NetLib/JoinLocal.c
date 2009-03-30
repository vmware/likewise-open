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


NTSTATUS
ResetAccountPasswordTimer(
    handle_t samr_b,
    PolicyHandle *account_h
    )
{
    const uint32 flags_enable  = ACB_WSTRUST;
    const uint32 flags_disable = ACB_WSTRUST | ACB_DISABLED;
    const uint32 level = 16;

    NTSTATUS status = STATUS_SUCCESS;
    WINERR err = ERROR_SUCCESS;
    UserInfo info;
    UserInfo16 *info16 = NULL;

    memset((void*)&info, 0, sizeof(info));
    info16 = &info.info16;

    goto_if_invalid_param_ntstatus(samr_b, cleanup);
    goto_if_invalid_param_ntstatus(account_h, cleanup);

    /* flip ACB_DISABLED flag - this way password timeout counter
       gets restarted */

    info16->account_flags = flags_enable;
    status = SamrSetUserInfo(samr_b, account_h, level, &info);
    goto_if_ntstatus_not_success(status, error);

    info16->account_flags = flags_disable;
    status = SamrSetUserInfo(samr_b, account_h, level, &info);
    goto_if_ntstatus_not_success(status, error);

    info16->account_flags = flags_enable;
    status = SamrSetUserInfo(samr_b, account_h, level, &info);
    goto_if_ntstatus_not_success(status, error);

cleanup:
    return status;

error:
    goto cleanup;
}


NTSTATUS
ResetWksAccount(
    NetConn *conn,
    wchar16_t *name,
    PolicyHandle *account_h
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    WINERR err = ERROR_SUCCESS;
    handle_t samr_b = NULL;
    UserInfo *info = NULL;

    goto_if_invalid_param_ntstatus(conn, cleanup);
    goto_if_invalid_param_ntstatus(name, cleanup);
    goto_if_invalid_param_ntstatus(account_h, cleanup);

    samr_b   = conn->samr.bind;

    status = SamrQueryUserInfo(samr_b, account_h, 16, &info);
    if (status == STATUS_SUCCESS &&
        !(info->info16.account_flags & ACB_WSTRUST)) {
        status = STATUS_INVALID_ACCOUNT_NAME;
        goto error;

    } else if (status != STATUS_SUCCESS) {
        goto error;
    }

    status = ResetAccountPasswordTimer(samr_b, account_h);
    goto_if_ntstatus_not_success(status, error);

cleanup:
    if (info) {
        SamrFreeMemory((void*)info);
    }

    return status;

error:
    goto cleanup;
}


NTSTATUS
CreateWksAccount(
    NetConn *conn,
    wchar16_t *name,
    PolicyHandle *account_h
    )
{
    const uint32 user_access = USER_ACCESS_GET_ATTRIBUTES |
                               USER_ACCESS_SET_PASSWORD |
                               USER_ACCESS_SET_ATTRIBUTES;
    NTSTATUS status = STATUS_SUCCESS;
    NTSTATUS ret = STATUS_SUCCESS;
    WINERR err = ERROR_SUCCESS;
    handle_t samr_b = NULL;
    uint32 access_granted = 0;
    uint32 rid = 0;
    wchar16_t *account_name = NULL;
    size_t account_name_cch = 0;
    PolicyHandle *domain_h = NULL;
    PwInfo pwinfo;
    UserInfo *info = NULL;

    memset((void*)&pwinfo, 0, sizeof(pwinfo));

    goto_if_invalid_param_ntstatus(conn, cleanup);
    goto_if_invalid_param_ntstatus(name, cleanup);
    goto_if_invalid_param_ntstatus(account_h, cleanup);

    samr_b   = conn->samr.bind;
    domain_h = &conn->samr.dom_handle;

    account_name_cch = wc16slen(name) + 2;
    status = NetAllocateMemory((void**)&account_name,
                               sizeof(wchar16_t) * account_name_cch,
                               NULL);
    goto_if_ntstatus_not_success(status, error);

    if (sw16printfw(
                account_name,
                account_name_cch,
                L"%ws$",
                name) < 0)
    {
        status = ErrnoToNtStatus(errno);
        goto_if_ntstatus_not_success(status, error);
    }

    status = SamrCreateUser2(samr_b, domain_h, account_name, ACB_WSTRUST,
                             user_access, account_h, &access_granted, &rid);
    goto_if_ntstatus_not_success(status, error);

    status = SamrQueryUserInfo(samr_b, account_h, 16, &info);
    if (status == STATUS_SUCCESS &&
        !(info->info16.account_flags & ACB_WSTRUST)) {
        status = STATUS_INVALID_ACCOUNT_NAME;
    }

    goto_if_ntstatus_not_success(status, error);

    /* It's not certain yet what is this call for here.
       Access denied is not fatal here, so we don't want to report
       a fatal error */
    ret = SamrGetUserPwInfo(samr_b, account_h, &pwinfo);
    if (ret != STATUS_SUCCESS &&
        ret != STATUS_ACCESS_DENIED) {
        status = ret;
    }

cleanup:
    if (info) {
        SamrFreeMemory((void*)info);
    }

    if (account_name) {
        NetFreeMemory((void*)account_name);
    }

    return status;

error:
    goto cleanup;
}


NTSTATUS
SetMachinePassword(
    NetConn *conn,
    PolicyHandle *account_h,
    uint32 new,
    wchar16_t *name,
    wchar16_t *password
    )
{	
	NTSTATUS status = STATUS_SUCCESS;
    WINERR err = ERROR_SUCCESS;
	handle_t samr_b = NULL;
	uint32 level = 0;
    uint32 password_len = 0;
	UserInfo25 *info25 = NULL;
	UserInfo26 *info26 = NULL;
	UserInfo pwinfo;
    UnicodeString *full_name = NULL;

    memset((void*)&pwinfo, 0, sizeof(pwinfo));

    goto_if_invalid_param_ntstatus(conn, cleanup);
    goto_if_invalid_param_ntstatus(account_h, cleanup);
    goto_if_invalid_param_ntstatus(name, cleanup);
    goto_if_invalid_param_ntstatus(password, cleanup);

	samr_b = conn->samr.bind;
	password_len = wc16slen(password);

	if (new) {
		/* set account password */
		info25 = &pwinfo.info25;
		status = EncPasswordEx(info25->password.data, password,
                               password_len, conn);
        goto_if_ntstatus_not_success(status, error);

        full_name = &info25->info.full_name;

		/* this clears ACB_DISABLED flag and thus makes the account
		   active with password timeout reset */
		info25->info.account_flags = ACB_WSTRUST;
		status = InitUnicodeString(full_name, name);
        goto_if_ntstatus_not_success(status, error);

		info25->info.fields_present = SAMR_FIELD_FULL_NAME |
                                      SAMR_FIELD_ACCT_FLAGS |
                                      SAMR_FIELD_PASSWORD;
		level = 25;

	} else {
		/* set account password */
		info26 = &pwinfo.info26;
		status = EncPasswordEx(info26->password.data, password,
                               password_len, conn);
        goto_if_ntstatus_not_success(status, error);

		level = 26;
	}

	status = SamrSetUserInfo(samr_b, account_h, level, &pwinfo);
    goto_if_ntstatus_not_success(status, error);

cleanup:
    if (full_name) {
        FreeUnicodeString(full_name);
    }

    return status;

error:
    goto cleanup;
}


NET_API_STATUS
DirectoryConnect(
    const wchar16_t *domain,
    LDAP **ldconn,
    wchar16_t **dn_context
    )
{
    WINERR err = ERROR_SUCCESS;
    int lderr = LDAP_SUCCESS;
    int close_lderr = LDAP_SUCCESS;
    LDAP *ld = NULL;
    LDAPMessage *info = NULL;
    LDAPMessage *res = NULL;
    wchar16_t *dn_context_name = NULL;
    wchar16_t **dn_context_val = NULL;

    goto_if_invalid_param_winerr(domain, cleanup);
    goto_if_invalid_param_winerr(ldconn, cleanup);
    goto_if_invalid_param_winerr(dn_context, cleanup);

    *ldconn     = NULL;
    *dn_context = NULL;

    lderr = LdapInitConnection(&ld, domain, ISC_REQ_INTEGRITY);
    goto_if_lderr_not_success(lderr, error);

    lderr = LdapGetDirectoryInfo(&info, &res, ld);
    goto_if_lderr_not_success(lderr, error);

    dn_context_name = ambstowc16s("defaultNamingContext");
    goto_if_no_memory_lderr(dn_context_name, error);

    dn_context_val = LdapAttributeGet(ld, info, dn_context_name, NULL);
    if (dn_context_val == NULL) {
        /* TODO: find more descriptive error code */
        lderr = LDAP_NO_SUCH_ATTRIBUTE;
        goto error;

    }

    *dn_context = wc16sdup(dn_context_val[0]);
    goto_if_no_memory_lderr((*dn_context), error);

    *ldconn = ld;

cleanup:
    SAFE_FREE(dn_context_name);

    if (dn_context_val) {
        LdapAttributeValueFree(dn_context_val);
    }

    if (res) {
        LdapMessageFree(res);
    }

    return LdapErrToWin32Error(lderr);

error:
    if (ld) {
        close_lderr = LdapCloseConnection(ld);
        if (lderr == LDAP_SUCCESS &&
            close_lderr != STATUS_SUCCESS) {
            lderr = close_lderr;
        }
    }

    *dn_context = NULL;
    *ldconn     = NULL;
    goto cleanup;
}


NET_API_STATUS
DirectoryDisconnect(
    LDAP *ldconn
    )
{
    int lderr = LdapCloseConnection(ldconn);
    return LdapErrToWin32Error(lderr);
}


NET_API_STATUS
MachDnsNameSearch(
    LDAP *ldconn,
    const wchar16_t *name,
    const wchar16_t *dn_context,
    const wchar16_t *dns_domain_name,
    wchar16_t **samacct)
{
    WINERR err = ERROR_SUCCESS;
    int lderr = LDAP_SUCCESS;
    LDAPMessage *res = NULL;
    wchar16_t *samacct_attr_name = NULL;
    wchar16_t **samacct_attr_val = NULL;

    goto_if_invalid_param_winerr(ldconn, cleanup);
    goto_if_invalid_param_winerr(name, cleanup);
    goto_if_invalid_param_winerr(dn_context, cleanup);
    goto_if_invalid_param_winerr(dns_domain_name, cleanup);
    goto_if_invalid_param_winerr(samacct, cleanup);

    *samacct = NULL;

    lderr = LdapMachDnsNameSearch(
                &res,
                ldconn,
                name,
                dns_domain_name,
                dn_context);
    goto_if_lderr_not_success(lderr, error);

    samacct_attr_name = ambstowc16s("sAMAccountName");
    goto_if_no_memory_lderr(samacct_attr_name, error);

    samacct_attr_val = LdapAttributeGet(ldconn, res, samacct_attr_name, NULL);
    if (!samacct_attr_val) {
        lderr = LDAP_NO_SUCH_ATTRIBUTE;
        goto error;
    }

    *samacct = wc16sdup(samacct_attr_val[0]);
    goto_if_no_memory_lderr((*samacct), error);

cleanup:

    SAFE_FREE(samacct_attr_name);
    LdapAttributeValueFree(samacct_attr_val);

    if (res)
    {
        LdapMessageFree(res);
    }

    return LdapErrToWin32Error(lderr);

error:

    *samacct = NULL;
    goto cleanup;
}


NET_API_STATUS
MachAcctSearch(
    LDAP *ldconn,
    const wchar16_t *name,
    const wchar16_t *dn_context,
    wchar16_t **dn
    )
{
    WINERR err = ERROR_SUCCESS;
    int lderr = LDAP_SUCCESS;
    LDAPMessage *res = NULL;
    wchar16_t *dn_attr_name = NULL;
    wchar16_t **dn_attr_val = NULL;

    goto_if_invalid_param_winerr(ldconn, cleanup);
    goto_if_invalid_param_winerr(name, cleanup);
    goto_if_invalid_param_winerr(dn_context, cleanup);
    goto_if_invalid_param_winerr(dn, cleanup);

    *dn = NULL;

    lderr = LdapMachAcctSearch(&res, ldconn, name, dn_context);
    goto_if_lderr_not_success(lderr, error);

    dn_attr_name = ambstowc16s("distinguishedName");
    goto_if_no_memory_lderr(dn_attr_name, error);

    dn_attr_val = LdapAttributeGet(ldconn, res, dn_attr_name, NULL);
    if (!dn_attr_val) {
        lderr = LDAP_NO_SUCH_ATTRIBUTE;
        goto error;
    }
    
    *dn = wc16sdup(dn_attr_val[0]);
    goto_if_no_memory_lderr((*dn), error);

cleanup:
    SAFE_FREE(dn_attr_name);
    LdapAttributeValueFree(dn_attr_val);

    if (res) {
        LdapMessageFree(res);
    }

    return LdapErrToWin32Error(lderr);

error:
    *dn = NULL;
    goto cleanup;
}


NET_API_STATUS
MachAcctCreate(
    LDAP *ld,
    const wchar16_t *machine,
    const wchar16_t *ou,
    int rejoin
    )
{
    WINERR err = ERROR_SUCCESS;
    int lderr = LDAP_SUCCESS;
    LDAPMessage *machacct = NULL;
    LDAPMessage *res = NULL;
    LDAPMessage *info = NULL;
    wchar16_t *dn_context_name = NULL;
    wchar16_t **dn_context_val = NULL;
    wchar16_t *dn_name = NULL;
    wchar16_t **dn_val = NULL;

    goto_if_invalid_param_winerr(ld, cleanup);
    goto_if_invalid_param_winerr(machine, cleanup);
    goto_if_invalid_param_winerr(ou, cleanup);

    lderr = LdapMachAcctCreate(ld, machine, ou);
    if (lderr == LDAP_ALREADY_EXISTS && rejoin) {
        lderr = LdapGetDirectoryInfo(&info, &res, ld);
        goto_if_lderr_not_success(lderr, error);

        dn_context_name = ambstowc16s("defaultNamingContext");
        goto_if_no_memory_lderr(dn_context_name, error);

        dn_context_val = LdapAttributeGet(ld, info, dn_context_name, NULL);
        if (dn_context_val == NULL) {
            /* TODO: find more descriptive error code */
            lderr = LDAP_NO_SUCH_ATTRIBUTE;
            goto error;
        }

        lderr = LdapMachAcctSearch(&machacct, ld, machine, dn_context_val[0]);
        goto_if_lderr_not_success(lderr, error);

        dn_name = ambstowc16s("distinguishedName");
        goto_if_no_memory_lderr(dn_name, error);

        dn_val = LdapAttributeGet(ld, machacct, dn_name, NULL);
        if (dn_val == NULL) {
            /* TODO: find more descriptive error code */
            lderr = LDAP_NO_SUCH_ATTRIBUTE;
            goto error;
        }

        lderr = LdapMachAcctMove(ld, dn_val[0], machine, ou);
        goto_if_lderr_not_success(lderr, error);
    }

cleanup:
    SAFE_FREE(dn_context_name);
    SAFE_FREE(dn_name);

    if (dn_context_val) {
        LdapAttributeValueFree(dn_context_val);
    }

    if (dn_val) {
        LdapAttributeValueFree(dn_val);
    }

    return LdapErrToWin32Error(lderr);

error:
    goto cleanup;
}


NET_API_STATUS
MachAcctSetAttribute(
    LDAP *ldconn,
    const wchar16_t *dn,
    const wchar16_t *attr_name,
    const wchar16_t **attr_val,
    int new
    )
{
    int lderr = LDAP_SUCCESS;

    lderr = LdapMachAcctSetAttribute(ldconn, dn, attr_name, attr_val, new);
    return LdapErrToWin32Error(lderr);
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
