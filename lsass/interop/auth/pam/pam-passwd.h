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
 *        pam-passwd.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Pluggable Authentication Module.
 *
 *        Password API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __PAM_PASSWD_H__
#define __PAM_PASSWD_H__

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        pam-passwd.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Pluggable Authentication Module
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *
 */

int
pam_sm_chauthtok(
    pam_handle_t* pamh,
    int           flags,
    int           argc,
    const char**  argv
    );

DWORD
LsaPamCheckCurrentPassword(
    pam_handle_t* pamh,
    PPAMCONTEXT pPamContext
    );

DWORD
LsaPamMustCheckCurrentPassword(
    HANDLE   hLsaConnection,
    PCSTR    pszLoginId,
    PBOOLEAN pbCheckOldPassword);

DWORD
LsaPamUpdatePassword(
    pam_handle_t* pamh,
    PPAMCONTEXT   pPamContext
    );

DWORD
LsaPamGetCurrentPassword(
    pam_handle_t* pamh,
    PPAMCONTEXT   pPamContext,
    PSTR*         ppszPassword
    );

DWORD
LsaPamGetOldPassword(
    pam_handle_t* pamh,
    PPAMCONTEXT pPamContext,
    PSTR* ppszPassword
    );

DWORD
LsaPamGetNewPassword(
    pam_handle_t* pamh,
    PPAMCONTEXT   pPamContext,
    PSTR*         ppszPassword
    );

#endif /* __PAM_PASSWD_H__ */
