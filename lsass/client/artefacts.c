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
 *        groups.c
 *
 * Abstract:
 * 
 *        Likewise Security and Authentication Subsystem (LSASS)
 * 
 *        NSSArtefact Lookup and Management API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 */
#include "client.h"


LSASS_API
DWORD
LsaBeginEnumNSSArtefacts(
    HANDLE  hLsaConnection,
    DWORD   dwInfoLevel,
    DWORD   dwMapType,
    DWORD   dwMaxNumNSSArtefacts,
    PHANDLE phResume
    )
{
    DWORD dwError = 0;
    PLSA_ENUM_NSS_ARTEFACTS_INFO pInfo = NULL;
    PLSAMESSAGE pMessage = NULL;
    DWORD   dwMsgLen = 0;
    PSTR    pszError = NULL;
    PSTR    pszGUID = NULL;
    
    BAIL_ON_INVALID_HANDLE(hLsaConnection);
    BAIL_ON_INVALID_POINTER(phResume);
    
    dwError = LsaValidateNSSArtefactInfoLevel(dwInfoLevel);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaMarshalBeginEnumNSSArtefactRecordsQuery(
                    dwInfoLevel,
                    dwMapType,
                    dwMaxNumNSSArtefacts,
                    NULL,
                    &dwMsgLen);
    BAIL_ON_LSA_ERROR(dwError);
        
    dwError = LsaBuildMessage(
                LSA_Q_BEGIN_ENUM_NSS_ARTEFACTS,
                dwMsgLen,
                1,
                1,
                &pMessage);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaMarshalBeginEnumNSSArtefactRecordsQuery(
                    dwInfoLevel,
                    dwMapType,
                    dwMaxNumNSSArtefacts,
                    pMessage->pData,
                    &dwMsgLen);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaSendMessage(hLsaConnection, pMessage);
    BAIL_ON_LSA_ERROR(dwError);
    
    LSA_SAFE_FREE_MESSAGE(pMessage);
    
    dwError = LsaGetNextMessage(hLsaConnection, &pMessage);
    BAIL_ON_LSA_ERROR(dwError);
    
    switch (pMessage->header.messageType) {
        case LSA_R_BEGIN_ENUM_NSS_ARTEFACTS:
        {
            dwError = LsaUnmarshalEnumRecordsToken(
                                pMessage->pData,
                                pMessage->header.messageLength,
                                &pszGUID);
            BAIL_ON_LSA_ERROR(dwError);
            
            break;
        }
        case LSA_ERROR:
        {
            DWORD dwSrvError = 0;
            
            dwError = LsaUnmarshalError(
                                pMessage->pData,
                                pMessage->header.messageLength,
                                &dwSrvError,
                                &pszError
                                );
            BAIL_ON_LSA_ERROR(dwError);
            dwError = dwSrvError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        }
        default:
        {
            dwError = LSA_ERROR_UNEXPECTED_MESSAGE;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }
    
    dwError = LsaAllocateMemory(
                    sizeof(LSA_ENUM_NSS_ARTEFACTS_INFO),
                    (PVOID*)&pInfo);
    BAIL_ON_LSA_ERROR(dwError);
    
    pInfo->dwNSSArtefactInfoLevel = dwInfoLevel;
    pInfo->dwNumMaxNSSArtefacts = dwMaxNumNSSArtefacts;
    pInfo->pszGUID = pszGUID;
    
    *phResume = (HANDLE)pInfo;
    
cleanup:

    LSA_SAFE_FREE_MESSAGE(pMessage);

    return dwError;
    
error:

    *phResume = (HANDLE)NULL;

    LSA_SAFE_FREE_STRING(pszGUID);

    goto cleanup;
}

LSASS_API
DWORD
LsaEnumNSSArtefacts(
    HANDLE  hLsaConnection,
    HANDLE  hResume,
    PDWORD  pdwNumNSSArtefactsFound,
    PVOID** pppNSSArtefactInfoList
    )
{
    DWORD dwError = 0;
    PLSAMESSAGE pMessage = NULL;
    DWORD   dwMsgLen = 0;
    PSTR    pszError = NULL;
    PVOID*  ppNSSArtefactInfoList = NULL;
    DWORD   dwNumNSSArtefactsFound = 0;
    DWORD   dwNSSArtefactInfoLevel = 0;
    PLSA_ENUM_NSS_ARTEFACTS_INFO pInfo = (PLSA_ENUM_NSS_ARTEFACTS_INFO)hResume;
    
    BAIL_ON_INVALID_HANDLE(hLsaConnection);
    BAIL_ON_INVALID_HANDLE(hResume);
    BAIL_ON_INVALID_POINTER(pdwNumNSSArtefactsFound);
    BAIL_ON_INVALID_POINTER(pppNSSArtefactInfoList);
    
    dwNSSArtefactInfoLevel = pInfo->dwNSSArtefactInfoLevel;

    dwError = LsaMarshalEnumRecordsToken(
                    pInfo->pszGUID,
                    NULL,
                    &dwMsgLen);
    BAIL_ON_LSA_ERROR(dwError);
        
    dwError = LsaBuildMessage(
                LSA_Q_ENUM_NSS_ARTEFACTS,
                dwMsgLen,
                1,
                1,
                &pMessage);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaMarshalEnumRecordsToken(
                    pInfo->pszGUID,
                    pMessage->pData,
                    &dwMsgLen);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaSendMessage(hLsaConnection, pMessage);
    BAIL_ON_LSA_ERROR(dwError);
    
    LSA_SAFE_FREE_MESSAGE(pMessage);
    
    dwError = LsaGetNextMessage(hLsaConnection, &pMessage);
    BAIL_ON_LSA_ERROR(dwError);
    
    switch (pMessage->header.messageType) {
        case LSA_R_ENUM_NSS_ARTEFACTS:
        {
            dwError = LsaUnmarshalNSSArtefactInfoList(
                                pMessage->pData,
                                pMessage->header.messageLength,
                                &dwNSSArtefactInfoLevel,
                                &ppNSSArtefactInfoList,
                                &dwNumNSSArtefactsFound
                                );
            BAIL_ON_LSA_ERROR(dwError);
            break;
        }
        case LSA_ERROR:
        {
            DWORD dwSrvError = 0;
            
            dwError = LsaUnmarshalError(
                                pMessage->pData,
                                pMessage->header.messageLength,
                                &dwSrvError,
                                &pszError
                                );
            BAIL_ON_LSA_ERROR(dwError);
            dwError = dwSrvError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        }
        default:
        {
            dwError = LSA_ERROR_UNEXPECTED_MESSAGE;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }
    
    *pdwNumNSSArtefactsFound = dwNumNSSArtefactsFound;
    *pppNSSArtefactInfoList = ppNSSArtefactInfoList;

cleanup:

    LSA_SAFE_FREE_MESSAGE(pMessage);
    LSA_SAFE_FREE_STRING(pszError);

    return dwError;
        
error:

    if (ppNSSArtefactInfoList) {
        LsaFreeNSSArtefactInfoList(dwNSSArtefactInfoLevel, ppNSSArtefactInfoList, dwNumNSSArtefactsFound);
    }
    
    if (pppNSSArtefactInfoList) {
        *pppNSSArtefactInfoList = NULL;
    }
    
    if (pdwNumNSSArtefactsFound) {
        *pdwNumNSSArtefactsFound = 0;
    }

    goto cleanup;
}

LSASS_API
DWORD
LsaEndEnumNSSArtefacts(
    HANDLE hLsaConnection,
    HANDLE hResume
    )
{
    DWORD dwError = 0;
    PLSA_ENUM_NSS_ARTEFACTS_INFO pInfo = (PLSA_ENUM_NSS_ARTEFACTS_INFO)hResume;
    PLSAMESSAGE pMessage = NULL;
    DWORD   dwMsgLen = 0;
    PSTR    pszError = NULL;
    
    BAIL_ON_INVALID_HANDLE(hLsaConnection);
    BAIL_ON_INVALID_HANDLE(hResume);
    
    dwError = LsaMarshalEnumRecordsToken(
                    pInfo->pszGUID,
                    NULL,
                    &dwMsgLen);
    BAIL_ON_LSA_ERROR(dwError);
        
    dwError = LsaBuildMessage(
                LSA_Q_END_ENUM_NSS_ARTEFACTS,
                dwMsgLen,
                1,
                1,
                &pMessage);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaMarshalEnumRecordsToken(
                    pInfo->pszGUID,
                    pMessage->pData,
                    &dwMsgLen);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaSendMessage(hLsaConnection, pMessage);
    BAIL_ON_LSA_ERROR(dwError);
    
    LSA_SAFE_FREE_MESSAGE(pMessage);
    
    dwError = LsaGetNextMessage(hLsaConnection, &pMessage);
    BAIL_ON_LSA_ERROR(dwError);
    
    switch (pMessage->header.messageType) {
        case LSA_R_END_ENUM_NSS_ARTEFACTS:
        {
            // Success
            break;
        }
        case LSA_ERROR:
        {
            DWORD dwSrvError = 0;
            
            dwError = LsaUnmarshalError(
                                pMessage->pData,
                                pMessage->header.messageLength,
                                &dwSrvError,
                                &pszError
                                );
            BAIL_ON_LSA_ERROR(dwError);
            dwError = dwSrvError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        }
        default:
        {
            dwError = LSA_ERROR_UNEXPECTED_MESSAGE;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }
    
    LsaFreeEnumNSSArtefactsInfo(pInfo);
    
cleanup:

    LSA_SAFE_FREE_MESSAGE(pMessage);

    return dwError;
    
error:

    goto cleanup;
}


DWORD
LsaValidateNSSArtefactInfoLevel(
    DWORD dwNSSArtefactInfoLevel
    )
{
    return ((dwNSSArtefactInfoLevel != 0) ? LSA_ERROR_INVALID_NSS_ARTEFACT_INFO_LEVEL : 0);
}

VOID
LsaFreeEnumNSSArtefactsInfo(
    PLSA_ENUM_NSS_ARTEFACTS_INFO pInfo
    )
{
    LSA_SAFE_FREE_STRING(pInfo->pszGUID);
    LsaFreeMemory(pInfo);
}
