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


NET_API_STATUS NetApiBufferFree(void *buffer)
{
    NTSTATUS status;
    NetConn *cn = NULL;
    NetConn *clean = NULL;

    /* When called with NULL argument do the cleanup.
       Otherwise just free the pointer */
    if (buffer) {
	free(buffer);
	return NtStatusToWin32Error(STATUS_SUCCESS);
    }

    /* close any remaining connections */
    cn = GetFirstConn(NULL);

    while (cn && cn->next) {

	DEL_CONN(cn);

	if (cn->samr.bind) {
	    status = SamrClose(cn->samr.bind, &cn->samr.dom_handle);
	    if (status != 0) return NtStatusToWin32Error(status);

	    status = SamrClose(cn->samr.bind, &cn->samr.btin_dom_handle);
	    if (status != 0) return NtStatusToWin32Error(status);

	    status = SamrClose(cn->samr.bind, &cn->samr.conn_handle);
	    if (status != 0) return NtStatusToWin32Error(status);
	}

	if (cn->lsa.bind) {
	    status = LsaClose(cn->lsa.bind, &cn->lsa.policy_handle);
	    if (status != 0) return NtStatusToWin32Error(status);
	}
	
	clean = cn;
	cn = cn->next;
	free(clean);
    }

    return NtStatusToWin32Error(STATUS_SUCCESS);
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
