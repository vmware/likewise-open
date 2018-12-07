#include <commonp.h>    /* Common declarations for all RPC runtime */
#include <com.h>        /* Common communications services */
#include <comprot.h>    /* Common protocol services */
#include <cnp.h>        /* NCA Connection private declarations */
#include <cnid.h>       /* NCA Connection local ID service */
#include <cnrcvr.h>     /* NCA Connection receiver thread */
#include <cnnet.h>      /* NCA Connection network service */
#include <cnpkt.h>      /* NCA Connection packet encoding */
#include <cnsm.h>       /* NCA Connection state machine service */
#include <cnassm.h>     /* NCA Connection association state machine */
#include <cnasgsm.h>    /* NCA Connection association group state machine */
#include <comauth.h>    /* Externals for Auth. Services sub-component   */
#include <cncall.h>     /* NCA connection call service */
#include <cnclsm.h>
#include <cnassoc.h>
#include <cn.h>
#include <cninline.h>

/*
 * This file contains macros converted to functions. This was done
 * for readability and debugability of this library. 
 *
 * Some functions still retain a "thin macro wrapper" to pass by
 * reference values which were set within the original macro
 * implementation. As functions are pass by value, a pointer
 * must be passed to change passed in values.
 *
 */

/***********************************************************************/
/* Moved from cnasgsm.h */


/***********************************************************************/
/*
 * R P C _ C N _ A S S O C _ G R P _ E V A L _ E V E N T
 *
 * This macro will be called when association group events are detected.
 */
void _RPC_CN_ASSOC_GRP_EVAL_EVENT(
    rpc_cn_assoc_grp_t  *assoc_grp,
    unsigned32 event_id,
    rpc_cn_assoc_p_t event_param,
    unsigned32 *st)
{
    RPC_CN_ASSOC_GRP_SM_TRC (assoc_grp, event_id);
    *st = rpc__cn_sm_eval_event (event_id,
                                (pointer_t) event_param,
                                (pointer_t) assoc_grp,
                                &assoc_grp->grp_state);
    if (assoc_grp->grp_state.cur_state == RPC_C_ASSOC_GRP_CLOSED)
    {
        rpc__cn_assoc_grp_dealloc (assoc_grp->grp_id);
    }
    RPC_CN_ASSOC_GRP_SM_TRC_STATE (assoc_grp);
}


/***********************************************************************/
/*
 * R P C _ C N _ A S S O C _ G R P _ I N S E R T _ E V E N T
 *
 * This macro will be called when an event is generated inside an
 * action routine of the association group state machine.
 */
void _RPC_CN_ASSOC_GRP_INSERT_EVENT(
    rpc_cn_assoc_grp_t  *assoc_grp,
    rpc_cn_sm_event_entry_p_t event)
{
    RPC_DBG_PRINTF (rpc_e_dbg_cn_state, RPC_C_CN_DBG_ASSOC_GRP_SM_TRACE,
                    ("STATE INSERT EVENT "));
    RPC_CN_ASSOC_GRP_SM_TRC (assoc_grp, event->event_id);
    rpc__cn_sm_insert_event (event,
                             &assoc_grp->grp_state);
}

/***********************************************************************/
/* Moved from cnassm.h */


/***********************************************************************/
/*
 * R P C _ C N _ A S S O C _ C H E C K _ S T
 *
 * This macro will check the given status. If not rpc_s_ok a local
 * error event will be inserted into the association event list.
 */
unsigned32 
_RPC_CN_ASSOC_CHECK_ST(
    rpc_cn_assoc_t *assoc,
    unsigned st)
{
    rpc_cn_sm_event_entry_t _event;

    if (st != rpc_s_ok)
    {
        (assoc)->assoc_local_status = st;
        _event.event_id = RPC_C_ASSOC_LOCAL_ERROR;
        RPC_CN_ASSOC_INSERT_EVENT (assoc, &_event);
        return (st);
    }
    return rpc_s_ok;
}


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
void 
_RPC_CN_ASSOC_EVAL_NETWORK_EVENT(
    rpc_cn_assoc_p_t assoc,
    unsigned8 event_id,
    rpc_cn_fragbuf_p_t fragbuf,
    unsigned32 *st)
{
    RPC_CN_ASSOC_SM_TRC (assoc, event_id);
    if (fragbuf)
    {
        (fragbuf)->freebuf = 0;
    }
    *st = rpc__cn_sm_eval_event ((event_id),
                                (pointer_t) (fragbuf),
                                (pointer_t) (assoc),
                                &((assoc)->assoc_state));
    assoc->assoc_flags &= ~RPC_C_CN_ASSOC_SCANNED;
    if ((fragbuf) != NULL)
    {
        (fragbuf)->freebuf = 1;
        (*(fragbuf)->fragbuf_dealloc)((fragbuf));
    }
    RPC_CN_ASSOC_SM_TRC_STATE (assoc);
}


