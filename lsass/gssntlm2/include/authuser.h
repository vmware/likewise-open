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
 *        authuser.h
 *
 * Abstract:
 *
 *        Basic credential structure
 *
 * Author: Todd Stecher (2007)
 *
 */
#ifndef _AUTHUSER_H_
#define _AUTHUSER_H_


typedef struct _AUTH_PROVIDER AUTH_PROVIDER, *PAUTH_PROVIDER;

typedef struct _AUTH_USER {
    LSA_STRING user;
    LSA_STRING domain;
    uid_t   uid;
    NTLM_OWF ntOWF; /* optional - for explicit creds */
    DWORD dwFlags;
    PAUTH_PROVIDER provider;
} AUTH_USER, *PAUTH_USER;

/*
 * AUTH_USER flags
 */
#define AUTH_USER_PASSWORD_SUPPLIED     0x00000001

DWORD
NTLMUnMarshalAuthUser(
    PSEC_BUFFER marshaledData,
    PAUTH_USER authUser
    );

DWORD
NTLMMarshalAuthUser(
    PAUTH_USER authUser,
    PSEC_BUFFER marshaledData
    );

DWORD
NTLMInitializeAuthUser(
    PLSA_STRING user,
    PLSA_STRING domain,
    PLSA_STRING password,
    uid_t uid,
    PAUTH_USER newAuthUser
    );

DWORD
NTLMCopyAuthUser(
    PAUTH_USER dest,
    PAUTH_USER src
    );

void
NTLMFreeAuthUser(
    PAUTH_USER authUser
    );

BOOLEAN
NTLMCompareAuthUsers(
    AUTH_USER *u1,
    AUTH_USER *u2
    );

#endif /*_AUTHUSER_H_*/
