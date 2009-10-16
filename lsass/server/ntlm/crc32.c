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
#include <krb5.h>

DWORD
NtlmCrc32(
    PBYTE pBuffer,
    DWORD dwBufferSize,
    PDWORD pdwCrc32
    )
{
    DWORD dwCrc32 = 0;
    DWORD dwError = LW_ERROR_SUCCESS;
    krb5_error_code KrbError= 0;
    krb5_data Input;
    krb5_checksum Output;

    memset(&Input, 0, sizeof(Input));
    memset(&Output, 0, sizeof(Output));

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

    if(KrbError)
    {
        dwError = LW_ERROR_KRB5_CALL_FAILED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    LW_ASSERT(Output.length == 4);

    memcpy(&dwCrc32, Output.contents, Output.length);

cleanup:
    krb5_free_checksum_contents(NULL, &Output);
    return *pdwCrc32 = dwCrc32;
error:
    dwCrc32 = 0;
    goto cleanup;
}
