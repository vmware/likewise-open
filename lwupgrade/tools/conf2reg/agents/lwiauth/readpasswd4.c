/*
 * Copyright (c) Likewise Software
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */
#include "includes.h"

static
DWORD
LwiDnsGetHostInfo(
    OUT OPTIONAL PSTR* ppszHostname,
    OUT OPTIONAL PSTR* ppszFqdn,
    OUT OPTIONAL PSTR* ppszDomain
    );

static
DWORD
LwiGetDomainSid(
    PTDB_CONTEXT pTdb,
    PCSTR pszDomain,
    PSTR *ppszSid
    );

static
DWORD
LwiGetMachineAccountPasswordSchannelType(
    PTDB_CONTEXT pTdb,
    PCSTR pszDomain,
    PDWORD pdwSchannelType);

static
DWORD
LwiGetMachineAccountPasswordLastChangeTime(
    PTDB_CONTEXT pTdb,
    PCSTR pszDomain,
    time_t* pLastChangeTime);

static
DWORD
LwiAllocateMachineAccountPassword(
    PTDB_CONTEXT pTdb,
    PCSTR pszDomain,
    PSTR *ppPassword);

static
DWORD
strupr(
    PSTR sz
    );

DWORD
LwiGetMachineInformationA(
    PLWISERVERINFO pConfig,
    PCSTR pszSecretsPath,
    PLWPS_PASSWORD_INFOA pPasswordInfo
    )
{
    DWORD dwError = 0;
    PTDB_CONTEXT pTdb = NULL;

    PSTR pszMachineAccountName = NULL;
    PSTR pszMachineAccountPassword = NULL;
    PSTR pszSid = NULL;
    PSTR pszHostname = NULL;
    PSTR pszHostDnsDomain = NULL;
    PSTR pszDomainName = NULL;
    PSTR pszDnsDomainName = NULL;
    time_t LastChangeTime = 0;
    DWORD dwSchannelType = 0;

    // Examine secrets.tdb
    dwError = LwiTdbOpen(pszSecretsPath, &pTdb);
    BAIL_ON_UP_ERROR(dwError);

    dwError = LwAllocateString(pConfig->pszWorkgroup, &pszDomainName);
    BAIL_ON_UP_ERROR(dwError);

    dwError = LwAllocateString(pConfig->pszRealm, &pszDnsDomainName);
    BAIL_ON_UP_ERROR(dwError);

    dwError = LwiAllocateMachineAccountPassword(
                pTdb,
                pConfig->pszWorkgroup,
                &pszMachineAccountPassword);
    BAIL_ON_UP_ERROR(dwError);

    dwError = LwiGetMachineAccountPasswordLastChangeTime(
                pTdb,
                pConfig->pszWorkgroup,
                &LastChangeTime);
    BAIL_ON_UP_ERROR(dwError);

    dwError = LwiGetMachineAccountPasswordSchannelType(
                pTdb,
                pConfig->pszWorkgroup,
                &dwSchannelType);
    BAIL_ON_UP_ERROR(dwError);

    dwError = LwiGetDomainSid(
                pTdb,
                pConfig->pszWorkgroup,
                &pszSid);
    BAIL_ON_UP_ERROR(dwError);

    LwiTdbClose(pTdb);
    pTdb = NULL;

    // Lookup information in additional sources.
    dwError = LwiDnsGetHostInfo(&pszHostname, NULL, &pszHostDnsDomain);
    BAIL_ON_UP_ERROR(dwError);

    dwError = LwAllocateStringPrintf(&pszMachineAccountName,"%s$", pszHostname);
    BAIL_ON_UP_ERROR(dwError);

    pPasswordInfo->pszDomainName = pszDomainName;
    pPasswordInfo->pszDnsDomainName = pszDnsDomainName;
    pPasswordInfo->pszSid = pszSid;
    pPasswordInfo->pszHostname = pszHostname;
    pPasswordInfo->pszHostDnsDomain = pszHostDnsDomain;
    pPasswordInfo->pszMachineAccount = pszMachineAccountName;
    pPasswordInfo->pszMachinePassword = pszMachineAccountPassword;
    pPasswordInfo->last_change_time = LastChangeTime;
    pPasswordInfo->dwSchannelType = dwSchannelType;

cleanup:

    if (pTdb)
    {
        LwiTdbClose(pTdb);
        pTdb = NULL;
    }

    return dwError;

error:
    LW_SAFE_FREE_STRING(pszMachineAccountName);
    LW_SAFE_FREE_STRING(pszMachineAccountPassword);
    LW_SAFE_FREE_STRING(pszSid);
    LW_SAFE_FREE_STRING(pszHostname);
    LW_SAFE_FREE_STRING(pszHostDnsDomain);
    LW_SAFE_FREE_STRING(pszDomainName);
    LW_SAFE_FREE_STRING(pszDnsDomainName);
    goto cleanup;
}

