#include "includes.h"

NTSTATUS
SmbProcessRenameFile(
    PSMB_CONNECTION pSmbRequest
    )
{
    NTSTATUS ntStatus = 0;

    ntStatus = UnmarshallRenameFileRequest(pSmbRequest);
    BAIL_ON_NT_STATUS(ntStatus);

#if 0
    ntStatus = SrvRenameFile(
                        hTreeObject
                        );
    BAIL_ON_NT_STATUS(ntStatus);
#endif


    ntStatus = MarshallRenameFileResponse(pSmbRequest);
    BAIL_ON_NT_STATUS(ntStatus);


    ntStatus = SmbSendReply(pSmbRequest);
    BAIL_ON_NT_STATUS(ntStatus);

error:

    return (ntStatus);
}


NTSTATUS
UnmarshallRenameFileRequest(
    PSMB_CONNECTION pSmbRequest
    )
{
    NTSTATUS ntStatus = 0;

    return (ntStatus);
}


NTSTATUS
MarshallRenameFileResponse(
    PSMB_CONNECTION pSmbRequest
    )
{
    NTSTATUS ntStatus = 0;

    return (ntStatus);

}
