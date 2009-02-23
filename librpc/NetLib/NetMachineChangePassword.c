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
#include <lwrpc/mpr.h>
#include <lwps/lwps.h>
#include "NetGetDcName.h"

#define MACHPASS_LEN  (16)

#if !defined(MAXHOSTNAMELEN)
#define MAXHOSTNAMELEN (256)
#endif

NET_API_STATUS
NetMachineChangePassword(
    void
    )
{
    NET_API_STATUS err = ERROR_SUCCESS;
    WINERR conn_err = ERROR_SUCCESS;
    NTSTATUS status = STATUS_SUCCESS;
    NETRESOURCE nr;
    HANDLE hStore = (HANDLE)NULL;
    PLWPS_PASSWORD_INFO pass_info = NULL;
    wchar16_t *username = NULL;
    wchar16_t *oldpassword = NULL;
    wchar16_t *newpassword = NULL;
    wchar16_t *domain_controller_name = NULL;
    size_t domain_controller_name_len = 0;
    char machine_pass[MACHPASS_LEN+1];
    char localname[MAXHOSTNAMELEN];

    memset((void*)&nr, 0, sizeof(nr));
    memset((void*)machine_pass, 0, sizeof(machine_pass));
    memset((void*)localname, 0, sizeof(localname));
    
    if (gethostname((char*)localname, sizeof(localname)) < 0) {
        err = ERROR_INTERNAL_ERROR;
        goto error;
    }

    status = LwpsOpenPasswordStore(LWPS_PASSWORD_STORE_DEFAULT, &hStore);
    goto_if_ntstatus_not_success(status, error);

    status = LwpsGetPasswordByHostName(hStore, localname, &pass_info);
    goto_if_ntstatus_not_success(status, error);

    if (!pass_info) {
        err = ERROR_INTERNAL_ERROR;
        goto error;
    }

    get_random_string(machine_pass, sizeof(machine_pass));

    status = NetpGetDcName(pass_info->pwszDnsDomainName, FALSE,
                           &domain_controller_name);
    goto_if_ntstatus_not_success(status, error);

    username    = pass_info->pwszMachineAccount;
    oldpassword = pass_info->pwszMachinePassword;
    newpassword = ambstowc16s((char*)machine_pass);

    if(newpassword == NULL)
    {
        err = ERROR_OUTOFMEMORY;
        goto error;
    }

    domain_controller_name_len = wc16slen(domain_controller_name);

    status = NetAllocateMemory((void**)&nr.RemoteName,
                               (domain_controller_name_len + 8) *
                               sizeof(wchar16_t),
                               NULL);
    goto_if_ntstatus_not_success(status, error);

    /* specify credentials for domain controller connection */
    sw16printf(nr.RemoteName, "\\\\%S\\IPC$", domain_controller_name);
    conn_err = WNetAddConnection2(&nr, oldpassword, username);
    goto_if_winerr_not_success(conn_err, error);

    err = NetUserChangePassword(domain_controller_name, username,
                                oldpassword, newpassword);
    goto_if_winerr_not_success(err, error);

    err = SaveMachinePassword(
              pass_info->pwszHostname,
              pass_info->pwszMachineAccount,
              pass_info->pwszDomainName,
              pass_info->pwszDnsDomainName,
              domain_controller_name,
              pass_info->pwszSID,
              newpassword);
    goto_if_winerr_not_success(err, error);

    if (pass_info) {
        LwpsFreePasswordInfo(hStore, pass_info);
        pass_info = NULL;
    }

    if (hStore != (HANDLE)NULL) {
        status = LwpsClosePasswordStore(hStore);
        hStore = NULL;
        goto_if_ntstatus_not_success(status, error);
    }

    /* release the domain controller connection creds */
    conn_err = WNetCancelConnection2(nr.RemoteName, 0, 0);
    goto_if_winerr_not_success(conn_err, error);

cleanup:
    if (nr.RemoteName) {
        NetFreeMemory((void*)nr.RemoteName);
    }

    SAFE_FREE(newpassword);
    SAFE_FREE(domain_controller_name);

    if (err == ERROR_SUCCESS &&
        status != STATUS_SUCCESS) {
        err = NtStatusToWin32Error(status);
    }

    return err;

error:
    if (conn_err == ERROR_SUCCESS &&
        nr.RemoteName != NULL) {
        conn_err = WNetCancelConnection2(nr.RemoteName, 0, 0);
    }

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
