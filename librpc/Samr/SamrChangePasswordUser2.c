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
SamrChangePasswordUser2(
    handle_t b,
    const wchar16_t *hostname,
    const wchar16_t *account,
    uint8 ntpass[516],
    uint8 ntverify[16],
    uint8 lm_change,
    uint8 lmpass[516],
    uint8 lmverify[16]
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    CryptPassword ntp, lmp;
    CryptPassword *lmpwd = NULL;
    HashPassword ntv, lmv;
    HashPassword *lmver = NULL;
    UnicodeString srv, acct;

    memset((void*)&ntp, 0, sizeof(ntp));
    memset((void*)&lmp, 0, sizeof(lmp));
    memset((void*)&ntv, 0, sizeof(ntv));
    memset((void*)&lmv, 0, sizeof(lmv));
    memset((void*)&srv, 0, sizeof(srv));
    memset((void*)&acct, 0, sizeof(acct));

    goto_if_invalid_param_ntstatus(b, cleanup);
    goto_if_invalid_param_ntstatus(hostname, cleanup);
    goto_if_invalid_param_ntstatus(account, cleanup);
    goto_if_invalid_param_ntstatus(ntpass, cleanup);
    goto_if_invalid_param_ntstatus(ntverify, cleanup);

    status = InitUnicodeString(&srv, hostname);
    goto_if_ntstatus_not_success(status, error);

    status = InitUnicodeString(&acct, account);
    goto_if_ntstatus_not_success(status, error);

    memcpy(ntp.data, ntpass, sizeof(ntp.data));
    memcpy(ntv.data, ntverify, sizeof(ntv.data));

    if (lm_change) {
        memcpy(lmp.data, lmpass, sizeof(lmp.data));
        memcpy(lmv.data, lmverify, sizeof(lmv.data));
        lmpwd = &lmp;
        lmver = &lmv;

    } else {
        lmpwd = NULL;
        lmver = NULL;
    }

    DCERPC_CALL(_SamrChangePasswordUser2(b, &srv, &acct, &ntp, &ntv,
                                         lm_change, lmpwd, lmver));
    goto_if_ntstatus_not_success(status, error);

cleanup:
    FreeUnicodeString(&acct);
    FreeUnicodeString(&srv);

    return status;

error:
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
