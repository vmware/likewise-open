/*
 *
 * Copyright (C) 2013-2015 VMware, Inc. All rights reserved.
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
 * Copyright (c) 2010 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of its
 *     contributors may be used to endorse or promote products derived from
 *     this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
**
**  NAME
**
**      cnassm.h
**
**  FACILITY:
**
**      Remote Procedure Call (RPC)
**
**  ABSTRACT:
**
**  Definitions of types/constants internal to the NCA Connection
**  RPC Protocol Service association state machine.
**
**
*/

#ifndef _CNASSM_H
#define _CNASSM_H	1

#include <cnsm.h>

/*
 * The default fragment size all implementations of the NCA
 * Connection protocol must be able to receive as defined in the NCA
 * Architecture spec.
 */
#define RPC_C_ASSOC_MUST_RECV_FRAG_SIZE         1432


/***********************************************************************/
/*
 * R P C _ C N _ A S S O C _ C H E C K _ S T
 *
 * This macro will check the given status. If not rpc_s_ok a local
 * error event will be inserted into the association event list.
 */

#define RPC_CN_ASSOC_CHECK_ST(assoc, st)\
    _RPC_CN_ASSOC_CHECK_ST(assoc, *st)



/***********************************************************************/
/*
 * R P C _ C N _ A S S O C _ S M _ T R C
 */
#ifdef DEBUG
#define RPC_CN_ASSOC_SM_TRC(assoc, event_id)\
{\
    if ((assoc)->assoc_flags & RPC_C_CN_ASSOC_CLIENT)\
    {\
        RPC_DBG_PRINTF (rpc_e_dbg_cn_state, RPC_C_CN_DBG_ASSOC_SM_TRACE, \
                        ("STATE CLIENT ASSOC: %x state->%s event->%s\n",\
                         assoc,\
                         rpc_g_cn_assoc_client_states[(assoc)->assoc_state.cur_state-RPC_C_CN_STATEBASE],\
                         rpc_g_cn_assoc_client_events[event_id-RPC_C_CN_STATEBASE]));\
    }\
    else\
    {\
        RPC_DBG_PRINTF (rpc_e_dbg_cn_state, RPC_C_CN_DBG_ASSOC_SM_TRACE, \
                        ("STATE SERVER ASSOC: %x state->%s event->%s\n",\
                         assoc,\
                         rpc_g_cn_assoc_server_states[(assoc)->assoc_state.cur_state-RPC_C_CN_STATEBASE],\
                         rpc_g_cn_assoc_server_events[event_id-RPC_C_CN_STATEBASE]));\
    }\
}
#else
#define RPC_CN_ASSOC_SM_TRC(assoc, event_id)
#endif /* DEBUG */


/***********************************************************************/
/*
 * R P C _ C N _ A S S O C _ S M _ T R C _ S T A T E
 */
#ifdef DEBUG
#define RPC_CN_ASSOC_SM_TRC_STATE(assoc)\
{\
    if ((assoc)->assoc_flags & RPC_C_CN_ASSOC_CLIENT)\
    {\
        RPC_DBG_PRINTF (rpc_e_dbg_cn_state, RPC_C_CN_DBG_ASSOC_SM_TRACE, \
                        ("STATE CLIENT ASSOC: %x new state->%s\n",\
                         assoc, \
                         rpc_g_cn_assoc_client_states[(assoc)->assoc_state.cur_state-RPC_C_CN_STATEBASE])); \
    }\
    else\
    {\
        RPC_DBG_PRINTF (rpc_e_dbg_cn_state, RPC_C_CN_DBG_ASSOC_SM_TRACE, \
                        ("STATE SERVER ASSOC: %x new state->%s\n",\
                         assoc, \
                         rpc_g_cn_assoc_server_states[(assoc)->assoc_state.cur_state-RPC_C_CN_STATEBASE])); \
    }\
}
#else
#define RPC_CN_ASSOC_SM_TRC_STATE(assoc)
#endif /* DEBUG */


