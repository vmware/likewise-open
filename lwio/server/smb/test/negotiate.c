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
 *        negotiate.c
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem (LWIO)
 *
 *        SMB NEGOTIATE "wire" API
 *
 * Author: Kaya Bekiroglu (kaya@likewisesoftware.com)
 *
 * @todo: support big endian architectures
 * @todo: add error logging code
 * @todo: switch to NT error codes where appropriate
 */

#include <config.h>
#include <lsmbsys.h>

#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <wchar16.h>

#include <lwio/lwio.h>

#include "negotiate.h"

typedef struct
{
    /* wordCount and byteCount are handled at a higher layer */

    uint8_t bufferFormat;       /* 0x02 -- Dialect */
    uchar8_t szDialectName[];    /* ASCII null-terminated string */
}  __attribute__((__packed__))  NEGOTIATE_REQUEST_DIALECT;

#ifdef API

/**
 * Marshall an SMB NEGOTIATE request
 *
 * @param pszDialects
 *                An array of zero terminated ASCII dialect strings
 * @param dialectCount
 *                Count of dialect strings
 * @param pBuffer Marshall buffer
 * @param bufferLen
 *                Size of marshall buffer
 * @param pBufferUsed
 *                On success, the amount of the marshall buffer used.  On
 *                EMSGSIZE, the total amount of marshall buffer needed
 *
 * @return Non-zero error code on error
 */
uint32_t
MarshallNegotiateRequest(
    uint8_t        *pBuffer,
    uint32_t        bufferLen,
    uint32_t       *pBufferUsed,
    const uchar8_t *pszDialects[],
    uint32_t        dialectCount
    )
{
    uint32_t error = 0;

    NEGOTIATE_REQUEST_DIALECT *pDialect = (NEGOTIATE_REQUEST_DIALECT*) pBuffer;
    uint32_t bufferUsed = 0;

    uint32_t i = 0;

    /* Input strings are trusted */
    for (i = 0; i < dialectCount; i++)
    {
        uint32_t len = sizeof(NEGOTIATE_REQUEST_DIALECT);

        if (bufferUsed + len <= bufferLen)
            // No endianness (single byte):
            pDialect->bufferFormat = 0x02;
        bufferUsed += len;

        if (bufferUsed + sizeof(NUL) <= bufferLen)
        {
            uint8_t *pCursor =
                (uint8_t*) stpncpy((char*) pDialect->szDialectName,
                    (const char *) pszDialects[i], bufferLen - bufferUsed);
            if (!*pCursor)
            {
                /* string fits */
                pCursor += sizeof(NUL);
                len = pCursor - pBuffer - bufferUsed;
                pDialect = (NEGOTIATE_REQUEST_DIALECT*) pCursor;
            }
            else
            {
                /* expensive length check */
                len = strlen((const char*) pszDialects[i]) + sizeof(NUL);
            }
        }
        else
        {
            /* expensive length check */
            len = strlen((const char *) pszDialects[i]) + sizeof(NUL);
        }

        bufferUsed += len;
    }

    if (bufferUsed > bufferLen)
    {
        error = EMSGSIZE;
    }

    *pBufferUsed = bufferUsed;

    return error;
}

uint32_t
UnmarshallNegotiateRequest(
    const uint8_t   *pBuffer,
    uint32_t         bufferLen,        /* From caller's byteCount */
    uchar8_t        *pszDialects[],
    uint32_t        *pDialectCount
    )
{
    uint32_t error = 0;

    /* NOTE: The buffer format cannot be trusted! */
    NEGOTIATE_REQUEST_DIALECT *pDialect = (NEGOTIATE_REQUEST_DIALECT*) pBuffer;
    uint32_t bufferLeft = bufferLen;

    uint32_t i = 0;

    while ((uint8_t *) pDialect < pBuffer + bufferLen)
    {
        uint32_t len = strnlen((const char *) pDialect->szDialectName,
            bufferLeft) + sizeof(NEGOTIATE_REQUEST_DIALECT) + sizeof(NUL);

        /* If the last string was (sneakily) not null terminated, bail! */
        if (len > bufferLeft)
            return EBADMSG;

        if (i < *pDialectCount)
            pszDialects[i] = (uchar8_t *) pDialect->szDialectName;

        pDialect = (NEGOTIATE_REQUEST_DIALECT*) ((uint8_t *) pDialect + len);
        bufferLeft -= len;
        i++;
    }

    if (i > *pDialectCount)
    {
        error = EMSGSIZE;
    }

    *pDialectCount = i;

    return error;
}

