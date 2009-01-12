/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see 
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

#include "iop.h"

#define IOP_CONFIG_TAG_DRIVER "driver:"

static
VOID
IopConfigFreeDriverConfig(
    IN OUT PIOP_DRIVER_CONFIG* ppDriverConfig
    )
{
    PIOP_DRIVER_CONFIG pDriverConfig = *ppDriverConfig;

    if (pDriverConfig)
    {
        IO_FREE(&pDriverConfig->pszName);
        IO_FREE(&pDriverConfig->pszPath);
        IoMemoryFree(pDriverConfig);
        *ppDriverConfig = NULL;
    }
}

static
DWORD
IopConfigParseStartSection(
    IN PCSTR pszSectionName,
    IN PVOID pData,
    OUT PBOOLEAN pbSkipSection,
    OUT PBOOLEAN pbContinue
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    BOOLEAN bContinue = TRUE;
    BOOLEAN bSkipSection = FALSE;
    PIOP_CONFIG_PARSE_STATE pState = (PIOP_CONFIG_PARSE_STATE) pData;
    PIOP_DRIVER_CONFIG pDriverConfig = NULL;
    PCSTR pszName = NULL;
    PLW_LIST_LINKS pLinks = NULL;

    assert(pszSectionName);
    assert(pState);
    assert(!pState->pDriverConfig);

    SMB_LOG_DEBUG("Section = '%s'", pszSectionName);

    if (strncasecmp(pszSectionName, IOP_CONFIG_TAG_DRIVER, sizeof(IOP_CONFIG_TAG_DRIVER)-1))
    {
        bSkipSection = TRUE;
        GOTO_CLEANUP_EE(EE);
    }

    pszName = pszSectionName + sizeof(IOP_CONFIG_TAG_DRIVER) - 1;
    if (IsNullOrEmptyString(pszName))
    {
        SMB_LOG_ERROR("No driver name was specified");

        status = STATUS_UNSUCCESSFUL;
        GOTO_CLEANUP_EE(EE);
    }

    // Check for duplicate driver config.
    for (pLinks = pState->pConfig->DriverConfigList.Next;
         pLinks != &pState->pConfig->DriverConfigList;
         pLinks = pLinks->Next)
    {
        PIOP_DRIVER_CONFIG pCheckDriverConfig = LW_STRUCT_FROM_FIELD(pLinks, IOP_DRIVER_CONFIG, Links);
        if (!strcasecmp(pCheckDriverConfig->pszName, pszName))
        {
            SMB_LOG_ERROR("Duplicate driver name '%s'", pszName);

            status = STATUS_UNSUCCESSFUL;
            GOTO_CLEANUP_EE(EE);
        }
    }

    status = IO_ALLOCATE(&pDriverConfig, IOP_DRIVER_CONFIG, sizeof(*pDriverConfig));
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = IoCStringDuplicate(&pDriverConfig->pszName, pszName);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    LwListInsertTail(&pState->pConfig->DriverConfigList, &pDriverConfig->Links);
    pState->pConfig->DriverCount++;

    pState->pDriverConfig = pDriverConfig;

cleanup:
    if (status)
    {
        pState->Status = status;

        IopConfigFreeDriverConfig(&pDriverConfig);

        bContinue = FALSE;
        bSkipSection = TRUE;
    }
    
    *pbSkipSection = bSkipSection;
    *pbContinue = bContinue;

    // TODO -- Error code mismatch?
    return status;
}

static
DWORD
IopConfigParseEndSection(
    IN PCSTR pszSectionName,
    IN PVOID pData,
    OUT PBOOLEAN pbContinue
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    PIOP_CONFIG_PARSE_STATE pState = (PIOP_CONFIG_PARSE_STATE) pData;
    BOOLEAN bContinue = TRUE;

    assert(pszSectionName);
    assert(pState);
    assert(pState->pDriverConfig);

    SMB_LOG_DEBUG("Section = '%s'", pszSectionName);

    // Finished last driver, if any.
    if (pState->pDriverConfig)
    {
        if (!pState->pDriverConfig->pszPath)
        {
            status = STATUS_UNSUCCESSFUL;
            GOTO_CLEANUP_EE(EE);
        }
        pState->pDriverConfig = NULL;
    }

cleanup:
    if (status)
    {
        pState->Status = status;
        bContinue = FALSE;
    }
    
    *pbContinue = bContinue;

    // TODO -- Error code mismatch?
    return status;

}

