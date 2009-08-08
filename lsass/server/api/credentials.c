
/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        credentials.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *
 *
 * Authors:
 */
#include "api.h"

static LSA_CREDENTIALS_STATE gCredState;

VOID
LsaInitializeCredentialsDatabase(
    VOID
    )
{
    gCredState.LsaCredsListLock = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;

    LsaListInit(&gCredState.LsaCredsList);
}

VOID
LsaShutdownCredentialsDatabase(
    VOID
    )
{
    BOOL bInLock = FALSE;
    PLSA_CREDENTIALS pCred = NULL;
    PLSA_LIST_LINKS pCredListEntry = NULL;

    ENTER_CREDS_LIST(bInLock);

    // sweep the credentials list
    while(!LsaListIsEmpty(&gCredState.LsaCredsList))
    {
        pCredListEntry = LsaListRemoveHead(&gCredState.LsaCredsList);
        pCred = LW_STRUCT_FROM_FIELD(
            pCredListEntry,
            LSA_CREDENTIALS,
            ListEntry);

        LwFreeMemory(pCred->pUserName);
        LwFreeMemory(pCred->pPassword);

        LwFreeMemory(pCred);
    }

    LEAVE_CREDS_LIST(bInLock);
}

DWORD
LsaAddCredential(
    IN PCSTR pszUserName,
    IN PCSTR pszPassword,
    IN OPTIONAL const PDWORD pdwUid,
    OUT PLSA_CRED_HANDLE phCredential
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    BOOL bInLock = FALSE;
    PLSA_CREDENTIALS pCredOld = NULL;
    PLSA_CREDENTIALS pCredNew = NULL;
    LSA_CRED_HANDLE CredHandle = INVALID_LSA_CRED_HANDLE;

    *phCredential = INVALID_LSA_CRED_HANDLE;

    if (!pszUserName  ||
        !pszPassword  ||
        !*pszUserName ||
        !*pszPassword ||
        (pdwUid && !*pdwUid))
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LW_ERROR(dwError);
    }

    ENTER_CREDS_LIST(bInLock);

    if (pdwUid)
    {
        pCredOld = LsaFindCredByUid(*pdwUid);
        CredHandle = pCredOld;
    }

    if (!pCredOld || !LsaCredContains(pCredOld, pszUserName, pszPassword))
    {
        dwError = LsaCreateCred(pszUserName, pszPassword, pdwUid, &pCredNew);
        BAIL_ON_LW_ERROR(dwError);

        CredHandle = pCredNew;

        LsaListInsertHead(&gCredState.LsaCredsList, &pCredNew->ListEntry);

        if(pCredOld)
        {
            LsaListRemove(&pCredOld->ListEntry);
            LsaReleaseCredentialUnsafe(pCredOld);
        }
    }

cleanup:

    *phCredential = CredHandle;

    if (bInLock)
    {
        LEAVE_CREDS_LIST(bInLock);
    }

    return dwError;

error:
    if(pCredOld)
    {
        LsaReleaseCredential(pCredOld);
    }

    CredHandle = INVALID_LSA_CRED_HANDLE;

    goto cleanup;
}

DWORD
LsaCreateCred(
    IN PCSTR pszUserName,
    IN PCSTR pszPassword,
    IN OPTIONAL const PDWORD pdwUid,
    OUT PLSA_CREDENTIALS* ppCredential
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PLSA_CREDENTIALS pCred = NULL;

    dwError = LwAllocateMemory(sizeof(*pCred), OUT_PPVOID(&pCred));
    BAIL_ON_LW_ERROR(dwError);

    dwError = LwAllocateString(pszUserName, &pCred->pUserName);
    BAIL_ON_LW_ERROR(dwError);

    dwError = LwAllocateString(pszPassword, &pCred->pPassword);
    BAIL_ON_LW_ERROR(dwError);

    pCred->nRefCount = 1;

    if (pdwUid)
    {
        pCred->dwUserId = *pdwUid;
    }

cleanup:
    *ppCredential = pCred;
    return dwError;
error:
    if(pCred)
    {
        LW_SAFE_FREE_STRING(pCred->pUserName);
        LW_SAFE_FREE_STRING(pCred->pPassword);

        LW_SAFE_FREE_MEMORY(pCred);
    }

    pCred = NULL;

    goto cleanup;
}

