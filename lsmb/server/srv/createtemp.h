
NTSTATUS
SmbProcessCreateTemporaryFile(
    PSMB_CONNECTION pSmbRequest
    );

NTSTATUS
SrvCreateTemporaryFile(
    HANDLE hTreeObject
    );

NTSTATUS
UnmarshallCreateTemporaryFileRequest(
    PSMB_CONNECTION pSmbRequest
    );

NTSTATUS
MarshallCreateTemporaryFileResponse(
    PSMB_CONNECTION pSmbRequest
    );

