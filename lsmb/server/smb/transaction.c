
#include "includes.h"

typedef struct
{
    uint8_t   FOR_REFERENCE_ONLY;

    uint16_t  setup[0];         /* Setup words (# = setupWordCount) */
    uint16_t  byteCount;        /* Count of data bytes */
    wchar16_t name[0];          /* Must be NULL */
    uint8_t   pad[0];           /* Pad to SHORT or LONG */
    uint8_t   parameters[0];    /* Param. bytes (# = parameterCount) */
    uint8_t   pad1[0];          /* Pad to SHORT or LONG */
    uint8_t   data[0];          /* Data bytes (# = DataCount) */
} TRANSACTION_REQUEST_DATA_non_castable;

static uint32_t
_MarshallTransactionParameterData(
    uint8_t  *pBuffer,
    uint32_t  bufferLen,
    uint32_t *pBufferUsed,
    uint8_t  *pParameters,
    uint32_t  parameterLen,
    uint16_t *pParameterOffset,
    uint8_t  *pData,
    uint32_t  dataLen,
    uint16_t *pDataOffset)
{
    uint32_t bufferUsed = 0;
    uint32_t alignment = 0;
    uint32_t error = 0;

#if 0 /* This code is useless as written (alignment always evals to 0).
         Disabling to work around build failure with newer gcc and -Werror */

    /* Align data to a four byte boundary */
    alignment = (4 - (bufferUsed % 4)) % 4;
    memset(pBuffer + bufferUsed, 0, alignment);
    bufferUsed += alignment;
#endif

    if (parameterLen && bufferUsed + parameterLen <= bufferLen)
        memcpy(pBuffer + bufferUsed, pParameters, parameterLen);
    *pParameterOffset = bufferUsed;
    bufferUsed += parameterLen;

    /* Align data to a four byte boundary */
    alignment = (4 - (bufferUsed % 4)) % 4;
    memset(pBuffer + bufferUsed, 0, alignment);
    bufferUsed += alignment;

    if (dataLen && bufferUsed + dataLen <= bufferLen)
        memcpy(pBuffer + bufferUsed, pData, dataLen);
    *pDataOffset = bufferUsed;
    bufferUsed += dataLen;

    if (bufferUsed > bufferLen)
    {
        error = EMSGSIZE;
    }

    *pBufferUsed = bufferUsed;

    return error;
}

static uint32_t
_MarshallTransactionSetupData(
    uint8_t   *pBuffer,
    uint32_t   bufferLen,
    uint32_t  *pBufferUsed,
    uint16_t  *pSetup,
    uint8_t    setupLen,
    wchar16_t *pwszName,
    uint8_t   *pParameters,
    uint32_t   parameterLen,
    uint16_t  *pParameterOffset,
    uint8_t   *pData,
    uint32_t   dataLen,
    uint16_t  *pDataOffset
    )
{
    uint32_t  error = 0;
    uint32_t  bufferUsed = 0;
    uint32_t  bufferUsedData = 0;
    uint32_t  alignment = 0;
    uint32_t  wstrlen = 0;
    uint16_t *pByteCount = NULL;

    if (setupLen && bufferUsed + setupLen <= bufferLen)
        memcpy(pBuffer, pSetup, setupLen);
    bufferUsed += setupLen;

    /* byteCount */
    pByteCount = (uint16_t *) (pBuffer + bufferUsed);
    bufferUsed += sizeof(uint16_t);

    if (pwszName)
    {
        /* Align string */
        alignment = (bufferUsed + 1) % 2;
        if (alignment)
        {
            *(pBuffer + bufferUsed) = 0;
            bufferUsed += alignment;
        }
        wstrlen = wc16oncpy((wchar16_t *) (pBuffer + bufferUsed), pwszName,
            bufferLen > bufferUsed ? bufferLen - bufferUsed : 0);
        bufferUsed += wstrlen * sizeof(*pByteCount);
    }

    error = _MarshallTransactionParameterData(pBuffer + bufferUsed,
        bufferLen > bufferUsed ? bufferLen - bufferUsed : 0, &bufferUsedData,
        pParameters, parameterLen, pParameterOffset, pData, dataLen,
        pDataOffset);
    if (error && error != EMSGSIZE)
    {
        return error;
    }
    *pParameterOffset += bufferUsed;
    *pDataOffset += bufferUsed;
    bufferUsed += bufferUsedData;

    if (bufferUsed > bufferLen)
    {
        error = EMSGSIZE;
    }
    else
    {
        /* Fill in the byte count */
        *pByteCount = (uint16_t) (pBuffer + bufferUsed -
            ((uint8_t *) pByteCount) - sizeof(*pByteCount));
    }
    *pBufferUsed = bufferUsed;

    return error;
}

