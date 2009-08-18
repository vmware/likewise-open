#ifndef _WINREG_SRV_H_
#define _WINREG_SRV_H_


DWORD
WinRegRegisterForRPC(
    PSTR pszServiceName,
    rpc_binding_vector_p_t* ppServerBinding
    );

DWORD
WinRegUnregisterForRPC(
    rpc_binding_vector_p_t pServerBinding
    );


#endif /* _WINREG_SRV_H_ */
