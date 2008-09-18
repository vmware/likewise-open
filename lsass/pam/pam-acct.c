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
 *        pam-acct.c
 *
 * Abstract:
 * 
 *        Likewise Security and Authentication Subsystem (LSASS)
 * 
 *        Pluggable Authentication Module
 *
 *        Account Management API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "pam-lsass.h"

/*
 * This is where we check if the password expired.
 * If the password is correct, but has expired, we return
 * PAM_NEW_AUTHTOK_REQD instead of PAM_SUCCESS
 */
int
pam_sm_acct_mgmt(
	pam_handle_t* pamh, 
	int           flags, 
	int           argc, 
	const char**  argv
	)
{
    DWORD dwError = 0;
    PPAMCONTEXT pPamContext = NULL;
    HANDLE hLsaConnection = (HANDLE)NULL;
    PLSA_USER_INFO_2 pUserInfo = NULL;
    DWORD dwUserInfoLevel = 2;
    PSTR  pszLoginId = NULL;
    
    LsaPamReadConfigFile();
    
    LSA_LOG_PAM_DEBUG("pam_sm_acct_mgmt::begin");
    
    dwError = LsaPamGetContext(
                    pamh,
                    flags,
                    argc,
                    argv,
                    &pPamContext);
    BAIL_ON_LSA_ERROR(dwError);
    
    if (pPamContext->bPasswordExpired)
    {
        // If during pam_sm_authenticate,
        // we detected that the password expired,
        // we handle it here
        pPamContext->bPasswordExpired = FALSE;
        dwError = LSA_ERROR_PASSWORD_EXPIRED;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    dwError = LsaPamGetLoginId(
                    pamh,
                    pPamContext,
                    &pszLoginId);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaValidateUser(
                    hLsaConnection,
                    pszLoginId,
                    NULL);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaFindUserByName(
                    hLsaConnection,
                    pszLoginId,
                    dwUserInfoLevel,
                    (PVOID*)&pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);
    
    if (pUserInfo->bPromptPasswordChange &&
        pUserInfo->dwDaysToPasswordExpiry) {
        
        CHAR szMessage[512];
        
        sprintf(szMessage, "Your password will expire in %d days\n",
               pUserInfo->dwDaysToPasswordExpiry);
        LsaPamConverse(pamh, szMessage, PAM_TEXT_INFO, NULL);
    }

cleanup:
   
    if (pUserInfo) {
        LsaFreeUserInfo(dwUserInfoLevel, (PVOID)pUserInfo);
    }
    
    if (hLsaConnection != (HANDLE)NULL)
    {
        LsaCloseServer(hLsaConnection);
    }

    LSA_SAFE_FREE_STRING(pszLoginId);
    
    LSA_LOG_PAM_DEBUG("pam_sm_acct_mgmt::end");

    return LsaPamMapErrorCode(dwError, pPamContext);
    
error:

    LSA_LOG_PAM_DEBUG("pam_sm_acct_mgmt failed [code:%d]", dwError);

    goto cleanup;
}
