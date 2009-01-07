#ifndef __SMBCFG_H__
#define __SMBCFG_H__

DWORD
SMBSrvSetupInitialConfig(
    VOID
    );

DWORD
SMBSrvRefreshConfig(
    IN PCSTR pszConfigFilePath
    );

VOID
SMBSrvFreeConfig(
    IN OUT PSMB_CONFIG pConfig
    );

VOID
SMBSrvFreeConfigContents(
    IN OUT PSMB_CONFIG pConfig
    );

#endif

