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
/*
**
**  NAME
**
**      cnsm.c
**
**  FACILITY:
**
**      Remote Procedure Call (RPC)
**
**  ABSTRACT:
**
**  The NCA Connection Protocol State Machine Service
**
**
*/

#include <commonp.h>    /* Common declarations for all RPC runtime */
#include <com.h>        /* Common communications services */
#include <comprot.h>    /* Common protocol services */
#include <cnp.h>        /* NCA Connection private declarations */
#include <cnsm.h>



typedef enum RPC_ACTION_TBL_e
{
    C_ASSOC_ACTION = 1,
    C_GRP_ACTION,
    C_CALL_ACTION,
    S_GRP_ACTION,
    S_CALL_ACTION,
    S_ASSOC_ACTION,
} RPC_ACTION_TBL_e_t;

typedef enum RPC_SM_TBL_e
{
    C_GRP_SM = 1,
    C_ASSOC_SM,
    C_CALL_SM,
    S_GRP_SM,
    S_ASSOC_SM,
    S_CALL_SM,
} RPC_SM_TBL_e_t;

/* Enable for verbose state machine action/state interpretation */
INTERNAL int
rpc__cn_sm_debug(void)
{
    static int dcerpc_sm_debug = -1;
    if (dcerpc_sm_debug == -1)
    {
        /*
         * The use of RPC_DEBUG here is deliberate. This is NOT yet integrated
         * with the existing rpclog.c debugging system, which also uses this
         * environment variable. The use here will not interfere with
         * RPC_DEBUG use in rpc_log.c, it will merely be enabled whould anyone
         * compile DCE/RPC #define debug, and then set RPC_DEBUG to some value.
         * Additional cnsm: debug messages will be emitted in that case.
         *
         * TBD:Adam-Integrate cnsm logging with RPC_DDBUG at a later date.
         */
        if (getenv("RPC_DEBUG"))
        {
            dcerpc_sm_debug = 1;
        }
        else
        {
            dcerpc_sm_debug = 0;
        }
    }
    return dcerpc_sm_debug;
}

PRIVATE char *rpc__state_table_lookup(RPC_SM_TBL_e_t type, int action)
{
    extern int rpc_g_cn_grp_client_states_len;
    extern int rpc_g_cn_assoc_client_states_len;
    extern int rpc_g_cn_call_client_states_len;
    extern int rpc_g_cn_grp_server_states_len;
    extern int rpc_g_cn_assoc_server_states_len;
    extern int rpc_g_cn_call_server_states_len;

    extern char *rpc_g_cn_grp_client_states[];
    extern char *rpc_g_cn_assoc_client_states[];
    extern char *rpc_g_cn_call_client_states[];
    extern char *rpc_g_cn_grp_server_states[];
    extern char *rpc_g_cn_assoc_server_states[];
    extern char *rpc_g_cn_call_server_states[];

    char *p = NULL;
    int action_idx = action - RPC_C_CN_STATEBASE ;
    switch(type)
    {
        case C_GRP_SM:
            if (action_idx < 0 || action_idx > rpc_g_cn_grp_client_states_len)
            {
                p = "C_GRP_SM: Invalid state!";
            }
            else
            {
                p = rpc_g_cn_grp_client_states[action_idx];
            }
        break;
        case C_ASSOC_SM:
            if (action_idx < 0 || action_idx > rpc_g_cn_assoc_client_states_len)
            {
                p = "C_ASSOC_SM: Invalid state!";
            }
            else
            {
                p = rpc_g_cn_assoc_client_states[action_idx];
            }
        break;
        case C_CALL_SM:
            if (action_idx < 0 || action_idx > rpc_g_cn_call_client_states_len)
            {
                p = "C_CALL_SM: Invalid state!";
            }
            else
            {
                p = rpc_g_cn_call_client_states[action_idx];
            }
        break;
        case S_GRP_SM:
            if (action_idx < 0 || action_idx > rpc_g_cn_grp_server_states_len)
            {
                p = "S_GRP_SM: Invalid state!";
            }
            else
            {
                p = rpc_g_cn_grp_server_states[action_idx];
            }
        break;
        case S_ASSOC_SM:
            if (action_idx < 0 || action_idx > rpc_g_cn_assoc_server_states_len)
            {
                p = "S_ASSOC_SM: Invalid state!";
            }
            else
            {
                p = rpc_g_cn_assoc_server_states[action_idx];
            }
        break;
        case S_CALL_SM:
            if (action_idx < 0 || action_idx > rpc_g_cn_call_server_states_len)
            {
                p = "S_CALL_SM: Invalid state!";
            }
            else
            {
                p = rpc_g_cn_call_server_states[action_idx];
            }
        break;
    }
    return p;
}

