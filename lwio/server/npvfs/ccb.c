
NTSTATUS
NpfsReadFile(
    PNPFS_CCB pCCB,
    PNPFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntStatus = 0;

    switch(pCCB->ContextType) {

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

error:
    return(ntStatus);
}


NTSTATUS
NpfsWriteFile(
    PNPFS_CCB pCCB,
    PNPFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntStatus = 0;

    switch(pCCB->ContextType) {

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

error:
    return(ntStatus);
}

NTSTATUS
NpfsServerReadFile(
    PNPFS_CCB pCCB,
    PNPFS_IRP_CONTEXT pIrpContext
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
                                pIrpContext
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
    PNPFS_CCB pCCB,
    PNPFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntStatus = 0;
    PNPFS_PIPE pPipe = NULL;

    ENTER_GLOBAL_READER_LOCK(&gServerLock);
    pPipe = pSCB->pPipe;
    ENTER_PIPE_READER_LOCK(&pPipe->Mutex);

    switch(pPipe->State) {

        case SERVER_PIPE_CONNECTED:
                ntStatus = NpfsServerReadFile_Connected(
                                pSCB,
                                pIrpContext
                                );
                BAIL_ON_NT_STATUS(ntStatus);
                break;


        case CLIENT_PIPE_DISCONNECTED:
                ntStatus = STATUS_PIPE_BROKEN;
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
    PNPFS_CCB pCCB,
    PNPFS_IRP_CONTEXT pIrpContext
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
                            pIrpContext
                            );
            BAIL_ON_NT_STATUS(ntStatus);

        case SERVER_PIPE_DISCONNECTED:
            ntStatus = STATUS_PIPE_BROKEN;
            break;
    }

error:

    LEAVE_PIPE_READER_LOCK(&pPipe->Mutex);
    LEAVE_GLOBAL_READER_LOCK(&gServerLock);

    return(ntStatus);
}


NTSTATUS
NpfsClientWriteFile(
    PNPFS_CCB pCCB,
    PNPFS_IRP_CONTEXT pIrpContext
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
                            pIrpContext
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


NTSTATUS
NpfsServerReadFile_Connected(
    PNPFS_CCB pSCB
    PNPFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntStatus = 0;
    PNPFS_PIPE pPipe = NULL;
    PVOID pBuffer = NULL;
    ULONG Length = 0;

    ENTER_CRITICAL_SECTION(&pSCB->InBoundMutex);

    ntStatus = NpfsDequeueBuffer(
                        pSCB->pMdlList,
                        pBuffer,
                        Length
                        );
    BAIL_ON_NT_STATUS(ntStatus);

error:

    LEAVE_CRITICAL_SECTION(&pSCB->InBoundMutex);

    return(ntStatus);
}

NTSTATUS
NpfsServerWriteFile_Connected(
    PNPFS_CCB pSCB,
    PNPFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntStatus = 0;
    PNPFS_PIPE pPipe = NULL;
    PVOID pBuffer = NULL;
    ULONG Length = 0;

    pPipe = pSCB->pPipe;
    pCCB = pPipe->pCCB;

    ENTER_CRITICAL_SECTION(&pCCB->InBoundMutex);

    ntStatus = NpfsEnqueueBuffer(
                        pCCB->pMdlList,
                        pBuffer,
                        Length
                        );
    BAIL_ON_NT_STATUS(ntStatus);

error:

    LEAVE_CRITICAL_SECTION(&pCCB->InBoundMutex);
    return(ntStatus);
}

NTSTATUS
NpfsClientReadFile_Connected(
    PNPFS_CCB pCCB,
    PNPFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntStatus = 0;
    PNPFS_PIPE pPipe = NULL;
    PVOID pBuffer = NULL;
    ULONG Length = 0;

    ENTER_CRITICAL_SECTION(&pCCB->InBoundMutex);

    ntStatus = NpfsDequeueBuffer(
                        pCCB->pMdlList,
                        pBuffer,
                        Length
                        );
    BAIL_ON_NT_STATUS(ntStatus);

error:

    LEAVE_CRITICAL_SECTION(&pCCB->InBoundMutex);

    return(ntStatus);
}

NTSTATUS
NpfsClientWriteFile_Connected(
    PNPFS_CCB pCCB,
    PNPFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntStatus = 0;
    PNPFS_PIPE pPipe = NULL;
    PNPFS_CCB pSCB = NULL;
    PVOID pBuffer = NULL;
    ULONG Length = 0;

    pPipe = pCCB->pPipe;
    pSCB = pPipe->pSCB;

    ENTER_CRITICAL_SECTION(&pSCB->InBoundMutex);

    ntStatus = NpfsEnqueueBuffer(
                        pSCB->pMdlList,
                        pBuffer,
                        Length
                        );
    BAIL_ON_NT_STATUS(ntStatus);

error:

    LEAVE_CRITICAL_SECTION(&pSCB->InBoundMutex);

    return(ntStatus);
}
