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
 *        context.h
 *
 * Abstract:
 *
 *        Server side context management functions.
 *
 * Author: Todd Stecher (2007)
 *
 */
#ifndef _CONTEXT_H_
#define _CONTEXT_H_

#define CONTEXT_OUTBOUND        0x1
#define CONTEXT_INBOUND         0x2
#define CONTEXT_BOTH            0x3
typedef struct _NTLM_CONTEXT {
    NTLM_LIST link;
    /* mutex? */
    ULONG refCount;
    ULONG flags;
    NTLM_CREDENTIAL *cred; /* credential used - refcounted */

    ULONG negotiateFlags;

    UCHAR challenge[NTLM_CHALLENGE_LENGTH];
    SEC_BUFFER_S baseKey;

    /* next message we expect to receive on this context */
    DWORD (*processNextMessage) (
        struct _NTLM_CONTEXT* pCtxt, 
        PSEC_BUFFER pInputToken,
        PSEC_BUFFER pOutputToken );

} NTLM_CONTEXT, *PNTLM_CONTEXT;
    

/* 
 * @todo - some gss query context calls may need
 * to be stubbed into lsassd (e.g. getting
 * info on a context "in progress"
 */ 

/* Context management routines */

void
NTLMDumpContext(
    DWORD lvl,
    NTLM_CONTEXT *pCtxt
    );

void
NTLMInsertContext(NTLM_CONTEXT *pCtxt);


void
NTLMSafeDereferenceContext(NTLM_CONTEXT **ppCtxt);

void
NTLMReferenceContext(NTLM_CONTEXT *pCtxt);


PNTLM_CONTEXT
NTLMLocateContext( 
    PNTLM_CONTEXT pCtxt,
    PNTLM_CREDENTIAL pCredential,
    DWORD direction
    );


DWORD
NTLMCreateContext(
    PNTLM_CREDENTIAL pCredentialHandle,
    DWORD direction,
    PNTLM_CONTEXT *ppContextOut
    );

BOOLEAN
NTLMCreateDummyContext(
    PNTLM_CONTEXT pDummy,
    PAUTH_USER user,
    ULONG negotiateFlags
    );

void
NTLMCleanupContextSystem(DWORD dwInitFlags);

ULONG
NTLMContextGetNegotiateFlags(PNTLM_CONTEXT pCtxt);

DWORD
NTLMInitializeContextSystem( void );

/* locking */
extern pthread_mutex_t g_contextMtx;

#define NTLM_LOCK_CONTEXTS(_x_)      pthread_mutex_lock(&g_contextMtx);
#define NTLM_UNLOCK_CONTEXTS(_x_)    pthread_mutex_unlock(&g_contextMtx);
/* @todo - per context lock? */
#define NTLM_LOCK_CONTEXT(_x_)      pthread_mutex_lock(&g_contextMtx);
#define NTLM_UNLOCK_CONTEXT(_x_)    pthread_mutex_unlock(&g_contextMtx);


#endif /* _CONTEXT_H_ */




