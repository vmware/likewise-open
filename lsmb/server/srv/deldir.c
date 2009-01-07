#include "includes.h"

NTSTATUS
SmbProcessDeleteDirectory(
    PSMB_CONNECTION pSmbRequest
    )
{
    NTSTATUS ntStatus = 0;
    HANDLE hTreeObject = (HANDLE)NULL;

    ntStatus = UnmarshallDeleteDirectoryRequest(pSmbRequest);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvDeleteFile(
                        hTreeObject,
                        0,
                        NULL
                        );
    BAIL_ON_NT_STATUS(ntStatus);


    ntStatus = MarshallDeleteDirectoryResponse(pSmbRequest);
    BAIL_ON_NT_STATUS(ntStatus);


    ntStatus = SmbSendReply(pSmbRequest);
    BAIL_ON_NT_STATUS(ntStatus);

error:

    return (ntStatus);
}

NTSTATUS
SrvDeleteFile(
    HANDLE hTreeObject,
    USHORT usSearchAttributes,
    LPWSTR pszFileName
    )
{
    NTSTATUS ntStatus = 0;

    return ntStatus;
}


NTSTATUS
UnmarshallDeleteDirectoryRequest(
    PSMB_CONNECTION pSmbRequest
    )
{
    NTSTATUS ntStatus = 0;

    return (ntStatus);
}


NTSTATUS
MarshallDeleteDirectoryResponse(
    PSMB_CONNECTION pSmbRequest
    )
{
    NTSTATUS ntStatus = 0;

    return (ntStatus);

}
