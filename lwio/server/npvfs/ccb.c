NTSTATUS
NpfsCreateScb(
    PNPFS_IRP_CONTEXT pIrpContext,
    PNPFS_FCB pFcb,
    PNPFS_CCB * ppScb
    )
{
    NTSTATUS ntStatus = 0;

    ntstatus = NpfsAllocateCCB(
                    pIrpContext,
                    pFcb,
                    NPFS_SERVER_CCB,
                    &pScb
                    );
    BAIL_ON_NT_STATUS(ntStatus);

    pScb->pFcb = pFcb;
    NpfsFcbAddReference(pFcb);







NTSTATUS
ReadFile(
    PNPFS_CCB pCCB
    ULONG Length,
    PVOID pBuffer
    )
{
    NTSTATUS ntStatus = 0;

    switch(CcbType) {

        case NPFS_CCB_SERVER:
            ntStatus = NpfsServerReadFile(
                            pCCB,
                            pIrpContext
                            );
            BAIL_ON_NT_STATUS(ntStatus);
            break;

        case NPFS_CCB_CLIENT:
            ntStatus = NpfsClientReadFile(
                            pCCB,
                            pIrpContext
                            );
            BAIL_ON_NT_STATUS(ntStatus);
            break;
    }


}


NTSTATUS
WriteFile(
    PNPFS_CCB pCCB
    ULONG Length,
    PVOID pBuffer
    )
{
    NTSTATUS ntStatus = 0;

    GET_ASSOCIATED_CCB(&pAssociatedCCB);
    GET_PARENT_PIPE(&pPipe);

    if (pPipe->Status = DISCONNECTED) {
        ntStatus = STATUS_PIPE_BROKEN;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if ((pPipe->Status == CLIENT_CLOSED) && pCCB->Mode == SERVER){
        ntStatus = STATUS_CLIENT_DISCONNECTED;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if ((pPipe->Status == SERVER_CLOSED) && pCCB->Mode == CLIENT) {
        ntStatus = STATUS_SERVER_CLOSED_CONNECTION;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if ((pPipe->Status == SERVER_

    ENTER_CRITICAL_SECTION(&pAssocCCB->Lock);

    WRITE_DATA_INFO_INBOUND_QUEUE;

    ntStatus = NpfsEnqueueMdl(
                    Length,
                    pBuffer,
                    &pMdl
                    );
    BAIL_ON_NTSTATUS(ntStatus);

    if (QueueWasEmpty) {

        SignalEvent();
    }

    LEAVE_CRITICAL_SECTION(&pAssocCB->Lock);

    return(ntStatus);

error:


    LEAVE_CRITICAL_SECTION

    return(ntStatus);

}


NTSTATUS
DisconnectCCB(
    )

