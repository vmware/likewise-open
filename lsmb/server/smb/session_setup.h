
uint32_t
UnmarshallSessionSetupRequest(
    const uint8_t   *pBuffer,
    uint32_t         bufferLen,
    uint8_t          messageAlignment,
    SESSION_SETUP_REQUEST_HEADER **ppHeader,
    uint8_t        **ppSecurityBlob,
    wchar16_t      **ppwszNativeOS,
    wchar16_t      **ppwszNativeLanMan,
    wchar16_t      **ppwszNativeDomain
    );

uint32_t
MarshallSessionSetupResponseData(
    uint8_t         *pBuffer,
    uint32_t         bufferLen,
    uint8_t          messageAlignment,
    uint32_t        *pBufferUsed,
    const uint8_t   *pSecurityBlob,
    uint16_t         blobLen,
    const wchar16_t *pwszNativeOS,
    const wchar16_t *pwszNativeLanMan,
    const wchar16_t *pwszNativeDomain
    );

typedef struct
{
    /* wordCount and byteCount are handled at a higher layer */
    /* AndX chains will be handled at a higher layer */

    uint16_t  maxBufferSize;   /* Client's maximum buffer size */
    uint16_t  maxMpxCount;     /* Actual maximum multiplexed pending
                                * requests */
    uint16_t  vcNumber;        /* 0 = first (only), nonzero=additional VC
                                * number */
    uint32_t  sessionKey;      /* Session key (valid iff VcNumber != 0) */
    uint16_t  ciPasswordLen;   /* Case insensitive password length, ASCII */
    uint16_t  csPasswordLen;   /* Case sensitive password length, Unicode */
    uint32_t  reserved;        /* Must be 0 */
    uint32_t  capabilities;    /* Client capabilities */
    uint16_t  byteCount;       /* Count of data bytes; min = 0 */

    /* Data immediately follows */
}  __attribute__((__packed__))  SESSION_SETUP_REQUEST_HEADER_NO_EXT;

