
VOID
SMBTreeAddReference(
    PSMB_TREE pTree
    );

VOID
SMBTreeRelease(
    PSMB_TREE pTree
    );

NTSTATUS
SMBTreeCreate(
    PSMB_TREE* ppTree
    );

NTSTATUS
SMBTreeAcquireMid(
    PSMB_TREE pTree,
    uint16_t* pwMid
    );

NTSTATUS
SMBTreeSetState(
    PSMB_TREE pTree,
    SMB_RESOURCE_STATE state
    );

NTSTATUS
SMBTreeInvalidate(
    PSMB_TREE      pTree,
    SMB_ERROR_TYPE errorType,
    uint32_t       error
    );

NTSTATUS
SMBSrvClientTreeAddResponse(
    PSMB_TREE pTree,
    SMB_RESPONSE *pResponse
    );

NTSTATUS
SMBTreeReceiveResponse(
    PSMB_TREE     pTree,
    PSMB_RESPONSE pResponse,
    PSMB_PACKET*  ppResponsePacket
    );

NTSTATUS
SMBTreeFindLockedResponseByMID(
    PSMB_TREE      pTree,
    uint16_t       wMid,
    PSMB_RESPONSE* ppResponse
    );
