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
 *        lsa_wbc.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 * 
 * Authors: Gerald Carter <gcarter@likewisesoftware.com>
 *
 */

#include "wbclient.h"
#include "lsawbclient_p.h"
#include <lsa/lsa.h>
#include "lsaclient.h"
#include "lsadatablob.h"

wbcErr wbcAuthenticateUser(const char *username,
			   const char *password)
{
	HANDLE hLsa = (HANDLE)NULL;	
	DWORD dwErr = LSA_ERROR_INTERNAL;
	wbcErr wbc_status = WBC_ERR_UNKNOWN_FAILURE;

	BAIL_ON_NULL_PTR_PARAM(username, dwErr);
	BAIL_ON_NULL_PTR_PARAM(password, dwErr);

	dwErr = LsaOpenServer(&hLsa);
	BAIL_ON_LSA_ERR(dwErr);

	dwErr = LsaAuthenticateUser(hLsa, username, password);
	BAIL_ON_LSA_ERR(dwErr);

	dwErr = LsaCloseServer(hLsa);
	hLsa = (HANDLE)NULL;
	BAIL_ON_LSA_ERR(dwErr);

done:
	if (hLsa) {
		LsaCloseServer(hLsa);
		hLsa = (HANDLE)NULL;
	}
	
	wbc_status = map_error_to_wbc_status(dwErr);

	return wbc_status;	
}


static int
FreeLsaAuthParams(void *p)
{
	PLSA_AUTH_USER_PARAMS pParams = (LSA_AUTH_USER_PARAMS *)p;

	if (!pParams) {		
		return 0;
	}

	_WBC_FREE(pParams->pszAccountName);
	_WBC_FREE(pParams->pszDomain);
	_WBC_FREE(pParams->pszWorkstation);

	switch (pParams->AuthType)
	{
	case LSA_AUTH_PLAINTEXT:
		_WBC_FREE(pParams->pass.clear.pszPassword);		
		break;
	case LSA_AUTH_CHAP:
		LsaDataBlobFree(&pParams->pass.chap.pChallenge);		
		LsaDataBlobFree(&pParams->pass.chap.pNT_resp);
		LsaDataBlobFree(&pParams->pass.chap.pLM_resp);		
		break;
	}
	
	return 0;
}


/******************************************************************
 */

static DWORD
CopyPlaintextParams(PLSA_AUTH_USER_PARAMS pLsaParams,
		    const struct wbcAuthUserParams *params)
{
	DWORD dwErr = LSA_ERROR_INTERNAL;
	PSTR pszPass = NULL;

	pszPass = _wbc_strdup(params->password.plaintext);
	BAIL_ON_NULL_PTR(pszPass, dwErr);	
	
	pLsaParams->pass.clear.pszPassword = pszPass;

done:
	return dwErr;
}

/******************************************************************
 */

static DWORD
CopyChapParams(PLSA_AUTH_USER_PARAMS pLsaParams,
	       const struct wbcAuthUserParams *params)
{
	DWORD dwErr = LSA_ERROR_INTERNAL;

	/* make sure we have at least one response */

	if ((params->password.response.nt_length == 0) &&
	    (params->password.response.lm_length == 0))
	{
		dwErr = LSA_ERROR_INVALID_PARAMETER;
		BAIL_ON_LSA_ERR(dwErr);
	}

	/* Challenge */

	dwErr = LsaDataBlobStore(&pLsaParams->pass.chap.pChallenge,
				 8,
				 (const PBYTE)params->password.response.challenge);
	BAIL_ON_LSA_ERR(dwErr);

	/* NT Response */

	dwErr = LsaDataBlobStore(&pLsaParams->pass.chap.pNT_resp,
				 params->password.response.nt_length,
				 (const PBYTE)params->password.response.nt_data );
	BAIL_ON_LSA_ERR(dwErr);

	/* LM Response */

	dwErr = LsaDataBlobStore(&pLsaParams->pass.chap.pLM_resp,
				 params->password.response.lm_length,
				 (const PBYTE)params->password.response.lm_data );
	BAIL_ON_LSA_ERR(dwErr);
	
done:
	if (!LSA_ERROR_IS_OK(dwErr)) {
		LsaDataBlobFree(&pLsaParams->pass.chap.pChallenge);		
		LsaDataBlobFree(&pLsaParams->pass.chap.pNT_resp);		
		LsaDataBlobFree(&pLsaParams->pass.chap.pLM_resp);		
	}
	
	return dwErr;
}

/******************************************************************
 */

