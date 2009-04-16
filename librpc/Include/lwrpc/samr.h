/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software
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
 * Abstract: Samr interface (rpc client library)
 *
 * Authors: Rafal Szczesniak (rafal@likewisesoftware.com)
 */


#ifndef _SAMR_H_
#define _SAMR_H_

#include <secdesc/sectypes.h>

#include <lwrpc/samrbinding.h>
#include <lwrpc/unicodestring.h>
#include <lwrpc/samrflags.h>
#include <lwrpc/userinfo.h>
#include <lwrpc/aliasinfo.h>
#include <lwrpc/domaininfo.h>
#include <lwrpc/displayinfo.h>


NTSTATUS
SamrConnect2(
    handle_t bind,
    const wchar16_t *sysname,
    uint32 access_mask,
    PolicyHandle *conn_handle
    );

NTSTATUS
SamrConnect3(
    handle_t bind,
    const wchar16_t *sysname,
    uint32 access_mask,
    PolicyHandle *conn_handle
    );

NTSTATUS
SamrConnect4(
    handle_t bind,
    const wchar16_t *sysname,
    uint32 client_version,
    uint32 access_mask,
    PolicyHandle *conn_handle
    );

NTSTATUS
SamrClose(
    handle_t bind,
    PolicyHandle *handle
    );

NTSTATUS
SamrLookupDomain(
    handle_t bind,
    PolicyHandle *handle,
    const wchar16_t *domain_name,
    PSID* sid
    );

NTSTATUS
SamrOpenDomain(
    handle_t bind,
    PolicyHandle *conn_handle,
    uint32 access_mask,
    PSID sid,
    PolicyHandle *dom_handle
    );

NTSTATUS
SamrQueryDomainInfo(
    handle_t b,
    PolicyHandle *domain_h,
    uint16 level,
    DomainInfo **info
    );

NTSTATUS
SamrLookupNames(
    handle_t bind,
    PolicyHandle *dom_handle,
    uint32 num_names,
    wchar16_t *names[],
    uint32 **rids,
    uint32 **types,
    uint32 *count
    );

NTSTATUS
SamrOpenUser(
    handle_t bind,
    PolicyHandle *dom_handle,
    uint32 access_mask,
    uint32 rid,
    PolicyHandle *user_handle
    );

NTSTATUS
SamrQueryUserInfo(
    handle_t bind,
    PolicyHandle *user_handle,
    uint16 level,
    UserInfo **info
    );

NTSTATUS
SamrEnumDomainUsers(
    handle_t bind,
    PolicyHandle *domain_handle,
    uint32 *resume,
    uint32 account_flags,
    uint32 max_size,
    wchar16_t ***ret_names,
    uint32 **ret_rids,
    uint32 *num_entries
    );

NTSTATUS
SamrChangePasswordUser2(
    handle_t b,
    const wchar16_t *hostname,
    const wchar16_t *account,
    uint8 ntpass[516],
    uint8 ntverify[16],
    uint8 lm_change,
    uint8 lmpass[516],
    uint8 lmverify[16]
    );

NTSTATUS
SamrEnumDomains(
    handle_t bind,
    PolicyHandle *handle,
    uint32 *resume,
    uint32 size,
    wchar16_t ***names,
    uint32 *num_entries
    );

NTSTATUS
SamrCreateUser(
    handle_t bind,
    PolicyHandle *domain_handle,
    wchar16_t *account_name,
    uint32 access_mask,
    PolicyHandle *user_handle,
    uint32 *rid
    );

NTSTATUS
SamrDeleteUser(
    handle_t bind,
    PolicyHandle *user_handle
    );

NTSTATUS
SamrSetUserInfo(
    handle_t bind,
    PolicyHandle *user_handle,
    uint16 level,
    UserInfo *info
    );

NTSTATUS
SamrCreateDomAlias(
    handle_t bind,
    PolicyHandle *domain_handle,
    wchar16_t *alias,
    uint32 access_mask,
    PolicyHandle *alias_handle,
    uint32 *rid
    );

NTSTATUS
SamrDeleteDomAlias(
    handle_t bind,
    PolicyHandle *alias_handle
    );

NTSTATUS
SamrOpenAlias(
    handle_t bind,
    PolicyHandle *domain_handle,
    uint32 access_mask,
    uint32 rid,
    PolicyHandle *alias_handle
    );

NTSTATUS
SamrQueryAliasInfo(
    handle_t bind,
    PolicyHandle *alias_handle,
    uint16 level,
    AliasInfo **info
    );

NTSTATUS
SamrSetAliasInfo(
    handle_t bind,
    PolicyHandle *alias_handle,
    uint16 level,
    AliasInfo *info
    );

NTSTATUS
SamrEnumDomainAliases(
    handle_t b,
    PolicyHandle *domain_h,
    uint32 *resume,
    uint32 account_flags,
    wchar16_t ***names,
    uint32 **rids,
    uint32 *count
    );

NTSTATUS
SamrGetAliasMembership(
    handle_t bind,
    PolicyHandle *domain_handle,
    PSID sids,
    uint32 num_sids,
    uint32 **rids,
    uint32 *num_rids
    );

NTSTATUS
SamrGetMembersInAlias(
    handle_t b,
    PolicyHandle *alias_handle,
    PSID** sids,
    uint32 *count
    );

NTSTATUS
SamrAddAliasMember(
    handle_t b,
    PolicyHandle *alias_h,
    PSID sid
    );

NTSTATUS
SamrDeleteAliasMember(
    handle_t b,
    PolicyHandle *alias_h,
    PSID sid
    );

NTSTATUS
SamrCreateUser2(
    handle_t bind,
    PolicyHandle *domain_handle,
    wchar16_t *account_name,
    uint32 account_flags,
    uint32 account_mask,
    PolicyHandle *account_handle,
    uint32 *access_granted,
    uint32 *rid
    );

NTSTATUS
SamrGetUserPwInfo(
    handle_t bind,
    PolicyHandle *user_handle,
    PwInfo *info
    );

NTSTATUS
SamrLookupRids(
    handle_t bind,
    PolicyHandle *domain_handle,
    uint32 num_rids,
    uint32 *rids,
    wchar16_t ***names,
    uint32 **types
    );

NTSTATUS
SamrGetUserGroups(
    handle_t bind,
    PolicyHandle *user_handle,
    uint32 **rids,
    uint32 **attributes,
    uint32 *count
    );

NTSTATUS
SamrQueryDisplayInfo(
    handle_t b,
    PolicyHandle *domain_h,
    uint16 level,
    uint32 start_idx,
    uint32 max_entries,
    uint32 buf_size,
    uint32 *out_total_size,
    uint32 *out_returned_size,
    SamrDisplayInfo **out_info
    );


NTSTATUS
SamrInitMemory(
    void
    );

NTSTATUS
SamrDestroyMemory(
    void
    );

NTSTATUS SamrFreeMemory(
    void *ptr
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
