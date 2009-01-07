
NTSTATUS
SmbProcessReadAndX(
    PSMB_CONNECTION pSmbRequest
    );

NTSTATUS
UnmarshallReadAndXRequest(
    PSMB_CONNECTION pSmbRequest,
    PBYTE* ppBuffer,
    PDWORD pdwBytesRead
    );

NTSTATUS
SrvReadFile(
    HANDLE hTreeObject,
    USHORT usFid,
    ULONG ulOffset,
    UCHAR  *pBuffer,
    USHORT MaxCount
    );

NTSTATUS
MarshallReadAndXResponse(
    PSMB_CONNECTION pSmbRequest
    );

