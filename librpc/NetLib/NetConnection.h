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

#ifndef _NET_CONNECTION_H_
#define _NET_CONNECTION_H_

#include <lwrpc/types.h>
#include <lwrpc/security.h>


typedef struct net_conn {
    wchar16_t *hostname;

    uint8 sess_key[16];
    uint32 sess_key_len;
    
    struct samr {
        handle_t bind;
	
        PolicyHandle conn_handle;
        uint32 conn_access;
	
        PolicyHandle dom_handle;
        uint32 dom_access;
        wchar16_t *dom_name;
        DomSid *dom_sid;
	
        PolicyHandle btin_dom_handle;
        uint32 btin_dom_access;
    } samr;

    struct lsa {
        handle_t bind;

        PolicyHandle policy_handle;
        uint32 lsa_access;
    } lsa;

    struct net_conn *next;

} NetConn;


NetConn* FirstConn(NetConn* conn, int set);

#define GetFirstConn(cn) FirstConn(cn, 0)
#define SetFirstConn(cn) FirstConn(cn, 1)

#define FIND_CONN(cn, name)                                     \
    {                                                           \
        NetConn *c = GetFirstConn(NULL);                        \
        while (c && wc16scmp(c->hostname, name)) c = c->next;   \
        cn = c;                                                 \
    }


#define ADD_CONN(cn)                     \
    {                                    \
        NetConn *c = GetFirstConn(NULL); \
        if (c) {                         \
            while (c->next) c = c->next; \
            c->next = cn;                \
            cn->next = NULL;             \
        } else {                         \
            SetFirstConn(cn);            \
        }                                \
    }


#define DEL_CONN(cn)                                \
    {                                               \
        NetConn *pc = NULL;                         \
        NetConn *c = GetFirstConn(NULL);            \
        while(c && c != cn) { pc = c; c = c->next; }    \
        if (!pc && c) {                                 \
            SetFirstConn(cn->next);                     \
        } else if (pc && c) {                           \
            pc->next = cn->next;                        \
        }                                               \
    }

#endif /* _NET_CONNECTION_H_ */

NTSTATUS
NetConnListInit(
    void
    );

NTSTATUS
NetConnListDestroy(
    void
    );


NTSTATUS
NetConnectSamr(
    NetConn **conn,
    const wchar16_t *hostname,
    uint32 req_dom_flags,
    uint32 req_btin_dom_flags,
    PIO_ACCESS_TOKEN access_token
    );

NTSTATUS
NetConnectLsa(
    NetConn **conn,
    const wchar16_t *hostname,
    uint32 req_lsa_flags,
    PIO_ACCESS_TOKEN access_token
    );


NTSTATUS
NetDisconnectSamr(
    NetConn *cn
    );


NTSTATUS
NetDisconnectLsa(
    NetConn *cn
    );

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