PRIVATE char *rpc__action_table_lookup(RPC_ACTION_TBL_e_t type, int action)
{
    char *p = NULL;
    switch(type)
    {
        case C_ASSOC_ACTION:
            switch(action)
            {
                case 0: p = "INIT_ASSOC"; break;
                case 1: p = "REQUEST_CONN"; break;
                case 2: p = "MARK_ASSOC"; break;
                case 3: p = "ADD_ASSOC_TO_GRP"; break;
                case 4: p = "REM_ASSOC_FROM_GRP"; break;
                case 5: p = "SET_SECONDARY_ADDR"; break;
                case 6: p = "AUTHENT3"; break;
                case 7: p = "SEND_ALT_CONTEXT_REQ"; break;
                case 8: p = "ABORT_ASSOC"; break;
                case 9: p = "SET_SHUTDOWN_REQUEST"; break;
                case 10: p = "DISCON_CALLS"; break;
                case 11: p = "INCR_ACTIVE"; break;
                case 12: p = "DECR_ACTIVE"; break;
                case 13: p = "MARK_SYNTAX_AND_SEC"; break;
                case 14: p = "MARK_ABORT"; break;
                case 15: p = "ADD_MARK_SET"; break;
                case 16: p = "REM_MARK_ABORT"; break;
                case 17: p = "REM_MARK"; break;
                case 18: p = "REM_MARK_DISCON"; break;
                case 19: p = "DECR_REM_MARK_ABORT"; break;
                case 20: p = "RPC__CN_ASSOC_SM_PROTOCOL_ERROR"; break;
                case 21: p = "PROCESS_FRAG"; break;
                case 22: p = "RETRY_ASSOC"; break;
                case 23: p = "SHUTDOWN_ALLOWED"; break;
                case 24:
                default: p = "ILLEGAL_EVENT_ABORT"; break;
            }
        break;
        case C_GRP_ACTION:
            switch(action)
            {
                case 0: p = "INCR_ASSOC_COUNT"; break;
                case 1: p = "DECR_ASSOC_COUNT"; break;
                case 2:
                default: p = "GRP_SM_PROTOCOL_ERROR"; break;
            }
        break;
        case C_CALL_ACTION:
            switch(action)
            {
                case 0: p = "TRANSMIT_REQ"; break;
                case 1: p = "HANDLE_RECV_FRAG"; break;
                case 2: p = "RAISE_FAULT"; break;
                case 3: p = "FORWARD_ALERT"; break;
                case 4: p = "ALLOCATE_ASSOC"; break;
                case 5: p = "ABORT_SEND"; break;
                case 6: p = "ABORT_RECV"; break;
                case 7: p = "SEND_LAST_FRAG"; break;
                case 8:
                default: p = "CALL_SM_PROTOCOL_ERROR"; break;
            }
        break;
        case S_GRP_ACTION:
            switch(action)
            {
                case 0: p = "CREATE_GROUP_ID"; break;
                case 1: p = "INCR_ASSOC_COUNT"; break;
                case 2: p = "DECR_ASSOC_COUNT"; break;
                case 3: p = "RUNDOWN_HANDLES"; break;
                case 4:
                default:p = "GRP_SM_PROTOCOL_ERROR"; break;
            }
        break;
        case S_CALL_ACTION:
            switch(action)
            {
                case 0: p = "HANDLE_FIRST_FRAG"; break;
                case 1: p = "HANDLE_FRAG"; break;
                case 2: p = "SEND_CALL_RESP"; break;
                case 3: p = "SEND_CALL_FAULT"; break;
                case 4: p = "PROCESS_ALERT_MSG"; break;
                case 5: p = "ABORT_RESP"; break;
                case 6: p = "ABORT_RESP_SEND_FAULT"; break;
                case 7: p = "STOP_ORPHAN"; break;
                case 8: p = "DISCARD_FRAGMENT"; break;
                case 9: p = "CALL_END"; break;
                case 10:
                default: p = "CALL_SM_PROTOCOL_ERROR"; break;
            }
        break;
        case S_ASSOC_ACTION:
            switch(action)
            {
                case  0: p = "ACCEPT_ASSOC";  break;
                case  1: p = "REJECT_ASSOC";  break;
                case  2: p = "ADD_ASSOC_TO_GRP";  break;
                case  3: p = "REM_ASSOC_FROM_GRP";  break;
                case  4: p = "DO_ALTER_CONT_REQ";  break;
                case  5: p = "SEND_ALTER_CONT_RESP";  break;
                case  6: p = "DO_AUTHENT3";  break;
                case  7: p = "DO_ASSOC_REQ";  break;
                case  8: p = "SEND_SHUTDOWN_REQ";  break;
                case  9: p = "INCR_ACTIVE";  break;
                case 10: p = "DECR_ACTIVE";  break;
                case 11: p = "ABORT_ASSOC";  break;
                case 12: p = "MARK_ASSOC";  break;
                case 13: p = "CANCEL_CALLS";  break;
                case 14: p = "ACCEPT_ADD";  break;
                case 15: p = "REM_MARK_ABORT";  break;
                case 16: p = "REM_MARK_CANCEL";  break;
                case 17: p = "INCR_DO_ALTER";  break;
                case 18: p = "SEND_DECR";  break;
                case 19: p = "MARK_ABORT";  break;
                case 20: p = "REM_MARK_ABORT_CAN";  break;
                case 21: p = "RPC__CN_ASSOC_SM_PROTOCOL_ERROR";  break;
                case 22: p = "DO_ASSOC_WAIT";  break;
                case 23:
                default: p = "DO_ASSOC";  break;
            }
        break;
    }
    return p;
}

