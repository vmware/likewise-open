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

#ifndef _SCHANNEL_H_
#define _SCHANNEL_H_

#include <schtypes.h>

extern const unsigned char schannel_sig_sign[];
extern const unsigned char schannel_sig_seal[];

struct schn_auth_ctx {
    uint8           session_key[16];
    unsigned char  *domain_name;
    unsigned char  *fqdn;
    unsigned char  *machine_name;
    uint32          sender_flags;
    uint32          seq_num;
    uint32          initialized;
    uint16          sign_type;
    uint16          seal_type;
    uint16          pad;
    uint16          flags;
};


struct schn_creds {
    uint32          flags1;
    uint32          flags2;
    unsigned char  *domain_name;
    unsigned char  *machine_name;
};


struct schn_tail {
    uint8           signature[8];
    uint8           seq_number[8];
    uint8           digest[8];
    uint8           nonce[8];
};


struct schn_blob {
    void          *base;
    size_t         len;
};


struct schn_nl_auth_sha2_signature {
    uint16 sign_alg;
    uint16 seal_alg;
    uint16 pad;
    uint16 flags;
};


#define SCHANNEL_SEC_LEVEL_INTEGRITY   (5)
#define SCHANNEL_SEC_LEVEL_PRIVACY     (6)


#define SCHANNEL_INITIATOR_FLAGS       (0x0080)
#define SCHANNEL_ACCEPTOR_FLAGS        (0x0000)


#define SCHANNEL_SIGN_ALG_UNDEF        (0)
#define SCHANNEL_SIGN_ALG_AES          (0x0013)
#define SCHANNEL_SIGN_ALG_MD4          (0x0077)


#define SCHANNEL_SEAL_ALG_UNDEF        (0)
#define SCHANNEL_SEAL_ALG_AES          (0x001a)
#define SCHANNEL_SEAL_ALG_MD4          (0x007a)

#define SEC_E_MESSAGE_ALTERED          (0x8009030F)
#define SEC_E_OUT_OF_SEQUENCE          (0x80090310)

#define SCHANNEL_MSG_TYPE_REQUEST            (0)
#define SCHANNEL_MSG_TYPE_RESPONSE           (1)

#define SCHANNEL_MSG_FLAGS_NETBIOS_DOMAIN    (0x00000001)
#define SCHANNEL_MSG_FLAGS_NETBIOS_HOST      (0x00000002)
#define SCHANNEL_MSG_FLAGS_DNS_DOMAIN        (0x00000004)
#define SCHANNEL_MSG_FLAGS_DNS_HOST          (0x00000008)
#define SCHANNEL_MSG_FLAGS_NETBIOS_HOST_UTF8 (0x00000010)
#define SCHANNEL_MSG_FLAGS_FQDN              (SCHANNEL_MSG_FLAGS_NETBIOS_HOST_UTF8 | SCHANNEL_MSG_FLAGS_DNS_DOMAIN)

uint32 schn_init_creds(struct schn_auth_ctx *ctx,
                       struct schn_blob     *creds);

uint32 schn_wrap(void                 *sec_ctx,
                 uint32                sec_level,
                 struct schn_blob     *in,
                 struct schn_blob     *out,
                 struct schn_tail     *tail);

uint32 schn_unwrap(void                 *sec_ctx,
                   uint32                sec_level,
                   struct schn_blob     *in,
                   struct schn_blob     *out,
                   struct schn_tail     *tail);

void schn_free_blob(struct schn_blob *b);


#endif /* _SCHANNEL_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
