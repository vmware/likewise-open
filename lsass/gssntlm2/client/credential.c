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

#include "client.h"


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
    DBG(lvl,("flags: 0x%x pid: %ld\n", pCred->flags, (long)pCred->processId));
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
    if (pCred->flags & NTLM_CREDENTIAL_USER_SUPPLIED)
        NTLM_SAFE_FREE(pCred->marshaledCredential.buffer);

    NTLM_SAFE_FREE(pCred);
}

BOOLEAN
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
    BOOLEAN addReference
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
    PSEC_BUFFER marshaledCredential,
    uid_t uid,
    pid_t processId,
    DWORD flags,
    BOOLEAN addHandleReference
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

        if (uid != pCred->uid)
            continue;

        if (pCred->flags != flags)
            continue;

        if (marshaledCredential)
        {
            if ((marshaledCredential->length !=
                    pCred->marshaledCredential.length) ||
                (memcmp(marshaledCredential->buffer,
                    pCred->marshaledCredential.buffer,
                    marshaledCredential->length)))
                continue;
        }

        /* found! */
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
    PSEC_BUFFER credentials,
    uid_t uid,
    pid_t processId,
    DWORD flags
    )
{
    PNTLM_CREDENTIAL pCred;
    DWORD dwError;

    pCred = NTLMAllocateMemory(sizeof(NTLM_CREDENTIAL));
    if (!pCred)
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_OUT_OF_MEMORY);

    if (credentials && credentials->length)
    {
        dwError = NTLMAllocCopySecBuffer(
                    &pCred->marshaledCredential,
                    credentials
                    );

        BAIL_ON_NTLM_ERROR(dwError);
        flags |= NTLM_CREDENTIAL_USER_SUPPLIED;
    }

    pCred->processId = processId;
    pCred->uid = uid;
    pCred->flags = flags;

    return pCred;

error:

    NTLM_SAFE_FREE(pCred);
    return NULL;
}

DWORD
NTLMAcquireCredentialHandle(
    PSEC_BUFFER marshaledCredential,
    uid_t uid,
    pid_t processId,
    DWORD flags,
    PNTLM_CREDENTIAL *ppCredentialOut
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    PNTLM_CREDENTIAL pCred = NULL;

    if (marshaledCredential && marshaledCredential->length)
        flags |= NTLM_CREDENTIAL_USER_SUPPLIED;

    NTLM_LOCK_CREDENTIALS();

    /* First check to see if we already have this cred */
    pCred = NTLMLocateCredential(
                marshaledCredential,
                uid,
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
                    marshaledCredential,
                    uid,
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


DWORD
NTLMGssReleaseCred(
    DWORD *minorStatus,
    PVOID credential
    )
{

    DWORD dwError = LSA_ERROR_SUCCESS;
    PNTLM_CREDENTIAL cred = NULL;

    cred = NTLMValidateCredential(
                (PNTLM_CREDENTIAL) credential,
                true
                );

    if (!cred)
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_INVALID_CREDENTIAL);

    /* remove reference from user mode caller */
    NTLMDereferenceCredentialHandle(cred);

    /* remove reference from above */
    /* this MAY delete handle if all user mode callers released */
    NTLMDereferenceCredential(cred);

error:

    *minorStatus = dwError;
    return dwError;
}

DWORD
NTLMGssAcquireSuppliedCred(
    DWORD          *pdwMinorStatus,
    PSEC_BUFFER     marshalledCredentials,
    uid_t           uid,
    DWORD           timeRequested,
    DWORD           credUsage,
    PVOID          *ppCredHandle,
    DWORD          *ppTimeValid
    )
{
    DWORD dwError;
    DWORD flags = credUsage;
    PNTLM_CREDENTIAL pCred = NULL;


    /*@todo - grok this from connection */
    pid_t processId = 0;


    dwError = NTLMAcquireCredentialHandle(
                    marshalledCredentials,
                    uid,
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
    uid_t           uid,
    DWORD           timeRequested,
    DWORD           credUsage,
    PVOID          *ppCredHandle,
    DWORD          *ppTimeValid
    )
{

    return NTLMGssAcquireSuppliedCred(
                pdwMinorStatus,
                NULL, /* supplied cred */
                uid,
                timeRequested,
                credUsage,
                ppCredHandle,
                ppTimeValid
                );
}



/*
 * @brief NTLMBuildSupplementalCredentials
 *
 * Useful routine for building a supplemental credentials
 * structure.
 *
 * @param username - duh!
 * @param domain - duh2!
 * @param password - duh3!
 * @param credBlob - user allocated sec_buffer, filled in by this routine.
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
 *
 * @returns errors due to validation, or alloc failure
 */
DWORD
NTLMBuildSupplementalCredentials(
    char *username,
    char *domain,
    char *password,
    PSEC_BUFFER credBlob
    )
{

    DWORD dwError;
    AUTH_USER authUser;
    uid_t uid = geteuid();

    LSA_STRING lsaUser;
    LSA_STRING lsaDomain;
    LSA_STRING lsaPassword;
    SEC_BUFFER creds;

    if (!username || !domain || !password) {
        return LSA_ERROR_INVALID_PARAMETER;
    }

    ZERO_STRUCT(lsaUser);
    ZERO_STRUCT(lsaDomain);
    ZERO_STRUCT(lsaPassword);
    ZERO_STRUCT(authUser);
    ZERO_STRUCT(creds);

    dwError = LsaInitializeLsaStringA(
                    username,
                    &lsaUser
                    );

    BAIL_ON_NTLM_ERROR(dwError);

    dwError = LsaInitializeLsaStringA(
                    domain,
                    &lsaDomain
                    );

    BAIL_ON_NTLM_ERROR(dwError);
    dwError = LsaInitializeLsaStringA(
                    password,
                    &lsaPassword
                    );

    BAIL_ON_NTLM_ERROR(dwError);
    dwError = NTLMInitializeAuthUser(
                    &lsaUser,
                    &lsaDomain,
                    &lsaPassword,
                    uid,
                    &authUser
                    );

    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NTLMMarshalAuthUser(
                    &authUser,
                    &creds
                    );

    BAIL_ON_NTLM_ERROR(dwError);

    memcpy(credBlob, &creds, sizeof(SEC_BUFFER));
    creds.buffer = NULL;

error:

    LsaFreeLsaString(&lsaUser);
    LsaFreeLsaString(&lsaDomain);

    /* @todo - safe password wiper */
    LsaFreeLsaString(&lsaPassword);
    NTLMFreeAuthUser(&authUser);
    NTLM_SAFE_FREE(creds.buffer);

    return dwError;
}

