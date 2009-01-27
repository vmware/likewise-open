/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see 
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */



/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        open.c
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem (LSMB)
 *
 *        SMB OPEN "wire" API
 *
 * Author: Kaya Bekiroglu (kaya@likewisesoftware.com)
 *
 * @todo: support big endian architectures
 * @todo: support AndX chain parsing
 * @todo: add error logging code
 * @todo: switch to NT error codes where appropriate
 */

#include "includes.h"

typedef struct
{
    uint8_t   FOR_REFERENCE_ONLY;

    wchar16_t name[0];          /* File to open or create */
} CREATE_REQUEST_DATA_non_castable;

/* ASCII is not supported */
/* @todo: test alignment restrictions on Win2k */
uint32_t
MarshallCreateRequestData(
    uint8_t         *pBuffer,
    uint32_t         bufferLen,
    uint8_t          messageAlignment,
    uint32_t        *pBufferUsed,
    const wchar16_t *pwszPath
    )
{
    DWORD dwError = 0;
    uint32_t bufferUsed = 0;
    uint32_t alignment = 0;

    /* Align strings */
    alignment = (bufferUsed + messageAlignment) % 2;
    if (alignment)
    {
        *(pBuffer + bufferUsed) = 0;
        bufferUsed += alignment;
    }

    dwError = SMBPacketAppendUnicodeString(pBuffer, bufferLen, &bufferUsed, pwszPath);
    BAIL_ON_SMB_ERROR(dwError);

error:
    *pBufferUsed = bufferUsed;
    return dwError;
}

uint32_t
UnmarshallSMBResponseCreate(
    const uint8_t  *pBuffer,
    uint32_t        bufferLen,
    CREATE_RESPONSE_HEADER **ppHeader
    )
{
    CREATE_RESPONSE_HEADER* pHeader = (CREATE_RESPONSE_HEADER*) pBuffer;
    uint32_t bufferUsed = sizeof(CREATE_RESPONSE_HEADER);

    /* NOTE: The buffer format cannot be trusted! */
    if (bufferLen < bufferUsed)
        return EBADMSG;

    // byte order conversions
    SMB_LTOH8_INPLACE(pHeader->oplockLevel);
    SMB_LTOH16_INPLACE(pHeader->fid);
    SMB_LTOH32_INPLACE(pHeader->createAction);
    SMB_LTOH64_INPLACE(pHeader->creationTime);
    SMB_LTOH64_INPLACE(pHeader->lastAccessTime);
    SMB_LTOH64_INPLACE(pHeader->lastWriteTime);
    SMB_LTOH64_INPLACE(pHeader->changeTime);
    SMB_LTOH32_INPLACE(pHeader->extFileAttributes);
    SMB_LTOH64_INPLACE(pHeader->allocationSize);
    SMB_LTOH64_INPLACE(pHeader->endOfFile);
    SMB_LTOH16_INPLACE(pHeader->fileType);
    SMB_LTOH16_INPLACE(pHeader->deviceState);
    SMB_LTOH8_INPLACE(pHeader->isDirectory);
    SMB_LTOH16_INPLACE(pHeader->byteCount);

    *ppHeader = pHeader;

    return 0;
}