VOID
LwiFreeMachineInformationContentsA(
    PLWPS_PASSWORD_INFOA pInfo
    )
{
    if (pInfo)
    {
        LW_SAFE_FREE_STRING(pInfo->pszDomainName);
        LW_SAFE_FREE_STRING(pInfo->pszDnsDomainName);
        LW_SAFE_FREE_STRING(pInfo->pszSid);
        LW_SAFE_FREE_STRING(pInfo->pszHostname);
        LW_SAFE_FREE_STRING(pInfo->pszHostDnsDomain);
        LW_SAFE_FREE_STRING(pInfo->pszMachineAccount);
        LW_SAFE_FREE_STRING(pInfo->pszMachinePassword);
    }
}

DWORD
LwiAllocateMachineInformationContentsW(
    PLWPS_PASSWORD_INFOA pInfo,
    PLWPS_PASSWORD_INFO pPasswordInfo
    )
{
    DWORD dwError = 0;

    PWSTR pwszMachineAccount = NULL;
    PWSTR pwszMachinePassword = NULL;
    PWSTR pwszSid = NULL;
    PWSTR pwszHostname = NULL;
    PWSTR pwszHostDnsDomain = NULL;
    PWSTR pwszDomainName = NULL;
    PWSTR pwszDnsDomainName = NULL;

    dwError = LwMbsToWc16s(pInfo->pszMachineAccount, &pwszMachineAccount);
    BAIL_ON_UP_ERROR(dwError);

    dwError = LwMbsToWc16s(
                pInfo->pszMachinePassword,
                &pwszMachinePassword);
    BAIL_ON_UP_ERROR(dwError);

    dwError = LwMbsToWc16s(
                pInfo->pszSid,
                &pwszSid);
    BAIL_ON_UP_ERROR(dwError);

    dwError = LwMbsToWc16s(
                pInfo->pszDomainName,
                &pwszDomainName);
    BAIL_ON_UP_ERROR(dwError);

    dwError = LwMbsToWc16s(
                pInfo->pszDnsDomainName,
                &pwszDnsDomainName);
    BAIL_ON_UP_ERROR(dwError);

    dwError = LwMbsToWc16s(
                pInfo->pszHostname,
                &pwszHostname);
    BAIL_ON_UP_ERROR(dwError);

    dwError = LwMbsToWc16s(
                pInfo->pszHostDnsDomain,
                &pwszHostDnsDomain);
    BAIL_ON_UP_ERROR(dwError);

    pPasswordInfo->pwszDomainName = pwszDomainName;
    pPasswordInfo->pwszDnsDomainName = pwszDnsDomainName;
    pPasswordInfo->pwszSID = pwszSid;
    pPasswordInfo->pwszHostname = pwszHostname;
    pPasswordInfo->pwszHostDnsDomain = pwszHostDnsDomain;
    pPasswordInfo->pwszMachineAccount = pwszMachineAccount;
    pPasswordInfo->pwszMachinePassword = pwszMachinePassword;
    pPasswordInfo->last_change_time = pInfo->last_change_time;
    pPasswordInfo->dwSchannelType = pInfo->dwSchannelType;

cleanup:

    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pwszMachineAccount);
    LW_SAFE_FREE_MEMORY(pwszMachinePassword);
    LW_SAFE_FREE_MEMORY(pwszSid);
    LW_SAFE_FREE_MEMORY(pwszDomainName);
    LW_SAFE_FREE_MEMORY(pwszDnsDomainName);
    LW_SAFE_FREE_MEMORY(pwszHostname);
    LW_SAFE_FREE_MEMORY(pwszHostDnsDomain);
    goto cleanup;
}