uint32_t
MarshallTransactionRequestData(
    uint8_t   *pBuffer,
    uint32_t   bufferLen,
    uint32_t  *pBufferUsed,
    uint16_t  *pSetup,
    uint8_t    setupLen,
    wchar16_t *pwszName,
    uint8_t   *pParameters,
    uint32_t   parameterLen,
    uint16_t  *pParameterOffset,
    uint8_t   *pData,
    uint32_t   dataLen,
    uint16_t  *pDataOffset
    )
{
    return _MarshallTransactionSetupData(pBuffer, bufferLen, pBufferUsed,
        pSetup, setupLen, pwszName, pParameters, parameterLen, pParameterOffset,
        pData, dataLen, pDataOffset);
}

static uint32_t
_UnmarshallTransactionParameterData(
    uint8_t   *pBuffer,
    uint32_t   bufferLen,
    uint8_t  **ppParameters,
    uint32_t   parameterLen,
    uint8_t  **ppData,
    uint32_t   dataLen)
{
    uint32_t bufferUsed = 0;

    bufferUsed += (4 - (bufferUsed % 4)) % 4;
    if (bufferUsed + parameterLen > bufferLen)
    {
        return EBADMSG;
    }

    if (parameterLen == 0)
    {
        *ppParameters = NULL;   /* Zero length parameters */
    }
    else
    {
        *ppParameters = (uint8_t*) pBuffer + bufferUsed;
    }
    bufferUsed += parameterLen;

    /* Align data to a four byte boundary */
    bufferUsed += (4 - (bufferUsed % 4)) % 4;
    if (bufferUsed + dataLen > bufferLen)
    {
        return EBADMSG;
    }

    if (dataLen == 0)
    {
        *ppData = NULL;         /* Zero length parameters */
    }
    else
    {
        *ppData = (uint8_t*) pBuffer + bufferUsed;
    }

    return 0;
}

static uint32_t
_UnmarshallTransactionSetupData(
    uint8_t    *pBuffer,
    uint32_t    bufferLen,
    uint16_t  **ppSetup,
    uint8_t     setupLen,
    uint16_t  **ppByteCount,
    wchar16_t **ppwszName,
    uint8_t   **ppParameters,
    uint32_t    parameterLen,
    uint8_t   **ppData,
    uint32_t    dataLen
    )
{
    uint32_t bufferUsed = 0;

    if (setupLen > bufferLen)
        return EBADMSG;

    if (setupLen == 0)
    {
        *ppSetup = NULL;        /* Zero length setup */
    }
    else
    {
        *ppSetup = (uint16_t*) pBuffer;
    }
    bufferUsed += setupLen;

    if (bufferUsed + sizeof(**ppByteCount) > bufferLen)
        return EBADMSG;
    *ppByteCount = (uint16_t*) (pBuffer + bufferUsed);
    bufferUsed += sizeof(**ppByteCount);

    if (ppwszName)
    {
        /* Align string */
        bufferUsed += bufferUsed % 2;
        if (bufferUsed > bufferLen)
        {
            return EBADMSG;
        }

        *ppwszName = (wchar16_t *) (pBuffer + bufferUsed);
        bufferUsed += sizeof(wchar16_t) * wc16snlen(*ppwszName,
            (bufferLen - bufferUsed) / sizeof(wchar16_t)) + sizeof(WNUL);
        if (bufferUsed > bufferLen)
        {
            return EBADMSG;
        }
    }

    return _UnmarshallTransactionParameterData(pBuffer, bufferLen,
        ppParameters, parameterLen, ppData, dataLen);
}

uint32_t
UnmarshallTransactionRequest(
    uint8_t    *pBuffer,
    uint32_t    bufferLen,
    TRANSACTION_REQUEST_HEADER **ppHeader,
    uint16_t  **ppSetup,
    uint8_t     setupLen,
    uint16_t  **ppByteCount,
    wchar16_t **ppwszName,
    uint8_t   **ppParameters,
    uint32_t    parameterLen,
    uint8_t   **ppData,
    uint32_t    dataLen
    )
{
    /* NOTE: The buffer format cannot be trusted! */
    uint32_t bufferUsed = sizeof(TRANSACTION_REQUEST_HEADER);
    if (bufferLen < bufferUsed)
        return EBADMSG;

    /* @todo: endian swap as appropriate */
    *ppHeader = (TRANSACTION_REQUEST_HEADER *) pBuffer;

    return _UnmarshallTransactionSetupData(pBuffer + bufferUsed,
        bufferLen - bufferUsed, ppSetup, setupLen, ppByteCount, ppwszName,
        ppParameters, parameterLen, ppData, dataLen);
}

