
DWORD
SMBSrvClientSessionCreate(
    PSMB_SOCKET   pSocket,
    uchar8_t      *pszPrincipal,
    PSMB_SESSION* ppSession
    );

DWORD
SMBSrvClientSessionIsStale_inlock(
    PSMB_SESSION pSession,
    PBOOLEAN     pbIsStale
    );

DWORD
SMBSrvClientSessionAddTreeById(
    PSMB_SESSION pSession,
    PSMB_TREE    pTree
    );

DWORD
SMBSrvClientSessionRemoveTreeById(
    PSMB_SESSION pSession,
    PSMB_TREE    pTree
    );

DWORD
SMBSrvClientSessionAddTreeByPath(
    PSMB_SESSION pSession,
    PSMB_TREE    pTree
    );

DWORD
SMBSrvClientSessionRemoveTreeByPath(
    PSMB_SESSION pSession,
    PSMB_TREE    pTree
    );

BOOLEAN
SMBSrvClientSessionSignMessages(
    PSMB_SESSION pSession
    );

DWORD
SMBSrvClientSessionRelease(
    PSMB_SESSION pSession
    );
