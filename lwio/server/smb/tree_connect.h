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

