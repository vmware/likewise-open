#include "smbserver.h"

NTSTATUS
SmbProcessReadAndX(
    PSMB_REQUEST pSmbRequest
    )
{
    NTSTATUS ntStatus = 0;

    pTreeConnection = GetTreeConnection(pSmbRequest);
    hTreeObject = pTreeConnection->hTreeConnect

    ntStatus = UnmarshallReadAndXRequest(pSmbRequest,&pBuffer, &dwBytesRead)
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvReadFile(
                        hTreeObject
                        );
    BAIL_ON_NT_STATUS(ntStatus);
    
    
    ntStatus = MarshallReadAndXResponse(pSmbRequest);
    BAIL_ON_NT_STATUS(ntStatus);


    ntStatus = SmbSendReply(pSmbRequest);
    BAIL_ON_NT_STATUS(ntStatus);

error:

    return (ntStatus);
}


NTSTATUS
UnmarshallReadAndXRequest(
    PSMB_REQUEST pSmbRequest
    )
{
    NTSTATUS ntStatus = 0;

    return (ntStatus);
}


NTSTATUS
MarshallReadAndXResponse(
    PSMB_REQUEST pSmbRequest
    )
{
    NTSTATUS ntStatus = 0;

    return (ntStatus);

}
