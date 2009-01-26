#ifndef __SRV_GSS_H__
#define __SRV_GSS_H__

NTSTATUS
SrvGssAcquireContext(
    PSRV_HOST_INFO pHostinfo,
    HANDLE         hGssOrig,
    PHANDLE        phGssNew
    );

BOOLEAN
SrvGssNegotiateIsComplete(
    HANDLE hGss,
    HANDLE hGssNegotiate
    );

NTSTATUS
SrvGssGetSessionKey(
    HANDLE hGss,
    HANDLE hGssNegotiate,
    PBYTE* ppSessionKey,
    PULONG pulSessionKeyLength
    );

NTSTATUS
SrvGssBeginNegotiate(
    HANDLE  hGss,
    PHANDLE phGssNegotiate
    );

NTSTATUS
SrvGssNegotiate(
    HANDLE  hGss,
    HANDLE  hGssResume,
    PBYTE   pSecurityInputBlob,
    ULONG   ulSecurityInputBlobLen,
    PBYTE*  ppSecurityOutputBlob,
    ULONG*  pulSecurityOutputBloblen
    );

VOID
SrvGssEndNegotiate(
    HANDLE hGss,
    HANDLE hGssNegotiate
    );

VOID
SrvGssReleaseContext(
    HANDLE hGss
    );

#endif /* __SRV_GSS_H__ */
