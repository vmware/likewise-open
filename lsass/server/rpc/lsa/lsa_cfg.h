#ifndef _LSA_CFG_H_
#define _LSA_CFG_H_


typedef struct samr_srv_config {
    PSTR pszLpcSocketPath;
    PSTR pszSamrLpcSocketPath;
} LSA_SRV_CONFIG, *PLSA_SRV_CONFIG;


typedef DWORD (*pFnLsaSrvConfigHandler)(PLSA_SRV_CONFIG pConfig,
					 PCSTR pszName,
					 PCSTR pszValue);


typedef struct samr_config_handler {
    PCSTR pszId;
    pFnLsaSrvConfigHandler pFnHandler;
} LSA_SRV_CONFIG_HANDLER, *PLSA_SRV_CONFIG_HANDLER;


DWORD
LsaSrvInitialiseConfig(
    PLSA_SRV_CONFIG pConfig
    );


DWORD
LsaSrvParseConfigFile(
    PCSTR pszConfigFilePath,
    PLSA_SRV_CONFIG pConfig
    );


DWORD
LsaSrvConfigGetLpcSocketPath(
    PSTR *ppszLpcSocketPath
    );


DWORD
LsaSrvConfigGetSamrLpcSocketPath(
    PSTR *ppszSamrLpcSocketPath
    );


DWORD
LsaSrvSetConfigFilePath(
    PCSTR pszConfigFilePath
    );


#endif /* _LSA_CFG_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
