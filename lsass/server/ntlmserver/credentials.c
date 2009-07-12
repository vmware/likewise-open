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
 *        credentials.c
 *
 * Abstract:
 *
 *        NTLM Credentials file
 *
 * Authors: Marc Guy (mguy@likewisesoftware.com)
 *
 */

#include "ntlmsrvapi.h"

/******************************************************************************/
DWORD
NtlmInitCredentials(
    OUT PNTLM_CREDENTIALS *ppNtlmCreds
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;

    if(!ppNtlmCreds)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_NTLM_ERROR(dwError);
    }

    dwError = LwAllocateMemory(
        sizeof(NTLM_CREDENTIALS),
        (PVOID*)(PVOID)ppNtlmCreds
        );

    BAIL_ON_NTLM_ERROR(dwError);

    memset((*ppNtlmCreds), 0, sizeof(NTLM_CREDENTIALS));

    BAIL_ON_NTLM_ERROR(dwError);

cleanup:
    return dwError;
error:
    if(*ppNtlmCreds)
    {
        LwFreeMemory(*ppNtlmCreds);
        *ppNtlmCreds = NULL;
    }
    goto cleanup;
}

/******************************************************************************/
DWORD
NtlmInsertCredentials(
    PNTLM_CREDENTIALS pNtlmCreds
    )
{
    // WARNING:
    // WARNING: Creds lock must already be acquired
    // WARNING:

    DWORD dwError = LW_ERROR_SUCCESS;
    BOOLEAN bCollision = FALSE;
    PNTLM_CREDENTIALS pCollisionCred = NULL;

    if(!pNtlmCreds)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_NTLM_ERROR(dwError);
    }

    // This handle should be random and will be our key into a either a
    // double linked list (bad) or a red-black tree (good)
    //
    do
    {
        bCollision = FALSE;

        dwError = NtlmGetRandomBuffer(
            (PBYTE)&(pNtlmCreds->CredHandle),
            sizeof(CredHandle)
            );

        if(LW_ERROR_SUCCESS != dwError)
        {
            break;
        }

        dwError = NtlmFindCredentials(
            &(pNtlmCreds->CredHandle),
            &pCollisionCred
            );

        if(dwError == LW_ERROR_SUCCESS)
        {
            bCollision = TRUE;

            // This removes the reference we added for the find function
            NtlmRemoveCredentials(&(pCollisionCred->CredHandle));
        }

    } while(bCollision);

    if(LW_ERROR_INVALID_TOKEN == dwError)
    {
        pNtlmCreds->pNext = gpNtlmCredsList;
        gpNtlmCredsList = pNtlmCreds;
        pNtlmCreds->dwRefCount++;
    }

cleanup:
    return dwError;
error:
    goto cleanup;
}

/******************************************************************************/
DWORD
NtlmRemoveCredentials(
    IN PCredHandle pCredHandle
    )
{
    // WARNING:
    // WARNING: Creds lock must already be acquired
    // WARNING:

    DWORD dwError = LW_ERROR_SUCCESS;
    PNTLM_CREDENTIALS pTrav = NULL;
    PNTLM_CREDENTIALS pHold = NULL;

    if(!pCredHandle)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_NTLM_ERROR(dwError);
    }

    pTrav = gpNtlmCredsList;

    if(!pTrav)
    {
        dwError = LW_ERROR_INVALID_TOKEN;
        BAIL_ON_NTLM_ERROR(dwError);
    }
    else if(pTrav->CredHandle.dwLower == pCredHandle->dwLower &&
            pTrav->CredHandle.dwUpper == pCredHandle->dwUpper)
    {
        pTrav->dwRefCount--;

        if(pTrav->dwRefCount <= 0)
        {
            gpNtlmCredsList = pTrav->pNext;
            NtlmFreeCredentials(pTrav);
        }
    }
    else
    {
        while(pTrav->pNext)
        {
            if(pTrav->pNext->CredHandle.dwLower == pCredHandle->dwLower&&
               pTrav->pNext->CredHandle.dwUpper == pCredHandle->dwUpper)
            {
                pHold->dwRefCount--;

                if(pHold->dwRefCount <= 0)
                {
                    pHold = pTrav->pNext;
                    pTrav->pNext = pHold->pNext;

                    NtlmFreeCredentials(pHold);
                }

                break;
            }
            pTrav = pTrav->pNext;
        }
        dwError = LW_ERROR_INVALID_TOKEN;
        BAIL_ON_NTLM_ERROR(dwError);
    }

cleanup:
    return dwError;
error:
    goto cleanup;
}

/******************************************************************************/
DWORD
NtlmRemoveAllCredentials(
    VOID
    )
{
    // WARNING:
    // WARNING: Creds lock must already be acquired
    // WARNING:
    // WARNING: Also, this wipes the creds... all of them... regardless
    // WARNING: of the reference count.  Only use this at shutdown.
    // WARNING:

    DWORD dwError = LW_ERROR_SUCCESS;
    PNTLM_CREDENTIALS pTrav = NULL;

    while(gpNtlmCredsList)
    {
        pTrav = gpNtlmCredsList;
        gpNtlmCredsList = pTrav->pNext;

        dwError = NtlmFreeCredentials(pTrav);
        BAIL_ON_NTLM_ERROR(dwError);
    }

cleanup:
    return dwError;
error:
    goto cleanup;
}

/******************************************************************************/
DWORD
NtlmFindCredentials(
    IN PCredHandle pCredHandle,
    OUT PNTLM_CREDENTIALS *ppNtlmCreds
    )
{
    // WARNING:
    // WARNING: Creds lock must already be acquired
    // WARNING:

    DWORD dwError = LW_ERROR_SUCCESS;
    PNTLM_CREDENTIALS pTrav = NULL;

    *ppNtlmCreds = NULL;

    if(!pCredHandle)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_NTLM_ERROR(dwError);
    }

    pTrav = gpNtlmCredsList;

    while(pTrav)
    {
        if(pTrav->CredHandle.dwLower == (*ppNtlmCreds)->CredHandle.dwLower &&
           pTrav->CredHandle.dwUpper == (*ppNtlmCreds)->CredHandle.dwUpper)
        {
            *ppNtlmCreds = pTrav;
            (*ppNtlmCreds)->dwRefCount++;
            break;
        }

        pTrav = pTrav->pNext;
    }

    if(!(*ppNtlmCreds))
    {
        dwError = LW_ERROR_INVALID_TOKEN;
    }

cleanup:
    return dwError;
error:
    goto cleanup;

}

/******************************************************************************/
DWORD
NtlmFreeCredentials(
    PNTLM_CREDENTIALS pNtlmCreds
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;

    if(!pNtlmCreds)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_NTLM_ERROR(dwError);
    }

    if(pNtlmCreds->pUserName)
    {
        LwFreeMemory(pNtlmCreds->pUserName);
    }

    if(pNtlmCreds->pPassword)
    {
        LwFreeMemory(pNtlmCreds->pPassword);
    }

    LwFreeMemory(pNtlmCreds);

cleanup:
    return dwError;
error:
    goto cleanup;
}
