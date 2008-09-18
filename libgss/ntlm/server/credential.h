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
 *        credential.h
 *
 * Abstract:
 *
 *        Credential handle management functions.
 *
 * Author: Todd Stecher (2007)
 *
 */

#ifndef _CREDENTIAL_H_
#define _CREDENTIAL_H_


/*
 * Valid credential flags.
 */ 
#define NTLM_CREDENTIAL_INBOUND         0x000000001
#define NTLM_CREDENTIAL_OUTBOUND        0x000000002
#define NTLM_CREDENTIAL_BOTH            0x000000003

#define NTLM_CREDENTIAL_IS_LOCAL        0x000100000
#define NTLM_CREDENTIAL_USER_SUPPLIED   0x000200000

/* 
 * Keys are tacked onto the end of the structure 
 */ 
typedef struct _NTLM_CREDENTIAL {

    NTLM_LIST link;
    DWORD handleCount;
    DWORD refCount;
    DWORD flags;

    pid_t          processId;

    /* this is where we get / store userinfo */
    /* also used for secure channel validation */
    AUTH_USER authUser;

} NTLM_CREDENTIAL, *PNTLM_CREDENTIAL;

void
NTLMDumpCredential(
    DWORD lvl,
    NTLM_CREDENTIAL *pCred
    );

void
NTLMRemoveCredential(NTLM_CREDENTIAL *pCred);

void
NTLMInsertCredential(NTLM_CREDENTIAL *pCred);
  
bool
NTLMDereferenceCredential(NTLM_CREDENTIAL *pCred);

void
NTLMSafeDereferenceCredential(NTLM_CREDENTIAL **ppCred);

bool
NTLMDereferenceCredential(NTLM_CREDENTIAL *pCred);

void
NTLMReferenceCredential(NTLM_CREDENTIAL *pCred);

PNTLM_CREDENTIAL
NTLMAllocateCredential(
    PAUTH_USER authUser,
    pid_t processId,
    DWORD flags
    );

PNTLM_CREDENTIAL
NTLMLocateCredential(
    PAUTH_USER authUser,
    pid_t processId,
    DWORD flags,
    bool addHandleReference
    );


DWORD
NTLMAcquireCredentialHandle(
    PAUTH_USER credentials,
    pid_t processId,
    DWORD flags,
    PNTLM_CREDENTIAL *ppCredentialOut
    );

DWORD
NTLMInitializeCredentialSystem( void );

NTLM_CREDENTIAL*
NTLMValidateCredential(
    NTLM_CREDENTIAL *pCred, 
    bool addReference
    );

/*
 * Helper macros
 */ 
#define NTLM_CREDENTIAL_REF(_x_)        NTLMReferenceCredential(_x_)
#define NTLM_CREDENTIAL_DEREF(_x_)      NTLMSafeDereferenceCredential(_x_)
#endif
