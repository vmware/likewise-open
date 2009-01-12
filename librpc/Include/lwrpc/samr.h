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
 * Abstract: Samr interface (rpc client library)
 *
 * Authors: Rafal Szczesniak (rafal@likewisesoftware.com)
 */


#ifndef _SAMR_H_
#define _SAMR_H_

#include <dce/rpc.h>

#include <secdesc/secdesc.h>

#include <lwrpc/unicodestring.h>
#include <lwrpc/samrflags.h>
#include <lwrpc/userinfo.h>
#include <lwrpc/aliasinfo.h>
#include <lwrpc/domaininfo.h>


NTSTATUS SamrConnect2(handle_t bind, const wchar16_t *sysname,
                      uint32 access_mask, PolicyHandle *conn_handle);

NTSTATUS SamrConnect4(handle_t bind, const wchar16_t *sysname,
                      uint32 access_mask, PolicyHandle *conn_handle);

NTSTATUS SamrClose(handle_t bind, PolicyHandle *handle);

NTSTATUS SamrLookupDomain(handle_t bind, PolicyHandle *handle,
                          wchar16_t *domain_name, DomSid **sid);

NTSTATUS SamrOpenDomain(handle_t bind, PolicyHandle *conn_handle,
                        uint32 access_mask, DomSid *sid,
                        PolicyHandle *dom_handle);

NTSTATUS SamrLookupNames(handle_t bind, PolicyHandle *dom_handle,
                         uint32 num_names, wchar16_t *names[],
                         uint32 **rids, uint32 **types, uint32 *count);

NTSTATUS SamrOpenUser(handle_t bind, PolicyHandle *dom_handle,
                      uint32 access_mask, uint32 rid,
                      PolicyHandle *user_handle);

NTSTATUS SamrQueryUserInfo(handle_t bind, PolicyHandle *user_handle,
                           uint16 level, UserInfo **info);

NTSTATUS SamrEnumDomainUsers(handle_t bind, PolicyHandle *domain_handle,
                             uint32 *resume, uint32 account_flags,
                             uint32 max_size, wchar16_t ***ret_names,
                             uint32 **ret_rids, uint32 *num_entries);

NTSTATUS SamrEnumDomains(handle_t bind, PolicyHandle *handle, uint32 *resume,
                         uint32 size, wchar16_t ***names, uint32 *num_entries);

NTSTATUS SamrCreateUser(handle_t bind, PolicyHandle *domain_handle,
                        wchar16_t *account_name, uint32 access_mask,
                        PolicyHandle *user_handle, uint32 *rid);

NTSTATUS SamrDeleteUser(handle_t bind, PolicyHandle *user_handle);

NTSTATUS SamrSetUserInfo(handle_t bind, PolicyHandle *user_handle,
                         uint16 level, UserInfo *info);

NTSTATUS SamrCreateDomAlias(handle_t bind, PolicyHandle *domain_handle,
                            wchar16_t *alias, uint32 access_mask,
                            PolicyHandle *alias_handle, uint32 *rid);

NTSTATUS SamrDeleteDomAlias(handle_t bind, PolicyHandle *alias_handle);

NTSTATUS SamrOpenAlias(handle_t bind, PolicyHandle *domain_handle,
                       uint32 access_mask, uint32 rid,
                       PolicyHandle *alias_handle);

NTSTATUS SamrQueryAliasInfo(handle_t bind, PolicyHandle *alias_handle,
                            uint16 level, AliasInfo **info);

NTSTATUS SamrSetAliasInfo(handle_t bind, PolicyHandle *alias_handle,
                          uint16 level, AliasInfo *info);

NTSTATUS SamrGetAliasMembership(handle_t bind, PolicyHandle *domain_handle,
                                DomSid *sids, uint32 num_sids, uint32 **rids,
                                uint32 *num_rids);

NTSTATUS SamrGetMembersInAlias(handle_t b, PolicyHandle *alias_handle,
                               DomSid ***sids, uint32 *count);

NTSTATUS SamrCreateUser2(handle_t bind, PolicyHandle *domain_handle,
                         wchar16_t *account_name, uint32 account_flags,
                         uint32 account_mask, PolicyHandle *account_handle,
                         uint32 *access_granted, uint32 *rid);

NTSTATUS SamrGetUserPwInfo(handle_t bind, PolicyHandle *user_handle,
                           PwInfo *info);

NTSTATUS SamrLookupRids(handle_t bind, PolicyHandle *domain_handle,
                        uint32 num_rids, uint32 *rids, wchar16_t ***names,
                        uint32 **types);

NTSTATUS SamrGetUserGroups(handle_t bind, PolicyHandle *user_handle,
                           uint32 **rids, uint32 **attributes,
                           uint32 *count);


NTSTATUS SamrInitMemory();

NTSTATUS SamrDestroyMemory();

NTSTATUS SamrFreeMemory(void *ptr);



#endif /* _SAMR_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
