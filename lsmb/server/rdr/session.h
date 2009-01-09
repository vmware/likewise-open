
DWORD
SMBSessionCreate(
    PSMB_SESSION* ppSession
    );

VOID
SMBSessionAddReference(
    PSMB_SESSION pSession
    );

VOID
SMBSessionInvalidate(
    PSMB_SESSION   pSession,
    SMB_ERROR_TYPE errorType,
    uint32_t       error
    );

VOID
SMBSessionSetState(
    PSMB_SESSION pSession,
    SMB_RESOURCE_STATE state
    );

VOID
SMBSessionUpdateLastActiveTime(
    PSMB_SESSION pSession
    );

DWORD
SMBSessionFindTreeByPath(
    PSMB_SESSION pSession,
    uchar8_t    *pszPath,
    PSMB_TREE*  ppTree
    );

DWORD
SMBSessionFindTreeById(
    PSMB_SESSION pSession,
    uint16_t     tid,
    PSMB_TREE*   ppTree
    );

DWORD
SMBSessionReceiveResponse(
    PSMB_SESSION pSession,
    PSMB_PACKET* ppPacket
    );

VOID
SMBSessionRelease(
    PSMB_SESSION pSession
    );


