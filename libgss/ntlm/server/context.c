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
 *        context.c
 *
 * Abstract:
 *
 *        Server context handle management functions.
 *
 * Author: Todd Stecher (2007)
 *
 */
#include "server.h"

NTLM_LIST *g_contextList;
pthread_mutex_t g_contextMtx;

void
NTLMDumpContext(
    DWORD lvl,
    NTLM_CONTEXT *pCtxt
    )
{

  if ((lvl & db_level) == 0)
        return;

  DBG(lvl,("context(0x%p) - %s\n", pCtxt, (pCtxt->flags & CONTEXT_OUTBOUND ?
              "outbound" : "inbound")));
  DBG(lvl,("link:0x%p:0x%p ref: %ld \n", pCtxt->link.prev, pCtxt->link.next, pCtxt->refCount));
  DBG(lvl,("Negflags: 0x%lx\n", pCtxt->negotiateFlags));


  /* @todo - dump out function table */
  /* @todo - dump out keys */
  /* @todo - dump out response message handler */
}

static void
NTLMRemoveContext(NTLM_CONTEXT *pCtxt)
{
    NTLM_LOCK_CONTEXTS();

    g_contextList = NTLMRemoveLink(g_contextList, &pCtxt->link);
    pCtxt->refCount--;

    NTLM_UNLOCK_CONTEXTS();
}

void
NTLMInsertContext(NTLM_CONTEXT *pCtxt)
{

    NTLM_LOCK_CONTEXTS();

    g_contextList = NTLMListPrepend(g_contextList, &pCtxt->link);
    pCtxt->refCount++;

    NTLM_UNLOCK_CONTEXTS();
}
   
static void
NTLMFreeContext(NTLM_CONTEXT *pCtxt)
{
    /* assert refCount == 0 */
    NTLM_SAFE_FREE(pCtxt);
}

static bool
NTLMDereferenceContext(NTLM_CONTEXT *pCtxt)
{
    if (!pCtxt)
	    return false;

    /* @todo interlocked version ? */
    NTLM_LOCK_CONTEXTS();
    if ( 0 == --pCtxt->refCount) {
        /* free */
        NTLMFreeContext(pCtxt);
        NTLM_UNLOCK_CONTEXTS();
        return true;
    }

    NTLM_UNLOCK_CONTEXTS();
    return false;
}

void
NTLMSafeDereferenceContext(NTLM_CONTEXT **ppCtxt)
{
    if (NTLMDereferenceContext(*ppCtxt))
        *ppCtxt = NULL;
}

void
NTLMReferenceContext(NTLM_CONTEXT *pCtxt)
{
    /* @todo interlocked? */
    NTLM_LOCK_CONTEXTS();
    pCtxt->refCount++;
    NTLM_UNLOCK_CONTEXTS();
}

PNTLM_CONTEXT
NTLMLocateContext( 
    PNTLM_CONTEXT pCtxt,
    PNTLM_CREDENTIAL pCredential,
    DWORD direction
    )
{
    NTLM_LIST *node;
    PNTLM_CONTEXT listCtxt = NULL;

    NTLM_LOCK_CONTEXTS();

    node = NTLMListFindNode(
            g_contextList, 
            &pCtxt->link
            );

    if (node) {

        listCtxt = (PNTLM_CONTEXT) node;

        if ((pCredential && pCredential != pCtxt->cred) ||
           ((direction & listCtxt->flags) == 0)) {

            listCtxt = NULL;
        }
        else
        {
            NTLMReferenceContext(listCtxt);
        }
    }

    NTLM_UNLOCK_CONTEXTS();
    return listCtxt;
}

BOOLEAN
NTLMCreateDummyContext(
    PNTLM_CONTEXT pDummy,
    PAUTH_USER user,
    ULONG negotiateFlags
)
{

    pDummy->cred = NTLMAllocateCredential(
                        user,
                        0,
                        NTLM_CREDENTIAL_BOTH
                        );

    if (pDummy->cred)
        return false;


    pDummy->negotiateFlags = negotiateFlags;

    return true;
}

    

DWORD
NTLMCreateContext(
    PNTLM_CREDENTIAL pCredential,
    DWORD direction,
    PNTLM_CONTEXT *ppContextOut
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    PNTLM_CONTEXT pCtxt = NULL;
   
    NTLM_LOCK_CONTEXTS();

    pCtxt = (PNTLM_CONTEXT) NTLMAllocateMemory(sizeof(NTLM_CONTEXT));
    if (!pCtxt)
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_OUT_OF_MEMORY);

    /* we need the credential handle to live alongside of context */

    /* assign default handler - e.g. negotiate message */
    if (direction & CONTEXT_INBOUND) 
        pCtxt->processNextMessage = NTLMProcessNegotiateMessage;
    else
        pCtxt->processNextMessage = NTLMBuildNegotiateMessage;

    /* other interesting flags? */
    pCtxt->flags = direction;

    NTLM_CREDENTIAL_REF(pCredential);
    pCtxt->cred = pCredential;

    /* insert it into the list */
    NTLMInsertContext(pCtxt);
    NTLMReferenceContext(pCtxt);
    *ppContextOut = pCtxt;
    pCtxt = NULL;

