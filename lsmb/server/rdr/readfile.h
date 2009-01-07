#ifndef __READFILE_H__
#define __READFILE_H__

DWORD
RdrReadFileEx(
    HANDLE hFile,
    DWORD  dwBytesToRead,
    PVOID* ppOutBuffer,
    PDWORD pdwBytesRead
    );

#endif /* __READFILE_H__ */
