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


NET_API_STATUS
NetUserChangePassword(
    IN  PCWSTR  pwszDomainName,
    IN  PCWSTR  pwszUserName,
    IN  PCWSTR  pwszOldPassword,
    IN  PCWSTR  pwszNewPassword
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    WINERR err = ERROR_SUCCESS;
    handle_t hSamrBinding = NULL;
    PSTR pszHostname = NULL;
    PWSTR pwszDomain = NULL;
    PWSTR pwszUser = NULL;
    size_t sOldPasswordLen = 0;
    size_t sNewPasswordLen = 0;
    BYTE OldNtHash[16];
    BYTE NewNtHash[16];
    BYTE NtPasswordBuffer[516];
    BYTE NtVerHash[16];
    PIO_CREDS pCreds = NULL;

    memset(OldNtHash, 0, sizeof(OldNtHash));
    memset(NewNtHash, 0, sizeof(NewNtHash));
    memset(NtPasswordBuffer, 0, sizeof(NtPasswordBuffer));
    memset(NtVerHash, 0, sizeof(NtVerHash));

    BAIL_ON_INVALID_PTR(pwszDomainName, err);
    BAIL_ON_INVALID_PTR(pwszUserName, err);
    BAIL_ON_INVALID_PTR(pwszOldPassword, err);
    BAIL_ON_INVALID_PTR(pwszNewPassword, err);

    ntStatus = LwIoGetActiveCreds(NULL, &pCreds);
    BAIL_ON_NT_STATUS(ntStatus);

    err = LwWc16sToMbs(pwszDomainName, &pszHostname);
    BAIL_ON_WIN_ERROR(err);

    err = LwAllocateWc16String(&pwszDomain, pwszDomainName);
    BAIL_ON_WIN_ERROR(err);

    err = LwAllocateWc16String(&pwszUser, pwszUserName);
    BAIL_ON_WIN_ERROR(err);

    ntStatus = InitSamrBindingDefault(&hSamrBinding, pszHostname, pCreds);
    BAIL_ON_NT_STATUS(ntStatus);

    err = LwWc16sLen(pwszOldPassword, &sOldPasswordLen);
    BAIL_ON_WIN_ERROR(err);

    err = LwWc16sLen(pwszNewPassword, &sNewPasswordLen);
    BAIL_ON_WIN_ERROR(err);

    /* prepare NT password hashes */
    md4hash(OldNtHash, pwszOldPassword);
    md4hash(NewNtHash, pwszNewPassword);

    /* encode password buffer */
    EncodePassBufferW16(NtPasswordBuffer, pwszNewPassword);
    rc4(NtPasswordBuffer, 516, OldNtHash, 16);

    /* encode NT verifier */
    des56(NtVerHash, OldNtHash, 8, NewNtHash);
    des56(&NtVerHash[8], &OldNtHash[8], 8, &NewNtHash[7]);

    ntStatus = SamrChangePasswordUser2(hSamrBinding,
                                       pwszDomain,
                                       pwszUser,
                                       NtPasswordBuffer,
                                       NtVerHash,
                                       0,
                                       NULL,
                                       NULL);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:
    if (hSamrBinding)
    {
        FreeSamrBinding(&hSamrBinding);
    }

    LW_SAFE_FREE_MEMORY(pszHostname);
    LW_SAFE_FREE_MEMORY(pwszDomain);
    LW_SAFE_FREE_MEMORY(pwszUser);

    if (pCreds)
    {
        LwIoDeleteCreds(pCreds);
    }

    if (err == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        err = NtStatusToWin32Error(ntStatus);
    }

    return err;

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
