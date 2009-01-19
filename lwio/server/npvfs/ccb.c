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
    PNPFS_PIPE pPipe = NULL;

    ENTER_GLOBAL_READER_LOCK(&gServerLock);
    pPipe = pSCB->pPipe;
    ENTER_PIPE_READER_LOCK(&pPipe->Mutex);

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

    LEAVE_PIPE_READER_LOCK(&pPipe->Mutex);
    LEAVE_GLOBAL_READER_LOCK(&gServerLock);

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
    PNPFS_PIPE pPipe = NULL;

    ENTER_GLOBAL_READER_LOCK(&gServerLock);
    pPipe = pSCB->pPipe;
    ENTER_PIPE_READER_LOCK(&pPipe->Mutex);

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

    LEAVE_PIPE_READER_LOCK(&pPipe->Mutex);
    LEAVE_GLOBAL_READER_LOCK(&gServerLock);

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

    ENTER_GLOBAL_READER_LOCK(&gServerLock);
    pPipe = pCCB->pPipe;
    ENTER_PIPE_READER_LOCK(&pPipe->Mutex);

    switch(pPipe->State) {

        case CLIENT_PIPE_CONNECTED:
            ntStatus = NpfsClientReadFile_Connected(
                            pCCB,
                            Length,
                            pBuffer
                            );
            BAIL_ON_NT_STATUS(ntStatus);

        case CLIENT_PIPE_DISCONNECTED:
            break;
    }

error:

    LEAVE_PIPE_READER_LOCK(&pPipe->Mutex);
    LEAVE_GLOBAL_READER_LOCK(&gServerLock);

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

    ENTER_GLOBAL_READER_LOCK(&gServerLock);
    pPipe = pCCB->pPipe;
    ENTER_PIPE_READER_LOCK(&pPipe->Mutex);

    switch(pPipe->State) {

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

    LEAVE_PIPE_READER_LOCK(&pPipe->Mutex);
    LEAVE_GLOBAL_READER_LOCK(&gServerLock);

    return(ntStatus);
}

