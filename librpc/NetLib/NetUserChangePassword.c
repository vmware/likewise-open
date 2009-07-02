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
#include "NetLibUserInfo.h"


NET_API_STATUS
NetUserChangePassword(
    const wchar16_t *domain,
    const wchar16_t *user,
    const wchar16_t *oldpassword,
    const wchar16_t *newpassword
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    WINERR err = ERROR_SUCCESS;
    handle_t samr_b = NULL;
    char *hostname = NULL;
    wchar16_t *domainname = NULL;
    wchar16_t *username = NULL;
    size_t oldlen = 0;
    size_t newlen = 0;
    uint8 old_nthash[16];
    uint8 new_nthash[16];
    uint8 ntpassbuf[516];
    uint8 ntverhash[16];
    PIO_ACCESS_TOKEN access_token = NULL;

    memset((void*)old_nthash, 0, sizeof(old_nthash));
    memset((void*)new_nthash, 0, sizeof(new_nthash));
    memset((void*)ntpassbuf, 0, sizeof(ntpassbuf));
    memset((void*)ntverhash, 0, sizeof(ntverhash));

    BAIL_ON_INVALID_PTR(domain);
    BAIL_ON_INVALID_PTR(user);
    BAIL_ON_INVALID_PTR(oldpassword);
    BAIL_ON_INVALID_PTR(newpassword);

    status = LwIoGetThreadAccessToken(&access_token);
    BAIL_ON_NT_STATUS(status);

    hostname = awc16stombs(domain);
    BAIL_ON_NO_MEMORY(hostname);

    domainname = wc16sdup(domain);
    BAIL_ON_NO_MEMORY(domainname);

    username = wc16sdup(user);
    BAIL_ON_NO_MEMORY(username);

    status = InitSamrBindingDefault(&samr_b, hostname, access_token);
    BAIL_ON_NTSTATUS_ERROR(status);

    oldlen = wc16slen(oldpassword);
    newlen = wc16slen(newpassword);

    /* prepare NT password hashes */
    md4hash(old_nthash, oldpassword);
    md4hash(new_nthash, newpassword);

    /* encode password buffer */
    EncodePassBufferW16(ntpassbuf, newpassword);
    rc4(ntpassbuf, 516, old_nthash, 16);

    /* encode NT verifier */
    des56(ntverhash, old_nthash, 8, new_nthash);
    des56(&ntverhash[8], &old_nthash[8], 8, &new_nthash[7]);

    status = SamrChangePasswordUser2(samr_b, domainname, username, ntpassbuf,
                                     ntverhash, 0, NULL, NULL);
    BAIL_ON_NTSTATUS_ERROR(status);

cleanup:
    if (samr_b) {
        FreeSamrBinding(&samr_b);
    }

    SAFE_FREE(hostname);
    SAFE_FREE(domainname);
    SAFE_FREE(username);

    if (access_token)
    {
        LwIoDeleteAccessToken(access_token);
    }

    if (err == ERROR_SUCCESS &&
        status != STATUS_SUCCESS) {
        err = NtStatusToWin32Error(status);
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
