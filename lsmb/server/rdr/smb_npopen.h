
uint32_t
NPOpen(
    SMB_TREE  *pTree,
    wchar16_t *pwszPath,
    DWORD dwDesiredAccess,
    DWORD dwSharedMode,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    uint16_t  *pFid
    );

