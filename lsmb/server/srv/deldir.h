
NTSTATUS
SmbProcessDeleteDirectory(
    PSMB_CONNECTION pSmbRequest
    );

NTSTATUS
SrvDeleteFile(
    HANDLE hTreeObject,
    USHORT usSearchAttributes,
    LPWSTR pszFileName
    );

NTSTATUS
UnmarshallDeleteDirectoryRequest(
    PSMB_CONNECTION pSmbRequest
    );

NTSTATUS
MarshallDeleteDirectoryResponse(
    PSMB_CONNECTION pSmbRequest
    );