typedef struct
{
    /* byteCount is handled at a higher layer */

    /* Non-CAP_EXTENDED_SECURITY fields are not implemented */
    /* @todo: decide if CAP_EXTENDED_SECURITY will be mandatory */

    uint8_t guid[16];           /* A globally unique identifier assigned to the
                                 * server; Present only when
                                 * CAP_EXTENDED_SECURITY is on in Capabilities
                                 * field */
    uint8_t securityBlob[];      /* Opaque Security Blob associated with the
                                 * security package if CAP_EXTENDED_SECURITY
                                 * is on in the Capabilities field; else
                                 * challenge for CIFS challenge/response
                                 * authentication */
}  __attribute__((__packed__))  NEGOTIATE_RESPONSE_DATA;

uint32_t
MarshallNegotiateResponseData(
    uint8_t  *pBuffer,
    uint32_t  bufferLen,
    uint32_t *pBufferUsed,
    const uint8_t *pGUID,
    const uint8_t *pSecurityBlob,
    uint32_t  blobLen
    )
{
    uint32_t error = 0;

    NEGOTIATE_RESPONSE_DATA *pData = (NEGOTIATE_RESPONSE_DATA*) pBuffer;
    uint32_t bufferUsed = 0;
    uint32_t len = sizeof(pData->guid) + blobLen;

    if (bufferUsed + len <= bufferLen)
    {
        memcpy(&(pData->guid), pGUID, sizeof(pData->guid));
        if (blobLen)
            memcpy(&(pData->securityBlob), pSecurityBlob, blobLen);
    }
    bufferUsed += len;

    if (bufferUsed > bufferLen)
    {
        error = EMSGSIZE;
    }

    *pBufferUsed = bufferUsed;

    return error;
}

uint32_t
UnmarshallNegotiateResponse(
    const uint8_t  *pBuffer,
    uint32_t        bufferLen,
    NEGOTIATE_RESPONSE_HEADER **ppHeader,
    uint8_t       **ppGUID,
    uint8_t       **ppSecurityBlob,
    uint32_t       *pBlobLen
    )
{
    NEGOTIATE_RESPONSE_DATA *pData = NULL;

    /* NOTE: The buffer format cannot be trusted! */
    uint32_t bufferUsed = sizeof(NEGOTIATE_RESPONSE_HEADER);
    if (bufferLen < bufferUsed)
        return EBADMSG;

    /* @todo: endian swap as appropriate */
    *ppHeader = (NEGOTIATE_RESPONSE_HEADER*) pBuffer;
    pData = (NEGOTIATE_RESPONSE_DATA*) (pBuffer + bufferUsed);
    bufferUsed += sizeof(pData->guid);

    if (bufferLen < bufferUsed)
        return EBADMSG;

    *pBlobLen = bufferLen - bufferUsed;

    if (*pBlobLen == 0)           /* zero length blob */
        *ppSecurityBlob = NULL;
    else
        *ppSecurityBlob = (uint8_t *) &(pData->securityBlob);

    return 0;
}

#endif /* API */

#ifdef TEST

#include "../../test/jenny_test.h"

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>

#include <moonunit/interface.h>

