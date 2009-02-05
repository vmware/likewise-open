/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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

#include "includes.h"


NET_API_STATUS NetLocalGroupAdd(const wchar16_t *hostname, uint32 level,
				void *buffer, uint32 *parm_err)
{
    const uint32 dom_access = DOMAIN_ACCESS_CREATE_ALIAS;
    const uint32 alias_access = ALIAS_ACCESS_LOOKUP_INFO |
                                ALIAS_ACCESS_SET_INFO;

    NTSTATUS status = STATUS_SUCCESS;
    NetConn *conn = NULL;
    handle_t samr_bind;
    PolicyHandle domain_handle, alias_handle;
    wchar16_t *alias_name, *comment;
    LOCALGROUP_INFO_0 *info0 = NULL;
    LOCALGROUP_INFO_1 *info1 = NULL;
    uint32 rid;
    AliasInfo info;

    switch (level) {
    case 0: info0 = (LOCALGROUP_INFO_0*) buffer;
	break;
    case 1: info1 = (LOCALGROUP_INFO_1*) buffer;
	break;
    default:
	return NtStatusToWin32Error(ERROR_INVALID_LEVEL);
    }

    alias_name = NULL;
    comment    = NULL;
	
    status = NetConnectSamr(&conn, hostname, dom_access, 0);
    if (status != 0) return NtStatusToWin32Error(status);

    switch (level) {
    case 0: alias_name = info0->lgrpi0_name;
	break;
    case 1: alias_name = info1->lgrpi1_name;
	comment    = info1->lgrpi1_comment;
	break;
    default:
	return NtStatusToWin32Error(STATUS_NOT_IMPLEMENTED);
    }

    samr_bind     = conn->samr.bind;
    domain_handle = conn->samr.dom_handle;

    status = SamrCreateDomAlias(samr_bind, &domain_handle, alias_name, alias_access,
				&alias_handle, &rid);
    if (status != 0) return NtStatusToWin32Error(status);

    if (comment) {
	memset(&info, 0, sizeof(info));
	InitUnicodeString(&info.description, comment);

	status = SamrSetAliasInfo(samr_bind, &alias_handle,
				  ALIAS_INFO_DESCRIPTION, &info);
	if (status != 0) return NtStatusToWin32Error(status);
    }

    status = SamrClose(samr_bind, &alias_handle);
    if (status != 0) return NtStatusToWin32Error(status);
	
    *parm_err = 0;

    return STATUS_SUCCESS;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
