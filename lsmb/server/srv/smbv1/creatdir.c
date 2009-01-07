#include "smbserver.h"

NTSTATUS
SmbProcessCreateDirectory(
    PSMB_REQUEST pSmbRequest
    )
{
    NTSTATUS ntStatus = 0;

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
UnmarshallCreateDirectoryRequest(
    PSMB_REQUEST pSmbRequest
    )
{
    NTSTATUS ntStatus = 0;

    return (ntStatus);
}


NTSTATUS
MarshallCreateDirectoryResponse(
    PSMB_REQUEST pSmbRequest
    )
{
    NTSTATUS ntStatus = 0;

    return (ntStatus);

}
