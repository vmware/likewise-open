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
 *        credential.c
 *
 * Abstract:
 *
 *        Credential handle management functions.
 *
 * Author: Todd Stecher (2007)
 *
 */
#include "server.h"

NTLM_LIST *g_credentialList;
pthread_mutex_t g_CredentialMtx;

#define NTLM_LOCK_CREDENTIALS(_x_)   pthread_mutex_lock(&g_CredentialMtx)
#define NTLM_UNLOCK_CREDENTIALS(_x_)   pthread_mutex_unlock(&g_CredentialMtx)

void
NTLMDumpCredential(
    DWORD lvl,
    NTLM_CREDENTIAL *pCred
    )
{
    if ((lvl & db_level) == 0)
        return;

    DBG(lvl,("credential(0x%p)\n", pCred));
    DBG(lvl,("handles: %d  ref: %d \n", pCred->handleCount, pCred->refCount));
    DBG(lvl,("flags: 0x%x pid: %d\n", pCred->flags, pCred->processId));
    NTLMDumpAuthUser(lvl, &pCred->authUser);
}


void
NTLMRemoveCredential(NTLM_CREDENTIAL *pCred)
{
    NTLM_LOCK_CREDENTIALS();
    g_credentialList = NTLMRemoveLink(g_credentialList, &pCred->link);
    NTLMDereferenceCredential(pCred);
    NTLM_UNLOCK_CREDENTIALS();
}


void
NTLMInsertCredential(NTLM_CREDENTIAL *pCred)
{
   NTLM_LOCK_CREDENTIALS();

   g_credentialList = NTLMListPrepend(g_credentialList, &pCred->link);
   pCred->refCount++;

   NTLM_UNLOCK_CREDENTIALS();
}

static void
NTLMFreeCredential(NTLM_CREDENTIAL *pCred)
{
    /* assert refCount == 0 */
    /* assert handlcount == 0 */
    /* assert removed from list */
    NTLM_SAFE_FREE(pCred);
}
 
bool
NTLMDereferenceCredential(NTLM_CREDENTIAL *pCred)
{

    if (!pCred)
        return false;

    /* @todo interlocked version ? */
    NTLM_LOCK_CREDENTIALS();
    if ( 0 == --pCred->refCount) {
        /* free */
        NTLMFreeCredential(pCred);
        NTLM_UNLOCK_CREDENTIALS();
        return true;
    }

    NTLM_UNLOCK_CREDENTIALS();
    return false;
}

void
NTLMSafeDereferenceCredential(NTLM_CREDENTIAL **ppCred)
{
    if (NTLMDereferenceCredential(*ppCred))
        *ppCred = NULL;
}



void
NTLMReferenceCredential(NTLM_CREDENTIAL *pCred)
{
    /* @todo interlocked? */
    NTLM_LOCK_CREDENTIALS();
    pCred->refCount++;
    NTLM_UNLOCK_CREDENTIALS();
}

void
NTLMDereferenceCredentialHandle(NTLM_CREDENTIAL *pCred)
{

    if (!pCred)
        return;

    /* @todo interlocked version ? */
    NTLM_LOCK_CREDENTIALS();
    if ( 0 == --pCred->handleCount) {
        /* 
         * remove it from the list - it will be cleaned up
         * when the caller removes its final reference
         */ 
        NTLMRemoveCredential(pCred);
    }

    /* @todo - debugging info */

    NTLM_UNLOCK_CREDENTIALS();
}


void
NTLMReferenceCredentialHandle(NTLM_CREDENTIAL *pCred)
{
    /* @todo interlocked? */
    NTLM_LOCK_CREDENTIALS();
    pCred->handleCount++;
    NTLM_UNLOCK_CREDENTIALS();
}

NTLM_CREDENTIAL*
NTLMValidateCredential(
    NTLM_CREDENTIAL *pCred, 
    bool addReference
    )
{
    NTLM_LIST *node;
    NTLM_LOCK_CREDENTIALS();
        
    node = NTLMListFindNode(
            g_credentialList, 
            &pCred->link
            );

    if (node && addReference) 
        NTLMReferenceCredential(pCred);

    NTLM_UNLOCK_CREDENTIALS();

    return (node ? pCred : NULL);
}



PNTLM_CREDENTIAL
NTLMLocateCredential(
    PAUTH_USER authUser,
    pid_t processId,
    DWORD flags,
    bool addHandleReference
    )
{
    NTLM_CREDENTIAL *pCred = NULL;
    NTLM_CREDENTIAL *pFound = NULL;
    NTLM_LIST *cur;

    NTLM_LOCK_CREDENTIALS();

    for (cur = g_credentialList; (!pFound || cur) ;cur = cur->next)
    {
        /* first time */
        if (!cur)
            break;

        pCred = (PNTLM_CREDENTIAL) cur;
        if (processId != pCred->processId)
            continue;
        
        if (!NTLMCompareAuthUsers(authUser, &pCred->authUser))
            continue;

        if (pCred->flags != flags)
            continue;

        pFound = pCred;
        NTLMReferenceCredential(pCred);
        if (addHandleReference)
            NTLMReferenceCredentialHandle(pCred);
    }

    NTLM_UNLOCK_CREDENTIALS();
    return pFound;
}

