#ifndef __WRITEFILE_H__
#define __WRITEFILE_H__

DWORD
RdrWriteFileEx(
    HANDLE hFile,
    DWORD  dwNumBytesToWrite,
    PVOID  pBuffer,
    PDWORD pdwNumBytesWritten
    );

#endif /* __WRITEFILE_H__ */
