#ifndef _DSSETUP_CFG_H_
#define _DSSETUP_CFG_H_


typedef struct dssetup_srv_config {
    PSTR pszLpcSocketPath;
    PSTR pszLsaLpcSocketPath;
} DSSETUP_SRV_CONFIG, *PDSSETUP_SRV_CONFIG;


typedef DWORD (*pFnDsrSrvConfigHandler)(PDSSETUP_SRV_CONFIG pConfig,
					 PCSTR pszName,
					 PCSTR pszValue);


typedef struct dssetup_config_handler {
    PCSTR pszId;
    pFnDsrSrvConfigHandler pFnHandler;
} DSSETUP_SRV_CONFIG_HANDLER, *PDSSETUP_SRV_CONFIG_HANDLER;


DWORD
DsrSrvInitialiseConfig(
    PDSSETUP_SRV_CONFIG pConfig
    );


DWORD
DsrSrvParseConfigFile(
    PCSTR pszConfigFilePath,
    PDSSETUP_SRV_CONFIG pConfig
    );


DWORD
DsrSrvConfigGetLpcSocketPath(
    PSTR *ppszLpcSocketPath
    );


DWORD
DsrSrvConfigGetLsaLpcSocketPath(
    PSTR *ppszSamrLpcSocketPath
    );


DWORD
DsrSrvSetConfigFilePath(
    PCSTR pszConfigFilePath
    );


#endif /* _DSSETUP_CFG_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