MU_TEST(lsmb_wire_negotiate, request_round_trip)
{
    uint8_t pBuffer[1024],
            pBuffer2[1024];
    char *ppszDialects[] = { "One", "Two", "Three", "Four", "Five", "Six" };
    size_t dialectCount = sizeof(ppszDialects) / sizeof(ppszDialects[0]);
    ssize_t i = 0;

    /*
     * The last two iterations check test sensitivity by randomly permuting
     * one bit.  These tests will fail if the bit is part of a pad.
     */
    for (; i < 4; i++)
    {
        uint32_t  bufferLen = 0,
                  buffer2Len = 0;
        uchar8_t *pszUnmarshalledDialects[dialectCount];
        uint32_t  unmarshalledDialectCount = 0;
        uint32_t  error = 0;

        if (i >= 2)
        {
            /* Ideally we'd fail on a crash and timeout but not generic
               failure */
            MU_EXPECT(MU_STATUS_ASSERTION);
        }

        memset(pBuffer, i % 2 ? 0x00 : 0xFF, sizeof(pBuffer));
        memset(pBuffer2, (i + 1) % 2 ? 0x00 : 0xFF, sizeof(pBuffer2));

        /* Test buffer sizing */
        error = MarshallNegotiateRequest(pBuffer, 0, &bufferLen,
            (const uchar8_t **) ppszDialects, dialectCount);
        MU_ASSERT(error == EMSGSIZE);

        error = MarshallNegotiateRequest(pBuffer, sizeof(pBuffer), &bufferLen,
            (const uchar8_t **) ppszDialects, dialectCount);
        MU_ASSERT(!error);

        if (i >= 2)
        {
            pBuffer[rand() % bufferLen] ^= 1 << rand() % 8;
        }

        /* Test buffer sizing */
        error = UnmarshallNegotiateRequest(pBuffer, bufferLen,
            pszUnmarshalledDialects, &unmarshalledDialectCount);
        MU_ASSERT(error == EMSGSIZE);
        MU_ASSERT(unmarshalledDialectCount == dialectCount);

        error = UnmarshallNegotiateRequest(pBuffer, bufferLen,
            pszUnmarshalledDialects, &unmarshalledDialectCount);
        MU_ASSERT(!error);

        /* Test buffer sizing */
        error = MarshallNegotiateRequest(pBuffer2, 0, &buffer2Len,
            (const uchar8_t **) pszUnmarshalledDialects,
            unmarshalledDialectCount); MU_ASSERT(error == EMSGSIZE);
        MU_ASSERT(bufferLen == buffer2Len);

        error = MarshallNegotiateRequest(pBuffer2, sizeof(pBuffer2),
            &buffer2Len, (const uchar8_t **) pszUnmarshalledDialects,
            unmarshalledDialectCount);
        MU_ASSERT(!error);
        MU_ASSERT(!memcmp(pBuffer, pBuffer2, bufferLen));
    }
}

MU_TEST(lsmb_wire_negotiate, response_round_trip)
{
    uint8_t pBuffer[1024],
            pBuffer2[1024];
    char *ppszDialects[] = { "One", "Two", "Three", "Four", "Five", "Six" };
    size_t dialectCount = sizeof(ppszDialects) / sizeof(ppszDialects[0]);
    ssize_t i = 0;

    /*
     * The last two iterations check test sensitivity by randomly permuting
     * one bit.  These tests will fail if the bit is part of a pad.
     */
    for (; i < 4; i++)
    {
        uint32_t  bufferLen = 0,
                  buffer2Len = 0;
        uchar8_t *pszUnmarshalledDialects[dialectCount];
        uint32_t  unmarshalledDialectCount = 0;
        uint32_t  error = 0;

        if (i >= 2)
        {
            /* Ideally we'd fail on a crash and timeout but not generic
               failure */
            MU_EXPECT(MU_STATUS_ASSERTION);
        }

        memset(pBuffer, i % 2 ? 0x00 : 0xFF, sizeof(pBuffer));
        memset(pBuffer2, (i + 1) % 2 ? 0x00 : 0xFF, sizeof(pBuffer2));

        /* Test buffer sizing */
        error = MarshallNegotiateRequest(pBuffer, 0, &bufferLen,
            (const uchar8_t **) ppszDialects, dialectCount);
        MU_ASSERT(error == EMSGSIZE);

        error = MarshallNegotiateRequest(pBuffer, sizeof(pBuffer), &bufferLen,
            (const uchar8_t **) ppszDialects, dialectCount);
        MU_ASSERT(!error);

        if (i >= 2)
        {
            pBuffer[rand() % bufferLen] ^= 1 << rand() % 8;
        }

        /* Test buffer sizing */
        error = UnmarshallNegotiateRequest(pBuffer, bufferLen,
            pszUnmarshalledDialects, &unmarshalledDialectCount);
        MU_ASSERT(error == EMSGSIZE);
        MU_ASSERT(unmarshalledDialectCount == dialectCount);

        error = UnmarshallNegotiateRequest(pBuffer, bufferLen,
            pszUnmarshalledDialects, &unmarshalledDialectCount);
        MU_ASSERT(!error);

        /* Test buffer sizing */
        error = MarshallNegotiateRequest(pBuffer2, 0, &buffer2Len,
            (const uchar8_t **) pszUnmarshalledDialects,
            unmarshalledDialectCount); MU_ASSERT(error == EMSGSIZE);
        MU_ASSERT(bufferLen == buffer2Len);

        error = MarshallNegotiateRequest(pBuffer2, sizeof(pBuffer2),
            &buffer2Len, (const uchar8_t **) pszUnmarshalledDialects,
            unmarshalledDialectCount);
        MU_ASSERT(!error);
        MU_ASSERT(!memcmp(pBuffer, pBuffer2, bufferLen));
    }
}

