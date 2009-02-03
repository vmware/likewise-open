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
#include <errno.h>
#include <ldap.h>
#include <gssapi/gssapi.h>

#include <config.h>
#include <lwrpc/types.h>
#include <wc16str.h>
#include <lw/ntstatus.h>
#include <lwrpc/winerror.h>
#include <lwrpc/allocate.h>
#include <lwrpc/LM.h>

#include "NetUtil.h"


static int LdapModSetStrValue(LDAPMod **mod,
                              const char *t, const wchar16_t *wsv, int chg)
{
    int lderr = LDAP_SUCCESS;
    LDAPMod *m = NULL;
    int count = 0;
    char *sv = NULL;

    if (!mod || !t || !wsv) return LDAP_PARAM_ERROR;

    sv = awc16stombs(wsv);
    goto_if_no_memory_lderr(sv, done);

    m = *mod;
    if (!m) {
        m = (LDAPMod*) malloc(sizeof(LDAPMod));
        goto_if_no_memory_lderr(m, done);
        
        m->mod_type   = NULL;
        m->mod_values = NULL;
        m->mod_op     = chg;
        *mod = m;
    }

    if (m->mod_op != chg) {
        lderr = LDAP_PARAM_ERROR;
        goto done;

    } else {
        m->mod_op = chg;
    }

    if (m->mod_type) {
        if (strcmp(m->mod_type, t)) {
            lderr = LDAP_PARAM_ERROR;
            goto done;
        }

    } else {
        m->mod_type = strdup(t);
    }

    if (m->mod_values) {
        while (m->mod_values[++count]);
    }

    count += 2;
    m->mod_values = (char**) realloc(m->mod_values, count * sizeof(char*));
    goto_if_no_memory_lderr(m->mod_values, done);

    m->mod_values[count - 2] = strdup(sv);
    m->mod_values[count - 1] = NULL;

done:
    SAFE_FREE(sv);

    return lderr;
}


int LdapModAddStrValue(LDAPMod **mod, const char *t, const wchar16_t *sv)
{
    return LdapModSetStrValue(mod, t, sv, LDAP_MOD_ADD);
}


int LdapModReplStrValue(LDAPMod **mod, const char *t, const wchar16_t *sv)
{
    return LdapModSetStrValue(mod, t, sv, LDAP_MOD_REPLACE);
}


int LdapModAddIntValue(LDAPMod **mod, const char *t, const int iv)
{
    wchar16_t sv[11] = {0};
    sw16printf(sv, "%u", iv);

    return LdapModAddStrValue(mod, t, (const wchar16_t*)sv);
}


int LdapModFree(LDAPMod **mod)
{
    LDAPMod *m;
    int i = 0;

    if (!mod) return LDAP_PARAM_ERROR;

    m = *mod;

    if (m) {
        SAFE_FREE(m->mod_type);
        while (m->mod_values[i++]) {
            SAFE_FREE(m->mod_values[i]);
        }

        SAFE_FREE(m->mod_values);
        SAFE_FREE(m);
    }

    return LDAP_SUCCESS;
}


extern int errno;

int LdapInitConnection(LDAP **ldconn, const wchar16_t *host,
                       uint32 security)
{
    int lderr = LDAP_SUCCESS;
    LDAP *ld = NULL;
    unsigned int version;
    char* ldap_srv = NULL;

    if (!ldconn || !host) return LDAP_PARAM_ERROR;

    ldap_srv = awc16stombs(host);
    goto_if_no_memory_lderr(ldap_srv, done);

    ld = (LDAP*) ldap_open(ldap_srv, LDAP_PORT);
    if (!ld) {
        lderr = errno;
        goto done;
    }

    *ldconn = ld;

    version = LDAP_VERSION3;
    lderr = ldap_set_option(ld, LDAP_OPT_PROTOCOL_VERSION, &version);
    if (lderr != LDAP_SUCCESS) goto done;

    lderr = ldap_set_option(ld, LDAP_OPT_REFERRALS, LDAP_OPT_OFF);
    if (lderr != LDAP_SUCCESS) goto done;

    security |= ISC_REQ_MUTUAL_AUTH | ISC_REQ_REPLAY_DETECT;

    lderr = ldap_set_option(ld, LDAP_OPT_SSPI_FLAGS, &security);
    if (lderr != LDAP_SUCCESS) goto done;

    lderr = ldap_set_option(ld, LDAP_OPT_X_GSSAPI_ALLOW_REMOTE_PRINCIPAL,
                            LDAP_OPT_ON);
    if (lderr != LDAP_SUCCESS) goto done;

    lderr = ldap_bind_s(ld, NULL, NULL, LDAP_AUTH_NEGOTIATE);
    if (lderr != LDAP_SUCCESS) goto done;

done:
    return lderr;
}


