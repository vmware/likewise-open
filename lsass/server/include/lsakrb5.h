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
 *        lsakrb5.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS) 
 *        
 *        Kerberos 5 API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Rafal Szczesniak <rszczesniak@likewisesoftware.com>
 *
 */
#ifndef __LSAKRB5_H__
#define __LSAKRB5_H__

#include <lwrpc/krb5pac.h>

typedef enum
{
    KRB5_InMemory_Cache,
    KRB5_File_Cache
} Krb5CacheType;

#ifdef WIN32

#define BAIL_ON_SEC_ERROR(dwMajorStatus)                        \
    if ((dwMajorStatus!= SEC_E_OK) &&                           \
        (dwMajorStatus != SEC_I_CONTINUE_NEEDED)) {             \
        LSA_LOG_ERROR("GSS API Error: %d", dwMajorStatus);      \
        dwError = LSA_ERROR_GSS_CALL_FAILED;                    \
        goto error;                                             \
    }

#else

#define BAIL_ON_SEC_ERROR(dwMajorStatus)                        \
    if ((dwMajorStatus!= GSS_S_COMPLETE) &&                     \
        (dwMajorStatus != GSS_S_CONTINUE_NEEDED)) {             \
        LSA_LOG_ERROR("GSS API Error: %d", dwMajorStatus);      \
        dwError = LSA_ERROR_GSS_CALL_FAILED;                    \
        goto error;                                             \
    }

#endif /* WIN32 */

typedef BOOLEAN (*LSA_KRB5_REALM_IS_OFFLINE_CALLBACK)(IN PCSTR pszRealmName);
typedef VOID (*LSA_KRB5_REALM_TRANSITION_OFFLINE_CALLBACK)(IN PCSTR pszRealmName);

DWORD
LsaKrb5Init(
    IN OPTIONAL LSA_KRB5_REALM_IS_OFFLINE_CALLBACK pfIsOfflineCallback,
    IN OPTIONAL LSA_KRB5_REALM_TRANSITION_OFFLINE_CALLBACK pfTransitionOfflineCallback
    );

DWORD
LsaKrb5GetDefaultRealm(
    PSTR* ppszRealm
    );

DWORD
LsaKrb5GetSystemCachePath(
    Krb5CacheType cacheType,
    PSTR*         ppszCachePath
    );

DWORD
LsaKrb5GetUserCachePath(
    uid_t         uid,
    Krb5CacheType cacheType,
    PSTR*         ppszCachePath
    );

DWORD
LsaKrb5SetDefaultCachePath(
    PCSTR pszCachePath,
    PSTR* ppszOriginalCachePath
    );

DWORD
LsaKrb5SetProcessDefaultCachePath(
    PCSTR pszCachePath
    );

DWORD
LsaSetupMachineSession(
    PCSTR  pszMachname,
    PCSTR  pszPassword,
    PCSTR  pszRealm,
    PCSTR  pszDomain,
    PDWORD pdwGoodUntilTime
    );

DWORD
LsaKrb5CleanupMachineSession(
    VOID
    );

DWORD
LsaSetupUserLoginSession(
    uid_t         uid,
    gid_t         gid,
    PCSTR         pszUsername,
    PCSTR         pszPassword,
    BOOLEAN bUpdateUserCache,
    PCSTR         pszServicePrincipal,
    PCSTR         pszServicePassword,
    PAC_LOGON_INFO **ppLogonInfo,
    PDWORD pdwGoodUntilTime
    );

DWORD
LsaKrb5GetTgt(
    PCSTR  pszUserPrincipal,
    PCSTR  pszPassword,
    PCSTR  pszCcPath,
    PDWORD pdwGoodUntilTime
    );

DWORD
LsaKrb5GetServiceTicketForUser(
    uid_t         uid,
    PCSTR         pszUserPrincipal,
    PCSTR         pszServername,
    PCSTR         pszDomain,
    Krb5CacheType cacheType
    );

DWORD
LsaKrb5Shutdown(
    VOID
    );

DWORD
LsaKrb5RefreshMachineTGT(
    PDWORD pdwGoodUntilTime
    );

DWORD
LsaKrb5GetMachineCreds(
    PCSTR pszHostname,
    PSTR* ppszUsername,
    PSTR* ppszPassword,
    PSTR* ppszDomainDnsName
    );

#endif /* __LSAKRB5_H__ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
