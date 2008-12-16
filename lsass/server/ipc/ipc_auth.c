/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        ipc_auth.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Inter-process communication (Server) API for Authentication
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "ipc.h"

LWMsgStatus
LsaSrvIpcAuthenticateUser(
    LWMsgAssoc* assoc,
    const LWMsgMessage* pRequest,
    LWMsgMessage* pResponse,
    void* data
    )
{
    DWORD dwError = 0;
    PLSA_IPC_AUTH_USER_REQ pReq = pRequest->object;
    PLSA_IPC_ERROR pError = NULL;

    dwError = LsaSrvAuthenticateUser(
                        (HANDLE)pReq->Handle,
                        pReq->pszLoginName,
                        pReq->pszPassword);

    if (!dwError)
    {
        pResponse->tag = LSA_R_AUTH_USER_SUCCESS;
        pResponse->object = NULL;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pResponse->tag = LSA_R_AUTH_USER_FAILURE;
        pResponse->object = pError;
    }

cleanup:
    return MAP_LSA_ERROR_IPC(dwError);

error:
    goto cleanup;
}

LWMsgStatus
LsaSrvIpcValidateUser(
    LWMsgAssoc* assoc,
    const LWMsgMessage* pRequest,
    LWMsgMessage* pResponse,
    void* data
    )
{
    DWORD dwError = 0;
    PLSA_IPC_AUTH_USER_REQ pReq = pRequest->object;
    PLSA_IPC_ERROR pError = NULL;

    dwError = LsaSrvValidateUser(
                        (HANDLE)pReq->Handle,
                        pReq->pszLoginName,
                        pReq->pszPassword);

    if (!dwError)
    {
        pResponse->tag = LSA_R_VALIDATE_USER_SUCCESS;
        pResponse->object = NULL;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pResponse->tag = LSA_R_VALIDATE_USER_FAILURE;
        pResponse->object = pError;
    }

cleanup:
    return MAP_LSA_ERROR_IPC(dwError);

error:
    goto cleanup;
}

LWMsgStatus
LsaSrvIpcCheckUserInList(
    LWMsgAssoc* assoc,
    const LWMsgMessage* pRequest,
    LWMsgMessage* pResponse,
    void* data
    )
{
    DWORD dwError = 0;
    PLSA_IPC_CHECK_USER_IN_LIST_REQ pReq = pRequest->object;
    PLSA_IPC_ERROR pError = NULL;

    dwError = LsaSrvCheckUserInList(
                        (HANDLE)pReq->Handle,
                        pReq->pszLoginName,
                        pReq->pszListName);

    if (!dwError)
    {
        pResponse->tag = LSA_R_CHECK_USER_IN_LIST_SUCCESS;
        pResponse->object = NULL;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pResponse->tag = LSA_R_CHECK_USER_IN_LIST_FAILURE;
        pResponse->object = pError;
    }

cleanup:
    return MAP_LSA_ERROR_IPC(dwError);

error:
    goto cleanup;
}

LWMsgStatus
LsaSrvIpcChangePassword(
    LWMsgAssoc* assoc,
    const LWMsgMessage* pRequest,
    LWMsgMessage* pResponse,
    void* data
    )
{
    DWORD dwError = 0;
    PLSA_IPC_CHANGE_PASSWORD_REQ pReq = pRequest->object;
    PLSA_IPC_ERROR pError = NULL;

    dwError = LsaSrvChangePassword(
                        (HANDLE)pReq->Handle,
                        pReq->pszLoginName,
                        pReq->pszOldPassword,
                        pReq->pszNewPassword);

    if (!dwError)
    {
        pResponse->tag = LSA_R_CHANGE_PASSWORD_SUCCESS;
        pResponse->object = NULL;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pResponse->tag = LSA_R_CHANGE_PASSWORD_FAILURE;
        pResponse->object = pError;
    }

cleanup:
    return MAP_LSA_ERROR_IPC(dwError);

error:
    goto cleanup;
}

/*********************************************************
 * The Extended Authenticate function.
 */

DWORD
LsaSrvIpcAuthenticateUserEx(
    LWMsgAssoc* assoc,
    const LWMsgMessage* pRequest,
    LWMsgMessage* pResponse,
    void* data
    )
{
	DWORD dwError = LSA_ERROR_NOT_IMPLEMENTED;
	PLSA_IPC_AUTH_USER_EX_REQ pReq = pRequest->object;
	LSA_AUTH_USER_PARAMS Params = {0};
	PLSA_AUTH_USER_INFO pUserInfo = NULL;
	PLSA_IPC_ERROR pError = NULL;

	BAIL_ON_LSA_ERROR(dwError);

	memset(&Params, 0x0, sizeof(Params));

	//memset(pUserInfo, 0x0, sizeof(UserInfo));

	/* Unmarshall the request */
    Params.AuthType = pReq->pParams->AuthType;
    Params.pass = pReq->pParams->pass;
    Params.pszAccountName = pReq->pParams->pszAccountName;
    Params.pszDomain = pReq->pParams->pszDomain;
    Params.pszWorkstation = pReq->pParams->pszWorkstation;

	/* Do the work  (obtain pUserInfo)*/

    if (!dwError)
    {
        pResponse->tag = LSA_R_AUTH_USER_EX_SUCCESS;
        pResponse->object = pUserInfo;
        pUserInfo = NULL;
    }
    else
    {
        dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pResponse->tag = LSA_R_AUTH_USER_EX_FAILURE;
        pResponse->object = pError;
    }

cleanup:
   LSA_SAFE_FREE_MEMORY(pUserInfo);

   return MAP_LSA_ERROR_IPC(dwError);

error:
   goto cleanup;
}

