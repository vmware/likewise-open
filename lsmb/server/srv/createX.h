
NTSTATUS
SmbProcessCreateAndX(
    PSMB_CONNECTION pSmbRequest
    );

NTSTATUS
SrvCreateFile(
    HANDLE hTreeObject
    );

NTSTATUS
UnmarshallCreateAndXRequest(
    PSMB_CONNECTION pSmbRequest
    );

NTSTATUS
MarshallCreateAndXResponse(
    PSMB_CONNECTION pSmbRequest
    );

