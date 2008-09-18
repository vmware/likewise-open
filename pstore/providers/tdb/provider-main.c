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
 *  Copyright (C) Likewise Software. All rights reserved.
 *  
 *  Module Name:
 *
 *     provider-main.c
 *
 *  Abstract:
 *
 *        Likewise Password Storage (LWPS)
 *
 *        API to support TDB Password Storage
 *
 *  Authors: Gerald Carter <gcarter@likewisesoftware.com>
 *
 */

#include "lwps-utils.h"
#include "lwps/lwps.h"
#include "lwps-provider.h"
#include "provider-main_p.h"
#include "util_str.h"
#include "param.h"
#include "util_tdbkey.h"

#define TDB_CTX_HANDLE_MAGIC      0x38A2AD8E
#define CFG_PROVIDER_NAME         "tdb"

#define BAIL_ON_INVALID_TDB_CTX(hParam)				\
        if (!hParam || (hParam->magic != TDB_CTX_HANDLE_MAGIC)) {	\
		dwError = LWPS_ERROR_INVALID_HANDLE;			\
		BAIL_ON_LWPS_ERROR(dwError);				\
        }

static PCSTR gpszTDBProviderName = "likewise-tdb-password-provider";
LWPS_PROVIDER_FUNC_TABLE gTDBProviderAPITable;


VOID
FreeMachineAccountInfo(
	PMACHINE_ACCT_INFO pAcctInfo
	)
{
	if (!pAcctInfo)
		return;

	LWPS_SAFE_FREE_STRING(pAcctInfo->pszDomainName);
	LWPS_SAFE_FREE_STRING(pAcctInfo->pszDomainDnsName);
	LWPS_SAFE_FREE_STRING(pAcctInfo->pszHostName);
	LWPS_SAFE_FREE_STRING(pAcctInfo->pszDomainSID);
	LWPS_SAFE_FREE_STRING(pAcctInfo->pszMachineAccountName);
	LWPS_SAFE_FREE_STRING(pAcctInfo->pszMachineAccountPassword);

	LwpsFreeMemory(pAcctInfo);	

	return;	
}


VOID
FreePasswordInfoStruct(
	PLWPS_PASSWORD_INFO pInfo
	)
{
	if (!pInfo)
		return;

	LWPS_SAFE_FREE_MEMORY(pInfo->pwszDomainName);
	LWPS_SAFE_FREE_MEMORY(pInfo->pwszDnsDomainName);
	LWPS_SAFE_FREE_MEMORY(pInfo->pwszHostname);
	LWPS_SAFE_FREE_MEMORY(pInfo->pwszSID);
	LWPS_SAFE_FREE_MEMORY(pInfo->pwszMachineAccount);
	LWPS_SAFE_FREE_MEMORY(pInfo->pwszMachinePassword);

	LwpsFreeMemory(pInfo);
	

	return;	
}


/************************************************************
 ************************************************************/

static DWORD
ConvertPasswordInfoFromMb(
	PMACHINE_ACCT_INFO pAcctInfo,
	PLWPS_PASSWORD_INFO *ppInfo
	)
{
	DWORD dwError = LWPS_ERROR_INTERNAL;

	dwError = LwpsAllocateMemory(sizeof(LWPS_PASSWORD_INFO), (PVOID*)ppInfo);
	BAIL_ON_LWPS_ERROR(dwError);	

	dwError = LwpsMbsToWc16s(pAcctInfo->pszDomainName,
				 &(*ppInfo)->pwszDomainName);	
	BAIL_ON_LWPS_ERROR(dwError);

	dwError = LwpsMbsToWc16s(pAcctInfo->pszMachineAccountPassword,
				 &(*ppInfo)->pwszMachinePassword);	
	BAIL_ON_LWPS_ERROR(dwError);

	dwError = LwpsMbsToWc16s(pAcctInfo->pszDomainDnsName,
				 &(*ppInfo)->pwszDnsDomainName);
	BAIL_ON_LWPS_ERROR(dwError);

	dwError = LwpsMbsToWc16s(pAcctInfo->pszDomainSID,
				 &(*ppInfo)->pwszSID);
	BAIL_ON_LWPS_ERROR(dwError);

	dwError = LwpsMbsToWc16s(pAcctInfo->pszHostName,
				 &(*ppInfo)->pwszHostname);
	BAIL_ON_LWPS_ERROR(dwError);

	dwError = LwpsMbsToWc16s(pAcctInfo->pszMachineAccountName,
				 &(*ppInfo)->pwszMachineAccount);
	BAIL_ON_LWPS_ERROR(dwError);
	
	(*ppInfo)->last_change_time = pAcctInfo->tPwdClientModifyTimestamp;	
	(*ppInfo)->dwSchannelType = pAcctInfo->dwSchannelType;	
	
done:
	return dwError;	

error:
	FreePasswordInfoStruct(*ppInfo);

	goto done;	
}

