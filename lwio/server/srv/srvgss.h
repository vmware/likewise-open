#ifndef __SRV_GSS_H__
#define __SRV_GSS_H__

NTSTATUS
SrvGssCreate(
    PSRV_GSS_CONTEXT* ppGssContext
    );

NTSTATUS
SrvGssNegotiate(
    PSRV_GSS_CONTEXT pGssContext,
    PBYTE            pSessionKey,
    ULONG            ulSessionKeyLength
    );

VOID
SrvGssFree(
    PSRV_GSS_CONTEXT pGssContext
    );

#endif /* __SRV_GSS_H__ */