/***********************************************************************/
/*
 * R P C _ C N _ A S S O C _ E V A L _ U S E R _ E V E N T
 *
 * This macro will be called when user level events are detected. If
 * the association status is bad then don't evaluate the user event.
 * The "scanned" bit in the association is turned off.
 */
void 
_RPC_CN_ASSOC_EVAL_USER_EVENT(
    rpc_cn_assoc_p_t assoc,
    unsigned8 event_id,
    rpc_cn_assoc_sm_work_t *event_param,
    unsigned32 *st)
{
    RPC_CN_ASSOC_SM_TRC (assoc, event_id);
    *st = assoc->assoc_status;
    if (*st == rpc_s_ok)
    {
        *st = rpc__cn_sm_eval_event ((event_id),
                                    (pointer_t) (event_param),
                                    (pointer_t) (assoc),
                                    &((assoc)->assoc_state));
        assoc->assoc_flags &= ~RPC_C_CN_ASSOC_SCANNED;
    }
    RPC_CN_ASSOC_SM_TRC_STATE (assoc);
}


/***********************************************************************/
/*
 * R P C _ C N _ A S S O C _ I N S E R T _ E V E N T
 *
 * This macro will be called when an event is generated inside an
 * action routine of the association state machine.
 */
void _RPC_CN_ASSOC_INSERT_EVENT(
    rpc_cn_assoc_t *assoc,
    rpc_cn_sm_event_entry_p_t event)
{
    RPC_DBG_PRINTF (rpc_e_dbg_cn_state, RPC_C_CN_DBG_ASSOC_SM_TRACE,
                    ("STATE INSERT EVENT "));
    RPC_CN_ASSOC_SM_TRC (assoc, event->event_id);
    rpc__cn_sm_insert_event (event,
                             &assoc->assoc_state);
}

/***********************************************************************/
/* Moved from cnassoc.h */

#ifdef RPC_CN_DEBUG_REFCNT
void RPC_CN_ASSOC_ACB_INC_REF(rpc_cn_assoc_p_t assoc)
{
    (assoc)->assoc_acb_ref_count++;
    RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                    ("(RPC_CN_ASSOC_ACB_INC_REF) assoc->%x new refcnt->%d\n",
                     assoc, assoc->assoc_acb_ref_count));
}
#else
void RPC_CN_ASSOC_ACB_INC_REF(rpc_cn_assoc_p_t assoc)
{
    assoc->assoc_acb_ref_count++;
}
#endif /* RPC_CN_DEBUG_REFCNT */

/*
 * R P C _ C N _ A S S O C _ A C B _ D E C _ R E F
 */

#ifdef RPC_CN_DEBUG_REFCNT
void RPC_CN_ASSOC_ACB_DEC_REF(rpc_cn_assoc_t *assoc)
{
    (assoc)->assoc_acb_ref_count--;
    RPC_DBG_PRINTF (rpc_e_dbg_general, RPC_C_CN_DBG_GENERAL,
                    ("(RPC_CN_ASSOC_ACB_DEC_REF) assoc->%x new refcnt->%d\n",
                     assoc, assoc->assoc_acb_ref_count));
}
#else
void RPC_CN_ASSOC_ACB_DEC_REF(rpc_cn_assoc_t *assoc)
{
    (assoc)->assoc_acb_ref_count--;
}
#endif /* RPC_CN_DEBUG_REFCNT */

/*
 * R P C _ C N _ A S S O C _ G R P
 */

rpc_cn_assoc_grp_p_t
_RPC_CN_ASSOC_GRP(
    rpc_cn_local_id_p_t grp_id)
{
    return RPC_CN_LOCAL_ID_VALID (*grp_id) ?
        &rpc_g_cn_assoc_grp_tbl.assoc_grp_vector[grp_id->parts.id_index] : NULL;
}

