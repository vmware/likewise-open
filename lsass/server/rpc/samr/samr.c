/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2008
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
 * Abstract: Samr wrapper functions called from DCE/RPC stubs (rpc server library)
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS _SamrConnect(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *system_name,
    /* [in] */ uint32 access_mask,
    /* [out] */ CONNECT_HANDLE *hConn
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = SamrConnect(IDL_handle,
                         system_name,
                         access_mask,
                         hConn);
    return status;
}


NTSTATUS _SamrClose(
    /* [in] */ handle_t IDL_handle,
    /* [in,context_handle] */ void *hIn,
    /* [out,context_handle] */ void **hOut
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = SamrClose(IDL_handle,
                       hIn,
                       hOut);
    return status;
}


NTSTATUS samr_Function02(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS samr_Function03(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS samr_Function04(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _SamrLookupDomain(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ CONNECT_HANDLE hConn,
    /* [in] */ UnicodeString *domain_name,
    /* [out] */ SID **sid
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = SamrLookupDomain(IDL_handle,
                              hConn,
                              domain_name,
                              sid);
    return status;
}


NTSTATUS _SamrEnumDomains(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ CONNECT_HANDLE hConn,
    /* [in, out] */ uint32 *resume,
    /* [in] */ uint32 size,
    /* [out] */ EntryArray **domains,
    /* [out] */ uint32 *num_entries
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = SamrEnumDomains(IDL_handle,
                             hConn,
                             resume,
                             size,
                             domains,
                             num_entries);
    return status;
}


NTSTATUS _SamrOpenDomain(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ CONNECT_HANDLE hConn,
    /* [in] */ uint32 access_mask,
    /* [in] */ SID *sid,
    /* [out] */ PolicyHandle *domain_handle
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = SamrOpenDomain(IDL_handle,
                            hConn,
                            access_mask,
                            sid,
                            domain_handle);
    return status;
}


NTSTATUS _SamrQueryDomainInfo(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ PolicyHandle *domain_handle,
    /* [in] */ uint16 level,
    /* [out] */ DomainInfo **info
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS samr_Function09(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS samr_Function0a(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS samr_Function0b(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _SamrCreateUser(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ PolicyHandle *domain_handle,
    /* [in] */ UnicodeString *account_name,
    /* [in] */ uint32 access_mask,
    /* [out] */ PolicyHandle *user_handle,
    /* [out] */ uint32 *rid
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _SamrEnumDomainUsers(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ PolicyHandle *domain_handle,
    /* [in, out] */ uint32 *resume,
    /* [in] */ uint32 account_flags,
    /* [in] */ uint32 max_size,
    /* [out] */ RidNameArray **names,
    /* [out] */ uint32 *num_entries
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _SamrCreateDomAlias(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ PolicyHandle *domain_handle,
    /* [in] */ UnicodeString *alias_name,
    /* [in] */ uint32 access_mask,
    /* [out] */ PolicyHandle *alias_handle,
    /* [out] */ uint32 *rid
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _SamrEnumDomainAliases(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ PolicyHandle *domain_handle,
    /* [in, out] */ uint32 *resume,
    /* [in] */ uint32 account_flags,
    /* [out] */ RidNameArray **names,
    /* [out] */ uint32 *num_entries
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _SamrGetAliasMembership(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ PolicyHandle *domain_handle,
    /* [in] */ SidArray *sids,
    /* [out] */ Ids *rids
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _SamrLookupNames(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ PolicyHandle *domain_handle,
    /* [in] */ uint32 num_names,
    /* [in] */ UnicodeString *names,
    /* [out] */ Ids *ids,
    /* [out] */ Ids *types
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _SamrLookupRids(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ PolicyHandle *domain_handle,
    /* [in] */ uint32 num_rids,
    /* [in] */ uint32 rids[],
    /* [out] */ UnicodeStringArray *names,
    /* [out] */ Ids *types
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS samr_Function13(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS samr_Function14(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS samr_Function15(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS samr_Function16(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS samr_Function17(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS samr_Function18(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS samr_Function19(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS samr_Function1a(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _SamrOpenAlias(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ PolicyHandle *domain_handle,
    /* [in] */ uint32 access_mask,
    /* [in] */ uint32 rid,
    /* [out] */ PolicyHandle *alias
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _SamrQueryAliasInfo(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ PolicyHandle *alias_handle,
    /* [in] */ uint16 level,
    /* [out] */ AliasInfo **info
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _SamrSetAliasInfo(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ PolicyHandle *alias_handle,
    /* [in] */ uint16 level,
    /* [in] */ AliasInfo *info
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _SamrDeleteDomAlias(
    /* [in] */ handle_t IDL_handle,
    /* [in, out] */ PolicyHandle *alias_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _SamrAddAliasMember(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ PolicyHandle *alias_handle,
    /* [in] */ SID *sid
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _SamrDeleteAliasMember(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ PolicyHandle *alias_handle,
    /* [in] */ SID *sid
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _SamrGetMembersInAlias(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ PolicyHandle *alias_handle,
    /* [out] */ SidArray *sids
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _SamrOpenUser(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ PolicyHandle *domain_handle,
    /* [in] */ uint32 access_mask,
    /* [in] */ uint32 rid,
    /* [out] */ PolicyHandle *user_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _SamrDeleteUser(
    /* [in] */ handle_t IDL_handle,
    /* [in, out] */ PolicyHandle *user_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _SamrQueryUserInfo(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ PolicyHandle *user_handle,
    /* [in] */ uint16 level,
    /* [out] */ UserInfo **info
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _SamrSetUserInfo(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ PolicyHandle *user_handle,
    /* [in] */ uint16 level,
    /* [in] */ UserInfo *info
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS samr_Function26(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _SamrGetUserGroups(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ PolicyHandle *user_handle,
    /* [out] */ RidWithAttributeArray **rids
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS samr_Function28(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS samr_Function29(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS samr_Function2a(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS samr_Function2b(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _SamrGetUserPwInfo(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ PolicyHandle *user_handle,
    /* [out] */ PwInfo *info
    )
{
}


NTSTATUS samr_Function2d(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS samr_Function2e(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS samr_Function2f(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS samr_Function30(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS samr_Function31(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _SamrCreateUser2(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ PolicyHandle *domain_handle,
    /* [in] */ UnicodeStringEx *account_name,
    /* [in] */ uint32 account_flags,
    /* [in] */ uint32 access_mask,
    /* [out] */ PolicyHandle *user_handle,
    /* [out] */ uint32 *access_granted,
    /* [out] */ uint32 *rid
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS samr_Function33(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS samr_Function34(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS samr_Function35(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS samr_Function36(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _SamrChangePasswordUser2(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ UnicodeString *server,
    /* [in] */ UnicodeString *account_name,
    /* [in] */ CryptPassword *nt_password,
    /* [in] */ HashPassword *nt_verifier,
    /* [in] */ uint8 lm_change,
    /* [in] */ CryptPassword *lm_password,
    /* [in] */ HashPassword *lm_verifier
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS samr_Function38(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS __SamrConnect2(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ uint32 size,
    /* [in] */ wchar16_t *system_name,
    /* [in] */ uint32 access_mask,
    /* [out] */ CONNECT_HANDLE *hConn
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = SamrSrvConnect2(IDL_handle,
                             size,
                             system_name,
                             access_mask,
                             hConn);
    return status;
}


NTSTATUS samr_Function3a(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS samr_Function3b(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS samr_Function3c(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS samr_Function3d(
    /* [in] */ handle_t IDL_handle
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}


NTSTATUS _SamrConnect4(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *system_name,
    /* [in] */ uint32 unknown,
    /* [in] */ uint32 access_mask,
    /* [out] */ CONNECT_HANDLE *hConn
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}



/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/

