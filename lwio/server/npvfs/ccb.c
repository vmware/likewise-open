#include "npfs.h"


NTSTATUS
NpfsCreateSCB(
    PNPFS_IRP_CONTEXT pIrpContext,
    PNPFS_CCB * ppSCB
    )
{
    NTSTATUS ntStatus = 0;
    PNPFS_CCB pSCB = NULL;

    ntStatus = NpfsAllocateMemory(
                    sizeof(NPFS_CCB),
                    &pSCB
                    );
    BAIL_ON_NT_STATUS(ntStatus);

    pSCB->CcbType = NPFS_CCB_SERVER;

    *ppSCB = pSCB;

    return(ntStatus);


error:

    *ppSCB = NULL;

    return(ntStatus);
}

NTSTATUS
NpfsCreateCCB(
    PNPFS_IRP_CONTEXT pIrpContext,
    PNPFS_CCB * ppCCB
    )
{
    NTSTATUS ntStatus = 0;
    PNPFS_CCB pCCB = NULL;


    ntStatus = NpfsAllocateMemory(
                    sizeof(NPFS_CCB),
                    &pCCB
                    );
    BAIL_ON_NT_STATUS(ntStatus);

    pCCB->CcbType = NPFS_CCB_CLIENT;

    *ppCCB = pCCB;

    return(ntStatus);


error:

    *ppCCB = NULL;

    return(ntStatus);
}

