/*
 *
 * (c) Copyright 1989 OPEN SOFTWARE FOUNDATION, INC.
 * (c) Copyright 1989 HEWLETT-PACKARD COMPANY
 * (c) Copyright 1989 DIGITAL EQUIPMENT CORPORATION
 * To anyone who acknowledges that this file is provided "AS IS"
 * without any express or implied warranty:
 *                 permission to use, copy, modify, and distribute this
 * file for any purpose is hereby granted without fee, provided that
 * the above copyright notices and this notice appears in all source
 * code copies, and that none of the names of Open Software
 * Foundation, Inc., Hewlett-Packard Company, or Digital Equipment
 * Corporation be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission.  Neither Open Software Foundation, Inc., Hewlett-
 * Packard Company, nor Digital Equipment Corporation makes any
 * representations about the suitability of this software for any
 * purpose.
 *
 */
/*
 */
#ifndef _CNNET_H
#define _CNNET_H	1
/*
**
**  NAME
**
**      cnnet.h
**
**  FACILITY:
**
**      Remote Procedure Call (RPC)
**
**  ABSTRACT:
**
**  Interface to the NCA Connection Protocol Service's Network Service.
**
**
*/

#include <dce/dce.h>

/*
 * C O N N E C T I O N   S T A T E S
 */
enum
{
     RPC_C_CN_CLOSED = 0,
     RPC_C_CN_CONNECTING,
     RPC_C_CN_OPEN,
};

/* Moved to cninline.c */
/*
 * R P C _ C N _ N E T W O R K _ I O V E C T O R _ T O _ I O V
 */

#define RPC_CN_NETWORK_IOVECTOR_TO_IOV(iovector, iov, iovcnt, bytes_to_send)\
    _RPC_CN_NETWORK_IOVECTOR_TO_IOV(iovector, iov, (unsigned32 *) &iovcnt, (unsigned32 *) &bytes_to_send)


/* Moved to cninline.c */
/*
 * R P C _ C N _ N E T W O R K _ I O V _ A D J U S T
 */

#define RPC_CN_NETWORK_IOV_ADJUST(iovp, iovcnt, cc)\
    _RPC_CN_NETWORK_IOV_ADJUST(iovp, (int *) &iovcnt, (int) cc)


/*
 * R P C _ _ C N _ N E T W O R K _ U S E _ P R O T S E Q
 */

PRIVATE void rpc__cn_network_use_protseq(
    rpc_protseq_id_t            /* pseq_id */,
    unsigned32                  /* max_calls */,
    rpc_addr_p_t                /* rpc_addr */,
    unsigned_char_p_t           /* endpoint */,
    unsigned32                  * /* st */
    );

/*
 * R P C _ _ C N _ N E T W O R K _ M O N
 */

PRIVATE void rpc__cn_network_mon(
    rpc_binding_rep_p_t     /* binding_r */,
    rpc_client_handle_t     /* client_h */,
    rpc_network_rundown_fn_t /* rundown */,
    unsigned32              * /* st */
    );

/*
 * R P C _ _ C N _ N E T W O R K _ S T O P _ M O N
 */

PRIVATE void rpc__cn_network_stop_mon(
    rpc_binding_rep_p_t     /* binding_r */,
    rpc_client_handle_t     /* client_h */,
    unsigned32              * /* st */
    );

/*
 * R P C _ _ C N _ N E T W O R K _ M A I N T
 */

PRIVATE void rpc__cn_network_maint(
    rpc_binding_rep_p_t     /* binding_r */,
    unsigned32              * /* st */
    );

/*
 * R P C _ _ C N _ N E T W O R K _ S T O P _ M A I N T
 */

PRIVATE void rpc__cn_network_stop_maint(
    rpc_binding_rep_p_t     /* binding_r */,
    unsigned32              * /* st */
    );

/*
 * R P C _ _ C N _ N E T W O R K _ C L O S E
 */

PRIVATE void rpc__cn_network_close(
    rpc_binding_rep_p_t     /* binding_r */,
    unsigned32              * /* st */
    );

/*
 * R P C _ _ C N _ N E T W O R K _ S E L E C T _ D I S P A T C H
 */

PRIVATE void rpc__cn_network_select_dispatch(
    rpc_socket_t            /* desc */,
    pointer_t               /* priv_info */,
    boolean32               /* is_active */,
    unsigned32              * /* st */
    );

/*
 * R P C _ _ C N _ N E T W O R K _ I N Q _ P R O T _ V E R S
 */

PRIVATE void rpc__cn_network_inq_prot_vers(
    unsigned8               * /* prot_id */,
    unsigned32              * /* version_major */,
    unsigned32              * /* version_minor */,
    unsigned32              * /* st */
    );

/*
 * R P C _ _ C N _ N E T W O R K _ R E Q _ C O N N E C T
 */

PRIVATE void rpc__cn_network_req_connect(
    rpc_addr_p_t            /* rpc_addr */,
    rpc_cn_assoc_p_t        /* assoc */,
    unsigned32              * /* st */
    );

/*
 * R P C _ _ C N _ N E T W O R K _ C L O S E _ C O N N E C T
 */

PRIVATE void rpc__cn_network_close_connect(
    rpc_cn_assoc_p_t        /* assoc */,
    unsigned32              * /* st */
    );


/*
 * R P C _ _ C N _ N E T W O R K _ C O N N E C T _ F A I L
 */

PRIVATE boolean32 rpc__cn_network_connect_fail(unsigned32);


/*
 * These two are internal API's so you can tweak the TCP buffering.
 */

/*
 * R P C _ _ C N _ S E T _ S O C K _ B U F F S I Z E
 */
PRIVATE void rpc__cn_set_sock_buffsize(
        unsigned32	  /* rsize */,
        unsigned32	  /* ssize */,
        error_status_t	* /* st */
    );

/*
 * R P C _ _ C N _ I N Q _ S O C K _ B U F F S I Z E
 */
PRIVATE void rpc__cn_inq_sock_buffsize(
        unsigned32	* /* rsize */,
        unsigned32	* /* ssize */,
        error_status_t  * /* st */
    );


#endif /* _CNNET_H */





