#include "includes.h"

NTSTATUS
SmbProcessReadAndX(
    PSMB_CONNECTION pSmbRequest
    )
{
    NTSTATUS ntStatus = 0;
    PBYTE pBuffer = NULL;
    DWORD dwBytesRead = 0;
    HANDLE hTreeObject = (HANDLE)NULL;

#if 0
    pTreeConnection = GetTreeConnection(pSmbRequest);
    hTreeObject = pTreeConnection->hTreeConnect
#endif

    ntStatus = UnmarshallReadAndXRequest(pSmbRequest, &pBuffer, &dwBytesRead);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvReadFile(
                        hTreeObject,
                        0,
                        0,
                        NULL,
                        0
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
    PSMB_CONNECTION pSmbRequest,
    PBYTE* ppBuffer,
    PDWORD pdwBytesRead
    )
{
    NTSTATUS ntStatus = 0;

    return (ntStatus);
}

NTSTATUS
SrvReadFile(
    HANDLE hTreeObject,
    USHORT usFid,
    ULONG ulOffset,
    UCHAR  *pBuffer,
    USHORT MaxCount
    )
{
    return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS
MarshallReadAndXResponse(
    PSMB_CONNECTION pSmbRequest
    )
{
    NTSTATUS ntStatus = 0;

    return (ntStatus);

}