/*
 * R P C _ C N _ A S S O C _ C A L L
 */

rpc_cn_call_rep_p_t
RPC_CN_ASSOC_CALL(rpc_cn_assoc_p_t assoc) 
{
    return assoc->call_rep;
}

/*
 * R P C _ C N _ A S S O C _ M A X _ X M I T _ F R A G
 */

unsigned16
RPC_CN_ASSOC_MAX_XMIT_FRAG(rpc_cn_assoc_p_t assoc)
{
    return assoc->assoc_max_xmit_frag;
}

/*
 * R P C _ C N _ A S S O C _ M A X _ R E C V _ F R A G
 */

unsigned16
RPC_CN_ASSOC_MAX_RECV_FRAG(rpc_cn_assoc_p_t assoc)
{
    return assoc->assoc_max_recv_frag;
}

/*
 * R P C _ C N _ A S S O C _ C O N T E X T _ I D
 */
 
unsigned32
RPC_CN_ASSOC_CONTEXT_ID(rpc_cn_assoc_p_t assoc)
{
    return assoc->assoc_pres_context_id;
}

/*
 * R P C _ C N _ A S S O C _ C O N T E X T _ I D _ P T R
 */
unsigned32 *
RPC_CN_ASSOC_CONTEXT_ID_PTR(rpc_cn_assoc_p_t assoc)
{
    return &assoc->assoc_pres_context_id;
}

/*
 * R P C _ C N _ A S S O C _ N D R _ F O R M A T
 */

ndr_format_p_t
RPC_CN_ASSOC_NDR_FORMAT(rpc_cn_assoc_p_t assoc)
{
    return &assoc->assoc_remote_ndr_format;
}

/*
 * R P C _ C N _ A S S O C _ S E C U R I T Y
 */

rpc_cn_assoc_sec_context_p_t
RPC_CN_ASSOC_SECURITY(rpc_cn_assoc_p_t assoc)
{
    return &assoc->security;
}

/*
 * R P C _ C N _ A S S O C _ W A K E U P
 */

void
RPC_CN_ASSOC_WAKEUP(rpc_cn_assoc_p_t assoc)
{
    rpc__cn_assoc_queue_dummy_frag(assoc);
}

/*
 * R P C _ C N _ A S S O C _ C A N C E L _ A N D _ W A K E U P
 */

void
RPC_CN_ASSOC_CANCEL_AND_WAKEUP(rpc_cn_assoc_p_t assoc)
{
    RPC_CALL_LOCK ((rpc_call_rep_t *) assoc->call_rep);
    rpc__cthread_cancel ((rpc_call_rep_t *) assoc->call_rep);
    rpc__cn_assoc_queue_dummy_frag(assoc);
    RPC_CALL_UNLOCK ((rpc_call_rep_t *) assoc->call_rep);
}


/***********************************************************************/
/* Moved from cnclsm.h */

/*
 * R P C _ C N _ P O S T _ C A L L _ S M _ E V E N T
 *
 * Posts an event to either a server or client call state machine.
 *
 * Sample usage:
 *
 *
 *    rpc_cn_assoc_p_t          assoc;
 *    unsigned8                 event;
 *    rpc_cn_fragbuf_p_t        fragbuf;
 *    unsigned32                st;
 *
 * RPC_CN_POST_CALL_SM_EVENT (assoc, event, fragbuf, st);
 */
void 
_RPC_CN_POST_CALL_SM_EVENT(
    rpc_cn_assoc_p_t assoc,
    unsigned8 event_id, 
    rpc_cn_fragbuf_p_t fragbuf, 
    unsigned32 *st)
{
    rpc_cn_call_rep_p_t crep;

    crep = RPC_CN_ASSOC_CALL (assoc);
    if (crep != NULL)
    {
        if (RPC_CN_PKT_CALL_ID ((rpc_cn_packet_p_t) RPC_CN_CREP_SEND_HDR (crep))
            ==
            RPC_CN_PKT_CALL_ID (RPC_CN_FRAGBUF_PKT_HDR (fragbuf)))
        {
            RPC_CN_CALL_SM_TRC (crep, event_id, (RPC_CN_PKT_CALL_ID ((rpc_cn_packet_p_t) RPC_CN_CREP_SEND_HDR (crep))));
            *st = rpc__cn_sm_eval_event (event_id, (pointer_t) fragbuf,
                 (pointer_t) crep, &(crep->call_state));
            RPC_CN_CALL_SM_TRC_STATE (crep, (RPC_CN_PKT_CALL_ID ((rpc_cn_packet_p_t) RPC_CN_CREP_SEND_HDR (crep))));
        }
        else
        {
            (*fragbuf->fragbuf_dealloc)(fragbuf);
        }
    }
    else
    {
        (*fragbuf->fragbuf_dealloc)(fragbuf);
    }
}