VOID
LwiFreeMachineInformationContentsW(
    PLWPS_PASSWORD_INFO pInfo
    )
{
    if (pInfo)
    {
        LW_SAFE_FREE_MEMORY(pInfo->pwszDomainName);
        LW_SAFE_FREE_MEMORY(pInfo->pwszDnsDomainName);
        LW_SAFE_FREE_MEMORY(pInfo->pwszSID);
        LW_SAFE_FREE_MEMORY(pInfo->pwszHostname);
        LW_SAFE_FREE_MEMORY(pInfo->pwszHostDnsDomain);
        LW_SAFE_FREE_MEMORY(pInfo->pwszMachineAccount);
        LW_SAFE_FREE_MEMORY(pInfo->pwszMachinePassword);
    }
}

static
DWORD
LwiDnsGetHostInfo(
    OUT OPTIONAL PSTR* ppszHostname,
    OUT OPTIONAL PSTR* ppszFqdn,
    OUT OPTIONAL PSTR* ppszDomain
    )
{
    DWORD dwError = 0;
    struct hostent * host = NULL;
    CHAR szBuffer[256] = { 0 };
    PSTR pszDot = NULL;
    PSTR pszFoundFqdn = NULL;
    PSTR pszFoundDomain = NULL;
    PSTR pszHostname = NULL;
    PSTR pszDomain = NULL;
    PSTR pszFqdn = NULL;

    if (!ppszHostname && !ppszFqdn && !ppszDomain)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_UP_ERROR(dwError);
    }

    if (gethostname(szBuffer, sizeof(szBuffer)-1) != 0)
    {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_UP_ERROR(dwError);
    }

    /* Test to see if the name is still dotted. If so we will chop it down to
       just the hostname field. */
    pszDot = strchr(szBuffer, '.');
    if (pszDot)
    {
        pszDot[0] = '\0';
    }

    if (ppszHostname)
    {
        dwError = LwAllocateString(szBuffer, &pszHostname);
        BAIL_ON_UP_ERROR(dwError);
    }

    if (!ppszFqdn && !ppszDomain)
    {
        // done, so bail out
        dwError = 0;
        goto error;
    }

    host = gethostbyname(szBuffer);
    if (!host)
    {
        dwError = LwMapHErrnoToLwError(h_errno);
        BAIL_ON_UP_ERROR(dwError);
    }

    //
    // We look for the first name that looks like an FQDN.  This is
    // the same heuristics used by other software such as Kerberos and
    // Samba.
    //
    pszDot = strchr(host->h_name, '.');
    if (pszDot)
    {
        pszFoundFqdn = host->h_name;
        pszFoundDomain = pszDot + 1;
    }
    else
    {
        int i;
        for (i = 0; host->h_aliases[i]; i++)
        {
            pszDot = strchr(host->h_aliases[i], '.');
            if (pszDot)
            {
                pszFoundFqdn = host->h_aliases[i];
                pszFoundDomain = pszDot + 1;
                break;
            }
        }
    }

    // If we still have nothing, just return the first name, but no Domain part.
    if (!pszFoundFqdn)
    {
        pszFoundFqdn = host->h_name;
    }
    if (pszFoundFqdn && !pszFoundFqdn[0])
    {
        pszFoundFqdn = NULL;
    }
    if (pszFoundDomain && !pszFoundDomain[0])
    {
        pszFoundDomain = NULL;
    }

    if (ppszFqdn)
    {
        if (pszFoundFqdn)
        {
            dwError = LwAllocateString(pszFoundFqdn, &pszFqdn);
            BAIL_ON_UP_ERROR(dwError);
        }
        else
        {
            dwError = ERROR_NOT_FOUND;
            BAIL_ON_UP_ERROR(dwError);
        }
    }

    if (ppszDomain)
    {
        if (pszFoundDomain)
        {
            dwError = LwAllocateString(pszFoundDomain, &pszDomain);
            BAIL_ON_UP_ERROR(dwError);
        }
        else
        {
            dwError = ERROR_NOT_FOUND;
            BAIL_ON_UP_ERROR(dwError);
        }
    }

    if (ppszHostname)
    {
        *ppszHostname = pszHostname;
    }

    if (ppszDomain)
    {
        *ppszDomain = pszDomain;
    }

    if (ppszFqdn)
    {
        *ppszFqdn = pszFqdn;
    }

