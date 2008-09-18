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
 *        parse.c
 *
 * Abstract:
 *
 *        Primitive parsing routines
 *
 * Author: Todd Stecher (2007)
 *
 */
#include "includes.h"

/*
 * ParseBytes()
 *
 * @brief Parses bytes from a PSEC_BUFFER, adjusting its length,
 * and pointer values.
 *
 * @param in pBuf - input buffer - adjusted during parsing.
 * @param in cBytes - number of bytes to parse
 * @param out pOutbuf - pointer at data buffer.
 */
bool
ParseBytes(
    PSEC_BUFFER pBuf, 
    DWORD cBytes, 
    PBYTE *pOutbuf
    )
{
    NEED_SIZE(cBytes);
    *pOutbuf = (PBYTE) pBuf->buffer;
    pBuf->length -= cBytes;
    pBuf->maxLength -= cBytes; /*@todo - adjust this? */
    pBuf->buffer += cBytes;
    return true;
}

/*
 * ParseUINT32()
 *
 * @brief Parses UINT32 from a PSEC_BUFFER, adjusting its length,
 * and pointer values.
 *
 * @param in pBuf - input buffer - adjusted during parsing.
 * @param out pUINT32 - resultant value
 */
bool
ParseUINT32(
    PSEC_BUFFER pBuf, 
    UINT32 *pUINT32
    )
{
    bool ret;
    UINT32 *local;
    /* @todo - adjust for endian */
    if (ParseBytes(pBuf, sizeof(UINT32), (PBYTE*) &local)) {
        *pUINT32 = *local;
        return true;
    }

    return false;
}

/*
 * ParseUINT64()
 *
 * @brief Parses UINT64 from a PSEC_BUFFER, adjusting its length,
 * and pointer values.
 *
 * @param in pBuf - input buffer - adjusted during parsing.
 * @param out pUINT64 - resultant value
 */
bool
ParseUINT64(
    PSEC_BUFFER pBuf, 
    UINT64 *pUINT64
)
{
    /* @todo - adjust for endian */
    return ParseBytes(pBuf, sizeof(UINT64), (PBYTE*) pUINT64);
}


/*
 * ParseLsaString
 *
 * @brief Pulls unicode_string from sec buffer, adjusting
 * its length and pointer values.
 *
 * @param in pBase - start of original secbuffer - used for offset
 * @param in pBuf - input buffer - adjusted during parsing.
 * @param out pLsaString - string 
 *
 * @note  pLsaString MUST BE FREED by calling LsaFreeLsaString()
 *
 */

bool
ParseLsaString(
    PSEC_BUFFER pBase,
    PSEC_BUFFER pBuf,
    PLSA_STRING pLsaString
)
{
    LSA_STRING tmp;
    DWORD bufNeeded;

    NEED_SIZE(sizeof(LSA_STRING));

    /* overflow  - need a better ptr macro here @todo*/
    bufNeeded = ((DWORD) pBuf->buffer) + pBuf->maxLength;
    memcpy(&tmp, pBuf, sizeof(LSA_STRING));

    /* make sure there's room in the message for string */
    if (pBase->length < bufNeeded) 
        return false;

    tmp.buffer = (wchar16_t*) (OFFSET_TO_PTR(pBase->buffer, tmp.buffer));

    BAIL_ON_LSA_ERROR(LsaCopyLsaString(pLsaString, &tmp));

    return true;

error:
    return false;
}