/***********************************************************************/
/*
 * R P C _ C N _ P O S T _ F I R S T _ C A L L _ S M _ E V E N T
 *
 * Posts the first [server] event to the call state machine.
 * This differs from the normal post call sm event because the
 * callid field has not been initialized upon the first server
 * event.
 *
 * Sample usage:
 *
 *    rpc_cn_call_rep_p_t       crep;
 *    rpc_cn_assoc_p_t          assoc;
 *    unsigned8                 event;
 *    rpc_cn_fragbuf_p_t        fragbuf;
 *    unsigned32                st;
 *
 * RPC_CN_POST_FIRST_CALL_SM_EVENT (crep, assoc, event, fragbuf, st);
 */
void _RPC_CN_POST_FIRST_CALL_SM_EVENT(
    rpc_cn_call_rep_p_t crep,
    rpc_cn_assoc_p_t assoc,
    unsigned8 event_id, 
    rpc_cn_fragbuf_p_t fragbuf, 
    unsigned32 *st)
{
    rpc__cn_assoc_alloc (assoc, st);
    if (*st == rpc_s_ok)
    {
        RPC_CN_CALL_SM_TRC (crep, event_id, (RPC_CN_PKT_CALL_ID (RPC_CN_FRAGBUF_PKT_HDR(fragbuf))));
        *st = rpc__cn_sm_eval_event (event_id, (pointer_t)fragbuf,
             (pointer_t)crep, &crep->call_state);
        RPC_CN_CALL_SM_TRC_STATE (crep, (RPC_CN_PKT_CALL_ID ((rpc_cn_packet_p_t) RPC_CN_CREP_SEND_HDR (crep))));
    }
}

/*
 * R P C _ C N _ C A L L _ E V A L _ E V E N T
 *
 * Posts an event from either a client caller or server call
 * executor thread to the call state machine.
 *
 * Sample usage:
 *
 *    rpc_cn_call_rep_p_t       crep;
 *    unsigned8                 event;
 *    pointer_t                 spc_struct;
 *    unsigned32                st;
 *
 * RPC_CN_CALL_EVAL_EVENT (event_id, spc_struct, crep, st);
 */
void _RPC_CN_CALL_EVAL_EVENT(
    unsigned8 event_id, 
    pointer_t spc_struct,
    rpc_cn_call_rep_p_t crep,
    unsigned32 *st)
{
    RPC_CN_CALL_SM_TRC (crep, event_id, (RPC_CN_PKT_CALL_ID ((rpc_cn_packet_p_t) RPC_CN_CREP_SEND_HDR (crep))));
    *st = rpc__cn_sm_eval_event (event_id, spc_struct,
                                (pointer_t)crep,
                                &crep->call_state);
    RPC_CN_CALL_SM_TRC_STATE (crep, (RPC_CN_PKT_CALL_ID ((rpc_cn_packet_p_t) RPC_CN_CREP_SEND_HDR (crep))));
}

/***********************************************************************/
/* Moved from cnfbuf.h */

/*
 * R P C _ C N _ F R A G B U F _ P K T _ H D R
 *
 * The unpacked header for a received fragment starts at the used
 * portion of the header overhead area.
 */

rpc_cn_packet_p_t
RPC_CN_FRAGBUF_PKT_HDR(pointer_t fbuf)
{
    return (rpc_cn_packet_p_t) ((rpc_cn_fragbuf_p_t)(fbuf))->data_p;
}

/***********************************************************************/
/*
 * R P C _ C N _ F R A G B U F _ A L L O C
 *
 */

