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
 *        marshal_session.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 * 
 *        Marshal/Unmarshal API for User Logon Session Messages
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *
 */
#include "ipc.h"

DWORD
LsaMarshalSession(
    PCSTR  pszLoginId,
    PSTR   pszBuffer,
    PDWORD pdwBufLen
    )
{
    DWORD dwError = 0;
    DWORD dwRequiredLen = 0;
    LSASESSIONHEADER header;
    DWORD dwOffset = 0;
    
    dwRequiredLen = LsaGetSessionBufferLength(pszLoginId);
    
    if (!pszBuffer) {
        *pdwBufLen = dwRequiredLen;
        goto cleanup;
    }
    
    if (*pdwBufLen < dwRequiredLen) {
        dwError = LSA_ERROR_INSUFFICIENT_BUFFER;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    memset(&header, 0, sizeof(header));
    
    dwOffset = sizeof(header);
    if (!IsNullOrEmptyString(pszLoginId)) {
        header.login.offset = dwOffset;
        header.login.length = strlen(pszLoginId);
        memcpy(pszBuffer+header.login.offset, pszLoginId, header.login.length);
    }
    memcpy(pszBuffer, &header, sizeof(header));
    
cleanup:

    return dwError;
    
error:

    goto cleanup;
}

DWORD
LsaGetSessionBufferLength(
    PCSTR  pszLoginId
    )
{
    DWORD dwLen = sizeof(LSASESSIONHEADER);
    if (!IsNullOrEmptyString(pszLoginId)) {
        dwLen += strlen(pszLoginId);
    }
    return dwLen;
}

DWORD
LsaUnmarshalSession(
    PCSTR pszMsgBuf,
    DWORD dwMsgLen,
    PSTR* ppszLoginId
    )
{
    DWORD dwError = 0;
    LSASESSIONHEADER header;
    PSTR pszLoginId = NULL;
    
    if (dwMsgLen < sizeof(LSASESSIONHEADER)) {
        dwError = LSA_ERROR_INVALID_MESSAGE;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    memset(&header, 0, sizeof(header));
    
    memcpy(&header, pszMsgBuf, sizeof(header));
    if (header.login.length) {
        dwError = LsaStrndup(
                        pszMsgBuf+header.login.offset,
                        header.login.length,
                        &pszLoginId);
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    *ppszLoginId = pszLoginId;

cleanup:

    return dwError;
    
error:
    
    *ppszLoginId = NULL;
    
    LSA_SAFE_FREE_STRING(pszLoginId);
    
    goto cleanup;
}