PNTLM_CREDENTIAL
NTLMAllocateCredential(
    PAUTH_USER authUser,
    pid_t processId,
    DWORD flags
    )
{
    PNTLM_CREDENTIAL pCred;
    DWORD dwError;
    
    pCred = NTLMAllocateMemory(sizeof(NTLM_CREDENTIAL));
    if (!pCred)
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_OUT_OF_MEMORY);

    dwError = NTLMCopyAuthUser(
                    &pCred->authUser,
                    authUser
                    );
    
    BAIL_ON_NTLM_ERROR(dwError);

    pCred->processId = processId;
    pCred->flags = flags;

    return pCred;
    
error:

    NTLMFreeAuthUser(&pCred->authUser);
    NTLM_SAFE_FREE(pCred);
    return NULL;
}

DWORD
NTLMAcquireCredentialHandle(
    PAUTH_USER credentials,
    pid_t processId,
    DWORD flags,
    PNTLM_CREDENTIAL *ppCredentialOut
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    PNTLM_CREDENTIAL pCred = NULL;
  
    NTLM_LOCK_CREDENTIALS();
    
    /* First check to see if we already have this cred */
    pCred = NTLMLocateCredential(
                credentials,
                processId, 
                flags,
                true
                );

    if (pCred) {
        *ppCredentialOut = pCred;
        return LSA_ERROR_SUCCESS;
    }

    /* create one */
    pCred = NTLMAllocateCredential(
                    credentials,
                    processId, 
                    flags
                    );

    if (!pCred) 
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_OUT_OF_MEMORY);
    

    NTLMInsertCredential(pCred);

    NTLMReferenceCredentialHandle(pCred);
    NTLMReferenceCredential(pCred);

    *ppCredentialOut = pCred;
    pCred = NULL;

error:

    NTLM_UNLOCK_CREDENTIALS();
    NTLMDereferenceCredential(pCred);

    return dwError;
}


#define NTLM_CREDENTIAL_MUTEX_INIT  0x1
#define NTLM_CREDENTIAL_USERDB_INIT 0x2
#define NTLM_CREDENTIAL_MASK_INIT 0xF

/*
 * NTLMCleanupCredentialSystem
 * 
 * @todo - This should only be called *after* all of the 
 * outstanding RPC calls have been made.
 */ 
void
NTLMCleanupCredentialSystem(DWORD dwInitFlags)
{
    if (!dwInitFlags)
        dwInitFlags = NTLM_CREDENTIAL_MASK_INIT;

    /* 
     * @todo - user db connection
     * if (dwInitFlags & NTLM_CREDENTIAL_USERDB_INIT)
     * cleanup
     */

    /* do this last to make sure noone is waiting */
    if (dwInitFlags & NTLM_CREDENTIAL_MUTEX_INIT)
        pthread_mutex_destroy(&g_CredentialMtx);

}

DWORD
NTLMInitializeCredentialSystem( void )
{
    DWORD dwError;
    DWORD dwInitialized = 0;
    pthread_mutexattr_t attr;

    dwError = pthread_mutexattr_init(&attr);
    if (dwError) {
        DBG(D_ERROR, ("Failed pthread attr init - %d\n", dwError));
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_INTERNAL);
    }

    dwError = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    if (dwError) {
        DBG(D_ERROR, ("Failed pthread attr set - %d\n", dwError));
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_INTERNAL);
    }

    dwError = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
    if (dwError) {
        DBG(D_ERROR, ("Failed pthread attr set - %d\n", dwError));
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_INTERNAL);
    }

    dwError = pthread_mutex_init(&g_CredentialMtx, &attr);
    if (dwError) {
        DBG(D_ERROR, ("Failed pthread init - %d\n", dwError));
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_INTERNAL);
    }

    dwInitialized |= NTLM_CREDENTIAL_MUTEX_INIT;


    /* @todo initilize connection to user db */
    dwInitialized |= NTLM_CREDENTIAL_USERDB_INIT;


    return LSA_ERROR_SUCCESS;

error:

    NTLMCleanupCredentialSystem(dwInitialized);

    return dwError;
}



/*
 * @brief NTLMUnmarshallSuppliedCreds
 *
 * Parse NTLMGSS_SUPPLIED_CRED structure.
 *
 * A properly formatted cred looks like this
 *
 *  @todo - alignment requirements
 * 
 *  *----------------------------*
 *  * base NTLMGSS_SUPPLIED_CRED *
 *  *----------------------------*
 *  * string 1                   *
 *  * string 2                   *
 *  * string 3                   *
 *  *----------------------------*
 *
 * @param  marshalledCreds - cred in a sec_buffer
 * @param uid_t - uid of presenter, used in auth_user
 * @param authUser - provided structure, free w/ NTLMFreeAuthUser()
 *
 * @returns error from buffer validation, allocation
 *
 * @NOTE - marshalledCreds will be converted to absolute form!
 *
 */ 