cleanup:
    return dwError;

error:
    LW_SAFE_FREE_STRING(pszHostname);
    LW_SAFE_FREE_STRING(pszFqdn);
    LW_SAFE_FREE_STRING(pszDomain);
    goto cleanup;
}

static
DWORD
LwiGetDomainSid(
    PTDB_CONTEXT pTdb,
    PCSTR pszDomain,
    PSTR *ppszSid
    )
{
    DWORD dwError = 0;
    PSTR pszKey = NULL;
    PDOM_SID pSid = NULL;
    DWORD dwSize = 0;
    PSTR pszSid = NULL;

    dwError = LwAllocateStringPrintf(
                &pszKey,
                "%s/%s",
                "SECRETS/SID",
                pszDomain);
    BAIL_ON_UP_ERROR(dwError);
    strupr(pszKey);

    dwError = LwiTdbAllocateFetch(
                pTdb,
                pszKey,
                (PVOID*)&pSid,
                &dwSize);
    BAIL_ON_UP_ERROR(dwError);

    dwError = LwSidBytesToString(
                (UCHAR*)pSid,
                8 + sizeof(DWORD) * pSid->num_auths,
                &pszSid);
    BAIL_ON_UP_ERROR(dwError);

    *ppszSid = pszSid;

cleanup:
    LW_SAFE_FREE_STRING(pszKey);
    LW_SAFE_FREE_MEMORY(pSid);
    return dwError;

error:
    LW_SAFE_FREE_STRING(pszSid);
    goto cleanup;
}

static
DWORD
LwiGetMachineAccountPasswordSchannelType(
    PTDB_CONTEXT pTdb,
    PCSTR pszDomain,
    PDWORD pdwSchannelType)
{
    DWORD dwError = 0;
    PSTR pszKey = NULL;
    PDWORD pdwAllocatedSchannelType = NULL;
    DWORD dwSize = 0;

    dwError = LwAllocateStringPrintf(
                &pszKey,
                "%s/%s",
                "SECRETS/MACHINE_SEC_CHANNEL_TYPE",
                pszDomain);
    BAIL_ON_UP_ERROR(dwError);
    strupr(pszKey);

    dwError = LwiTdbAllocateFetch(
                pTdb,
                pszKey,
                (PVOID*)&pdwAllocatedSchannelType,
                &dwSize);
    BAIL_ON_UP_ERROR(dwError);

    *pdwSchannelType = *pdwAllocatedSchannelType;

cleanup:
    LW_SAFE_FREE_STRING(pszKey);
    LW_SAFE_FREE_MEMORY(pdwAllocatedSchannelType);
    return dwError;

error:
    goto cleanup;
}

static
DWORD
LwiGetMachineAccountPasswordLastChangeTime(
    PTDB_CONTEXT pTdb,
    PCSTR pszDomain,
    time_t* pLastChangeTime)
{
    DWORD dwError = 0;
    PSTR pszKey = NULL;
    time_t *pAllocatedTime = NULL;
    DWORD dwSize = 0;

    dwError = LwAllocateStringPrintf(
                &pszKey,
                "%s/%s",
                "SECRETS/MACHINE_LAST_CHANGE_TIME",
                pszDomain);
    BAIL_ON_UP_ERROR(dwError);
    strupr(pszKey);

    dwError = LwiTdbAllocateFetch(
                pTdb,
                pszKey,
                (PVOID*)&pAllocatedTime,
                &dwSize);
    BAIL_ON_UP_ERROR(dwError);

    *pLastChangeTime = *pAllocatedTime;

cleanup:
    LW_SAFE_FREE_STRING(pszKey);
    LW_SAFE_FREE_MEMORY(pAllocatedTime);
    return dwError;

error:
    goto cleanup;
}