PRIVATE int rpc__action_table_entry_count(
    rpc_cn_sm_ctlblk_p_t sm,
    RPC_ACTION_TBL_e_t *ret_table_type,
    char **ret_table_name)
{
    extern rpc_cn_sm_action_fn_t rpc_g_cn_client_assoc_act_tbl[];
    extern rpc_cn_sm_action_fn_t rpc_g_cn_client_grp_action_tbl[];
    extern rpc_cn_sm_action_fn_t rpc_g_cn_client_call_action_tbl[];
    extern rpc_cn_sm_action_fn_t rpc_g_cn_server_assoc_act_tbl[];
    extern rpc_cn_sm_action_fn_t rpc_g_cn_server_grp_action_tbl[];
    extern rpc_cn_sm_action_fn_t rpc_g_cn_server_call_action_tbl[];

    extern int rpc_g_cn_client_assoc_act_tbl_len;
    extern int rpc_g_cn_client_grp_action_tbl_len;
    extern int rpc_g_cn_client_call_action_tbl_len;
    extern int rpc_g_cn_server_assoc_act_tbl_len;
    extern int rpc_g_cn_server_grp_action_tbl_len;
    extern int rpc_g_cn_server_call_action_tbl_len;

    int table_size = 0;
    RPC_ACTION_TBL_e_t table_type = 0;
    char *table_name = NULL;
    rpc_cn_sm_action_fn_p_t table = sm->action_tbl;

    if (table == rpc_g_cn_client_assoc_act_tbl)
    {
        table_size = rpc_g_cn_client_assoc_act_tbl_len;
        table_type = C_ASSOC_ACTION;
        table_name = "c_assoc_action";
    }
    else if (table == rpc_g_cn_client_grp_action_tbl)
    {
        table_size = rpc_g_cn_client_grp_action_tbl_len;
        table_type = C_GRP_ACTION;
        table_name = "c_grp_action";
    }
    else if (table == rpc_g_cn_client_call_action_tbl)
    {
        table_size = rpc_g_cn_client_call_action_tbl_len;
        table_type = C_CALL_ACTION;
        table_name = "c_call_action";
    }
    else if (table == rpc_g_cn_server_assoc_act_tbl)
    {
        table_size = rpc_g_cn_server_assoc_act_tbl_len;
        table_type = S_ASSOC_ACTION;
        table_name = "s_assoc_action";
    }
    else if (table == rpc_g_cn_server_grp_action_tbl)
    {
        table_size = rpc_g_cn_server_grp_action_tbl_len;
        table_type = S_GRP_ACTION;
        table_name = "s_grp_action";
    }
    else if (table == rpc_g_cn_server_call_action_tbl)
    {
        table_size = rpc_g_cn_server_call_action_tbl_len;
        table_type = S_CALL_ACTION;
        table_name = "s_call_action";
    }
    *ret_table_name = table_name;
    *ret_table_type = table_type;
    return table_size;
}