int LdapCloseConnection(LDAP *ldconn)
{
    int lderr = LDAP_SUCCESS;

    lderr = ldap_unbind_s(ldconn);

    return lderr;
}


int LdapGetDirectoryInfo(LDAPMessage **info, LDAPMessage **result, LDAP *ld)
{
    const char *basedn = "";
    const int scope = LDAP_SCOPE_BASE;
    const char *filter = "(objectClass=*)";
    char *allattr[] = { NULL };
    int lderr = LDAP_SUCCESS;
    LDAPMessage *res = NULL, *entry = NULL;
    int count;
    char *attr;
    BerElement *be;

    if (!ld || !info || !result) return LDAP_PARAM_ERROR;

    lderr = ldap_search_ext_s(ld, basedn, scope, filter, allattr, 0,
                              NULL, NULL, NULL, 0, &res);
    if (lderr != LDAP_SUCCESS) goto done;

    entry = ldap_first_entry(ld, res);
    if (!entry) goto done;

    *info   = entry;
    *result = res;

done:
    return lderr;
}


wchar16_t **LdapAttributeGet(LDAP *ld, LDAPMessage *info, const wchar16_t *name,
                             int *count)
{
    int lderr = LDAP_SUCCESS;
    int i, vcount;
    wchar16_t *attr_w16;
    wchar16_t **out = NULL;
    char *attr = NULL;
    BerElement *be = NULL;
    char **value = NULL;

    if (!info || !name) return NULL;

    attr = ldap_first_attribute(ld, info, &be);

    while (attr) {

        attr_w16 = ambstowc16s(attr);
        goto_if_no_memory_lderr(attr_w16, fail);

        if (wc16scmp(attr_w16, name) == 0) {
            value = (char**) ldap_get_values(ld, info, attr);
            vcount = ldap_count_values(value);

            out = (wchar16_t**) realloc(out, sizeof(wchar16_t*) * (vcount+1));
            goto_if_no_memory_lderr(out, fail);

            for (i = 0; i < vcount; i++) {
                out[i] = ambstowc16s(value[i]);
                goto_if_no_memory_lderr(out[i], fail);
            }

            if (count) *count = vcount;

            ldap_value_free(value);
            ber_free(be, 0);
            goto done;
        }

        ldap_memfree(attr);
        attr = ldap_next_attribute(ld, info, be);
    }

fail:
    out = NULL;

done:
    SAFE_FREE(attr_w16);
    ldap_memfree(attr);

    out[vcount] = NULL;
    return out;
}


void LdapAttributeValueFree(wchar16_t *val[])
{
    int i = 0;

    if (val == NULL) return;

    for (i = 0; val[i]; i++) SAFE_FREE(val[i]);
    SAFE_FREE(val);
}


wchar16_t* LdapAttrValDn(const wchar16_t *name, const wchar16_t *value,
                         const wchar16_t *base)
{
    size_t name_len, value_len, base_len, dn_len;
    wchar16_t *dn = NULL;

    if (!name || !value) return NULL;

    name_len  = wc16slen(name);
    value_len = wc16slen(value);
    base_len  = (base) ? wc16slen(base) : 0;

    dn_len = base_len + value_len + name_len + 2;
    dn = (wchar16_t*) malloc(sizeof(wchar16_t) * (dn_len + 1));
    if (dn == NULL) return NULL;

    if (base) {
        sw16printf(dn, "%S=%S,%S", name, value, base);
    } else {
        sw16printf(dn, "%S=%S", name, value);
    }

    return dn;
}


wchar16_t* LdapAttrValSamAcctName(const wchar16_t *name)
{
    size_t name_len, samacct_len;
    wchar16_t *samacct = NULL;

    if (!name) return NULL;

    name_len = wc16slen(name);

    samacct_len = name_len + 1;
    samacct = (wchar16_t*) malloc(sizeof(wchar16_t) * (samacct_len + 1));
    if (samacct == NULL) return NULL;

    sw16printf(samacct, "%S$", name);

    return samacct;
}


