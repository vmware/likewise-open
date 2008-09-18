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

#include <stdio.h>
#include <string.h>
#include <lwrpc/types.h>
#include <lwrpc/security.h>
#include <md5.h>
#include <hmac_md5.h>
#include <byteops.h>
#include <lwrpc/netrdefs.h>


void NetrCredentialsInit(NetrCredentials *creds,
                         uint8 cli_chal[8], uint8 srv_chal[8],
                         uint8 pass_hash[16], uint32 neg_flags)
{
    struct md5context md5ctx;
    hmac_md5_ctx hmacmd5ctx;

    if (creds == NULL) return;

    creds->negotiate_flags = neg_flags;
    creds->channel_type    = SCHANNEL_WKSTA;  /* default schannel type */

    memcpy(creds->pass_hash, pass_hash, sizeof(creds->pass_hash));

    memcpy(creds->cli_chal.data, cli_chal, sizeof(creds->cli_chal.data));
    memcpy(creds->srv_chal.data, srv_chal, sizeof(creds->srv_chal.data));

    memset(creds->session_key, 0, sizeof(creds->session_key));

    if (creds->negotiate_flags & NETLOGON_NEG_128BIT) {
        uint8 zero[4] = {0};
        uint8 dig[16];

        hmac_md5_init(&hmacmd5ctx, creds->pass_hash, sizeof(creds->pass_hash));
        md5init(&md5ctx);
        md5update(&md5ctx, zero, sizeof(zero));
        md5update(&md5ctx, creds->cli_chal.data, 8);
        md5update(&md5ctx, creds->srv_chal.data, 8);
        md5final(&md5ctx, dig);

        hmac_md5_update(&hmacmd5ctx, dig, sizeof(dig));
        hmac_md5_final(&hmacmd5ctx, creds->session_key);

        des112(creds->cli_cred.data, creds->cli_chal.data, creds->session_key);
        des112(creds->srv_cred.data, creds->srv_chal.data, creds->session_key);
        memcpy(creds->seed.data, creds->cli_chal.data, sizeof(creds->seed.data));
    
    } else {
        uint32 sum1[2];
        uint8 sum2[8];

        sum1[0] = GETUINT32(creds->cli_chal.data, 0) +
                  GETUINT32(creds->srv_chal.data, 0);
        sum1[1] = GETUINT32(creds->cli_chal.data, 4) +
                  GETUINT32(creds->srv_chal.data, 4);
        SETUINT32(sum2, 0, sum1[0]);
        SETUINT32(sum2, 4, sum1[1]);

        memset(creds->session_key, 0, sizeof(creds->session_key));
        des128(creds->session_key, sum2, creds->pass_hash);
        des112(creds->cli_cred.data, creds->cli_chal.data, creds->session_key);
        des112(creds->srv_cred.data, creds->srv_chal.data, creds->session_key);

        memcpy(creds->seed.data, creds->cli_chal.data, sizeof(creds->seed.data));
    }
}


int NetrCredentialsCorrect(NetrCredentials *creds, uint8 srv_creds[8])
{
    int ret;

    if (creds == NULL) return 0;

    ret = memcmp(creds->srv_cred.data, srv_creds, sizeof(creds->srv_cred.data));
    return (ret == 0) ? 1 : 0;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
