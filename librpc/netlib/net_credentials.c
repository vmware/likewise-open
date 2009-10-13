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
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        LMcreds.h
 *
 * Abstract:
 *
 *        Network Management API (a.k.a. LanMan API) rpc client library
 *
 *        Session credentials handling functions
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NET_API_STATUS
NetCreateKrb5CredentialsW(
    PWSTR              pwszPrincipal,
    PWSTR              pwszCache,
    NET_CREDS_HANDLE  *phCreds
    )
{
    DWORD err = ERROR_SUCCESS;
    NTSTATUS status = STATUS_SUCCESS;
    LW_PIO_CREDS pCreds = NULL;

    BAIL_ON_INVALID_PTR(pwszPrincipal);
    BAIL_ON_INVALID_PTR(pwszCache);
    BAIL_ON_INVALID_PTR(phCreds);

    status = LwIoCreateKrb5CredsW(pwszPrincipal,
                                  pwszCache,
                                  &pCreds);
    BAIL_ON_NTSTATUS_ERROR(status);

    *phCreds = pCreds;

cleanup:
    if (err == ERROR_SUCCESS &&
        status != STATUS_SUCCESS)
    {
        err = LwNtStatusToWin32Error(status);
    }

    return err;

error:
    if (pCreds)
    {
        LwIoDeleteCreds(pCreds);
    }

    goto cleanup;
}


NET_API_STATUS
NetCreateKrb5CredentialsA(
    PSTR               pszPrincipal,
    PSTR               pszCache,
    NET_CREDS_HANDLE  *phCreds
    )
{
    DWORD err = ERROR_SUCCESS;
    NTSTATUS status = STATUS_SUCCESS;
    LW_PIO_CREDS pCreds = NULL;

    BAIL_ON_INVALID_PTR(pszPrincipal);
    BAIL_ON_INVALID_PTR(pszCache);
    BAIL_ON_INVALID_PTR(phCreds);

    status = LwIoCreateKrb5CredsA(pszPrincipal,
                                  pszCache,
                                  &pCreds);
    BAIL_ON_NTSTATUS_ERROR(status);

    *phCreds = (NET_CREDS_HANDLE)pCreds;

cleanup:
    if (err == ERROR_SUCCESS &&
        status != STATUS_SUCCESS)
    {
        err = LwNtStatusToWin32Error(status);
    }

    return err;

error:
    if (pCreds)
    {
        LwIoDeleteCreds(pCreds);
    }

    goto cleanup;
}


NET_API_STATUS
NetCreateNtlmCredentialsW(
    PWSTR              pwszUsername,
    PWSTR              pwszPassword,
    PWSTR              pwszDomainName,
    DWORD              dwFlags,
    NET_CREDS_HANDLE  *phCreds
    )
{
    DWORD err = ERROR_NOT_SUPPORTED;
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;

    BAIL_ON_INVALID_PTR(phCreds);

    *phCreds = NULL;

cleanup:
    return err;

error:
    goto cleanup;
}


NET_API_STATUS
NetCreateNtlmCredentialsA(
    PSTR               pszUsername,
    PSTR               pszPassword,
    PSTR               pszDomainName,
    DWORD              dwFlags,
    NET_CREDS_HANDLE  *phCreds
    )
{
    DWORD err = ERROR_NOT_SUPPORTED;
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;

    BAIL_ON_INVALID_PTR(phCreds);

    *phCreds = NULL;

cleanup:
    return err;

error:
    goto cleanup;
}


NET_API_STATUS
NetSetCredentials(
    NET_CREDS_HANDLE hCreds
    )
{
    DWORD err = ERROR_SUCCESS;
    NTSTATUS status = STATUS_SUCCESS;
    LW_PIO_CREDS pCreds = (LW_PIO_CREDS)hCreds;

    status = LwIoSetThreadCreds(pCreds);
    BAIL_ON_NTSTATUS_ERROR(status);

cleanup:
    if (err == ERROR_SUCCESS &&
        status != STATUS_SUCCESS)
    {
        err = LwNtStatusToWin32Error(status);
    }

    return err;

error:
    goto cleanup;
}


VOID
NetDeleteCredentials(
    NET_CREDS_HANDLE *phCreds
    )
{
    LW_PIO_CREDS pCreds = NULL;

    if (phCreds == NULL) return;

    pCreds = (LW_PIO_CREDS)(*phCreds);
    LwIoDeleteCreds(pCreds);

    *phCreds = NULL;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
