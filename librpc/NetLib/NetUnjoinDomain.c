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
#include "NetUtil.h"
#include <lwrpc/mpr.h>
#include <lwps/lwps.h>


#if !defined(MAXHOSTNAMELEN)
#define MAXHOSTNAMELEN (256)
#endif

NET_API_STATUS NetUnjoinDomainLocal(const wchar16_t *machine, 
                                    const wchar16_t *domain,
                                    const wchar16_t *account,
                                    const wchar16_t *password,
                                    uint32 options)
{
    const uint32 domain_access  = DOMAIN_ACCESS_ENUM_ACCOUNTS |
                                  DOMAIN_ACCESS_OPEN_ACCOUNT |
                                  DOMAIN_ACCESS_LOOKUP_INFO_2 |
                                  DOMAIN_ACCESS_CREATE_USER;
    NETRESOURCE nr;
    int err;
    PolicyHandle account_h;
    HANDLE hStore = (HANDLE)NULL;
    PLWPS_PASSWORD_INFO pi = NULL;
    NetConn *conn = NULL;
    NTSTATUS status = STATUS_SUCCESS;
    NTSTATUS close_status = STATUS_SUCCESS;
    char localname[MAXHOSTNAMELEN];
    wchar16_t *domain_controller_name = NULL;
    size_t domain_controller_name_len;
    BOOLEAN wnet_connected = FALSE;
    wchar16_t *machine_name = NULL;
    PIO_ACCESS_TOKEN access_token = NULL;

    if (gethostname((char*)localname, sizeof(localname)) < 0) {
       /* TODO: figure out better error code */
       return ERROR_INVALID_PARAMETER;
    }

    machine_name = wc16sdup(machine);
    goto_if_no_memory_ntstatus(machine_name, done);

    status = NetpGetDcName(domain, FALSE, &domain_controller_name);
    goto_if_ntstatus_not_success(status, done);

    domain_controller_name_len = wc16slen(domain_controller_name);
    nr.RemoteName = (wchar16_t*) malloc((domain_controller_name_len + 8) * sizeof(wchar16_t));
    goto_if_no_memory_winerr(nr.RemoteName, done);

    /* specify credentials for domain controller connection */
    sw16printf(nr.RemoteName, "\\\\%S\\IPC$", domain_controller_name);
    err = WNetAddConnection2(&nr, password, account);
    if (err != ERROR_SUCCESS) {
        status = Win32ErrorToNtStatus(err);
        goto done;
    }
    wnet_connected = TRUE;

    status = LwpsOpenPasswordStore(LWPS_PASSWORD_STORE_DEFAULT, &hStore);
    goto_if_ntstatus_not_success(status, done);

    status = LwpsGetPasswordByHostName(hStore, localname, &pi);
    goto_if_ntstatus_not_success(status, done);

    /* zero the machine password */
    memset((void*)pi->pwszMachinePassword, 0,
           wc16slen(pi->pwszMachinePassword));
    pi->last_change_time = time(NULL);

    status = LwpsWritePasswordToAllStores(pi);
    goto_if_ntstatus_not_success(status, done);

    /* disable the account only if requested */
    if (options & NETSETUP_ACCT_DELETE) {
        if (account && password)
        {
            status = LwIoCreatePlainAccessTokenW(account, password, &access_token);
            goto_if_ntstatus_not_success(status, done);
        }
        else
        {
            status = LwIoGetThreadAccessToken(&access_token);
            goto_if_ntstatus_not_success(status, done);
        }

        status = NetConnectSamr(&conn, domain_controller_name, domain_access, 0, access_token);
        if (status != STATUS_SUCCESS) goto done;

        status = DisableWksAccount(conn, machine_name, &account_h);
        /* there's no need to check status code just return it */

        close_status = NetDisconnectSamr(conn);
        if (status == STATUS_SUCCESS &&
            close_status != STATUS_SUCCESS) {
            return NtStatusToWin32Error(close_status);
        }
    }

done:

    if (pi) {
        LwpsFreePasswordInfo(hStore, pi);
    }

    if (hStore != (HANDLE)NULL) {
       LwpsClosePasswordStore(hStore);
    }

    /* release the domain controller connection creds */
    if (wnet_connected)
    {
        err = WNetCancelConnection2(nr.RemoteName, 0, 0);
        if (err != 0 && status != 0)
        {
            status = Win32ErrorToNtStatus(err);
        }
    }

    if (access_token)
    {
        LwIoDeleteAccessToken(access_token);
    }

    SAFE_FREE(nr.RemoteName);
    SAFE_FREE(domain_controller_name);
    SAFE_FREE(machine_name);

    return NtStatusToWin32Error(status);
}


NET_API_STATUS NetUnjoinDomain(const wchar16_t *hostname,
                               const wchar16_t *account,
                               const wchar16_t *password,
                               uint32 options)
{
    NET_API_STATUS err = ERROR_SUCCESS;
    NTSTATUS status, close_status;
    wchar16_t *domain = NULL;
    HANDLE hStore = (HANDLE)NULL;
    PLWPS_PASSWORD_INFO pi = NULL;

    /* at the moment we support only locally triggered join */
    if (hostname) {
        status = STATUS_NOT_IMPLEMENTED;

    } else {
        char hostname[MAXHOSTNAMELEN];
        wchar16_t host[MAXHOSTNAMELEN];

        if (gethostname((char*)hostname, sizeof(hostname)) < 0) {
            /* TODO: figure out better error code */
            return ERROR_INVALID_PARAMETER;
        }

        mbstowc16s(host, hostname, sizeof(wchar16_t)*MAXHOSTNAMELEN);

        status = LwpsOpenPasswordStore(LWPS_PASSWORD_STORE_DEFAULT, &hStore);
        if (status != STATUS_SUCCESS) {
            err = NtStatusToWin32Error(status);
            goto close;
        }

        status = LwpsGetPasswordByHostName(hStore, hostname, &pi);
        if (status != STATUS_SUCCESS) {
            err = NtStatusToWin32Error(status);
            goto close;
        }

        domain = pi->pwszDnsDomainName;

        err = NetUnjoinDomainLocal(host, domain, account, password, options);
    }

close:
 
    if (pi) {
       LwpsFreePasswordInfo(hStore, pi);
    }

    if (hStore != (HANDLE)NULL) {
       close_status = LwpsClosePasswordStore(hStore);
       if (err == ERROR_SUCCESS &&
           close_status != STATUS_SUCCESS) {
           err = NtStatusToWin32Error(close_status);
           return err;
       }
    }

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
