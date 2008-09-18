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
 *        marshal_records.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Marshal/Unmarshal API for User/Group Enumeration Messages
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *
 */
#include "ipc.h"

DWORD
LsaMarshalBeginEnumRecordsQuery(
    DWORD  dwInfoLevel,
    DWORD  dwNumMaxRecords,
    PSTR   pszBuffer,
    PDWORD pdwBufLen
    )
{
    DWORD dwError = 0;
    LSA_BEGIN_ENUM_RECORDS_HEADER header = {0};
    DWORD dwRequiredLength = sizeof(header);
    
    if (!pszBuffer) {
        *pdwBufLen = dwRequiredLength;
        goto cleanup;
    }
    
    if (*pdwBufLen < dwRequiredLength) {
        dwError = LSA_ERROR_INSUFFICIENT_BUFFER;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    header.dwInfoLevel = dwInfoLevel;
    header.dwNumMaxRecords = dwNumMaxRecords;
    
    memcpy(pszBuffer, &header, sizeof(header));
    
cleanup:

    return dwError;
    
error:

    goto cleanup;
}

DWORD
LsaUnmarshalBeginEnumRecordsQuery(
    PCSTR  pszMsgBuf,
    DWORD  dwMsgLen,
    PDWORD pdwInfoLevel,
    PDWORD pdwNumMaxRecords
    )
{
    DWORD dwError = 0;
    LSA_BEGIN_ENUM_RECORDS_HEADER header = {0};
    
    if (dwMsgLen < sizeof(header)) {
        dwError = LSA_ERROR_INVALID_MESSAGE;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    memcpy(&header, pszMsgBuf, sizeof(header));
    
    *pdwInfoLevel = header.dwInfoLevel;
    *pdwNumMaxRecords = header.dwNumMaxRecords;
    
cleanup:

    return dwError;
    
error:

    goto cleanup;
}

DWORD
LsaMarshalEnumRecordsToken(
    PCSTR  pszGUID,
    PSTR   pszBuffer,
    PDWORD pdwBufLen
    )
{
    DWORD dwError = 0;
    DWORD dwOffset = 0;
    LSA_ENUM_RECORDS_TOKEN_HEADER header;
    DWORD dwRequiredBufLen = sizeof(header);
    
    memset(&header, 0, sizeof(header));
   
    if (!IsNullOrEmptyString(pszGUID)) {
        dwRequiredBufLen += strlen(pszGUID);
    }
    
    if (!pszBuffer) {
       *pdwBufLen = dwRequiredBufLen;
       goto cleanup;
    }
    
    if (*pdwBufLen < dwRequiredBufLen) {
        dwError = LSA_ERROR_INSUFFICIENT_BUFFER;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    dwOffset = sizeof(header);
    memset(&header, 0, sizeof(header));
    if (!IsNullOrEmptyString(pszGUID)) {
        header.token.offset = dwOffset;
        header.token.length = strlen(pszGUID);
    }
    
    memcpy(pszBuffer, &header, sizeof(header));
    if (!IsNullOrEmptyString(pszGUID)) {
       memcpy(pszBuffer+header.token.offset, pszGUID, header.token.length);
    }
    
cleanup:

    return dwError;
    
error:

    goto cleanup;
}

DWORD
LsaUnmarshalEnumRecordsToken(
    PCSTR  pszMsgBuf,
    DWORD  dwMsgLen,
    PSTR*  ppszGUID
    )
{
    DWORD dwError = 0;
    LSA_ENUM_RECORDS_TOKEN_HEADER header;
    PSTR pszGUID = NULL;
    
    memset(&header, 0, sizeof(header));
    
    if (dwMsgLen < sizeof(header)) {
        dwError = LSA_ERROR_INVALID_MESSAGE;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    memcpy(&header, pszMsgBuf, sizeof(header));
    if (header.token.length) {
        dwError = LsaStrndup(
                        pszMsgBuf+header.token.offset,
                        header.token.length,
                        &pszGUID);
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    *ppszGUID = pszGUID;
    
cleanup:

    return dwError;
    
error:

    *ppszGUID = NULL;
    
    LSA_SAFE_FREE_STRING(pszGUID);

    goto cleanup;
}
