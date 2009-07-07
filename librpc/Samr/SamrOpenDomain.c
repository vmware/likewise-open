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

#include "includes.h"


NTSTATUS
SamrOpenDomain(
    IN  handle_t      hSamrBinding,
    IN  PolicyHandle *phConn,
    IN  UINT32        AccessMask,
    IN  PSID          pSid,
    OUT PolicyHandle *phDomain
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PolicyHandle hDomain = {0};

    BAIL_ON_INVALID_PTR(hSamrBinding, ntStatus);
    BAIL_ON_INVALID_PTR(phConn, ntStatus);
    BAIL_ON_INVALID_PTR(pSid, ntStatus);
    BAIL_ON_INVALID_PTR(phDomain, ntStatus);

    DCERPC_CALL(ntStatus, _SamrOpenDomain(hSamrBinding,
                                          phConn,
                                          AccessMask,
                                          pSid,
                                          &hDomain));
    BAIL_ON_NT_STATUS(ntStatus);
	
    *phDomain = hDomain;

cleanup:
    return ntStatus;

error:
    memset(phDomain, 0, sizeof(*phDomain));

    goto cleanup;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
