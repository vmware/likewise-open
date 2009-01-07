#include "includes.h"

NTSTATUS
SmbProcessSessionSetup(
    PSMB_CONNECTION pSmbRequest
    )
{
    NTSTATUS ntStatus = 0;

#if 0
    ntStatus = MarshallSessionResponse(pSmbRequest);
    BAIL_ON_NT_STATUS(ntStatus);
#endif


    ntStatus = SmbSendReply(pSmbRequest);
    BAIL_ON_NT_STATUS(ntStatus);

error:

    return (ntStatus);
}

