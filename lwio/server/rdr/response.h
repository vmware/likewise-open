
NTSTATUS
SMBResponseCreate(
    uint16_t       wMid,
    SMB_RESPONSE **ppResponse
    );

VOID
SMBResponseFree(
    PSMB_RESPONSE pResponse
    );

VOID
SMBResponseInvalidate_InLock(
    PSMB_RESPONSE pResponse,
    NTSTATUS ntStatus
    );

VOID
SMBResponseUnlock(
    PSMB_RESPONSE pResponse
    );

