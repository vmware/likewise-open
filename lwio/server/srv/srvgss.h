#ifndef __SRV_GSS_H__
#define __SRV_GSS_H__

NTSTATUS
SrvGssAcquireContext(
    HANDLE  hGssOrig,
    PHANDLE phGssNew
    );

BOOLEAN
SrvGssNegotiateIsComplete(
    HANDLE hGss,
    HANDLE hGssNegotiate
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
