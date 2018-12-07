rpc_cn_auth_epv_t *RPC_CN_AUTH_PROT_EPV(rpc_authn_protocol_id_t prot);

void _RPC_CN_ASSOC_GRP_EVAL_EVENT(
    rpc_cn_assoc_grp_t  *assoc_grp,
    unsigned32 event_id,
    rpc_cn_assoc_p_t event_param,
    unsigned32 *st);

void _RPC_CN_ASSOC_GRP_INSERT_EVENT(
    rpc_cn_assoc_grp_t  *assoc_grp,
    rpc_cn_sm_event_entry_p_t event);

void _RPC_CN_ASSOC_INSERT_EVENT(
    rpc_cn_assoc_t *assoc,
    rpc_cn_sm_event_entry_p_t event);

unsigned32
_RPC_CN_ASSOC_CHECK_ST(
    rpc_cn_assoc_t *assoc,
    unsigned st);

void
_RPC_CN_ASSOC_EVAL_NETWORK_EVENT(
    rpc_cn_assoc_p_t assoc,
    unsigned8 event_id,
    rpc_cn_fragbuf_p_t fragbuf,
    unsigned32 *st);

void
_RPC_CN_ASSOC_EVAL_USER_EVENT(
    rpc_cn_assoc_p_t assoc,
    unsigned8 event_id,
    rpc_cn_assoc_sm_work_t *event_param,
    unsigned32 *st);

void RPC_CN_ASSOC_ACB_INC_REF(rpc_cn_assoc_p_t assoc);

void RPC_CN_ASSOC_ACB_INC_REF(rpc_cn_assoc_p_t assoc);

void RPC_CN_ASSOC_ACB_DEC_REF(rpc_cn_assoc_t *assoc);

void RPC_CN_ASSOC_ACB_DEC_REF(rpc_cn_assoc_t *assoc);

rpc_cn_assoc_grp_p_t
_RPC_CN_ASSOC_GRP(
    rpc_cn_local_id_p_t grp_id);

rpc_cn_call_rep_p_t
RPC_CN_ASSOC_CALL(rpc_cn_assoc_p_t assoc);

unsigned16
RPC_CN_ASSOC_MAX_XMIT_FRAG(rpc_cn_assoc_p_t assoc);

unsigned16
RPC_CN_ASSOC_MAX_RECV_FRAG(rpc_cn_assoc_p_t assoc);

unsigned32
RPC_CN_ASSOC_CONTEXT_ID(rpc_cn_assoc_p_t assoc);

unsigned32 *
RPC_CN_ASSOC_CONTEXT_ID_PTR(rpc_cn_assoc_p_t assoc);

ndr_format_p_t
RPC_CN_ASSOC_NDR_FORMAT(rpc_cn_assoc_p_t assoc);

rpc_cn_assoc_sec_context_p_t
RPC_CN_ASSOC_SECURITY(rpc_cn_assoc_p_t assoc);

void
RPC_CN_ASSOC_WAKEUP(rpc_cn_assoc_p_t assoc);

void
RPC_CN_ASSOC_CANCEL_AND_WAKEUP(rpc_cn_assoc_p_t assoc);

void
_RPC_CN_POST_CALL_SM_EVENT(
    rpc_cn_assoc_p_t assoc,
    unsigned8 event_id, 
    rpc_cn_fragbuf_p_t fragbuf, 
    unsigned32 *st);

void _RPC_CN_POST_FIRST_CALL_SM_EVENT(
    rpc_cn_call_rep_p_t crep,
    rpc_cn_assoc_p_t assoc,
    unsigned8 event_id, 
    rpc_cn_fragbuf_p_t fragbuf, 
    unsigned32 *st);

void _RPC_CN_CALL_EVAL_EVENT(
    unsigned8 event_id, 
    pointer_t spc_struct,
    rpc_cn_call_rep_p_t crep,
    unsigned32 *st);