wchar16_t* LdapAttrValDnsHostName(const wchar16_t *name, const wchar16_t *dnsdomain)
{
    size_t name_len, dnsdomain_len, dnsname_len;
    wchar16_t *dnsname = NULL;

    if (!name) return NULL;

    name_len      = wc16slen(name);
    dnsdomain_len = (dnsdomain) ? wc16slen(dnsdomain) : 0;

    dnsname_len = name_len + dnsdomain_len + 1;
    dnsname = (wchar16_t*) malloc(sizeof(wchar16_t) * (dnsname_len + 1));
    if (dnsname == NULL) return NULL;

    if (dnsdomain) {
        sw16printf(dnsname, "%S.%S", name, dnsdomain);
    } else {
        sw16printf(dnsname, "%S", name);
    }

    return dnsname;
}


wchar16_t *LdapAttrValSvcPrincipalName(const wchar16_t *name)
{
    size_t name_len, svcpn_len;
    wchar16_t *svcpn = NULL;

    if (!name) return NULL;

    name_len = wc16slen(name);

    svcpn_len = name_len + 5;
    svcpn = (wchar16_t*) malloc(sizeof(wchar16_t) * (svcpn_len + 1));
    if (svcpn == NULL) return NULL;

    sw16printf(svcpn, "HOST/%S", name);

    return svcpn;
}


int LdapMachAcctCreate(LDAP *ld, const wchar16_t *name, const wchar16_t *ou)
{
    int lderr = LDAP_SUCCESS;
    size_t account_ou_len;
    size_t dn_size, samacct_size;
    wchar16_t *cn_name;
    wchar16_t *machname_lc = NULL;
    wchar16_t *dname = NULL;
    char *dn = NULL;
    wchar16_t *samacct = NULL;
    wchar16_t *objclass[5] = {0};
    int flags, i;
    LDAPMod *name_m = NULL;
    LDAPMod *samacct_m = NULL;
    LDAPMod *objectclass_m = NULL;
    LDAPMod *acctflags_m = NULL;
    LDAPMod *attrs[5];

    if (!ld || !name || !ou) return LDAP_PARAM_ERROR;

    machname_lc = wc16sdup(name);
    wc16slower(machname_lc);

    cn_name = ambstowc16s("cn");
    dname = LdapAttrValDn(cn_name, machname_lc, ou);
    goto_if_no_memory_lderr(dname, done);

    dn = awc16stombs(dname);
    goto_if_no_memory_lderr(dn, done);

    samacct = LdapAttrValSamAcctName(name);
    goto_if_no_memory_lderr(samacct, done);

    objclass[0] = ambstowc16s("top");
    objclass[1] = ambstowc16s("organizationalPerson");
    objclass[2] = ambstowc16s("user");
    objclass[3] = ambstowc16s("computer");
    objclass[4] = NULL;

    flags = UF_ACCOUNTDISABLE | UF_WORKSTATION_TRUST_ACCOUNT;

    LdapModAddStrValue(&name_m, "name", machname_lc);
    LdapModAddStrValue(&samacct_m, "sAMAccountName", samacct);
    LdapModAddStrValue(&objectclass_m, "objectClass", objclass[0]);
    LdapModAddStrValue(&objectclass_m, "objectClass", objclass[1]);
    LdapModAddStrValue(&objectclass_m, "objectClass", objclass[2]);
    LdapModAddStrValue(&objectclass_m, "objectClass", objclass[3]);
    LdapModAddIntValue(&acctflags_m, "userAccountControl", flags);

    attrs[0] = name_m;
    attrs[1] = samacct_m;
    attrs[2] = objectclass_m;
    attrs[3] = acctflags_m;
    attrs[4] = NULL;

    lderr = ldap_add_ext_s(ld, dn, attrs, NULL, NULL);
    if (lderr != LDAP_SUCCESS) goto done;

done:
    LdapModFree(&name_m);
    LdapModFree(&samacct_m);
    LdapModFree(&objectclass_m);
    LdapModFree(&acctflags_m);

    SAFE_FREE(machname_lc);
    SAFE_FREE(dname);
    SAFE_FREE(dn);
    SAFE_FREE(samacct);
    SAFE_FREE(cn_name);
    for (i = 0; objclass[i]; i++) SAFE_FREE(objclass[i]);

    return lderr;
}


