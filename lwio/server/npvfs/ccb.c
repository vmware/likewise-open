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
NpfsServerReadFile(
    PNPFS_CCB pCCB
    ULONG Length,
    PVOID pBuffer
    )
{
    NTSTATUS ntStatus = 0;

    return(ntStatus);
}


NTSTATUS
NpfsServerWriteFile(
    PNPFS_CCB pCCB
    ULONG Length,
    PVOID pBuffer
    )
{
    NTSTATUS ntStatus = 0;

    switch(pSCB->State) {

        case SERVER_PIPE_CONNECTED:
                ntStatus = NpfsServerReadFile_Connected(
                                pSCB,
                                Length,
                                Buffer
                                );
                BAIL_ON_NT_STATUS(ntStatus);
                break;


        case SERVER_PIPE_DISCONNECTED:
                break;

        case SERVER_PIPE_WAITING_FOR_CONNECTION:
                break;

    }

error:

    return(ntStatus);
}

NTSTATUS
NpfsClientReadFile(
    PNPFS_CCB pCCB
    ULONG Length,
    PVOID pBuffer
    )
{
    NTSTATUS ntStatus = 0;
    PNPFS_PIPE pPipe = NULL;

    ENTER_CRITICAL_SECTION(&pCCB->Lock);

    switch(pCCB->State) {

        case CLIENT_PIPE_CONNECTED:
            btStatus = NpfsClientReadFile_Connected(
                            pCCB,
                            Length,
                            pBuffer
                            );
            BAIL_ON_NT_STATUS(ntStatus);

        case CLIENT_PIPE_DISCONNECTED:
            break;
    }

error:

    LEAVE_CRITICAL_SECTION(&pCCB->Lock);

    return(ntStatus);
}


NTSTATUS
NpfsClientWriteFile(
    PNPFS_CCB pCCB
    ULONG Length,
    PVOID pBuffer
    )
{
    NTSTATUS ntStatus = 0;
    PNPFS_PIPE pPipe = NULL;

    switch(pCCB->State) {

        case CLIENT_PIPE_CONNECTED:
            ntStatus = NpfsClientWriteFile_Connected(
                            pCCB,
                            Length,
                            pBuffer
                            );
            BAIL_ON_NT_STATUS(ntStatus);
            break;

        case CLIENT_PIPE_DISCONNECTED:
            break;

    }
error:

    return(ntStatus);
}

