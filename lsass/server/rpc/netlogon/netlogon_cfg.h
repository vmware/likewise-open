#ifndef _NETLOGON_CFG_H_
#define _NETLOGON_CFG_H_


typedef struct netlogon_srv_config
{
    PSTR pszLpcSocketPath;
    PSTR pszDefaultLoginShell;
    PSTR pszHomedirPrefix;
    PSTR pszHomedirTemplate;
    BOOLEAN bRegisterTcpIp;
} NETLOGON_SRV_CONFIG, *PNETLOGON_SRV_CONFIG;


DWORD
NetlogonSrvInitialiseConfig(
    PNETLOGON_SRV_CONFIG pConfig
    );


VOID
NetlogonSrvFreeConfigContents(
    PNETLOGON_SRV_CONFIG pConfig
    );


DWORD
NetlogonSrvReadRegistry(
    PNETLOGON_SRV_CONFIG pConfig
    );


DWORD
NetlogonSrvConfigGetLpcSocketPath(
    PSTR *ppszLpcSocketPath
    );


DWORD
NetlogonSrvConfigGetDefaultLoginShell(
    PSTR *ppszDefaultLoginShell
    );


DWORD
NetlogonSrvConfigGetHomedirPrefix(
    PSTR *ppszLpcSocketPath
    );


DWORD
NetlogonSrvConfigGetHomedirTemplate(
    PSTR *ppszHomedirTemplate
    );

DWORD
NetlogonSrvConfigShouldRegisterTcpIp( BOOLEAN* pbResult
    );

#endif /* _NETLOGON_CFG_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
