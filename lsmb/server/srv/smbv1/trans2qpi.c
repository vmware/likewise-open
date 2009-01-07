#include "smbserver.h"

NTSTATUS
SmbProcessWriteAndX(
    PSMB_REQUEST pSmbRequest
    )
{
    NTSTATUS ntStatus = 0;

    ntStatus = UnmarshallWriteAndXRequest(pSmbRequest);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvWriteFile(
                        hTreeObject
                        );
    BAIL_ON_NT_STATUS(ntStatus);
    
    
    ntStatus = MarshallWriteAndXResponse(pSmbRequest);
    BAIL_ON_NT_STATUS(ntStatus);


    ntStatus = SmbSendReply(pSmbRequest);
    BAIL_ON_NT_STATUS(ntStatus);

error:

    return (ntStatus);
}


NTSTATUS
UnmarshallWriteAndXRequest(
    PSMB_REQUEST pSmbRequest
    )
{
    NTSTATUS ntStatus = 0;

    return (ntStatus);
}


NTSTATUS
MarshallWriteAndXResponse(
    PSMB_REQUEST pSmbRequest
    )
{
    NTSTATUS ntStatus = 0;

    return (ntStatus);

}
