/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
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

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        vfs_provider.c
 *
 * Abstract:
 *
 *        Likewise Server Message Block (LSMB)
 *
 *        NT VFS  Provider Management API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "includes.h"

static
DWORD
IOMgrProviderConfigStartSection(
    PCSTR    pszSectionName,
    PVOID    pData,
    PBOOLEAN pbSkipSection,
    PBOOLEAN pbContinue
    );

static
DWORD
IOMgrProviderConfigNameValuePair(
    PCSTR    pszName,
    PCSTR    pszValue,
    PVOID    pData,
    PBOOLEAN pbContinue
    );

static
DWORD
IOMgrInitProvider(
    PCSTR pszConfigFilePath,
    PNTVFS_PROVIDER pProvider
    );

static
DWORD
IOMgrValidateProvider(
    PNTVFS_PROVIDER pProvider
    );

static
VOID
IOMgrFreeProviderStack(
    PSMB_STACK pProviderStack
    );

static
DWORD
IOMgrFreeProviderInStack(
    PVOID pItem,
    PVOID pUserData
    );

static
VOID
IOMgrFreeProviderArray(
    PNTVFS_PROVIDER pProviderArray,
    DWORD         dwNumProviders
    );

static
VOID
IOMgrFreeProvider(
    PNTVFS_PROVIDER pProvider
    );

static
VOID
IOMgrFreeProviderContents(
    PNTVFS_PROVIDER pProvider
    );

DWORD
IOMgrInitProviders(
    PCSTR pszConfigFilePath
    )
{
    DWORD dwError = 0;
    PNTVFS_PROVIDER pProvider = NULL;
    PSMB_STACK pProviderStack = NULL;

    PSMB_STACK pFinalProviderStack = NULL;
    PNTVFS_PROVIDER pProviderArray = NULL;
    DWORD dwNumProviders = 0;

    dwError = SMBParseConfigFile(
                    pszConfigFilePath,
                    SMB_CFG_OPTION_STRIP_ALL,
                    &IOMgrProviderConfigStartSection,
                    NULL,
                    &IOMgrProviderConfigNameValuePair,
                    NULL,
                    (PVOID)&pProviderStack);
    BAIL_ON_SMB_ERROR(dwError);

    pProvider = (PNTVFS_PROVIDER) SMBStackPop(&pProviderStack);

    while (pProvider)
    {
        dwError = IOMgrInitProvider(pszConfigFilePath, pProvider);

        if (dwError)
        {
            SMB_LOG_ERROR("Failed to load provider [%s] at [%s] [error code:%d]",
                            SMB_SAFE_LOG_STRING(pProvider->pszName),
                            SMB_SAFE_LOG_STRING(pProvider->pszProviderLibpath),
                            dwError);

            IOMgrFreeProvider(pProvider);
            pProvider = NULL;
            dwError = 0;
        }
        else
        {
            // Add to good provider stack
            dwError = SMBStackPush(
                            pProvider,
                            &pFinalProviderStack);
            BAIL_ON_SMB_ERROR(dwError);

            dwNumProviders++;
        }

        pProvider = (PNTVFS_PROVIDER) SMBStackPop(&pProviderStack);
    }

    if (dwNumProviders)
    {
        DWORD iProvider = 0;

        // Null terminated array of provider pointers
        dwError = SMBAllocateMemory(
                        sizeof(NTVFS_PROVIDER) * dwNumProviders,
                        (PVOID*)&pProviderArray);
        BAIL_ON_SMB_ERROR(dwError);

        // enforce order from config file
        pFinalProviderStack = SMBStackReverse(pFinalProviderStack);

        while ((pProvider = (PNTVFS_PROVIDER) SMBStackPop(&pFinalProviderStack)) != NULL)
        {
            memcpy(&pProviderArray[iProvider++], pProvider, sizeof(NTVFS_PROVIDER));

            SMBFreeMemory(pProvider);
        }
    }

    gpVFSProviderArray = pProviderArray;
    gdwNumVFSProviders = dwNumProviders;

cleanup:

    if (pProviderStack)
    {
        IOMgrFreeProviderStack(pProviderStack);
    }

    if (pFinalProviderStack)
    {
        IOMgrFreeProviderStack(pFinalProviderStack);
    }

    return dwError;

error:

    if (pProviderArray) {
        IOMgrFreeProviderArray(pProviderArray, dwNumProviders);
    }

    goto cleanup;
}

VOID
IOMgrFreeProviders(
    VOID
    )
{
    if (gdwNumVFSProviders)
    {
        IOMgrFreeProviderArray(gpVFSProviderArray, gdwNumVFSProviders);
    }
}

