/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
