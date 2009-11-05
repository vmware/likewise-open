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
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software 2007
 * All rights reserved.
 *
 * Authors:
 *
 */
#include "includes.h"

static
DWORD
LwiConfigPrint(
    FILE *fp,
    PLWISERVERINFO pConfig
    );

static
VOID
LwiConfigFreeContents(
    PLWISERVERINFO pConfig
    );

static
DWORD
LwiSetConfigDefaults(
    PLWISERVERINFO pConfig
    )
{
    DWORD dwError = 0;

    LW_SAFE_FREE_STRING(pConfig->pszWorkgroup);
    pConfig->pszWorkgroup = NULL;

    LW_SAFE_FREE_STRING(pConfig->pszRealm);
    pConfig->pszRealm = NULL;

    LW_SAFE_FREE_STRING(pConfig->pszShellTemplate);
    dwError = LwAllocateString("/bin/bash", &pConfig->pszShellTemplate);
    BAIL_ON_UP_ERROR(dwError);

    LW_SAFE_FREE_STRING(pConfig->pszHomeDirTemplate);
    dwError = LwAllocateString("/home/%D/%U", &pConfig->pszHomeDirTemplate);
    BAIL_ON_UP_ERROR(dwError);

cleanup:
    return dwError;
error:
    LwiConfigFreeContents(pConfig);
    goto cleanup;
}

static
VOID
LwiConfigFreeContents(
    PLWISERVERINFO pConfig
    )
{
    LW_SAFE_FREE_STRING(pConfig->pszWorkgroup);
    LW_SAFE_FREE_STRING(pConfig->pszRealm);
    LW_SAFE_FREE_STRING(pConfig->pszShellTemplate);
    LW_SAFE_FREE_STRING(pConfig->pszHomeDirTemplate);
}

/* call back functions to get the values from config file */
static
DWORD
LwiConfigSectionHandler(
    BOOLEAN bSectionStart,
    PCSTR pszSectionName,
    PVOID pData,
    PBOOLEAN pbContinue
    )
{
    DWORD dwError = 0;

    *pbContinue = TRUE;

    return dwError;
}

static
DWORD
LwiConfigNameValuePair(
    PCSTR pszSectionName,
    PCSTR pszName,
    PCSTR pszValue,
    PVOID pData,
    PBOOLEAN pbContinue
    )
{
    DWORD dwError = 0;
    PLWISERVERINFO pConfig = (PLWISERVERINFO)pData;
    PSTR pszWorkgroup = NULL;
    PSTR pszRealm = NULL;
    PSTR pszShellTemplate = NULL;
    PSTR pszHomeDirTemplate = NULL;

    if (!strcmp(pszName, "workgroup"))
    {
        dwError = LwAllocateString(pszValue, &pszWorkgroup);
        BAIL_ON_UP_ERROR(dwError);

        LW_SAFE_FREE_STRING(pConfig->pszWorkgroup);
        pConfig->pszWorkgroup = pszWorkgroup;
        pszWorkgroup = NULL;
    }
    else if (!strcmp(pszName, "realm"))
    {
        dwError = LwAllocateString(pszValue, &pszRealm);
        BAIL_ON_UP_ERROR(dwError);

        LW_SAFE_FREE_STRING(pConfig->pszRealm);
        pConfig->pszRealm = pszRealm;
        pszRealm = NULL;
    }
    else if (!strcmp(pszName, "template shell"))
    {
        dwError = LwAllocateString(pszValue, &pszShellTemplate);
        BAIL_ON_UP_ERROR(dwError);

        LW_SAFE_FREE_STRING(pConfig->pszShellTemplate);
        pConfig->pszShellTemplate = pszShellTemplate;
        pszShellTemplate = NULL;
    }
    else if (!strcmp(pszName, "template homedir"))
    {
        dwError = LwAllocateString(pszValue, &pszHomeDirTemplate);
        BAIL_ON_UP_ERROR(dwError);

        LW_SAFE_FREE_STRING(pConfig->pszHomeDirTemplate);
        pConfig->pszHomeDirTemplate = pszHomeDirTemplate;
        pszHomeDirTemplate = NULL;
    }

    *pbContinue = TRUE;

cleanup:

    LW_SAFE_FREE_STRING(pszWorkgroup);
    LW_SAFE_FREE_STRING(pszRealm);
    LW_SAFE_FREE_STRING(pszShellTemplate);
    LW_SAFE_FREE_STRING(pszHomeDirTemplate);

    return dwError;

error:
    goto cleanup;
}

