
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
    //pthread_mutexattr_t MutexAttribs;
    //pthread_mutexattr_settype(&MutexAttribs, PTHREAD_MUTEX_RECURSIVE);
    //pthread_mutex_init(gCredState.LsaCredsListLock);

    gCredState.LsaCredsListLock = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;

    LwListInit(&gCredState.LsaCredsList);

    return;
}

VOID
LsaShutdownCredentialsDatabase(
    VOID
    )
{
    BOOL bInLock = FALSE;
    PLSA_CREDENTIALS pCred = NULL;
    PLW_LIST_LINKS pCredListEntry = NULL;

    ENTER_CREDS_LIST(bInLock);

        // sweep the credentials list
        while(!LwListIsEmpty(&gCredState.LsaCredsList))
        {
            pCredListEntry = LwListRemoveHead(&gCredState.LsaCredsList);
            pCred = LW_STRUCT_FROM_FIELD(
                pCredListEntry,
                LSA_CREDENTIALS,
                ListEntry);

            LwFreeMemory(pCred->pUserName);
            LwFreeMemory(pCred->pPassword);

            LwFreeMemory(pCred);
        }

    LEAVE_CREDS_LIST(bInLock);

    // Only needed if we call pthread_mutex_init
    //dwError = pthread_mutex_destroy(&gCredState.LsaCredsListLock);
    //if(dwError)
    //{
    //    dwError = LwMapErrnoToLwError(errno);
    //    BAIL_ON_LW_ERROR(dwError);
    //}

    return;
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
    PLW_LIST_LINKS pCredListEntry = NULL;
    PLSA_CREDENTIALS pCred = NULL;
    LSA_CRED_HANDLE CredHandle = NULL;

    *phCredential = NULL;

    if (!pszUserName  ||
        !pszPassword  ||
        !*pszUserName ||
        !*pszPassword ||
        !*pdwUid)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LW_ERROR(dwError);
    }

    ENTER_CREDS_LIST(bInLock);

        //try to find the user. If he exists, just bump the ref and return
        if (pdwUid)
        {
            for (pCredListEntry = gCredState.LsaCredsList.Next;
                 pCredListEntry != &gCredState.LsaCredsList;
                 pCredListEntry = pCredListEntry->Next)
            {
                pCred = LW_STRUCT_FROM_FIELD(
                    pCredListEntry,
                    LSA_CREDENTIALS,
                    ListEntry);

                if (pCred->dwUserId == *pdwUid)
                {
                    CredHandle = pCred;
                    pCred->nRefCount++;
                    break;
                }
            }
        }

        if (!(CredHandle))
        {
            dwError = LwAllocateMemory(
                sizeof(LSA_CREDENTIALS),
                (PVOID*)(PVOID)&pCred);
            BAIL_ON_LW_ERROR(dwError);

            dwError = LwAllocateMemory(
                strlen(pszUserName) + 1,
                (PVOID*)(PVOID)&pCred->pUserName);
            BAIL_ON_LW_ERROR(dwError);

            strcpy(pCred->pUserName, pszUserName);

            dwError = LwAllocateMemory(
                strlen(pszPassword) + 1,
                (PVOID*)(PVOID)&pCred->pPassword);
            BAIL_ON_LW_ERROR(dwError);

            strcpy(pCred->pPassword, pszPassword);

            pCred->nRefCount = 1;

            if (pdwUid)
            {
                pCred->dwUserId = *pdwUid;
            }

            CredHandle = pCred;

            LwListInsertHead(&gCredState.LsaCredsList, &pCred->ListEntry);
        }

    *phCredential = CredHandle;

cleanup:

    if (bInLock)
    {
        LEAVE_CREDS_LIST(bInLock);
    }

    return dwError;
error:

    *phCredential = NULL;

    goto cleanup;
}