DWORD
NTLMUnmarshallSuppliedCreds(
    PSEC_BUFFER marshalledCreds,
    uid_t uid,
    PAUTH_USER pAuthUser
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    PNTLMGSS_SUPPLIED_CREDS suppliedCreds;

    if (marshalledCreds->length <= sizeof(NTLMGSS_SUPPLIED_CREDS))
        return LSA_ERROR_INSUFFICIENT_BUFFER;

    suppliedCreds = (PNTLMGSS_SUPPLIED_CREDS) marshalledCreds->buffer;

    /* validate buffers and repoint locally */
    if (!NTLMValidateMarshalledLsaString(
            (PBYTE) suppliedCreds,
            marshalledCreds->maxLength,
            &suppliedCreds->user
            )) {
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_INSUFFICIENT_BUFFER);
    }

    suppliedCreds->user.buffer = (wchar16_t*) OFFSET_TO_PTR(suppliedCreds,
        suppliedCreds->user.buffer);

    if (!NTLMValidateMarshalledLsaString(
            (PBYTE) suppliedCreds,
            marshalledCreds->maxLength,
            &suppliedCreds->domain
            )) {
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_INSUFFICIENT_BUFFER);
    }

    suppliedCreds->domain.buffer = (wchar16_t*) OFFSET_TO_PTR(suppliedCreds,
        suppliedCreds->domain.buffer);

    if (!NTLMValidateMarshalledLsaString(
            (PBYTE) suppliedCreds,
            marshalledCreds->maxLength,
            &suppliedCreds->password
            )) {
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_INSUFFICIENT_BUFFER);
    }

    suppliedCreds->password.buffer = (wchar16_t*) OFFSET_TO_PTR(suppliedCreds,
        suppliedCreds->password.buffer);

    /* build auth password */
    if(!NTLMInitializeAuthUser(
                &suppliedCreds->user,
                &suppliedCreds->domain,
                &suppliedCreds->password,
                uid,
                pAuthUser
                )) {
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_OUT_OF_MEMORY);
    }


error:


    return dwError;
}



/*
 * Server side GSS stubs
 */


DWORD
NTLMGssReleaseCred(
    PVOID pCred
    )
{

    DWORD dwError = LSA_ERROR_SUCCESS;
    PNTLM_CREDENTIAL cred = NULL;

    cred = NTLMValidateCredential(
                (PNTLM_CREDENTIAL) pCred,
                true
                );

    if (!cred) {
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_INVALID_CREDENTIAL);
    }

    NTLMDereferenceCredentialHandle(cred);

error:

    NTLMDereferenceCredential(cred);
    return dwError;
}

DWORD
NTLMGssAcquireSuppliedCred(
    DWORD          *pdwMinorStatus,
    PSEC_BUFFER     marshalledCredentials,
    DWORD           timeRequested,
    OID_SET        *pDesiredMechanisms,
    DWORD           credUsage,
    PVOID          *ppCredHandle,
    OID_SET       **ppActualMechanisms,
    DWORD          *ppTimeValid
    )
{
    DWORD dwError;
    DWORD flags = credUsage;
    PNTLM_CREDENTIAL pCred = NULL;
    AUTH_USER credentials;


    /*@todo - grok this from connection */
    uid_t uid = 0; 
    pid_t processId = 0; 

    if (marshalledCredentials) {
        /* unmarshall the supplied credential */
        dwError = NTLMUnmarshallSuppliedCreds(
                    marshalledCredentials,
                    uid,
                    &credentials
                    );

        BAIL_ON_NTLM_ERROR(dwError);
        flags |= NTLM_CREDENTIAL_USER_SUPPLIED;

    } else {

        /* time to pull it from the local db */
        dwError = NTLMGetAuthUserFromUid(
                        uid,
                        &credentials
                        );

        BAIL_ON_NTLM_ERROR(dwError);
    }

    dwError = NTLMAcquireCredentialHandle(
                    &credentials,
                    processId,
                    flags,
                    &pCred
                    );

    BAIL_ON_NTLM_ERROR(dwError);

    *ppCredHandle = pCred;

    /* 
     * note - handle count is for extra-process
     * refcount is used internally only
     */ 
    NTLMDereferenceCredential(pCred);
    *pdwMinorStatus = dwError;
    return LSA_ERROR_SUCCESS;

error:

    *pdwMinorStatus = dwError;
    NTLMDereferenceCredentialHandle(pCred);
    NTLMDereferenceCredential(pCred);

    return dwError;
}

DWORD
NTLMGssAcquireCred(
    DWORD          *pdwMinorStatus,
    DWORD           timeRequested,
    OID_SET        *pDesiredMechanisms,
    DWORD           credUsage,
    PVOID          *ppCredHandle,
    OID_SET       **ppActualMechanisms,
    DWORD          *ppTimeValid
    )
{

    return NTLMGssAcquireSuppliedCred(
                pdwMinorStatus,
                NULL, /* supplied cred */
                timeRequested,
                pDesiredMechanisms,
                credUsage, 
                ppCredHandle,
                ppActualMechanisms,
                ppTimeValid
                );
}

