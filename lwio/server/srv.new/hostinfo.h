#ifndef __SRV_HOSTINFO_H__
#define __SRV_HOSTINFO_H__

NTSTATUS
SrvAcquireHostInfo(
    PSRV_HOST_INFO  pOrigHostInfo,
    PSRV_HOST_INFO* ppNewHostInfo
    );

VOID
SrvReleaseHostInfo(
    PSRV_HOST_INFO pHostinfo
    );

#endif /* __SRV_HOSTINFO_H__ */
