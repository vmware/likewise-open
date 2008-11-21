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
 *        auth.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Client Authentication API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "client.h"

LSASS_API
DWORD
LsaAuthenticateUser(
    HANDLE hLsaConnection,
    PCSTR  pszLoginName,
    PCSTR  pszPassword
    )
{
    DWORD dwError = 0;
    PLSAMESSAGE pMessage = NULL;
    DWORD   dwMsgLen = 0;
    PSTR    pszError = NULL;

    BAIL_ON_INVALID_HANDLE(hLsaConnection);
    BAIL_ON_INVALID_STRING(pszLoginName);

    dwError = LsaMarshalCredentials(
                    pszLoginName,
                    pszPassword,
                    NULL,
                    NULL,
                    &dwMsgLen);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaBuildMessage(
                LSA_Q_AUTH_USER,
                dwMsgLen,
                1,
                1,
                &pMessage);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaMarshalCredentials(
                    pszLoginName,
                    pszPassword,
                    NULL,
                    pMessage->pData,
                    &dwMsgLen);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSendMessage(hLsaConnection, pMessage);
    BAIL_ON_LSA_ERROR(dwError);

    LSA_SAFE_FREE_MESSAGE(pMessage);

    dwError = LsaGetNextMessage(hLsaConnection, &pMessage);
    BAIL_ON_LSA_ERROR(dwError);

    switch (pMessage->header.messageType) {
        case LSA_R_AUTH_USER:
        {
            break;
        }
        case LSA_ERROR:
        {
            DWORD dwSrvError = 0;

            //
            // TODO: Log error string if available
            //
            dwError = LsaUnmarshalError(
                          pMessage->pData,
                          pMessage->header.messageLength,
                          &dwSrvError,
                          &pszError
                          );
            BAIL_ON_LSA_ERROR(dwError);
            dwError = dwSrvError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        }
        default:
        {
            dwError = LSA_ERROR_UNEXPECTED_MESSAGE;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

cleanup:

    LSA_SAFE_FREE_MESSAGE(pMessage);
    LSA_SAFE_FREE_STRING(pszError);

    return dwError;

error:

    goto cleanup;
}

LSASS_API
DWORD
LsaValidateUser(
    HANDLE hLsaConnection,
    PCSTR  pszLoginName,
    PCSTR  pszPassword
    )
{
    DWORD dwError = 0;
    PLSAMESSAGE pMessage = NULL;
    DWORD   dwMsgLen = 0;
    PSTR    pszError = NULL;

    BAIL_ON_INVALID_HANDLE(hLsaConnection);
    BAIL_ON_INVALID_STRING(pszLoginName);

    dwError = LsaMarshalCredentials(
                    pszLoginName,
                    pszPassword,
                    NULL,
                    NULL,
                    &dwMsgLen);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaBuildMessage(
                LSA_Q_VALIDATE_USER,
                dwMsgLen,
                1,
                1,
                &pMessage);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaMarshalCredentials(
                    pszLoginName,
                    pszPassword,
                    NULL,
                    pMessage->pData,
                    &dwMsgLen);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSendMessage(hLsaConnection, pMessage);
    BAIL_ON_LSA_ERROR(dwError);

    LSA_SAFE_FREE_MESSAGE(pMessage);

    dwError = LsaGetNextMessage(hLsaConnection, &pMessage);
    BAIL_ON_LSA_ERROR(dwError);

    switch (pMessage->header.messageType) {
        case LSA_R_VALIDATE_USER:
        {
            break;
        }
        case LSA_ERROR:
        {
            DWORD dwSrvError = 0;

            //
            // TODO: Log error string if available
            //
            dwError = LsaUnmarshalError(
                          pMessage->pData,
                          pMessage->header.messageLength,
                          &dwSrvError,
                          &pszError
                          );
            BAIL_ON_LSA_ERROR(dwError);
            dwError = dwSrvError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        }
        default:
        {
            dwError = LSA_ERROR_UNEXPECTED_MESSAGE;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

cleanup:

    LSA_SAFE_FREE_MESSAGE(pMessage);
    LSA_SAFE_FREE_STRING(pszError);

    return dwError;

error:

    goto cleanup;
}

LSASS_API
DWORD
LsaCheckUserInList(
    HANDLE   hLsaConnection,
    PCSTR    pszLoginName,
    PCSTR    pszListName
    )
{
    DWORD   dwError = 0;
    PLSAMESSAGE pMessage = NULL;
    DWORD   dwMsgLen = 0;
    PSTR    pszError = NULL;

    BAIL_ON_INVALID_HANDLE(hLsaConnection);
    BAIL_ON_INVALID_STRING(pszLoginName);

    dwError = LsaMarshalCredentials(
                    pszLoginName,
                    pszListName,
                    NULL,
                    NULL,
                    &dwMsgLen);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaBuildMessage(
                    LSA_Q_CHECK_USER_IN_LIST,
                    dwMsgLen,
                    1,
                    1,
                    &pMessage);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaMarshalCredentials(
                    pszLoginName,
                    pszListName,
                    NULL,
                    pMessage->pData,
                    &dwMsgLen);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSendMessage(hLsaConnection, pMessage);
    BAIL_ON_LSA_ERROR(dwError);

    LSA_SAFE_FREE_MESSAGE(pMessage);

    dwError = LsaGetNextMessage(hLsaConnection, &pMessage);
    BAIL_ON_LSA_ERROR(dwError);

    switch (pMessage->header.messageType) {
        case LSA_R_CHECK_USER_IN_LIST:
        {
            break;
        }
        case LSA_ERROR:
        {
            DWORD dwSrvError = 0;

            //
            // TODO: Log error string if available
            //
            dwError = LsaUnmarshalError(
                          pMessage->pData,
                          pMessage->header.messageLength,
                          &dwSrvError,
                          &pszError
                          );
            BAIL_ON_LSA_ERROR(dwError);
            dwError = dwSrvError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        }
        default:
        {
            dwError = LSA_ERROR_UNEXPECTED_MESSAGE;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

cleanup:

    LSA_SAFE_FREE_MESSAGE(pMessage);
    LSA_SAFE_FREE_STRING(pszError);

    return dwError;

error:

    goto cleanup;
}

LSASS_API
DWORD
LsaChangePassword(
    HANDLE hLsaConnection,
    PCSTR  pszLoginName,
    PCSTR  pszNewPassword,
    PCSTR  pszOldPassword
    )
{
    DWORD dwError = 0;
    PLSAMESSAGE pMessage = NULL;
    DWORD   dwMsgLen = 0;
    PSTR    pszError = NULL;

    BAIL_ON_INVALID_HANDLE(hLsaConnection);
    BAIL_ON_INVALID_STRING(pszLoginName);

    dwError = LsaMarshalCredentials(
                    pszLoginName,
                    pszNewPassword,
                    pszOldPassword,
                    NULL,
                    &dwMsgLen);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaBuildMessage(
                LSA_Q_CHANGE_PASSWORD,
                dwMsgLen,
                1,
                1,
                &pMessage);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaMarshalCredentials(
                    pszLoginName,
                    pszNewPassword,
                    pszOldPassword,
                    pMessage->pData,
                    &dwMsgLen);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSendMessage(hLsaConnection, pMessage);
    BAIL_ON_LSA_ERROR(dwError);

    LSA_SAFE_FREE_MESSAGE(pMessage);

    dwError = LsaGetNextMessage(hLsaConnection, &pMessage);
    BAIL_ON_LSA_ERROR(dwError);

    switch (pMessage->header.messageType) {
        case LSA_R_CHANGE_PASSWORD:
        {
            break;
        }
        case LSA_ERROR:
        {
            DWORD dwSrvError = 0;

            //
            // TODO: Log error string if available
            //
            dwError = LsaUnmarshalError(
                          pMessage->pData,
                          pMessage->header.messageLength,
                          &dwSrvError,
                          &pszError
                          );
            BAIL_ON_LSA_ERROR(dwError);
            dwError = dwSrvError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        }
        default:
        {
            dwError = LSA_ERROR_UNEXPECTED_MESSAGE;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

cleanup:

    LSA_SAFE_FREE_MESSAGE(pMessage);
    LSA_SAFE_FREE_STRING(pszError);

    return dwError;

error:

    goto cleanup;
}



LSASS_API
DWORD
LsaAuthenticateUserEx(
	HANDLE hLsaConnection,
	LSA_AUTH_USER_PARAMS *pParams,
	LSA_AUTH_USER_INFO *pUserInfo
	)
{
    DWORD dwError = 0;
    PLSAMESSAGE pMessage = NULL;
    PSTR    pszError = NULL;
    LSA_AUTH_USER_INFO UserInfo;    

    BAIL_ON_INVALID_HANDLE(hLsaConnection);
    BAIL_ON_INVALID_POINTER(pParams);

    memset(&UserInfo, 0x0, sizeof(LSA_AUTH_USER_INFO));

    /* Marshall and send the request */

    dwError = LsaMarshallAuthenticateUserExQuery(
	    &pMessage,
	    pParams);    
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSendMessage(hLsaConnection, pMessage);
    BAIL_ON_LSA_ERROR(dwError);

    LSA_SAFE_FREE_MESSAGE(pMessage);

    /* Get the reply and check for success */

    dwError = LsaGetNextMessage(hLsaConnection, &pMessage);
    BAIL_ON_LSA_ERROR(dwError);

    switch (pMessage->header.messageType) {
    case LSA_R_AUTH_USER_EX:
    {
	    /* Success */
            break;
    }
    
    case LSA_ERROR:
    {
            DWORD dwSrvError = 0;

            dwError = LsaUnmarshalError(
		    pMessage->pData,
		    pMessage->header.messageLength,
		    &dwSrvError,
		    &pszError
		    );
            BAIL_ON_LSA_ERROR(dwError);
            dwError = dwSrvError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
    }

    default:
            dwError = LSA_ERROR_UNEXPECTED_MESSAGE;
            BAIL_ON_LSA_ERROR(dwError);
	    break;
    }

    /* Now unmarshall the response and fill in the data for the caller */
    /* Unfinished */

    dwError = LsaUnmarshallAuthenticateUserExReply(
	    pMessage->pData,
	    &pMessage->header.messageLength,
	    &UserInfo
	    );    
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LSA_SAFE_FREE_MESSAGE(pMessage);
    LSA_SAFE_FREE_STRING(pszError);

    return dwError;

error:

    goto cleanup;
}
