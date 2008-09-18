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

#include <string.h>
#include <gssapi/gssapi.h>

#include "includes.h"
#include "NetUtil.h"


NTSTATUS ResetAccountPasswordTimer(handle_t samr_b, PolicyHandle *account_h)
{
    const uint32 flags_enable  = ACB_WSTRUST;
    const uint32 flags_disable = ACB_WSTRUST | ACB_DISABLED;
    const uint32 level = 16;

    NTSTATUS status;
    UserInfo info = {0};
    UserInfo16 *info16;

    info16 = &info.info16;

    /* flip ACB_DISABLED flag - this way password timeout counter
       gets restarted */

    info16->account_flags = flags_enable;
    status = SamrSetUserInfo(samr_b, account_h, level, &info);
    if (status != STATUS_SUCCESS) return status;

    info16->account_flags = flags_disable;
    status = SamrSetUserInfo(samr_b, account_h, level, &info);
    if (status != STATUS_SUCCESS) return status;

    info16->account_flags = flags_enable;
    status = SamrSetUserInfo(samr_b, account_h, level, &info);
    if (status != STATUS_SUCCESS) return status;

    return status;
}


NTSTATUS ResetWksAccount(NetConn *conn, wchar16_t *name,
                         PolicyHandle *account_h)
{
    const uint32 user_access = USER_ACCESS_GET_ATTRIBUTES |
                               USER_ACCESS_SET_ATTRIBUTES |
                               USER_ACCESS_SET_PASSWORD;
    NTSTATUS status;
    handle_t samr_b;
    uint32 num_domains, rid;
    wchar16_t *names[1];
    UserInfo *info = NULL;

    samr_b   = conn->samr.bind;

    status = SamrQueryUserInfo(samr_b, account_h, 16, &info);
    if (status == STATUS_SUCCESS &&
        !(info->info16.account_flags & ACB_WSTRUST)) {
        status = STATUS_INVALID_ACCOUNT_NAME;
        goto done;

    } else if (status != STATUS_SUCCESS) {
        goto done;
    }

    status = ResetAccountPasswordTimer(samr_b, account_h);
    if (status != STATUS_SUCCESS) goto done;

done:
    if (info) SamrFreeMemory((void*)info);

    return status;
}


NTSTATUS CreateWksAccount(NetConn *conn, wchar16_t *name,
                          PolicyHandle *account_h)
{
    const uint32 user_access = USER_ACCESS_GET_ATTRIBUTES |
                               USER_ACCESS_SET_PASSWORD |
                               USER_ACCESS_SET_ATTRIBUTES;
    NTSTATUS status, ret;
    handle_t samr_b;
    uint32 access_granted, rid;
    wchar16_t *sysname, **domains, *account_name;
    PolicyHandle *domain_h;
    DomSid *dom_sid = NULL;
    PwInfo pwinfo;
    UserInfo *info = NULL;

    samr_b   = conn->samr.bind;
    domain_h = &conn->samr.dom_handle;

    account_name = (wchar16_t*) malloc(sizeof(wchar16_t) *
                                       (wc16slen(name) + 2));
    if (account_name == NULL) return STATUS_NO_MEMORY;

    sw16printf(account_name, "%S$", name);

    status = SamrCreateUser2(samr_b, domain_h, account_name, ACB_WSTRUST,
                             user_access, account_h, &access_granted, &rid);
    if (status != STATUS_SUCCESS) goto done;

    status = SamrQueryUserInfo(samr_b, account_h, 16, &info);
    if (status == STATUS_SUCCESS &&
        !(info->info16.account_flags & ACB_WSTRUST)) {
        status = STATUS_INVALID_ACCOUNT_NAME;
        goto done;

    } else if (status != STATUS_SUCCESS) {
        goto done;
    }


    /* It's not certain yet what is this call for here.
       Access denied is not fatal here, so we don't want to report
       a fatal error */
    ret = SamrGetUserPwInfo(samr_b, account_h, &pwinfo);
    if (ret != STATUS_SUCCESS &&
        ret != STATUS_ACCESS_DENIED) {
        status = ret;
        goto done;
    }

done:
    if (info) SamrFreeMemory((void*)info);
    SAFE_FREE(account_name);

    return status;
}


NTSTATUS SetMachinePassword(NetConn *conn, PolicyHandle *account_h,
                            uint32 new, wchar16_t *name, wchar16_t *password)
{	
	NTSTATUS status;
	handle_t samr_b;
	uint32 level, password_len;
	UserInfo25 *info25;
	UserInfo26 *info26;
	UserInfo pwinfo = {0};
    UnicodeString *full_name = NULL;

	samr_b = conn->samr.bind;
	password_len = wc16slen(password);

	if (new) {
		/* set account password */
		info25 = &pwinfo.info25;
		status = EncPasswordEx(info25->password.data, password,
                               password_len, conn);
		if (status != STATUS_SUCCESS) goto cleanup;

        full_name = &info25->info.full_name;

		/* this clears ACB_DISABLED flag and thus makes the account
		   active with password timeout reset */
		info25->info.account_flags = ACB_WSTRUST;
		status = InitUnicodeString(full_name, name);
        if (status != STATUS_SUCCESS) goto cleanup;

		info25->info.fields_present = SAMR_FIELD_FULL_NAME |
                                      SAMR_FIELD_ACCT_FLAGS |
                                      SAMR_FIELD_PASSWORD;
		level = 25;

	} else {
		/* set account password */
		info26 = &pwinfo.info26;
		status = EncPasswordEx(info26->password.data, password,
                               password_len, conn);
		if (status != STATUS_SUCCESS) goto cleanup;
		level = 26;
	}

	status = SamrSetUserInfo(samr_b, account_h, level, &pwinfo);

cleanup:
    if (full_name) {
        FreeUnicodeString(full_name);
    }

    return status;
}


