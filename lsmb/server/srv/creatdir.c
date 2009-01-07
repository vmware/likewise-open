#include "includes.h"

NTSTATUS
SmbProcessCreateDirectory(
    PSMB_CONNECTION pSmbRequest
    )
{
    NTSTATUS ntStatus = 0;
    HANDLE   hTreeObject = (HANDLE)NULL;

    ntStatus = UnmarshallCreateDirectoryRequest(pSmbRequest);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvCreateDirectory(
                        hTreeObject
                        );
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = MarshallCreateDirectoryResponse(pSmbRequest);
    BAIL_ON_NT_STATUS(ntStatus);


    ntStatus = SmbSendReply(pSmbRequest);
    BAIL_ON_NT_STATUS(ntStatus);

error:

    return (ntStatus);
}

NTSTATUS
SrvCreateDirectory(
    HANDLE hTreeObject
    )
{
    NTSTATUS ntStatus = 0;

    return ntStatus;
}


NTSTATUS
UnmarshallCreateDirectoryRequest(
    PSMB_CONNECTION pSmbRequest
    )
{
    NTSTATUS ntStatus = 0;

    return (ntStatus);
}


NTSTATUS
MarshallCreateDirectoryResponse(
    PSMB_CONNECTION pSmbRequest
    )
{
    NTSTATUS ntStatus = 0;

    return (ntStatus);
}
