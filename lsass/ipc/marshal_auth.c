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
 *        marshal_auth.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Marshal/Unmarshal API for Authentication Messages
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *
 */
#include "ipc.h"

DWORD
LsaMarshalCredentials(
    PCSTR  pszLoginName,
    PCSTR  pszPassword,
    PCSTR  pszOldPassword,
    PSTR   pszBuffer,
    PDWORD pdwBufLen
    )
{
    DWORD dwError = 0;
    DWORD dwOffset = 0;
    DWORD dwRequiredBufferLength = 0;
    LSACREDENTIALHEADER respHdr;
    
    dwError = LsaGetCredentialBufferLength(
                    pszLoginName,
                    pszPassword,
                    pszOldPassword,
                    &dwRequiredBufferLength);
    BAIL_ON_LSA_ERROR(dwError);
        
    if (!pszBuffer) {
        *pdwBufLen = dwRequiredBufferLength;
        goto cleanup;
    }
    
    if (*pdwBufLen < dwRequiredBufferLength)
    {
        dwError = LSA_ERROR_INSUFFICIENT_BUFFER;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    dwOffset += sizeof(respHdr);
    // Prepare and write the header
    memset(&respHdr, 0, sizeof(respHdr));
    if (!IsNullOrEmptyString(pszLoginName)) {
       respHdr.login.length = strlen(pszLoginName);
       respHdr.login.offset = dwOffset;
       memcpy(pszBuffer+dwOffset, pszLoginName, respHdr.login.length);
       dwOffset += respHdr.login.length+1;
    }
    
    if (!IsNullOrEmptyString(pszPassword)) {
       respHdr.passwd.length = strlen(pszPassword);
       respHdr.passwd.offset = dwOffset;
       memcpy(pszBuffer+dwOffset, pszPassword, respHdr.passwd.length);
       dwOffset += respHdr.passwd.length+1;
    }
    
    if (!IsNullOrEmptyString(pszOldPassword)) {
       respHdr.old_passwd.length = strlen(pszOldPassword);
       respHdr.old_passwd.offset = dwOffset;
       memcpy(pszBuffer+dwOffset, pszOldPassword, respHdr.old_passwd.length);
       dwOffset += respHdr.old_passwd.length+1;
    }
    
    memcpy(pszBuffer, &respHdr, sizeof(respHdr));
    
cleanup:

    return dwError;
    
error:

    *pdwBufLen = 0;
    goto cleanup;
}

DWORD
LsaUnmarshalCredentials(
    PCSTR pszMsgBuf,
    DWORD dwMsgLen,
    PSTR* ppszLoginName,
    PSTR* ppszPassword,
    PSTR* ppszOldPassword
    )
{
    DWORD dwError = 0;
    LSACREDENTIALHEADER respHeader;
    PSTR pszLoginName = NULL;
    PSTR pszPassword = NULL;
    PSTR pszOldPassword = NULL;
    
    memcpy(&respHeader, pszMsgBuf, sizeof(respHeader));
    
    if (respHeader.login.length) {
       dwError = LsaAllocateMemory(respHeader.login.length+1,
                                   (PVOID*)&pszLoginName);
       BAIL_ON_LSA_ERROR(dwError);
       
       memcpy(pszLoginName, pszMsgBuf+respHeader.login.offset, respHeader.login.length);
    }
    
    if (respHeader.passwd.length) {
       dwError = LsaAllocateMemory(respHeader.passwd.length+1,
                                   (PVOID*)&pszPassword);
       BAIL_ON_LSA_ERROR(dwError);
       memcpy(pszPassword, pszMsgBuf+respHeader.passwd.offset, respHeader.passwd.length);
    }
    
    if (respHeader.old_passwd.length) {
       dwError = LsaAllocateMemory(respHeader.old_passwd.length+1,
                                   (PVOID*)&pszOldPassword);
       BAIL_ON_LSA_ERROR(dwError);
       memcpy(pszOldPassword, pszMsgBuf+respHeader.old_passwd.offset, respHeader.old_passwd.length);
    }
    
    *ppszLoginName = pszLoginName;
    *ppszPassword = pszPassword;
    if (ppszOldPassword) {
       *ppszOldPassword = pszOldPassword;
       pszOldPassword = NULL;
    }
    
cleanup:

    LSA_SAFE_CLEAR_FREE_STRING(pszOldPassword);

    return dwError;
    
error:

    LSA_SAFE_FREE_STRING(pszLoginName);
    LSA_SAFE_CLEAR_FREE_STRING(pszPassword);
    
    *ppszLoginName = NULL;
    *ppszPassword = NULL;
    *ppszOldPassword = NULL;
    
    goto cleanup;
}

DWORD
LsaGetCredentialBufferLength(
    PCSTR  pszLoginName,
    PCSTR  pszPassword,
    PCSTR  pszOldPassword,
    PDWORD pdwBufLen
    )
{
    DWORD dwError = 0;
    DWORD dwBufLen = 0;

    dwBufLen += sizeof(LSACREDENTIALHEADER);
    
    if (!IsNullOrEmptyString(pszLoginName)) {
        dwBufLen += strlen(pszLoginName) + 1;
    }
    
    if (!IsNullOrEmptyString(pszPassword)) {
        dwBufLen += strlen(pszPassword) + 1;
    }
    
    if (!IsNullOrEmptyString(pszOldPassword)) {
        dwBufLen += strlen(pszOldPassword) + 1;
    }
    
    *pdwBufLen = dwBufLen;
    
    return dwError;
}
