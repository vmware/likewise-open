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
#include <lwrpc/mpr.h>
#include <lwps/lwps.h>
#include "NetGetDcName.h"

#define MACHPASS_LEN  (16)

#if !defined(MAXHOSTNAMELEN)
#define MAXHOSTNAMELEN (256)
#endif

NET_API_STATUS NetMachineChangePassword()
{
    NET_API_STATUS err = ERROR_SUCCESS;
    NTSTATUS status = STATUS_SUCCESS;
    int ret = 0;
    NETRESOURCE nr;
    HANDLE hStore = (HANDLE)NULL;
    PLWPS_PASSWORD_INFO pass_info = NULL;
    wchar16_t *username = NULL;
    wchar16_t *oldpassword = NULL;
    wchar16_t *newpassword = NULL;
    wchar16_t *domain_controller_name = NULL;
    size_t domain_controller_name_len;
    char machine_pass[MACHPASS_LEN+1];
    char localname[MAXHOSTNAMELEN];
    
    if (gethostname((char*)localname, sizeof(localname)) < 0) {
        /* TODO: figure out better error code */
        return ERROR_INVALID_PARAMETER;
    }

    status = LwpsOpenPasswordStore(LWPS_PASSWORD_STORE_DEFAULT, &hStore);
    if (status != 0) {
        err = NtStatusToWin32Error(status);
        goto done;
    }

    status = LwpsGetPasswordByHostName(hStore, localname, &pass_info);
    if (status != 0) {
        err = NtStatusToWin32Error(status);
        goto done;
    }

    if (!pass_info) {
        err = NtStatusToWin32Error(STATUS_INTERNAL_ERROR);
        goto done;
    }

    get_random_string(machine_pass, sizeof(machine_pass));

    status = NetpGetDcName(pass_info->pwszDnsDomainName, FALSE,
                           &domain_controller_name);
    if (status != 0) {
        err = NtStatusToWin32Error(status);
        goto done;
    }

    username        = pass_info->pwszMachineAccount;
    oldpassword     = pass_info->pwszMachinePassword;
    newpassword     = ambstowc16s((char*)machine_pass);

    if(newpassword == NULL)
    {
        err = NtStatusToWin32Error(STATUS_NO_MEMORY);
        goto done;
    }

    domain_controller_name_len = wc16slen(domain_controller_name);
    nr.RemoteName = (wchar16_t*) malloc((domain_controller_name_len + 8) *
                                        sizeof(wchar16_t));
    if (nr.RemoteName == NULL) {
        err = NtStatusToWin32Error(STATUS_NO_MEMORY);
        goto done;
    }

    /* specify credentials for domain controller connection */
    sw16printf(nr.RemoteName, "\\\\%S\\IPC$", domain_controller_name);
    ret = WNetAddConnection2(&nr, oldpassword, username);
    if (ret != 0) return err;

    err = NetUserChangePassword(domain_controller_name, username,
                                oldpassword, newpassword);
    if (err != 0) goto done;

    err = SaveMachinePassword(
              pass_info->pwszHostname,
              pass_info->pwszDomainName,
              pass_info->pwszDnsDomainName,
              domain_controller_name,
              pass_info->pwszSID,
              newpassword);

    if (err != 0) goto done;

done:

    /* release the domain controller connection creds */
    err = WNetCancelConnection2(nr.RemoteName, 0, 0);
    if (err != 0) return err;

    /* We're hoping for best here, so ignore returned status */
    if (pass_info) {
       LwpsFreePasswordInfo(hStore, pass_info);
    }

    if (hStore != (HANDLE)NULL) {
        status = LwpsClosePasswordStore(hStore);
    }

    SAFE_FREE(nr.RemoteName);
    SAFE_FREE(newpassword);
    SAFE_FREE(domain_controller_name);

    return err;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
