#include "includes.h"

NTSTATUS
SmbProcessQueryFileInformationAndX(
    PSMB_CONNECTION pSmbRequest
    )
{
    NTSTATUS ntStatus = 0;

#if 0
    ntStatus = UnmarshallQueryFileInformationAndXRequest(pSmbRequest);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvQueryFileInformation(
                        hTreeObject
                        );
    BAIL_ON_NT_STATUS(ntStatus);


    ntStatus = MarshallQueryFileInformationAndXResponse(pSmbRequest);
    BAIL_ON_NT_STATUS(ntStatus);


    ntStatus = SmbSendReply(pSmbRequest);
    BAIL_ON_NT_STATUS(ntStatus);

error:
#endif

    return (ntStatus);
}


NTSTATUS
UnmarshallQueryFileInformationAndXRequest(
    PSMB_CONNECTION pSmbRequest
    )
{
    NTSTATUS ntStatus = 0;

    return (ntStatus);
}


NTSTATUS
MarshallQueryFileInformationAndXResponse(
    PSMB_CONNECTION pSmbRequest
    )
{
    NTSTATUS ntStatus = 0;

    return (ntStatus);

}