static
DWORD
IOMgrProviderConfigStartSection(
    PCSTR    pszSectionName,
    PVOID    pData,
    PBOOLEAN pbSkipSection,
    PBOOLEAN pbContinue
    )
{
    DWORD dwError = 0;
    PSMB_STACK* ppProviderStack = (PSMB_STACK*)pData;
    PNTVFS_PROVIDER pProvider = NULL;
    BOOLEAN bContinue = TRUE;
    BOOLEAN bSkipSection = FALSE;
    PCSTR   pszLibName = NULL;

    BAIL_ON_INVALID_POINTER(ppProviderStack);

    if (IsNullOrEmptyString(pszSectionName) ||
        strncasecmp(pszSectionName, IOMGR_CFG_TAG_VFS_PROVIDER, sizeof(IOMGR_CFG_TAG_VFS_PROVIDER)-1))
    {
        bSkipSection = TRUE;
        goto done;
    }

    pszLibName = pszSectionName + sizeof(IOMGR_CFG_TAG_VFS_PROVIDER) - 1;
    if (IsNullOrEmptyString(pszLibName)) {
        SMB_LOG_WARNING("No vfs provider Plugin name was specified");
        bSkipSection = TRUE;
        goto done;
    }

    dwError = SMBAllocateMemory(
                sizeof(NTVFS_PROVIDER),
                (PVOID*)&pProvider);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBAllocateString(
                pszLibName,
                &pProvider->pszId);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBStackPush(
                pProvider,
                ppProviderStack
                );
    BAIL_ON_SMB_ERROR(dwError);

done:

    *pbSkipSection = bSkipSection;
    *pbContinue = bContinue;

cleanup:

    return dwError;

error:

    *pbContinue = FALSE;
    *pbSkipSection = TRUE;

    if (pProvider)
    {
        IOMgrFreeProvider(pProvider);
    }

    goto cleanup;
}

static
DWORD
IOMgrProviderConfigNameValuePair(
    PCSTR    pszName,
    PCSTR    pszValue,
    PVOID    pData,
    PBOOLEAN pbContinue
    )
{
    DWORD dwError = 0;
    PSMB_STACK* ppProviderStack = (PSMB_STACK*)pData;
    PNTVFS_PROVIDER pProvider = NULL;
    PSTR pszProviderLibpath = NULL;

    BAIL_ON_INVALID_POINTER(ppProviderStack);

    pProvider = (PNTVFS_PROVIDER) SMBStackPeek(*ppProviderStack);

    if (!pProvider)
    {
        dwError = SMB_ERROR_INTERNAL;
        BAIL_ON_SMB_ERROR(dwError);
    }

    if (strcasecmp(pszName, "path") == 0)
    {
        if (!IsNullOrEmptyString(pszValue)) {
            dwError = SMBAllocateString(
                        pszValue,
                        &pszProviderLibpath);
            BAIL_ON_SMB_ERROR(dwError);
        }

        //don't allow redefinition a value within a section.
        if (pProvider->pszProviderLibpath != NULL)
        {
            SMB_LOG_WARNING("path redefined in configuration file");
            SMB_SAFE_FREE_STRING(pProvider->pszProviderLibpath);
        }

        pProvider->pszProviderLibpath = pszProviderLibpath;
        pszProviderLibpath = NULL;
    }

    *pbContinue = TRUE;

cleanup:

    return dwError;

error:

    SMB_SAFE_FREE_STRING(pszProviderLibpath);

    *pbContinue = FALSE;

    goto cleanup;
}

static
DWORD
IOMgrInitProvider(
    PCSTR pszConfigFilePath,
    PNTVFS_PROVIDER pProvider
    )
{
    DWORD dwError = 0;
    PFNVFSINITIALIZEPROVIDER pfnInitProvider = NULL;
    PCSTR  pszError = NULL;
    PSTR pszProviderLibpath = NULL;

    if (IsNullOrEmptyString(pProvider->pszProviderLibpath)) {
        dwError = ENOENT;
        BAIL_ON_SMB_ERROR(dwError);
    }

    pszProviderLibpath = pProvider->pszProviderLibpath;

    dlerror();

    pProvider->pLibHandle = dlopen(pszProviderLibpath, RTLD_NOW | RTLD_GLOBAL);
    if (pProvider->pLibHandle == NULL) {
       SMB_LOG_ERROR("Failed to open vfs provider at path [%s]", pszProviderLibpath);

       pszError = dlerror();
       if (!IsNullOrEmptyString(pszError)) {
          SMB_LOG_ERROR("%s", pszError);
       }

       dwError = SMB_ERROR_INVALID_VFS_PROVIDER;
       BAIL_ON_SMB_ERROR(dwError);
    }

    dlerror();
    pfnInitProvider = (PFNVFSINITIALIZEPROVIDER)dlsym(
                                        pProvider->pLibHandle,
                                        IOMGR_SYMBOL_NAME_INITIALIZE_PROVIDER);
    if (pfnInitProvider == NULL) {
       SMB_LOG_ERROR("Ignoring invalid vfs provider at path [%s]", pszProviderLibpath);

       pszError = dlerror();
       if (!IsNullOrEmptyString(pszError)) {
          SMB_LOG_ERROR("%s", pszError);
       }

       dwError = SMB_ERROR_INVALID_VFS_PROVIDER;
       BAIL_ON_SMB_ERROR(dwError);
    }

    dlerror();
    pProvider->pFnShutdown = (PFNVFSSHUTDOWNPROVIDER)dlsym(
                                        pProvider->pLibHandle,
                                        IOMGR_SYMBOL_NAME_SHUTDOWN_PROVIDER);
    if (pProvider->pFnShutdown == NULL) {
       SMB_LOG_ERROR("Ignoring invalid vfs provider at path [%s]", pszProviderLibpath);

       pszError = dlerror();
       if (!IsNullOrEmptyString(pszError)) {
          SMB_LOG_ERROR("%s", pszError);
       }

       dwError = SMB_ERROR_INVALID_VFS_PROVIDER;
       BAIL_ON_SMB_ERROR(dwError);
    }

    dwError = pfnInitProvider(
                    pszConfigFilePath,
                    &pProvider->pszName,
                    &pProvider->pFnTable);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = IOMgrValidateProvider(pProvider);
    BAIL_ON_SMB_ERROR(dwError);

cleanup:

    return dwError;

error:

    goto cleanup;
}