static
DWORD
IopConfigParseNameValuePair(
    IN PCSTR pszName,
    IN PCSTR pszValue,
    IN PVOID pData,
    OUT PBOOLEAN pbContinue
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    PIOP_CONFIG_PARSE_STATE pState = (PIOP_CONFIG_PARSE_STATE) pData;
    BOOLEAN bContinue = TRUE;

    assert(pszName);
    assert(pszValue);
    assert(pState);
    assert(pState->pDriverConfig);
    assert(pState->pDriverConfig->pszName);

    SMB_LOG_DEBUG("Driver = '%s', Name = '%s', Value = '%s'",
                  pState->pDriverConfig->pszName,
                  pszName,
                  pszValue);

    if (strcasecmp(pszName, "path"))
    {
        GOTO_CLEANUP_EE(EE);
    }

    if (pState->pDriverConfig->pszPath)
    {
        SMB_LOG_ERROR("Path for driver '%s' is already defined as '%s'",
                      pState->pDriverConfig->pszName,
                      pState->pDriverConfig->pszPath);

        status = STATUS_UNSUCCESSFUL;
        GOTO_CLEANUP_EE(EE);
    }

    if (IsNullOrEmptyString(pszValue))
    {
        SMB_LOG_ERROR("Empty path for driver '%s'",
                      pState->pDriverConfig->pszName);

        status = STATUS_UNSUCCESSFUL;
        GOTO_CLEANUP_EE(EE);
    }

    status = IoCStringDuplicate(&pState->pDriverConfig->pszPath,
                                pszValue);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

cleanup:
    if (status)
    {
        pState->Status = status;

        bContinue = FALSE;
    }

    *pbContinue = bContinue;

    // TODO -- Error code mismatch?
    return status;
}

VOID
IopConfigFreeConfig(
    IN OUT PIOP_CONFIG* ppConfig
    )
{
    PIOP_CONFIG pConfig = *ppConfig;
    if (pConfig)
    {
        PLW_LIST_LINKS pLinks = NULL;

        for (pLinks = pConfig->DriverConfigList.Next;
             pLinks != &pConfig->DriverConfigList;
             pLinks = pLinks->Next)
        {
            PIOP_DRIVER_CONFIG pDriverConfig = LW_STRUCT_FROM_FIELD(pLinks, IOP_DRIVER_CONFIG, Links);

            LwListRemove(pLinks);
            IopConfigFreeDriverConfig(&pDriverConfig);
        }
        IoMemoryFree(pConfig);
        *ppConfig = NULL;
    }
}

NTSTATUS
IopConfigParse(
    OUT PIOP_CONFIG* ppConfig,
    IN PCSTR pszConfigFilePath
    )
{
    NTSTATUS status = 0;
    DWORD dwError = 0;
    PIOP_CONFIG pConfig = NULL;
    IOP_CONFIG_PARSE_STATE parseState = { 0 };

    status = IO_ALLOCATE(&pConfig, IOP_CONFIG, sizeof(*pConfig));
    GOTO_CLEANUP_ON_STATUS(status);

    LwListInit(&pConfig->DriverConfigList);

    parseState.pConfig = pConfig;

    dwError = SMBParseConfigFile(
                    pszConfigFilePath,
                    SMB_CFG_OPTION_STRIP_ALL,
                    IopConfigParseStartSection,
                    NULL,
                    IopConfigParseNameValuePair,
                    IopConfigParseEndSection,
                    &parseState);
    if (dwError)
    {
        // TODO-Error code issues?
        status = STATUS_UNSUCCESSFUL;
        GOTO_CLEANUP_ON_STATUS(status);
    }

    status = parseState.Status;
    GOTO_CLEANUP_ON_STATUS(status);

    assert(!parseState.pDriverConfig);

cleanup:
    assert(!(dwError && !status));

    if (status)
    {
        IopConfigFreeConfig(&pConfig);
    }

    *ppConfig = pConfig;

    return status;
}
