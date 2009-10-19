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
 * Authors: Kyle Stemen <kstemen@likewise.com>
 *
 */
#include "ntlmsrvapi.h"
#include <krb5.h>

DWORD
NtlmCrc32(
    PBYTE pBuffer,
    DWORD dwBufferSize,
    PDWORD pdwCrc32
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    krb5_error_code KrbError= 0;
    krb5_data Input;
    krb5_checksum Output;
    DWORD dwChecksum = 0;

    memset(&Input, 0, sizeof(Input));
    memset(&Output, 0, sizeof(Output));

    // NTLM uses the "Preset to -1" and "Post-invert" variant of CRC32, where
    // the checksum is initialized to -1 before taking the input data into
    // account. After processing the input data, the one's complement of the
    // checksum is calculated. (see
    // http://en.wikipedia.org/wiki/Computation_of_CRC#Preset_to_.E2.88.921 )

    if (dwBufferSize < 4)
    {
        // Calculating this 4 byte magic sequence will set the current checksum
        // to -1. The remaining 4 bytes are used for the input data.
        BYTE pbTemp[] = { 0x62, 0xF5, 0x26, 0x92, 0, 0, 0, 0 };
        memcpy(pbTemp + 4, pBuffer, dwBufferSize);

        Input.data = pbTemp;
        Input.length = 4 + dwBufferSize;

        KrbError = krb5_c_make_checksum(
            NULL,
            CKSUMTYPE_CRC32,
            NULL,
            0,
            &Input,
            &Output
            );
    }
    else
    {
        // Initializing the checksum to -1 is the same as inverting the first
        // 32bits of the input data. So this will be temporarily done on the
        // input data.
        int i = 0;
        for (i = 0; i < 4; i++)
        {
            pBuffer[i] ^= 0xFF;
        }

        Input.data = (PCHAR)pBuffer;
        Input.length = dwBufferSize;

        KrbError = krb5_c_make_checksum(
            NULL,
            CKSUMTYPE_CRC32,
            NULL,
            0,
            &Input,
            &Output
            );

        for (i = 0; i < 4; i++)
        {
            pBuffer[i] ^= 0xFF;
        }
    }

    if(KrbError)
    {
        dwError = LW_ERROR_KRB5_CALL_FAILED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    LW_ASSERT(Output.length == 4);

    memcpy(&dwChecksum, Output.contents, Output.length);
    // Do the post-invert
    *pdwCrc32 = dwChecksum ^ 0xFFFFFFFF;

cleanup:
    krb5_free_checksum_contents(NULL, &Output);
    return dwError;

error:
    *pdwCrc32 = 0;
    goto cleanup;
}
