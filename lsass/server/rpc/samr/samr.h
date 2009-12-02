/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2009
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

/*
 * Abstract: Samr rpc server functions (rpc server library)
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#ifndef _SAMR_H_
#define _SAMR_H_


NTSTATUS
SamrSrvConnect(
    handle_t hBinding,
    const wchar16_t *system_name,
    UINT32 access_mask,
    CONNECT_HANDLE *hConn
    );


NTSTATUS
SamrSrvClose(
    handle_t bind,
    void *hIn,
    void **hOut
    );


NTSTATUS
SamrSrvQuerySecurity(
    handle_t hBinding,
    void *hObject,
    UINT32 security_info,
    PSECURITY_DESCRIPTOR_BUFFER *secdesc
    );


NTSTATUS
SamrSrvLookupDomain(
    handle_t hBinding,
    CONNECT_HANDLE hConn,
    UnicodeString *domain_name,
    SID **sid
    );


NTSTATUS
SamrSrvEnumDomains(
    handle_t hBinding,
    CONNECT_HANDLE hConn,
    UINT32 *resume,
    UINT32 size,
    EntryArray **domains,
    UINT32 *num_entries
    );


NTSTATUS
SamrSrvOpenDomain(
    handle_t hBinding,
    CONNECT_HANDLE hConn,
    UINT32 access_mask,
    SID *sid,
    DOMAIN_HANDLE *hDomain
    );


NTSTATUS
SamrSrvQueryDomainInfo(
    handle_t hBinding,
    DOMAIN_HANDLE hDomain,
    UINT16 level,
    DomainInfo **info
    );


NTSTATUS
SamrSrvCreateUser(
    handle_t hBinding,
    DOMAIN_HANDLE hDomain,
    UnicodeString *account_name,
    UINT32 access_mask,
    ACCOUNT_HANDLE *hUser,
    UINT32 *rid
    );


NTSTATUS
SamrSrvEnumDomainUsers(
    handle_t hBinding,
    DOMAIN_HANDLE hDomain,
    UINT32 *resume,
    UINT32 account_flags,
    UINT32 max_size,
    RidNameArray **names,
    UINT32 *num_entries
    );


NTSTATUS
SamrSrvCreateDomAlias(
    handle_t hBinding,
    DOMAIN_HANDLE hDomain,
    UnicodeString *alias_name,
    UINT32 access_mask,
    ACCOUNT_HANDLE *hAlias,
    UINT32 *rid
    );


NTSTATUS
SamrSrvEnumDomainAliases(
    handle_t hBinding,
    DOMAIN_HANDLE hDomain,
    UINT32 *resume,
    UINT32 account_flags,
    RidNameArray **names,
    UINT32 *num_entries
    );


NTSTATUS
SamrSrvGetAliasMembership(
    handle_t hBinding,
    DOMAIN_HANDLE hDomain,
    SidArray *pSids,
    Ids *pRids
    );


NTSTATUS
SamrSrvLookupNames(
    handle_t hBinding,
    DOMAIN_HANDLE hDomain,
    UINT32 num_names,
    UnicodeString *names,
    Ids *ids,
    Ids *types
    );


NTSTATUS
SamrSrvLookupRids(
    handle_t IDL_handle,
    DOMAIN_HANDLE hDomain,
    UINT32 dwNumRids,
    UINT32 *pdwRids,
    UnicodeStringArray *pNames,
    Ids *pTypes
    );


NTSTATUS
SamrSrvOpenAlias(
    handle_t hBinding,
    DOMAIN_HANDLE hDomain,
    UINT32 access_mask,
    UINT32 rid,
    ACCOUNT_HANDLE *hAlias
    );


NTSTATUS
SamrSrvQueryAliasInfo(
    handle_t hBinding,
    ACCOUNT_HANDLE hUser,
    UINT16 level,
    AliasInfo **info
    );


NTSTATUS
SamrSrvSetAliasInfo(
    handle_t hBinding,
    ACCOUNT_HANDLE hAlias,
    UINT16 level,
    AliasInfo *pInfo
    );


NTSTATUS
SamrSrvDeleteAccount(
    handle_t hBinding,
    ACCOUNT_HANDLE hAccountIn,
    ACCOUNT_HANDLE *hAccountOut
    );


NTSTATUS
SamrSrvDeleteDomAlias(
    handle_t hBinding,
    ACCOUNT_HANDLE hAccountIn,
    ACCOUNT_HANDLE *hAccountOut
    );


NTSTATUS
SamrSrvAddAliasMember(
    handle_t IDL_handle,
    ACCOUNT_HANDLE hAlias,
    PSID pSid
    );


NTSTATUS
SamrSrvDeleteAliasMember(
    handle_t IDL_handle,
    ACCOUNT_HANDLE hAlias,
    PSID pSid
    );


NTSTATUS
SamrSrvGetMembersInAlias(
    handle_t hBinding,
    ACCOUNT_HANDLE hAlias,
    SidArray *sids
    );


NTSTATUS
SamrSrvOpenUser(
    handle_t hBinding,
    DOMAIN_HANDLE hDomain,
    UINT32 access_mask,
    UINT32 rid,
    ACCOUNT_HANDLE *hUser
    );


NTSTATUS
SamrSrvDeleteUser(
    handle_t hBinding,
    ACCOUNT_HANDLE hUserIn,
    ACCOUNT_HANDLE *phUserOut
    );


NTSTATUS
SamrSrvQueryUserInfo(
    handle_t hBinding,
    ACCOUNT_HANDLE hUser,
    UINT16 level,
    UserInfo **info
    );


NTSTATUS
SamrSrvSetUserInfo(
    handle_t hBinding,
    ACCOUNT_HANDLE hUser,
    UINT16 level,
    UserInfo *pInfo
    );


NTSTATUS
SamrSrvQueryDisplayInfo(
    handle_t IDL_handle,
    DOMAIN_HANDLE hDomain,
    UINT16 level,
    UINT32 start_idx,
    UINT32 max_entries,
    UINT32 buf_size,
    UINT32 *total_size,
    UINT32 *returned_size,
    SamrDisplayInfo *info
    );


NTSTATUS
SamrSrvRemoveMemberFromForeignDomain(
    handle_t IDL_handle,
    DOMAIN_HANDLE hDomain,
    PSID sid
    );


NTSTATUS
SamrSrvCreateUser2(
    handle_t hBinding,
    DOMAIN_HANDLE hDomain,
    UnicodeStringEx *account_name,
    UINT32 account_flags,
    UINT32 access_mask,
    ACCOUNT_HANDLE *hUser,
    UINT32 *access_granted,
    UINT32 *rid
    );


NTSTATUS
SamrSrvConnect2(
    handle_t hBinding,
    UINT32 size,
    const wchar16_t *sysname,
    UINT32 access_mask,
    CONNECT_HANDLE *hConn
    );


NTSTATUS
SamrSrvEnumDomainAccounts(
    handle_t hBinding,
    DOMAIN_HANDLE hDomain,
    UINT32 *resume,
    DWORD dwObjectClass,
    UINT32 account_flags,
    UINT32 max_size,
    RidNameArray **names,
    UINT32 *num_entries
    );


NTSTATUS
SamrSrvOpenAccount(
    handle_t hBinding,
    DOMAIN_HANDLE hDomain,
    UINT32 access_mask,
    UINT32 rid,
    UINT32 objectClass,
    ACCOUNT_HANDLE *hAccount
    );


NTSTATUS
SamrSrvCreateAccount(
    handle_t hBinding,
    DOMAIN_HANDLE hDomain,
    UnicodeStringEx *account_name,
    DWORD dwObjectClass,
    UINT32 account_flags,
    UINT32 access_mask,
    ACCOUNT_HANDLE *hAccount,
    UINT32 *access_granted,
    UINT32 *rid
    );


NTSTATUS
SamrSrvConnect3(
    handle_t hBinding,
    UINT32 size,
    const wchar16_t *system_name,
    UINT32 unknown1,
    UINT32 access_mask,
    CONNECT_HANDLE *hConn
    );


NTSTATUS
SamrSrvConnect4(
    handle_t hBinding,
    UINT32 size,
    wchar16_t *system_name,
    UINT32 unknown,
    UINT32 access_mask,
    CONNECT_HANDLE *hConn
    );


NTSTATUS
SamrSrvConnect5(
    handle_t hBinding,
    UINT32 size,
    wchar16_t *system_name,
    UINT32 access_mask,
    UINT32 level_in,
    SamrConnectInfo *info_in,
    UINT32 *level_out,
    SamrConnectInfo *info_out,
    CONNECT_HANDLE *hConn
    );


NTSTATUS
SamrSrvGetSystemCreds(
    LW_PIO_CREDS *ppCreds
    );


#endif /* _SAMR_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/

