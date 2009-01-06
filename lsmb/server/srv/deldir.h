
NTSTATUS
SmbProcessDeleteDirectory(
    PSMB_CONNECTION pSmbRequest
    );

NTSTATUS
SrvDeleteFile(
    HANDLE hTreeObject
    );

NTSTATUS
UnmarshallDeleteDirectoryRequest(
    PSMB_CONNECTION pSmbRequest
    );

NTSTATUS
MarshallDeleteDirectoryResponse(
    PSMB_CONNECTION pSmbRequest
    );

