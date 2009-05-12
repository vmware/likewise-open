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

#include "ntlm.h"





/* conversion routines gss -> lsa -> gss */


BOOLEAN
NTLMConvertGSSBufferToSecBufferAllocate(
    gss_buffer_t gssBuffer,
    PSEC_BUFFER secBuffer,
    BOOLEAN allocate
    )
{
    if (gssBuffer->length > MAX_USHORT)
        return false;

    if (gssBuffer->length == 0) {
        secBuffer->buffer = NULL;
    } else {
        if (allocate) {
            secBuffer->buffer = NTLMAllocateMemory(gssBuffer->length);
            if (NULL == secBuffer->buffer)
                return false;
            memcpy(secBuffer->buffer, gssBuffer->value, gssBuffer->length);
        } else
            secBuffer->buffer = gssBuffer->value;
    }

    secBuffer->length = secBuffer->maxLength = gssBuffer->length;

    return true;
}

void
NTLMConvertGSSBufferToSecBuffer(
    gss_buffer_t gssBuffer,
    PSEC_BUFFER secBuffer
    )
{
    NTLMConvertGSSBufferToSecBufferAllocate(
                gssBuffer,
                secBuffer,
                false);
}

BOOLEAN
NTLMConvertSecBufferToGSS(
    PSEC_BUFFER secBuffer,
    gss_buffer_t gssBuffer,
    BOOLEAN allocate
    )
{
    if (secBuffer->length == 0) {
        gssBuffer->value = NULL;
        gssBuffer->length = 0;
        return true;
    }

    /*
     * A note about secbuffers - the length is data, maxLength is buffer
     * size.  Only worry about filled in data - typically length == max_length
     */
    if (allocate) {
        gssBuffer->value = NTLMAllocateMemory(secBuffer->length);
        if (NULL == gssBuffer->value)
            return false;
        memcpy(gssBuffer->value, secBuffer->buffer, secBuffer->length);
    } else
        gssBuffer->value = secBuffer->buffer;

    gssBuffer->length = secBuffer->length;

    return true;
}










OM_uint32
ntlm_gss_indicate_mechs(
     OM_uint32 *minor_status,
     gss_OID_set *mech_set
	)
{
}


OM_uint32
ntlm_gss_display_status(
     OM_uint32 *minor_status,
     OM_uint32 status_value,
     int status_type,
     gss_OID mech_type,
     OM_uint32 *message_context,
     gss_buffer_t status_string
	)
{
}


OM_uint32
gss_release_buffer(
    OM_uint32 *minorStatus,

