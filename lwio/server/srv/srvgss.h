#ifndef __SRV_GSS_H__
#define __SRV_GSS_H__

NTSTATUS
SrvGssInit(
    VOID
    );

NTSTATUS
SrvGssCreate(
    PSRV_GSS_CONTEXT* ppGssContext,
    PBYTE*            ppSessionKey,
    PULONG            pulSessionKeyLength
    );

NTSTATUS
SrvGssNegotiate(
    PSRV_GSS_CONTEXT pGssContext,
    PBYTE            pSecurityInputBlob,
    ULONG            ulSecurityInputBlobLen,
    PBYTE*           ppSecurityOutputBlob,
    ULONG*           pulSecurityOutputBloblen
    );

VOID
SrvGssFree(
    PSRV_GSS_CONTEXT pGssContext
    );

NTSTATUS
SrvGssShutdown(
    VOID
    );

#endif /* __SRV_GSS_H__ */
