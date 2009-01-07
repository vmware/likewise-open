#include "includes.h"

uint32_t
MarshallWriteRequestData(
    uint8_t         *pBuffer,
    uint32_t         bufferLen,
    uint8_t          messageAlignment,
    uint32_t        *pBufferUsed,
    uint16_t        *pDataOffset,
    uint8_t         *pWriteBuffer,
    uint16_t        wWriteLen
    )
{
    uint32_t error = 0;

    uint32_t bufferUsed = 0;
    uint32_t alignment = 0;
    uint32_t dataOffset = 0;

    alignment = (bufferUsed + messageAlignment) % 2;
    if (alignment)
    {
        *(pBuffer + bufferUsed) = 0;
        bufferUsed += alignment;
    }

    dataOffset = bufferUsed;

    memcpy(pBuffer + bufferUsed, pWriteBuffer, wWriteLen);

    bufferUsed += wWriteLen;

    if (bufferUsed > bufferLen)
    {
        error = EMSGSIZE;
        goto error;
    }

    *pBufferUsed = bufferUsed;
    *pDataOffset = (uint16_t)dataOffset;

cleanup:

    return error;

error:

    *pBufferUsed = 0;

    goto cleanup;
}

