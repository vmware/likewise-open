
NTSTATUS
SmbProcessCreateDirectory(
    PSMB_CONNECTION pSmbRequest
    );

NTSTATUS
SrvCreateDirectory(
    HANDLE hTreeObject
    );


NTSTATUS
UnmarshallCreateDirectoryRequest(
    PSMB_CONNECTION pSmbRequest
    );


NTSTATUS
MarshallCreateDirectoryResponse(
    PSMB_CONNECTION pSmbRequest
    );
