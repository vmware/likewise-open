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
 *        marshal_error.c
 *
 * Abstract:
 *
 *        Likewise Site Manager
 *
 *        Marshal/Unmarshal API for Error Messages
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *
 */
#include "includes.h"

DWORD
LWNetMarshalError(
    DWORD errorCode,
    PCSTR pszErrorMessage,
    PSTR  pszBuf,
    PDWORD pdwBufLen
    )
{
    DWORD dwError = 0;
    DWORD dwOffset = 0;
    LWNETERRORRECORDHEADER errHeader;
    
    dwError = LWNetGetErrorBufferLength(errorCode, pszErrorMessage, pdwBufLen);
    BAIL_ON_LWNET_ERROR(dwError);
    
    if (pszBuf) {
        dwOffset += sizeof(errHeader);
        memset(&errHeader, 0, sizeof(errHeader));
        errHeader.errorCode = errorCode;
        errHeader.message.offset = dwOffset;
        if (!IsNullOrEmptyString(pszErrorMessage)) {
            errHeader.message.length = strlen(pszErrorMessage);
            memcpy(pszBuf+dwOffset, pszErrorMessage, errHeader.message.length);
        }
        memcpy(pszBuf, &errHeader, sizeof(errHeader));
    }

    return dwError;
    
error:

    *pdwBufLen = 0;
    
    return dwError;
}

DWORD
LWNetUnmarshalError(
    PCSTR  pszMsgBuf,
    DWORD  dwMsgLen,
    PDWORD pdwError,
    PSTR*  ppszError
    )
{
    DWORD dwError = 0;
    LWNETERRORRECORDHEADER errHeader;
    PSTR pszError = NULL;
    
    // TODO : Check incoming message length
    
    memcpy(&errHeader, pszMsgBuf, sizeof(errHeader));
    
    *pdwError = errHeader.errorCode;
    if (errHeader.message.length) {
        dwError = LWNetAllocateMemory(errHeader.message.length+1, (PVOID*)&pszError);
        BAIL_ON_LWNET_ERROR(dwError);
        
        memcpy(pszError, pszMsgBuf+errHeader.message.offset, errHeader.message.length);
    }
    
    *ppszError = pszError;
    
cleanup:

    return dwError;
    
error:

    LWNET_SAFE_FREE_STRING(pszError);
    
    *ppszError = NULL;
    
    goto cleanup;
}

DWORD
LWNetGetErrorBufferLength(
    DWORD dwError,
    PCSTR pszErrorMessage,
    PDWORD pdwBufLen
    )
{
    DWORD dwBufLen = sizeof(LWNETERRORRECORDHEADER);
    
    if (!IsNullOrEmptyString(pszErrorMessage)) {
        dwBufLen += strlen(pszErrorMessage) + 1;
    }
    
    *pdwBufLen = dwBufLen;
    
    return 0;
}
