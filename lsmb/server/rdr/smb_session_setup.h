
uint32_t
SessionSetup(
    PSMB_SOCKET pSocket,
    BOOLEAN     bSignMessages,
    PBYTE       pPrimerSessionKey,
    DWORD       dwPrimerSessionKeyLen,
    uint16_t   *pUID,
    PBYTE*      ppSessionKey,
    PDWORD      pdwSessionKeyLength,
    PHANDLE     phGSSContext
    );

