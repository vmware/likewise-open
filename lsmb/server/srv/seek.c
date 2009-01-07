#include "includes.h"

NTSTATUS
SmbProcessSeek(
    PSMB_CONNECTION pSmbRequest
    )
{
    NTSTATUS ntStatus = 0;

    ntStatus = UnmarshallSeekRequest(pSmbRequest);
    BAIL_ON_NT_STATUS(ntStatus);

#if 0
    ntStatus = SrvWriteFile(
                        hTreeObject
                        );
    BAIL_ON_NT_STATUS(ntStatus);
#endif


    ntStatus = MarshallSeekResponse(pSmbRequest);
    BAIL_ON_NT_STATUS(ntStatus);


    ntStatus = SmbSendReply(pSmbRequest);
    BAIL_ON_NT_STATUS(ntStatus);

error:

    return (ntStatus);
}


NTSTATUS
UnmarshallSeekRequest(
    PSMB_CONNECTION pSmbRequest
    )
{
    NTSTATUS ntStatus = 0;

    return (ntStatus);
}


NTSTATUS
MarshallSeekResponse(
    PSMB_CONNECTION pSmbRequest
    )
{
    NTSTATUS ntStatus = 0;

    return (ntStatus);

}
