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

#include "client.h"


BOOLEAN initialized;
#define MAX_USHORT  0xFFFF



/*
 * Helper Exports
 */

VOID
NTLMGssFreeString(PLSA_STRING str)
{
    if (!str)
        return;

    NTLM_SAFE_FREE(str->buffer);
    memset(str, 0, sizeof(LSA_STRING));
}
VOID
NTLMGssFreeSecBuffer(PSEC_BUFFER buf)
{
    if (!buf)
        return;

    NTLM_SAFE_FREE(buf->buffer);
    memset(buf, 0, sizeof(SEC_BUFFER));
}

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
NTLMConvertSecBufferToGSSBufferAllocate(
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
    if (allocate)
    {
        gssBuffer->value = NTLMAllocateMemory(secBuffer->length);
        if (NULL == gssBuffer->value)
            return false;
        memcpy(gssBuffer->value, secBuffer->buffer, secBuffer->length);
        gssBuffer->length = secBuffer->length;
    }
    else
    {
        gssBuffer->value = secBuffer->buffer;
        gssBuffer->length = secBuffer->length;
        secBuffer->buffer = NULL;
        secBuffer->length = secBuffer->maxLength = 0;
    }

    return true;
}

void
NTLMConvertSecBufferToGSSBuffer(
    PSEC_BUFFER secBuffer,
    gss_buffer_t gssBuffer
    )
{
    NTLMConvertSecBufferToGSSBufferAllocate(
                        secBuffer,
                        gssBuffer,
                        false
                        );
}

OM_uint32
NTLMTranslateLsaErrorToGssError(DWORD lsaError)
{
    switch(lsaError) {
        case LSA_ERROR_SUCCESS:
            return GSS_S_COMPLETE;
        case LSA_WARNING_CONTINUE_NEEDED:
            return GSS_S_CONTINUE_NEEDED;
        case LSA_ERROR_BAD_MECH:
            return GSS_S_BAD_MECH;
        case LSA_ERROR_BAD_NAME:
            return GSS_S_BAD_NAME;
        case LSA_ERROR_BAD_NAMETYPE:
            return GSS_S_BAD_NAMETYPE;
        case LSA_ERROR_NO_CONTEXT:
            return GSS_S_NO_CONTEXT;
        case LSA_ERROR_NO_CRED:
            return GSS_S_NO_CRED;
        case LSA_ERROR_NOT_IMPLEMENTED:
            return GSS_S_UNAVAILABLE;
        /* @todo - add others */
        default:
            return GSS_S_FAILURE;
    }

    return GSS_S_FAILURE;
}

/*
 *      Assorted GSSAPI function calls
 */

OM_uint32
ntlm_gss_release_buffer(
    OM_uint32 *minorStatus,
    gss_buffer_t buffer
)
{
    if (buffer->value)
        NTLM_SAFE_FREE(buffer->value);

    buffer->value = NULL;
    buffer->length = 0;

    *minorStatus = GSS_S_COMPLETE;
    return GSS_S_COMPLETE;
}

OM_uint32
ntlm_gss_indicate_mechs(
     OM_uint32 *minor_status,
     gss_OID_set *mech_set
)
{
    return GSS_S_UNAVAILABLE;
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
    return GSS_S_UNAVAILABLE;
}

OM_uint32
NTLMInitializedCheck(
    void
    )
{
    if (!initialized)
        return GSS_S_UNAVAILABLE;

    return GSS_S_COMPLETE;
}


OM_uint32
ntlm_gss_init(OM_uint32 *minorStatus)
{
    DWORD dwError;

    if (initialized)
    {
        *minorStatus = GSS_S_COMPLETE;
        return GSS_S_COMPLETE;
    }

    dwError = NTLMInitializeContextSystem();
    if (dwError)
    {
        *minorStatus = dwError;
        return NTLMTranslateMajorStatus(dwError);
    }

    dwError = NTLMInitializeCredentialSystem();
    if (dwError)
    {
        *minorStatus = dwError;
        return NTLMTranslateMajorStatus(dwError);
    }

    dwError = NTLMInitializeCrypto();
    if (dwError)
    {
        *minorStatus = dwError;
        return NTLMTranslateMajorStatus(dwError);
    }

    dwError = NTLMInitUtilityFunctions();
    if (dwError)
    {
        *minorStatus = dwError;
        return NTLMTranslateMajorStatus(dwError);
    }

    *minorStatus = 0;
    initialized = true;
    return GSS_S_COMPLETE;
}