/*
 * There are 3 table types x 2; server and client tables:
 *    rpc_g_cn_server_grp_sm
 *    rpc_g_cn_server_assoc_sm
 *    rpc_g_cn_server_call_sm
 */
PRIVATE int rpc__sm_table_entry_count(
    rpc_cn_sm_ctlblk_p_t sm,
    RPC_SM_TBL_e_t *ret_table_type,
    int *ret_entry_len,
    char **ret_table_name)
{
    /* Client tables and lengths*/
    extern rpc_cn_sm_state_entry_p_t rpc_g_cn_client_grp_sm[];
    extern rpc_cn_sm_state_entry_p_t rpc_g_cn_client_assoc_sm[];
    extern rpc_cn_sm_state_entry_p_t rpc_g_cn_client_call_sm[];
    extern int rpc_g_cn_client_grp_sm_len;
    extern int rpc_g_cn_client_assoc_sm_len;
    extern int rpc_g_cn_client_call_sm_len;
    extern int rpc_g_cn_client_grp_sm_entry_len;
    extern int rpc_g_cn_client_assoc_sm_entry_len;
    extern int rpc_g_cn_client_call_sm_entry_len;

    /* Server tables and lengths*/
    extern rpc_cn_sm_state_entry_p_t rpc_g_cn_server_grp_sm[];
    extern rpc_cn_sm_state_entry_p_t rpc_g_cn_server_assoc_sm[];
    extern rpc_cn_sm_state_entry_p_t rpc_g_cn_server_call_sm[];
    extern int rpc_g_cn_server_grp_sm_len;
    extern int rpc_g_cn_server_assoc_sm_len;
    extern int rpc_g_cn_server_call_sm_len;
    extern int rpc_g_cn_server_grp_sm_entry_len;
    extern int rpc_g_cn_server_assoc_sm_entry_len;
    extern int rpc_g_cn_server_call_sm_entry_len;

    int table_size = 0;
    int entry_len = 0;
    RPC_SM_TBL_e_t table_type = 0;
    char *table_name = NULL;
    rpc_cn_sm_state_entry_p_t *table = sm->state_tbl;

    if (table == rpc_g_cn_client_grp_sm)
    {
        table_size = rpc_g_cn_client_grp_sm_len;
        entry_len = rpc_g_cn_client_grp_sm_entry_len;
        table_type = C_GRP_SM;
        table_name = "c_grp_sm";
    }
    else if (table == rpc_g_cn_client_assoc_sm)
    {
        table_size = rpc_g_cn_client_assoc_sm_len;
        entry_len = rpc_g_cn_client_assoc_sm_entry_len;
        table_type = C_ASSOC_SM;
        table_name = "c_assoc_sm";
    }
    else if (table == rpc_g_cn_client_call_sm)
    {
        table_size = rpc_g_cn_client_call_sm_len;
        entry_len = rpc_g_cn_client_call_sm_entry_len;
        table_type = C_CALL_SM;
        table_name = "c_call_sm";
    }
    else if (table == rpc_g_cn_server_grp_sm)
    {
        table_size = rpc_g_cn_server_grp_sm_len;
        entry_len = rpc_g_cn_server_grp_sm_entry_len;
        table_type = S_GRP_SM;
        table_name = "s_grp_sm";
    }
    else if (table == rpc_g_cn_server_assoc_sm)
    {
        table_size = rpc_g_cn_server_assoc_sm_len;
        entry_len = rpc_g_cn_server_assoc_sm_entry_len;
        table_type = S_ASSOC_SM;
        table_name = "s_assoc_sm";
    }
    else if (table == rpc_g_cn_server_call_sm)
    {
        table_size = rpc_g_cn_server_call_sm_len;
        entry_len = rpc_g_cn_server_call_sm_entry_len;
        table_type = S_CALL_SM;
        table_name = "s_call_sm";
    }
    *ret_table_name = table_name;
    *ret_table_type = table_type;
    *ret_entry_len = entry_len;
    return table_size;
}