typedef struct
{
    uint8_t   FOR_REFERENCE_ONLY;

    uint8_t   pad[0];            /* Pad to SHORT or LONG */
    uint8_t   parameters[0];     /* Param. bytes (# = parameterCount) */
    uint8_t   pad1[0];           /* Pad to SHORT or LONG */
    uint8_t   data[0];           /* Data bytes (# = DataCount) */
} TRANSACTION_SECONDARY_REQUEST_DATA_non_castable;

uint32_t
MarshallTransactionSecondaryRequestData(
    uint8_t  *pBuffer,
    uint32_t  bufferLen,
    uint32_t *pBufferUsed,
    uint8_t  *pParameters,
    uint32_t  parameterLen,
    uint16_t *pParameterOffset,
    uint8_t  *pData,
    uint32_t  dataLen,
    uint16_t *pDataOffset
    )
{
    return _MarshallTransactionParameterData(pBuffer, bufferLen, pBufferUsed,
        pParameters, parameterLen, pParameterOffset, pData, dataLen,
        pDataOffset);
}

uint32_t
UnmarshallTransactionSecondaryRequest(
    uint8_t   *pBuffer,
    uint32_t   bufferLen,
    TRANSACTION_SECONDARY_REQUEST_HEADER **ppHeader,
    uint8_t  **ppParameters,
    uint32_t   parameterLen,
    uint8_t  **ppData,
    uint32_t   dataLen
    )
{
    /* NOTE: The buffer format cannot be trusted! */
    uint32_t bufferUsed = sizeof(TRANSACTION_SECONDARY_REQUEST_HEADER);
    if (bufferLen < bufferUsed)
        return EBADMSG;

    /* @todo: endian swap as appropriate */
    *ppHeader = (TRANSACTION_SECONDARY_REQUEST_HEADER *) pBuffer;

    return _UnmarshallTransactionParameterData(pBuffer + bufferUsed,
        bufferLen - bufferUsed, ppParameters, parameterLen, ppData, dataLen);
}

/* This is identical to TRANSACTION_SECONDARY_REQUEST_DATA */
typedef struct
{
    uint8_t   FOR_REFERENCE_ONLY;

    uint16_t  setup[0];         /* Setup words (# = SetupWordCount) */
    uint16_t  byteCount;        /* Count of data bytes */
    uint8_t   pad[0];           /* Pad to SHORT or LONG */
    uint8_t   parameters[0];    /* Parameter bytes (# = ParameterCount) */
    uint8_t   pad1[0];          /* Pad to SHORT or LONG */
    uint8_t   data[0];          /* Data bytes (# = DataCount) */
} TRANSACTION_SECONDARY_RESPONSE_DATA_non_castable;

uint32_t
MarshallTransactionSecondaryResponseData(
    uint8_t  *pBuffer,
    uint32_t  bufferLen,
    uint32_t *pBufferUsed,
    uint16_t *pSetup,
    uint8_t   setupLen,
    uint8_t  *pParameters,
    uint32_t  parameterLen,
    uint16_t *pParameterOffset,
    uint8_t  *pData,
    uint32_t  dataLen,
    uint16_t *pDataOffset
    )
{
    return _MarshallTransactionSetupData(pBuffer, bufferLen, pBufferUsed,
        pSetup, setupLen, NULL, pParameters, parameterLen,
        pParameterOffset, pData, dataLen, pDataOffset);
}

uint32_t
UnmarshallTransactionSecondaryResponse(
    uint8_t   *pBuffer,
    uint32_t   bufferLen,
    TRANSACTION_SECONDARY_RESPONSE_HEADER **ppHeader,
    uint16_t **ppSetup,
    uint8_t    setupLen,
    uint16_t **ppByteCount,
    uint8_t  **ppParameters,
    uint32_t   parameterLen,
    uint8_t  **ppData,
    uint32_t   dataLen
    )
{
    /* NOTE: The buffer format cannot be trusted! */
    uint32_t bufferUsed = sizeof(TRANSACTION_SECONDARY_RESPONSE_HEADER);
    if (bufferLen < bufferUsed)
        return EBADMSG;

    /* @todo: endian swap as appropriate */
    *ppHeader = (TRANSACTION_SECONDARY_RESPONSE_HEADER *) pBuffer;

    return _UnmarshallTransactionSetupData(pBuffer + bufferUsed,
        bufferLen - bufferUsed, ppSetup, setupLen, ppByteCount, NULL,
        ppParameters, parameterLen, ppData, dataLen);
}
