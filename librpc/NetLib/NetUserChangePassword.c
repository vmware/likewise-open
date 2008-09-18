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


NET_API_STATUS NetUserChangePassword(const wchar16_t *domain,
				     const wchar16_t *username,
				     const wchar16_t *oldpassword,
				     const wchar16_t *newpassword)
{
    NTSTATUS status;
    handle_t samr_b;
    unsigned char *hostname;
    size_t oldlen, newlen;
    uint8 old_nthash[16], new_nthash[16];
    uint8 ntpassbuf[516], ntverhash[16];

    hostname = (unsigned char*)awc16stombs(domain);
    if (hostname == NULL) return NtStatusToWin32Error(STATUS_NO_MEMORY);

    status = InitSamrBindingDefault(&samr_b, hostname);
    if (status != 0) goto done;

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

    status = SamrChangePasswordUser2(samr_b, domain, username, ntpassbuf,
				     ntverhash, 0, NULL, NULL);
    if (status != 0) goto done;

done:
    FreeSamrBinding(&samr_b);
    SAFE_FREE(hostname);

    return NtStatusToWin32Error(status);
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