/***********************************************************************/
/*
 * R P C _ C N _ A S S O C _ E V A L _ N E T W O R K _ E V E N T
 *
 * This macro will be called by the network receiver thread when an
 * association network event is detected. The "scanned" bit in
 * the association is turned off. This bit is used in finding
 * associations to reclaim. The fragbuf is freed if provided as an
 * event parameter.
 */


extern unsigned32     rpc__cn_sm_eval_event (
    unsigned32                  /* event_id */,
    pointer_t                   /* event_parameter */,
    pointer_t                   /* spc_struct */,
    rpc_cn_sm_ctlblk_p_t         /* sm */);


/***********************************************************************/
/*
 * R P C _ C N _ A S S O C _ E V A L _ N E T W O R K _ E V E N T
 *
 * This macro will be called by the network receiver thread when an
 * association network event is detected. The "scanned" bit in
 * the association is turned off. This bit is used in finding
 * associations to reclaim. The fragbuf is freed if provided as an
 * event parameter.
 */
#define RPC_CN_ASSOC_EVAL_NETWORK_EVENT(assoc, event_id, fragbuf, st)\
    _RPC_CN_ASSOC_EVAL_NETWORK_EVENT(assoc, event_id, fragbuf, (unsigned32 *) &(st))


/***********************************************************************/
/*
 * R P C _ C N _ A S S O C _ E V A L _ U S E R _ E V E N T
 *
 * This macro will be called when user level events are detected. If
 * the association status is bad then don't evaluate the user event.
 * The "scanned" bit in the association is turned off.
 */
#define RPC_CN_ASSOC_EVAL_USER_EVENT(assoc, event_id, event_param, st)\
    _RPC_CN_ASSOC_EVAL_USER_EVENT(assoc, event_id, event_param, (unsigned32 *) &(st))


/***********************************************************************/
/*
 * R P C _ C N _ A S S O C _ I N S E R T _ E V E N T
 *
 * This macro will be called when an event is generated inside an
 * action routine of the association state machine.
 */

#define RPC_CN_ASSOC_INSERT_EVENT(assoc, event)\
    _RPC_CN_ASSOC_INSERT_EVENT(assoc, event)


/***********************************************************************/
/*
 * A S S O C   E V E N T S
 */

/*
 * Events common to both client and server state machines and a
 * comment as to who generated them: the user of the association
 * services or the network.
 *
 * Note: local_error is not defined in the architecture. It is an
 * event indicating a fatal error has occured while processing an
 * event in the state machine.
 *
 * State values are incremented by 100 to distinguish them from
 * action routine indexes which are all < 100.  This was done as
 * an efficiency measure to the engine, rpc__cn_sm_eval_event().
 */
