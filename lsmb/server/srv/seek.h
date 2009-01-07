
NTSTATUS
SmbProcessSeek(
    PSMB_CONNECTION pSmbRequest
    );

NTSTATUS
UnmarshallSeekRequest(
    PSMB_CONNECTION pSmbRequest
    );

NTSTATUS
MarshallSeekResponse(
    PSMB_CONNECTION pSmbRequest
    );