BOOL
LsaCredContains(
    PLSA_CREDENTIALS pCred,
    PCSTR pszUserName,
    PCSTR pszPassword
    )
{
    BOOL bMatches = TRUE;

    if(!pCred)
    {
        bMatches = FALSE;
    }

    if(strcasecmp(pszUserName, pCred->pUserName))
    {
        bMatches = FALSE;
    }

    if(strcmp(pszPassword, pCred->pPassword))
    {
        bMatches = FALSE;
    }

    return bMatches;
}

VOID
LsaReferenceCredential(
    IN LSA_CRED_HANDLE hCredential
    )
{
    BOOL bInLock = FALSE;
    PLSA_CREDENTIALS pCred = NULL;

    if(hCredential != INVALID_LSA_CRED_HANDLE)
    {
        pCred = hCredential;

        ENTER_CREDS_LIST(bInLock);

        pCred->nRefCount++;

        LEAVE_CREDS_LIST(bInLock);
    }
}

VOID
LsaReleaseCredential(
    IN LSA_CRED_HANDLE hCredential
    )
{
    BOOL bInLock = FALSE;

    if(hCredential && hCredential != INVALID_LSA_CRED_HANDLE)
    {
        ENTER_CREDS_LIST(bInLock);

        LsaReleaseCredentialUnsafe(hCredential);

        LEAVE_CREDS_LIST(bInLock);
    }
}

VOID
LsaReleaseCredentialUnsafe(
    IN LSA_CRED_HANDLE hCredential
    )
{
    PLSA_CREDENTIALS pCred = NULL;

    if(hCredential && hCredential != INVALID_LSA_CRED_HANDLE)
    {
        pCred = hCredential;

        pCred->nRefCount--;

        LW_ASSERT(pCred->nRefCount >= 0);

        if (!(pCred->nRefCount))
        {
            LsaListRemove(&pCred->ListEntry);

            LwFreeMemory(pCred->pUserName);
            LwFreeMemory(pCred->pPassword);

            LwFreeMemory(pCred);
        }
    }
}

LSA_CRED_HANDLE
LsaGetCredential(
    IN DWORD dwUid
    )
{
    BOOL bInLock = FALSE;
    PLSA_CREDENTIALS pCred = NULL;

    ENTER_CREDS_LIST(bInLock);

    pCred = LsaFindCredByUid(dwUid);

    LEAVE_CREDS_LIST(bInLock);

    if(!pCred)
    {
        pCred = INVALID_LSA_CRED_HANDLE;
    }

    return pCred;
}

PLSA_CREDENTIALS
LsaFindCredByUid(
    IN DWORD dwUid
    )
{
    PLSA_CREDENTIALS pCredTrav = NULL;
    PLSA_LIST_LINKS pCredListEntry = NULL;
    PLSA_CREDENTIALS pCred = NULL;

    for (pCredListEntry = gCredState.LsaCredsList.Next;
         pCredListEntry != &gCredState.LsaCredsList;
         pCredListEntry = pCredListEntry->Next)
    {
        pCredTrav = LW_STRUCT_FROM_FIELD(
            pCredListEntry,
            LSA_CREDENTIALS,
            ListEntry);

        if (dwUid == pCredTrav->dwUserId)
        {
            pCredTrav->nRefCount++;
            pCred = pCredTrav;
            break;
        }
    }

    return pCred;
}

VOID
LsaGetCredentialInfo(
    IN LSA_CRED_HANDLE CredHandle,
    OUT OPTIONAL PCSTR* pszUserName,
    OUT OPTIONAL PCSTR* pszPassword,
    OUT OPTIONAL PDWORD pUid
    )
{
    if(pszUserName)
    {
        *pszUserName = NULL;
    }
    if(pszPassword)
    {
        *pszPassword = NULL;
    }
    if(pUid)
    {
        *pUid = 0;
    }

    if(CredHandle != INVALID_LSA_CRED_HANDLE)
    {
        PLSA_CREDENTIALS pCred = CredHandle;
        BOOL bInLock = FALSE;

        ENTER_CREDS_LIST(bInLock);

        if(pszUserName)
        {
            *pszUserName = pCred->pUserName;
        }

        if(pszPassword)
        {
            *pszPassword = pCred->pPassword;
        }

        if(pUid)
        {
            *pUid = pCred->dwUserId;
        }

        LEAVE_CREDS_LIST(bInLock);
    }
}
