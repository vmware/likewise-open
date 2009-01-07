#include "smbserver.h"

NTSTATUS
SmbProcessLogoffAndX(
    PSMB_REQUEST pSmbRequest
    )
{
    NTSTATUS ntStatus = 0;

    ntStatus = MarshallLogoffAndXResponse(pSmbRequest);
    BAIL_ON_NT_STATUS(ntStatus);


    ntStatus = SmbSendReply(pSmbRequest);
    BAIL_ON_NT_STATUS(ntStatus);

error:

    return (ntStatus);
}