void _RPC_CN_FRAGBUF_ALLOC(
    rpc_cn_fragbuf_p_t *fragbuf,
    unsigned32 size,
    unsigned32 *st)
{
    if (size <= RPC_C_CN_SMALL_FRAG_SIZE)
    {
        *fragbuf = rpc__cn_fragbuf_alloc (false);
    }
    else
    {
        *fragbuf = rpc__cn_fragbuf_alloc (true);
    }
    (*fragbuf)->data_size = size;
    *st = rpc_s_ok;
}

/* Moved from cnnet.h */

/*
 * R P C _ C N _ N E T W O R K _ I O V E C T O R _ T O _ I O V
 */

void
_RPC_CN_NETWORK_IOVECTOR_TO_IOV(
    rpc_iovector_p_t iovector, 
    rpc_socket_iovec_p_t iov,
    unsigned32 *iovcnt, 
    unsigned32 *bytes_to_send)
{
    unsigned8   _num_elts;

    for (_num_elts = 0, *bytes_to_send = 0; _num_elts < iovector->num_elt; _num_elts++)
    {
        iov[_num_elts].iov_base = (byte_p_t) (iovector->elt[_num_elts].data_addr);
        iov[_num_elts].iov_len = (int) (iovector->elt[_num_elts].data_len);
        *bytes_to_send += (unsigned32) iov[_num_elts].iov_len;
    }
    *iovcnt = _num_elts;

}

/*
 * R P C _ C N _ N E T W O R K _ I O V _ A D J U S T
 */

void
_RPC_CN_NETWORK_IOV_ADJUST(
    rpc_socket_iovec_p_t iovp,
    int *iovcnt,
    int cc)
{
    unsigned8   _num_elts;
    size_t _bytes_to_adjust;

    for (_bytes_to_adjust = cc, _num_elts = 0; /*loop forever*/; _num_elts++, iovp++)
    {
        if (iovp->iov_len > _bytes_to_adjust)
        {
            iovp->iov_len -= _bytes_to_adjust;
            iovp->iov_base = (unsigned char *)(iovp->iov_base) + _bytes_to_adjust;
            break;
        }
        _bytes_to_adjust -= iovp->iov_len;
    }
    *iovcnt -= _num_elts;
}

/* Moved from cnp.h */
boolean32
RPC_CN_AUTH_CRED_CHANGED(
    rpc_cn_sec_context_p_t sec,
    unsigned32 *st)
{
    return sec->sec_cn_info->cn_epv->cred_changed(sec, st);
}

void
RPC_CN_AUTH_FMT_CLIENT_REQ(
    rpc_cn_assoc_sec_context_p_t assoc_sec,
    rpc_cn_sec_context_p_t sec,
    pointer_t auth_value,
    unsigned32 *auth_value_len,
    pointer_t last_auth_pos,
    unsigned32 *auth_len_remain,
    boolean retry,
    unsigned32 *st)
{
    sec->sec_cn_info->cn_epv->fmt_client_req(
        assoc_sec,
        sec,
        auth_value,
        auth_value_len,
        last_auth_pos,
        auth_len_remain,
        retry,
        st);
}

void
RPC_CN_AUTH_FMT_SRVR_RESP(
    unsigned32 verify_st,
    rpc_cn_assoc_sec_context_p_t assoc_sec,
    rpc_cn_sec_context_p_t sec,
    pointer_t req_auth_value,
    unsigned16 req_auth_value_len,
    pointer_t auth_value,
    unsigned32 *auth_value_len)
{
    sec->sec_cn_info->cn_epv->fmt_srvr_resp(
        verify_st,
        assoc_sec,
        sec,
        req_auth_value,
        req_auth_value_len,
        auth_value,
        auth_value_len);
}

void
RPC_CN_AUTH_FREE_PROT_INFO(
    rpc_auth_info_p_t info,
    rpc_cn_auth_info_p_t *cn_info)
{
    (*cn_info)->cn_epv->free_prot_info(info, cn_info);
}

void
RPC_CN_AUTH_PRE_CALL(
    rpc_cn_assoc_sec_context_p_t assoc_sec,
    rpc_cn_sec_context_t *sec,
    pointer_t auth_value,
    unsigned32 *auth_value_len,
    unsigned32 *st)
{
    sec->sec_cn_info->cn_epv->pre_call(assoc_sec, sec, auth_value, auth_value_len, st);
}

