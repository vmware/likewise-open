#ifndef _SRVSVC_SRV_H_
#define _SRVSVC_SRV_H_


DWORD
SrvSvcRegisterForRPC(
    PSTR pszServiceName,
    rpc_binding_vector_p_t* ppServerBinding
    );


DWORD
SrvSvcListenForRPC(
    VOID
    );


DWORD
SrvSvcUnregisterForRPC(
    rpc_binding_vector_p_t pServerBinding
    );


#endif /* _SRVSVC_SRV_H_ */
