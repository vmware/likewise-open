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
 *        gss.c
 *
 * Abstract:
 * 
 *        Likewise Security and Authentication Subsystem (LSASS)
 * 
 *        GSS client stub
 *
 * Authors: Todd Stecher (v-todds@likewise.com)
 *
 */
#include "client.h"

#define ZERO_STRUCT(_s_) memset((char*)&(_s_), 0, sizeof((_s_))
/*
 *  LsaGSSBuildAuthMessage
 *
 *  Builds NTLM authenticate message, which requires access to creds inside of
 *  lsassd.
 */
LSASS_API
DWORD
LsaGSSBuildAuthMessage(
    HANDLE          hLsaConnection,
    PSEC_BUFFER     credentials,
    PSEC_BUFFER_S   serverChallenge,
    PSEC_BUFFER     targetInfo,
    ULONG           negotiateFlags,
    PSEC_BUFFER     authenticateMessage,
    PSEC_BUFFER_S   baseSessionKey
    )
{
    DWORD dwError = 0;
    DWORD dwMsgLen = 0;
    DWORD dwMsgError = 0;
    PLSAMESSAGE pMessage = NULL;
    
    BAIL_ON_INVALID_HANDLE(hLsaConnection);
    BAIL_ON_INVALID_POINTER(credentials);
    BAIL_ON_INVALID_POINTER(baseSessionKey);
    BAIL_ON_INVALID_POINTER(serverChallenge);
    BAIL_ON_INVALID_POINTER(targetInfo);
    
    dwError = LsaMarshalGSSMakeAuthMsgQ(
                    credentials,
                    serverChallenge,
                    targetInfo,
                    negotiateFlags,
                    NULL,
                    &dwMsgLen
                    );

    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaBuildMessage(
                LSA_Q_GSS_MAKE_AUTH_MSG,
                dwMsgLen,
                1,
                1,
                &pMessage
                );

    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaMarshalGSSMakeAuthMsgQ(
                    credentials,
                    serverChallenge,
                    targetInfo,
                    negotiateFlags,
                    pMessage->pData,
                    &dwMsgLen
                    );

    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaSendMessage(hLsaConnection, pMessage);
    BAIL_ON_LSA_ERROR(dwError);
    
    LSA_SAFE_FREE_MESSAGE(pMessage);
    
    dwError = LsaGetNextMessage(hLsaConnection, &pMessage);
    BAIL_ON_LSA_ERROR(dwError);
        
    if (pMessage->header.messageType != LSA_R_GSS_MAKE_AUTH_MSG)
        BAIL_WITH_LSA_ERROR(LSA_ERROR_UNEXPECTED_MESSAGE);

    dwError = LsaUnMarshalGSSMakeAuthMsgR(
                    pMessage->pData,
                    pMessage->header.messageLength,
                    &dwMsgError,
                    authenticateMessage,
                    baseSessionKey
                    );

    BAIL_ON_LSA_ERROR(dwError);
    BAIL_ON_LSA_ERROR(dwMsgError);
    
error:

    if (dwMsgError)
        dwError = dwMsgError;

    LSA_SAFE_FREE_MESSAGE(pMessage);
    return dwError;
    
}


/*
 *  LsaGSSValidateAuthMessage
 *
 *  Validate NTLM authenticate message, which requires access to creds inside of
 *  lsassd.
 */

LSASS_API
DWORD
LsaGSSValidateAuthMessage(
    HANDLE          hLsaConnection,
    ULONG           negFlags,
    PSEC_BUFFER_S   serverChallenge,
    PSEC_BUFFER     targetInfo,
    PSEC_BUFFER     authenticateMessage,
    PSEC_BUFFER_S   baseSessionKey
    )
{
    DWORD dwError = 0;
    DWORD dwMsgLen = 0;
    DWORD dwMsgError = 0;
    PLSAMESSAGE pMessage = NULL;

    BAIL_ON_INVALID_HANDLE(hLsaConnection);
    BAIL_ON_INVALID_POINTER(authenticateMessage);
    BAIL_ON_INVALID_POINTER(baseSessionKey);
    
    dwError = LsaMarshalGSSCheckAuthMsgQ(
                    negFlags,
                    serverChallenge,
                    targetInfo,
                    authenticateMessage,
                    NULL,
                    &dwMsgLen
                    );

    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaBuildMessage(
                LSA_Q_GSS_CHECK_AUTH_MSG,
                dwMsgLen,
                1,
                1,
                &pMessage
                );

    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaMarshalGSSCheckAuthMsgQ(
                    negFlags,
                    serverChallenge,
                    targetInfo,
                    authenticateMessage,
                    pMessage->pData,
                    &dwMsgLen
                    );

    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaSendMessage(hLsaConnection, pMessage);
    BAIL_ON_LSA_ERROR(dwError);
    
    LSA_SAFE_FREE_MESSAGE(pMessage);
    
    dwError = LsaGetNextMessage(hLsaConnection, &pMessage);
    BAIL_ON_LSA_ERROR(dwError);
        
    if (pMessage->header.messageType != LSA_R_GSS_CHECK_AUTH_MSG)
        BAIL_WITH_LSA_ERROR(LSA_ERROR_UNEXPECTED_MESSAGE);

    dwError = LsaUnMarshalGSSCheckAuthMsgR(
                    pMessage->pData,
                    pMessage->header.messageLength,
                    &dwMsgError,
                    baseSessionKey
                    );

    BAIL_ON_LSA_ERROR(dwError);
    BAIL_ON_LSA_ERROR(dwMsgError);


error:

    if (dwMsgError)
        dwError = dwMsgError;

    LSA_SAFE_FREE_MESSAGE(pMessage);
    return dwError;
}

