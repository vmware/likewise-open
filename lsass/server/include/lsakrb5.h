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

#include <uuid/uuid.h>
#include <lwrpc/krb5pac.h>
#include <lwio/lwio.h>


typedef BOOLEAN (*LSA_KRB5_REALM_IS_OFFLINE_CALLBACK)(IN PCSTR pszRealmName);
typedef VOID (*LSA_KRB5_REALM_TRANSITION_OFFLINE_CALLBACK)(IN PCSTR pszRealmName);

DWORD
LwKrb5Init(
    IN OPTIONAL LSA_KRB5_REALM_IS_OFFLINE_CALLBACK pfIsOfflineCallback,
    IN OPTIONAL LSA_KRB5_REALM_TRANSITION_OFFLINE_CALLBACK pfTransitionOfflineCallback
    );

DWORD
LwKrb5GetDefaultRealm(
    PSTR* ppszRealm
    );

DWORD
LwKrb5GetUserCachePath(
    uid_t         uid,
    Krb5CacheType cacheType,
    PSTR*         ppszCachePath
    );

DWORD
LwKrb5SetDefaultCachePath(
    PCSTR pszCachePath,
    PSTR* ppszOriginalCachePath
    );

DWORD
LwKrb5SetProcessDefaultCachePath(
    PCSTR pszCachePath
    );

DWORD
LsaSetupMachineSession(
    PCSTR  pszMachname,
    PCSTR  pszSamAccountName,
    PCSTR  pszPassword,
    PCSTR  pszRealm,
    PCSTR  pszDomain,
    PDWORD pdwGoodUntilTime
    );

DWORD
LwKrb5CleanupMachineSession(
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
LwKrb5GetTgt(
    PCSTR  pszUserPrincipal,
    PCSTR  pszPassword,
    PCSTR  pszCcPath,
    PDWORD pdwGoodUntilTime
    );

DWORD
LwKrb5GetServiceTicketForUser(
    uid_t         uid,
    PCSTR         pszUserPrincipal,
    PCSTR         pszServername,
    PCSTR         pszDomain,
    Krb5CacheType cacheType
    );

DWORD
LwKrb5Shutdown(
    VOID
    );

DWORD
LwKrb5RefreshMachineTGT(
    PDWORD pdwGoodUntilTime
    );

typedef struct _LSA_ACCESS_TOKEN_FREE_INFO *PLSA_ACCESS_TOKEN_FREE_INFO;

DWORD
LsaSetSMBAccessToken(
    IN PCSTR pszDomain,
    IN PCSTR pszUsername,
    IN PCSTR pszPassword,
    IN BOOLEAN bSetDefaultCachePath,
    OUT PLSA_ACCESS_TOKEN_FREE_INFO* ppFreeInfo,
    OUT OPTIONAL LW_PIO_ACCESS_TOKEN* ppOldToken
    );

void
LsaFreeSMBAccessToken(
    IN OUT PLSA_ACCESS_TOKEN_FREE_INFO* ppFreeInfo
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
