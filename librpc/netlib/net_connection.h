/* Editor Settings: expandtabs and use 4 spaces for indentation
* ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
*/

/*
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software    2007-2008
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef _NET_CONNECTION_H_
#define _NET_CONNECTION_H_

#include <lwrpc/types.h>


typedef struct net_conn {
    wchar16_t *hostname;

    UINT8 sess_key[16];
    UINT32 sess_key_len;

    struct samr {
        handle_t bind;

        CONNECT_HANDLE hConn;
        UINT32 conn_access;

        DOMAIN_HANDLE hDomain;
        UINT32 dom_access;
        wchar16_t *dom_name;
        PSID dom_sid;

        DOMAIN_HANDLE hBtinDomain;
        UINT32 btin_dom_access;
    } samr;

    struct lsa {
        handle_t bind;

        POLICY_HANDLE hPolicy;
        UINT32 lsa_access;
    } lsa;

    struct net_conn *next;

} NetConn;


typedef struct net_conn_list {
    struct net_conn *conn;
    pthread_mutex_t mutex;
} NetConnList;



#define CONN_LIST_LOCK(list)                      \
    do {                                          \
        int ret = 0;                              \
        ret = pthread_mutex_lock(&(list)->mutex); \
        if (ret) {                                \
            status = STATUS_UNSUCCESSFUL;         \
            goto error;                           \
                                                  \
        } else {                                  \
            locked = 1;                           \
        }                                         \
    } while (0);


#define CONN_LIST_UNLOCK(list)                      \
    do {                                            \
        int ret = 0;                                \
        if (!locked) break;                         \
        ret = pthread_mutex_unlock(&(list)->mutex); \
        if (ret && status == STATUS_SUCCESS) {      \
            status = STATUS_UNSUCCESSFUL;           \
                                                    \
        } else {                                    \
            locked = 0;                             \
        }                                           \
    } while (0);


NTSTATUS
NetConnListInit(
    void
    );


NTSTATUS
NetConnListDestroy(
    void
    );


NTSTATUS
NetConnAdd(
    NetConn *c
    );


NTSTATUS
NetConnDelete(
    NetConn *c
    );


NetConn*
FindNetConn(
    const wchar16_t *name
    );


NTSTATUS
NetConnectSamr(
    NetConn **conn,
    const wchar16_t *hostname,
    UINT32 req_dom_flags,
    UINT32 req_btin_dom_flags,
    PIO_CREDS creds
    );


NTSTATUS
NetConnectLsa(
    NetConn **conn,
    const wchar16_t *hostname,
    UINT32 req_lsa_flags,
    PIO_CREDS creds
    );


NTSTATUS
NetDisconnectSamr(
    NetConn *cn
    );


NTSTATUS
NetDisconnectLsa(
    NetConn *cn
    );


NTSTATUS
NetDisconnectAll(
    void
    );


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
