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
 *        ntlmgsssrv.h
 *
 * Abstract:
 *
 * Server exports for GSS calls
 *
 *
 * Author: Todd Stecher (2007)
 *
 */
#ifndef _NTLM_SRV_H_
#define _NTLM_SRV_H_


/*
 * Server APIs - the LSASSD dispatch routines should call these
 */

DWORD
NTLMGssInitializeServer(
    void
    );

DWORD
NTLMGssBuildAuthenticateMessage(
    ULONG negFlags,
    uid_t uid,
    PSEC_BUFFER marshaledCredential,
    PSEC_BUFFER_S serverChallenge,
    PSEC_BUFFER targetInfo,
    PSEC_BUFFER pOutputToken,
    PSEC_BUFFER_S baseSessionKey
    );


DWORD
NTLMGssCheckAuthenticateMessage(
    ULONG negFlags,
    PSEC_BUFFER_S serverChallenge,
    PSEC_BUFFER targetInfo,
    PSEC_BUFFER authenticateMessageToken,
    PSEC_BUFFER_S baseSessionKey
    );

VOID
NTLMGssFreeSecBuffer(
    PSEC_BUFFER buf
    );

DWORD
NTLMGssTeardownServer(
    void
    );

#endif /* _NTLM_SRV_H_ */