rpc_cn_packet_p_t
RPC_CN_FRAGBUF_PKT_HDR(pointer_t fbuf);

void _RPC_CN_FRAGBUF_ALLOC(
    rpc_cn_fragbuf_p_t *fragbuf,
    unsigned32 size,
    unsigned32 *st);

void
_RPC_CN_NETWORK_IOVECTOR_TO_IOV(
    rpc_iovector_p_t iovector, 
    rpc_socket_iovec_p_t iov,
    unsigned32 *iovcnt, 
    unsigned32 *bytes_to_send);

void
_RPC_CN_NETWORK_IOV_ADJUST(
    rpc_socket_iovec_p_t iovp,
    int *iovcnt,
    int cc);

boolean32
RPC_CN_AUTH_CRED_CHANGED(
    rpc_cn_sec_context_p_t sec,
    unsigned32 *st);

void
RPC_CN_AUTH_FMT_CLIENT_REQ(
    rpc_cn_assoc_sec_context_p_t assoc_sec,
    rpc_cn_sec_context_p_t sec,
    pointer_t auth_value,
    unsigned32 *auth_value_len,
    pointer_t last_auth_pos,
    unsigned32 *auth_len_remain,
    boolean retry,
    unsigned32 *st);

void
RPC_CN_AUTH_FMT_SRVR_RESP(
    unsigned32 verify_st,
    rpc_cn_assoc_sec_context_p_t assoc_sec,
    rpc_cn_sec_context_p_t sec,
    pointer_t req_auth_value,
    unsigned16 req_auth_value_len,
    pointer_t auth_value,
    unsigned32 *auth_value_len);

void
RPC_CN_AUTH_FREE_PROT_INFO(
    rpc_auth_info_p_t info,
    rpc_cn_auth_info_p_t *cn_info);

void
RPC_CN_AUTH_PRE_CALL(
    rpc_cn_assoc_sec_context_p_t assoc_sec,
    rpc_cn_sec_context_t *sec,
    pointer_t auth_value,
    unsigned32 *auth_value_len,
    unsigned32 *st);

void
RPC_CN_AUTH_PRE_SEND(
    rpc_cn_assoc_sec_context_p_t assoc_sec,
    rpc_cn_sec_context_p_t sec,
    rpc_socket_iovec_p_t iovp,
    int iovlen,
    rpc_socket_iovec_p_t out_iov,
    unsigned32 *st);

void
RPC_CN_AUTH_VFY_CLIENT_REQ(
    rpc_cn_assoc_sec_context_p_t assoc_sec, 
    rpc_cn_sec_context_p_t sec, 
    pointer_t auth_value, 
    unsigned32 auth_value_len, 
    boolean old_client, 
    unsigned32 *st);

void
RPC_CN_AUTH_VFY_SRVR_RESP(
    rpc_cn_assoc_sec_context_p_t assoc_sec, 
    rpc_cn_sec_context_p_t sec, 
    pointer_t auth_value, 
    unsigned32 auth_value_len, 
    unsigned32 *st);

void
RPC_CN_AUTH_ADD_REFERENCE(rpc_auth_info_p_t info);

void
RPC_CN_AUTH_RELEASE_REFERENCE(rpc_auth_info_p_t *info);

unsigned32
RPC_CN_AUTH_CVT_ID_API_TO_WIRE(rpc_authn_protocol_id_t id, unsigned32 *st);

rpc_authn_protocol_id_t 
RPC_CN_AUTH_CVT_ID_WIRE_TO_API(unsigned32 id, unsigned32 *st);

boolean32
RPC_CN_AUTH_INQ_SUPPORTED(rpc_authn_protocol_id_t id);

size_t
RPC_CN_ALIGN_PTR(unsigned8 *ptr, unsigned32 boundary);

unsigned32
RPC_CN_AUTH_REQUIRED(rpc_auth_info_p_t info);

unsigned32
RPC_CN_PKT_AUTH_REQUIRED(rpc_auth_info_p_t info);