void
RPC_CN_AUTH_PRE_SEND(
    rpc_cn_assoc_sec_context_p_t assoc_sec,
    rpc_cn_sec_context_p_t sec,
    rpc_socket_iovec_p_t iovp,
    int iovlen,
    rpc_socket_iovec_p_t out_iov,
    unsigned32 *st)
{
    sec->sec_cn_info->cn_epv->pre_send(assoc_sec, sec, iovp, iovlen, out_iov, st);
}

void
RPC_CN_AUTH_VFY_CLIENT_REQ(
    rpc_cn_assoc_sec_context_p_t assoc_sec, 
    rpc_cn_sec_context_p_t sec, 
    pointer_t auth_value, 
    unsigned32 auth_value_len, 
    boolean old_client, 
    unsigned32 *st)
{
    sec->sec_cn_info->cn_epv->vfy_client_req(assoc_sec, sec, auth_value, auth_value_len, old_client, st);
}

void
RPC_CN_AUTH_VFY_SRVR_RESP(
    rpc_cn_assoc_sec_context_p_t assoc_sec, 
    rpc_cn_sec_context_p_t sec, 
    pointer_t auth_value, 
    unsigned32 auth_value_len, 
    unsigned32 *st)
{
    (*(sec)->sec_cn_info->cn_epv->vfy_srvr_resp)(assoc_sec, sec, auth_value, auth_value_len, st);
}

void
RPC_CN_AUTH_ADD_REFERENCE(rpc_auth_info_p_t info)
{
    rpc__auth_info_reference(info);
}

void
RPC_CN_AUTH_RELEASE_REFERENCE(rpc_auth_info_p_t *info)
{
    rpc__auth_info_release(info);
}

unsigned32
RPC_CN_AUTH_CVT_ID_API_TO_WIRE(rpc_authn_protocol_id_t id, unsigned32 *st)
{
   return rpc__auth_cvt_id_api_to_wire(id,st);
}

rpc_authn_protocol_id_t 
RPC_CN_AUTH_CVT_ID_WIRE_TO_API(unsigned32 id, unsigned32 *st)
{
    return rpc__auth_cvt_id_wire_to_api(id,st);
}

boolean32
RPC_CN_AUTH_INQ_SUPPORTED(rpc_authn_protocol_id_t id)
{
    return rpc__auth_inq_supported(id);
}

/*
 * R P C _ C N _ A L I G N _ P T R
 *
 * RPC Pointer Alignment macro
 *
 * Casting to (unsigned long) is needed because bitwise operations are not
 * allowed with pointers as an operand.
 *
 * NOTE: Assumption sizeof(unsigned long) = sizeof(unsigned8 *).  This
 *       assumption may not be correct for all machines.
 */

/* ??? */

#if 0 /* Original macro */
/*
 * This does not work properly when "boundary" is 4 byte variable, as
 * the generated mask is 4 bytes, and will truncate the upper 4 bytes
 * of a 64 bit pointer. This is fixed in the below function.
 */
#define RPC_CN_ALIGN_PTR(ptr, boundary) \
    ((size_t) ((unsigned8 *)(ptr) + ((boundary)-1)) & ~((boundary)-1))
#endif /* if 0 */

size_t
RPC_CN_ALIGN_PTR(unsigned8 *ptr, unsigned32 align)
{
   size_t boundary = align;
   return (size_t) (ptr + (boundary-1)) & ~(boundary-1);
}

unsigned32
RPC_CN_AUTH_REQUIRED(rpc_auth_info_p_t info)
{
    return info != NULL && info->authn_protocol != rpc_c_authn_none;
}

unsigned32
RPC_CN_PKT_AUTH_REQUIRED(rpc_auth_info_p_t info)
{ 
    return info != NULL &&
           info->authn_level != rpc_c_protect_level_none &&
           info->authn_level != rpc_c_protect_level_connect;
}

rpc_cn_auth_epv_t *RPC_CN_AUTH_PROT_EPV(rpc_authn_protocol_id_t prot)
{
    return (rpc_cn_auth_epv_t *)(rpc__auth_rpc_prot_epv(prot, RPC_C_PROTOCOL_ID_NCACN));
}
