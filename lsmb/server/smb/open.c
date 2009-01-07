/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
    uint32_t error = 0;

    uint32_t bufferUsed = 0;
    uint32_t alignment = 0;
    uint32_t wstrlen = 0;

    /* Align strings */
    alignment = (bufferUsed + messageAlignment) % 2;
    if (alignment)
    {
        *(pBuffer + bufferUsed) = 0;
        bufferUsed += alignment;
    }

    wstrlen = wc16oncpy((wchar16_t *) (pBuffer + bufferUsed), pwszPath,
        bufferLen > bufferUsed ? bufferLen - bufferUsed : 0);
    bufferUsed += wstrlen * sizeof(wchar16_t);

    if (bufferUsed > bufferLen)
    {
        error = EMSGSIZE;
    }

    *pBufferUsed = bufferUsed;

    return error;
}

uint32_t
UnmarshallSMBResponseCreate(
    const uint8_t  *pBuffer,
    uint32_t        bufferLen,
    CREATE_RESPONSE_HEADER **ppHeader
    )
{
    /* NOTE: The buffer format cannot be trusted! */
    uint32_t bufferUsed = sizeof(CREATE_RESPONSE_HEADER);
    if (bufferLen < bufferUsed)
        return EBADMSG;

    /* @todo: endian swap as appropriate */
    *ppHeader = (CREATE_RESPONSE_HEADER*) pBuffer;

    return 0;
}
