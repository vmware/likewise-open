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
 *        Marshal/Unmarshal API for Messages related to NSSArtefacts
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *
 */
#include "ipc.h"

DWORD
LsaMarshalNSSArtefactInfoList(
    PVOID* ppNSSArtefactInfoList,
    DWORD  dwInfoLevel,
    DWORD  dwNumNSSArtefacts,
    PSTR   pszBuffer,
    PDWORD pdwBufLen
    )
{
    DWORD dwError = 0;
    DWORD dwRequiredBufLen = 0;
    DWORD dwBytesWritten = 0;
    LSA_USER_NSS_ARTEFACT_RECORD_PREAMBLE header;

    dwError = LsaComputeNSSArtefactBufferSize(
                    ppNSSArtefactInfoList,
                    dwInfoLevel,
                    dwNumNSSArtefacts,
                    &dwRequiredBufLen);
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
    header.dwNumRecords = dwNumNSSArtefacts;

    memcpy(pszBuffer, &header, sizeof(header));
    dwBytesWritten = sizeof(header);

    switch (dwInfoLevel)
    {
        case 0:
        {
            DWORD dwTmpBytesWritten = 0;

            dwError = LsaMarshalNSSArtefact_0_InfoList(
                            ppNSSArtefactInfoList,
                            dwNumNSSArtefacts,
                            dwBytesWritten,
                            pszBuffer,
                            &dwTmpBytesWritten);
            BAIL_ON_LSA_ERROR(dwError);

            dwBytesWritten += dwTmpBytesWritten;

            break;
        }
        default:
        {
            // We would have caught this when computing the
            // size. Adding this here as well for completeness
            dwError = LSA_ERROR_UNSUPPORTED_NSS_ARTEFACT_LEVEL;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaComputeNSSArtefactBufferSize(
    PVOID* ppNSSArtefactInfoList,
    DWORD  dwInfoLevel,
    DWORD  dwNumNSSArtefacts,
    PDWORD pdwBufLen
    )
{
    DWORD dwError = 0;
    DWORD dwTotalBufLen = sizeof(LSA_USER_NSS_ARTEFACT_RECORD_PREAMBLE);

    if (dwNumNSSArtefacts) {

        switch(dwInfoLevel)
        {
            case 0:
            {
                dwTotalBufLen += LsaComputeBufferSize_NSSArtefact0(
                                    ppNSSArtefactInfoList,
                                    dwNumNSSArtefacts
                                    );
                break;
            }

            default:
            {
                dwError = LSA_ERROR_UNSUPPORTED_NSS_ARTEFACT_LEVEL;
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
LsaComputeBufferSize_NSSArtefact0(
    PVOID* ppNSSArtefactInfoList,
    DWORD  dwNumNSSArtefacts
    )
{
    DWORD dwBufLen = 0;
    DWORD iNSSArtefact = 0;
    PLSA_NSS_ARTEFACT_INFO_0 pNSSArtefactInfo = NULL;

    for (iNSSArtefact = 0; iNSSArtefact < dwNumNSSArtefacts; iNSSArtefact++)
    {
        pNSSArtefactInfo = (PLSA_NSS_ARTEFACT_INFO_0)*(ppNSSArtefactInfoList+iNSSArtefact);
        dwBufLen += sizeof(LSA_NSS_ARTEFACT_0_RECORD_HEADER);

        if (!IsNullOrEmptyString(pNSSArtefactInfo->pszName)) {
            dwBufLen += strlen(pNSSArtefactInfo->pszName);
        }

        if (!IsNullOrEmptyString(pNSSArtefactInfo->pszValue)) {
            dwBufLen += strlen(pNSSArtefactInfo->pszValue);
        }
    }

    return dwBufLen;
}

/*
 * PVOID* ppNSSArtefactInfoList
 *        Array of NSSArtefact Info structures
 * DWORD  dwNumNSSArtefacts
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
LsaMarshalNSSArtefact_0_InfoList(
    PVOID* ppNSSArtefactInfoList,
    DWORD  dwNumNSSArtefacts,
    DWORD  dwBeginOffset,
    PSTR   pszBuffer,
    PDWORD pdwDataBytesWritten
    )
{
    DWORD dwError = 0;
    // This is where we are from the beginning of the buffer
    DWORD iNSSArtefact = 0;
    // This is where we will start writing strings
    DWORD dwCurrentDataOffset = dwBeginOffset + (sizeof(LSA_NSS_ARTEFACT_0_RECORD_HEADER) * dwNumNSSArtefacts);
    DWORD dwTotalDataBytesWritten = 0;

    for (iNSSArtefact = 0; iNSSArtefact < dwNumNSSArtefacts; iNSSArtefact++)
    {
        DWORD dwDataBytesWritten = 0;
        PSTR  pszHeaderOffset = pszBuffer + dwBeginOffset + sizeof(LSA_NSS_ARTEFACT_0_RECORD_HEADER) * iNSSArtefact;
        PSTR  pszDataOffset = pszBuffer + dwCurrentDataOffset;

        dwError = LsaMarshalNSSArtefact_0(
                        (PLSA_NSS_ARTEFACT_INFO_0)*(ppNSSArtefactInfoList+iNSSArtefact),
                        pszHeaderOffset,
                        pszDataOffset,
                        dwCurrentDataOffset,
                        &dwDataBytesWritten);
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
LsaMarshalNSSArtefact_0(
    PLSA_NSS_ARTEFACT_INFO_0 pNSSArtefactInfo,
    PSTR   pszHeaderBuffer,
    PSTR   pszDataBuffer,
    DWORD  dwDataBeginOffset,
    PDWORD pdwDataBytesWritten
    )
{
    DWORD dwError = 0;
    DWORD dwGlobalOffset = dwDataBeginOffset;
    DWORD dwOffset = 0;
    LSA_NSS_ARTEFACT_0_RECORD_HEADER header;
    DWORD dwDataBytesWritten = 0;

    // Prepare and write the header
    memset(&header, 0, sizeof(header));

    if (!IsNullOrEmptyString(pNSSArtefactInfo->pszName)) {
       header.name.length = strlen(pNSSArtefactInfo->pszName);
       // We always indicate the offset from the beginning of the buffer
       header.name.offset = dwGlobalOffset;
       memcpy(pszDataBuffer+dwOffset, pNSSArtefactInfo->pszName, header.name.length);
       dwOffset += header.name.length;
       dwGlobalOffset += header.name.length;
       dwDataBytesWritten += header.name.length;
    }

    if (!IsNullOrEmptyString(pNSSArtefactInfo->pszValue)) {
       header.value.length = strlen(pNSSArtefactInfo->pszValue);
       // We always indicate the offset from the beginning of the buffer
       header.value.offset = dwGlobalOffset;
       memcpy(pszDataBuffer+dwOffset, pNSSArtefactInfo->pszValue, header.value.length);
       dwOffset += header.value.length;
       dwGlobalOffset += header.value.length;
       dwDataBytesWritten += header.value.length;
    }

    memcpy(pszHeaderBuffer, &header, sizeof(header));

    *pdwDataBytesWritten = dwDataBytesWritten;

    return dwError;
}

DWORD
LsaUnmarshalNSSArtefactInfoList(
    PCSTR   pszMsgBuf,
    DWORD   dwMsgLen,
    PDWORD  pdwInfoLevel,
    PVOID** pppNSSArtefactInfoList,
    PDWORD  pdwNumNSSArtefacts
    )
{
    DWORD dwError = 0;
    PVOID* ppNSSArtefactInfoList = NULL;
    LSA_USER_NSS_ARTEFACT_RECORD_PREAMBLE header;
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

            dwError = LsaUnmarshalNSSArtefact_0_InfoList(
                            pszMsgBuf,
                            pszMsgBuf+sizeof(header),
                            &ppNSSArtefactInfoList,
                            header.dwNumRecords);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        }

        default:
        {
            dwError = LSA_ERROR_UNSUPPORTED_NSS_ARTEFACT_LEVEL;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    *pppNSSArtefactInfoList = ppNSSArtefactInfoList;
    *pdwNumNSSArtefacts = header.dwNumRecords;
    *pdwInfoLevel = dwInfoLevel;

cleanup:

    return dwError;

error:

    *pppNSSArtefactInfoList = NULL;
    *pdwNumNSSArtefacts = 0;
    *pdwInfoLevel = 0;

    if (ppNSSArtefactInfoList) {
        LsaFreeNSSArtefactInfoList(
                  header.dwInfoLevel,
                  (PVOID*)ppNSSArtefactInfoList,
                  header.dwNumRecords);
    }

    goto cleanup;
}

DWORD
LsaUnmarshalNSSArtefact_0_InfoList(
    PCSTR pszMsgBuf,
    PCSTR pszHdrBuf,
    PVOID** pppNSSArtefactInfoList,
    DWORD dwNumNSSArtefacts
    )
{
    DWORD dwError = 0;
    PLSA_NSS_ARTEFACT_INFO_0* ppNSSArtefactInfoList = NULL;
    PLSA_NSS_ARTEFACT_INFO_0 pNSSArtefactInfo = NULL;
    DWORD dwNSSArtefactInfoLevel = 0;
    DWORD iNSSArtefact = 0;

    dwError = LsaAllocateMemory(
                    sizeof(PLSA_NSS_ARTEFACT_INFO_0) * dwNumNSSArtefacts,
                    (PVOID*)&ppNSSArtefactInfoList);
    BAIL_ON_LSA_ERROR(dwError);

    for (iNSSArtefact = 0; iNSSArtefact < dwNumNSSArtefacts; iNSSArtefact++) {
        LSA_NSS_ARTEFACT_0_RECORD_HEADER header;

        memcpy(&header,
               pszHdrBuf + (iNSSArtefact * sizeof(LSA_NSS_ARTEFACT_0_RECORD_HEADER)),
               sizeof(LSA_NSS_ARTEFACT_0_RECORD_HEADER));

        dwError = LsaUnmarshalNSSArtefact_0(
                      pszMsgBuf,
                      &header,
                      &pNSSArtefactInfo);
        BAIL_ON_LSA_ERROR(dwError);

        *(ppNSSArtefactInfoList+iNSSArtefact) = pNSSArtefactInfo;
        pNSSArtefactInfo = NULL;
    }

    *pppNSSArtefactInfoList = (PVOID*)ppNSSArtefactInfoList;

cleanup:

    return dwError;

error:

    *pppNSSArtefactInfoList = NULL;

    if (pNSSArtefactInfo) {
        LsaFreeNSSArtefactInfo(dwNSSArtefactInfoLevel, pNSSArtefactInfo);
    }

    if (ppNSSArtefactInfoList) {
        LsaFreeNSSArtefactInfoList(
               dwNSSArtefactInfoLevel,
               (PVOID*)ppNSSArtefactInfoList,
               dwNumNSSArtefacts
               );
    }

    goto cleanup;
}

DWORD
LsaUnmarshalNSSArtefact_0(
    PCSTR pszMsgBuf,
    PLSA_NSS_ARTEFACT_0_RECORD_HEADER pHeader,
    PLSA_NSS_ARTEFACT_INFO_0* ppNSSArtefactInfo
    )
{
    DWORD dwError = 0;
    PLSA_NSS_ARTEFACT_INFO_0 pNSSArtefactInfo = NULL;
    DWORD dwNSSArtefactInfoLevel = 0;

    dwError = LsaAllocateMemory(
                    sizeof(LSA_NSS_ARTEFACT_INFO_0),
                    (PVOID*)&pNSSArtefactInfo);
    BAIL_ON_LSA_ERROR(dwError);

    if (pHeader->name.length) {

        dwError = LsaAllocateMemory(
                      pHeader->name.length+1,
                      (PVOID*)&pNSSArtefactInfo->pszName);
        BAIL_ON_LSA_ERROR(dwError);

        memcpy(pNSSArtefactInfo->pszName, pszMsgBuf+pHeader->name.offset, pHeader->name.length);
    }

    if (pHeader->value.length) {

        dwError = LsaAllocateMemory(
                      pHeader->value.length+1,
                      (PVOID*)&pNSSArtefactInfo->pszValue);
        BAIL_ON_LSA_ERROR(dwError);

        memcpy(pNSSArtefactInfo->pszValue, pszMsgBuf+pHeader->value.offset, pHeader->value.length);
    }

    *ppNSSArtefactInfo = pNSSArtefactInfo;

cleanup:

    return dwError;

error:

    *ppNSSArtefactInfo = NULL;

    if (pNSSArtefactInfo) {
        LsaFreeNSSArtefactInfo(dwNSSArtefactInfoLevel, pNSSArtefactInfo);
    }

    goto cleanup;
}

DWORD
LsaMarshalBeginEnumNSSArtefactRecordsQuery(
    DWORD  dwInfoLevel,
    PCSTR  pszMapName,
    LSA_NIS_MAP_QUERY_FLAGS dwFlags,
    DWORD  dwNumMaxRecords,
    PSTR   pszBuffer,
    PDWORD pdwBufLen
    )
{
    DWORD dwError = 0;
    LSA_BEGIN_ENUM_NSS_ARTEFACT_RECORDS_HEADER header = {0};
    DWORD dwRequiredLength = sizeof(header);

    BAIL_ON_INVALID_STRING(pszMapName);

    dwRequiredLength += strlen(pszMapName);

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
    header.dwFlags = dwFlags;
    header.mapName.length = strlen(pszMapName);
    header.mapName.offset = sizeof(header);

    memcpy(pszBuffer, &header, sizeof(header));
    memcpy(pszBuffer + header.mapName.offset, pszMapName, header.mapName.length);

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaUnmarshalBeginEnumNSSArtefactRecordsQuery(
    PCSTR  pszMsgBuf,
    DWORD  dwMsgLen,
    PDWORD pdwInfoLevel,
    LSA_NIS_MAP_QUERY_FLAGS* pdwFlags,
    PSTR*  ppszMapName,
    PDWORD pdwNumMaxRecords
    )
{
    DWORD dwError = 0;
    LSA_BEGIN_ENUM_NSS_ARTEFACT_RECORDS_HEADER header = {0};
    PSTR pszMapName = NULL;

    if (dwMsgLen < sizeof(header)) {
        dwError = LSA_ERROR_INVALID_MESSAGE;
        BAIL_ON_LSA_ERROR(dwError);
    }

    memcpy(&header, pszMsgBuf, sizeof(header));

    dwError = LsaStrndup(
                   pszMsgBuf + header.mapName.offset,
                   header.mapName.length,
                   &pszMapName);
    BAIL_ON_LSA_ERROR(dwError);

    *pdwInfoLevel = header.dwInfoLevel;
    *pdwFlags = header.dwFlags;
    *ppszMapName = pszMapName;
    *pdwNumMaxRecords = header.dwNumMaxRecords;

cleanup:

    return dwError;

error:

    LSA_SAFE_FREE_STRING(pszMapName);

    goto cleanup;
}

DWORD
LsaMarshalFindNSSArtefactByKeyQuery(
    DWORD  dwInfoLevel,
    PCSTR  pszKeyName,
    PCSTR  pszMapName,
    LSA_NIS_MAP_QUERY_FLAGS dwFlags,
    PSTR   pszBuffer,
    PDWORD pdwBufLen
    )
{
    DWORD dwError = 0;
    LSA_FIND_NSS_ARTEFACT_BY_KEY_HEADER header = {0};
    DWORD dwRequiredLength = sizeof(header);
    DWORD dwOffset = 0;

    BAIL_ON_INVALID_STRING(pszMapName);
    BAIL_ON_INVALID_STRING(pszKeyName);

    dwRequiredLength += strlen(pszMapName);
    dwRequiredLength += strlen(pszKeyName);

    if (!pszBuffer) {
        *pdwBufLen = dwRequiredLength;
        goto cleanup;
    }

    if (*pdwBufLen < dwRequiredLength) {
        dwError = LSA_ERROR_INSUFFICIENT_BUFFER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    header.dwInfoLevel = dwInfoLevel;
    header.dwFlags = dwFlags;

    dwOffset = sizeof(header);

    header.mapName.length = strlen(pszMapName);
    header.mapName.offset = dwOffset;

    memcpy(pszBuffer + dwOffset, pszMapName, header.mapName.length);
    dwOffset += header.mapName.length;

    header.keyName.length = strlen(pszKeyName);
    header.keyName.offset = dwOffset;

    memcpy(pszBuffer + dwOffset, pszKeyName, header.keyName.length);

    memcpy(pszBuffer, &header, sizeof(header));

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaUnmarshalFindNSSArtefactByKeyQuery(
    PCSTR  pszMsgBuf,
    DWORD  dwMsgLen,
    PDWORD pdwInfoLevel,
    LSA_NIS_MAP_QUERY_FLAGS* pdwFlags,
    PSTR*  ppszMapName,
    PSTR*  ppszKeyName
    )
{
    DWORD dwError = 0;
    LSA_FIND_NSS_ARTEFACT_BY_KEY_HEADER header = {0};
    PSTR pszMapName = NULL;
    PSTR pszKeyName = NULL;

    if (dwMsgLen < sizeof(header)) {
        dwError = LSA_ERROR_INVALID_MESSAGE;
        BAIL_ON_LSA_ERROR(dwError);
    }

    memcpy(&header, pszMsgBuf, sizeof(header));

    dwError = LsaStrndup(
                   pszMsgBuf + header.mapName.offset,
                   header.mapName.length,
                   &pszMapName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaStrndup(
                   pszMsgBuf + header.keyName.offset,
                   header.keyName.length,
                   &pszKeyName);
    BAIL_ON_LSA_ERROR(dwError);

    *pdwInfoLevel = header.dwInfoLevel;
    *pdwFlags = header.dwFlags;
    *ppszMapName = pszMapName;
    *ppszKeyName = pszKeyName;

cleanup:

    return dwError;

error:

    LSA_SAFE_FREE_STRING(pszMapName);
    LSA_SAFE_FREE_STRING(pszKeyName);

    goto cleanup;
}
