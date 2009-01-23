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
 *        Enumeration State Utilities
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Wei Fu (wfu@likewisesoftware.com)
 *
 */
#include "adprovider.h"

static
DWORD
AD_AddEnumState(
    PAD_ENUM_STATE* ppStateList,
    PCSTR pszGUID,
    DWORD dwInfoLevel,
    PCSTR pszMapName,
    LSA_NIS_MAP_QUERY_FLAGS dwFlags,
    PAD_ENUM_STATE* ppNewEnumState
    );

static
PAD_ENUM_STATE
AD_FindEnumState(
    PAD_ENUM_STATE pStateList,
    PCSTR pszGUID
    );

static
VOID
AD_FreeEnumState(
    PAD_ENUM_STATE* ppStateList,
    PCSTR pszGUID
    );

VOID
AD_FreeStateList(
    PAD_ENUM_STATE pStateList
    )
{
    PAD_ENUM_STATE pState = NULL;

    while (pStateList)
    {
        pState = pStateList;
        pStateList = pStateList->pNext;

        LSA_SAFE_FREE_STRING(pState->pszGUID);

        LsaFreeCookieContents(&pState->Cookie);
        if (pState->hDirectory)
        {
            LsaLdapCloseDirectory(pState->hDirectory);
        }

        LsaFreeMemory(pState);
    }
}

DWORD
AD_AddUserState(
    HANDLE  hProvider,
    PCSTR   pszGUID,
    DWORD   dwInfoLevel,
    PAD_ENUM_STATE* ppEnumState
    )
{
    PAD_PROVIDER_CONTEXT pContext = (PAD_PROVIDER_CONTEXT)hProvider;

    return AD_AddEnumState(
                &pContext->pUserEnumStateList,
                pszGUID,
                dwInfoLevel,
                NULL,
                0,
                ppEnumState);
}

PAD_ENUM_STATE
AD_FindUserState(
    HANDLE hProvider,
    PCSTR  pszGUID
    )
{
    PAD_PROVIDER_CONTEXT pContext = (PAD_PROVIDER_CONTEXT)hProvider;

    return AD_FindEnumState(pContext->pUserEnumStateList, pszGUID);
}

VOID
AD_FreeUserState(
    HANDLE hProvider,
    PCSTR  pszGUID
    )
{
    PAD_PROVIDER_CONTEXT pContext = (PAD_PROVIDER_CONTEXT)hProvider;

    return AD_FreeEnumState(&pContext->pUserEnumStateList, pszGUID);
}

DWORD
AD_AddGroupState(
    HANDLE hProvider,
    PCSTR  pszGUID,
    DWORD  dwInfoLevel,
    PAD_ENUM_STATE* ppEnumState
    )
{
    PAD_PROVIDER_CONTEXT pContext = (PAD_PROVIDER_CONTEXT)hProvider;

    return AD_AddEnumState(
                    &pContext->pGroupEnumStateList,
                    pszGUID,
                    dwInfoLevel,
                    NULL,
                    0,
                    ppEnumState);
}

PAD_ENUM_STATE
AD_FindGroupState(
    HANDLE hProvider,
    PCSTR  pszGUID
    )
{
    PAD_PROVIDER_CONTEXT pContext = (PAD_PROVIDER_CONTEXT)hProvider;

    return AD_FindEnumState(pContext->pGroupEnumStateList, pszGUID);
}

VOID
AD_FreeGroupState(
    HANDLE hProvider,
    PCSTR  pszGUID
    )
{
    PAD_PROVIDER_CONTEXT pContext = (PAD_PROVIDER_CONTEXT)hProvider;

    return AD_FreeEnumState(&pContext->pGroupEnumStateList, pszGUID);
}

DWORD
AD_AddNSSArtefactState(
    HANDLE hProvider,
    PCSTR  pszGUID,
    DWORD  dwInfoLevel,
    PCSTR  pszMapName,
    LSA_NIS_MAP_QUERY_FLAGS dwFlags,
    PAD_ENUM_STATE* ppEnumState
    )
{
    PAD_PROVIDER_CONTEXT pContext = (PAD_PROVIDER_CONTEXT)hProvider;

    return AD_AddEnumState(
                    &pContext->pNSSArtefactEnumStateList,
                    pszGUID,
                    dwInfoLevel,
                    pszMapName,
                    dwFlags,
                    ppEnumState);
}

PAD_ENUM_STATE
AD_FindNSSArtefactState(
    HANDLE hProvider,
    PCSTR  pszGUID
    )
{
    PAD_PROVIDER_CONTEXT pContext = (PAD_PROVIDER_CONTEXT)hProvider;

    return AD_FindEnumState(pContext->pNSSArtefactEnumStateList, pszGUID);
}

VOID
AD_FreeNSSArtefactState(
    HANDLE hProvider,
    PCSTR  pszGUID
    )
{
    PAD_PROVIDER_CONTEXT pContext = (PAD_PROVIDER_CONTEXT)hProvider;

    return AD_FreeEnumState(&pContext->pNSSArtefactEnumStateList, pszGUID);
}

static
DWORD
AD_AddEnumState(
    PAD_ENUM_STATE* ppStateList,
    PCSTR pszGUID,
    DWORD dwInfoLevel,
    PCSTR pszMapName,
    LSA_NIS_MAP_QUERY_FLAGS dwFlags,
    PAD_ENUM_STATE* ppNewEnumState
    )
{
    DWORD dwError = 0;
    PAD_ENUM_STATE pEnumState = NULL;
    BOOLEAN bFreeState = FALSE;

    if (!(pEnumState = AD_FindEnumState(*ppStateList, pszGUID))) {
        dwError = LsaAllocateMemory(
                        sizeof(AD_ENUM_STATE),
                        (PVOID*)&pEnumState);
        BAIL_ON_LSA_ERROR(dwError);
        bFreeState = TRUE;

        dwError = LsaAllocateString(pszGUID, &pEnumState->pszGUID);
        BAIL_ON_LSA_ERROR(dwError);

        pEnumState->dwInfoLevel = dwInfoLevel;
        pEnumState->dwMapFlags = dwFlags;

        if (pszMapName)
        {
            dwError = LsaAllocateString(pszMapName, &pEnumState->pszMapName);
            BAIL_ON_LSA_ERROR(dwError);
        }

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
       AD_FreeStateList(pEnumState);
    }

    goto cleanup;
}

static
PAD_ENUM_STATE
AD_FindEnumState(
    PAD_ENUM_STATE pStateList,
    PCSTR pszGUID
    )
{
    PAD_ENUM_STATE pEnumState = NULL;
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

static
VOID
AD_FreeEnumState(
    PAD_ENUM_STATE* ppStateList,
    PCSTR pszGUID
    )
{
    PAD_ENUM_STATE pPrevState = NULL;
    PAD_ENUM_STATE pState = *ppStateList;
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
        AD_FreeStateList(pState);
    }
}
