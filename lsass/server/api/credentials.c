
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

DWORD
LsaInitializeCredentialsDatabase()
{
    DWORD dwError = LW_ERROR_SUCCESS;

    LwListInit(&gLsaCredsList);

cleanup:
    return dwError;
error:
    goto cleanup;
}

DWORD
LsaShutdownCredentialsDatabase()
{
    DWORD dwError = LW_ERROR_SUCCESS;
    BOOL bInLock = FALSE;
    PLSA_CREDENTIALS pCred = NULL;
    PLW_LIST_LINKS pCredListEntry = NULL;

    ENTER_CREDS_LIST(bInLock);

        // sweep the credentials list
        while(!LwListIsEmpty(gLsaCredsList))
        {
            pCredListEntry = LwListRemoveHead(gLsaCredsList);
            pCred = LW_STRUCT_FROM_FIELD(
                pCredListEntry,
                LSA_CREDENTIALS,
                ListEntry);

            LwFreeMemory(pCred->pUserName);
            LwFreeMemory(pCred->pPassword);

            LwFreeMemory(pCred);
        }

    LEAVE_CREDS_LIST(bInLock);


    dwError = pthread_mutex_destroy(&gLsaCredsListLock);
    if (LW_ERROR_SUCCESS != dwError)
    {
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_NTLM_ERROR(dwError);
    }

    gpLsaCredsList = NULL;

cleanup:
    return dwError;
error:
    goto cleanup;
}

DWORD
LsaAddCredential(
    IN PWSTR pszUserName,
    IN PWSTR pszPassword,
    IN OPTIONAL PDWORD pdwUid,
    OUT LSA_CRED_HANDLE* phCredential
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    BOOL bInLock = FALSE;
    PLW_LIST_LINKS pCredListEntry = NULL;
    PLSA_CREDENTIALS pCred = NULL;

    *phCredential = NULL;

    if (!pszUserName || !pszPassword || !*pszUserName || !*pszPassword)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LW_ERROR(dwError);
    }

    ENTER_CREDS_LIST(bInLock);

        //try to find the user. If he exists, just bump the ref and return
        if (pdwUid)
        {
            for (pCredListEntry = gLsaCredsList->Next;
                 pCredListEntry != gLsaCredsList;
                 pCredListEntry = pCredListEntry->Next)
            {
                pCred = LW_STRUCT_FROM_FIELD(
                    pCredListEntry,
                    LSA_CREDENTIALS,
                    ListEntry);

                if (pCred->dwUserId == *pdwUid)
                {
                    *phCredentail = pCred;
                    pCred->dwRefCount++;
                    break;
                }
            }
        }

        if (!(*phCredentail))
        {
            dwError = LwAllocateMemory(
                sizeof(LSA_CREDENTIALS),
                (PVOID*)(PVOID)&pCred);
            BAIL_ON_LW_ERROR(dwError);

            dwError = LwAllocateMemory(
                (wc16slen(pszUserName) + 1) * sizeof(WCHAR),
                (PVOID*)(PVOID)&pCred->pUserName);
            BAIL_ON_LW_ERROR(dwError);

            wc16scpy(pCred->pUserName, pszUserName);

            dwError = LwAllocateMemory(
                (wc16slen(pszPassword) + 1) * sizeof(WCHAR),
                (PVOID*)(PVOID)&pCred->pPassword);
            BAIL_ON_LW_ERROR(dwError);

            wc16scpy(pCred->pPassword, pszPassword);

            pCred->dwRefCount = 1;

            if (pdwUid)
            {
                pCred->dwUserId = *pdwUid;
            }

            LwListInsertHead(&pCred->ListEntry);
        }

    LEAVE_CREDS_LIST(bInLock);

cleanup:
    return dwError;
error:
    if (bInLock)
    {
        LEAVE_CREDS_LIST(bInLock);
    }
    goto cleanup;
}

DWORD
LsaReleaseCredential(
    IN LSA_CRED_HANDLE hCredential
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    BOOL bInLock = FALSE;
    PLSA_CREDENTIALS pCred = hCredential;

    ENTER_CREDS_LIST(bInLock);

        pCred->dwRefCount--;

        if (!(pCred->dwRefCount))
        {
            LwListRemove(&pCred->ListEntry);

            LwFreeMemory(pCred->pUserName);
            LwFreeMemory(pCred->pPassword);

            LwFreeMemory(pCred);
        }

    LEAVE_CREDS_LIST(bInLock);

cleanup:
    return dwError;
error:
    if (bInLock)
    {
        LEAVE_CREDS_LIST(bInLock);
    }
    goto cleanup;
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

    *phCredential = NULL;

    ENTER_CREDS_LIST(bInLock);

        for (pCredListEntry = gLsaCredsList->Next;
             pCredListEntry != gLsaCredsList;
             pCredListEntry = pCredListEntry->Next)
        {
            pCred = LW_STRUCT_FROM_FIELD(
                pCredListEntry,
                LSA_CREDENTIALS,
                ListEntry);

            if (dwUid == pCred->dwUserId)
            {
                *phCredential = pCred;
            }
        }

        if (!*phCredential)
        {
            dwError = LW_ERROR_NO_CRED;
            BAIL_ON_LW_ERROR(dwError);
        }

    LEAVE_CREDS_LIST(bInLock);

cleanup:
    return dwError;
error:
    if (bInLock)
    {
        LEAVE_CREDS_LIST(bInLock);
    }
    goto cleanup;
}

DWORD
LsaGetCredentialInfo(
    IN LSA_CRED_HANDLE,
    OUT PWSTR* pszUserName,
    OUT PWSTR* pszPassword,
    OUT PDWORD pUid
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PLSA_CREDENTIALS pCred = LSA_CRED_HANDLE;

    ENTER_CREDS_LIST(bInLock);

        *pszUserName = pCred->pUserName;
        *pszPassword = pCred->pPassword;
        *pUid = pCred->dwUserId;

    LEAVE_CREDS_LIST(bInLock);

cleanup:
    return dwError;
error:
    goto cleanup;
}
