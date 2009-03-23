
NTSTATUS
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
    NTSTATUS ntStatus
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

NTSTATUS
SMBSessionFindTreeByPath(
    IN PSMB_SESSION pSession,
    IN PCSTR pszPath,
    OUT PSMB_TREE* ppTree
    );

NTSTATUS
SMBSessionFindTreeById(
    PSMB_SESSION pSession,
    uint16_t     tid,
    PSMB_TREE*   ppTree
    );

NTSTATUS
SMBSessionReceiveResponse(
    IN PSMB_SESSION pSession,
    IN BOOLEAN bVerifySignature,
    IN DWORD dwExpectedSequence,
    OUT PSMB_PACKET* ppPacket
    );

VOID
SMBSessionRelease(
    PSMB_SESSION pSession
    );

VOID
SMBSessionFree(
    PSMB_SESSION pSession
    );

