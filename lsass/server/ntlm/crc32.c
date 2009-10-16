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
 *        crc32.c
 *
 * Abstract:
 *        CRC32-C generator
 *
 * Authors: Marc Guy (mguy@likewisesoftware.com)
 *
 */
#include "ntlmsrvapi.h"

VOID
ShiftLeft(
    PDWORD nCrc,
    PBYTE pBuffer,
    DWORD nBitOffset
    )
{
    DWORD nByteOffset = nBitOffset / 8;
    DWORD nRemainder = nBitOffset % 8;
    DWORD nResult = *nCrc;

    nResult <<= 1;

    nResult |=
#if 0
        (pBuffer[nByteOffset] & (1 << (7 - nRemainder))) >> (7 - nRemainder);
#else
        (pBuffer[nByteOffset] & (1 << (nRemainder))) >> (nRemainder);
#endif

    *nCrc = nResult;
}

DWORD
NtlmCrc32(
    PBYTE pBuffer,
    DWORD nLength
    )
{
    DWORD nCrc = -1;
    DWORD Poly = 0x04C11DB7;

    DWORD i = 0;
    DWORD nBits = nLength * 8;

    for(i = 0; i < nBits; i++)
    {
        nCrc = LW_ENDIAN_SWAP32(nCrc);

        if(nCrc & 0x80000000)
        {
            ShiftLeft(&nCrc, pBuffer, i);
            nCrc ^= Poly;
        }
        else
        {
            ShiftLeft(&nCrc, pBuffer, i);
        }

        nCrc = LW_ENDIAN_SWAP32(nCrc);

    }

    nCrc = ~nCrc;
    return nCrc;
}