VOID
LsaReleaseCredential(
    IN LSA_CRED_HANDLE hCredential
    )
{
    BOOL bInLock = FALSE;
    PLSA_CREDENTIALS pCred = (PLSA_CREDENTIALS)hCredential;

    ENTER_CREDS_LIST(bInLock);

        pCred->nRefCount--;

        LW_ASSERT(pCred->nRefCount >= 0);

        if (!(pCred->nRefCount))
        {
            LwListRemove(&pCred->ListEntry);

            LwFreeMemory(pCred->pUserName);
            LwFreeMemory(pCred->pPassword);

            LwFreeMemory(pCred);
        }

    LEAVE_CREDS_LIST(bInLock);
    return;
}

DWORD
LsaGetCredential(
    IN DWORD dwUid,
    OUT PLSA_CRED_HANDLE phCredential
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    BOOL bInLock = FALSE;
    PLSA_CREDENTIALS pCred = NULL;
    PLW_LIST_LINKS pCredListEntry = NULL;
    LSA_CRED_HANDLE CredHandle = NULL;

    *phCredential = NULL;

    ENTER_CREDS_LIST(bInLock);

        for (pCredListEntry = gCredState.LsaCredsList.Next;
             pCredListEntry != &gCredState.LsaCredsList;
             pCredListEntry = pCredListEntry->Next)
        {
            pCred = LW_STRUCT_FROM_FIELD(
                pCredListEntry,
                LSA_CREDENTIALS,
                ListEntry);

            if (dwUid == pCred->dwUserId)
            {
                pCred->nRefCount++;
                CredHandle = pCred;
            }
        }

        if (!CredHandle)
        {
            dwError = LW_ERROR_NO_CRED;
            BAIL_ON_LW_ERROR(dwError);
        }

        *phCredential = CredHandle;

cleanup:

    if (bInLock)
    {
        LEAVE_CREDS_LIST(bInLock);
    }

    return dwError;

error:

    *phCredential = NULL;

    goto cleanup;
}

VOID
LsaGetCredentialInfo(
    IN LSA_CRED_HANDLE CredHandle,
    OUT OPTIONAL PSTR* pszUserName,
    OUT OPTIONAL PSTR* pszPassword,
    OUT OPTIONAL PDWORD pUid
    )
{
    PLSA_CREDENTIALS pCred = (PLSA_CREDENTIALS)CredHandle;
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

    return;
}

DWORD
LsaModifyCredential(
    IN LSA_CRED_HANDLE CredHandle,
    OUT OPTIONAL PCSTR pszUserName,
    OUT OPTIONAL PCSTR pszPassword,
    OUT OPTIONAL const PDWORD pUid
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PLSA_CREDENTIALS pCred = CredHandle;
    BOOL bInLock = FALSE;
    PSTR pUserName = NULL;
    PSTR pPassword = NULL;

    if(!pCred)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LW_ERROR(dwError);
    }

    ENTER_CREDS_LIST(bInLock);

        if(pszUserName)
        {
            dwError = LwAllocateMemory(
                strlen(pszUserName) + 1,
                (PVOID*)(PVOID)pUserName);
            BAIL_ON_LW_ERROR(dwError);

            if(pCred->pUserName)
            {
                LW_SAFE_FREE_STRING(pCred->pUserName);
            }

            memcpy(pUserName, pszUserName, strlen(pszUserName));

            pCred->pUserName = pUserName;
        }

        if(pszPassword)
        {
            dwError = LwAllocateMemory(
                strlen(pszPassword) + 1,
                (PVOID*)(PVOID)pPassword);
            BAIL_ON_LW_ERROR(dwError);

            if(pCred->pPassword)
            {
                memset(pCred->pPassword, 0, strlen(pCred->pPassword));
                LW_SAFE_FREE_STRING(pCred->pPassword);
            }

            memcpy(pPassword, pszPassword, strlen(pszPassword));

            pCred->pPassword = pPassword;
        }

        if(pUid)
        {
            pCred->dwUserId = *pUid;
        }

    LEAVE_CREDS_LIST(bInLock);

cleanup:
    return dwError;
error:
    goto cleanup;
}