#if 0
MU_TEST(lsmb_wire_negotiate, request_round_trip_covering_array)
{
    uchar8_t *jennyParms = malloc(1024*1024);
    char *jennyExclusions = NULL;
    uchar8_t *jennyCursor = jennyParms;
    size_t i = 0;

    uint8_t *pBlock = malloc(1024*1024);
    uint8_t *pBlock1 = malloc(1024*1024);
    uint8_t *pBlock2 = malloc(1024*1024);
    uint8_t *pBlock3 = malloc(1024*1024);

    /*
    uint32_t
    MarshallNegotiateRequest(
        uint8_t        *pBuffer,
        uint32_t        bufferLen,
        uint32_t       *pBufferUsed,
        const uchar8_t *pszDialects[],
        uint32_t        dialectCount )
    */

    uint32_t *jennyInput = malloc(1024*1024);
    size_t inputLen = 0;

    /* pBuffer */
    add_aligned_pointer_u8(jennyInput, &inputLen);

    /* Final buffer size */
    add_length_u32(jennyInput, &inputLen);
    add_length_adjustment_u32(jennyInput, &inputLen);

    /* Hardcoded at 3 dialects */
    add_string_8(jennyInput, &inputLen, &jennyExclusions);
    add_string_8(jennyInput, &inputLen, &jennyExclusions);
    add_string_8(jennyInput, &inputLen, &jennyExclusions);

    jennyCursor += sprintf((char*) jennyCursor, "-n3 ");

    for (i = 0; i < inputLen; i++)
    {
        jennyCursor += sprintf((char*) jennyCursor, "%d ", jennyInput[i]);
    }
    if (jennyExclusions)
    {
        jennyCursor += sprintf((char*) jennyCursor, "%s ", jennyExclusions);
    }

    char *jennyCmd;
    int error = asprintf(&jennyCmd, "common/test/jenny %s", jennyParms);
    if (error)
    {

    }
    printf(jennyCmd);
    FILE* jennyOutputFile = popen((char*) jennyCmd, "r");

    char scanChar;
    uint32_t dummy;
    while (EOF != fscanf(jennyOutputFile, " %d%c ", &dummy, &scanChar))
    {
        uint8_t *pBuffer = pBlock + get_alignment_pointer_u8(scanChar);
        uint32_t bufferLen = 0;
        int32_t  bufferAdjust = 0;
        uint32_t bufferUsed = 0;
        uint32_t dialectCount = 3;
        const uchar8_t *pszDialects[3];

        fscanf(jennyOutputFile, " %d%c ", &dummy, &scanChar);
        bufferLen = get_length_u32(scanChar);
        fscanf(jennyOutputFile, " %d%c ", &dummy, &scanChar);
        bufferAdjust = get_length_adjustment_i32(scanChar);

        pszDialects[0] = get_string_8(jennyOutputFile, pBlock1);
        pszDialects[1] = get_string_8(jennyOutputFile, pBlock2);
        pszDialects[2] = get_string_8(jennyOutputFile, pBlock3);

        printf(".");
        MarshallNegotiateRequest(pBuffer, bufferLen, &bufferUsed,
            pszDialects, dialectCount);
    }
    pclose(jennyOutputFile);
}
#endif

#if 0
MU_TEST(lsmb_wire_negotiate, response_round_trip_covering_array)
{
    MU_ASSERT(0);
}
#endif

#endif /* TEST */
