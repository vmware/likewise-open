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
 *        marshal_user.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Marshal/Unmarshal API for Messages related to Users
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *
 */
#include "ipc.h"

DWORD
LsaMarshalUserInfoList(
    PVOID* ppUserInfoList,
    DWORD  dwInfoLevel,
    DWORD  dwNumUsers,
    PSTR   pszBuffer,
    PDWORD pdwBufLen
    )
{
    DWORD dwError = 0;
    DWORD dwRequiredBufLen = 0;
    DWORD dwBytesWritten = 0;
    LSA_USER_GROUP_RECORD_PREAMBLE header;

    dwError = LsaComputeUserBufferSize(
                    ppUserInfoList,
                    dwInfoLevel,
                    dwNumUsers,
                    &dwRequiredBufLen
                    );
    BAIL_ON_LSA_ERROR(dwError);
    
    if (!pszBuffer) {
        *pdwBufLen = dwRequiredBufLen;
        goto cleanup;
    }
    
    if (*pdwBufLen < dwRequiredBufLen) {
        dwError = LSA_ERROR_INSUFFICIENT_BUFFER;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    memset(&header, 0, sizeof(header));
    header.dwInfoLevel = dwInfoLevel;
    header.dwNumRecords = dwNumUsers;
    
    memcpy(pszBuffer, &header, sizeof(header));
    dwBytesWritten = sizeof(header);
    
    switch (dwInfoLevel)
    {
        case 0:
        {
            DWORD dwTmpBytesWritten = 0;
            
            dwError = LsaMarshalUser_0_InfoList(
                            ppUserInfoList,
                            dwNumUsers,
                            dwBytesWritten,
                            pszBuffer,
                            &dwTmpBytesWritten
                            );
            BAIL_ON_LSA_ERROR(dwError);
            
            dwBytesWritten += dwTmpBytesWritten;
            
            break;
        }
        case 1:
        {
            DWORD dwTmpBytesWritten = 0;
            
            dwError = LsaMarshalUser_1_InfoList(
                            ppUserInfoList,
                            dwNumUsers,
                            dwBytesWritten,
                            pszBuffer,
                            &dwTmpBytesWritten
                            );
            BAIL_ON_LSA_ERROR(dwError);
            
            dwBytesWritten += dwTmpBytesWritten;
            
            break;
        }
        case 2:
        {
            DWORD dwTmpBytesWritten = 0;
            
            dwError = LsaMarshalUser_2_InfoList(
                            ppUserInfoList,
                            dwNumUsers,
                            dwBytesWritten,
                            pszBuffer,
                            &dwTmpBytesWritten
                            );
            BAIL_ON_LSA_ERROR(dwError);
            
            dwBytesWritten += dwTmpBytesWritten;
            
            break;
        }
        default:
        {
            // We would have caught this when computing the
            // size. Adding this here as well for completeness
            dwError = LSA_ERROR_UNSUPPORTED_USER_LEVEL;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }
    
cleanup:

    return dwError;
    
error:

    goto cleanup;
}

DWORD
LsaComputeUserBufferSize(
    PVOID* ppUserInfoList,
    DWORD  dwInfoLevel,
    DWORD  dwNumUsers,
    PDWORD pdwBufLen
    )
{
    DWORD dwError = 0;
    DWORD dwTotalBufLen = sizeof(LSA_USER_GROUP_RECORD_PREAMBLE);
    
    if (dwNumUsers) {
        
        switch(dwInfoLevel)
        {
            case 0:
            {
                dwTotalBufLen += LsaComputeBufferSize_User0(
                                    ppUserInfoList,
                                    dwNumUsers
                                    );
                break;
            }
            case 1:
            {
                dwTotalBufLen += LsaComputeBufferSize_User1(
                                    ppUserInfoList,
                                    dwNumUsers
                                    );
                            break;
            }
            case 2:
            {
                dwTotalBufLen += LsaComputeBufferSize_User2(
                                    ppUserInfoList,
                                    dwNumUsers
                                    );
                            break;
            }
            default:
            {
                dwError = LSA_ERROR_UNSUPPORTED_USER_LEVEL;
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
    }
    
    *pdwBufLen = dwTotalBufLen;
    
cleanup:

    return dwError;
    
error:

     *pdwBufLen = 0;

     goto cleanup;
}

DWORD
LsaComputeBufferSize_User0(
    PVOID* ppUserInfoList,
    DWORD  dwNumUsers
    )
{
    DWORD dwBufLen = 0;
    DWORD iUser = 0;
    PLSA_USER_INFO_0 pUserInfo = NULL;
    
    for (iUser = 0; iUser < dwNumUsers; iUser++)
    {
        pUserInfo = (PLSA_USER_INFO_0)*(ppUserInfoList+iUser);
        dwBufLen += sizeof(LSA_USER_0_RECORD_HEADER);
        
        if (!IsNullOrEmptyString(pUserInfo->pszName)) {
            dwBufLen += strlen(pUserInfo->pszName);
        }
        if (!IsNullOrEmptyString(pUserInfo->pszPasswd)) {
            dwBufLen += strlen(pUserInfo->pszPasswd);
        }
        if (!IsNullOrEmptyString(pUserInfo->pszGecos)) {
            dwBufLen += strlen(pUserInfo->pszGecos);
        }
        if (!IsNullOrEmptyString(pUserInfo->pszShell)) {
            dwBufLen += strlen(pUserInfo->pszShell);
        }
        if (!IsNullOrEmptyString(pUserInfo->pszHomedir)) {
            dwBufLen += strlen(pUserInfo->pszHomedir);
        }
        if (!IsNullOrEmptyString(pUserInfo->pszSid)) {
            dwBufLen += strlen(pUserInfo->pszSid);
        }
    }
    
    return dwBufLen;
}

DWORD
LsaComputeBufferSize_User1(
    PVOID* ppUserInfoList,
    DWORD  dwNumUsers
    )
{
    DWORD dwBufLen = 0;
    DWORD iUser = 0;
    PLSA_USER_INFO_1 pUserInfo = NULL;
    
    for (iUser = 0; iUser < dwNumUsers; iUser++)
    {
        pUserInfo = (PLSA_USER_INFO_1)*(ppUserInfoList+iUser);
        dwBufLen += sizeof(LSA_USER_1_RECORD_HEADER);
        
        if (!IsNullOrEmptyString(pUserInfo->pszName)) {
            dwBufLen += strlen(pUserInfo->pszName);
        }
        if (!IsNullOrEmptyString(pUserInfo->pszPasswd)) {
            dwBufLen += strlen(pUserInfo->pszPasswd);
        }
        if (!IsNullOrEmptyString(pUserInfo->pszGecos)) {
            dwBufLen += strlen(pUserInfo->pszGecos);
        }
        if (!IsNullOrEmptyString(pUserInfo->pszShell)) {
            dwBufLen += strlen(pUserInfo->pszShell);
        }
        if (!IsNullOrEmptyString(pUserInfo->pszHomedir)) {
            dwBufLen += strlen(pUserInfo->pszHomedir);
        }
        if (!IsNullOrEmptyString(pUserInfo->pszSid)) {
            dwBufLen += strlen(pUserInfo->pszSid);
        }
        if (!IsNullOrEmptyString(pUserInfo->pszUPN)) {
            dwBufLen += strlen(pUserInfo->pszUPN);
        }
        // Make sure we send nulls for the LMHash and NTHash values.
    }
    
    return dwBufLen;
}

DWORD
LsaComputeBufferSize_User2(
    PVOID* ppUserInfoList,
    DWORD  dwNumUsers
    )
{
    DWORD dwBufLen = 0;
    DWORD iUser = 0;
    PLSA_USER_INFO_2 pUserInfo = NULL;
    
    for (iUser = 0; iUser < dwNumUsers; iUser++)
    {
        pUserInfo = (PLSA_USER_INFO_2)*(ppUserInfoList+iUser);
        dwBufLen += sizeof(LSA_USER_2_RECORD_HEADER);
        
        if (!IsNullOrEmptyString(pUserInfo->pszName)) {
            dwBufLen += strlen(pUserInfo->pszName);
        }        
        if (!IsNullOrEmptyString(pUserInfo->pszPasswd)) {
            dwBufLen += strlen(pUserInfo->pszPasswd);
        }
        if (!IsNullOrEmptyString(pUserInfo->pszGecos)) {
            dwBufLen += strlen(pUserInfo->pszGecos);
        }
        if (!IsNullOrEmptyString(pUserInfo->pszShell)) {
            dwBufLen += strlen(pUserInfo->pszShell);
        }
        if (!IsNullOrEmptyString(pUserInfo->pszHomedir)) {
            dwBufLen += strlen(pUserInfo->pszHomedir);
        }
        if (!IsNullOrEmptyString(pUserInfo->pszSid)) {
            dwBufLen += strlen(pUserInfo->pszSid);
        }
        if (!IsNullOrEmptyString(pUserInfo->pszUPN)) {
            dwBufLen += strlen(pUserInfo->pszUPN);
        }
        // Make sure we send nulls for the LMHash and NTHash values.
    }
    
    return dwBufLen;
}

DWORD
LsaMarshalUser_0_InfoList(
    PVOID* ppUserInfoList,
    DWORD  dwNumUsers,
    DWORD  dwBeginOffset,
    PSTR   pszBuffer,
    PDWORD pdwDataBytesWritten
    )
{
    DWORD dwError = 0;
    // This is where we are from the beginning of the buffer
    DWORD iUser = 0;
    // This is where we will start writing strings
    DWORD dwCurrentDataOffset = dwBeginOffset + (sizeof(LSA_USER_0_RECORD_HEADER) * dwNumUsers);
    PSTR  pszData = pszBuffer + dwCurrentDataOffset;
    DWORD dwTotalDataBytesWritten = 0;
    
    for (iUser = 0; iUser < dwNumUsers; iUser++)
    {
        DWORD dwDataBytesWritten = 0;
        PSTR  pszHeaderOffset = pszBuffer + dwBeginOffset + sizeof(LSA_USER_0_RECORD_HEADER) * iUser;
        PSTR  pszDataOffset = pszData + dwTotalDataBytesWritten;
        
        dwError = LsaMarshalUser_0(
                        (PLSA_USER_INFO_0)*(ppUserInfoList+iUser),
                        pszHeaderOffset,
                        pszDataOffset,
                        dwCurrentDataOffset,
                        &dwDataBytesWritten
                        );
        BAIL_ON_LSA_ERROR(dwError);
        dwTotalDataBytesWritten += dwDataBytesWritten;
        dwCurrentDataOffset += dwDataBytesWritten;
    }
    
    *pdwDataBytesWritten = dwTotalDataBytesWritten;
    
cleanup:

    return dwError;
    
error:

    *pdwDataBytesWritten = 0;

    goto cleanup;
}

DWORD
LsaMarshalUser_0(
    PLSA_USER_INFO_0 pUserInfo,
    PSTR   pszHeaderBuffer,
    PSTR   pszDataBuffer,
    DWORD  dwDataBeginOffset,
    PDWORD pdwDataBytesWritten
    )
{
    DWORD dwError = 0;
    // This is the offset from the beginning of the buffer
    DWORD dwGlobalOffset = dwDataBeginOffset;
    DWORD dwOffset = 0;
    LSA_USER_0_RECORD_HEADER header;
    DWORD dwDataBytesWritten = 0;
    
    // Prepare and write the header
    memset(&header, 0, sizeof(header));
    
    header.uid = pUserInfo->uid;
    header.gid = pUserInfo->gid;
 
    if (!IsNullOrEmptyString(pUserInfo->pszName)) {
       header.name.length = strlen(pUserInfo->pszName);
       header.name.offset = dwGlobalOffset;
       memcpy(pszDataBuffer+dwOffset, pUserInfo->pszName, header.name.length);
       dwOffset += header.name.length;
       dwGlobalOffset += header.name.length;
       dwDataBytesWritten += header.name.length;
    }
    
    if (!IsNullOrEmptyString(pUserInfo->pszPasswd)) {
       header.passwd.length = strlen(pUserInfo->pszPasswd);
       header.passwd.offset = dwGlobalOffset;
       memcpy(pszDataBuffer+dwOffset, pUserInfo->pszPasswd, header.passwd.length);
       dwOffset += header.passwd.length;
       dwGlobalOffset += header.passwd.length;
       dwDataBytesWritten += header.passwd.length;
    }
  
    if (!IsNullOrEmptyString(pUserInfo->pszGecos)) {
        header.gecos.length = strlen(pUserInfo->pszGecos);
        header.gecos.offset = dwGlobalOffset;
        memcpy(pszDataBuffer+dwOffset, pUserInfo->pszGecos, header.gecos.length);
        dwOffset += header.gecos.length;
        dwGlobalOffset += header.gecos.length;
        dwDataBytesWritten += header.gecos.length;
    }
    
    if (!IsNullOrEmptyString(pUserInfo->pszShell)) {
        header.shell.length = strlen(pUserInfo->pszShell);
        header.shell.offset = dwGlobalOffset;
        memcpy(pszDataBuffer+dwOffset, pUserInfo->pszShell, header.shell.length);
        dwOffset += header.shell.length;
        dwGlobalOffset += header.shell.length;
        dwDataBytesWritten += header.shell.length;
    }
    
    if (!IsNullOrEmptyString(pUserInfo->pszHomedir)) {
        header.homedir.length = strlen(pUserInfo->pszHomedir);
        header.homedir.offset = dwGlobalOffset;
        memcpy(pszDataBuffer+dwOffset, pUserInfo->pszHomedir, header.homedir.length);
        dwOffset += header.homedir.length;
        dwGlobalOffset += header.homedir.length;
        dwDataBytesWritten += header.homedir.length;
    }

    if (!IsNullOrEmptyString(pUserInfo->pszSid)) {
        header.sid.length = strlen(pUserInfo->pszSid);
        header.sid.offset = dwGlobalOffset;
        memcpy(pszDataBuffer+dwOffset, pUserInfo->pszSid, header.sid.length);
        dwOffset += header.sid.length;
        dwGlobalOffset += header.sid.length;
        dwDataBytesWritten += header.sid.length;
    }

    memcpy(pszHeaderBuffer, &header, sizeof(header));
    
    *pdwDataBytesWritten = dwDataBytesWritten;

    return dwError;
}

DWORD
LsaMarshalUser_1_InfoList(
    PVOID* ppUserInfoList,
    DWORD  dwNumUsers,
    DWORD  dwBeginOffset,
    PSTR   pszBuffer,
    PDWORD pdwDataBytesWritten
    )
{
    DWORD dwError = 0;
    // This is where we are from the beginning of the buffer
    DWORD iUser = 0;
    // This is where we will start writing strings
    DWORD dwCurrentDataOffset = dwBeginOffset + (sizeof(LSA_USER_1_RECORD_HEADER) * dwNumUsers);
    PSTR  pszData = pszBuffer + dwCurrentDataOffset;
    DWORD dwTotalDataBytesWritten = 0;
    
    for (iUser = 0; iUser < dwNumUsers; iUser++)
    {
        DWORD dwDataBytesWritten = 0;
        PSTR  pszHeaderOffset = pszBuffer + dwBeginOffset + sizeof(LSA_USER_1_RECORD_HEADER) * iUser;
        PSTR  pszDataOffset = pszData + dwTotalDataBytesWritten;
        
        dwError = LsaMarshalUser_1(
                        (PLSA_USER_INFO_1)*(ppUserInfoList+iUser),
                        pszHeaderOffset,
                        pszDataOffset,
                        dwCurrentDataOffset,
                        &dwDataBytesWritten
                        );
        BAIL_ON_LSA_ERROR(dwError);
        dwTotalDataBytesWritten += dwDataBytesWritten;
        dwCurrentDataOffset += dwDataBytesWritten;
    }
    
    *pdwDataBytesWritten = dwTotalDataBytesWritten;
    
cleanup:

    return dwError;
    
error:

    *pdwDataBytesWritten = 0;

    goto cleanup;
}

DWORD
LsaMarshalUser_1(
    PLSA_USER_INFO_1 pUserInfo,
    PSTR   pszHeaderBuffer,
    PSTR   pszDataBuffer,
    DWORD  dwDataBeginOffset,
    PDWORD pdwDataBytesWritten
    )
{
    DWORD dwError = 0;
    // This is the offset from the beginning of the buffer
    DWORD dwGlobalOffset = dwDataBeginOffset;
    DWORD dwOffset = 0;
    LSA_USER_1_RECORD_HEADER header;
    DWORD dwDataBytesWritten = 0;
    
    // Prepare and write the header
    memset(&header, 0, sizeof(header));

    dwError = LsaMarshalUser_0(
        &pUserInfo->info0,
        (PSTR)&header,
        pszDataBuffer,
        dwGlobalOffset,
        &dwDataBytesWritten);
    BAIL_ON_LSA_ERROR(dwError);
    dwGlobalOffset += dwDataBytesWritten;
    dwOffset += dwDataBytesWritten;
    
    header.bIsLocalUser = pUserInfo->bIsLocalUser;

    header.bIsGeneratedUPN = pUserInfo->bIsGeneratedUPN;
 
    if (!IsNullOrEmptyString(pUserInfo->pszUPN)) {
        header.upn.length = strlen(pUserInfo->pszUPN);
        header.upn.offset = dwGlobalOffset;
        memcpy(pszDataBuffer+dwOffset, pUserInfo->pszUPN, header.upn.length);
        dwOffset += header.upn.length;
        dwGlobalOffset += header.upn.length;
        dwDataBytesWritten += header.upn.length;
    }
    
    // Don't send out the OWF Values

    memcpy(pszHeaderBuffer, &header, sizeof(header));
    
    *pdwDataBytesWritten = dwDataBytesWritten;

error:
    return dwError;
}

DWORD
LsaMarshalUser_2_InfoList(
    PVOID* ppUserInfoList,
    DWORD  dwNumUsers,
    DWORD  dwBeginOffset,
    PSTR   pszBuffer,
    PDWORD pdwDataBytesWritten
    )
{
    DWORD dwError = 0;
    // This is where we are from the beginning of the buffer
    DWORD iUser = 0;
    // This is where we will start writing strings
    DWORD dwCurrentDataOffset = dwBeginOffset + (sizeof(LSA_USER_2_RECORD_HEADER) * dwNumUsers);
    PSTR  pszData = pszBuffer + dwCurrentDataOffset;
    DWORD dwTotalDataBytesWritten = 0;
    
    for (iUser = 0; iUser < dwNumUsers; iUser++)
    {
        DWORD dwDataBytesWritten = 0;
        PSTR  pszHeaderOffset = pszBuffer + dwBeginOffset + sizeof(LSA_USER_2_RECORD_HEADER) * iUser;
        PSTR  pszDataOffset = pszData + dwTotalDataBytesWritten;
        
        dwError = LsaMarshalUser_2(
                        (PLSA_USER_INFO_2)*(ppUserInfoList+iUser),
                        pszHeaderOffset,
                        pszDataOffset,
                        dwCurrentDataOffset,
                        &dwDataBytesWritten
                        );
        BAIL_ON_LSA_ERROR(dwError);
        dwTotalDataBytesWritten += dwDataBytesWritten;
        dwCurrentDataOffset += dwDataBytesWritten;
    }
    
    *pdwDataBytesWritten = dwTotalDataBytesWritten;
    
cleanup:

    return dwError;
    
error:

    *pdwDataBytesWritten = 0;

    goto cleanup;
}

DWORD
LsaMarshalUser_2(
    PLSA_USER_INFO_2 pUserInfo,
    PSTR   pszHeaderBuffer,
    PSTR   pszDataBuffer,
    DWORD  dwDataBeginOffset,
    PDWORD pdwDataBytesWritten
    )
{
    DWORD dwError = 0;
    LSA_USER_2_RECORD_HEADER header;
    DWORD dwDataBytesWritten = 0;
    
    // Prepare and write the header
    memset(&header, 0, sizeof(header));

    dwError = LsaMarshalUser_1(
        &pUserInfo->info1,
        (PSTR)&header,
        pszDataBuffer,
        dwDataBeginOffset,
        &dwDataBytesWritten);
    BAIL_ON_LSA_ERROR(dwError);
    
    header.dwAccountExpired = pUserInfo->bAccountExpired;
    header.dwPasswordExpired = pUserInfo->bPasswordExpired;
    header.dwPasswordNeverExpires = pUserInfo->bPasswordNeverExpires;
    header.dwPromptChangePassword = pUserInfo->bPromptPasswordChange;
    header.dwUserCanChangePassword = pUserInfo->bUserCanChangePassword;
    header.dwAccountLocked = pUserInfo->bAccountLocked;
    header.dwAccountDisabled = pUserInfo->bAccountDisabled;
    header.dwDaysToPasswordExpiry = pUserInfo->dwDaysToPasswordExpiry;
 
    memcpy(pszHeaderBuffer, &header, sizeof(header));
    
    *pdwDataBytesWritten = dwDataBytesWritten;

error:
    return dwError;
}

DWORD
LsaUnmarshalUserInfoList(
    PCSTR   pszMsgBuf,
    DWORD   dwMsgLen,
    PDWORD  pdwInfoLevel,
    PVOID** pppUserInfoList,
    PDWORD  pdwNumUsers
    )
{
    DWORD dwError = 0;
    PVOID* ppUserInfoList = NULL;
    LSA_USER_GROUP_RECORD_PREAMBLE header;
    DWORD dwOffset = 0;
    
    if (dwMsgLen < sizeof(header)) {
        dwError = LSA_ERROR_INVALID_MESSAGE;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    memcpy(&header, pszMsgBuf, sizeof(header));
    dwOffset += sizeof(header);
    
    if (!header.dwNumRecords) {
        *pppUserInfoList = NULL;
        goto cleanup;
    }
    
    switch(header.dwInfoLevel)
    {
        case 0:
        {
            dwError = LsaUnmarshalUser_0_InfoList(
                            pszMsgBuf,
                            pszMsgBuf+dwOffset,
                            header.dwNumRecords,
                            &ppUserInfoList
                            );
            BAIL_ON_LSA_ERROR(dwError);
            break;
        }
        case 1:
        {
            dwError = LsaUnmarshalUser_1_InfoList(
                             pszMsgBuf,
                             pszMsgBuf+dwOffset,
                             header.dwNumRecords,
                             &ppUserInfoList
                             );
            BAIL_ON_LSA_ERROR(dwError);
            break;
        }
        case 2:
        {
            dwError = LsaUnmarshalUser_2_InfoList(
                             pszMsgBuf,
                             pszMsgBuf+dwOffset,
                             header.dwNumRecords,
                             &ppUserInfoList
                             );
            BAIL_ON_LSA_ERROR(dwError);
            break;
        }
        default:
        {
            dwError = LSA_ERROR_UNSUPPORTED_USER_LEVEL;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }
    
    *pppUserInfoList = ppUserInfoList;
    *pdwNumUsers = header.dwNumRecords;
    
cleanup:

    return dwError;
    
error:

    *pppUserInfoList = NULL;
    *pdwNumUsers = 0;

    goto cleanup;
}

DWORD
LsaUnmarshalUser_0_InfoList(
    PCSTR   pszMsgBuf,
    PCSTR   pszHdrBuf,
    DWORD   dwNumUsers,
    PVOID** pppUserInfoList
    )
{
    DWORD dwError = 0;
    PLSA_USER_INFO_0* ppUserInfoList = NULL;
    PLSA_USER_INFO_0 pUserInfo = NULL;
    DWORD iUser = 0;
    DWORD dwUserInfoLevel = 0;
    
    dwError = LsaAllocateMemory(
                    sizeof(LSA_USER_INFO_0) * dwNumUsers,
                    (PVOID*)&ppUserInfoList);
    BAIL_ON_LSA_ERROR(dwError);
    
    for (iUser = 0; iUser < dwNumUsers; iUser++) {
        LSA_USER_0_RECORD_HEADER header;
        
        memcpy(&header, pszHdrBuf + (iUser * sizeof(LSA_USER_0_RECORD_HEADER)), sizeof(LSA_USER_0_RECORD_HEADER));
        
        dwError = LsaAllocateMemory(
                        sizeof(LSA_USER_INFO_0),
                        (PVOID*)&pUserInfo);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaUnmarshalUserInPlace_0(
                        pszMsgBuf,
                        pUserInfo,
                        &header
                        );
        BAIL_ON_LSA_ERROR(dwError);
        
        *(ppUserInfoList+iUser) = pUserInfo;
        pUserInfo = NULL;
    }
    
    *pppUserInfoList = (PVOID*)ppUserInfoList;
    
cleanup:

    return dwError;
    
error:

    *pppUserInfoList = NULL;
    
    if (pUserInfo) {
        LsaFreeUserInfo(dwUserInfoLevel, pUserInfo);
    }
    
    if (ppUserInfoList) {
        LsaFreeUserInfoList(dwUserInfoLevel, (PVOID*)ppUserInfoList, dwNumUsers);
    }

    goto cleanup;
}

DWORD
LsaUnmarshalUserInPlace_0(
    PCSTR pszMsgBuf,
    PLSA_USER_INFO_0 pUserInfo,
    PLSA_USER_0_RECORD_HEADER pUserInfoHeader
    )
{
    DWORD dwError = 0;
    
    pUserInfo->uid = pUserInfoHeader->uid;
    pUserInfo->gid = pUserInfoHeader->gid;
    
    if (pUserInfoHeader->name.length) {
        dwError = LsaStrndup(
                        pszMsgBuf+pUserInfoHeader->name.offset,
                        pUserInfoHeader->name.length,
                        &pUserInfo->pszName);
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    if (pUserInfoHeader->passwd.length) {
        dwError = LsaStrndup(
                        pszMsgBuf+pUserInfoHeader->passwd.offset,
                        pUserInfoHeader->passwd.length,
                        &pUserInfo->pszPasswd);
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    if (pUserInfoHeader->gecos.length) {
        dwError = LsaStrndup(
                        pszMsgBuf+pUserInfoHeader->gecos.offset,
                        pUserInfoHeader->gecos.length,
                        &pUserInfo->pszGecos);
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    if (pUserInfoHeader->shell.length) {
        dwError = LsaStrndup(
                        pszMsgBuf+pUserInfoHeader->shell.offset,
                        pUserInfoHeader->shell.length,
                        &pUserInfo->pszShell);
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    if (pUserInfoHeader->homedir.length) {
        dwError = LsaStrndup(
                        pszMsgBuf+pUserInfoHeader->homedir.offset,
                        pUserInfoHeader->homedir.length,
                        &pUserInfo->pszHomedir);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserInfoHeader->sid.length) {
        dwError = LsaStrndup(
                        pszMsgBuf+pUserInfoHeader->sid.offset,
                        pUserInfoHeader->sid.length,
                        &pUserInfo->pszSid);
        BAIL_ON_LSA_ERROR(dwError);
    }
    
error:
    return dwError;
}

DWORD
LsaUnmarshalUser_1_InfoList(
    PCSTR   pszMsgBuf,
    PCSTR   pszHdrBuf,
    DWORD   dwNumUsers,
    PVOID** pppUserInfoList
    )
{
    DWORD dwError = 0;
    PLSA_USER_INFO_1* ppUserInfoList = NULL;
    PLSA_USER_INFO_1 pUserInfo = NULL;
    DWORD iUser = 0;
    DWORD dwUserInfoLevel = 1;
    
    dwError = LsaAllocateMemory(
                    sizeof(LSA_USER_INFO_1) * dwNumUsers,
                    (PVOID*)&ppUserInfoList);
    BAIL_ON_LSA_ERROR(dwError);
    
    for (iUser = 0; iUser < dwNumUsers; iUser++) {
        LSA_USER_1_RECORD_HEADER header;
        
        memcpy(&header, pszHdrBuf + (iUser * sizeof(LSA_USER_1_RECORD_HEADER)), sizeof(LSA_USER_1_RECORD_HEADER));

        dwError = LsaAllocateMemory(
                        sizeof(LSA_USER_INFO_1),
                        (PVOID*)&pUserInfo);
        BAIL_ON_LSA_ERROR(dwError);
        
        dwError = LsaUnmarshalUserInPlace_1(
                        pszMsgBuf,
                        &header,
                        pUserInfo
                        );
        BAIL_ON_LSA_ERROR(dwError);
        
        *(ppUserInfoList+iUser) = pUserInfo;
        pUserInfo = NULL;
    }
    
    *pppUserInfoList = (PVOID*)ppUserInfoList;
    
cleanup:

    return dwError;
    
error:

    *pppUserInfoList = NULL;
    
    if (pUserInfo) {
        LsaFreeUserInfo(dwUserInfoLevel, pUserInfo);
    }
    
    if (ppUserInfoList) {
        LsaFreeUserInfoList(dwUserInfoLevel, (PVOID*)ppUserInfoList, dwNumUsers);
    }

    goto cleanup;
}

DWORD
LsaUnmarshalUserInPlace_1(
    PCSTR pszMsgBuf,
    PLSA_USER_1_RECORD_HEADER pUserInfoHeader,
    PLSA_USER_INFO_1 pUserInfo
    )
{
    DWORD dwError = 0;
    
    dwError = LsaUnmarshalUserInPlace_0(
        pszMsgBuf,
        &pUserInfo->info0,
        &pUserInfoHeader->record0);
    BAIL_ON_LSA_ERROR(dwError);

    pUserInfo->bIsLocalUser = pUserInfoHeader->bIsLocalUser;

    pUserInfo->bIsGeneratedUPN = pUserInfoHeader->bIsGeneratedUPN;
    
    if (pUserInfoHeader->upn.length) {
        dwError = LsaStrndup(
                        pszMsgBuf+pUserInfoHeader->upn.offset,
                        pUserInfoHeader->upn.length,
                        &pUserInfo->pszUPN);
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    // Don't get the OWF values
    
error:
    return dwError;
}

DWORD
LsaUnmarshalUser_2_InfoList(
    PCSTR   pszMsgBuf,
    PCSTR   pszHdrBuf,
    DWORD   dwNumUsers,
    PVOID** pppUserInfoList
    )
{
    DWORD dwError = 0;
    PLSA_USER_INFO_2* ppUserInfoList = NULL;
    PLSA_USER_INFO_2 pUserInfo = NULL;
    DWORD iUser = 0;
    DWORD dwUserInfoLevel = 2;
    
    dwError = LsaAllocateMemory(
                    sizeof(LSA_USER_INFO_2) * dwNumUsers,
                    (PVOID*)&ppUserInfoList);
    BAIL_ON_LSA_ERROR(dwError);
    
    for (iUser = 0; iUser < dwNumUsers; iUser++) {
        LSA_USER_2_RECORD_HEADER header;
        
        memcpy(&header, pszHdrBuf + (iUser * sizeof(LSA_USER_2_RECORD_HEADER)), sizeof(LSA_USER_2_RECORD_HEADER));
        
        dwError = LsaAllocateMemory(
                        sizeof(LSA_USER_INFO_2),
                        (PVOID*)&pUserInfo);
        BAIL_ON_LSA_ERROR(dwError);
        
        dwError = LsaUnmarshalUserInPlace_2(
                        pszMsgBuf,
                        &header,
                        pUserInfo
                        );
        BAIL_ON_LSA_ERROR(dwError);
        
        *(ppUserInfoList+iUser) = pUserInfo;
        pUserInfo = NULL;
    }
    
    *pppUserInfoList = (PVOID*)ppUserInfoList;
    
cleanup:

    return dwError;
    
error:

    *pppUserInfoList = NULL;
    
    if (pUserInfo) {
        LsaFreeUserInfo(dwUserInfoLevel, pUserInfo);
    }
    
    if (ppUserInfoList) {
        LsaFreeUserInfoList(dwUserInfoLevel, (PVOID*)ppUserInfoList, dwNumUsers);
    }

    goto cleanup;
}

DWORD
LsaUnmarshalUserInPlace_2(
    PCSTR pszMsgBuf,
    PLSA_USER_2_RECORD_HEADER pUserInfoHeader,
    PLSA_USER_INFO_2 pUserInfo
    )
{
    DWORD dwError = 0;
    
    dwError = LsaUnmarshalUserInPlace_1(
        pszMsgBuf,
        &pUserInfoHeader->record1,
        &pUserInfo->info1);
    BAIL_ON_LSA_ERROR(dwError);

    pUserInfo->bAccountDisabled = pUserInfoHeader->dwAccountDisabled;
    pUserInfo->bAccountExpired = pUserInfoHeader->dwAccountExpired;
    pUserInfo->bPasswordExpired = pUserInfoHeader->dwPasswordExpired;
    pUserInfo->bPasswordNeverExpires = pUserInfoHeader->dwPasswordNeverExpires;
    pUserInfo->bPromptPasswordChange = pUserInfoHeader->dwPromptChangePassword;
    pUserInfo->bUserCanChangePassword = pUserInfoHeader->dwUserCanChangePassword;
    pUserInfo->bAccountExpired = pUserInfoHeader->dwAccountExpired;
    pUserInfo->dwDaysToPasswordExpiry = pUserInfoHeader->dwDaysToPasswordExpiry;
    
error:
    return dwError;
}

DWORD
LsaMarshalFindUserByNameQuery(
    PCSTR  pszLoginId,
    DWORD  dwInfoLevel,
    PSTR   pszBuffer,
    PDWORD pdwBufLen
    )
{
    DWORD dwError = 0;
    LSA_QUERY_RECORD_BY_NAME_HEADER header = {0};
    DWORD dwRequiredBufLen = 0;
    
    dwRequiredBufLen = LsaComputeBufferSize_FindUserByNameQuery(
                            pszLoginId,
                            dwInfoLevel
                            );
    if (!pszBuffer) {
        *pdwBufLen = dwRequiredBufLen;
        goto cleanup;
    }
    
    if (*pdwBufLen < dwRequiredBufLen) {
        dwError = LSA_ERROR_INSUFFICIENT_BUFFER;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    header.dwInfoLevel = dwInfoLevel;
    
    if (!IsNullOrEmptyString(pszLoginId)) {
       header.name.offset = sizeof(header);
       header.name.length = strlen(pszLoginId);
       memcpy(pszBuffer+header.name.offset, pszLoginId, header.name.length);
    }
    
    memcpy(pszBuffer, &header, sizeof(header));
    
cleanup:

    return dwError;
    
error:

    goto cleanup;
}

DWORD
LsaComputeBufferSize_FindUserByNameQuery(
    PCSTR pszLoginId,
    DWORD dwInfoLevel
    )
{
    return sizeof(LSA_QUERY_RECORD_BY_NAME_HEADER) + (IsNullOrEmptyString(pszLoginId) ? 0 : strlen(pszLoginId));
}

DWORD
LsaUnmarshalFindUserByNameQuery(
    PCSTR pszMsgBuf,
    DWORD dwMsgLen,
    PSTR* ppszLoginId,
    PDWORD pdwInfoLevel
    )
{
    DWORD dwError = 0;
    PSTR pszLoginId = NULL;
    LSA_QUERY_RECORD_BY_NAME_HEADER header = {0};
    
    if (dwMsgLen < sizeof(header)) {
        dwError = LSA_ERROR_INVALID_MESSAGE;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    memcpy(&header, pszMsgBuf, sizeof(header));
    
    if (header.name.length) {
        dwError = LsaStrndup(pszMsgBuf+header.name.offset, header.name.length, &pszLoginId);
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    *ppszLoginId = pszLoginId;
    *pdwInfoLevel = header.dwInfoLevel;
    
cleanup:

    return dwError;
    
error:

    *ppszLoginId = NULL;
    *pdwInfoLevel = 0;
    
    LSA_SAFE_FREE_STRING(pszLoginId);
    
    goto cleanup;
}

DWORD
LsaMarshalFindUserByIdQuery(
    uid_t  uid,
    DWORD  dwInfoLevel,
    PSTR   pszBuffer,
    PDWORD pdwBufLen
    )
{
    DWORD dwError = 0;
    LSA_QUERY_RECORD_BY_ID_HEADER header = {0};
    DWORD dwRequiredBufLen = 0;
    
    dwRequiredBufLen = LsaComputeBufferSize_FindUserByIdQuery(
                            uid,
                            dwInfoLevel
                            );
    if (!pszBuffer) {
        *pdwBufLen = dwRequiredBufLen;
        goto cleanup;
    }
    
    if (*pdwBufLen < dwRequiredBufLen) {
        dwError = LSA_ERROR_INSUFFICIENT_BUFFER;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    header.dwInfoLevel = dwInfoLevel;
    header.id = uid;
    
    memcpy(pszBuffer, &header, sizeof(header));
    
cleanup:

    return dwError;
    
error:

    goto cleanup;    
}

DWORD
LsaComputeBufferSize_FindUserByIdQuery(
    uid_t uid,
    DWORD dwInfoLevel
    )
{
    return sizeof(LSA_QUERY_RECORD_BY_ID_HEADER);
}

DWORD
LsaUnmarshalFindUserByIdQuery(
    PCSTR  pszMsgBuf,
    DWORD  dwMsgLen,
    uid_t* pUid,
    PDWORD pdwInfoLevel
    )
{
    DWORD dwError = 0;
    LSA_QUERY_RECORD_BY_ID_HEADER header = {0};
    
    if (dwMsgLen < sizeof(header)) {
        dwError = LSA_ERROR_INVALID_MESSAGE;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    memcpy(&header, pszMsgBuf, sizeof(header));
    
    *pUid = header.id;
    *pdwInfoLevel = header.dwInfoLevel;
    
cleanup:

    return dwError;
    
error:

    *pUid = 0;
    *pdwInfoLevel = 0;

    goto cleanup;
}

DWORD
LsaMarshalDeleteUserByIdQuery(
    uid_t uid,
    PSTR  pszBuffer,
    PDWORD pdwBufLen
    )
{
    DWORD dwError = 0;
    DWORD dwBufLenRequired = sizeof(uid_t);
    
    if (!pszBuffer) {
        *pdwBufLen = dwBufLenRequired;
        goto cleanup;
    }
    
    if (*pdwBufLen < dwBufLenRequired) {
        dwError = LSA_ERROR_INSUFFICIENT_BUFFER;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    memcpy(pszBuffer, &uid, sizeof(uid_t));
    
cleanup:

    return dwError;
    
error:

    goto cleanup;
}

DWORD
LsaUnmarshalDeleteUserByIdQuery(
    PCSTR pszMsgBuf,
    DWORD dwMsgLen,
    uid_t* pUid
    )
{
    DWORD dwError = 0;
    
    if (dwMsgLen < sizeof(uid_t)) {
        dwError = LSA_ERROR_INVALID_MESSAGE;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    memcpy(pUid, pszMsgBuf, sizeof(uid_t));
    
cleanup:

    return dwError;
    
error:

    *pUid = 0;
    
    goto cleanup;
}

DWORD
LsaMarshalUserModInfo(
    PLSA_USER_MOD_INFO pUserModInfo,
    PSTR   pszBuffer,
    PDWORD pdwBufLen
    )
{
    DWORD dwError = 0;
    LSA_MOD_USER_RECORD_HEADER header = {0};
    DWORD dwRequiredBufLen = 0;
    PSTR  pszData = NULL;
    DWORD dwOffset = 0;
    
    dwRequiredBufLen = LsaComputeBufferSize_UserModInfo(
                                pUserModInfo);
    
    if (!pszBuffer) {
       *pdwBufLen = dwRequiredBufLen;
       goto cleanup;
    }
    
    if (*pdwBufLen < dwRequiredBufLen) {
       dwError = LSA_ERROR_INSUFFICIENT_BUFFER;
       BAIL_ON_LSA_ERROR(dwError);
    }
    
    header.uid = pUserModInfo->uid;
    
    header.actions.bEnableUser = pUserModInfo->actions.bEnableUser;
    header.actions.bDisableUser = pUserModInfo->actions.bDisableUser;
    header.actions.bUnlockUser = pUserModInfo->actions.bUnlockUser;
    header.actions.bChangePasswordOnNextLogon = pUserModInfo->actions.bSetChangePasswordOnNextLogon;
    header.actions.bSetPasswordNeverExpires = pUserModInfo->actions.bSetPasswordNeverExpires;
    header.actions.bSetPasswordMustExpire = pUserModInfo->actions.bSetPasswordMustExpire;
    header.actions.bAddToGroups = pUserModInfo->actions.bAddToGroups;
    header.actions.bRemoveFromGroups = pUserModInfo->actions.bRemoveFromGroups;
    header.actions.bSetAccountExpiryDate = pUserModInfo->actions.bSetAccountExpiryDate;
    
    dwOffset = sizeof(LSA_MOD_USER_RECORD_HEADER);
    pszData = pszBuffer + dwOffset;
    
    if (pUserModInfo->actions.bAddToGroups &&
        !IsNullOrEmptyString(pUserModInfo->pszAddToGroups)) {
       header.addToGroups.length = strlen(pUserModInfo->pszAddToGroups);
       header.addToGroups.offset = dwOffset;
       memcpy(pszData, pUserModInfo->pszAddToGroups, header.addToGroups.length);
       pszData += header.addToGroups.length;
       dwOffset += header.addToGroups.length;
    }
    
    if (pUserModInfo->actions.bRemoveFromGroups &&
        !IsNullOrEmptyString(pUserModInfo->pszRemoveFromGroups)) {
       header.removeFromGroups.length = strlen(pUserModInfo->pszRemoveFromGroups);
       header.removeFromGroups.offset = dwOffset;
       memcpy(pszData, pUserModInfo->pszRemoveFromGroups, header.removeFromGroups.length);
       pszData += header.removeFromGroups.length;
       dwOffset += header.removeFromGroups.length;
    }
    
    if (pUserModInfo->actions.bSetAccountExpiryDate &&
        !IsNullOrEmptyString(pUserModInfo->pszExpiryDate)) {
       header.accountExpiryDate.length = strlen(pUserModInfo->pszExpiryDate);
       header.accountExpiryDate.offset = dwOffset;
       memcpy(pszData, pUserModInfo->pszExpiryDate, header.accountExpiryDate.length);
       // pszData += header.accountExpiryDate.length;
       // dwOffset += header.accountExpiryDate.length;
    }
    
    memcpy(pszBuffer, &header, sizeof(header));
    
cleanup:

    return dwError;
    
error:

    goto cleanup;
}

DWORD
LsaComputeBufferSize_UserModInfo(
    PLSA_USER_MOD_INFO pUserModInfo
    )
{
    DWORD dwBufLen = sizeof(LSA_MOD_USER_RECORD_HEADER);
    
    if (pUserModInfo->actions.bAddToGroups &&
        !IsNullOrEmptyString(pUserModInfo->pszAddToGroups)) {
       dwBufLen += strlen(pUserModInfo->pszAddToGroups);
    }
    
    if (pUserModInfo->actions.bRemoveFromGroups &&
        !IsNullOrEmptyString(pUserModInfo->pszRemoveFromGroups)) {
       dwBufLen += strlen(pUserModInfo->pszRemoveFromGroups);
    }
    
    if (pUserModInfo->actions.bSetAccountExpiryDate &&
        !IsNullOrEmptyString(pUserModInfo->pszExpiryDate)) {
       dwBufLen += strlen(pUserModInfo->pszExpiryDate);
    }
    
    return dwBufLen;
}

DWORD
LsaUnmarshalUserModInfo(
    PCSTR  pszMsgBuf,
    DWORD  dwMsgLen,
    PLSA_USER_MOD_INFO* ppUserModInfo
    )
{
    DWORD dwError = 0;
    PLSA_USER_MOD_INFO pUserModInfo = NULL;
    LSA_MOD_USER_RECORD_HEADER header = {0};
    
    dwError = LsaAllocateMemory(
                    sizeof(LSA_USER_MOD_INFO),
                    (PVOID*)&pUserModInfo);
    BAIL_ON_LSA_ERROR(dwError);
    
    if (dwMsgLen < sizeof(header)) {
       dwError = LSA_ERROR_INVALID_MESSAGE;
       BAIL_ON_LSA_ERROR(dwError);
    }
    
    memcpy(&header, pszMsgBuf, sizeof(header));
    
    pUserModInfo->uid = header.uid;
    pUserModInfo->actions.bEnableUser = header.actions.bEnableUser;
    pUserModInfo->actions.bDisableUser = header.actions.bDisableUser;
    pUserModInfo->actions.bUnlockUser = header.actions.bUnlockUser;
    pUserModInfo->actions.bAddToGroups = header.actions.bAddToGroups;
    pUserModInfo->actions.bSetAccountExpiryDate = header.actions.bSetAccountExpiryDate;
    pUserModInfo->actions.bAddToGroups = header.actions.bAddToGroups;
    pUserModInfo->actions.bRemoveFromGroups = header.actions.bRemoveFromGroups;
    pUserModInfo->actions.bSetChangePasswordOnNextLogon = header.actions.bChangePasswordOnNextLogon;
    pUserModInfo->actions.bSetPasswordNeverExpires = header.actions.bSetPasswordNeverExpires;
    pUserModInfo->actions.bSetPasswordMustExpire = header.actions.bSetPasswordMustExpire;
    
    if (header.actions.bAddToGroups &&
        header.addToGroups.length) {
       dwError = LsaStrndup(
                       pszMsgBuf + header.addToGroups.offset,
                       header.addToGroups.length,
                       &pUserModInfo->pszAddToGroups);
       BAIL_ON_LSA_ERROR(dwError);
    }
    
    if (header.actions.bRemoveFromGroups &&
        header.removeFromGroups.length) {
       dwError = LsaStrndup(
                       pszMsgBuf + header.removeFromGroups.offset,
                       header.removeFromGroups.length,
                       &pUserModInfo->pszRemoveFromGroups);
       BAIL_ON_LSA_ERROR(dwError);
    }
    
    if (header.actions.bSetAccountExpiryDate &&
        header.accountExpiryDate.length) {
       dwError = LsaStrndup(
                       pszMsgBuf + header.accountExpiryDate.offset,
                       header.accountExpiryDate.length,
                       &pUserModInfo->pszExpiryDate);
       BAIL_ON_LSA_ERROR(dwError);
    }
    
    *ppUserModInfo = pUserModInfo;
    
cleanup:

    return dwError;
    
error:

    *ppUserModInfo = NULL;
    
    if (pUserModInfo) {
       LsaFreeUserModInfo(pUserModInfo);
    }

    goto cleanup;
}

DWORD
LsaMarshalGetNamesBySidListQuery(
    size_t          sCount,
    PSTR*           ppszSidList,
    PSTR   pszBuffer,
    PDWORD pdwBufLen)
{
    DWORD dwError = 0;
    DWORD dwRequiredBufLen = 0;
    size_t sIndex = 0;
    size_t sLen = 0;
    //do not free
    PLSA_QUERY_GET_NAMES_BY_SID_LIST pHeader =
        (PLSA_QUERY_GET_NAMES_BY_SID_LIST)pszBuffer;
    //do not free
    PSTR pszDataPos = NULL;

    if (sCount > DWORD_MAX)
    {
        dwError = ERANGE;
        BAIL_ON_LSA_ERROR(dwError);
    }

    /*Marshal the data in this format:
     * DWORD count (return an error if sCount is larger than a DWORD)
     * LSADATACOORDINATES coords for sid1
     * LSADATACOORDINATES coords for sid2
     * ...
     * string data1
     * string data2
     * ...
     */
    
    dwRequiredBufLen =
        sizeof(LSA_QUERY_GET_NAMES_BY_SID_LIST) +
        //There is already space for one set of coords in the struct
        sizeof(LSADATACOORDINATES) * (sCount - 1)
        ;

    for (sIndex = 0; sIndex < sCount; sIndex++)
    {
        sLen = strlen(ppszSidList[sIndex]);
        if (sLen > DWORD_MAX)
        {
            dwError = ERANGE;
            BAIL_ON_LSA_ERROR(dwError);
        }
        dwRequiredBufLen += (DWORD)sLen; //data for string
    }

    if (!pszBuffer) {
        *pdwBufLen = dwRequiredBufLen;
        goto cleanup;
    }
    
    if (*pdwBufLen < dwRequiredBufLen) {
        dwError = LSA_ERROR_INSUFFICIENT_BUFFER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pszDataPos = pszBuffer +
        sizeof(LSA_QUERY_GET_NAMES_BY_SID_LIST) +
        //There is already space for one set of coords in the struct
        sizeof(LSADATACOORDINATES) * (sCount - 1)
        ;

    pHeader->dwCount = (DWORD)sCount;

    for (sIndex = 0; sIndex < sCount; sIndex++)
    {
        sLen = strlen(ppszSidList[sIndex]);

        pHeader->coordinates[sIndex].offset = pszDataPos - pszBuffer;
        pHeader->coordinates[sIndex].length = (DWORD)sLen;

        memcpy(pszDataPos, ppszSidList[sIndex], sLen);
        pszDataPos += sLen;
    }
    
cleanup:

    return dwError;
    
error:

    goto cleanup;    
}

DWORD
LsaUnmarshalGetNamesBySidListQuery(
    PCSTR   pszMsgBuf,
    DWORD   dwMsgLen,
    size_t* psCount,
    PSTR**  pppszSidList)
{
    DWORD dwError = 0;
    size_t sIndex = 0;
    size_t sCount = 0;
    //do not free
    PLSA_QUERY_GET_NAMES_BY_SID_LIST pHeader =
        (PLSA_QUERY_GET_NAMES_BY_SID_LIST)pszMsgBuf;
    PSTR* ppszSidList = NULL;

    if (dwMsgLen < sizeof(DWORD))
    {
        dwError = LSA_ERROR_INVALID_MESSAGE;
        BAIL_ON_LSA_ERROR(dwError);
    }

    /*Marshal the data in this format:
     * DWORD count (return an error if sCount is larger than a DWORD)
     * LSADATACOORDINATES coords for sid1
     * LSADATACOORDINATES coords for sid2
     * ...
     * string data1
     * string data2
     * ...
     */
    
    sCount = pHeader->dwCount;
    if (dwMsgLen < sizeof(LSA_QUERY_GET_NAMES_BY_SID_LIST) +
        sizeof(LSADATACOORDINATES) * (sCount - 1))
    {
        dwError = LSA_ERROR_INVALID_MESSAGE;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaAllocateMemory(
                    sizeof(*ppszSidList) * sCount,
                    (PVOID*)&ppszSidList);
    BAIL_ON_LSA_ERROR(dwError);

    for (sIndex = 0; sIndex < sCount; sIndex++)
    {
        if (pHeader->coordinates[sIndex].length > 0)
        {
            /* Check that the coordinates are valid.
             * 1. Make sure the offset doesn't wrap around to the beginning
             *    of the buffer.
             * 2. Make sure the offset doesn't start past the end of the
             *    buffer.
             * 3. Make sure the offset plus the length doesn't extend past
             *    the end of the buffer.
             */
            if (pszMsgBuf + pHeader->coordinates[sIndex].offset < pszMsgBuf ||
                pHeader->coordinates[sIndex].offset >= dwMsgLen ||
                pHeader->coordinates[sIndex].offset +
                pHeader->coordinates[sIndex].length > dwMsgLen)
            {
                dwError = LSA_ERROR_INVALID_MESSAGE;
                BAIL_ON_LSA_ERROR(dwError);
            }
            dwError = LsaStrndup(
                            pszMsgBuf + pHeader->coordinates[sIndex].offset,
                            pHeader->coordinates[sIndex].length,
                            &ppszSidList[sIndex]);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    *psCount = sCount;
    *pppszSidList = ppszSidList;

cleanup:

    return dwError;
    
error:
    *psCount = 0;
    *pppszSidList = NULL;

    LsaFreeStringArray(ppszSidList, sCount);

    goto cleanup;    
}

DWORD
LsaMarshalGetNamesBySidListReply(
    size_t          sCount,
    PSTR*           ppszDomainNames,
    PSTR*           ppszSamAccounts,
    ADAccountType*  pTypes,
    PSTR            pszBuffer,
    PDWORD          pdwBufLen)
{
    DWORD dwError = 0;
    DWORD dwRequiredBufLen = 0;
    size_t sIndex = 0;
    size_t sLen = 0;
    //do not free
    PLSA_REPLY_GET_NAMES_BY_SID_LIST pHeader =
        (PLSA_REPLY_GET_NAMES_BY_SID_LIST)pszBuffer;
    //do not free
    PSTR pszDataPos = NULL;

    if (sCount > DWORD_MAX)
    {
        dwError = ERANGE;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwRequiredBufLen =
        sizeof(LSA_REPLY_GET_NAMES_BY_SID_LIST) +
        sizeof(pHeader->entries[0]) * sCount -
        sizeof(pHeader->entries);

    for (sIndex = 0; sIndex < sCount; sIndex++)
    {
        if (!IsNullOrEmptyString(ppszDomainNames[sIndex]))
        {
            sLen = strlen(ppszDomainNames[sIndex]);
            if (sLen > DWORD_MAX)
            {
                dwError = ERANGE;
                BAIL_ON_LSA_ERROR(dwError);
            }
            dwRequiredBufLen += (DWORD)sLen; //data for string
        }

        if (!IsNullOrEmptyString(ppszSamAccounts[sIndex]))
        {
            sLen = strlen(ppszSamAccounts[sIndex]);
            if (sLen > DWORD_MAX)
            {
                dwError = ERANGE;
                BAIL_ON_LSA_ERROR(dwError);
            }
            dwRequiredBufLen += (DWORD)sLen; //data for string
        }
    }

    if (!pszBuffer) {
        *pdwBufLen = dwRequiredBufLen;
        goto cleanup;
    }
    
    if (*pdwBufLen < dwRequiredBufLen) {
        dwError = LSA_ERROR_INSUFFICIENT_BUFFER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pszDataPos = pszBuffer +
        sizeof(LSA_REPLY_GET_NAMES_BY_SID_LIST) +
        sizeof(pHeader->entries[0]) * sCount -
        sizeof(pHeader->entries);

    pHeader->dwCount = (DWORD)sCount;

    for (sIndex = 0; sIndex < sCount; sIndex++)
    {
        if (!IsNullOrEmptyString(ppszDomainNames[sIndex]))
        {
            sLen = strlen(ppszDomainNames[sIndex]);

            pHeader->entries[sIndex].domainName.offset = pszDataPos - pszBuffer;
            pHeader->entries[sIndex].domainName.length = (DWORD)sLen;

            memcpy(pszDataPos, ppszDomainNames[sIndex], sLen);
            pszDataPos += sLen;
        }

        if (!IsNullOrEmptyString(ppszSamAccounts[sIndex]))
        {
            sLen = strlen(ppszSamAccounts[sIndex]);

            pHeader->entries[sIndex].samAccount.offset = pszDataPos - pszBuffer;
            pHeader->entries[sIndex].samAccount.length = (DWORD)sLen;

            memcpy(pszDataPos, ppszSamAccounts[sIndex], sLen);
            pszDataPos += sLen;
        }
        
        pHeader->entries[sIndex].dwType = pTypes[sIndex];
    }
    
cleanup:

    return dwError;
    
error:

    goto cleanup;    
}

DWORD
LsaUnmarshalGetNamesBySidListReply(
    PCSTR           pszMsgBuf,
    DWORD           dwMsgLen,
    size_t*         psCount,
    PLSA_SID_INFO*  ppSIDInfoList
    )
{
    DWORD dwError = 0;
    size_t sIndex = 0;
    size_t sCount = 0;
    PLSA_SID_INFO pSIDInfoList = NULL;
    //do not free
    PLSA_REPLY_GET_NAMES_BY_SID_LIST pHeader =
        (PLSA_REPLY_GET_NAMES_BY_SID_LIST)pszMsgBuf;

    if (dwMsgLen < sizeof(DWORD))
    {
        dwError = LSA_ERROR_INVALID_MESSAGE;
        BAIL_ON_LSA_ERROR(dwError);
    }

    sCount = pHeader->dwCount;
    if (dwMsgLen < sizeof(LSA_REPLY_GET_NAMES_BY_SID_LIST) +
        sizeof(pHeader->entries[0]) * sCount -
        sizeof(pHeader->entries))
    {
        dwError = LSA_ERROR_INVALID_MESSAGE;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaAllocateMemory(
                    sizeof(LSA_SID_INFO) * sCount,
                    (PVOID*)&pSIDInfoList);
    BAIL_ON_LSA_ERROR(dwError);

    for (sIndex = 0; sIndex < sCount; sIndex++)
    {
        pSIDInfoList[sIndex].accountType = (ADAccountType)pHeader->entries[sIndex].dwType;
        if (pSIDInfoList[sIndex].accountType == AccountType_NotFound)
        {
            continue;
        }
        
        if (pHeader->entries[sIndex].domainName.length > 0)
        {
            /* Check that the coordinates are valid.
             * 1. Make sure the offset doesn't wrap around to the beginning
             *    of the buffer.
             * 2. Make sure the offset doesn't start past the end of the
             *    buffer.
             * 3. Make sure the offset plus the length doesn't extend past
             *    the end of the buffer.
             */
            if (pszMsgBuf + pHeader->entries[sIndex].domainName.offset <
                    pszMsgBuf ||
                pHeader->entries[sIndex].domainName.offset >= dwMsgLen ||
                pHeader->entries[sIndex].domainName.offset +
                    pHeader->entries[sIndex].domainName.length > dwMsgLen)
            {
                dwError = LSA_ERROR_INVALID_MESSAGE;
                BAIL_ON_LSA_ERROR(dwError);
            }
            dwError = LsaStrndup(
                        pszMsgBuf + pHeader->entries[sIndex].domainName.offset,
                        pHeader->entries[sIndex].domainName.length,
                        &pSIDInfoList[sIndex].pszDomainName);
            BAIL_ON_LSA_ERROR(dwError);
        }
        if (pHeader->entries[sIndex].samAccount.length > 0)
        {
            /* Check that the coordinates are valid.
             * 1. Make sure the offset doesn't wrap around to the beginning
             *    of the buffer.
             * 2. Make sure the offset doesn't start past the end of the
             *    buffer.
             * 3. Make sure the offset plus the length doesn't extend past
             *    the end of the buffer.
             */
            if (pszMsgBuf + pHeader->entries[sIndex].samAccount.offset <
                    pszMsgBuf ||
                pHeader->entries[sIndex].samAccount.offset >= dwMsgLen ||
                pHeader->entries[sIndex].samAccount.offset +
                    pHeader->entries[sIndex].samAccount.length > dwMsgLen)
            {
                dwError = LSA_ERROR_INVALID_MESSAGE;
                BAIL_ON_LSA_ERROR(dwError);
            }
            dwError = LsaStrndup(
                        pszMsgBuf + pHeader->entries[sIndex].samAccount.offset,
                        pHeader->entries[sIndex].samAccount.length,
                        &pSIDInfoList[sIndex].pszSamAccountName);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    *psCount = sCount;
    *ppSIDInfoList = pSIDInfoList;

cleanup:

    return dwError;
    
error:

    *psCount = 0;
    *ppSIDInfoList = NULL;
    
    if (pSIDInfoList) {
        LsaFreeSIDInfoList(pSIDInfoList, sCount);
    }

    goto cleanup;    
}
