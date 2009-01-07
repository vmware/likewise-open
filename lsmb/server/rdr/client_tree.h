
DWORD
SMBSrvClientTreeOpen(
    PCSTR pszHostname,
    PCSTR pszPrincipal,
    PCSTR pszSharename,
    PSMB_TREE* ppTree
    );

DWORD
SMBSrvClientTreeAddResponse(
    PSMB_TREE     pTree,
    PSMB_RESPONSE pResponse
    );

DWORD
SMBSrvClientTreeIsStale_inlock(
    PSMB_TREE pTree,
    PBOOLEAN  pbIsStale
    );

DWORD
SMBSrvClientTreeClose(
    PSMB_TREE pTree
    );
