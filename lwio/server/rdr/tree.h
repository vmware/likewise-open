
VOID
SMBTreeAddReference(
    PSMB_TREE pTree
    );

VOID
SMBTreeRelease(
    PSMB_TREE pTree
    );

DWORD
SMBTreeCreate(
    PSMB_TREE* ppTree
    );

DWORD
SMBTreeAcquireMid(
    PSMB_TREE pTree,
    uint16_t* pwMid
    );

DWORD
SMBTreeSetState(
    PSMB_TREE pTree,
    SMB_RESOURCE_STATE state
    );

DWORD
SMBTreeInvalidate(
    PSMB_TREE      pTree,
    SMB_ERROR_TYPE errorType,
    uint32_t       error
    );

DWORD
SMBSrvClientTreeAddResponse(
    PSMB_TREE pTree,
    SMB_RESPONSE *pResponse
    );

DWORD
SMBTreeReceiveResponse(
    PSMB_TREE     pTree,
    PSMB_RESPONSE pResponse,
    PSMB_PACKET*  ppResponsePacket
    );

DWORD
SMBTreeFindLockedResponseByMID(
    PSMB_TREE      pTree,
    uint16_t       wMid,
    PSMB_RESPONSE* ppResponse
    );