/***********************************************************************/


/*
**++
**
**  ROUTINE NAME:       rpc__cn_sm_init
**
**  SCOPE:              PRIVATE - declared in cnsm.h
**
**  DESCRIPTION:
**
**  The routine will be used to initialize a state machine control
**  block. Depending on the type of state machine it will be called
**  in various places. It basically just fills in the table pointers
**  given, sets the state to closed and initializes the event list.
**
**  INPUTS:
**
**      state_tbl       The state table this state machine is to use.
**      action_tbl      The action routine table this state machine
**                      is to use.
**	tbl_id		The identifier of the particular action table
**			we are storing in the control block.
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      sm              The state machine control block which is to
**                      be initialized.
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     none
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE void rpc__cn_sm_init
(
rpc_cn_sm_state_entry_p_t       *state_tbl,
rpc_cn_sm_action_fn_p_t         action_tbl,
rpc_cn_sm_ctlblk_p_t            sm,
unsigned32			tbl_id
)
{

    /*
     * Put the pointers to the given tables into the state machine
     * control block.
     */
    sm->state_tbl = state_tbl;
    sm->action_tbl = action_tbl;

    /*
     * Set the initial state in the state machine control block to
     * "closed".
     */
    sm->cur_state = RPC_C_SM_CLOSED_STATE;

    /*
     * Store the tbl_id in the controlblock and use it later
     * to selectively bypass calls to the event evaluation
     * routine, going directly to the action routines.
     */
     sm->tbl_id = tbl_id;

    /*
     * Initialize the state machine control block event list.
     */
    rpc__cn_sm_init_event_list(sm);
}


/*
**++
**
**  ROUTINE NAME:       rpc__cn_sm_init_event_list
**
**  SCOPE:              PRIVATE - declared in cnsm.h
**
**  DESCRIPTION:
**
** This routine will initialize the event list contained in the
** specified state machine control block. This routine is called as
** part of initializing the state machine control block.
**
**  INPUTS:             none
**
**  INPUTS/OUTPUTS:     none
**
**  OUTPUTS:
**
**      sm              The state machine control block containing
**                      the event list to be initialized.
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     none
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE void rpc__cn_sm_init_event_list
(
 rpc_cn_sm_ctlblk_t      *sm
)
{
    /*
     * Set up the event list so that it's empty. This means the state
     * must be set to "empty" and the head and tail indices must be
     * zeroed.
     */
    sm->event_list_state = RPC_C_CN_SM_EVENT_LIST_EMPTY;
    sm->event_list_hindex = 0;
    sm->event_list_tindex = 0;
}


