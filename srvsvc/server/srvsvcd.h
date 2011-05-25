#ifndef _LIBMAIN_H_
#define _LIBMAIN_H_


DWORD
SrvSvcInitSecurity(
    void
    );

DWORD
SrvSvcRpcInitialize(
    VOID
    );

VOID
SrvSvcRpcShutdown(
    VOID
    );

DWORD
SrvSvcDSNotify(
    VOID
    );

VOID
SrvSvcDSShutdown(
    VOID
    );

DWORD
SrvSvcSMNotify(
    VOID
    );



#endif /* _LIBMAIN_H_ */
