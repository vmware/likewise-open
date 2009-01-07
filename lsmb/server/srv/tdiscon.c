#include "includes.h"

NTSTATUS
SmbProcessTreeDisconnectAndX(
    PSMB_CONNECTION pSmbRequest
    )
{
    NTSTATUS ntStatus = 0;

#if 0
    ntStatus = MarshallTconAndXResponse(pSmbRequest);
    BAIL_ON_NT_STATUS(ntStatus);


    ntStatus = SmbSendReply(pSmbRequest);
    BAIL_ON_NT_STATUS(ntStatus);

error:
#endif

    return (ntStatus);
}

