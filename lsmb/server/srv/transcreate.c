#include "includes.h"

NTSTATUS
SmbProcessTransCreateAndX(
    PSMB_CONNECTION pSmbRequest
    )
{
    NTSTATUS ntStatus = 0;

#if 0
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

#endif

    return (ntStatus);
}


NTSTATUS
UnmarshallTransWriteAndXRequest(
    PSMB_CONNECTION pSmbRequest
    )
{
    NTSTATUS ntStatus = 0;

    return (ntStatus);
}


NTSTATUS
MarshallTransWriteAndXResponse(
    PSMB_CONNECTION pSmbRequest
    )
{
    NTSTATUS ntStatus = 0;

    return (ntStatus);

}
