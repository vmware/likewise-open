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



NET_API_STATUS
NetMachineChangePassword(
    void
    )
{
    NET_API_STATUS err = ERROR_SUCCESS;
    WINERR conn_err = ERROR_SUCCESS;
    NTSTATUS status = STATUS_SUCCESS;
    HANDLE hStore = (HANDLE)NULL;
    PLWPS_PASSWORD_INFO pass_info = NULL;
    wchar16_t *username = NULL;
    wchar16_t *oldpassword = NULL;
    wchar16_t *newpassword = NULL;
    wchar16_t *domain_controller_name = NULL;
    size_t domain_controller_name_len = 0;
    char machine_pass[MACHPASS_LEN+1];
    char *localname = NULL;

    memset((void*)machine_pass, 0, sizeof(machine_pass));

    err = NetGetHostInfo(&localname);
    BAIL_ON_WINERR_ERROR(err);

    status = LwpsOpenPasswordStore(LWPS_PASSWORD_STORE_DEFAULT, &hStore);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = LwpsGetPasswordByHostName(hStore, localname, &pass_info);
    BAIL_ON_NTSTATUS_ERROR(status);

    if (!pass_info) {
        err = ERROR_INTERNAL_ERROR;
        goto error;
    }

    get_random_string(machine_pass, sizeof(machine_pass));

    status = NetpGetRwDcName(pass_info->pwszDnsDomainName, FALSE,
                           &domain_controller_name);
    BAIL_ON_NTSTATUS_ERROR(status);

    username    = pass_info->pwszMachineAccount;
    oldpassword = pass_info->pwszMachinePassword;
    newpassword = ambstowc16s((char*)machine_pass);

    if(newpassword == NULL)
    {
        err = ERROR_OUTOFMEMORY;
        goto error;
    }

    domain_controller_name_len = wc16slen(domain_controller_name);

    err = NetUserChangePassword(domain_controller_name, username,
                                oldpassword, newpassword);
    BAIL_ON_WINERR_ERROR(err);

    err = SaveMachinePassword(
              pass_info->pwszHostname,
              pass_info->pwszMachineAccount,
              pass_info->pwszHostDnsDomain ? pass_info->pwszHostDnsDomain
                                           : pass_info->pwszDnsDomainName,
              pass_info->pwszDomainName,
              pass_info->pwszDnsDomainName,
              domain_controller_name,
              pass_info->pwszSID,
              newpassword);
    BAIL_ON_WINERR_ERROR(err);

    if (pass_info) {
        LwpsFreePasswordInfo(hStore, pass_info);
        pass_info = NULL;
    }

    if (hStore != (HANDLE)NULL) {
        status = LwpsClosePasswordStore(hStore);
        hStore = NULL;
        BAIL_ON_NTSTATUS_ERROR(status);
    }

cleanup:
    if (localname)
    {
        NetFreeMemory(localname);
    }

    SAFE_FREE(newpassword);
    SAFE_FREE(domain_controller_name);

    if (err == ERROR_SUCCESS &&
        status != STATUS_SUCCESS) {
        err = NtStatusToWin32Error(status);
    }

    return err;

error:
    if (pass_info) {
        LwpsFreePasswordInfo(hStore, pass_info);
    }

    if (hStore != (HANDLE)NULL) {
        LwpsClosePasswordStore(hStore);
    }

    if (err == ERROR_SUCCESS &&
        status == STATUS_SUCCESS &&
        conn_err != ERROR_SUCCESS) {
        /* overwrite error code with connection error code only if everything
           else was fine so far */
        err = conn_err;
    }

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