/*
**++
**
**  ROUTINE NAME:       rpc__cn_sm_eval_event
**
**  SCOPE:              PRIVATE - declared in cnsm.h
**
**  DESCRIPTION:
**
**  This routine will be used to evaluate an event for the specified
**  state machine. It handles the main work of running a state
**  machine. It will look up either action or state transition for
**  events.  The lookup will return either a state or action.
**  Distinguish between states and actions by numeric range.
**
**  INPUTS:
**
**      event_id        The number of the event to be processed.
**      event_param     The special event related parameter which is
**                      to be passed to the predicate and action
**                      routines for the processing of this event.
**      spc_struct      A special parameter which is to be passed to
**                      the predicate and action routines for the
**                      processing of this event and any subsequent
**                      events which are inserted on the event list.
**
**  INPUTS/OUTPUTS:
**
**      sm              The state machine control block to be used
**                      in processing this event.
**
**  OUTPUTS:            none
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     The status returned by the last action
**                      routine invoked.
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE unsigned32     rpc__cn_sm_eval_event
(
  unsigned32              event_id,
  pointer_t               event_parameter,
  pointer_t               spc_struct,
  rpc_cn_sm_ctlblk_t      *sm
)
{
    rpc_cn_sm_event_entry_t     next_event;
    unsigned8                   action_index;
    boolean                     more_events;
    rpc_cn_sm_state_entry_t     *sm_entry;
    int                         state_tbl_sz = 0;
    int                         state_tbl_entry_sz = 0;
    RPC_SM_TBL_e_t              state_tbl_type = 0;
    int                         state_entry_index = 0;
    char                        *state_tbl_name = NULL;
    int                         act_tbl_sz = 0;
    RPC_ACTION_TBL_e_t          act_tbl_type = 0;
    char                        *act_tbl_name = NULL;

    /*
     * Initialize action status to ok.  This allows state transitions
     * which do not invoke action routines to signal normal completion.
     */
    sm->action_status = rpc_s_ok;

    /*
     * Set up the first event to be evaluated using the input args.
     */
    next_event.event_id = event_id;
    next_event.event_param = event_parameter;

    /*
     * Process the first event and any events which get added.
     */
    more_events = true;
    while (more_events)
    {
        /*
         * Pick up the state table entry to the current state. The
	 * value in the state table is going to be either a next
	 * state or an action.  If it is an action, we take that
	 * action and within the action routine, update sm->cur_state.
         *
	 * In cases where there is no action but there is a next state,
	 * we can just store the next state.  We distinguish between
	 * actions and state by their numeric value.
         *
         * Next states are
	 * always greater than or equal to rpc_c_cn_statebase.
	 * Since states are also used to access an array entry, we need
	 * to subtract the value we added in order to distinguish it
	 * from an action routine, before we can access the array.
         */
        /*
         * Perform range check on sm->cur_state. First, determine the
         * size of the state table referenced by sm->state_tbl.
         */
        state_tbl_sz = rpc__sm_table_entry_count(
                           sm,
                           &state_tbl_type,
                           &state_tbl_entry_sz,
                           &state_tbl_name);
        if (sm->cur_state - RPC_C_CN_STATEBASE < 0 ||
            sm->cur_state - RPC_C_CN_STATEBASE >= state_tbl_sz)
        {
            return rpc_s_sm_invalid_state;
        }

        /*
         * Index the State machine table (client or server):
         *     grp_sm;
         *     assoc_sm;
         *     call_sm;
         */
        sm_entry = sm->state_tbl[(sm->cur_state -
			RPC_C_CN_STATEBASE)];
        /*
         * Look up the index of the action routine using the current
         * state and the id of the event being processed.
         */

        /* Range check "sm_entry" (rpc_cn_sm_state_entry_p_t) */
        state_entry_index = next_event.event_id - RPC_C_CN_STATEBASE;
        if (state_entry_index < 0 || state_entry_index >= state_tbl_entry_sz)
        {
            return rpc_s_sm_invalid_state;
        }
        action_index = sm_entry[state_entry_index].action;

        /*
         * If there is no action to take, just transition to the next
	 * state which is the value returned from  the state table
	 * lookup (sm_entry).
         */
	if (action_index >= RPC_C_CN_STATEBASE)
	{
            sm->cur_state = action_index;
            if (rpc__cn_sm_debug())
            {
                rpc__printf("cnsm: S (t=%-7s:l=%d) -> (s[%s])\n",
                  state_tbl_name,
                  state_tbl_sz,
                  rpc__state_table_lookup(state_tbl_type, action_index));
	    }
	}
	else
	{
            /*
             * Must perform range check on action_index. First, determine the
             * size of the state table referenced by sm->action_tbl.
             */
            act_tbl_sz = rpc__action_table_entry_count(
                             sm,
                             &act_tbl_type,
                             &act_tbl_name);
            if (action_index >= act_tbl_sz)
            {
                return rpc_s_sm_invalid_state;
            }

            if (rpc__cn_sm_debug())
            {
                rpc__printf("cnsm: t=%ld tid=%x A (t=%-7s:l=%d) 	-> (a[%-20s])\n",
                  time(NULL),
                  pthread_self(),
                  act_tbl_name,
                  act_tbl_sz,
                  rpc__action_table_lookup(act_tbl_type, action_index));
            }
            /*
             * Call the action routine.  The spc_struct and event_param
	     * and sm will be passed to the action routine.  Note
	     * that sm is changed within the action routines.  Each
	     * action routine will update sm->cur_state.
             */
	    sm->cur_event = next_event.event_id;
            sm->action_status = sm->action_tbl[action_index](
                                      spc_struct, next_event.event_param, (pointer_t) sm);
         }

        /*
         * Get the next event, if any, off the event list in the
         * state machine control block.  RPC_CN_SM_GET_NEXT_EVENT
	 * will set more_events to true or false depending on
	 * whether there are more events to process.
         */
        RPC_CN_SM_GET_NEXT_EVENT (sm, &next_event, more_events);
    }
    return (sm->action_status);
}