/************************************************************
 ************************************************************/

static DWORD
ConvertPasswordInfoToMb(
	PLWPS_PASSWORD_INFO pInfo,
	PMACHINE_ACCT_INFO *ppAcctInfo
	)
{
	DWORD dwError = LWPS_ERROR_INTERNAL;

	dwError = LwpsAllocateMemory(sizeof(MACHINE_ACCT_INFO), 
				     (PVOID*)ppAcctInfo);
	BAIL_ON_LWPS_ERROR(dwError);

	dwError = LwpsWc16sToMbs(pInfo->pwszDomainName,
				 &(*ppAcctInfo)->pszDomainName);
	BAIL_ON_LWPS_ERROR(dwError);

	dwError = LwpsWc16sToMbs(pInfo->pwszMachinePassword,
				 &(*ppAcctInfo)->pszMachineAccountPassword);
	BAIL_ON_LWPS_ERROR(dwError);

	dwError = LwpsWc16sToMbs(pInfo->pwszDnsDomainName,
				 &(*ppAcctInfo)->pszDomainDnsName);
	BAIL_ON_LWPS_ERROR(dwError);

	dwError = LwpsWc16sToMbs(pInfo->pwszSID,
				 &(*ppAcctInfo)->pszDomainSID);
	BAIL_ON_LWPS_ERROR(dwError);

	dwError = LwpsWc16sToMbs(pInfo->pwszHostname,
				 &(*ppAcctInfo)->pszHostName);
	BAIL_ON_LWPS_ERROR(dwError);

	dwError = LwpsWc16sToMbs(pInfo->pwszMachineAccount,
		&(*ppAcctInfo)->pszMachineAccountName);
	BAIL_ON_LWPS_ERROR(dwError);

	(*ppAcctInfo)->tPwdClientModifyTimestamp = pInfo->last_change_time; 
	(*ppAcctInfo)->dwSchannelType = pInfo->dwSchannelType;
	
done:
	return dwError;	

error:
	FreeMachineAccountInfo(*ppAcctInfo);	

	goto done;	
}

/************************************************************
 ************************************************************/

static DWORD 
Tdb_ConfigStartSection(
	PCSTR    pszSectionName,
	PVOID    pData,
	PBOOLEAN pbSkipSection,
	PBOOLEAN pbContinue
	)
{
	DWORD dwError = 0;
	PCSTR pszProviderName = NULL;
	BOOLEAN bContinue = TRUE;
	BOOLEAN bSkipSection = FALSE;

	if (!StrnEqual(pszSectionName, LWPS_CFG_PROVIDER_TAG, 
		       strlen(LWPS_CFG_PROVIDER_TAG)))
	{
		bSkipSection = TRUE;
		goto done;
	}

	pszProviderName = pszSectionName + strlen(LWPS_CFG_PROVIDER_TAG);
	if (!StrEqual(pszProviderName, CFG_PROVIDER_NAME)) {
		bSkipSection = TRUE;
		goto done;
	}
	
	*pbSkipSection = bSkipSection;
	*pbContinue = bContinue;

done:
	return dwError;
}

/************************************************************
 ************************************************************/

DWORD
Tdb_ConfigNameValuePair(
	PCSTR    pszName,
	PCSTR    pszValue,
	PVOID    pData,
	PBOOLEAN pbContinue
	)
{
	DWORD dwError = LWPS_ERROR_INTERNAL;
	BOOLEAN bContinue = FALSE;

	BAIL_ON_INVALID_POINTER(pszName);
	BAIL_ON_INVALID_POINTER(pszValue);

	/* Path to secrets.tdb */

	if (StrEqual(pszName, "db path")) {
		dwError = TdbSetDbPath(pszValue);
		BAIL_ON_LWPS_ERROR(dwError);
	}

	bContinue = TRUE;
	dwError = LWPS_ERROR_SUCCESS;

error:
	*pbContinue = bContinue;

	return dwError;
}