NET_API_STATUS DirectoryConnect(const wchar16_t *domain,
                                LDAP **ldconn, wchar16_t **dn_context)
{
    int lderr = LDAP_SUCCESS;
    int close_lderr = LDAP_SUCCESS;
    LDAP *ld = NULL;
    LDAPMessage *info = NULL;
    LDAPMessage *res = NULL;
    wchar16_t *dn_context_name = NULL;
    wchar16_t **dn_context_val = NULL;

    if (!domain || !ldconn || !dn_context) return ERROR_INVALID_PARAMETER;

    *ldconn     = NULL;
    *dn_context = NULL;

    lderr = LdapInitConnection(&ld, domain, ISC_REQ_INTEGRITY);
    if (lderr != LDAP_SUCCESS) goto done;

    lderr = LdapGetDirectoryInfo(&info, &res, ld);
    if (lderr != LDAP_SUCCESS) goto disconn;

    dn_context_name = ambstowc16s("defaultNamingContext");
    goto_if_no_memory_lderr(dn_context_name, disconn);

    dn_context_val = LdapAttributeGet(ld, info, dn_context_name, NULL);
    if (dn_context_val == NULL) {
        /* TODO: find more descriptive error code */
        lderr = LDAP_NO_SUCH_ATTRIBUTE;
        goto disconn;

    } else {
        *dn_context = wc16sdup(dn_context_val[0]);
        *ldconn = ld;
        goto done;
    }

disconn:
    close_lderr = LdapCloseConnection(ld);
    if (lderr == LDAP_SUCCESS &&
        close_lderr != STATUS_SUCCESS) {
        lderr = close_lderr;
    }

done:
    SAFE_FREE(dn_context_name);
    LdapAttributeValueFree(dn_context_val);

    return LdapErrToWin32Error(lderr);
}


NET_API_STATUS DirectoryDisconnect(LDAP *ldconn)
{
    int lderr = LdapCloseConnection(ldconn);
    return LdapErrToWin32Error(lderr);
}


NET_API_STATUS MachAcctSearch(LDAP *ldconn, const wchar16_t *name,
                              const wchar16_t *dn_context,
                              wchar16_t **dn)
{
    int lderr = LDAP_SUCCESS;
    LDAPMessage *res = NULL;
    wchar16_t *dn_attr_name = NULL;
    wchar16_t **dn_attr_val = NULL;

    if (!ldconn || !name || !dn_context || !dn) {
        return ERROR_INVALID_PARAMETER;
    }

    *dn = NULL;

    lderr = LdapMachAcctSearch(&res, ldconn, name, dn_context);
    if (lderr != LDAP_SUCCESS) goto done;

    dn_attr_name = ambstowc16s("distinguishedName");
    goto_if_no_memory_lderr(dn_attr_name, done);

    dn_attr_val = LdapAttributeGet(ldconn, res, dn_attr_name, NULL);
    if (!dn_attr_val) {
        lderr = LDAP_NO_SUCH_ATTRIBUTE;
        goto done;
    }
    
    *dn = wc16sdup(dn_attr_val[0]);

done: 
    SAFE_FREE(dn_attr_name);
    LdapAttributeValueFree(dn_attr_val);

    return LdapErrToWin32Error(lderr);
}


NET_API_STATUS MachAcctCreate(LDAP *ld,
                              const wchar16_t *machine,
                              const wchar16_t *ou,
                              int rejoin)
{
    int lderr = LDAP_SUCCESS;
    LDAPMessage *machacct = NULL;
    LDAPMessage *res = NULL;
    LDAPMessage *info = NULL;
    wchar16_t *dn_context_name = NULL;
    wchar16_t **dn_context_val = NULL;
    wchar16_t *dn_name = NULL;
    wchar16_t **dn_val = NULL;

    lderr = LdapMachAcctCreate(ld, machine, ou);
    if (lderr == LDAP_ALREADY_EXISTS && rejoin) {
        lderr = LdapGetDirectoryInfo(&info, &res, ld);
        if (lderr != LDAP_SUCCESS) goto done;

        dn_context_name = ambstowc16s("defaultNamingContext");
        goto_if_no_memory_lderr(dn_context_name, done);

        dn_context_val = LdapAttributeGet(ld, info, dn_context_name, NULL);
        if (dn_context_val == NULL) {
            /* TODO: find more descriptive error code */
            lderr = LDAP_NO_SUCH_ATTRIBUTE;
            goto done;
        }

        lderr = LdapMachAcctSearch(&machacct, ld, machine, dn_context_val[0]);
        if (lderr != LDAP_SUCCESS) goto done;

        dn_name = ambstowc16s("distinguishedName");
        goto_if_no_memory_lderr(dn_name, done);

        dn_val = LdapAttributeGet(ld, machacct, dn_name, NULL);
        if (dn_val == NULL) {
            /* TODO: find more descriptive error code */
            lderr = LDAP_NO_SUCH_ATTRIBUTE;
            goto done;
        }

        lderr = LdapMachAcctMove(ld, dn_val[0], machine, ou);
        if (lderr != LDAP_SUCCESS) goto done;

    }

done:
    SAFE_FREE(dn_context_name);
    SAFE_FREE(dn_name);
    LdapAttributeValueFree(dn_context_val);
    LdapAttributeValueFree(dn_val);

    return LdapErrToWin32Error(lderr);
}


NET_API_STATUS MachAcctSetAttribute(LDAP *ldconn, const wchar16_t *dn,
                                    const wchar16_t *attr_name,
                                    const wchar16_t *attr_val[],
                                    int new)
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