/*
**++
**
**  ROUTINE NAME:       rpc__cn_sm_insert_event
**
**  SCOPE:              PRIVATE - declared in cnsm.h
**
**  DESCRIPTION:
**
**  This routine inserts a new event entry on the state machine
**  control block event list. If the list is full the event can't be
**  inserted and false will be returned.
**
**  INPUTS:
**
**      event           The event entry being inserted.
**
**  INPUTS/OUTPUTS:
**
**      sm              The state machine control block containing
**                      the event list.
**
**  OUTPUTS:
**
**  IMPLICIT INPUTS:    none
**
**  IMPLICIT OUTPUTS:   none
**
**  FUNCTION VALUE:     none
**
**  SIDE EFFECTS:       none
**
**--
**/

PRIVATE void rpc__cn_sm_insert_event
(
  rpc_cn_sm_event_entry_p_t       event,
  rpc_cn_sm_ctlblk_t              *sm
)
{
#ifdef DEBUG
    /*
     * Check whether the event list is full. This condition occurs
     * when the head and tail indices are equal and the state
     * indicates the list is not empty.
     */
    if ((sm->event_list_hindex == sm->event_list_tindex) &&
        (sm->event_list_state != RPC_C_CN_SM_EVENT_LIST_EMPTY))
    {
	/*
	 * rpc_m_eventlist_full
	 * "(%s) Event list full"
	 */
	RPC_DCE_SVC_PRINTF ((
	    DCE_SVC(RPC__SVC_HANDLE, "%s"),
	    rpc_svc_cn_state,
	    svc_c_sev_fatal | svc_c_action_abort,
	    rpc_m_eventlist_full,
	    "rpc__cn_sm_insert_event" ));
    }
#endif

    /*
     * There's room on the event list. Add the new entry to the tail
     * of the list.
     */
    sm->event_list[sm->event_list_tindex].event_id = event->event_id;
    sm->event_list[sm->event_list_tindex].event_param = event->event_param;

    /*
     * Add the event to the event list by incrementing the tail
     * index and checking for wraparound. Also set the state of the
     * event list to non-empty.
     */
    sm->event_list_tindex = (sm->event_list_tindex + 1) &
                            (RPC_C_CN_SM_EVENT_LIST_MAX_ENTRIES - 1);
    sm->event_list_state = ~RPC_C_CN_SM_EVENT_LIST_EMPTY;
}