/************************************************************
 ************************************************************/

DWORD
LwpsInitializeProvider(
    PCSTR pszConfigFilePath,
    PSTR* ppszName,
    PLWPS_PROVIDER_FUNC_TABLE* ppFnTable
    )
{
    DWORD dwError = LWPS_ERROR_INTERNAL;

    BAIL_IF_NOT_SUPERUSER(geteuid());

    dwError = TdbInitProviderParams();
    BAIL_ON_LWPS_ERROR(dwError);    

    if (!IsNullOrEmptyString(pszConfigFilePath)) {
	dwError = LwpsParseConfigFile(
		pszConfigFilePath,
		LWPS_CFG_OPTION_STRIP_ALL,
		&Tdb_ConfigStartSection,
		NULL,
		&Tdb_ConfigNameValuePair,
		NULL,
		NULL);
	BAIL_ON_LWPS_ERROR(dwError);
    }

    *ppszName = (PSTR)gpszTDBProviderName;
    *ppFnTable = &gTDBProviderAPITable;


cleanup:
    return dwError;

error:
    *ppszName = NULL;
    *ppFnTable = NULL;

    goto cleanup;
}

/************************************************************
 ************************************************************/

DWORD
TDB_OpenProvider(
    PHANDLE phProvider
    )
{
    DWORD dwError = LWPS_ERROR_INTERNAL;
    PTDB_PROVIDER_CONTEXT pContext = NULL;
    PSTR pszDbPath = NULL;    

    BAIL_IF_NOT_SUPERUSER(geteuid());
    BAIL_ON_INVALID_POINTER(phProvider);

    dwError = LwpsAllocateMemory(
                  sizeof(TDB_PROVIDER_CONTEXT),
                  (PVOID*)&pContext);
    BAIL_ON_LWPS_ERROR(dwError);

    pContext->magic = TDB_CTX_HANDLE_MAGIC;

    dwError = TdbGetDbPath(&pszDbPath);    
    BAIL_ON_LWPS_ERROR(dwError);

    pContext->pTdb = tdb_open(pszDbPath, 
			      0, 
			      TDB_DEFAULT, 
			      O_RDWR|O_CREAT, 
			      0600);
    if (pContext->pTdb == NULL) {
	    dwError = LWPS_ERROR_UNEXPECTED_DB_RESULT;	    
	    BAIL_ON_LWPS_ERROR(dwError);	    
    }

    *phProvider = (HANDLE)pContext;
    
    dwError = LWPS_ERROR_SUCCESS;

cleanup:
    return dwError;

error:
    if (pContext) {
	    if (pContext->pTdb) {
		    tdb_close(pContext->pTdb);
		    pContext->pTdb = NULL;		    
	    }
	    LwpsFreeMemory(pContext);
    }

    *phProvider = (HANDLE)NULL;

    goto cleanup;
}

/************************************************************
 ************************************************************/

DWORD
TDB_ReadPasswordByDomain(
    HANDLE hProvider,
    PCSTR  pszDomain,
    PLWPS_PASSWORD_INFO* ppInfo
    )
{
    DWORD dwError = LWPS_ERROR_INTERNAL;
    PTDB_PROVIDER_CONTEXT pContext = (PTDB_PROVIDER_CONTEXT)hProvider;
    PSTR pszMachineKey = NULL;
    PMACHINE_ACCT_INFO pAcctInfo = NULL;

    BAIL_IF_NOT_SUPERUSER(geteuid());
    BAIL_ON_INVALID_POINTER(ppInfo);

    BAIL_ON_INVALID_TDB_CTX(pContext);

    dwError = TdbFetchMachineAccountInfo(pContext, 
					 pszDomain,
					 &pAcctInfo);
    BAIL_ON_LWPS_ERROR(dwError);
    
    dwError = ConvertPasswordInfoFromMb(pAcctInfo,
					ppInfo);
    BAIL_ON_LWPS_ERROR(dwError);    

cleanup:
    if (pszMachineKey)
	    LwpsFreeMemory(pszMachineKey);    

    return dwError;

error:
    FreeMachineAccountInfo(pAcctInfo);    
    FreePasswordInfoStruct(*ppInfo);

    goto cleanup;
}

/************************************************************
 ************************************************************/


