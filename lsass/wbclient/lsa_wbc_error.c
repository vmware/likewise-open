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
 *        lsa_wbc_error.c
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
#include "lsaclient.h"
#include "lwnet.h"

struct _ErrorMap {
	DWORD dwError;
	wbcErr wbcError;
};

static struct _ErrorMap LsaErrorTable[] = {
	{ LSA_ERROR_SUCCESS, WBC_ERR_SUCCESS },
	{ LSA_ERROR_NOT_IMPLEMENTED, WBC_ERR_NOT_IMPLEMENTED },
	{ LSA_ERROR_INTERNAL, WBC_ERR_UNKNOWN_FAILURE },
	{ LSA_ERROR_OUT_OF_MEMORY, WBC_ERR_NO_MEMORY },
	{ LSA_ERROR_INVALID_SID, WBC_ERR_INVALID_SID },
	{ LSA_ERROR_INVALID_PARAMETER, WBC_ERR_INVALID_PARAM },
	{ LSA_ERROR_SERVICE_NOT_AVAILABLE, WBC_ERR_WINBIND_NOT_AVAILABLE },
	{ LSA_ERROR_NO_SUCH_DOMAIN, WBC_ERR_DOMAIN_NOT_FOUND },
	{ LSA_ERROR_INVALID_SERVICE_RESPONSE, WBC_ERR_INVALID_RESPONSE },
	{ LSA_ERROR_NSS_ERROR, WBC_ERR_NSS_ERROR },
	{ LSA_ERROR_AUTH_ERROR, WBC_ERR_AUTH_ERROR }
};

static struct _ErrorMap NetlogonErrorTable[] = {
	{ LWNET_ERROR_SUCCESS, WBC_ERR_SUCCESS },
	
};

static struct _ErrorMap WbcErrorTable[] = {
	{ LSA_ERROR_SUCCESS, WBC_ERR_SUCCESS },
	{ LSA_ERROR_NOT_IMPLEMENTED, WBC_ERR_NOT_IMPLEMENTED },
	{ LSA_ERROR_INTERNAL, WBC_ERR_UNKNOWN_FAILURE },
	{ LSA_ERROR_OUT_OF_MEMORY, WBC_ERR_NO_MEMORY },
	{ LSA_ERROR_INVALID_SID, WBC_ERR_INVALID_SID },
	{ LSA_ERROR_INVALID_PARAMETER, WBC_ERR_INVALID_PARAM },
	{ LSA_ERROR_SERVICE_NOT_AVAILABLE, WBC_ERR_WINBIND_NOT_AVAILABLE },
	{ LSA_ERROR_NO_SUCH_DOMAIN, WBC_ERR_DOMAIN_NOT_FOUND },
	{ LSA_ERROR_INVALID_SERVICE_RESPONSE, WBC_ERR_INVALID_RESPONSE },
	{ LSA_ERROR_NSS_ERROR, WBC_ERR_NSS_ERROR },
	{ LSA_ERROR_AUTH_ERROR, WBC_ERR_AUTH_ERROR }
};
	
static wbcErr map_lsa_to_wbc_error(DWORD err)
{
	int i = 0;
	size_t num_map_entries = sizeof(LsaErrorTable) / sizeof(struct _ErrorMap);

	for (i=0; i<num_map_entries; i++) {
		if (LsaErrorTable[i].dwError == err) {			
			return LsaErrorTable[i].wbcError;
		}
	}
	
	return WBC_ERR_UNKNOWN_FAILURE;	
}

static wbcErr map_netlogon_to_wbc_error(DWORD err)
{
	int i = 0;
	size_t num_map_entries = sizeof(NetlogonErrorTable) / sizeof(struct _ErrorMap);

	for (i=0; i<num_map_entries; i++) {
		if (NetlogonErrorTable[i].dwError == err) {			
			return NetlogonErrorTable[i].wbcError;
		}
	}
	
	return WBC_ERR_UNKNOWN_FAILURE;	
}

wbcErr map_error_to_wbc_status(DWORD err)
{
	if (err == 0)
		return WBC_ERR_SUCCESS;

	/* Use the correct mapping table */

	if (LWNET_ERROR_MASK(err) == LWNET_ERROR_MASK(LWNET_ERROR_SUCCESS))
		return map_netlogon_to_wbc_error(err);
	
	if (LSA_ERROR_MASK(err) == LSA_ERROR_MASK(LSA_ERROR_SUCCESS))
		return map_lsa_to_wbc_error(err);
	

	/* Return the default value generic error */

	return WBC_ERR_UNKNOWN_FAILURE;	
}


DWORD map_wbc_to_lsa_error(wbcErr err)
{
	int i = 0;
	size_t num_map_entries = sizeof(WbcErrorTable) / sizeof(struct _ErrorMap);

	for (i=0; i<num_map_entries; i++) {
		if (WbcErrorTable[i].wbcError == err) {			
			return WbcErrorTable[i].dwError;
		}
	}
	
	return LSA_ERROR_INTERNAL;	
}
