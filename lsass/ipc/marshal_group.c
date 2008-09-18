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
 *        marshal_group.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Marshal/Unmarshal API for Messages related to Groups
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *
 */
#include "ipc.h"

DWORD
LsaMarshalGroupInfoList(
    PVOID* ppGroupInfoList,
    DWORD  dwInfoLevel,
    DWORD  dwNumGroups,
    PSTR   pszBuffer,
    PDWORD pdwBufLen
    )
{
    DWORD dwError = 0;
    DWORD dwRequiredBufLen = 0;
    DWORD dwBytesWritten = 0;
    LSA_USER_GROUP_RECORD_PREAMBLE header;

    dwError = LsaComputeGroupBufferSize(
                    ppGroupInfoList,
                    dwInfoLevel,
                    dwNumGroups,
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
    header.dwNumRecords = dwNumGroups;
    
    memcpy(pszBuffer, &header, sizeof(header));
    dwBytesWritten = sizeof(header);
    
    switch (dwInfoLevel)
    {
        case 0:
        {
            DWORD dwTmpBytesWritten = 0;
            
            dwError = LsaMarshalGroup_0_InfoList(
                            ppGroupInfoList,
                            dwNumGroups,
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
            
            dwError = LsaMarshalGroup_1_InfoList(
                            ppGroupInfoList,
                            dwNumGroups,
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
            dwError = LSA_ERROR_UNSUPPORTED_GROUP_LEVEL;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }
        
cleanup:

    return dwError;
    
error:

    goto cleanup;
}

DWORD
LsaComputeGroupBufferSize(
    PVOID* ppGroupInfoList,
    DWORD  dwInfoLevel,
    DWORD  dwNumGroups,
    PDWORD pdwBufLen
    )
{
    DWORD dwError = 0;
    DWORD dwTotalBufLen = sizeof(LSA_USER_GROUP_RECORD_PREAMBLE);
    
    if (dwNumGroups) {
        
        switch(dwInfoLevel)
        {
            case 0:
            {
                dwTotalBufLen += LsaComputeBufferSize_Group0(
                                    ppGroupInfoList,
                                    dwNumGroups
                                    );
                break;
            }
            case 1:
            {
                dwTotalBufLen += LsaComputeBufferSize_Group1(
                                    ppGroupInfoList,
                                    dwNumGroups
                                    );
                break;
            }

            default:
            {
                dwError = LSA_ERROR_UNSUPPORTED_GROUP_LEVEL;
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
LsaComputeBufferSize_Group0(
    PVOID* ppGroupInfoList,
    DWORD  dwNumGroups
    )
{
    DWORD dwBufLen = 0;
    DWORD iGroup = 0;
    PLSA_GROUP_INFO_0 pGroupInfo = NULL;
    
    for (iGroup = 0; iGroup < dwNumGroups; iGroup++)
    {
        pGroupInfo = (PLSA_GROUP_INFO_0)*(ppGroupInfoList+iGroup);
        dwBufLen += sizeof(LSA_GROUP_0_RECORD_HEADER);
        
        if (!IsNullOrEmptyString(pGroupInfo->pszName)) {
            dwBufLen += strlen(pGroupInfo->pszName);
        }

        if (!IsNullOrEmptyString(pGroupInfo->pszSid)) {
            dwBufLen += strlen(pGroupInfo->pszSid);
        }
    }
    
    return dwBufLen;
}

DWORD
LsaComputeBufferSize_Group1(
    PVOID* ppGroupInfoList,
    DWORD  dwNumGroups
    )
{
    DWORD dwBufLen = 0;
    DWORD iGroup = 0;
    PLSA_GROUP_INFO_1 pGroupInfo = NULL;
    
    for (iGroup = 0; iGroup < dwNumGroups; iGroup++)
    {
        pGroupInfo = (PLSA_GROUP_INFO_1)*(ppGroupInfoList+iGroup);
        dwBufLen += sizeof(LSA_GROUP_1_RECORD_HEADER);
        
        if (!IsNullOrEmptyString(pGroupInfo->pszName)) {
            dwBufLen += strlen(pGroupInfo->pszName);
        }
        if (!IsNullOrEmptyString(pGroupInfo->pszPasswd)) {
            dwBufLen += strlen(pGroupInfo->pszPasswd);
        }
        if (!IsNullOrEmptyString(pGroupInfo->pszSid)) {
            dwBufLen += strlen(pGroupInfo->pszSid);
        }
        dwBufLen += LsaGetGroupMemberBufferLength(pGroupInfo->ppszMembers);
    }
    
    return dwBufLen;
}

/*
 * PVOID* ppGroupInfoList
 *        Array of Group Info structures
 * DWORD  dwNumGroups
 *        How many groups are in the array
 * DWORD  dwBeginOffset
 *        Bytes from beginning of buffer
 * PSTR   pszBuffer
 *        Very beginning of the buffer
 * PDWORD pdwBytesWritten
 *        The headers are followed by the data.
 *        This is how much string/data we wrote
 */
DWORD
LsaMarshalGroup_0_InfoList(
    PVOID* ppGroupInfoList,
    DWORD  dwNumGroups,
    DWORD  dwBeginOffset,
    PSTR   pszBuffer,
    PDWORD pdwDataBytesWritten
    )
{
    DWORD dwError = 0;
    // This is where we are from the beginning of the buffer
    DWORD iGroup = 0;
    // This is where we will start writing strings
    DWORD dwCurrentDataOffset = dwBeginOffset + (sizeof(LSA_GROUP_0_RECORD_HEADER) * dwNumGroups);
    DWORD dwTotalDataBytesWritten = 0;
    
    for (iGroup = 0; iGroup < dwNumGroups; iGroup++)
    {
        DWORD dwDataBytesWritten = 0;
        PSTR  pszHeaderOffset = pszBuffer + dwBeginOffset + sizeof(LSA_GROUP_0_RECORD_HEADER) * iGroup;
        PSTR  pszDataOffset = pszBuffer + dwCurrentDataOffset;
        
        dwError = LsaMarshalGroup_0(
                        (PLSA_GROUP_INFO_0)*(ppGroupInfoList+iGroup),
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
LsaMarshalGroup_0(
    PLSA_GROUP_INFO_0 pGroupInfo,
    PSTR   pszHeaderBuffer,
    PSTR   pszDataBuffer,
    DWORD  dwDataBeginOffset,
    PDWORD pdwDataBytesWritten
    )
{
    DWORD dwError = 0;
    DWORD dwGlobalOffset = dwDataBeginOffset;
    DWORD dwOffset = 0;
    LSA_GROUP_0_RECORD_HEADER header;
    DWORD dwDataBytesWritten = 0;
    
    // Prepare and write the header
    memset(&header, 0, sizeof(header));
    
    header.gid = pGroupInfo->gid;
    
    if (!IsNullOrEmptyString(pGroupInfo->pszName)) {
       header.name.length = strlen(pGroupInfo->pszName);
       // We always indicate the offset from the beginning of the buffer
       header.name.offset = dwGlobalOffset;
       memcpy(pszDataBuffer+dwOffset, pGroupInfo->pszName, header.name.length);
       dwOffset += header.name.length;
       dwGlobalOffset += header.name.length;
       dwDataBytesWritten += header.name.length;
    }
    
    if (!IsNullOrEmptyString(pGroupInfo->pszSid)) {
       header.sid.length = strlen(pGroupInfo->pszSid);
       header.sid.offset = dwGlobalOffset;
       memcpy(pszDataBuffer+dwOffset, pGroupInfo->pszSid, header.sid.length);
       dwOffset += header.sid.length;
       dwGlobalOffset += header.sid.length;
       dwDataBytesWritten += header.sid.length;
    }
    
    memcpy(pszHeaderBuffer, &header, sizeof(header));
    
    *pdwDataBytesWritten = dwDataBytesWritten;

    return dwError;
}

/*
 * PVOID* ppGroupInfoList
 *        Array of Group Info structures
 * DWORD  dwNumGroups
 *        How many groups are in the array
 * DWORD  dwBeginOffset
 *        Bytes from beginning of buffer
 * PSTR   pszBuffer
 *        Very beginning of the buffer
 * PDWORD pdwBytesWritten
 *        The headers are followed by the data.
 *        This is how much string/data we wrote
 */
DWORD
LsaMarshalGroup_1_InfoList(
    PVOID* ppGroupInfoList,
    DWORD  dwNumGroups,
    DWORD  dwBeginOffset,
    PSTR   pszBuffer,
    PDWORD pdwDataBytesWritten
    )
{
    DWORD dwError = 0;
    // This is where we are from the beginning of the buffer
    DWORD iGroup = 0;
    // This is where we will start writing strings
    DWORD dwCurrentDataOffset = dwBeginOffset + (sizeof(LSA_GROUP_1_RECORD_HEADER) * dwNumGroups);
    DWORD dwTotalDataBytesWritten = 0;
    
    for (iGroup = 0; iGroup < dwNumGroups; iGroup++)
    {
        DWORD dwDataBytesWritten = 0;
        PSTR  pszHeaderOffset = pszBuffer + dwBeginOffset + sizeof(LSA_GROUP_1_RECORD_HEADER) * iGroup;
        PSTR  pszDataOffset = pszBuffer + dwCurrentDataOffset;
        
        dwError = LsaMarshalGroup_1(
                        (PLSA_GROUP_INFO_1)*(ppGroupInfoList+iGroup),
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
LsaMarshalGroup_1(
    PLSA_GROUP_INFO_1 pGroupInfo,
    PSTR   pszHeaderBuffer,
    PSTR   pszDataBuffer,
    DWORD  dwDataBeginOffset,
    PDWORD pdwDataBytesWritten
    )
{
    DWORD dwError = 0;
    DWORD dwGlobalOffset = dwDataBeginOffset;
    DWORD dwOffset = 0;
    LSA_GROUP_1_RECORD_HEADER header;
    DWORD dwDataBytesWritten = 0;
    
    // Prepare and write the header
    memset(&header, 0, sizeof(header));
    
    header.gid = pGroupInfo->gid;
 
    if (!IsNullOrEmptyString(pGroupInfo->pszName)) {
       header.name.length = strlen(pGroupInfo->pszName);
       // We always indicate the offset from the beginning of the buffer
       header.name.offset = dwGlobalOffset;
       memcpy(pszDataBuffer+dwOffset, pGroupInfo->pszName, header.name.length);
       dwOffset += header.name.length;
       dwGlobalOffset += header.name.length;
       dwDataBytesWritten += header.name.length;
    }
    
    if (!IsNullOrEmptyString(pGroupInfo->pszPasswd)) {
       header.passwd.length = strlen(pGroupInfo->pszPasswd);
       header.passwd.offset = dwGlobalOffset;
       memcpy(pszDataBuffer+dwOffset, pGroupInfo->pszPasswd, header.passwd.length);
       dwOffset += header.passwd.length;
       dwGlobalOffset += header.passwd.length;
       dwDataBytesWritten += header.passwd.length;
    }
    
    if (!IsNullOrEmptyString(pGroupInfo->pszSid)) {
       header.sid.length = strlen(pGroupInfo->pszSid);
       header.sid.offset = dwGlobalOffset;
       memcpy(pszDataBuffer+dwOffset, pGroupInfo->pszSid, header.sid.length);
       dwOffset += header.sid.length;
       dwGlobalOffset += header.sid.length;
       dwDataBytesWritten += header.sid.length;
    }
    
    if (pGroupInfo->ppszMembers) {
       PSTR* ppszGroupMembers = pGroupInfo->ppszMembers;
       PSTR  pszCurrent = pszDataBuffer+dwOffset;
       
       header.gr_mem.length = LsaGetGroupMemberBufferLength(pGroupInfo->ppszMembers);
       header.gr_mem.offset = dwGlobalOffset;
       
       while (!IsNullOrEmptyString(*ppszGroupMembers)) {
             DWORD dwMemberLen = strlen(*ppszGroupMembers);
             memcpy(pszCurrent, *ppszGroupMembers, dwMemberLen);
             ppszGroupMembers++;
             pszCurrent += dwMemberLen + 1;
             *pszCurrent = 0;        
       }       
       pszCurrent++;
       *pszCurrent = 0;
       dwDataBytesWritten += header.gr_mem.length;        
    }
    
    memcpy(pszHeaderBuffer, &header, sizeof(header));
    
    *pdwDataBytesWritten = dwDataBytesWritten;

    return dwError;
}

DWORD
LsaGetGroupMemberBufferLength(
    PSTR* ppszGroupMembers
    )
{
    DWORD dwGrMemberLen = 0;
    PSTR* ppszGrMem = ppszGroupMembers;
    DWORD nMembers = 0;
            
    while (ppszGrMem && !IsNullOrEmptyString(*ppszGrMem)) {
          nMembers++;
          dwGrMemberLen += strlen(*ppszGrMem) + 1;
          ppszGrMem++;
    }
  
    return dwGrMemberLen + 1;
}

DWORD
LsaUnmarshalGroupInfoList(
    PCSTR   pszMsgBuf,
    DWORD   dwMsgLen,
    PDWORD  pdwInfoLevel,
    PVOID** pppGroupInfoList,
    PDWORD  pdwNumGroups
    )
{
    DWORD dwError = 0;
    PVOID* ppGroupInfoList = NULL;
    LSA_USER_GROUP_RECORD_PREAMBLE header;
    DWORD dwOffset = 0;
    DWORD dwInfoLevel = 0;
    
    if (dwMsgLen < sizeof(header)) {
        dwError = LSA_ERROR_INVALID_MESSAGE;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    memcpy(&header, pszMsgBuf, sizeof(header));
    dwOffset += sizeof(header);
    
    switch(header.dwInfoLevel)
    {
        case 0:
        {
            dwInfoLevel = 0;

            dwError = LsaUnmarshalGroup_0_InfoList(
                            pszMsgBuf,
                            pszMsgBuf+sizeof(header),
                            &ppGroupInfoList,
                            header.dwNumRecords);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        }
        case 1:
        {
            dwInfoLevel = 1;

            dwError = LsaUnmarshalGroup_1_InfoList(
                            pszMsgBuf,
                            pszMsgBuf+sizeof(header),
                            &ppGroupInfoList,
                            header.dwNumRecords);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        }
        default:
        {
            dwError = LSA_ERROR_UNSUPPORTED_GROUP_LEVEL;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }
    
    *pppGroupInfoList = ppGroupInfoList;
    *pdwNumGroups = header.dwNumRecords;
    *pdwInfoLevel = dwInfoLevel;
    
cleanup:

    return dwError;
    
error:

    *pppGroupInfoList = NULL;
    *pdwNumGroups = 0;
    *pdwInfoLevel = 0;
    
    if (ppGroupInfoList) {
        LsaFreeGroupInfoList(
                  header.dwInfoLevel,
                  (PVOID*)ppGroupInfoList,
                  header.dwNumRecords);
    }

    goto cleanup;
}

DWORD
LsaUnmarshalGroup_0_InfoList(
    PCSTR pszMsgBuf,
    PCSTR pszHdrBuf,
    PVOID** pppGroupInfoList,
    DWORD dwNumGroups
    )
{
    DWORD dwError = 0;
    PLSA_GROUP_INFO_0* ppGroupInfoList = NULL;
    PLSA_GROUP_INFO_0 pGroupInfo = NULL;
    DWORD dwGroupInfoLevel = 0;
    DWORD iGroup = 0;
    
    dwError = LsaAllocateMemory(
                    sizeof(PLSA_GROUP_INFO_0) * dwNumGroups,
                    (PVOID*)&ppGroupInfoList
                    );
    BAIL_ON_LSA_ERROR(dwError);
    
    for (iGroup = 0; iGroup < dwNumGroups; iGroup++) {
        LSA_GROUP_0_RECORD_HEADER header;
                
        memcpy(&header, pszHdrBuf + (iGroup * sizeof(LSA_GROUP_0_RECORD_HEADER)), sizeof(LSA_GROUP_0_RECORD_HEADER));
                
        dwError = LsaUnmarshalGroup_0(pszMsgBuf,
                                      &header,
                                      &pGroupInfo);
        BAIL_ON_LSA_ERROR(dwError);
        
        *(ppGroupInfoList+iGroup) = pGroupInfo;
        pGroupInfo = NULL;
    }
    
    *pppGroupInfoList = (PVOID*)ppGroupInfoList;
    
cleanup:

    return dwError;
    
error:

    *pppGroupInfoList = NULL;
    
    if (pGroupInfo) {
        LsaFreeGroupInfo(dwGroupInfoLevel, pGroupInfo);
    }
    
    if (ppGroupInfoList) {
        LsaFreeGroupInfoList(
               dwGroupInfoLevel,
               (PVOID*)ppGroupInfoList,
               dwNumGroups
               );
    }

    goto cleanup;
}

DWORD
LsaUnmarshalGroup_0(
    PCSTR pszMsgBuf,
    PLSA_GROUP_0_RECORD_HEADER pHeader,
    PLSA_GROUP_INFO_0* ppGroupInfo
    )
{
    DWORD dwError = 0;
    PLSA_GROUP_INFO_0 pGroupInfo = NULL;
    DWORD dwGroupInfoLevel = 0;
    
    dwError = LsaAllocateMemory(
                    sizeof(LSA_GROUP_INFO_0),
                    (PVOID*)&pGroupInfo);
    BAIL_ON_LSA_ERROR(dwError);
    
    pGroupInfo->gid = pHeader->gid;
    
    if (pHeader->name.length) {
        dwError = LsaAllocateMemory(
                      pHeader->name.length+1,
                      (PVOID*)&pGroupInfo->pszName);
        BAIL_ON_LSA_ERROR(dwError);
        memcpy(pGroupInfo->pszName, pszMsgBuf+pHeader->name.offset, pHeader->name.length);
    }
        
    if (pHeader->sid.length) {
        dwError = LsaAllocateMemory(
                      pHeader->sid.length+1,
                      (PVOID*)&pGroupInfo->pszSid);
        BAIL_ON_LSA_ERROR(dwError);
        memcpy(pGroupInfo->pszSid, pszMsgBuf+pHeader->sid.offset, pHeader->sid.length);
    }
        
    *ppGroupInfo = pGroupInfo;
    
cleanup:

    return dwError;

error:

    *ppGroupInfo = NULL;
    
    if (pGroupInfo) {
        LsaFreeGroupInfo(dwGroupInfoLevel, pGroupInfo);
    }

    goto cleanup;
}

DWORD
LsaUnmarshalGroup_1_InfoList(
    PCSTR pszMsgBuf,
    PCSTR pszHdrBuf,
    PVOID** pppGroupInfoList,
    DWORD dwNumGroups
    )
{
    DWORD dwError = 0;
    PLSA_GROUP_INFO_1* ppGroupInfoList = NULL;
    PLSA_GROUP_INFO_1 pGroupInfo = NULL;
    DWORD dwGroupInfoLevel = 1;
    DWORD iGroup = 0;
    
    dwError = LsaAllocateMemory(
                    sizeof(PLSA_GROUP_INFO_1) * dwNumGroups,
                    (PVOID*)&ppGroupInfoList
                    );
    BAIL_ON_LSA_ERROR(dwError);
    
    for (iGroup = 0; iGroup < dwNumGroups; iGroup++) {
        LSA_GROUP_1_RECORD_HEADER header;
                
        memcpy(&header, pszHdrBuf + (iGroup * sizeof(LSA_GROUP_1_RECORD_HEADER)), sizeof(LSA_GROUP_1_RECORD_HEADER));
                
        dwError = LsaUnmarshalGroup_1(pszMsgBuf,
                                      &header,
                                      &pGroupInfo
                                      );
        BAIL_ON_LSA_ERROR(dwError);
        
        *(ppGroupInfoList+iGroup) = pGroupInfo;
        pGroupInfo = NULL;
    }
    
    *pppGroupInfoList = (PVOID*)ppGroupInfoList;
    
cleanup:

    return dwError;
    
error:

    *pppGroupInfoList = NULL;
    
    if (pGroupInfo) {
        LsaFreeGroupInfo(dwGroupInfoLevel, pGroupInfo);
    }
    
    if (ppGroupInfoList) {
        LsaFreeGroupInfoList(
               dwGroupInfoLevel,
               (PVOID*)ppGroupInfoList,
               dwNumGroups
               );
    }

    goto cleanup;
}

DWORD
LsaUnmarshalGroup_1(
    PCSTR pszMsgBuf,
    PLSA_GROUP_1_RECORD_HEADER pHeader,
    PLSA_GROUP_INFO_1* ppGroupInfo
    )
{
    DWORD dwError = 0;
    PLSA_GROUP_INFO_1 pGroupInfo = NULL;
    PSTR  pszGroupMembers = NULL;
    DWORD dwGroupInfoLevel = 1;
    
    dwError = LsaAllocateMemory(
                    sizeof(LSA_GROUP_INFO_1),
                    (PVOID*)&pGroupInfo
                    );
    BAIL_ON_LSA_ERROR(dwError);
    
    pGroupInfo->gid = pHeader->gid;
    
    if (pHeader->name.length) {
        dwError = LsaAllocateMemory(
                        pHeader->name.length+1,
                        (PVOID*)&pGroupInfo->pszName
                        );
        BAIL_ON_LSA_ERROR(dwError);
        memcpy(pGroupInfo->pszName, pszMsgBuf+pHeader->name.offset, pHeader->name.length);
    }
    
    if (pHeader->passwd.length) {
        dwError = LsaAllocateMemory(
                        pHeader->passwd.length+1,
                        (PVOID*)&pGroupInfo->pszPasswd
                        );
        BAIL_ON_LSA_ERROR(dwError);
        memcpy(pGroupInfo->pszPasswd, pszMsgBuf+pHeader->passwd.offset, pHeader->passwd.length);
    }
    
    if (pHeader->sid.length) {
        dwError = LsaAllocateMemory(
                        pHeader->sid.length+1,
                        (PVOID*)&pGroupInfo->pszSid
                        );
        BAIL_ON_LSA_ERROR(dwError);
        memcpy(pGroupInfo->pszSid, pszMsgBuf+pHeader->sid.offset, pHeader->sid.length);
    }
    
    if (pHeader->gr_mem.length) {
        
        dwError = LsaAllocateMemory(
                        pHeader->gr_mem.length+1,
                        (PVOID*)&pszGroupMembers
                        );
        BAIL_ON_LSA_ERROR(dwError);
        memcpy(pszGroupMembers, pszMsgBuf+pHeader->gr_mem.offset, pHeader->gr_mem.length);
        
        dwError = LsaUnmarshalGroupMembers(
                        pszGroupMembers,
                        &pGroupInfo->ppszMembers
                        );
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    *ppGroupInfo = pGroupInfo;
    
cleanup:

    LSA_SAFE_FREE_STRING(pszGroupMembers);

    return dwError;

error:

    *ppGroupInfo = NULL;
    
    if (pGroupInfo) {
        LsaFreeGroupInfo(dwGroupInfoLevel, pGroupInfo);
    }

    goto cleanup;
}

DWORD
LsaUnmarshalGroupMembers(
    PCSTR  pszGroupMembers,
    PSTR** pppszMembers
    )
{
    DWORD dwError = 0;
    PSTR* ppszMembers = NULL;
    DWORD dwNumMembers = 0;
    DWORD iMember = 0;
    PCSTR pszMember = pszGroupMembers;
    PCSTR pszIter = pszGroupMembers;
    DWORD dwLength = 0;
    
    dwNumMembers = LsaFindNumberOfMembers(pszGroupMembers);
    if (!dwNumMembers) {
        *pppszMembers = NULL;
        goto cleanup;
    }
    
    dwError = LsaAllocateMemory(
                    sizeof(PSTR) * (dwNumMembers+1),
                    (PVOID*)&ppszMembers
                    );
    BAIL_ON_LSA_ERROR(dwError);
    
    do
    {
      dwLength = 0;
      while (pszIter && *pszIter) {
          dwLength++;
          pszIter++;
      }
      
      if (dwLength) {
         dwError = LsaStrndup(pszMember, dwLength, ppszMembers+iMember);
         BAIL_ON_LSA_ERROR(dwError);
         
         // Next member
         pszMember = ++pszIter;
         iMember++;
      }
      
    } while (dwLength);
    
    *pppszMembers = ppszMembers;
    
cleanup:

    return dwError;
    
error:

    *pppszMembers = NULL;
    
    if (ppszMembers) {
        LsaFreeStringArray(ppszMembers, dwNumMembers);
    }

    goto cleanup;
}

DWORD
LsaFindNumberOfMembers(
    PCSTR  pszGroupMembers
    )
{
    DWORD dwNumMembers = 0;
    PCSTR pszIter = pszGroupMembers;
    DWORD dwLength = 0;
    
    // stop at 2 consecutive nulls
    do
    {
        dwLength = 0;
        
        while (pszIter && *pszIter++) {
            dwLength++;
        }
        
        if (dwLength) {
           dwNumMembers++;
           pszIter++;
        }
    } while (dwLength);
    
    return dwNumMembers;
}

DWORD
LsaMarshalFindGroupByNameQuery(
    PCSTR  pszGroupName,
    DWORD  dwInfoLevel,
    PSTR   pszBuffer,
    PDWORD pdwBufLen
    )
{
    DWORD dwError = 0;
    LSA_QUERY_RECORD_BY_NAME_HEADER header = {0};
    DWORD dwRequiredBufLen = 0;
    
    dwRequiredBufLen = LsaComputeBufferSize_FindGroupByNameQuery(
                            pszGroupName,
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
    
    if (!IsNullOrEmptyString(pszGroupName)) {
       header.name.offset = sizeof(header);
       header.name.length = strlen(pszGroupName);
       memcpy(pszBuffer+header.name.offset, pszGroupName, header.name.length);
    }
    
    memcpy(pszBuffer, &header, sizeof(header));
    
cleanup:

    return dwError;
    
error:

    goto cleanup;
}

DWORD
LsaComputeBufferSize_FindGroupByNameQuery(
    PCSTR pszGroupName,
    DWORD dwInfoLevel
    )
{
    return sizeof(LSA_QUERY_RECORD_BY_NAME_HEADER) + (IsNullOrEmptyString(pszGroupName) ? 0 : strlen(pszGroupName));
}

DWORD
LsaUnmarshalFindGroupByNameQuery(
    PCSTR pszMsgBuf,
    DWORD dwMsgLen,
    PSTR* ppszGroupName,
    PDWORD pdwInfoLevel
    )
{
    DWORD dwError = 0;
    PSTR pszGroupName = NULL;
    LSA_QUERY_RECORD_BY_NAME_HEADER header = {0};
    
    if (dwMsgLen < sizeof(header)) {
        dwError = LSA_ERROR_INVALID_MESSAGE;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    memcpy(&header, pszMsgBuf, sizeof(header));
    
    if (header.name.length) {
        dwError = LsaStrndup(pszMsgBuf+header.name.offset, header.name.length, &pszGroupName);
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    *ppszGroupName = pszGroupName;
    *pdwInfoLevel = header.dwInfoLevel;
    
cleanup:

    return dwError;
    
error:

    *ppszGroupName = NULL;
    *pdwInfoLevel = 0;
    
    LSA_SAFE_FREE_STRING(pszGroupName);
    
    goto cleanup;
}

DWORD
LsaMarshalFindGroupByIdQuery(
    gid_t  gid,
    DWORD  dwInfoLevel,
    PSTR   pszBuffer,
    PDWORD pdwBufLen
    )
{
    DWORD dwError = 0;
    LSA_QUERY_RECORD_BY_ID_HEADER header = {0};
    DWORD dwRequiredBufLen = 0;
    
    dwRequiredBufLen = LsaComputeBufferSize_FindGroupByIdQuery(
                            gid,
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
    header.id = gid;
    
    memcpy(pszBuffer, &header, sizeof(header));
    
cleanup:

    return dwError;
    
error:

    goto cleanup;
}

DWORD
LsaComputeBufferSize_FindGroupByIdQuery(
    gid_t gid,
    DWORD dwInfoLevel
    )
{
    return sizeof(LSA_QUERY_RECORD_BY_ID_HEADER);
}

DWORD
LsaUnmarshalFindGroupByIdQuery(
    PCSTR  pszMsgBuf,
    DWORD  dwMsgLen,
    gid_t* pGid,
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
    
    *pGid = header.id;
    *pdwInfoLevel = header.dwInfoLevel;
    
cleanup:

    return dwError;
    
error:

    *pGid = 0;
    *pdwInfoLevel = 0;

    goto cleanup;
}

DWORD
LsaMarshalDeleteGroupByIdQuery(
    gid_t  gid,
    PSTR   pszBuffer,
    PDWORD pdwBufLen
    )
{
    DWORD dwError = 0;
    DWORD dwBufLenRequired = sizeof(gid_t);
    
    if (!pszBuffer) {
        *pdwBufLen = dwBufLenRequired;
        goto cleanup;
    }
    
    if (*pdwBufLen < dwBufLenRequired) {
        dwError = LSA_ERROR_INSUFFICIENT_BUFFER;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    memcpy(pszBuffer, &gid, sizeof(gid_t));
    
cleanup:

    return dwError;
    
error:

    goto cleanup;
}

DWORD
LsaUnmarshalDeleteGroupByIdQuery(
    PCSTR  pszMsgBuf,
    DWORD  dwMsgLen,
    gid_t* pGid
    )
{
    DWORD dwError = 0;
    
    if (dwMsgLen < sizeof(gid_t)) {
        dwError = LSA_ERROR_INVALID_MESSAGE;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    memcpy(pGid, pszMsgBuf, sizeof(gid_t));
    
cleanup:

    return dwError;
    
error:

    *pGid = 0;
    
    goto cleanup;
}

DWORD
LsaMarshalGetGroupsForUserQuery(
    uid_t  uid,
    DWORD  dwInfoLevel,
    PSTR   pszBuffer,
    PDWORD pdwBufLen
    )
{
    DWORD dwError = 0;
    LSA_QUERY_RECORD_BY_ID_HEADER header = {0};
    DWORD dwRequiredBufLen = 0;
    
    dwRequiredBufLen = LsaComputeBufferSize_GetGroupsForUserQuery(
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
LsaComputeBufferSize_GetGroupsForUserQuery(
    uid_t uid,
    DWORD dwInfoLevel
    )
{
    return sizeof(LSA_QUERY_RECORD_BY_ID_HEADER);
}

DWORD
LsaUnmarshalGetGroupsForUserQuery(
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

