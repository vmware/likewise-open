#ifndef _WKSSVC_SRV_H_
#define _WKSSVC_SRV_H_


DWORD
WksSvcRegisterForRPC(
    PSTR pszServiceName,
    rpc_binding_vector_p_t* ppServerBinding
    );


DWORD
WksSvcListenForRPC(
    VOID
    );


DWORD
WksSvcUnregisterForRPC(
    rpc_binding_vector_p_t pServerBinding
    );


#endif /* _WKSSVC_SRV_H_ */
