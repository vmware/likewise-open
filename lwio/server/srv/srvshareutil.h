#ifndef __SRVSHAREUTIL_H__
#define __SRVSHAREUTIL_H__

VOID
SrvFreeShareInfoList(
    DWORD        dwShareInfoLevel,
    PSHARE_INFO* ppShareInfoList,
    DWORD        dwNumShares
    );

VOID
SrvFreeShareInfo(
    DWORD       dwShareInfoLevel,
    PSHARE_INFO pShareInfo
    );

#endif /* __SRVSHAREUTIL_H__ */