static
DWORD
IOMgrValidateProvider(
    PNTVFS_PROVIDER pProvider
    )
{
    if (!pProvider ||
        !pProvider->pFnTable ||
        !pProvider->pFnTable->pfnVFSLWCreateFile ||
        !pProvider->pFnTable->pfnVFSLWReadFile ||
        !pProvider->pFnTable->pfnVFSLWWriteFile ||
        !pProvider->pFnTable->pfnVFSLWGetSessionKey ||
        !pProvider->pFnTable->pfnVFSLWCloseFile ||
        !pProvider->pFnTable->pfnVFSTreeConnect ||
        !pProvider->pFnTable->pfnVFSNTCreate ||
        !pProvider->pFnTable->pfnVFSNTTransactCreate ||
        !pProvider->pFnTable->pfnVFSCreateTemporary ||
        !pProvider->pFnTable->pfnVFSReadFile ||
        !pProvider->pFnTable->pfnVFSWriteFile ||
        !pProvider->pFnTable->pfnVFSLockFile ||
        !pProvider->pFnTable->pfnVFSSeekFile ||
        !pProvider->pFnTable->pfnVFSFlushFile ||
        !pProvider->pFnTable->pfnVFSCloseFile ||
        !pProvider->pFnTable->pfnVFSCloseFileAndDisconnect ||
        !pProvider->pFnTable->pfnVFSDeleteFile ||
        !pProvider->pFnTable->pfnVFSRenameFile ||
        !pProvider->pFnTable->pfnVFSCopyFile ||
        !pProvider->pFnTable->pfnVFSTrans2QueryFileInformation ||
        !pProvider->pFnTable->pfnVFSTrans2SetPathInformation ||
        !pProvider->pFnTable->pfnVFSTrans2QueryPathInformation ||
        !pProvider->pFnTable->pfnVFSTrans2CreateDirectory ||
        !pProvider->pFnTable->pfnVFSDeleteDirectory ||
        !pProvider->pFnTable->pfnVFSCheckDirectory ||
        !pProvider->pFnTable->pfnVFSTrans2FindFirst2 ||
        !pProvider->pFnTable->pfnVFSTrans2FindNext2 ||
        !pProvider->pFnTable->pfnVFSNTTransactNotifyChange ||
        !pProvider->pFnTable->pfnVFSTrans2GetDfsReferral
        )
    {
        return SMB_ERROR_INVALID_VFS_PROVIDER;
    }

    return 0;
}

static
VOID
IOMgrFreeProviderStack(
    PSMB_STACK pProviderStack
    )
{
    SMBStackForeach(
                pProviderStack,
                &IOMgrFreeProviderInStack,
                NULL);
    SMBStackFree(pProviderStack);
}

static
DWORD
IOMgrFreeProviderInStack(
    PVOID pItem,
    PVOID pUserData
    )
{
    if (pItem)
    {
        IOMgrFreeProvider((PNTVFS_PROVIDER)pItem);
    }

    return 0;
}

static
VOID
IOMgrFreeProviderArray(
    PNTVFS_PROVIDER pProviderArray,
    DWORD dwNumProviders
    )
{
    DWORD iProvider = 0;

    for (; iProvider < dwNumProviders; iProvider++)
    {
        PNTVFS_PROVIDER pProvider = &pProviderArray[iProvider];

        IOMgrFreeProviderContents(pProvider);
    }

    SMBFreeMemory(pProviderArray);
}

static
VOID
IOMgrFreeProvider(
    PNTVFS_PROVIDER pProvider
    )
{
    IOMgrFreeProviderContents(pProvider);
    SMBFreeMemory(pProvider);
}

static
VOID
IOMgrFreeProviderContents(
    PNTVFS_PROVIDER pProvider
    )
{
    if (pProvider) {

        SMB_SAFE_FREE_STRING(pProvider->pszProviderLibpath);

        if (pProvider->pFnShutdown) {

           pProvider->pFnShutdown(
                       pProvider->pszName,
                       pProvider->pFnTable
                       );

        }

        if (pProvider->pLibHandle) {
           dlclose(pProvider->pLibHandle);
        }

        SMB_SAFE_FREE_STRING(pProvider->pszId);
    }
}

