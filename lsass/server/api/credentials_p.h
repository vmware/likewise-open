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
 *        credentials_p.h
 *
 * Abstract:
 *
 *        Private header file
 *
 * Authors:
 *
 */

#ifndef __CREDENTIALS_P_H__
#define __CREDENTIALS_P_H__

#include <lwdef.h>
#include <lwerror.h>
#include <lwsecurityidentifier.h>
#include <lsalist.h>

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

PLSA_CREDENTIALS
LsaFindCredByUid(
    IN DWORD dwUid
    );

DWORD
LsaCreateCred(
    IN PCSTR pszUserName,
    IN PCSTR pszPassword,
    IN OPTIONAL const PDWORD pdwUid,
    OUT PLSA_CREDENTIALS* ppCredential
    );

DWORD
LsaCredContains(
    PLSA_CREDENTIALS pCred,
    PCSTR pszUserName,
    PCSTR pszPassword
    );

VOID
LsaReleaseCredentialUnsafe(
    IN LSA_CRED_HANDLE hCredential
    );

#endif /* __CREDENTIALS_P_H__ */