DWORD
LwiauthConfFileToRegFile(
    PCSTR pszConfFile,
    PCSTR pszSecretsFile,
    PCSTR pszRegFile
    )
{
    DWORD dwError = 0;
    LWISERVERINFO Config;
    LWPS_PASSWORD_INFOA PassInfoA;
    LWPS_PASSWORD_INFO PassInfoW;
    FILE *fp = NULL;

    memset(&Config, 0, sizeof(Config));
    memset(&PassInfoA, 0, sizeof(PassInfoA));
    memset(&PassInfoW, 0, sizeof(PassInfoW));

    fp = fopen(pszRegFile, "w");
    if (!fp)
    {
        dwError = LwMapErrnoToLwError(errno);
    }
    BAIL_ON_UP_ERROR(dwError);

    dwError = LwiSetConfigDefaults(&Config);
    BAIL_ON_UP_ERROR(dwError);

    dwError = UpParseSambaConfigFile(
                pszConfFile,
                &LwiConfigSectionHandler,
                &LwiConfigNameValuePair,
                &Config);
    BAIL_ON_UP_ERROR(dwError);

    dwError = LwiGetMachineInformationA(&Config, pszSecretsFile, &PassInfoA);
    BAIL_ON_UP_ERROR(dwError);

#if 0
    fprintf(stdout, "MachineAccount:%s\n", PassInfoA.pszMachineAccount);
    fprintf(stdout, "MachinePassword:%s\n", PassInfoA.pszMachinePassword);
    fprintf(stdout, "Hostname:%s\n", PassInfoA.pszHostname);
    fprintf(stdout, "HostDnsDomain:%s\n", PassInfoA.pszHostDnsDomain);
    fprintf(stdout, "DomainName:%s\n",  PassInfoA.pszDomainName);
    fprintf(stdout, "DnsDomainName:%s\n", PassInfoA.pszDnsDomainName);
    fprintf(stdout, "SID:%s\n", PassInfoA.pszSid);
    fprintf(stdout, "LastChangeTime:%lu\n", (unsigned long)PassInfoA.last_change_time);
    fprintf(stdout, "SchannelType:%lu\n", (unsigned long)PassInfoA.dwSchannelType);
#endif

    dwError = LwiAllocateMachineInformationContentsW(
                &PassInfoA,
                &PassInfoW);
    BAIL_ON_UP_ERROR(dwError);

    dwError = LwpsWritePasswordToAllStores(&PassInfoW);
    BAIL_ON_UP_ERROR(dwError);

    dwError = LwiConfigPrint(fp, &Config);
    BAIL_ON_UP_ERROR(dwError);

cleanup:

    if (fp)
    {
        fclose(fp);
    }

    LwiConfigFreeContents(&Config);
    LwiFreeMachineInformationContentsA(&PassInfoA);
    LwiFreeMachineInformationContentsW(&PassInfoW);

    return dwError;

error:

    goto cleanup;
}

static
DWORD
LwiConfigPrint(
    FILE *fp,
    PLWISERVERINFO pConfig
    )
{
    DWORD dwError = 0;

    if (fputs(
            "[HKEY_THIS_MACHINE\\Services]\n\n"
            "[HKEY_THIS_MACHINE\\Services\\lsass]\n\n"
            "[HKEY_THIS_MACHINE\\Services\\lsass\\Parameters]\n\n"
            "[HKEY_THIS_MACHINE\\Services\\lsass\\Parameters\\Providers]\n\n"
            "[HKEY_THIS_MACHINE\\Services\\lsass\\Parameters\\Providers\\ActiveDirectory]\n",
            fp) < 0)
    {
        dwError = LwMapErrnoToLwError(errno);
    }
    BAIL_ON_UP_ERROR(dwError);

    dwError = UpPrintString(fp, "LoginShellTemplate", pConfig->pszShellTemplate);
    BAIL_ON_UP_ERROR(dwError);

    dwError = UpPrintString(fp, "HomeDirTemplate", pConfig->pszHomeDirTemplate);
    BAIL_ON_UP_ERROR(dwError);

error:
    return dwError;
}

