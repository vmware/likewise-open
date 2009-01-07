
DWORD
RdrSocketInit(
    VOID
    );

DWORD
SMBSrvClientSocketCreate(
    uchar8_t    *pszHostname,
    PSMB_SOCKET* ppSocket
    );

DWORD
SMBSrvClientSocketIsStale_inlock(
    PSMB_SOCKET pSocket,
    PBOOLEAN    pbIsStale
    );

DWORD
SMBSrvClientSocketAddSessionByPrincipal(
    PSMB_SOCKET  pSocket,
    PSMB_SESSION pSession
    );

DWORD
SMBSrvClientSocketRemoveSessionByPrincipal(
    PSMB_SOCKET  pSocket,
    PSMB_SESSION pSession
    );

DWORD
SMBSrvClientSocketAddSessionByUID(
    PSMB_SOCKET  pSocket,
    PSMB_SESSION pSession
    );

DWORD
SMBSrvClientSocketRemoveSessionByUID(
    PSMB_SOCKET  pSocket,
    PSMB_SESSION pSession
    );

DWORD
RdrSocketShutdown(
    VOID
    );
