#include "smbserver.h"

NTSTATUS
SmbProcessReadAndX(
    PSMB_REQUEST pSmbRequest
    )
{
    NTSTATUS ntStatus = 0;

    ntStatus = MarshallReadAndXResponse(pSmbRequest);
    BAIL_ON_NT_STATUS(ntStatus);


    ntStatus = SmbSendReply(pSmbRequest);
    BAIL_ON_NT_STATUS(ntStatus);

error:

    return (ntStatus);
}

