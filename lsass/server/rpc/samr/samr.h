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
SamrConnect(
    handle_t hBinding,
    const wchar16_t *system_name,
    uint32 access_mask,
    CONNECT_HANDLE *hConn
    );


NTSTATUS
SamrClose(
    handle_t bind,
    void *hIn,
    void **hOut
    );


NTSTATUS
SamrLookupDomain(
    handle_t hBinding,
    CONNECT_HANDLE hConn,
    UnicodeString *domain_name,
    SID **sid
    );


NTSTATUS
SamrEnumDomains(
    handle_t hBinding,
    CONNECT_HANDLE hConn,
    uint32 *resume,
    uint32 size,
    EntryArray **domains,
    uint32 *num_entries
    );


NTSTATUS
SamrOpenDomain(
    handle_t hBinding,
    CONNECT_HANDLE hConn,
    uint32 access_mask,
    SID *sid,
    PolicyHandle *domain_handle
    );


NTSTATUS
SamrSrvConnect2(
    handle_t hBinding,
    uint32 size,
    const wchar16_t *sysname,
    uint32 access_mask,
    CONNECT_HANDLE *hConn
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

