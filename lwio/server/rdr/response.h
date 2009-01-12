
DWORD
SMBResponseCreate(
    uint16_t       wMid,
    SMB_RESPONSE **ppResponse
    );

VOID
SMBResponseFree(
    PSMB_RESPONSE pResponse
    );

VOID
SMBResponseInvalidate(
    PSMB_RESPONSE pResponse,
    SMB_ERROR_TYPE errorType,
    uint32_t networkError
    );

VOID
SMBResponseUnlock(
    PSMB_RESPONSE pResponse
    );

