#ifndef _SRVSVC_SRV_H_
#define _SRVSVC_SRV_H_


DWORD
SrvSvcRegisterForRPC(
    PSTR pszServiceName,
    rpc_binding_vector_p_t* ppServerBinding
    );

PVOID
SrvSvcListenForRPC(
    PVOID pArg
    );

DWORD
SrvSvcUnregisterForRPC(
    rpc_binding_vector_p_t pServerBinding
    );

BOOLEAN
SrvSvcRpcIsListening(
    VOID
    );

DWORD
SrvSvcRpcStopListening(
    VOID
    );

#endif /* _SRVSVC_SRV_H_ */
