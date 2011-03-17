#ifndef _LIBMAIN_H_
#define _LIBMAIN_H_


DWORD
SrvSvcSetServerDefaults(
    VOID
    );

DWORD
SrvSvcReadConfigSettings(
    VOID
    );

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
