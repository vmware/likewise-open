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
 * Authors: Rafal Szczesniak (rafal@likewisesoftware.com)
 */

#include "includes.h"


VOID
NetrCredentialsInit(
    OUT NetrCredentials *pCreds,
    IN  BYTE             CliChal[8],
    IN  BYTE             SrvChal[8],
    IN  BYTE             PassHash[16],
    IN  UINT32           NegFlags
    )
{
    struct md5context md5ctx;
    hmac_md5_ctx hmacmd5ctx;

    if (pCreds == NULL) return;

    memset((void*)&md5ctx, 0, sizeof(md5ctx));
    memset((void*)&hmacmd5ctx, 0, sizeof(hmacmd5ctx));

    pCreds->negotiate_flags = NegFlags;
    pCreds->channel_type    = SCHANNEL_WKSTA;  /* default schannel type */
    pCreds->sequence        = time(NULL);

    memcpy(pCreds->pass_hash, PassHash, sizeof(pCreds->pass_hash));
    memset(pCreds->session_key, 0, sizeof(pCreds->session_key));

    if (pCreds->negotiate_flags & NETLOGON_NEG_128BIT) {
        BYTE Zero[4] = {0};
        BYTE Digest[16];

        hmac_md5_init(&hmacmd5ctx,
                      pCreds->pass_hash,
                      sizeof(pCreds->pass_hash));

        md5init(&md5ctx);
        md5update(&md5ctx, Zero, sizeof(Zero));
        md5update(&md5ctx, CliChal, 8);
        md5update(&md5ctx, SrvChal, 8);
        md5final(&md5ctx, Digest);

        hmac_md5_update(&hmacmd5ctx,
                        Digest,
                        sizeof(Digest));
        hmac_md5_final(&hmacmd5ctx,
                       pCreds->session_key);

        des112(pCreds->cli_chal.data,
               CliChal,
               pCreds->session_key);
        des112(pCreds->srv_chal.data,
               SrvChal,
               pCreds->session_key);

        memcpy(pCreds->seed.data,
               pCreds->cli_chal.data,
               sizeof(pCreds->seed.data));
    
    } else {
        UINT32 Sum1[2];
        BYTE Sum2[8];

        Sum1[0] = GETUINT32(CliChal, 0) +
                  GETUINT32(SrvChal, 0);
        Sum1[1] = GETUINT32(CliChal, 4) +
                  GETUINT32(SrvChal, 4);

        SETUINT32(Sum2, 0, Sum1[0]);
        SETUINT32(Sum2, 4, Sum1[1]);

        memset(pCreds->session_key,
               0,
               sizeof(pCreds->session_key));

        des128(pCreds->session_key,
               Sum2,
               pCreds->pass_hash);
        des112(pCreds->cli_chal.data,
               CliChal,
               pCreds->session_key);
        des112(pCreds->srv_chal.data,
               SrvChal,
               pCreds->session_key);

        memcpy(pCreds->seed.data,
               pCreds->cli_chal.data,
               sizeof(pCreds->seed.data));
    }
}


BOOLEAN
NetrCredentialsCorrect(
    IN  NetrCredentials  *pCreds,
    IN  BYTE              SrvCreds[8]
    )
{
    BOOLEAN bCorrect = FALSE;

    if (pCreds == NULL) goto error;

    if (memcmp(pCreds->srv_chal.data,
               SrvCreds,
               sizeof(pCreds->srv_chal.data)) == 0)
    {
        bCorrect = TRUE;
    }

cleanup:
    return bCorrect;

error:
    goto cleanup;
}


VOID
NetrCredentialsCliStep(
    IN OUT NetrCredentials *pCreds
    )
{
    NetrCred Chal;

    memset((void*)&Chal, 0, sizeof(Chal));

    memcpy(Chal.data,
           pCreds->seed.data,
           sizeof(Chal.data));
    SETUINT32(Chal.data,
              0,
              GETUINT32(pCreds->seed.data, 0) + pCreds->sequence);
    des112(pCreds->cli_chal.data,
           Chal.data,
           pCreds->session_key);


    memcpy(Chal.data,
           pCreds->seed.data,
           sizeof(Chal.data));
    SETUINT32(Chal.data,
              0,
              GETUINT32(pCreds->seed.data, 0) + pCreds->sequence + 1);
    des112(pCreds->srv_chal.data,
           Chal.data,
           pCreds->session_key);

    /* reseed */
    memcpy(Chal.data,
           pCreds->seed.data,
           sizeof(Chal.data));
    SETUINT32(Chal.data,
              0,
              GETUINT32(pCreds->seed.data, 0) + pCreds->sequence + 1);

    pCreds->seed = Chal;
}


VOID
NetrCredentialsSrvStep(
    IN OUT NetrCredentials *pCreds
    )
{
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