static DWORD InitLsaAuthParams(PLSA_AUTH_USER_PARAMS pLsaParams,
			       const struct wbcAuthUserParams *params)
{
	DWORD dwErr = LSA_ERROR_INTERNAL;

	/* Check the auth level requested to validate input parms */

	switch (params->level)
	{
	case WBC_AUTH_USER_LEVEL_PLAIN:
		pLsaParams->AuthType = LSA_AUTH_PLAINTEXT;		
		break;
		
	case WBC_AUTH_USER_LEVEL_RESPONSE:
		pLsaParams->AuthType = LSA_AUTH_CHAP;		
		break;

	case WBC_AUTH_USER_LEVEL_HASH:
		dwErr = LSA_ERROR_NOT_IMPLEMENTED;
		BAIL_ON_LSA_ERR(dwErr);		
		break;

	default:
		dwErr = LSA_ERROR_INVALID_PARAMETER;
		BAIL_ON_LSA_ERR(dwErr);
	}

	/* Get the string data first */

	pLsaParams->pszAccountName = _wbc_strdup(params->account_name);
	BAIL_ON_NULL_PTR(pLsaParams->pszAccountName, dwErr);

	if (params->domain_name) {
		pLsaParams->pszDomain = _wbc_strdup(params->domain_name);
		BAIL_ON_NULL_PTR(pLsaParams->pszDomain, dwErr);
	}

	if (params->workstation_name) {
		pLsaParams->pszWorkstation = _wbc_strdup(params->workstation_name);
		BAIL_ON_NULL_PTR(pLsaParams->pszWorkstation, dwErr);
	}

	/* Now deal with the level specific parms */

	switch (pLsaParams->AuthType)
	{
	case LSA_AUTH_PLAINTEXT:
		dwErr = CopyPlaintextParams(pLsaParams, params);
		BAIL_ON_LSA_ERR(dwErr);		
		break;
		
	case LSA_AUTH_CHAP:
		dwErr = CopyChapParams(pLsaParams, params);
		BAIL_ON_LSA_ERR(dwErr);
		break;		
	}
	
done:
	if (!LSA_ERROR_IS_OK(dwErr)) {		
		/* Cleans up members bu leaves top level structure */
		FreeLsaAuthParams(pLsaParams);
	}
		
	return dwErr;
}

/******************************************************************
 */

static DWORD
CopyLsaUserInfoToWbcInfo(
	struct wbcAuthUserInfo *pWbcUserInfo,
	PLSA_AUTH_USER_INFO pUserInfo
	)
{
	return LSA_ERROR_NOT_IMPLEMENTED;	
}



/******************************************************************
 */

static PCSTR
BuildDomainAccountName(
	PCSTR pszDomain,
	PCSTR pszAccountName
	)
{
	PSTR pszFullname = NULL;
	DWORD dwLen = 0;

	if (!pszDomain || !pszAccountName) {
		return NULL;		
	}
	
	/* Include space for '\' and terminating NULL */

	dwLen = strlen(pszDomain) + strlen(pszAccountName) + 2;
	pszFullname = _wbc_malloc(dwLen, NULL);
	
	snprintf(pszFullname, dwLen, "%s\\%s", pszDomain, pszAccountName);

	return pszFullname;
}

/******************************************************************
 */

wbcErr wbcAuthenticateUserEx(const struct wbcAuthUserParams *params,
			     struct wbcAuthUserInfo **info,
			     struct wbcAuthErrorInfo **error)
{
	HANDLE hLsa = (HANDLE)NULL;	
	DWORD dwErr = LSA_ERROR_INTERNAL;	
	wbcErr wbc_status = WBC_ERR_UNKNOWN_FAILURE;
	LSA_AUTH_USER_PARAMS *pLsaParams = NULL;
	LSA_AUTH_USER_INFO *pLsaUserInfo = NULL;
	struct wbcAuthUserInfo *pWbcUserInfo = NULL;	

	/* Sanity and setup */

	BAIL_ON_NULL_PTR_PARAM(params, dwErr);
	BAIL_ON_NULL_PTR_PARAM(params->account_name, dwErr);

	pLsaParams = _wbc_malloc_zero(sizeof(LSA_AUTH_USER_PARAMS),
				      NULL);
	BAIL_ON_NULL_PTR(pLsaParams, dwErr);
	
	/* Open connection to the server and get moving */

	dwErr = LsaOpenServer(&hLsa);
	BAIL_ON_LSA_ERR(dwErr);

	dwErr = InitLsaAuthParams(pLsaParams, params);
	BAIL_ON_LSA_ERR(dwErr);
 
	switch (pLsaParams->AuthType)
	{
	case LSA_AUTH_PLAINTEXT:
	{
		PCSTR pszFullUsername = NULL;
		
		/* We need the fully qualified name here */

		pszFullUsername = BuildDomainAccountName(pLsaParams->pszDomain,
							 pLsaParams->pszAccountName);
		BAIL_ON_NULL_PTR(pszFullUsername, dwErr);

		dwErr = LsaAuthenticateUser(hLsa, 
					    pszFullUsername,
					    pLsaParams->pass.clear.pszPassword);
		_WBC_FREE(pszFullUsername);
		BAIL_ON_LSA_ERR(dwErr);		
		break;
	}
	
	case LSA_AUTH_CHAP:
	{		
		dwErr = LsaAuthenticateUserEx(hLsa, 
					      pLsaParams, 
					      &pLsaUserInfo);
		BAIL_ON_LSA_ERR(dwErr);
		break;
	}	
	}

	dwErr = LsaCloseServer(hLsa);
	hLsa = (HANDLE)NULL;
	BAIL_ON_LSA_ERR(dwErr);

	/* Copy the out parms now if we have an out pointer */

	if (!info || !pLsaUserInfo->pszAccount) {
		goto done;
	}
	
	pWbcUserInfo = _wbc_malloc_zero(sizeof(struct wbcAuthUserInfo),
					NULL);

	dwErr = CopyLsaUserInfoToWbcInfo(pWbcUserInfo, pLsaUserInfo);
	BAIL_ON_LSA_ERR(dwErr);	

done:
	if (hLsa) {
		LsaCloseServer(hLsa);
		hLsa = (HANDLE)NULL;
	}
	_WBC_FREE(pLsaParams);
	// FreeLsaAuthUserInfo(pLsaUserInfo);
	
	wbc_status = map_error_to_wbc_status(dwErr);

	return wbc_status;	
}

