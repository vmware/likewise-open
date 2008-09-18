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
 *        enum-state.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 * 
 *        Local Authentication Provider
 * 
 *        Enumeration State Utilities
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "localprovider.h"

DWORD
LsaProviderLocal_AddUserState(
    HANDLE  hProvider,
    PCSTR   pszGUID,
    DWORD   dwInfoLevel,
    PLOCAL_PROVIDER_ENUM_STATE* ppEnumState
    )
{
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;
    
    return LsaProviderLocal_AddEnumState(
                &pContext->pUserEnumStateList,
                pszGUID,
                dwInfoLevel,
                ppEnumState);
}

PLOCAL_PROVIDER_ENUM_STATE
LsaProviderLocal_FindUserState(
    HANDLE hProvider,
    PCSTR  pszGUID
    )
{
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;
    
    return LsaProviderLocal_FindEnumState(
                    pContext->pUserEnumStateList,
                    pszGUID);
}

VOID
LsaProviderLocal_FreeUserState(
    HANDLE hProvider,
    PCSTR  pszGUID
    )
{
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;
    
    return LsaProviderLocal_FreeEnumState(&pContext->pUserEnumStateList, pszGUID);
}

DWORD
LsaProviderLocal_AddGroupState(
    HANDLE hProvider,
    PCSTR  pszGUID,
    DWORD  dwInfoLevel,
    PLOCAL_PROVIDER_ENUM_STATE* ppEnumState
    )
{
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;
    
    return LsaProviderLocal_AddEnumState(
                    &pContext->pGroupEnumStateList,
                    pszGUID,
                    dwInfoLevel,
                    ppEnumState);
}

DWORD
LsaProviderLocal_AddEnumState(
    PLOCAL_PROVIDER_ENUM_STATE* ppStateList,
    PCSTR pszGUID,
    DWORD dwInfoLevel,
    PLOCAL_PROVIDER_ENUM_STATE* ppNewEnumState
    )
{
    DWORD dwError = 0;
    PLOCAL_PROVIDER_ENUM_STATE pEnumState = NULL;
    BOOLEAN bFreeState = FALSE;
    
    if (!(pEnumState = LsaProviderLocal_FindEnumState(*ppStateList, pszGUID))) {
        dwError = LsaAllocateMemory(
                        sizeof(LOCAL_PROVIDER_ENUM_STATE),
                        (PVOID*)&pEnumState);
        BAIL_ON_LSA_ERROR(dwError);
        bFreeState = TRUE;
        
        dwError = LsaAllocateString(pszGUID, &pEnumState->pszGUID);
        BAIL_ON_LSA_ERROR(dwError);
        
        pEnumState->dwInfoLevel = dwInfoLevel;
        
        pEnumState->pNext = *ppStateList;
        *ppStateList = pEnumState;
        
        bFreeState = FALSE;
    }
    
    if (ppNewEnumState) {
       *ppNewEnumState = pEnumState;
    }
    
cleanup:

    return dwError;
    
error:

    if (ppNewEnumState) {
        *ppNewEnumState = NULL;
    }
    
    if (bFreeState && pEnumState) {
       LsaProviderLocal_FreeStateList(pEnumState);
    }

    goto cleanup;
}

PLOCAL_PROVIDER_ENUM_STATE
LsaProviderLocal_FindGroupState(
    HANDLE hProvider,
    PCSTR  pszGUID
    )
{
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;
    
    return LsaProviderLocal_FindEnumState(
                pContext->pGroupEnumStateList,
                pszGUID);
}

PLOCAL_PROVIDER_ENUM_STATE
LsaProviderLocal_FindEnumState(
    PLOCAL_PROVIDER_ENUM_STATE pStateList,
    PCSTR pszGUID
    )
{
    PLOCAL_PROVIDER_ENUM_STATE pEnumState = NULL;
    while (pStateList) {
        if (!strcasecmp(pStateList->pszGUID, pszGUID)) {
            pEnumState = pStateList;
            break;
        } else {
            pStateList = pStateList->pNext;
        }
    }
    return pEnumState;
}

VOID
LsaProviderLocal_FreeGroupState(
    HANDLE hProvider,
    PCSTR  pszGUID
    )
{
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;
    
    return LsaProviderLocal_FreeEnumState(&pContext->pGroupEnumStateList, pszGUID);
}

VOID
LsaProviderLocal_FreeEnumState(
    PLOCAL_PROVIDER_ENUM_STATE* ppStateList,
    PCSTR pszGUID
    )
{
    PLOCAL_PROVIDER_ENUM_STATE pPrevState = NULL;
    PLOCAL_PROVIDER_ENUM_STATE pState = *ppStateList;
    while (pState) {
        if (!strcasecmp(pState->pszGUID, pszGUID)) {
            if (!pPrevState) {
                *ppStateList = pState->pNext;
            } else {
                pPrevState->pNext = pState->pNext;
            }
            pState->pNext = NULL;
            break;
        } else {
            pPrevState = pState;
            pState = pState->pNext;
        }
    }
    if (pState) {
        LsaProviderLocal_FreeStateList(pState);
    }
}

VOID
LsaProviderLocal_FreeStateList(
    PLOCAL_PROVIDER_ENUM_STATE pStateList
    )
{
    PLOCAL_PROVIDER_ENUM_STATE pState = NULL;
    
    while (pStateList) {
        pState = pStateList;
        pStateList = pStateList->pNext;
        
        LSA_SAFE_FREE_STRING(pState->pszGUID);
        LsaFreeMemory(pState);
    }
}

