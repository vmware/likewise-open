/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
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
 *        NSSArtefact Lookup and Management (Server)
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "api.h"


DWORD
LsaSrvBeginEnumNSSArtefacts(
    HANDLE hServer,
    DWORD  dwMapType,
    DWORD  dwNSSArtefactInfoLevel,
    DWORD  dwMaxNumNSSArtefacts,
    PSTR*  ppszGUID
    )
{
    DWORD dwError = 0;
    PLSA_SRV_RECORD_ENUM_STATE pEnumState = NULL;
    PSTR pszGUID = NULL;
    
    dwError = LsaSrvAddNSSArtefactEnumState(
                    hServer,
                    dwMapType,
                    dwNSSArtefactInfoLevel,
                    dwMaxNumNSSArtefacts,
                    &pEnumState);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaAllocateString(pEnumState->pszGUID, &pszGUID);
    BAIL_ON_LSA_ERROR(dwError);
    
    *ppszGUID = pszGUID;
    
cleanup:

    return dwError;
    
error:

    *ppszGUID = NULL;

    goto cleanup;
}

DWORD
LsaSrvEnumNSSArtefacts(
    HANDLE  hServer,
    PCSTR   pszGUID,
    PDWORD  pdwNSSArtefactInfoLevel,
    PVOID** pppNSSArtefactInfoList,
    PDWORD  pdwNumNSSArtefactsFound
    )
{
    DWORD dwError = 0;
    PLSA_SRV_RECORD_ENUM_STATE pEnumState = NULL;
    PVOID* ppNSSArtefactInfoList_accumulate = NULL;
    DWORD  dwTotalNumNSSArtefactsFound = 0;
    PVOID* ppNSSArtefactInfoList = NULL;
    DWORD  dwNumNSSArtefactsFound = 0;
    DWORD  dwNumNSSArtefactsRemaining = 0;
    DWORD  dwNSSArtefactInfoLevel = 0;
    DWORD  dwMapType = 0;
    
    pEnumState = LsaSrvFindNSSArtefactEnumState(hServer, pszGUID);
    if (!pEnumState) {
        dwError = LSA_ERROR_INTERNAL;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    dwNSSArtefactInfoLevel = pEnumState->dwInfoLevel;
    dwNumNSSArtefactsRemaining = pEnumState->dwNumMaxRecords;
    dwMapType = pEnumState->dwMapType;
    
    while (dwNumNSSArtefactsRemaining &&
           pEnumState->pCurProviderState)
    {
        PLSA_SRV_PROVIDER_STATE pProviderState = pEnumState->pCurProviderState;
        PLSA_AUTH_PROVIDER pProvider = pProviderState->pProvider;
        HANDLE hProvider = pProviderState->hProvider;
        HANDLE hResume = pProviderState->hResume;

        dwNumNSSArtefactsFound = 0;
        
        
        dwError = pProvider->pFnTable->pfnEnumNSSArtefacts(
                        hProvider,
                        hResume,
                        dwNumNSSArtefactsRemaining,
                        &dwNumNSSArtefactsFound,
                        &ppNSSArtefactInfoList);
                       
        
        if (dwError) {
           if (dwError != LSA_ERROR_NO_MORE_GROUPS) {
              BAIL_ON_LSA_ERROR(dwError);
           }
        }
        
        dwNumNSSArtefactsRemaining -= dwNumNSSArtefactsFound;        
        
        if (dwNumNSSArtefactsRemaining) {
           pEnumState->pCurProviderState = pEnumState->pCurProviderState->pNext;
           if (dwError == LSA_ERROR_NO_MORE_GROUPS){ 
             dwError = 0;           
           }
        }
        
        dwError = LsaCoalesceNSSArtefactInfoList(
                        &ppNSSArtefactInfoList,
                        &dwNumNSSArtefactsFound,
                        &ppNSSArtefactInfoList_accumulate,
                        &dwTotalNumNSSArtefactsFound);
        BAIL_ON_LSA_ERROR(dwError);
    }
   
    *pdwNSSArtefactInfoLevel = dwNSSArtefactInfoLevel;
    *pppNSSArtefactInfoList = ppNSSArtefactInfoList_accumulate;
    *pdwNumNSSArtefactsFound = dwTotalNumNSSArtefactsFound;
    
cleanup:
    
    return(dwError);

error:
    
    *pdwNSSArtefactInfoLevel = 0;
    *pppNSSArtefactInfoList = NULL;
    *pdwNumNSSArtefactsFound = 0;
    
    
    if (ppNSSArtefactInfoList) {
        LsaFreeNSSArtefactInfoList(dwNSSArtefactInfoLevel, ppNSSArtefactInfoList, dwNumNSSArtefactsFound);
    }
    
    if (ppNSSArtefactInfoList_accumulate) {
        LsaFreeNSSArtefactInfoList(dwNSSArtefactInfoLevel, ppNSSArtefactInfoList_accumulate, dwTotalNumNSSArtefactsFound);
    }

    goto cleanup;
}

DWORD
LsaSrvEndEnumNSSArtefacts(
    HANDLE hServer,
    PSTR   pszGUID
    )
{
    DWORD dwError = 0;
    PLSA_SRV_RECORD_ENUM_STATE pEnumState = NULL;
    PLSA_SRV_PROVIDER_STATE pProviderState = NULL;
    
    pEnumState = LsaSrvFindNSSArtefactEnumState(hServer, pszGUID);
    if (!pEnumState) {
        dwError = LSA_ERROR_INTERNAL;
        BAIL_ON_LSA_ERROR(dwError);
    }

    for (pProviderState = pEnumState->pProviderStateList;
         pProviderState;
         pProviderState = pProviderState->pNext)
    {
        PLSA_AUTH_PROVIDER pProvider = pProviderState->pProvider;
        if (pProvider) {
           HANDLE hProvider = pProviderState->hProvider;
           pProvider->pFnTable->pfnEndEnumNSSArtefacts(
                                       hProvider,
                                       pszGUID);
        }
    }
        
    LsaSrvFreeNSSArtefactEnumState(
                        hServer,
                        pszGUID);

cleanup:

    return dwError;
    
error:

    goto cleanup;
}

