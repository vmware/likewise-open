
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
#include <lsasrvcred.h>

#define ENTER_CREDS_LIST(bInLock)                               \
    do                                                          \
    {                                                           \
        if (!bInLock)                                           \
        {                                                       \
            pthread_mutex_lock(&gCredState.LsaCredsListLock);   \
            bInLock = TRUE;                                     \
        }                                                       \
    } while (0)

#define LEAVE_CREDS_LIST(bReleaseLock)                          \
    do                                                          \
    {                                                           \
        if (bReleaseLock)                                       \
        {                                                       \
            pthread_mutex_unlock(&gCredState.LsaCredsListLock); \
            bReleaseLock = FALSE;                               \
        }                                                       \
    } while (0)

typedef struct _LSA_CREDENTIALS
{
    PSTR            pUserName;
    PSTR            pPassword;
    DWORD           dwUserId;
    LONG            nRefCount;
    LSA_LIST_LINKS   ListEntry;
} LSA_CREDENTIALS,  *PLSA_CREDENTIALS;

typedef struct _LSA_CREDENTIALS_STATE
{
    LSA_LIST_LINKS LsaCredsList;
    pthread_mutex_t LsaCredsListLock;
} LSA_CREDENTIALS_STATE, *PLSA_CREDENTIALS_STATE;

static LSA_CREDENTIALS_STATE gCredState = {{0,0}, PTHREAD_MUTEX_INITIALIZER};

static
PLSA_CREDENTIALS
LsaFindCredByUidUnsafe(
    IN DWORD dwUid
    )
{
    // WARNING - gCredState.LsaCredsListLock lock must already be acquired

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

static
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

static
BOOL
LsaCredContains(
    PLSA_CREDENTIALS pCred,
    PCSTR pszUserName,
    PCSTR pszPassword
    )
{
    BOOLEAN bMatches = TRUE;

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

static
VOID
LsaReleaseCredentialUnsafe(
    IN PLSA_CRED_HANDLE phCredential
    )
{
    PLSA_CREDENTIALS pCred = NULL;

    if(phCredential && *phCredential)
    {
        pCred = *phCredential;

        pCred->nRefCount--;

        LW_ASSERT(pCred->nRefCount >= 0);

        if (!(pCred->nRefCount))
        {
            LsaListRemove(&pCred->ListEntry);

            LW_SAFE_FREE_MEMORY(pCred->pUserName);
            LW_SAFE_FREE_MEMORY(pCred->pPassword);

            LW_SAFE_FREE_MEMORY(pCred);
        }

        *phCredential = NULL;
    }
}

VOID
LsaInitializeCredentialsDatabase(
    VOID
    )
{
    ENTER_CREDS_LIST(bInLock);

    LsaListInit(&gCredState.LsaCredsList);

    LEAVE_CREDS_LIST(bInLock);
}

VOID
LsaShutdownCredentialsDatabase(
    VOID
    )
{
    BOOLEAN bInLock = FALSE;
    PLSA_CREDENTIALS pCred = NULL;
    PLSA_LIST_LINKS pCredListEntry = NULL;

    ENTER_CREDS_LIST(bInLock);

    if(gCredState.LsaCredsList.Next && gCredState.LsaCredsList.Prev)
    {
        // sweep the credentials list
        while(!LsaListIsEmpty(&gCredState.LsaCredsList))
        {
            pCredListEntry = LsaListRemoveHead(&gCredState.LsaCredsList);
            pCred = LW_STRUCT_FROM_FIELD(
                pCredListEntry,
                LSA_CREDENTIALS,
                ListEntry);

            LW_SAFE_FREE_MEMORY(pCred->pUserName);
            LW_SAFE_FREE_MEMORY(pCred->pPassword);

            LW_SAFE_FREE_MEMORY(pCred);
        }
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
    BOOLEAN bInLock = FALSE;
    PLSA_CREDENTIALS pCredOld = NULL;
    PLSA_CREDENTIALS pCredNew = NULL;
    LSA_CRED_HANDLE CredHandle = NULL;

    *phCredential = NULL;

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
        pCredOld = LsaFindCredByUidUnsafe(*pdwUid);
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

            // This release is NOT intended to destroy the credential (some one
            // may still be using it).  It's intended to remove the reference
            // added by the LsaFindCredByUid called above.
            LsaReleaseCredentialUnsafe(&pCredOld);
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
    LsaReleaseCredentialUnsafe(&pCredOld);

    CredHandle = NULL;

    goto cleanup;
}

VOID
LsaReferenceCredential(
    IN LSA_CRED_HANDLE hCredential
    )
{
    BOOLEAN bInLock = FALSE;
    PLSA_CREDENTIALS pCred = NULL;

    if(hCredential)
    {
        pCred = hCredential;

        ENTER_CREDS_LIST(bInLock);

        pCred->nRefCount++;

        LEAVE_CREDS_LIST(bInLock);
    }
}

VOID
LsaReleaseCredential(
    IN PLSA_CRED_HANDLE phCredential
    )
{
    BOOLEAN bInLock = FALSE;

    if(phCredential && *phCredential)
    {
        ENTER_CREDS_LIST(bInLock);

        LsaReleaseCredentialUnsafe(phCredential);

        LEAVE_CREDS_LIST(bInLock);
    }
}

LSA_CRED_HANDLE
LsaGetCredential(
    IN DWORD dwUid
    )
{
    BOOLEAN bInLock = FALSE;
    PLSA_CREDENTIALS pCred = NULL;

    ENTER_CREDS_LIST(bInLock);

    pCred = LsaFindCredByUidUnsafe(dwUid);

    LEAVE_CREDS_LIST(bInLock);

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
    PLSA_CREDENTIALS pCred = CredHandle;
    BOOLEAN bInLock = FALSE;

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
