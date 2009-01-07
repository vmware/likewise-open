#ifndef __SMBKRB5_H__
#define __SMBKRB5_H__

DWORD
SMBKrb5Init(
    PCSTR pszHostname,
    PCSTR pszDomain
    );

DWORD
SMBKrb5SetDefaultCachePath(
    PCSTR pszCachePath,
    PSTR* ppszOrigCachePath
    );

DWORD
SMBGSSContextBuild(
    PCSTR     pszServerName,
    PHANDLE   phSMBGSSContext
    );

BOOLEAN
SMBGSSContextNegotiateComplete(
    HANDLE hSMBGSSContext
    );

DWORD
SMBGSSContextNegotiate(
    HANDLE hSMBGSSContext,
    PBYTE  pSecurityInputBlob,
    DWORD  dwSecurityInputBlobLength,
    PBYTE* ppSecurityBlob,
    PDWORD pdwSecurityBlobLength
    );

DWORD
SMBGSSContextGetSessionKey(
    HANDLE hSMBGSSContext,
    PBYTE* ppSessionKey,
    PDWORD pdwSessionKeyLength
    );

VOID
SMBGSSContextFree(
    HANDLE hSMBGSSContext
    );

DWORD
SMBKrb5Shutdown(
    VOID
    );

#endif /* __SMBKRB5_H__ */
