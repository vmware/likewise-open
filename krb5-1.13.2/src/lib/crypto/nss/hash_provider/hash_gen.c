/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* lib/crypto/nss/hash_provider/hash_gen.c */
/*
 * Copyright (c) 2010 Red Hat, Inc.
 * All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 *
 *  * Neither the name of Red Hat, Inc., nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "crypto_int.h"
#include "sechash.h"
#include "nss_gen.h"

krb5_error_code
k5_nss_gen_hash(HASH_HashType hashType, const krb5_crypto_iov *data,
                size_t num_data, krb5_data *output)
{
    unsigned int i;
    HASHContext *ctx;
    krb5_error_code ret;

    ret = k5_nss_init();
    if (ret)
        return ret;

    if (output->length != HASH_ResultLen(hashType))
        return KRB5_CRYPTO_INTERNAL;

    ctx = HASH_Create(hashType);
    if (!ctx)
        return ENOMEM;

    HASH_Begin(ctx);
    for (i=0; i < num_data; i++) {
        const krb5_crypto_iov *iov = &data[i];

        if (iov->data.length && SIGN_IOV(iov)) {
            HASH_Update(ctx, (unsigned char *) iov->data.data,
                        iov->data.length);
        }
    }

    HASH_End(ctx, (unsigned char *)output->data,
             &output->length, output->length);
    HASH_Destroy(ctx);

    return 0;
}
