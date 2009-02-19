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
 *        tree_connect.c
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem (LSMB)
 *
 *        SMB TREE CONNECT "wire" API
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

    uint8_t   password[0];      /* Password */
    wchar16_t path[0];          /* Server name and share name */
    /* @todo: resolve inconsistency in SNIA spec which lists service as a
       Unicode string. */
    uchar8_t  service[0];       /* Service name */
} TREE_CONNECT_REQUEST_DATA_non_castable;

/* ASCII strings are not supported */
/* @todo: test alignment restrictions on Win2k */
uint32_t
MarshallTreeConnectRequestData(
    OUT uint8_t* pBuffer,
    IN uint32_t bufferLen,
    IN uint8_t messageAlignment,
    OUT uint32_t* pBufferUsed,
    IN const wchar16_t* pwszPath,
    IN const char* pszService
    )
{
    DWORD dwError = 0;
    uint32_t bufferUsed = 0;
    uint32_t alignment = 0;

    /* The password field is obsolete in modern dialects */

    /* Align string */
    alignment = (bufferUsed + messageAlignment) % 2;
    if (alignment)
    {
        *(pBuffer + bufferUsed) = 0;
        bufferUsed += alignment;
    }

    dwError = SMBPacketAppendUnicodeString(pBuffer, bufferLen, &bufferUsed, pwszPath);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBPacketAppendString(pBuffer, bufferLen, &bufferUsed, pszService);
    BAIL_ON_SMB_ERROR(dwError);

error:
    *pBufferUsed = bufferUsed;

    return dwError;
}

uint32_t
UnmarshallTreeConnectRequest(
    const uint8_t *pBuffer,
    uint32_t       bufferLen,
    uint8_t        messageAlignment,
    TREE_CONNECT_REQUEST_HEADER **ppHeader,
    uint8_t      **ppPassword,
    wchar16_t    **ppwszPath,
    uchar8_t     **ppszService
    )
{
    /* NOTE: The buffer format cannot be trusted! */
    uint32_t bufferUsed = sizeof(TREE_CONNECT_REQUEST_HEADER);
    if (bufferLen < bufferUsed)
        return EBADMSG;

    /* @todo: endian swap as appropriate */
    *ppHeader = (TREE_CONNECT_REQUEST_HEADER*) pBuffer;

    uint32_t passwordLen = (*ppHeader)->passwordLength;
    bufferUsed += passwordLen;

    if (bufferUsed > bufferLen)
        return EBADMSG;

    if (passwordLen == 0)
    {
        *ppPassword = NULL;     /* Zero length password */
    }
    else
    {
        *ppPassword = (uint8_t*) pBuffer;
    }

    /* Align strings */
    bufferUsed += (bufferUsed + messageAlignment) % 2;
    if (bufferUsed > bufferLen)
    {
        return EBADMSG;
    }

    *ppwszPath = (wchar16_t *) (pBuffer + bufferUsed);
    bufferUsed += sizeof(wchar16_t) * wc16snlen(*ppwszPath,
        (bufferLen - bufferUsed) / sizeof(wchar16_t)) + sizeof(WNUL);
    if (bufferUsed > bufferLen)
    {
        return EBADMSG;
    }

    *ppszService = (uchar8_t *) (pBuffer + bufferUsed);
    bufferUsed += sizeof(wchar16_t) * strnlen((char*) *ppszService,
        bufferLen - bufferUsed) + sizeof(NUL);
    if (bufferUsed > bufferLen)
    {
        return EBADMSG;
    }

    return 0;
}

typedef struct
{
    uint8_t   FOR_REFERENCE_ONLY;

    uchar8_t  service[0];      /* Service type connected (always ASCII) */
    wchar16_t nativeFileSystem[0];  /* Native file system for this tree*/
} TREE_CONNECT_RESPONSE_DATA_non_castable;

uint32_t
MarshallTreeConnectResponseData(
    uint8_t         *pBuffer,
    uint32_t         bufferLen,
    uint8_t          messageAlignment,
    uint32_t        *pBufferUsed,
    const uchar8_t  *pszService,
    const wchar16_t *pwszNativeFileSystem
    )
{
    uint32_t error = 0;

    uint32_t bufferUsed = 0;
    uint32_t alignment = 0;
    uint32_t wstrlen = 0;

    /* Input strings are trusted */
    uint8_t *pCursor =
        (uint8_t*) lsmb_stpncpy((char*) pBuffer, (char*) pszService,
            bufferLen > bufferUsed ? bufferLen - bufferUsed : 0);
    if (!*pCursor && bufferLen > bufferUsed)
    {
        /* string fits */
        pCursor += sizeof(NUL);
        bufferUsed += pCursor - pBuffer - bufferUsed;
    }
    else
        /* expensive length check */
        bufferUsed += strlen((char *) pszService) + sizeof(NUL);

    /* Align string */
    alignment = (bufferUsed + messageAlignment) % 2;
    if (alignment)
    {
        *(pBuffer + bufferUsed) = 0;
        bufferUsed += alignment;
    }

    wstrlen = wc16oncpy((wchar16_t *) (pBuffer + bufferUsed),
        pwszNativeFileSystem,
        bufferLen > bufferUsed ? bufferLen - bufferUsed : 0);
    bufferUsed += wstrlen * sizeof(wchar16_t);

    *pBufferUsed = bufferUsed;

    if (bufferUsed > bufferLen)
    {
        error = EMSGSIZE;
    }

    return error;
}

uint32_t
UnmarshallTreeConnectResponse(
    const uint8_t    *pBuffer,
    uint32_t          bufferLen,
    uint8_t           messageAlignment,
    TREE_CONNECT_RESPONSE_HEADER **ppHeader,
    uchar8_t        **ppszService,
    wchar16_t       **ppwszNativeFilesystem
    )
{
    /* NOTE: The buffer format cannot be trusted! */
    uint32_t bufferUsed = sizeof(TREE_CONNECT_RESPONSE_HEADER);
    if (bufferLen < bufferUsed)
        return EBADMSG;

    /* @todo: endian swap as appropriate */
    *ppHeader = (TREE_CONNECT_RESPONSE_HEADER*) pBuffer;

    *ppszService = (uchar8_t *) pBuffer + bufferUsed;
    bufferUsed = strnlen((char *) *ppszService, bufferLen - bufferUsed) +
        sizeof(NUL);
    if (bufferUsed > bufferLen)
    {
        return EBADMSG;
    }

    /* Align string */
    bufferUsed += (bufferUsed + messageAlignment) % 2;
    if (bufferUsed > bufferLen)
    {
        return EBADMSG;
    }

    *ppwszNativeFilesystem = (wchar16_t *) (pBuffer + bufferUsed);
    bufferUsed += sizeof(wchar16_t) * wc16snlen(*ppwszNativeFilesystem,
        (bufferLen - bufferUsed) / sizeof(wchar16_t)) + sizeof(WNUL);
    if (bufferUsed > bufferLen)
    {
        return EBADMSG;
    }

    return 0;
}

