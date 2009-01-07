#include "includes.h"

NTSTATUS
SmbProcessNTRename(
    PSMB_CONNECTION pSmbRequest
    )
{
    NTSTATUS ntStatus = 0;
    HANDLE hTreeObject = (HANDLE)NULL;

    ntStatus = UnmarshallNTRenameRequest(pSmbRequest);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvRenameFile(
                        hTreeObject,
                        0,
                        NULL,
                        NULL
                        );
    BAIL_ON_NT_STATUS(ntStatus);


    ntStatus = MarshallNTRenameResponse(pSmbRequest);
    BAIL_ON_NT_STATUS(ntStatus);


    ntStatus = SmbSendReply(pSmbRequest);
    BAIL_ON_NT_STATUS(ntStatus);

error:

    return (ntStatus);
}


NTSTATUS
UnmarshallNTRenameRequest(
    PSMB_CONNECTION pSmbRequest
    )
{
    NTSTATUS ntStatus = 0;

    return (ntStatus);
}

NTSTATUS
SrvRenameFile(
    HANDLE hTreeObject,
    USHORT usSearchAttributes,
    LPWSTR pszOldFileName,
    LPWSTR pszNewFileName
    )
{
    return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS
MarshallNTRenameResponse(
    PSMB_CONNECTION pSmbRequest
    )
{
    NTSTATUS ntStatus = 0;

    return (ntStatus);

}
