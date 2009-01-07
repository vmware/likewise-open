#include "includes.h"

NTSTATUS
SmbProcessQueryFileInformation(
    PSMB_CONNECTION pSmbRequest
    )
{
    NTSTATUS ntStatus = 0;

#if 0
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

#endif

    return (ntStatus);
}


NTSTATUS
UnmarshallQueryFileInformationRequest(
    PSMB_CONNECTION pSmbRequest
    )
{
    NTSTATUS ntStatus = 0;

    return (ntStatus);
}


NTSTATUS
MarshallQueryFileInformationResponse(
    PSMB_CONNECTION pSmbRequest
    )
{
    NTSTATUS ntStatus = 0;

    return (ntStatus);

}
