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
 * Abstract: NetAPI connection management functions (rpc client library)
 *
 * Authors: Rafal Szczesniak (rafal@likewisesoftware.com)
 */

#include "includes.h"

extern NetConn *first;


NetConn *FirstConn(NetConn *cn, int set)
{
    if (set) first = cn;
    return first;
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


NTSTATUS NetConnectSamr(NetConn **conn, const wchar16_t *hostname,
                        uint32 req_dom_flags, uint32 req_btin_dom_flags,
                        PIO_ACCESS_TOKEN access_token)
{
    const uint32 conn_flags = SAMR_ACCESS_OPEN_DOMAIN |
                              SAMR_ACCESS_ENUM_DOMAINS;

    const uint32 dom_flags = DOMAIN_ACCESS_ENUM_ACCOUNTS |
                             DOMAIN_ACCESS_OPEN_ACCOUNT |
                             DOMAIN_ACCESS_LOOKUP_INFO_2;
			     
    const uint32 btin_dom_flags = DOMAIN_ACCESS_ENUM_ACCOUNTS |
                                  DOMAIN_ACCESS_OPEN_ACCOUNT |
                                  DOMAIN_ACCESS_LOOKUP_INFO_2;
    const uint32 size = 128;
    const char *builtin = "BUILTIN";
    const char *localhost = "127.0.0.1";

    handle_t samr_b;
    NetConn *cn, *lookup;
    PolicyHandle conn_handle;
    PolicyHandle dom_handle;
    PolicyHandle btin_dom_handle;
    PSID btin_dom_sid = NULL;
    PSID dom_sid = NULL;
    uint32 conn_access, dom_access, btin_dom_access;
    uint32 resume, entries, i;
    wchar16_t **dom_names = NULL;
    wchar16_t *dom_name = NULL;
    wchar16_t localhost_addr[10];
    uint8 *sess_key;
    unsigned16 sess_key_len;
    NTSTATUS status = STATUS_SUCCESS;
    RPCSTATUS rpcstatus = 0;

    if (conn == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    samr_b = NULL;
    cn = NULL;

    /* check if requested connection is a localhost connection */
    if (hostname == NULL) {
        size_t addr_size = sizeof(localhost_addr)/sizeof(wchar16_t);
        memset(localhost_addr, 0, addr_size);
        mbstowc16s(localhost_addr, localhost, addr_size);
        hostname = (wchar16_t*)localhost_addr;
    }

    /* look for existing connection and check if it already has
       required access rights */
    FIND_CONN(lookup, hostname);
    if (lookup &&
        (req_dom_flags == 0 ||
         ((lookup->samr.dom_access & req_dom_flags) == req_dom_flags)) &&
        (req_btin_dom_flags == 0 ||
         ((lookup->samr.btin_dom_access & req_btin_dom_flags) == req_btin_dom_flags))) {

        *conn = lookup;
        return STATUS_SUCCESS;
    }

    if (lookup == NULL) {
        /* create a new connection */
        cn = (NetConn*) malloc(sizeof(NetConn));
        if (cn == NULL) return STATUS_NO_MEMORY;
        memset(cn, 0, sizeof(NetConn));

    } else {
        cn = lookup;
    }

    if (cn->samr.conn_access == 0 ||
        cn->samr.bind == NULL) {
        char *host = NULL;

        conn_access = conn_flags;

        host = awc16stombs(hostname);
        if (host == NULL) return STATUS_NO_MEMORY;

        rpcstatus = InitSamrBindingDefault(&samr_b, host, access_token);
        if (rpcstatus != 0) return STATUS_UNSUCCESSFUL;

        SAFE_FREE(host);

        status = SamrConnect2(samr_b, hostname, conn_access, &conn_handle);
        if (status != 0) return status;

        cn->samr.conn_access = conn_access;
        cn->samr.bind        = samr_b;
        cn->samr.conn_handle = conn_handle;

        sess_key     = NULL;
        sess_key_len = 0;

        GetSessionKey(samr_b, &sess_key, &sess_key_len, &rpcstatus);

        if (rpcstatus == 0)
        {
            memcpy((void*)cn->sess_key, sess_key, sizeof(cn->sess_key));
            cn->sess_key_len = (uint32)sess_key_len;
        }

    } else {
        samr_b      = cn->samr.bind;
        conn_handle = cn->samr.conn_handle;
    }

    /* check if requested builtin domain access flags have been
       specified and whether they match already opened handle's
       access rights */
    if (req_btin_dom_flags != 0 &&
        cn->samr.btin_dom_access != 0 &&
        (cn->samr.btin_dom_access & req_btin_dom_flags) != req_btin_dom_flags) {

        status = SamrClose(samr_b, &cn->samr.btin_dom_handle);
        if (status != 0) return status;

        memset(&cn->samr.btin_dom_handle, 0, sizeof(cn->samr.btin_dom_handle));
        cn->samr.btin_dom_access = 0;
    }

    if (cn->samr.btin_dom_access == 0) {
        btin_dom_access = btin_dom_flags | req_btin_dom_flags;
        conn_handle = cn->samr.conn_handle;

        status = RtlAllocateSidFromCString(&btin_dom_sid, SID_BUILTIN_DOMAIN);
        if (status != 0) return status;

        status = SamrOpenDomain(samr_b, &conn_handle, btin_dom_access,
                                btin_dom_sid, &btin_dom_handle);
        if (status != 0) return status;

        cn->samr.btin_dom_handle = btin_dom_handle;
        cn->samr.btin_dom_access = btin_dom_access;

        RTL_FREE(&btin_dom_sid);
    }

    /* check if requested host's domain access flags have been
       specified and whether they match already opened handle's
       access rights */
    if (req_dom_flags != 0 &&
        cn->samr.dom_access != 0 &&
        (cn->samr.dom_access & req_dom_flags) != req_dom_flags) {

        status = SamrClose(samr_b, &cn->samr.dom_handle);
        if (status != 0) return status;

        memset(&cn->samr.dom_handle, 0, sizeof(cn->samr.dom_handle));
        cn->samr.dom_access = 0;
        if (cn->samr.dom_sid) free(cn->samr.dom_sid);
        cn->samr.dom_sid = NULL;
    }

    if (cn->samr.dom_access == 0) {
        resume    = 0;
        entries   = 0;
        dom_names = NULL;

        do {
            status = SamrEnumDomains(samr_b, &conn_handle, &resume, size,
                                     &dom_names, &entries);
            if (status != STATUS_SUCCESS &&
                status != STATUS_MORE_ENTRIES) return status;

            for (i = 0; i < entries; i++) {
                char n[32]; /* any netbios name can fit here */

                wc16stombs(n, dom_names[i], sizeof(n));

                /* pick up first domain name that is not a builtin domain */
                if (strcasecmp(n, builtin)) {
                    dom_name = wc16sdup(dom_names[i]);

                    SamrFreeMemory((void*)dom_names);
                    goto domain_name_found;
                }
            }

            if (dom_names) SamrFreeMemory((void*)dom_names);
            dom_names = NULL;

        } while (status == STATUS_MORE_ENTRIES);
        
domain_name_found:
        status = SamrLookupDomain(samr_b, &conn_handle, dom_name, &dom_sid);
        if (status != 0) return status;

        dom_access = dom_flags | req_dom_flags;

        status = SamrOpenDomain(samr_b, &conn_handle, dom_access, dom_sid, &dom_handle);
        if (status != 0) return status;

        cn->samr.dom_handle = dom_handle;
        cn->samr.dom_access = dom_access;
        cn->samr.dom_name   = dom_name;

        if (dom_sid) {
            RtlSidCopyAlloc(&cn->samr.dom_sid, dom_sid);
            SamrFreeMemory((void*)dom_sid);
        }
    }

    /* set the host name if it's completely new connection */
    if (cn->hostname == NULL) {
        cn->hostname = wc16sdup(hostname);
    }

    /* add newly created connection */
    if (cn != lookup) ADD_CONN(cn);

    /* return initialised connection and status code */
    *conn = cn;
    return STATUS_SUCCESS;
}


NTSTATUS NetConnectLsa(NetConn **conn, const wchar16_t *hostname,
                       uint32 req_lsa_flags, PIO_ACCESS_TOKEN access_token)
{
    const uint32 lsa_flags = LSA_ACCESS_LOOKUP_NAMES_SIDS;
    const wchar_t *localhost = L"127.0.0.1";

    handle_t lsa_b;
    NetConn *cn, *lookup;
    PolicyHandle policy_handle;
    uint32 lsa_access;
    wchar16_t localhost_addr[10];
    NTSTATUS status = STATUS_SUCCESS;
    RPCSTATUS rpcstatus = 0;

    if (conn == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    lsa_b = NULL;
    cn = NULL;

    /* check if requested connection is a localhost connection */
    if (hostname == NULL) {
        size_t addr_size = sizeof(localhost_addr)/sizeof(wchar16_t);
        memset(localhost_addr, 0, addr_size);
        wcstowc16s(localhost_addr, localhost, addr_size);
        hostname = (wchar16_t*)localhost_addr;
    }

    /* look for existing connection */
    FIND_CONN(lookup, hostname);
    if (lookup &&
        (req_lsa_flags == 0 || (lookup->lsa.lsa_access & req_lsa_flags))) {

        *conn = lookup;
        return STATUS_SUCCESS;
    }

    if (lookup == NULL) {
        /* create a new connection */
        cn = (NetConn*) malloc(sizeof(NetConn));
        if (cn == NULL) return STATUS_NO_MEMORY;
        memset(cn, 0, sizeof(NetConn));

    } else {
        cn = lookup;
    }

    if (!(cn->lsa.lsa_access & req_lsa_flags) &&
        cn->lsa.bind) {
        status = LsaClose(lsa_b, &cn->lsa.policy_handle);
	
        memset(&cn->lsa.policy_handle, 0, sizeof(cn->lsa.policy_handle));
        cn->lsa.lsa_access = 0;
    }

    if (cn->lsa.lsa_access == 0 ||
        cn->lsa.bind == NULL) {
        char *host = NULL;

        lsa_access = lsa_flags | req_lsa_flags;

        host = awc16stombs(hostname);
        if (host == NULL) return STATUS_NO_MEMORY;

        rpcstatus = InitLsaBindingDefault(&lsa_b, host, access_token);
        if (rpcstatus != 0) return STATUS_UNSUCCESSFUL;

        SAFE_FREE(host);

        status = LsaOpenPolicy2(lsa_b, hostname, NULL, lsa_access,
                                &policy_handle);
        if (status != 0) return status;

        cn->lsa.bind          = lsa_b;
        cn->lsa.policy_handle = policy_handle;
        cn->lsa.lsa_access    = lsa_access;
    }

    /* set the host name if it's completely new connection */
    if (cn->hostname == NULL) {
        cn->hostname = wc16sdup(hostname);
    }

    /* add newly created connection (if it is in fact new) */
    if (cn != lookup) ADD_CONN(cn);

    /* return initialised connection and status code */
    *conn = cn;
    return STATUS_SUCCESS;
}


NTSTATUS NetDisconnectSamr(NetConn *cn)
{
    NTSTATUS status;
    handle_t samr_b;

    if (cn == NULL) return STATUS_INVALID_PARAMETER;

    samr_b = cn->samr.bind;
    
    status = SamrClose(samr_b, &cn->samr.dom_handle);
    if (status != 0) return status;

    memset(&cn->samr.dom_handle, 0, sizeof(cn->samr.dom_handle));
    cn->samr.dom_access = 0;
    SAFE_FREE(cn->samr.dom_name);
    SAFE_FREE(cn->samr.dom_sid);

    status = SamrClose(samr_b, &cn->samr.btin_dom_handle);
    if (status != 0) return status;
    memset(&cn->samr.btin_dom_handle, 0, sizeof(cn->samr.btin_dom_handle));
    cn->samr.btin_dom_access = 0;

    status = SamrClose(samr_b, &cn->samr.conn_handle);
    if (status != 0) return status;
    memset(&cn->samr.conn_handle, 0, sizeof(cn->samr.conn_handle));
    cn->samr.conn_access = 0;

    FreeSamrBinding(&samr_b);
    cn->samr.bind = NULL;

    if (cn->lsa.bind == NULL) {
        DEL_CONN(cn);

        SAFE_FREE(cn->hostname);
        SAFE_FREE(cn);
    }

    return STATUS_SUCCESS;
}


NTSTATUS NetDisconnectLsa(NetConn *cn)
{
    NTSTATUS status;
    handle_t lsa_b;

    if (cn == NULL) return STATUS_INVALID_PARAMETER;

    lsa_b = cn->lsa.bind;

    status = LsaClose(lsa_b, &cn->lsa.policy_handle);
    if (status != 0) return status;
    memset(&cn->lsa.policy_handle, 0, sizeof(cn->lsa.policy_handle));
    cn->lsa.lsa_access = 0;

    FreeLsaBinding(&lsa_b);
    cn->lsa.bind = NULL;

    if (cn->samr.bind == NULL) {
        DEL_CONN(cn);

        SAFE_FREE(cn->hostname);
        SAFE_FREE(cn);
    }

    return STATUS_SUCCESS;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
