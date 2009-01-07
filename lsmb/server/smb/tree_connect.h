#ifndef __TREECONNECT_H__
#define __TREECONNECT_H__

#ifdef TEST
#include <moonunit/interface.h>
#endif /* TEST */

uint32_t
UnmarshallTreeConnectRequest(
    const uint8_t *pBuffer,
    uint32_t       bufferLen,
    uint8_t        messageAlignment,
    TREE_CONNECT_REQUEST_HEADER **ppHeader,
    uint8_t      **ppPassword,
    wchar16_t    **ppwszPath,
    uchar8_t     **ppszService
    );

typedef struct
{
    /* wordCount and byteCount are handled at a higher layer */
    /* AndX chains will be handled at a higher layer */

    uint16_t  optionalSupport;  /* Optional support bits */
                                /*      SMB_SUPPORT_SEARCH_BITS = 0x0001 */
                                /* Exclusive search bits */
                                /*      ("MUST HAVE BITS") supported */
                                /*      SMB_SHARE_IS_IN_DFS = 0x0002 */
    uint16_t  passwordLength;   /* Length of password */
    uint16_t  byteCount;        /* Count of data bytes; min = 3 */

    /* Data immediately follows */
}  __attribute__((__packed__))  TREE_CONNECT_RESPONSE_HEADER;

uint32_t
MarshallTreeConnectResponseData(
    uint8_t         *pBuffer,
    uint32_t         bufferLen,
    uint8_t          messageAlignment,
    uint32_t        *pBufferUsed,
    const uchar8_t  *pszService,
    const wchar16_t *pwszNativeFileSystem
    );

uint32_t
UnmarshallTreeConnectResponse(
    const uint8_t    *pBuffer,
    uint32_t          bufferLen,
    uint8_t           messageAlignment,
    TREE_CONNECT_RESPONSE_HEADER **ppHeader,
    uchar8_t        **ppszService,
    wchar16_t       **ppwszNativeFileSystem
    );

#endif

