#include "smbserver.h"

NTSTATUS
SmbProcessQueryFileInformation(
    PSMB_REQUEST pSmbRequest
    )
{
    NTSTATUS ntStatus = 0;

    ntStatus = UnmarshallQueryFileInformationRequest(pSmbRequest);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvQueryFileInformation(
                        hTreeObject
                        );
    BAIL_ON_NT_STATUS(ntStatus);
    
    
    ntStatus = MarshallQueryFileInformationResponse(pSmbRequest);
    BAIL_ON_NT_STATUS(ntStatus);


    ntStatus = SmbSendReply(pSmbRequest);
    BAIL_ON_NT_STATUS(ntStatus);

error:

    return (ntStatus);
}


NTSTATUS
UnmarshallQueryFileInformationRequest(
    PSMB_REQUEST pSmbRequest
    )
{
    NTSTATUS ntStatus = 0;

    return (ntStatus);
}


NTSTATUS
MarshallQueryFileInformationResponse(
    PSMB_REQUEST pSmbRequest
    )
{
    NTSTATUS ntStatus = 0;

    return (ntStatus);

}