int LdapMachAcctSearch(LDAPMessage **out, LDAP *ld, const wchar16_t *name,
                       const wchar16_t *base)
{
    const char *filter_fmt = "(&(objectClass=computer)(sAMAccountName=%S))";

    int lderr = LDAP_SUCCESS;
    size_t machname_len, basedn_len, filter_len;
    size_t samacct_len;
    wchar16_t *samacct = NULL;
    char *basedn = NULL;
    wchar16_t *filterw16 = NULL;
    char *filter = NULL;
    LDAPMessage *res = NULL;
    LDAPControl **sctrl = NULL, **cctrl = NULL;

    if (!out || !ld || !name || !base) return LDAP_PARAM_ERROR;

    basedn = awc16stombs(base);
    goto_if_no_memory_lderr(basedn, done);
    basedn_len = strlen(basedn);

    samacct = LdapAttrValSamAcctName(name);
    goto_if_no_memory_lderr(samacct, done);
    samacct_len = wc16slen(samacct);

    filter_len = samacct_len + strlen(filter_fmt);
    filterw16 = (wchar16_t*) malloc(sizeof(wchar16_t) * filter_len);
    goto_if_no_memory_lderr(filterw16, done);

    sw16printf(filterw16, filter_fmt, samacct);

    filter = awc16stombs(filterw16);
    goto_if_no_memory_lderr(filter, done);

    lderr = ldap_search_ext_s(ld, basedn, LDAP_SCOPE_SUBTREE, filter, NULL, 0,
                              sctrl, cctrl, NULL, 0, &res);
    if (lderr != LDAP_SUCCESS) goto done;

done:
    SAFE_FREE(filter);
    SAFE_FREE(filterw16);
    SAFE_FREE(samacct);
    SAFE_FREE(basedn);

    *out = res;

    return lderr;
}


int LdapMachAcctMove(LDAP *ld, const wchar16_t *dn, const wchar16_t *name,
                     const wchar16_t *newparent)
{
    const char *cn_fmt = "cn=%s";

    int lderr = LDAP_SUCCESS;
    size_t newname_len;
    char *dname = NULL;
    char *machname = NULL;
    char *newname = NULL;
    char *newpar = NULL;
    LDAPControl **sctrl = NULL, **cctrl = NULL;

    dname = awc16stombs(dn);
    goto_if_no_memory_lderr(dname, done);

    machname = awc16stombs(name);
    goto_if_no_memory_lderr(machname, done);

    newname_len = wc16slen(name) + strlen(cn_fmt);
    newname = (char*) malloc(sizeof(char) * newname_len);
    goto_if_no_memory_lderr(newname, done);
    snprintf(newname, newname_len, cn_fmt, machname);

    newpar = awc16stombs(newparent);
    goto_if_no_memory_lderr(newpar, done);

    lderr = ldap_rename_s(ld, dname, newname, newpar, 1, sctrl, cctrl);

done:
    SAFE_FREE(newpar);
    SAFE_FREE(newname);
    SAFE_FREE(machname);
    SAFE_FREE(dname);

    return lderr;
}


int LdapMachAcctSetAttribute(LDAP *ld, const wchar16_t *dn,
                             const wchar16_t *name, const wchar16_t *value[],
                             int new)
{
    int lderr = LDAP_SUCCESS;
    LDAPMod *mod = NULL;
    LDAPMod *mods[2];
    LDAPControl **sctrl = NULL, **cctrl = NULL;
    char *dname = NULL;
    char *n = NULL;
    char *v = NULL;
    int i;

    dname = awc16stombs(dn);
    goto_if_no_memory_lderr(dname, done);

    n = awc16stombs(name);
    goto_if_no_memory_lderr(n, done);

    for (i = 0; value[i] != NULL; i++) {
        if (new) {
            lderr = LdapModAddStrValue(&mod, n, value[i]);
        } else {
            lderr = LdapModReplStrValue(&mod, n, value[i]);
        }

        if (lderr != LDAP_SUCCESS) goto done;
    }

    mods[0] = mod;
    mods[1] = NULL;

    lderr = ldap_modify_ext_s(ld, dname, mods, sctrl, cctrl);

done:
    SAFE_FREE(n);
    SAFE_FREE(dname);

    return lderr;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