static
DWORD
LwiAllocateMachineAccountPassword(
    PTDB_CONTEXT pTdb,
    PCSTR pszDomain,
    PSTR *ppszPassword)
{
    DWORD dwError = 0;
    PSTR pszKey = NULL;
    PSTR pszPassword = NULL;
    DWORD dwSize = 0;

    dwError = LwAllocateStringPrintf(
                &pszKey,
                "%s/%s",
                "SECRETS/MACHINE_PASSWORD", // SECRETS/$MACHINE.ACC <- NT4 hash
                pszDomain);
    BAIL_ON_UP_ERROR(dwError);

    strupr(pszKey);

    dwError = LwiTdbAllocateFetch(pTdb, pszKey, (PVOID*)&pszPassword, &dwSize);
    BAIL_ON_UP_ERROR(dwError);

    *ppszPassword = pszPassword;

cleanup:
    LW_SAFE_FREE_STRING(pszKey);
    return dwError;

error:
    LW_SAFE_FREE_STRING(pszPassword);
    goto cleanup;
}

#if 0
static
DWORD
LwiAllocateMachineAccountName(
    PTDB_CONTEXT pTdb,
    PCSTR pszDomain,
    PSTR *ppszName)
{
    DWORD dwError = 0;
    PSTR pszKey = NULL;
    PSTR pszName = NULL;
    DWORD dwSize = 0;

    dwError = LwAllocateStringPrintf(
                &pszKey,
                "%s/%s",
                "SECRETS/SECRETS_MACHINE_TRUST_ACCOUNT_NAME",
                pszDomain);
    BAIL_ON_UP_ERROR(dwError);

    strupr(pszKey);

    dwError = LwiTdbAllocateFetch(pTdb, pszKey, (PVOID*)&pszName, &dwSize);
    BAIL_ON_UP_ERROR(dwError);

    *ppszName = pszName;

cleanup:
    LW_SAFE_FREE_STRING(pszKey);
    return dwError;

error:
    LW_SAFE_FREE_STRING(pszName);
    goto cleanup;
}
#endif

static
DWORD
strupr(
    PSTR sz
    )
{
    if (sz)
    {
        while (*sz != '\0')
        {
            *sz = toupper(*sz);
            sz++;
        }
    }
    return 0;
}

#if 0
DWORD
MovePassword(
    PSTR pszDomainName
    )
{
    DWORD dwError = 0;
    HANDLE hStore = NULL;
    PLWPS_PASSWORD_INFO pInfo = NULL;

    dwError = LwpsOpenPasswordStore(LWPS_PASSWORD_STORE_TDB, &hStore);
    BAIL_ON_UP_ERROR(dwError);

    dwError = LwpsGetPasswordByDomainName(hStore, pszDomainName, &pInfo);
    BAIL_ON_UP_ERROR(dwError);

    dwError = LwpsClosePasswordStore(hStore);
    BAIL_ON_UP_ERROR(dwError);
    hStore = NULL;

    dwError = LwiDnsGetHostInfo(&pszHostname, NULL, &pszHostDnsDomain);
    BAIL_ON_UP_ERROR(dwError);

    if (!pInfo->pwszHostname)
    {
        dwError = LwMbsToWc16s(pszHostname, &pInfo->pwszHostname);
        BAIL_ON_UP_ERROR(dwError);
    }

    if (!pInfo->pwszHostDnsDomain)
    {
        dwError = LwMbsToWc16s(pszHostDnsDomain, &pInfo->pwszHostDnsDomain);
        BAIL_ON_UP_ERROR(dwError);
    }

    if (!pInfo->pwszMachineAccount)
    {
        dwError = LwAllocateStringPrintf(
                    &pszMachineAccount,
                    "%s$",
                    pszHostname);
        BAIL_ON_UP_ERROR(dwError);

        dwError = LwMbsToWc16s(pszMachineAccount, &pInfo->pwszMachineAccount);
        BAIL_ON_UP_ERROR(dwERror);
    }

    dwError = LwpsWritePasswordToAllStores(pInfo);
    BAIL_ON_UP_ERROR(dwError);

cleanup:
    return dwError;
error:
    goto cleanup;
}
#endif
