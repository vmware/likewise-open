#ifndef __GETSESSKEY_H__
#define __GETSESSKEY_H__

DWORD
RdrGetSessionKey(
    HANDLE hFile,
    PDWORD pdwSessionKeyLength,
    PBYTE* ppSessionKey
    );

#endif /* __GETSESSKEY_H__ */