DWORD
TDB_ReadPasswordByHostName(
    HANDLE hProvider,
    PCSTR  pszDomainName,
    PLWPS_PASSWORD_INFO* ppInfo
    )
{
	DWORD dwError = LWPS_ERROR_NOT_IMPLEMENTED;
	PTDB_PROVIDER_CONTEXT pContext = (PTDB_PROVIDER_CONTEXT)hProvider;
	
	BAIL_IF_NOT_SUPERUSER(geteuid());
	BAIL_ON_INVALID_POINTER(ppInfo);

	BAIL_ON_INVALID_TDB_CTX(pContext);

error:
    return dwError;    
}

/************************************************************
 ************************************************************/

DWORD
TDB_WritePassword(
    HANDLE hProvider,
    PLWPS_PASSWORD_INFO pInfo
    )
{
    DWORD dwError = LWPS_ERROR_NOT_IMPLEMENTED;
    PTDB_PROVIDER_CONTEXT pContext = (PTDB_PROVIDER_CONTEXT)hProvider;    
    PMACHINE_ACCT_INFO pAcctInfo = NULL;

    BAIL_IF_NOT_SUPERUSER(geteuid());
    BAIL_ON_INVALID_POINTER(pInfo);

    BAIL_ON_INVALID_TDB_CTX(pContext);    

    dwError = ConvertPasswordInfoToMb(pInfo, &pAcctInfo);    
    BAIL_ON_LWPS_ERROR(dwError);

    dwError = TdbStoreMachineAccountInfo(pContext, 
					 pAcctInfo->pszDomainName,
					 pAcctInfo);
    BAIL_ON_LWPS_ERROR(dwError);    

cleanup:
    FreeMachineAccountInfo(pAcctInfo);

    return dwError;

error:    
    goto cleanup;
}

/************************************************************
 ************************************************************/

DWORD
TDB_DeleteAllEntries(
    HANDLE hProvider
    )
{
    DWORD dwError = LWPS_ERROR_NOT_IMPLEMENTED;
    PTDB_PROVIDER_CONTEXT pContext = (PTDB_PROVIDER_CONTEXT)hProvider;

    BAIL_IF_NOT_SUPERUSER(geteuid());

    BAIL_ON_INVALID_TDB_CTX(pContext);    

    /* This will have to be a TDB traversal but I'm leaving it 
       blank for now.   --jerry */

cleanup:
    return dwError;

error:
    goto cleanup;
}

/************************************************************
 ************************************************************/

VOID
TDB_FreePassword(
    PLWPS_PASSWORD_INFO pInfo
    )
{
	FreePasswordInfoStruct(pInfo);	
}

/************************************************************
 ************************************************************/

DWORD
TDB_CloseProvider(
    HANDLE hProvider
    )
{
    DWORD dwError = LWPS_ERROR_NOT_IMPLEMENTED;
    PTDB_PROVIDER_CONTEXT pContext = (PTDB_PROVIDER_CONTEXT)hProvider;

    BAIL_IF_NOT_SUPERUSER(geteuid());

    BAIL_ON_INVALID_TDB_CTX(pContext);

    if (pContext->pTdb) {
	    tdb_close(pContext->pTdb);
	    pContext->pTdb = NULL;
    }

    /* Make sure there is no valid infor in the hanble
       so it cannot be reused accidentally */

    memset(pContext, 0x0, sizeof(TDB_PROVIDER_CONTEXT));    

    LwpsFreeMemory(pContext);

    dwError = LWPS_ERROR_SUCCESS;

cleanup:
    return dwError;

error:
    goto cleanup;
}

/************************************************************
 ************************************************************/

DWORD
LwpsShutdownProvider(
    PSTR pszName,
    PLWPS_PROVIDER_FUNC_TABLE pFnTable
    )
{
    DWORD dwError = LWPS_ERROR_NOT_IMPLEMENTED;

    BAIL_IF_NOT_SUPERUSER(geteuid());

    dwError = TdbReleaseProviderParams();
    BAIL_ON_LWPS_ERROR(dwError);    
    
cleanup:
    return dwError;

error:
    goto cleanup;
}


/************************************************************
 Provider dispatch table
 ************************************************************/

LWPS_PROVIDER_FUNC_TABLE gTDBProviderAPITable = {
	&TDB_OpenProvider,
	&TDB_ReadPasswordByHostName,
	&TDB_ReadPasswordByDomain,
	&TDB_WritePassword,
	&TDB_DeleteAllEntries,
	&TDB_FreePassword,
	&TDB_CloseProvider
};
