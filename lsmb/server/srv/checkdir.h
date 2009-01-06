
NTSTATUS
SmbProcessCheckDirectory(
    PSMB_CONNECTION pSmbRequest
    );

NTSTATUS
SrvCheckDirectory(
    HANDLE hTreeObject
    );

NTSTATUS
UnmarshallCheckDirectoryRequest(
    PSMB_CONNECTION pSmbRequest
    );

NTSTATUS
MarshallCheckDirectoryResponse(
    PSMB_CONNECTION pSmbRequest
    );