enum {
        RPC_C_ASSOC_ABORT_REQ          = 101,   /* user         */
        RPC_C_ASSOC_NO_CONN_IND        = 104,  /* network      */
        RPC_C_ASSOC_ALLOCATE_REQ       = 109,  /* user         */
        RPC_C_ASSOC_DEALLOCATE_REQ     = 110,  /* user         */
        RPC_C_ASSOC_LOCAL_ERROR        = 112,  /* internal     */
        RPC_C_ASSOC_SHUTDOWN_REQ       = 111,   /* user */

/*
 * Events only applicable to client state machine
 *
 * Note: calls_done is= 12, in the architecture. I'm
 * making it= 13, here so local_error will be 12 and therefore the same
 * as the server local_error event.
 *
 * Note: shutdown_ind is= 11, in the architecture. I'm
 * making it= 14, here so shutdown_req will be 11 and therefore the same
 * as the server shutdown_req event.
 */
        RPC_C_ASSOC_REQ                = 100,  /* user         */
        RPC_C_ASSOC_REQUEST_CONN_ACK   = 102,  /* network      */
        RPC_C_ASSOC_REQUEST_CONN_NACK  = 103,  /* network      */
        RPC_C_ASSOC_ACCEPT_CONF        = 105,  /* network      */
        RPC_C_ASSOC_REJECT_CONF        = 106,  /* network      */
        RPC_C_ASSOC_ALTER_CONTEXT_REQ  = 107,  /* user         */
        RPC_C_ASSOC_ALTER_CONTEXT_CONF = 108,  /* network      */
        RPC_C_ASSOC_CALLS_DONE         = 113,  /* user         */
        RPC_C_ASSOC_SHUTDOWN_IND       = 114,  /* network      */

/*
 * Events only applicable to server state machine
 */
/*
 * Note: alter_context_resp is= 4, in the architecture. I'm
 * making it= 5, here so no_conn_ind will be 4 and therefore the same
 * as the client no_conn_ind event.
 *
 * Note: accept_resp is= 1, in the architecture. I'm making
 * it= 13, here so abort_req will be 1 and therefore the same as the
 * client abort_req event.
 */
        RPC_C_ASSOC_IND                = 100,  /* network      */
        RPC_C_ASSOC_REJECT_RESP        = 102,  /* user         */
        RPC_C_ASSOC_ALTER_CONTEXT_IND  = 103,  /* network      */
        RPC_C_ASSOC_ALTER_CONTEXT_RESP = 105,  /* user         */
        RPC_C_ASSOC_AUTH3_IND          = 106,  /* network      */
        RPC_C_ASSOC_AUTH3_ACK          = 107,  /* user         */
        RPC_C_ASSOC_AUTH3_NACK         = 108,  /* user         */
        RPC_C_ASSOC_ACCEPT_RESP        = 113,   /* user        */
        RPC_C_ASSOC_ASSOC_COMPLETE_RESP= 114,   /* user        */


/***********************************************************************/
/*
 * C L I E N T   A S S O C   S T A T E S
 */

        RPC_C_CLIENT_ASSOC_CLOSED              = 100,
        RPC_C_CLIENT_ASSOC_CONNECT_WAIT        = 101,
        RPC_C_CLIENT_ASSOC_INIT_WAIT           = 102,
        RPC_C_CLIENT_ASSOC_OPEN                = 103,
        RPC_C_CLIENT_ASSOC_ACTIVE              = 104,
        RPC_C_CLIENT_ASSOC_CALL_DONE_WAIT      = 105,
        RPC_C_CLIENT_ASSOC_STATES              = 106,
};

/***********************************************************************/
/*
 * C L I E N T   A S S O C   T A B L E S
 */
EXTERNAL rpc_cn_sm_state_entry_p_t rpc_g_cn_client_assoc_sm [];
EXTERNAL rpc_cn_sm_action_fn_t     rpc_g_cn_client_assoc_act_tbl [];

EXTERNAL char   *rpc_g_cn_assoc_client_events [];
EXTERNAL char   *rpc_g_cn_assoc_client_states [];


/***********************************************************************/
/*
 * S E R V E R   A S S O C   S T A T E S
 */
enum {
        RPC_C_SERVER_ASSOC_CLOSED              = 100,
        RPC_C_SERVER_ASSOC_REQUESTED           = 101,
        RPC_C_SERVER_ASSOC_AUTH3_WAIT          = 102,
        RPC_C_SERVER_ASSOC_AUTH3               = 103,
        RPC_C_SERVER_ASSOC_OPEN                = 104,
        RPC_C_SERVER_ASSOC_WAIT                = 105,
        RPC_C_SERVER_ASSOC_STATES              = 106,
};

/***********************************************************************/
/*
 * S E R V E R   A S S O C   T A B L E S
 */
EXTERNAL rpc_cn_sm_state_entry_p_t rpc_g_cn_server_assoc_sm [];
EXTERNAL rpc_cn_sm_action_fn_t     rpc_g_cn_server_assoc_act_tbl [];

EXTERNAL const char   *rpc_g_cn_assoc_server_events [];
EXTERNAL const char   *rpc_g_cn_assoc_server_states [];

#endif /* _CNASSM_H */