error:
    NTLM_UNLOCK_CONTEXTS();

    if (pCtxt)
        NTLMDereferenceContext(pCtxt);

    return dwError;
}


#define NTLM_CONTEXT_MUTEX_INIT  0x1
#define NTLM_CONTEXT_USERDB_INIT 0x2
#define NTLM_CONTEXT_HASHTABLE_INIT 0x4
#define NTLM_CONTEXT_MASK_INIT 0xF



/*
 * NTLMCleanupContextSystem
 * 
 * @todo - This should only be called *after* all of the 
 * outstanding RPC calls have been made.
 */ 
void
NTLMCleanupContextSystem(DWORD dwInitFlags)
{
    if (!dwInitFlags)
        dwInitFlags = NTLM_CONTEXT_MASK_INIT;

    /* do this last to make sure noone is waiting */
    if (dwInitFlags & NTLM_CONTEXT_MUTEX_INIT)
        pthread_mutex_destroy(&g_contextMtx);

}

DWORD
NTLMInitializeContextSystem( void )
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

    if (pthread_mutex_init(&g_contextMtx, &attr))
        return LSA_ERROR_INTERNAL;
	
    dwInitialized |= NTLM_CONTEXT_MUTEX_INIT;

    return LSA_ERROR_SUCCESS;

error:

    NTLMCleanupContextSystem(dwInitialized);

    return dwError;
}





/*
 *
 * CONTEXT APIS
 *
 */

DWORD
NTLMGssInitSecContext(
    DWORD          *pdwMinorStatus,
    PVOID           pCredential,
    PVOID          *pContext,
    PLSA_STRING     pTargetName,
    OID            *pMechType,
    DWORD           dwReqFlags,
    DWORD           dwReqTime,
    PSEC_BUFFER     channelBindings,
    PSEC_BUFFER     inputToken,
    OID           **ppActualMechType,
    PSEC_BUFFER    *ppOutputToken,
    DWORD          *ppFlags,
    DWORD          *ppTimeValid
    )
{

    DWORD dwError;
    DWORD msgError = LSA_ERROR_SUCCESS;
    NTLM_CREDENTIAL *pCred = NULL;
    NTLM_CONTEXT *pCtxt;
    SEC_BUFFER outputToken;

    ZERO_STRUCT(outputToken);
    
    if (!pContext)
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_INVALID_CONTEXT);

    pCtxt = (PNTLM_CONTEXT) (*pContext);

    /* locate, and reference credential handle */
    /* @todo - GSSAPI RFC doesn't require a valid cred handle */
    /* if (!pCred) NTLMCreateCredential() */
    pCred = NTLMValidateCredential(
                (NTLM_CREDENTIAL*) pCredential,
                true
                );

    if (NULL == pCred) {
        dwError = LSA_ERROR_INVALID_CREDENTIAL;
        BAIL_ON_NTLM_ERROR(dwError);
    }

    /* check credential usage */
    if ((pCred->flags & GSS_C_INITIATE) == 0)  {
        dwError = LSA_ERROR_INVALID_CREDENTIAL;
        BAIL_ON_NTLM_ERROR(dwError);
    }

    /* validate oid, possibly alter mech */

    /* validate secbuffer data */

    /* @todo validate uid, pid */

    /* @todo - what flags do we care about?*/

    /* if null context, create a new one */
    if (pCtxt == GSS_C_NO_CONTEXT) {

        dwError = NTLMCreateContext( 
                        pCred,
                        CONTEXT_OUTBOUND,
                        &pCtxt
                        );

        BAIL_ON_NTLM_ERROR(dwError);

    } else {

        pCtxt = NTLMLocateContext(
                    pCtxt,
                    pCred,
                    CONTEXT_OUTBOUND
                    );

        if (pCtxt == NULL) {
            dwError = LSA_ERROR_NO_CONTEXT;
            BAIL_ON_NTLM_ERROR(dwError);
        }
    }

    /* process message */
    msgError = pCtxt->processNextMessage(
                        pCtxt,
                        inputToken,
                        &outputToken
                        );

    if (msgError != LSA_WARNING_CONTINUE_NEEDED)
        BAIL_ON_NTLM_ERROR(msgError);

    /* copy output token */
    dwError = NTLMAllocTransferSecBuffer(
                    ppOutputToken,
                    &outputToken
                    );

    BAIL_ON_NTLM_ERROR(dwError);

    *pContext = pCtxt;
    NTLMDumpContext(D_ERROR, pCtxt); /*@todo - dumb this down to D_CTXT */

    /* @todo - finalized contexts will be exported to client lib */

