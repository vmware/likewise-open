#include "includes.h"

NTSTATUS
SmbProcessSetPathInformation(
    PSMB_CONNECTION pSmbRequest
    )
{
    NTSTATUS ntStatus = 0;

#if 0
    ntStatus = UnmarshallSetPathInformationRequest(pSmbRequest);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvSetPathInformation(
                        hTreeObject
                        );
    BAIL_ON_NT_STATUS(ntStatus);


    ntStatus = MarshallSetPathInformationResponse(pSmbRequest);
    BAIL_ON_NT_STATUS(ntStatus);


    ntStatus = SmbSendReply(pSmbRequest);
    BAIL_ON_NT_STATUS(ntStatus);

error:

#endif

    return (ntStatus);
}


NTSTATUS
UnmarshallSetPathInformationAndXRequest(
    PSMB_CONNECTION pSmbRequest
    )
{
    NTSTATUS ntStatus = 0;

    return (ntStatus);
}


NTSTATUS
MarshallSetPathInformationAndXResponse(
    PSMB_CONNECTION pSmbRequest
    )
{
    NTSTATUS ntStatus = 0;

    return (ntStatus);

}
