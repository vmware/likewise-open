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
 *        join.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 * 
 *        Join to Active Directory
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "includes.h"

#define LSA_JOIN_OU_PREFIX "OU="
#define LSA_JOIN_DC_PREFIX "DC="

DWORD
LsaNetJoinDomain(
    PCSTR pszHostname,
    PCSTR pszDomain,
    PCSTR pszOU,
    PCSTR pszUsername,
    PCSTR pszPassword,
    PCSTR pszOSName,
    PCSTR pszOSVersion,
    DWORD dwFlags
    )
{
    DWORD dwError = 0;
    PSTR  pszOU_DN = NULL;
    PWSTR pwszHostname = NULL;
    PWSTR pwszDomain = NULL;
    PWSTR pwszOU = NULL;
    PWSTR pwszOSName = NULL;
    PWSTR pwszOSVersion = NULL;
    DWORD dwOptions = (NETSETUP_JOIN_DOMAIN |
                       NETSETUP_ACCT_CREATE |
                       NETSETUP_DOMAIN_JOIN_IF_JOINED);
    krb5_error_code ret = 0;
    krb5_context ctx = NULL;
    krb5_ccache cc = NULL;
    PSTR pszOldCachePath = NULL;
    PCSTR pszNewCachePath = NULL;
    
    BAIL_ON_INVALID_STRING(pszHostname);
    BAIL_ON_INVALID_STRING(pszDomain);
    BAIL_ON_INVALID_STRING(pszUsername);

    dwError = NpcInit();
    BAIL_ON_LSA_ERROR(dwError);
    
    if (geteuid() != 0) {
        dwError = EACCES;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if ( !(dwFlags & LSA_NET_JOIN_DOMAIN_NOTIMESYNC) )
    {    
        dwError = LsaSyncTimeToDC(pszDomain);
        BAIL_ON_LSA_ERROR(dwError);
    }

    ret = krb5_init_context(&ctx);
    BAIL_ON_KRB_ERROR(ctx, ret);

    /* Generates a new filed based credentials cache in /tmp. The file will
     * be owned by root and only accessible by root.
     */
    ret = krb5_cc_new_unique(
            ctx, 
            "FILE",
            "hint",
            &cc);
    BAIL_ON_KRB_ERROR(ctx, ret);

    pszNewCachePath = krb5_cc_get_name(ctx, cc);
    
    dwError = LsaKrb5GetTgt(
		pszUsername,
		pszPassword,
		pszNewCachePath,
                NULL);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaKrb5SetDefaultCachePath(
		pszNewCachePath,
                NULL);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = NpcGetCredCacheName(&pszOldCachePath);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = NpcSetCredCacheName(pszNewCachePath);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaMbsToWc16s(
                    pszHostname,
                    &pwszHostname);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaMbsToWc16s(
                    pszDomain,
                    &pwszDomain);
    BAIL_ON_LSA_ERROR(dwError);
    
    if (!IsNullOrEmptyString(pszOU)) {
            
        dwError = LsaBuildOrgUnitDN(
                    pszDomain,
                    pszOU,
                    &pszOU_DN);
        BAIL_ON_LSA_ERROR(dwError);
        
        dwError = LsaMbsToWc16s(
                    pszOU_DN,
                    &pwszOU);
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    if (!IsNullOrEmptyString(pszOSName)) {
        dwError = LsaMbsToWc16s(
                    pszOSName,
                    &pwszOSName);
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    if (!IsNullOrEmptyString(pszOSVersion)) {
        dwError = LsaMbsToWc16s(
                    pszOSVersion,
                    &pwszOSVersion);
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    dwError = Win32ErrorToErrno(
        NetJoinDomainLocal(
            pwszHostname,
            pwszDomain,
            pwszOU,
            NULL,
            NULL,
            dwOptions,
            pwszOSName,
            pwszOSVersion));
    BAIL_ON_LSA_ERROR(dwError);
    
cleanup:

    if (pszOldCachePath != NULL)
    {
        NpcSetCredCacheName(pszOldCachePath);
        NpcFreeCredCacheName(pszOldCachePath);
    }

    LSA_SAFE_FREE_STRING(pszOU_DN);
    LSA_SAFE_FREE_MEMORY(pwszHostname);
    LSA_SAFE_FREE_MEMORY(pwszDomain);
    LSA_SAFE_FREE_MEMORY(pwszOU);
    LSA_SAFE_FREE_MEMORY(pwszOSName);
    LSA_SAFE_FREE_MEMORY(pwszOSVersion);

    if (ctx != NULL)
    {
        if (cc != NULL)
        {
            krb5_cc_destroy(ctx, cc);
        }
        krb5_free_context(ctx);
    }

    NpcCleanup();

    return dwError;
    
error:

    goto cleanup;
}

DWORD
LsaSrvJoinBuildDomSid(
    PCSTR pszSID,
    DomSid** ppDomSID
    )
{
    DWORD dwError = 0;
    DomSid* pDomSID = NULL;
    PLSA_SECURITY_IDENTIFIER pSID = NULL;
    PBYTE pPos = NULL;
    uint8 subAuthCount;
    
    dwError = LsaAllocSecurityIdentifierFromString(
                    pszSID,
                    &pSID);
    BAIL_ON_LSA_ERROR(dwError);

    pPos = pSID->pucSidBytes;
    subAuthCount = pPos[1];
    
    dwError = SidAllocate(
                    &pDomSID,
                    subAuthCount);
    BAIL_ON_LSA_ERROR(dwError);
    
    pPos = pSID->pucSidBytes;
    
    memcpy(&pDomSID->revision, pPos++, 1);
    
    pDomSID->subauth_count = subAuthCount;
    pPos++;
    
    memcpy(pDomSID->authid, pPos, sizeof(pDomSID->authid)/sizeof(pDomSID->authid[0]));
    
    pPos += sizeof(pDomSID->authid)/sizeof(pDomSID->authid[0]);
    
    memcpy(pDomSID->subauth, pPos, pDomSID->subauth_count * sizeof(UINT32));
    
    *ppDomSID = pDomSID;
    
cleanup:

    if (pSID) {
        LsaFreeSecurityIdentifier(pSID);
    }

    return dwError;
    
error:

    *ppDomSID = NULL;
    
    if (pDomSID) {
        SidFree(pDomSID);
    }

    goto cleanup;
}

DWORD
LsaBuildOrgUnitDN(
    PCSTR pszDomain,
    PCSTR pszOU,
    PSTR* ppszOU_DN
    )
{
    DWORD dwError = 0;
    PSTR  pszOU_DN = NULL;
    PSTR  pszWriteMark = NULL;
    PCSTR pszIter = NULL;
    DWORD dwRequiredOULen = 0;
    DWORD dwRequiredDomainLen = 0;
    size_t stLength = 0;
    DWORD nOUParts = 0;
    DWORD nDomainParts = 0;
    
    BAIL_ON_INVALID_STRING(pszDomain);
    BAIL_ON_INVALID_STRING(pszOU);
    
    // Figure out the length required to write the OU DN
    pszIter = pszOU;
    while ((stLength = strcspn(pszIter, "/")) != 0) {
        dwRequiredOULen += sizeof(LSA_JOIN_OU_PREFIX) - 1;
        dwRequiredOULen += stLength;
        nOUParts++;
        
        pszIter += stLength;
        
        stLength = strspn(pszIter, "/");
        pszIter += stLength;
    }
    
    // Figure out the length required to write the Domain DN
    pszIter = pszDomain;
    while ((stLength = strcspn(pszIter, ".")) != 0) {
        dwRequiredDomainLen += sizeof(LSA_JOIN_DC_PREFIX) - 1;
        dwRequiredDomainLen += stLength;
        nDomainParts++;
        
        pszIter += stLength;
        
        stLength = strspn(pszIter, ".");
        pszIter += stLength;
    }
    
    dwError = LsaAllocateMemory(
                    sizeof(CHAR) * (dwRequiredDomainLen + 
                                    dwRequiredOULen +
                                    nOUParts + 
                                    nDomainParts),
                    (PVOID*)&pszOU_DN);
    BAIL_ON_LSA_ERROR(dwError);
    
    // Write out the Domain DN
    pszWriteMark = pszOU_DN + sizeof(CHAR) * (dwRequiredOULen + nOUParts) - 1;
    pszIter = pszDomain;
    while ((stLength = strcspn(pszIter, ".")) != 0) {        
        *pszWriteMark++ = ',';
        
        memcpy(pszWriteMark, LSA_JOIN_DC_PREFIX, sizeof(LSA_JOIN_DC_PREFIX) - 1);
        pszWriteMark += sizeof(LSA_JOIN_DC_PREFIX) - 1;
        
        memcpy(pszWriteMark, pszIter, stLength);
        pszWriteMark += stLength;
        
        pszIter += stLength;
        
        stLength = strspn(pszIter, ".");
        pszIter += stLength;
    }
    
    // Write out the OU DN in reverse
    if (!strcasecmp(pszOU, "Computers"))
    {
        //Use the CN prefix for Computers
        memcpy(pszOU_DN, "CN=Computers,", sizeof("CN=Computers,")-1);
    }
    else
    {
        pszIter = pszOU;
        pszWriteMark = pszOU_DN + sizeof(CHAR) * (dwRequiredOULen + nOUParts);
        while ((stLength = strcspn(pszIter, "/")) != 0) {
            DWORD dwOffset = sizeof(LSA_JOIN_OU_PREFIX) - 1;
            dwOffset += stLength;
            dwOffset += 1; /* for comma */
            
            pszWriteMark -= dwOffset;
            
            memcpy(pszWriteMark,
                    LSA_JOIN_OU_PREFIX,
                    sizeof(LSA_JOIN_OU_PREFIX) - 1);
            pszWriteMark += sizeof(LSA_JOIN_OU_PREFIX) - 1;
            
            memcpy(pszWriteMark, pszIter, stLength);
            pszWriteMark += stLength;
            
            memcpy(pszWriteMark, ",", 1);
            pszWriteMark++;
            
            pszWriteMark -= dwOffset;
            
            pszIter += stLength;
            
            stLength = strspn(pszIter, "/");
            pszIter += stLength;
        }
    }
    
    *ppszOU_DN = pszOU_DN;
    
cleanup:

    return dwError;
    
error:

    *ppszOU_DN = NULL;
    
    LSA_SAFE_FREE_STRING(pszOU_DN);

    goto cleanup;
}

DWORD
LsaNetTestJoinDomain(
    PBOOLEAN pbIsJoined
    )
{
    DWORD dwError = 0;
    BOOLEAN bIsJoined = FALSE;
    HANDLE hStore = (HANDLE)NULL;
    PLWPS_PASSWORD_INFO pPassInfo = NULL;
    PSTR pszHostname = NULL;
    
    dwError = LsaDnsGetHostInfo(&pszHostname);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LwpsOpenPasswordStore(
                LWPS_PASSWORD_STORE_SQLDB,
                &hStore);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LwpsGetPasswordByHostName(
                hStore,
                pszHostname,
                &pPassInfo);

    switch(dwError)
    {
        case LWPS_ERROR_INVALID_ACCOUNT:
            bIsJoined = FALSE;
            dwError = 0;
            break;
        case 0:
            bIsJoined = TRUE;
            break;
        default:
            BAIL_ON_LSA_ERROR(dwError);
    }
    
    *pbIsJoined = bIsJoined;
    
cleanup:

    if (pPassInfo) {
        LwpsFreePasswordInfo(hStore, pPassInfo);
    }
    
    if (hStore != (HANDLE)NULL) {
        LwpsClosePasswordStore(hStore);
    }
    
    LSA_SAFE_FREE_STRING(pszHostname);

    return dwError;
    
error:

    *pbIsJoined = FALSE;

    goto cleanup;
}

VOID
LsaEnableDebugLog(
    VOID
    )
{
    NetEnableDebug();
}

VOID
LsaDisableDebugLog(
    VOID
    )
{
    NetDisableDebug();
}

DWORD
LsaSyncTimeToDC(
    PCSTR  pszDomain
    )
{
    DWORD dwError = 0;
    UNIX_TIME_T dcTime = 0;
    time_t ttDCTime = 0;
    
    dwError = LWNetGetDCTime(
                    pszDomain,
                    &dcTime);
    BAIL_ON_LSA_ERROR(dwError);
    
    ttDCTime = (time_t) dcTime;
    
    if (labs(ttDCTime - time(NULL)) > gdwClockDriftSecs) {
        dwError = LsaSetSystemTime(ttDCTime);
        BAIL_ON_LSA_ERROR(dwError);
    }
    
cleanup:

    return dwError;
    
error:

    goto cleanup;
}

DWORD
LsaBuildMachineAccountInfo(
    PLWPS_PASSWORD_INFO pInfo,
    PLSA_MACHINE_ACCT_INFO* ppAcctInfo
    )
{
    DWORD dwError = 0;
    PLSA_MACHINE_ACCT_INFO pAcctInfo = NULL;
    
    dwError = LsaAllocateMemory(
                    sizeof(LSA_MACHINE_ACCT_INFO),
                    (PVOID*)&pAcctInfo);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaWc16sToMbs(
                    pInfo->pwszDnsDomainName,
                    &pAcctInfo->pszDnsDomainName);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaWc16sToMbs(
                    pInfo->pwszDomainName,
                    &pAcctInfo->pszDomainName);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaWc16sToMbs(
                    pInfo->pwszHostname,
                    &pAcctInfo->pszHostname);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaWc16sToMbs(
                    pInfo->pwszMachineAccount,
                    &pAcctInfo->pszMachineAccount);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaWc16sToMbs(
                    pInfo->pwszMachinePassword,
                    &pAcctInfo->pszMachinePassword);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaWc16sToMbs(
                    pInfo->pwszSID,
                    &pAcctInfo->pszSID);
    BAIL_ON_LSA_ERROR(dwError);
    
    pAcctInfo->dwSchannelType = pInfo->dwSchannelType;
    pAcctInfo->last_change_time = pInfo->last_change_time;
    
    *ppAcctInfo = pAcctInfo;
    
cleanup:

    return dwError;
    
error:

    *ppAcctInfo = NULL;
    
    if (pAcctInfo) {
        LsaFreeMachineAccountInfo(pAcctInfo);
    }
    
    goto cleanup;
}

VOID
LsaFreeMachineAccountInfo(
    PLSA_MACHINE_ACCT_INFO pAcctInfo
    )
{
    LSA_SAFE_FREE_STRING(pAcctInfo->pszDnsDomainName);
    LSA_SAFE_FREE_STRING(pAcctInfo->pszDomainName);
    LSA_SAFE_FREE_STRING(pAcctInfo->pszHostname);
    LSA_SAFE_FREE_STRING(pAcctInfo->pszMachineAccount);
    LSA_SAFE_FREE_STRING(pAcctInfo->pszMachinePassword);
    LSA_SAFE_FREE_STRING(pAcctInfo->pszSID);
    LsaFreeMemory(pAcctInfo);
}
