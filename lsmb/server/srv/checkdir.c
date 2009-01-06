#include "includes.h"

NTSTATUS
SmbProcessCheckDirectory(
    PSMB_CONNECTION pSmbRequest
    )
{
    NTSTATUS ntStatus = 0;
    HANDLE hTreeObject = (HANDLE)NULL;

    ntStatus = UnmarshallCheckDirectoryRequest(pSmbRequest);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvCheckDirectory(
                        hTreeObject
                        );
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = MarshallCheckDirectoryResponse(pSmbRequest);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SmbSendReply(pSmbRequest);
    BAIL_ON_NT_STATUS(ntStatus);

error:

    return (ntStatus);
}

NTSTATUS
SrvCheckDirectory(
    HANDLE hTreeObject
    )
{
    NTSTATUS ntStatus = 0;

    return (ntStatus);
}

NTSTATUS
UnmarshallCheckDirectoryRequest(
    PSMB_CONNECTION pSmbRequest
    )
{
    NTSTATUS ntStatus = 0;

    return (ntStatus);
}


NTSTATUS
MarshallCheckDirectoryResponse(
    PSMB_CONNECTION pSmbRequest
    )
{
    NTSTATUS ntStatus = 0;

    return (ntStatus);

}
