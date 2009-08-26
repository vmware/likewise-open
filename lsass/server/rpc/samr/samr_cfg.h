#ifndef _SAMR_CFG_H_
#define _SAMR_CFG_H_


typedef struct samr_srv_config
{
    PSTR pszLpcSocketPath;
    PSTR pszDefaultLoginShell;
    PSTR pszHomedirPrefix;
    PSTR pszHomedirTemplate;

} SAMR_SRV_CONFIG, *PSAMR_SRV_CONFIG;


typedef DWORD (*pFnSamrSrvConfigHandler)(PSAMR_SRV_CONFIG pConfig,
					 PCSTR pszName,
					 PCSTR pszValue);


typedef struct samr_config_handler
{
    PCSTR pszId;
    pFnSamrSrvConfigHandler pFnHandler;

} SAMR_SRV_CONFIG_HANDLER, *PSAMR_SRV_CONFIG_HANDLER;


DWORD
SamrSrvInitialiseConfig(
    PSAMR_SRV_CONFIG pConfig
    );


DWORD
SamrSrvParseConfigFile(
    PCSTR pszConfigFilePath,
    PSAMR_SRV_CONFIG pConfig
    );


DWORD
SamrSrvConfigGetLpcSocketPath(
    PSTR *ppszLpcSocketPath
    );


DWORD
SamrSrvConfigGetDefaultLoginShell(
    PSTR *ppszDefaultLoginShell
    );


DWORD
SamrSrvConfigGetHomedirPrefix(
    PSTR *ppszLpcSocketPath
    );


DWORD
SamrSrvConfigGetHomedirTemplate(
    PSTR *ppszHomedirTemplate
    );


DWORD
SamrSrvSetConfigFilePath(
    PCSTR pszConfigFilePath
    );


#endif /* _SAMR_CFG_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
