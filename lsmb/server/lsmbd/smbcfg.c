#include "includes.h"

static
DWORD
SMBSrvParseConfig(
    IN PCSTR pszConfigFilePath,
    IN OUT PSMB_CONFIG pConfig
    );

static
DWORD
SMBSrvConfigStartSection(
    PCSTR    pszSectionName,
    PVOID    pData,
    PBOOLEAN pbSkipSection,
    PBOOLEAN pbContinue
    );

static
DWORD
SMBSrvInitializeConfig(
    IN OUT PSMB_CONFIG pConfig
    );

static
DWORD
SMBSrvConfigNameValuePair(
    PCSTR    pszName,
    PCSTR    pszValue,
    PVOID    pData,
    PBOOLEAN pbContinue
    );

static
DWORD
SMBSrvTransferConfigContents(
    PSMB_CONFIG pSrcConfig,
    PSMB_CONFIG pDstConfig
    );

DWORD
SMBSrvSetupInitialConfig(
    VOID
    )
{
    DWORD dwError = 0;
    BOOLEAN bUnlockConfig = FALSE;

    SMB_LOCK_SERVERCONFIG(bUnlockConfig);

    dwError = SMBSrvInitializeConfig(gpServerConfig);

    SMB_UNLOCK_SERVERCONFIG(bUnlockConfig);

    return dwError;
}

DWORD
SMBSrvRefreshConfig(
    PCSTR       pszConfigFilePath
    )
{
    DWORD dwError = 0;
    BOOLEAN bUnlockConfig = FALSE;
    SMB_CONFIG smb_config;

    dwError = SMBSrvParseConfig(
                    pszConfigFilePath,
                    &smb_config);
    BAIL_ON_SMB_ERROR(dwError);

    SMB_LOCK_SERVERCONFIG(bUnlockConfig);

    dwError = SMBSrvTransferConfigContents(
                    &smb_config,
                    gpServerConfig);
    BAIL_ON_SMB_ERROR(dwError);

cleanup:

    SMB_UNLOCK_SERVERCONFIG(bUnlockConfig);

    return dwError;

error:

    SMBSrvFreeConfigContents(&smb_config);

    goto cleanup;
}

static
DWORD
SMBSrvParseConfig(
    IN PCSTR pszConfigFilePath,
    IN OUT PSMB_CONFIG pConfig
    )
{
    DWORD dwError = 0;
    SMB_CONFIG smb_config;

    memset(&smb_config, 0, sizeof(SMB_CONFIG));

    BAIL_ON_INVALID_STRING(pszConfigFilePath);

    dwError = SMBSrvInitializeConfig(&smb_config);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBParseConfigFile(
                    pszConfigFilePath,
                    SMB_CFG_OPTION_STRIP_ALL,
                    &SMBSrvConfigStartSection,
                    NULL,
                    &SMBSrvConfigNameValuePair,
                    NULL,
                    &smb_config);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBSrvTransferConfigContents(
                    &smb_config,
                    pConfig);
    BAIL_ON_SMB_ERROR(dwError);

cleanup:

    return dwError;

error:

    SMBSrvFreeConfigContents(&smb_config);

    goto cleanup;
}

static
DWORD
SMBSrvConfigStartSection(
    PCSTR    pszSectionName,
    PVOID    pData,
    PBOOLEAN pbSkipSection,
    PBOOLEAN pbContinue
    )
{
    DWORD dwError = 0;
    BOOLEAN bContinue = TRUE;
    BOOLEAN bSkipSection = FALSE;

    if (IsNullOrEmptyString(pszSectionName) ||
        (strncasecmp(pszSectionName, "global", sizeof("global")-1)))
    {
        bSkipSection = TRUE;
    }

    *pbSkipSection = bSkipSection;
    *pbContinue = bContinue;

    return dwError;
}

static
DWORD
SMBSrvConfigNameValuePair(
    PCSTR    pszName,
    PCSTR    pszValue,
    PVOID    pData,
    PBOOLEAN pbContinue
    )
{
    DWORD dwError = 0;
    PSMB_CONFIG pConfig = (PSMB_CONFIG)pData;

    BAIL_ON_INVALID_POINTER(pConfig);
    BAIL_ON_INVALID_STRING(pszName);

    *pbContinue = TRUE;

cleanup:

    return dwError;

error:

    *pbContinue = FALSE;

    goto cleanup;
}

static
DWORD
SMBSrvInitializeConfig(
    IN OUT PSMB_CONFIG pConfig
    )
{
    return 0;
}

static
DWORD
SMBSrvTransferConfigContents(
    PSMB_CONFIG pSrcConfig,
    PSMB_CONFIG pDstConfig
    )
{
    SMBSrvFreeConfigContents(pDstConfig);

    memcpy(pDstConfig, pSrcConfig, sizeof(*pSrcConfig));
    memset(pSrcConfig, 0, sizeof(*pSrcConfig));

    return 0;
}

VOID
SMBSrvFreeConfig(
    IN OUT PSMB_CONFIG pConfig
    )
{
    SMBSrvFreeConfigContents(pConfig);

    SMBFreeMemory(pConfig);
}

VOID
SMBSrvFreeConfigContents(
    IN OUT PSMB_CONFIG pConfig
    )
{
    // Nothing to do right now
}

