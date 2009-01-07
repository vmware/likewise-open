#include "smbserver.h"

NTSTATUS
SmbProcessCreateTemporaryFile(
    PSMB_REQUEST pSmbRequest
    )
{
    NTSTATUS ntStatus = 0;

    ntStatus = UnmarshallCreateTemporaryFileRequest(pSmbRequest);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvCreateTemporaryFile(
                        hTreeObject
                        );
    BAIL_ON_NT_STATUS(ntStatus);
    
    
    ntStatus = MarshallCreateTemporaryFileResponse(pSmbRequest);
    BAIL_ON_NT_STATUS(ntStatus);


    ntStatus = SmbSendReply(pSmbRequest);
    BAIL_ON_NT_STATUS(ntStatus);

error:

    return (ntStatus);
}


NTSTATUS
UnmarshallCreateTemporaryFileRequest(
    PSMB_REQUEST pSmbRequest
    )
{
    NTSTATUS ntStatus = 0;

    return (ntStatus);
}


NTSTATUS
MarshallCreateTemporaryFileResponse(
    PSMB_REQUEST pSmbRequest
    )
{
    NTSTATUS ntStatus = 0;

    return (ntStatus);

}