error:

    if (dwError == LSA_ERROR_SUCCESS && msgError )
        dwError = msgError;

    NTLMFreeSecBuffer(&outputToken);
    NTLMDereferenceCredential(pCred);
    NTLMDereferenceContext(pCtxt);

    return dwError;
}

DWORD
NTLMGssAcceptSecContext(
    DWORD          *pdwMinorStatus,
    PVOID           pCredential,
    PVOID          *pContext,
    PSEC_BUFFER     inputToken,
    PSEC_BUFFER     inputChannelBindings,
    PLSA_STRING    *pSrcName,
    OID           **pMechType,
    PSEC_BUFFER    *ppOutputToken,
    DWORD          *pFlags,
    DWORD          *pTimeValid
    )
{
    DWORD dwError;
    DWORD msgError = LSA_ERROR_SUCCESS;
    NTLM_CREDENTIAL *pCred = NULL;
    NTLM_CONTEXT *pCtxt;
    SEC_BUFFER outputToken;

    ZERO_STRUCT(outputToken);

    if (!pContext)
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_INVALID_CONTEXT);

    pCtxt = (PNTLM_CONTEXT) (*pContext);

    /* locate, and reference credential handle */
    pCred = NTLMValidateCredential(
                (NTLM_CREDENTIAL*) pCredential,
                true
                );

    if (NULL == pCred) {
        dwError = LSA_ERROR_INVALID_CREDENTIAL;
        BAIL_ON_NTLM_ERROR(dwError);
    }

    /* check credential usage */
    if ((pCred->flags & GSS_C_ACCEPT) == 0) {
        dwError = LSA_ERROR_INVALID_CREDENTIAL;
        BAIL_ON_NTLM_ERROR(dwError);
    }

    /* validate oid, possibly alter mech */

    /* validate secbuffer data */

    /* @todo validate uid, pid */

    /* if null context, create a new one */
    if (*pContext == GSS_C_NO_CONTEXT) {

        dwError = NTLMCreateContext( 
                        pCred,
                        CONTEXT_INBOUND,
                        &pCtxt
                        );

        BAIL_ON_NTLM_ERROR(dwError);

    } else {

        pCtxt = NTLMLocateContext(
                    (PNTLM_CONTEXT) *pContext,
                    pCred,
                    CONTEXT_INBOUND
                    );

        if (pCtxt == NULL) {
            dwError = LSA_ERROR_NO_CONTEXT;
            BAIL_ON_NTLM_ERROR(dwError);
        }
    }

    /* process message */
    msgError = pCtxt->processNextMessage(
                    pCtxt,
                    inputToken,
                    &outputToken);

    if (msgError != LSA_WARNING_CONTINUE_NEEDED)
        BAIL_ON_NTLM_ERROR(msgError);

    /* finalized context */
    /* prepare context for export */
    /* @todo return src name the caller (domain\user) */
    /*if (dwError == LSA_ERROR_SUCCESS) {
        dwError = NTLMBuildSrcName(pCtxt);
        BAIL_ON_NTLM_ERROR(dwError);
    }*/

    /* copy output token */
    dwError = NTLMAllocTransferSecBuffer(
                    ppOutputToken,
                    &outputToken
                    );

    BAIL_ON_NTLM_ERROR(dwError);

    *pContext = pCtxt;
    NTLMDumpContext(D_ERROR, pCtxt); /*@todo - dumb this down to D_CTXT */

error:

    if (dwError == LSA_ERROR_SUCCESS && msgError)
        dwError = msgError;

    /*@todo set minor status ? Or just tranlsate in client */
    NTLMFreeSecBuffer(&outputToken);
    NTLMDereferenceCredential(pCred);
    NTLMDereferenceContext(pCtxt);

    return dwError;

}

DWORD
NTLMGssDeleteSecContext(
    DWORD          *pdwMinorStatus,
    PVOID           pContext,
    PSEC_BUFFER     pOutputToken
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    PNTLM_CONTEXT pCtxt = NULL;

    /* we don't use this (yet?) */
    ZERO_STRUCTP(pOutputToken);

    pCtxt = NTLMLocateContext( 
                pContext,
                NULL,
                CONTEXT_BOTH
                );

    if (!pCtxt)
        BAIL_WITH_NTLM_ERROR(LSA_ERROR_INVALID_CONTEXT);
    
    NTLMRemoveContext(pCtxt);

error:
    
    /* this should do it, unless its in use */
    if (pCtxt)
        NTLMDereferenceContext(pCtxt);

    return dwError;

}

ULONG
NTLMContextGetNegotiateFlags(
    PNTLM_CONTEXT pCtxt
)
{
    ULONG negFlags;
    NTLM_LOCK_CONTEXTS(pCtxt);
    negFlags = pCtxt->negotiateFlags;
    NTLM_UNLOCK_CONTEXTS(pCtxt);

    return negFlags;

}
