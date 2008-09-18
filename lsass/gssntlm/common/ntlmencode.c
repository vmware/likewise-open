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

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *      ntlmencode.c
 *
 * Abstract:
 *
 * endian safe encoding routines
 *
 * Author: Todd Stecher (2007)
 *
 */
#include <ntlmcommon.h>

void
NTLMPutSecBuffer(
    PSEC_BUFFER sb,
    PBYTE ptr,
    ULONG *bufofs,
    ULONG *ofs
    )
{
    PUT_USHORT(ptr,(*ofs),sb->length);
    (*ofs) += sizeof(USHORT);

    PUT_USHORT(ptr,(*ofs),sb->maxLength);
    (*ofs) += sizeof(USHORT);

    PUT_BYTES(ptr,(*bufofs), sb->buffer, sb->length);
    PUT_ULONG(ptr,(*ofs), (*bufofs));
    (*bufofs) += sb->length;
    (*ofs) += sizeof(ULONG);
}

void
NTLMPutSecBufferS(
    PSEC_BUFFER_S sb,
    PBYTE ptr,
    ULONG *bufofs,
    ULONG *ofs
    )
{
    SEC_BUFFER tmp;
    tmp.length = sb->length;
    tmp.maxLength = sb->maxLength;
    tmp.buffer = sb->buffer;

    NTLMPutSecBuffer(&tmp, ptr, bufofs, ofs);
}

void
NTLMPutLsaString(
    PLSA_STRING str,
    PBYTE ptr,
    ULONG *bufofs,
    ULONG *ofs
)
{
    SEC_BUFFER tmp;
    /* messages are never null terminated */
    tmp.length = tmp.maxLength = str->length;
    tmp.buffer = (PBYTE) str->buffer;
    NTLMPutSecBuffer(&tmp, ptr, bufofs,ofs);
}

void
NTLMPutLsaStringAligned(
    PLSA_STRING str,
    PBYTE ptr,
    ULONG *bufofs,
    ULONG *ofs
    )
{
    NTLMPutLsaString(
        str,
        ptr,
        bufofs,
        ofs
        );

    (*bufofs) = ALIGN_UP_8((*bufofs));
}

ULONG
NTLMGetSecBuffer(
    PSEC_BUFFER dst,
    PSEC_BUFFER src,
    ULONG *ofs
    )
{
    ULONG dwError = LSA_ERROR_SUCCESS;
    ULONG bufofs;

    if ((*ofs) + sizeof(SEC_BUFFER) > src->length) 
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_INSUFFICIENT_BUFFER);

    dst->length = GET_USHORT(src->buffer, (*ofs));
    (*ofs) += sizeof(USHORT);

    dst->maxLength = GET_USHORT(src->buffer, (*ofs));
    (*ofs) += sizeof(USHORT);

    bufofs = GET_ULONG(src->buffer, (*ofs));
    (*ofs) += sizeof(ULONG);
        
    if ((bufofs + dst->length) > src->length) 
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_INSUFFICIENT_BUFFER);

    dst->buffer = NTLMAllocateMemory(dst->length);
    if (!dst->buffer)
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_OUT_OF_MEMORY);

    GET_BYTES(dst->buffer, bufofs, src->buffer, dst->length); 

error:
    return dwError;
}

ULONG
NTLMGetSecBufferAlloc(
    PSEC_BUFFER dst,
    PSEC_BUFFER src,
    ULONG *ofs
    )
{
    dst->buffer = NTLMAllocateMemory(src->maxLength);
    if (!dst->buffer)
        return LSA_ERROR_INSUFFICIENT_BUFFER;

    dst->length = src->length;
    dst->maxLength = src->maxLength;

    return NTLMGetSecBuffer(
                    dst, 
                    src, 
                    ofs
                    );
}

ULONG
NTLMGetLsaString(
    PLSA_STRING dst,
    PSEC_BUFFER src,
    ULONG *ofs
    )
{

    return NTLMGetSecBuffer(
                (PSEC_BUFFER) dst,
                src,
                ofs
                );
}


DWORD
NTLMParseMessageHeader(
    PSEC_BUFFER pbuf,
    ULONG *ofs,
    DWORD expectedMsg
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    UCHAR cmp[NTLM_SIGNATURE_SIZE] = NTLM_SIGNATURE;
    UCHAR tmp[NTLM_SIGNATURE_SIZE];

    if ((*ofs) + sizeof(ULONG) + NTLM_SIGNATURE_SIZE >= pbuf->length)
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_INSUFFICIENT_BUFFER);

    GET_BYTES(tmp, (*ofs), pbuf->buffer, NTLM_SIGNATURE_SIZE);
    *ofs += NTLM_SIGNATURE_SIZE;

    if (memcmp(tmp, cmp, NTLM_SIGNATURE_SIZE))
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_INVALID_MESSAGE);

    if (expectedMsg != GET_ULONG(pbuf->buffer, (*ofs)))
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_INVALID_MESSAGE);

    (*ofs) += sizeof(ULONG);

error:

    /* @todo debug */
    return dwError;
}
