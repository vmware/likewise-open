#ifndef __IPC_H__
#define __IPC_H__

LWMsgStatus
SMBSrvIpcRefreshConfiguration(
    LWMsgAssoc*             pAssoc,
    const LWMsgMessage*     pRequest,
    LWMsgMessage*           pResponse,
    void* pData
    );

LWMsgStatus
SMBSrvIpcSetLogInfo(
    LWMsgAssoc*             pAssoc,
    const LWMsgMessage*     pRequest,
    LWMsgMessage*           pResponse,
    void* pData
    );

LWMsgStatus
SMBSrvIpcGetLogInfo(
    LWMsgAssoc*             pAssoc,
    const LWMsgMessage*     pRequest,
    LWMsgMessage*           pResponse,
    void* pData
    );

LWMsgStatus
SMBSrvIpcCallNamedPipe(
    LWMsgAssoc*             pAssoc,
    const LWMsgMessage*     pRequest,
    LWMsgMessage*           pResponse,
    void* pData
    );

LWMsgStatus
SMBSrvIpcCreateNamedPipe(
    LWMsgAssoc*             pAssoc,
    const LWMsgMessage*     pRequest,
    LWMsgMessage*           pResponse,
    void* pData
    );

LWMsgStatus
SMBSrvIpcGetNamedPipeInfo(
    LWMsgAssoc*             pAssoc,
    const LWMsgMessage*     pRequest,
    LWMsgMessage*           pResponse,
    void* pData
    );

LWMsgStatus
SMBSrvIpcConnectNamedPipe(
    LWMsgAssoc*             pAssoc,
    const LWMsgMessage*     pRequest,
    LWMsgMessage*           pResponse,
    void* pData
    );

LWMsgStatus
SMBSrvIpcTransactNamedPipe(
    LWMsgAssoc*             pAssoc,
    const LWMsgMessage*     pRequest,
    LWMsgMessage*           pResponse,
    void* pData
    );

LWMsgStatus
SMBSrvIpcWaitNamedPipe(
    LWMsgAssoc*             pAssoc,
    const LWMsgMessage*     pRequest,
    LWMsgMessage*           pResponse,
    void* pData
    );

LWMsgStatus
SMBSrvIpcGetClientComputerName(
    LWMsgAssoc*             pAssoc,
    const LWMsgMessage*     pRequest,
    LWMsgMessage*           pResponse,
    void* pData
    );

LWMsgStatus
SMBSrvIpcGetClientProcessId(
    LWMsgAssoc*             pAssoc,
    const LWMsgMessage*     pRequest,
    LWMsgMessage*           pResponse,
    void* pData
    );

LWMsgStatus
SMBSrvIpcGetServerProcessId(
    LWMsgAssoc*             pAssoc,
    const LWMsgMessage*     pRequest,
    LWMsgMessage*           pResponse,
    void* pData
    );

LWMsgStatus
SMBSrvIpcGetClientSessionId(
    LWMsgAssoc*             pAssoc,
    const LWMsgMessage*     pRequest,
    LWMsgMessage*           pResponse,
    void* pData
    );

LWMsgStatus
SMBSrvIpcPeekNamedPipe(
    LWMsgAssoc*             pAssoc,
    const LWMsgMessage*     pRequest,
    LWMsgMessage*           pResponse,
    void* pData
    );

LWMsgStatus
SMBSrvIpcDisconnectNamedPipe(
    LWMsgAssoc*             pAssoc,
    const LWMsgMessage*     pRequest,
    LWMsgMessage*           pResponse,
    void* pData
    );

LWMsgStatus
SMBSrvIpcCreateFile(
    LWMsgAssoc*             pAssoc,
    const LWMsgMessage*     pRequest,
    LWMsgMessage*           pResponse,
    void* pData
    );

LWMsgStatus
SMBSrvIpcSetNamedPipeHandleState(
    LWMsgAssoc*             pAssoc,
    const LWMsgMessage*     pRequest,
    LWMsgMessage*           pResponse,
    void* pData
    );

LWMsgStatus
SMBSrvIpcReadFile(
    LWMsgAssoc*             pAssoc,
    const LWMsgMessage*     pRequest,
    LWMsgMessage*           pResponse,
    void* pData
    );

LWMsgStatus
SMBSrvIpcWriteFile(
    LWMsgAssoc*             pAssoc,
    const LWMsgMessage*     pRequest,
    LWMsgMessage*           pResponse,
    void* pData
    );

LWMsgStatus
SMBSrvIpcCloseFile(
    LWMsgAssoc*             pAssoc,
    const LWMsgMessage*     pRequest,
    LWMsgMessage*           pResponse,
    void* pData
    );

LWMsgStatus
SMBSrvIpcGetSessionKey(
    LWMsgAssoc*             pAssoc,
    const LWMsgMessage*     pRequest,
    LWMsgMessage*           pResponse,
    void* pData
    );

#endif /* __IPC_H__ */
