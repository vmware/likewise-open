#include "includes.h"

NTSTATUS
SmbProcessTreeConnectAndX(
    PSMB_CONNECTION pSmbRequest
    )
{
    NTSTATUS ntStatus = 0;

    pSession = GetSessionObject(pSmbRequest);

    ntStatus = MarshallTconAndXResponse(pSmbRequest);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NtVfsOpenTree(
                    pszShareName,
                    &hTreeObject
                    );
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = TreeConnCreateObject(
                        pszShareName,
                        hTreeObject,
                        &pTreeConnection
                        );
    BAIL_ON_NT_STATUS(ntStatus);

    pTreeConnection->pSessionObject = pSessionObject;


    ntStatus = SmbSendReply(pSmbRequest);
    BAIL_ON_NT_STATUS(ntStatus);

error:

    return (ntStatus);
}

